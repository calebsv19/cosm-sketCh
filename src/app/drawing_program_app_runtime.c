#include "drawing_program/drawing_program_app_main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "drawing_program_app_main_internal.h"
#include "core_font.h"
#include "core_theme.h"
#include "drawing_program/drawing_program_project_state.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_session_prefs.h"
#include "drawing_program/drawing_program_snapshot.h"
#include "drawing_program/drawing_program_ui_color_state.h"

static CoreResult drawing_program_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static int drawing_program_trace_ui_state_enabled(void) {
    const char *value = getenv("DRAWING_PROGRAM_TRACE_UI_STATE");
    if (!value || value[0] == '\0' || value[0] == '0') {
        return 0;
    }
    return 1;
}

static uint8_t drawing_program_clamp_percent(uint8_t value) {
    return (value > 100u) ? 100u : value;
}

static void drawing_program_ui_layer_opacity_set(DrawingProgramAppContext *ctx,
                                                 uint32_t layer_id,
                                                 uint8_t opacity_percent) {
    uint8_t i;
    uint8_t opacity = drawing_program_clamp_percent(opacity_percent);
    if (!ctx || layer_id == 0u) {
        return;
    }
    for (i = 0u; i < ctx->ui.layer_opacity_entry_count; ++i) {
        if (ctx->ui.layer_opacity_layer_ids[i] == layer_id) {
            ctx->ui.layer_opacity_values[i] = opacity;
            return;
        }
    }
    if (ctx->ui.layer_opacity_entry_count >= DRAWING_PROGRAM_MAX_LAYERS) {
        return;
    }
    ctx->ui.layer_opacity_layer_ids[ctx->ui.layer_opacity_entry_count] = layer_id;
    ctx->ui.layer_opacity_values[ctx->ui.layer_opacity_entry_count] = opacity;
    ctx->ui.layer_opacity_entry_count += 1u;
}

static void drawing_program_ui_layer_opacity_sync_with_document(DrawingProgramAppContext *ctx) {
    uint8_t compact_count = 0u;
    uint8_t i;
    if (!ctx) {
        return;
    }
    for (i = 0u; i < ctx->ui.layer_opacity_entry_count; ++i) {
        uint32_t layer_id = ctx->ui.layer_opacity_layer_ids[i];
        uint32_t ignored = 0u;
        if (layer_id != 0u &&
            drawing_program_document_layer_index_for_id(&ctx->document, layer_id, &ignored).code == CORE_OK) {
            ctx->ui.layer_opacity_layer_ids[compact_count] = layer_id;
            ctx->ui.layer_opacity_values[compact_count] =
                drawing_program_clamp_percent(ctx->ui.layer_opacity_values[i]);
            compact_count += 1u;
        }
    }
    ctx->ui.layer_opacity_entry_count = compact_count;
    for (i = 0u; i < ctx->document.layer_count; ++i) {
        drawing_program_ui_layer_opacity_set(ctx, ctx->document.layers[i].layer_id, 100u);
    }
}

static void drawing_program_normalize_ui_state(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    if (ctx->ui.theme_preset_id >= (uint32_t)CORE_THEME_PRESET_COUNT) {
        ctx->ui.theme_preset_id = (uint32_t)CORE_THEME_PRESET_DARK_DEFAULT;
    }
    if (ctx->ui.font_preset_id >= (uint32_t)CORE_FONT_PRESET_COUNT ||
        !core_font_preset_name((CoreFontPresetId)ctx->ui.font_preset_id)) {
        ctx->ui.font_preset_id = (uint32_t)CORE_FONT_PRESET_IDE;
    }
    if (ctx->ui.font_zoom_step < -2) {
        ctx->ui.font_zoom_step = -2;
    } else if (ctx->ui.font_zoom_step > 4) {
        ctx->ui.font_zoom_step = 4;
    }
    if (ctx->ui.left_panel_slot > 1u) {
        ctx->ui.left_panel_slot = 0u;
    }
    if (ctx->ui.right_panel_slot > 3u) {
        ctx->ui.right_panel_slot = 0u;
    }
    drawing_program_ui_color_normalize_state(ctx);
    if (ctx->ui.tool_brush_size < 1u) {
        ctx->ui.tool_brush_size = 1u;
    } else if (ctx->ui.tool_brush_size > 16u) {
        ctx->ui.tool_brush_size = 16u;
    }
    if (ctx->ui.tool_brush_opacity < 1u) {
        ctx->ui.tool_brush_opacity = 1u;
    } else if (ctx->ui.tool_brush_opacity > 100u) {
        ctx->ui.tool_brush_opacity = 100u;
    }
    if (ctx->ui.tool_brush_spacing < 1u) {
        ctx->ui.tool_brush_spacing = 1u;
    } else if (ctx->ui.tool_brush_spacing > 16u) {
        ctx->ui.tool_brush_spacing = 16u;
    }
    if (ctx->ui.tool_brush_hardness < 1u) {
        ctx->ui.tool_brush_hardness = 1u;
    } else if (ctx->ui.tool_brush_hardness > 100u) {
        ctx->ui.tool_brush_hardness = 100u;
    }
    if (ctx->ui.tool_eraser_size < 1u) {
        ctx->ui.tool_eraser_size = 1u;
    } else if (ctx->ui.tool_eraser_size > 16u) {
        ctx->ui.tool_eraser_size = 16u;
    }
    if (ctx->ui.tool_shape_stroke_width < 1u) {
        ctx->ui.tool_shape_stroke_width = 1u;
    } else if (ctx->ui.tool_shape_stroke_width > 16u) {
        ctx->ui.tool_shape_stroke_width = 16u;
    }
    if (ctx->ui.tool_shape_mode > 2u) {
        ctx->ui.tool_shape_mode = 0u;
    }
    if (ctx->ui.tool_shape_target_mode > (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_OBJECT) {
        ctx->ui.tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
    }
    if (ctx->ui.tool_fill_tolerance > DRAWING_PROGRAM_UI_FILL_TOLERANCE_MAX) {
        ctx->ui.tool_fill_tolerance = DRAWING_PROGRAM_UI_FILL_TOLERANCE_MAX;
    }
    if (ctx->ui.tool_select_mode > (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_SUBTRACT) {
        ctx->ui.tool_select_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
    }
    drawing_program_ui_layer_opacity_sync_with_document(ctx);
}

static void drawing_program_input_intake(uint64_t frame_index,
                                         uint32_t total_frames,
                                         uint8_t simulate_events,
                                         DrawingProgramInputEventRaw *out_raw) {
    if (!out_raw) {
        return;
    }
    memset(out_raw, 0, sizeof(*out_raw));
    out_raw->frame_index = frame_index;
    if (!simulate_events) {
        return;
    }

    out_raw->pointer_event_count = 1u;
    out_raw->event_count = 1u;
    if (frame_index == 0u) {
        out_raw->key_event_count = 1u;
        out_raw->event_count += 1u;
    }

    if (frame_index + 1u >= (uint64_t)total_frames) {
        out_raw->quit_event_count = 1u;
        out_raw->quit_requested = 1u;
        out_raw->event_count += 1u;
    }
}

static CoreResult drawing_program_render_project_and_update_counters(
    DrawingProgramAppContext *ctx,
    const DrawingProgramInputInvalidationResult *invalidation) {
    DrawingProgramRenderInvalidation render_invalidation;
    CoreResult result;
    if (!ctx || !invalidation) {
        return drawing_program_invalid("null render projection input");
    }
    memset(&render_invalidation, 0, sizeof(render_invalidation));
    render_invalidation.full_invalidate = invalidation->full_invalidate;
    render_invalidation.invalidation_reason_bits = invalidation->invalidation_reason_bits;
    render_invalidation.target_invalidation_count = invalidation->target_invalidation_count;
    render_invalidation.full_invalidation_count = invalidation->full_invalidation_count;

    result = drawing_program_render_project_frame(&ctx->document,
                                                  &ctx->layer_rasters,
                                                  &ctx->editor,
                                                  &render_invalidation,
                                                  &ctx->runtime.render_projection);
    if (result.code != CORE_OK) {
        return result;
    }

    ctx->runtime.render_frames_projected_total += 1u;
    ctx->runtime.render_layers_visible_total += (uint64_t)ctx->runtime.render_projection.visible_layer_count;
    if (ctx->runtime.render_projection.full_redraw) {
        ctx->runtime.render_full_redraw_total += 1u;
    } else if (ctx->runtime.render_projection.targeted_redraw) {
        ctx->runtime.render_target_redraw_total += 1u;
    }
    ctx->runtime.render_last_has_active_layer = ctx->runtime.render_projection.has_active_layer;
    ctx->runtime.render_last_active_layer_id = ctx->runtime.render_projection.active_layer_id;

    return core_result_ok();
}

/* RS1 Update phase: input intake/route + immediate/deferred dispatch. */
static CoreResult drawing_program_frame_update(DrawingProgramAppContext *ctx,
                                               uint32_t frame_index,
                                               uint32_t total_frames,
                                               DrawingProgramInputEventRaw *raw_out,
                                               DrawingProgramInputEventNormalized *normalized_out,
                                               DrawingProgramInputRouteResult *route_out,
                                               DrawingProgramInputInvalidationResult *invalidation_out) {
    CoreResult result;
    DrawingProgramOverlayAdapterResult adapter_result;
    if (!ctx || !raw_out || !normalized_out || !route_out || !invalidation_out) {
        return drawing_program_invalid("null RS1 update argument");
    }
    adapter_result = drawing_program_adapter_runtime_tick(ctx);
    if (!adapter_result.ok) {
        CoreResult err = { CORE_ERR_FORMAT, adapter_result.reason };
        return err;
    }
    adapter_result = drawing_program_adapter_input_route_runtime(ctx);
    if (!adapter_result.ok) {
        CoreResult err = { CORE_ERR_FORMAT, adapter_result.reason };
        return err;
    }
    drawing_program_input_intake((uint64_t)frame_index, total_frames, ctx->session.headless, raw_out);
    result = drawing_program_runtime_orchestration_plan_frame(raw_out, normalized_out, route_out, invalidation_out);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_runtime_orchestration_dispatch_immediate(ctx, normalized_out);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_runtime_orchestration_submit_deferred(ctx, normalized_out);
    if (result.code != CORE_OK) {
        return result;
    }
    ctx->runtime.input_events_processed += (uint64_t)raw_out->event_count;
    ctx->runtime.input_actions_emitted += (uint64_t)normalized_out->action_count;
    ctx->runtime.routed_global_total += (uint64_t)route_out->routed_global_count;
    ctx->runtime.routed_pane_total += (uint64_t)route_out->routed_pane_count;
    ctx->runtime.routed_fallback_total += (uint64_t)route_out->routed_fallback_count;
    ctx->runtime.invalidation_target_total += (uint64_t)invalidation_out->target_invalidation_count;
    ctx->runtime.invalidation_full_total += (uint64_t)invalidation_out->full_invalidation_count;
    ctx->runtime.invalidation_reason_bits_total += (uint64_t)invalidation_out->invalidation_reason_bits;
    return core_result_ok();
}

/* RS1 RenderDerive phase: derive projection/transforms only. */
static CoreResult drawing_program_frame_render_derive(DrawingProgramAppContext *ctx,
                                                      const DrawingProgramInputInvalidationResult *invalidation,
                                                      DrawingProgramViewportTransform *viewport_out) {
    CoreResult result;
    if (!ctx || !invalidation || !viewport_out) {
        return drawing_program_invalid("null RS1 render_derive argument");
    }
    result = drawing_program_render_project_and_update_counters(ctx, invalidation);
    if (result.code != CORE_OK) {
        return result;
    }
    drawing_program_viewport_transform_from_state(&ctx->editor, &ctx->document, viewport_out);
    return core_result_ok();
}

/* RS1 RenderSubmit phase: submit to active backend/runtime adapter. */
static CoreResult drawing_program_frame_render_submit(DrawingProgramAppContext *ctx) {
    DrawingProgramOverlayAdapterResult adapter_result;
    if (!ctx) {
        return drawing_program_invalid("null RS1 render_submit argument");
    }
    adapter_result = drawing_program_adapter_render_runtime_base(ctx);
    if (!adapter_result.ok) {
        CoreResult err = { CORE_ERR_FORMAT, adapter_result.reason };
        return err;
    }
    return core_result_ok();
}

CoreResult drawing_program_app_state_seed(DrawingProgramAppContext *ctx) {
    CoreResult result;
    DrawingProgramOverlayAdapterResult adapter_result;
    if (!ctx) {
        return drawing_program_invalid("null app context");
    }
    result = drawing_program_pane_host_init(ctx);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_document_init_with_shape(
        &ctx->document, ctx->session.seed_canvas_logical_width, ctx->session.seed_canvas_logical_height, 1u);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_layer_raster_store_init_from_document(&ctx->layer_rasters, &ctx->document);
    if (result.code != CORE_OK) {
        return result;
    }
    drawing_program_object_store_reset(&ctx->object_store);
    drawing_program_object_selection_reset(&ctx->object_selection);
    drawing_program_editor_state_init(&ctx->editor, &ctx->document);
    drawing_program_history_init(&ctx->history);
    drawing_program_ui_layer_opacity_sync_with_document(ctx);
    drawing_program_selection_reset(&ctx->selection);
    adapter_result = drawing_program_overlay_adapter_init(ctx);
    if (!adapter_result.ok) {
        CoreResult err = { CORE_ERR_FORMAT, adapter_result.reason };
        drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
        return err;
    }
    ctx->runtime.state_seeded = 1u;
    return core_result_ok();
}

CoreResult drawing_program_app_subsystems_init(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return drawing_program_invalid("null app context");
    }
    if (!ctx->runtime.state_seeded) {
        return drawing_program_invalid("state must be seeded before subsystem init");
    }
    ctx->runtime.subsystems_ready = 1u;
    return core_result_ok();
}

CoreResult drawing_program_runtime_start(DrawingProgramAppContext *ctx) {
    CoreResult result;
    CoreResult load_result;
    uint8_t upgraded_legacy_checker_seed = 0u;
    uint8_t saved_project_exists = 0u;
    if (!ctx) {
        return drawing_program_invalid("null app context");
    }
    if (!ctx->runtime.subsystems_ready) {
        return drawing_program_invalid("subsystems must be initialized before runtime start");
    }

    if (ctx->session.canvas_size_cli_override) {
        load_result = (CoreResult){ CORE_ERR_NOT_FOUND, "snapshot load bypassed by explicit canvas-size override" };
    } else {
        result = drawing_program_project_state_current_exists(ctx, &saved_project_exists);
        if (result.code != CORE_OK) {
            return result;
        }
        if (saved_project_exists) {
            load_result = drawing_program_project_state_load_current(ctx);
        } else if (ctx->session.preset_path_cli_override && ctx->session.preset_path &&
                   access(ctx->session.preset_path, F_OK) == 0) {
            load_result = drawing_program_snapshot_load(ctx, ctx->session.preset_path);
        } else {
            load_result = (CoreResult){ CORE_ERR_NOT_FOUND, "no saved project selected for boot" };
        }
    }
    result = load_result;
    if (drawing_program_trace_ui_state_enabled()) {
        fprintf(stderr,
                "drawing_program trace runtime_start after_load code=%d tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u color=%u leafs=%u\n",
                (int)result.code,
                (unsigned)ctx->editor.active_tool,
                (unsigned)ctx->ui.theme_preset_id,
                (unsigned)ctx->ui.font_preset_id,
                (int)ctx->ui.font_zoom_step,
                (unsigned)ctx->ui.left_panel_slot,
                (unsigned)ctx->ui.right_panel_slot,
                (unsigned)ctx->ui.active_color_index,
                (unsigned)ctx->pane_host.leaf_count);
    }
    ctx->runtime.snapshot_loaded_from_preset =
        (load_result.code == CORE_OK && ctx->pane_host.leaf_count >= 4u) ? 1u : 0u;
    if (!ctx->runtime.snapshot_loaded_from_preset) {
        /* Keep scaffold visuals deterministic: fall back to the seeded 4-pane host. */
        (void)drawing_program_pane_host_init(ctx);
        if (ctx->session.persist_enabled) {
            result = drawing_program_snapshot_save(ctx, ctx->session.preset_path);
            if (drawing_program_trace_ui_state_enabled()) {
                fprintf(stderr,
                        "drawing_program trace runtime_start fallback_save code=%d path=%s\n",
                        (int)result.code,
                        ctx->session.preset_path ? ctx->session.preset_path : "(null)");
            }
        }
    }
    result = drawing_program_document_upgrade_legacy_checker_seed(&ctx->document, &upgraded_legacy_checker_seed);
    if (result.code != CORE_OK) {
        return result;
    }
    if (upgraded_legacy_checker_seed && ctx->session.persist_enabled) {
        (void)drawing_program_snapshot_save(ctx, ctx->session.preset_path);
    }
    drawing_program_selection_cancel_transient(&ctx->selection);
    drawing_program_normalize_ui_state(ctx);
    if (drawing_program_trace_ui_state_enabled()) {
        fprintf(stderr,
                "drawing_program trace runtime_start normalized tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u color=%u\n",
                (unsigned)ctx->editor.active_tool,
                (unsigned)ctx->ui.theme_preset_id,
                (unsigned)ctx->ui.font_preset_id,
                (int)ctx->ui.font_zoom_step,
                (unsigned)ctx->ui.left_panel_slot,
                (unsigned)ctx->ui.right_panel_slot,
                (unsigned)ctx->ui.active_color_index);
    }

    if (ctx->session.bridge_workspace_check_requested) {
        result = drawing_program_snapshot_bridge_check_workspace_preset(ctx->session.bridge_workspace_preset_path);
        if (result.code != CORE_OK) {
            return result;
        }
        printf("drawing_program workspace preset bridge check OK: %s\n",
               ctx->session.bridge_workspace_preset_path ? ctx->session.bridge_workspace_preset_path : "(null)");
    }
    if (ctx->session.bridge_workspace_import_requested) {
        result = drawing_program_snapshot_bridge_import_workspace_preset(ctx, ctx->session.bridge_workspace_preset_path);
        if (result.code != CORE_OK) {
            return result;
        }
        printf("drawing_program workspace preset import OK: %s\n",
               ctx->session.bridge_workspace_preset_path ? ctx->session.bridge_workspace_preset_path : "(null)");
    }

    ctx->runtime.runtime_started = 1u;
    return core_result_ok();
}

CoreResult drawing_program_app_run_loop(DrawingProgramAppContext *ctx) {
    CoreResult result;
    uint32_t frames;
    uint32_t i;
    DrawingProgramInputEventRaw raw;
    DrawingProgramInputEventNormalized normalized;
    DrawingProgramInputRouteResult route;
    DrawingProgramInputInvalidationResult invalidation;
    DrawingProgramViewportTransform viewport_transform;
    if (!ctx) {
        return drawing_program_invalid("null app context");
    }
    if (!ctx->runtime.runtime_started) {
        return drawing_program_invalid("runtime must start before run loop");
    }

    frames = ctx->session.headless ? ctx->session.smoke_frames : 1u;
    if (frames == 0u) {
        frames = 1u;
    }

    for (i = 0; i < frames; ++i) {
        DrawingProgramScreenPoint screen_probe;
        DrawingProgramSamplePoint sample_probe;
        result = drawing_program_frame_update(ctx, i, frames, &raw, &normalized, &route, &invalidation);
        if (result.code != CORE_OK) {
            return result;
        }
        result = drawing_program_frame_render_derive(ctx, &invalidation, &viewport_transform);
        if (result.code != CORE_OK) {
            return result;
        }
        screen_probe.x = 100.0f + (float)i;
        screen_probe.y = 100.0f + (float)i;
        if (drawing_program_screen_to_sample(viewport_transform, screen_probe, &sample_probe)) {
            ctx->runtime.viewport_sample_probe_success_total += 1u;
        }
        result = drawing_program_frame_render_submit(ctx);
        if (result.code != CORE_OK) {
            return result;
        }
        ctx->runtime.frame_counter += 1u;
    }

    return core_result_ok();
}

CoreResult drawing_program_app_shutdown(DrawingProgramAppContext *ctx) {
    CoreResult result = core_result_ok();
    if (!ctx) {
        return drawing_program_invalid("null app context");
    }
    if (!ctx->session.persist_enabled) {
        if (ctx->session.export_json_requested) {
            result = drawing_program_snapshot_export_debug_json(ctx, ctx->session.export_json_path);
            if (result.code != CORE_OK) {
                drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
                return result;
            }
        }
        drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
        return core_result_ok();
    }
    result = drawing_program_session_prefs_save(ctx);
    if (result.code != CORE_OK) {
        drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
        return result;
    }
    result = drawing_program_app_ensure_parent_dir(ctx->session.preset_path);
    if (result.code != CORE_OK) {
        drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
        return result;
    }
    result = drawing_program_snapshot_save(ctx, ctx->session.preset_path);
    if (drawing_program_trace_ui_state_enabled()) {
        fprintf(stderr,
                "drawing_program trace shutdown save_result code=%d path=%s tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u color=%u\n",
                (int)result.code,
                ctx->session.preset_path ? ctx->session.preset_path : "(null)",
                (unsigned)ctx->editor.active_tool,
                (unsigned)ctx->ui.theme_preset_id,
                (unsigned)ctx->ui.font_preset_id,
                (int)ctx->ui.font_zoom_step,
                (unsigned)ctx->ui.left_panel_slot,
                (unsigned)ctx->ui.right_panel_slot,
                (unsigned)ctx->ui.active_color_index);
    }
    if (result.code != CORE_OK) {
        fprintf(stderr,
                "drawing_program: snapshot save failed code=%d message=%s path=%s\n",
                (int)result.code,
                result.message ? result.message : "(null)",
                ctx->session.preset_path ? ctx->session.preset_path : "(null)");
        drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
        return result;
    }
    if (ctx->session.export_json_requested) {
        result = drawing_program_snapshot_export_debug_json(ctx, ctx->session.export_json_path);
        if (result.code != CORE_OK) {
            fprintf(stderr,
                    "drawing_program: snapshot debug export failed code=%d message=%s json=%s\n",
                    (int)result.code,
                    result.message ? result.message : "(null)",
                    ctx->session.export_json_path ? ctx->session.export_json_path : "(null)");
            drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
            return result;
        }
        printf("drawing_program snapshot debug export OK: preset=%s json=%s\n",
               ctx->session.preset_path ? ctx->session.preset_path : "(null)",
               ctx->session.export_json_path ? ctx->session.export_json_path : "(null)");
    }
    drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
    return core_result_ok();
}

CoreResult drawing_program_app_set_pane_host_bounds(DrawingProgramAppContext *ctx, float width, float height) {
    if (!ctx) {
        return drawing_program_invalid("null app context");
    }
    if (width < 64.0f || height < 64.0f) {
        return drawing_program_invalid("invalid pane host bounds");
    }
    if (ctx->pane_host_bounds_width == width && ctx->pane_host_bounds_height == height) {
        return core_result_ok();
    }
    ctx->pane_host_bounds_width = width;
    ctx->pane_host_bounds_height = height;
    return drawing_program_pane_host_rebuild(ctx);
}
