#include "drawing_program/drawing_program_runtime_orchestration.h"

#include <string.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_history.h"
#include "drawing_program/drawing_program_object_rasterize.h"

static CoreResult orchestration_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static CoreResult set_active_tool(DrawingProgramAppContext *ctx, DrawingProgramToolKind tool) {
    if (!ctx) {
        return orchestration_invalid("null app context for set tool");
    }
    if (ctx->editor.active_tool != tool) {
        ctx->editor.active_tool = tool;
        ctx->tool_switch_total += 1u;
    }
    return core_result_ok();
}

static CoreResult find_layer_index(const DrawingProgramAppContext *ctx, uint32_t layer_id, uint32_t *out_index) {
    if (!ctx) {
        return orchestration_invalid("null app context for layer index query");
    }
    return drawing_program_document_layer_index_for_id(&ctx->document, layer_id, out_index);
}

static CoreResult sync_layer_raster_bindings(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return orchestration_invalid("null app context for layer raster sync");
    }
    return drawing_program_layer_raster_store_sync_document_layers(&ctx->layer_rasters, &ctx->document);
}

static CoreResult set_active_layer_id_internal(DrawingProgramAppContext *ctx, uint32_t layer_id) {
    CoreResult result;
    uint32_t ignored_index = 0u;
    if (!ctx || layer_id == 0u) {
        return orchestration_invalid("invalid set active layer request");
    }
    result = find_layer_index(ctx, layer_id, &ignored_index);
    if (result.code != CORE_OK) {
        return result;
    }
    ctx->editor.active_layer_id = layer_id;
    return core_result_ok();
}

CoreResult drawing_program_runtime_orchestration_set_active_layer_id(
    DrawingProgramAppContext *ctx,
    uint32_t layer_id) {
    return set_active_layer_id_internal(ctx, layer_id);
}

CoreResult drawing_program_runtime_orchestration_resolve_active_layer(
    const DrawingProgramAppContext *ctx,
    uint32_t *out_layer_id,
    uint32_t *out_layer_index,
    uint8_t *out_visible,
    uint8_t *out_locked) {
    uint32_t index = 0u;
    CoreResult result;
    if (!ctx) {
        return orchestration_invalid("null app context for active layer resolve");
    }
    if (!out_layer_id && !out_layer_index && !out_visible && !out_locked) {
        return orchestration_invalid("no output requested for active layer resolve");
    }
    if (ctx->editor.active_layer_id == 0u) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "active layer not set" };
    }
    result = find_layer_index(ctx, ctx->editor.active_layer_id, &index);
    if (result.code != CORE_OK) {
        return result;
    }
    if (out_layer_id) {
        *out_layer_id = ctx->document.layers[index].layer_id;
    }
    if (out_layer_index) {
        *out_layer_index = index;
    }
    if (out_visible) {
        *out_visible = ctx->document.layers[index].visible;
    }
    if (out_locked) {
        *out_locked = ctx->document.layers[index].locked;
    }
    return core_result_ok();
}

static int active_layer_allows_edits(const DrawingProgramAppContext *ctx, uint32_t *out_layer_index) {
    uint32_t index = 0u;
    CoreResult result;
    if (!ctx || ctx->editor.active_layer_id == 0u) {
        return 0;
    }
    result = find_layer_index(ctx, ctx->editor.active_layer_id, &index);
    if (result.code != CORE_OK) {
        return 0;
    }
    if (out_layer_index) {
        *out_layer_index = index;
    }
    return (ctx->document.layers[index].visible && !ctx->document.layers[index].locked) ? 1 : 0;
}

static CoreResult cycle_active_layer(DrawingProgramAppContext *ctx, int direction) {
    uint32_t index = 0u;
    CoreResult result;
    if (!ctx || ctx->document.layer_count == 0u) {
        return orchestration_invalid("invalid active layer cycle request");
    }
    result = find_layer_index(ctx, ctx->editor.active_layer_id, &index);
    if (result.code != CORE_OK) {
        ctx->editor.active_layer_id = ctx->document.layers[0].layer_id;
        return core_result_ok();
    }
    if (direction < 0) {
        index = (index == 0u) ? (ctx->document.layer_count - 1u) : (index - 1u);
    } else {
        index = (index + 1u) % ctx->document.layer_count;
    }
    ctx->editor.active_layer_id = ctx->document.layers[index].layer_id;
    return core_result_ok();
}

static CoreResult stamp_center_sample(DrawingProgramAppContext *ctx) {
    uint32_t sx;
    uint32_t sy;
    uint32_t active_index = 0u;
    uint8_t value = drawing_program_color_value_from_index(
        ctx ? drawing_program_color_index_clamp(ctx->ui_active_color_index) : drawing_program_color_default_index());
    if (!ctx) {
        return orchestration_invalid("null app context for stamp center sample");
    }
    if (!active_layer_allows_edits(ctx, &active_index)) {
        (void)active_index;
        return core_result_ok();
    }
    sx = ctx->document.raster_width > 0u ? (ctx->document.raster_width / 2u) : 0u;
    sy = ctx->document.raster_height > 0u ? (ctx->document.raster_height / 2u) : 0u;
    switch (ctx->editor.active_tool) {
        case DRAWING_PROGRAM_TOOL_ERASER:
            value = drawing_program_color_eraser_value();
            break;
        case DRAWING_PROGRAM_TOOL_BRUSH:
        case DRAWING_PROGRAM_TOOL_FILL:
        case DRAWING_PROGRAM_TOOL_LINE:
        case DRAWING_PROGRAM_TOOL_RECT:
        case DRAWING_PROGRAM_TOOL_CIRCLE:
        case DRAWING_PROGRAM_TOOL_SELECT:
        case DRAWING_PROGRAM_TOOL_MOVE:
        case DRAWING_PROGRAM_TOOL_PICKER:
        case DRAWING_PROGRAM_TOOL_PATH:
        default:
            value = drawing_program_color_value_from_index(drawing_program_color_index_clamp(ctx->ui_active_color_index));
            break;
    }
    return drawing_program_history_apply_set_sample_value(&ctx->history,
                                                          &ctx->document,
                                                          &ctx->layer_rasters,
                                                          ctx->editor.active_layer_id,
                                                          sx,
                                                          sy,
                                                          value);
}

static CoreResult clear_canvas_samples(DrawingProgramAppContext *ctx) {
    uint8_t clear_value;
    uint32_t slot;
    uint32_t active_index = 0u;
    CoreResult result;
    if (!ctx) {
        return orchestration_invalid("null app context for clear canvas");
    }
    clear_value = drawing_program_color_eraser_value();
    if (ctx->document.raster_sample_count > 0u) {
        memset(ctx->document.raster_samples, (int)clear_value, (size_t)ctx->document.raster_sample_count);
    }
    if (ctx->layer_rasters.slot_samples &&
        ctx->layer_rasters.sample_count == ctx->document.raster_sample_count &&
        ctx->layer_rasters.sample_count > 0u) {
        for (slot = 0u; slot < ctx->layer_rasters.slot_capacity; ++slot) {
            if (ctx->layer_rasters.slot_layer_ids[slot] == 0u) {
                continue;
            }
            memset(ctx->layer_rasters.slot_samples + ((size_t)slot * (size_t)ctx->layer_rasters.sample_count),
                   (int)clear_value,
                   (size_t)ctx->layer_rasters.sample_count);
        }
    }
    result = find_layer_index(ctx, ctx->editor.active_layer_id, &active_index);
    if (result.code != CORE_OK && ctx->document.layer_count > 0u) {
        ctx->editor.active_layer_id = ctx->document.layers[0].layer_id;
        active_index = 0u;
        result = core_result_ok();
    }
    if (result.code == CORE_OK && active_index < ctx->document.layer_count) {
        ctx->document.layers[active_index].visible = 1u;
        ctx->document.layers[active_index].locked = 0u;
    }
    drawing_program_selection_reset(&ctx->selection);
    return core_result_ok();
}

static CoreResult rasterize_selected_objects(DrawingProgramAppContext *ctx) {
    uint32_t active_index = 0u;
    CoreResult result;
    if (!ctx) {
        return orchestration_invalid("null app context for rasterize objects");
    }
    if (!active_layer_allows_edits(ctx, &active_index) || active_index >= ctx->document.layer_count) {
        return core_result_ok();
    }
    if (ctx->editor.active_layer_id == 0u || ctx->object_selection.count == 0u) {
        return core_result_ok();
    }
    result = drawing_program_object_rasterize_selection_to_layer(&ctx->document,
                                                                 &ctx->layer_rasters,
                                                                 &ctx->history,
                                                                 &ctx->object_store,
                                                                 &ctx->object_selection,
                                                                 ctx->editor.active_layer_id,
                                                                 0);
    if (result.code != CORE_OK) {
        return result;
    }
    return core_result_ok();
}

CoreResult drawing_program_runtime_orchestration_apply_workflow_control(
    DrawingProgramAppContext *ctx,
    DrawingProgramWorkflowControl control) {
    uint32_t i;
    if (!ctx) {
        return orchestration_invalid("null app context for workflow control");
    }
    switch (control) {
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_BRUSH:
            return set_active_tool(ctx, DRAWING_PROGRAM_TOOL_BRUSH);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_ERASER:
            return set_active_tool(ctx, DRAWING_PROGRAM_TOOL_ERASER);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_FILL:
            return set_active_tool(ctx, DRAWING_PROGRAM_TOOL_FILL);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_LINE:
            return set_active_tool(ctx, DRAWING_PROGRAM_TOOL_LINE);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_RECT:
            return set_active_tool(ctx, DRAWING_PROGRAM_TOOL_RECT);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_CIRCLE:
            return set_active_tool(ctx, DRAWING_PROGRAM_TOOL_CIRCLE);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_SELECT:
            return set_active_tool(ctx, DRAWING_PROGRAM_TOOL_SELECT);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_MOVE:
            return set_active_tool(ctx, DRAWING_PROGRAM_TOOL_MOVE);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_PICKER:
            return set_active_tool(ctx, DRAWING_PROGRAM_TOOL_PICKER);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_PATH:
            return set_active_tool(ctx, DRAWING_PROGRAM_TOOL_PATH);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_ADD_LAYER: {
            uint32_t layer_id = 0u;
            CoreResult result = drawing_program_document_add_layer(&ctx->document, 0, &layer_id);
            if (result.code != CORE_OK) {
                return result;
            }
            result = sync_layer_raster_bindings(ctx);
            if (result.code != CORE_OK) {
                return result;
            }
            drawing_program_selection_reset(&ctx->selection);
            return set_active_layer_id_internal(ctx, layer_id);
        }
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_DELETE_ACTIVE_LAYER: {
            uint32_t removed_index = 0u;
            uint32_t next_active_index = 0u;
            CoreResult result;
            if (ctx->editor.active_layer_id == 0u) {
                return (CoreResult){ CORE_ERR_NOT_FOUND, "active layer not set" };
            }
            result = drawing_program_document_remove_layer(&ctx->document,
                                                           ctx->editor.active_layer_id,
                                                           &removed_index);
            if (result.code != CORE_OK) {
                return result;
            }
            result = sync_layer_raster_bindings(ctx);
            if (result.code != CORE_OK) {
                return result;
            }
            if (ctx->document.layer_count == 0u) {
                ctx->editor.active_layer_id = 0u;
            } else {
                next_active_index = (removed_index >= ctx->document.layer_count)
                                        ? (ctx->document.layer_count - 1u)
                                        : removed_index;
                ctx->editor.active_layer_id = ctx->document.layers[next_active_index].layer_id;
                ctx->document.layers[next_active_index].visible = 1u;
            }
            drawing_program_selection_reset(&ctx->selection);
            return core_result_ok();
        }
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_PREV:
            return cycle_active_layer(ctx, -1);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_NEXT:
            return cycle_active_layer(ctx, 1);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_UP:
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_DOWN: {
            int direction = (control == DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_UP) ? 1 : -1;
            uint32_t moved_index = 0u;
            CoreResult result;
            if (ctx->editor.active_layer_id == 0u) {
                return (CoreResult){ CORE_ERR_NOT_FOUND, "active layer not set" };
            }
            result = drawing_program_document_move_layer(&ctx->document,
                                                         ctx->editor.active_layer_id,
                                                         direction,
                                                         &moved_index);
            if (result.code != CORE_OK) {
                return result;
            }
            return sync_layer_raster_bindings(ctx);
        }
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY:
            if (ctx->editor.active_layer_id == 0u) {
                return (CoreResult){ CORE_ERR_NOT_FOUND, "active layer not set" };
            }
            for (i = 0u; i < ctx->document.layer_count; ++i) {
                if (ctx->document.layers[i].layer_id == ctx->editor.active_layer_id) {
                    return drawing_program_history_apply_set_layer_visibility(&ctx->history,
                                                                             &ctx->document,
                                                                             ctx->editor.active_layer_id,
                                                                             (uint8_t)!ctx->document.layers[i].visible);
                }
            }
            return (CoreResult){ CORE_ERR_NOT_FOUND, "active layer not found" };
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_LOCK:
            if (ctx->editor.active_layer_id == 0u) {
                return (CoreResult){ CORE_ERR_NOT_FOUND, "active layer not set" };
            }
            for (i = 0u; i < ctx->document.layer_count; ++i) {
                if (ctx->document.layers[i].layer_id == ctx->editor.active_layer_id) {
                    return drawing_program_document_set_layer_locked(&ctx->document,
                                                                     ctx->editor.active_layer_id,
                                                                     (uint8_t)!ctx->document.layers[i].locked,
                                                                     0);
                }
            }
            return (CoreResult){ CORE_ERR_NOT_FOUND, "active layer not found" };
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_UNDO:
            return drawing_program_history_undo(&ctx->history, &ctx->document, &ctx->layer_rasters, &ctx->object_store);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_REDO:
            return drawing_program_history_redo(&ctx->history, &ctx->document, &ctx->layer_rasters, &ctx->object_store);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS:
            return clear_canvas_samples(ctx);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_HISTORY:
            drawing_program_history_clear(&ctx->history);
            return core_result_ok();
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_RASTERIZE_SELECTED_OBJECTS:
            return rasterize_selected_objects(ctx);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE:
            return stamp_center_sample(ctx);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_NONE:
        default:
            return core_result_ok();
    }
}

CoreResult drawing_program_runtime_orchestration_plan_frame(
    const DrawingProgramInputEventRaw *raw,
    DrawingProgramInputEventNormalized *out_normalized,
    DrawingProgramInputRouteResult *out_route,
    DrawingProgramInputInvalidationResult *out_invalidation) {
    uint32_t bits = 0u;
    if (!raw || !out_normalized || !out_route || !out_invalidation) {
        return orchestration_invalid("null orchestration plan frame argument");
    }

    memset(out_normalized, 0, sizeof(*out_normalized));
    out_normalized->has_quit_action = raw->quit_event_count > 0u ? 1u : 0u;
    out_normalized->has_window_action = raw->window_event_count > 0u ? 1u : 0u;
    out_normalized->has_keyboard_action = raw->key_event_count > 0u ? 1u : 0u;
    out_normalized->has_pointer_action = raw->pointer_event_count > 0u ? 1u : 0u;
    out_normalized->has_wheel_action = raw->wheel_event_count > 0u ? 1u : 0u;
    out_normalized->action_count =
        raw->quit_event_count +
        raw->window_event_count +
        raw->key_event_count +
        raw->pointer_event_count +
        raw->wheel_event_count;
    /* P4-S3: all normalized actions remain immediate; queued lane is explicit but deferred. */
    out_normalized->immediate_action_count = out_normalized->action_count;
    out_normalized->queued_action_count = 0u;
    out_normalized->ignored_action_count = raw->other_event_count;
    if (raw->key_event_count > 0u) {
        out_normalized->has_tool_switch_action = 1u;
        out_normalized->requested_tool_kind = (uint8_t)DRAWING_PROGRAM_TOOL_SELECT;
    }

    memset(out_route, 0, sizeof(*out_route));
    out_route->target_policy = DRAWING_PROGRAM_INPUT_ROUTE_TARGET_FALLBACK;
    if (out_normalized->has_quit_action || out_normalized->has_window_action || out_normalized->has_keyboard_action) {
        if (out_normalized->has_quit_action) {
            out_route->routed_global_count += 1u;
        }
        if (out_normalized->has_window_action) {
            out_route->routed_global_count += 1u;
        }
        if (out_normalized->has_keyboard_action) {
            out_route->routed_global_count += 1u;
        }
        out_route->target_policy = DRAWING_PROGRAM_INPUT_ROUTE_TARGET_GLOBAL;
        out_route->consumed = 1u;
    }
    if (out_normalized->has_pointer_action || out_normalized->has_wheel_action) {
        if (out_normalized->has_pointer_action) {
            out_route->routed_pane_count += 1u;
        }
        if (out_normalized->has_wheel_action) {
            out_route->routed_pane_count += 1u;
        }
        if (out_route->target_policy == DRAWING_PROGRAM_INPUT_ROUTE_TARGET_FALLBACK) {
            out_route->target_policy = DRAWING_PROGRAM_INPUT_ROUTE_TARGET_PANE;
        }
        out_route->consumed = 1u;
    }
    if (!out_route->consumed) {
        out_route->routed_fallback_count = out_normalized->action_count > 0u ? out_normalized->action_count : 1u;
        out_route->target_policy = DRAWING_PROGRAM_INPUT_ROUTE_TARGET_FALLBACK;
    }

    memset(out_invalidation, 0, sizeof(*out_invalidation));
    if (out_normalized->has_quit_action) {
        bits |= DRAWING_PROGRAM_INPUT_INVALIDATE_REASON_QUIT;
    }
    if (out_normalized->has_window_action) {
        bits |= DRAWING_PROGRAM_INPUT_INVALIDATE_REASON_WINDOW;
    }
    if (out_normalized->has_keyboard_action) {
        bits |= DRAWING_PROGRAM_INPUT_INVALIDATE_REASON_KEYBOARD;
    }
    if (out_normalized->has_pointer_action) {
        bits |= DRAWING_PROGRAM_INPUT_INVALIDATE_REASON_POINTER;
    }
    if (out_normalized->has_wheel_action) {
        bits |= DRAWING_PROGRAM_INPUT_INVALIDATE_REASON_WHEEL;
    }
    out_invalidation->invalidation_reason_bits = bits;
    out_invalidation->full_invalidate =
        (out_normalized->has_quit_action || out_normalized->has_window_action) ? 1u : 0u;
    if (out_invalidation->full_invalidate) {
        out_invalidation->full_invalidation_count = 1u;
    } else if (out_route->consumed) {
        out_invalidation->target_invalidation_count = 1u;
    }
    return core_result_ok();
}

CoreResult drawing_program_runtime_orchestration_dispatch_immediate(
    DrawingProgramAppContext *ctx,
    const DrawingProgramInputEventNormalized *normalized) {
    if (!ctx || !normalized) {
        return orchestration_invalid("null orchestration immediate dispatch argument");
    }
    if (normalized->has_pointer_action && ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_BRUSH) {
        CoreResult draw_result = drawing_program_runtime_orchestration_apply_workflow_control(
            ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE);
        if (draw_result.code != CORE_OK) {
            return draw_result;
        }
    }
    if (normalized->has_tool_switch_action) {
        DrawingProgramToolKind next_tool = (DrawingProgramToolKind)normalized->requested_tool_kind;
        CoreResult switch_result = set_active_tool(ctx, next_tool);
        if (switch_result.code != CORE_OK) {
            return switch_result;
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_runtime_orchestration_submit_deferred(
    DrawingProgramAppContext *ctx,
    const DrawingProgramInputEventNormalized *normalized) {
    if (!ctx || !normalized) {
        return orchestration_invalid("null orchestration deferred submit argument");
    }
    if (normalized->queued_action_count > 0u) {
        CoreResult err = { CORE_ERR_FORMAT,
                           "queued/deferred orchestration lane is declared but not implemented in phase 4" };
        return err;
    }
    /* P4-S3 guardrail: worker/scheduler lane remains explicit and intentionally deferred. */
    return core_result_ok();
}
