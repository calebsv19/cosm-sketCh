#include "drawing_program/drawing_program_runtime_orchestration.h"

#include <string.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_history.h"

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

static CoreResult stamp_center_sample(DrawingProgramAppContext *ctx) {
    uint32_t sx;
    uint32_t sy;
    uint8_t value = drawing_program_color_value_from_index(
        ctx ? drawing_program_color_index_clamp(ctx->ui_active_color_index) : drawing_program_color_default_index());
    if (!ctx) {
        return orchestration_invalid("null app context for stamp center sample");
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
        default:
            value = drawing_program_color_value_from_index(drawing_program_color_index_clamp(ctx->ui_active_color_index));
            break;
    }
    return drawing_program_history_apply_set_sample_value(&ctx->history, &ctx->document, sx, sy, value);
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
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_UNDO:
            return drawing_program_history_undo(&ctx->history, &ctx->document);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_REDO:
            return drawing_program_history_redo(&ctx->history, &ctx->document);
        case DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_HISTORY:
            drawing_program_history_clear(&ctx->history);
            return core_result_ok();
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
