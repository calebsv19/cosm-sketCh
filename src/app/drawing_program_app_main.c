#include "drawing_program/drawing_program_app_main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "core_font.h"
#include "core_theme.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"

typedef enum DrawingProgramAppStage {
    DRAWING_PROGRAM_APP_STAGE_INIT = 0,
    DRAWING_PROGRAM_APP_STAGE_BOOTSTRAPPED,
    DRAWING_PROGRAM_APP_STAGE_CONFIG_LOADED,
    DRAWING_PROGRAM_APP_STAGE_STATE_SEEDED,
    DRAWING_PROGRAM_APP_STAGE_SUBSYSTEMS_READY,
    DRAWING_PROGRAM_APP_STAGE_RUNTIME_STARTED,
    DRAWING_PROGRAM_APP_STAGE_LOOP_COMPLETED,
    DRAWING_PROGRAM_APP_STAGE_SHUTDOWN_COMPLETED
} DrawingProgramAppStage;

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

static void drawing_program_normalize_ui_state(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    if (ctx->ui_theme_preset_id >= (uint32_t)CORE_THEME_PRESET_COUNT) {
        ctx->ui_theme_preset_id = (uint32_t)CORE_THEME_PRESET_DARK_DEFAULT;
    }
    if (ctx->ui_font_preset_id >= (uint32_t)CORE_FONT_PRESET_COUNT ||
        !core_font_preset_name((CoreFontPresetId)ctx->ui_font_preset_id)) {
        ctx->ui_font_preset_id = (uint32_t)CORE_FONT_PRESET_IDE;
    }
    if (ctx->ui_font_zoom_step < -2) {
        ctx->ui_font_zoom_step = -2;
    } else if (ctx->ui_font_zoom_step > 4) {
        ctx->ui_font_zoom_step = 4;
    }
    if (ctx->ui_left_panel_slot > 1u) {
        ctx->ui_left_panel_slot = 0u;
    }
    if (ctx->ui_right_panel_slot > 1u) {
        ctx->ui_right_panel_slot = 0u;
    }
    ctx->ui_active_color_index = drawing_program_color_index_clamp(ctx->ui_active_color_index);
}

static int drawing_program_transition_stage(DrawingProgramAppStage *stage,
                                            DrawingProgramAppStage expected,
                                            DrawingProgramAppStage next,
                                            const char *label) {
    if (!stage) {
        return 0;
    }
    if (*stage != expected) {
        fprintf(stderr,
                "drawing_program: lifecycle order violation label=%s expected=%d actual=%d next=%d\n",
                label ? label : "unknown",
                (int)expected,
                (int)(*stage),
                (int)next);
        return 0;
    }
    *stage = next;
    return 1;
}

static void drawing_program_lifecycle_note(const DrawingProgramAppContext *ctx, const char *stage_name) {
    if (!ctx || !ctx->print_lifecycle) {
        return;
    }
    printf("drawing_program lifecycle: %s\n", stage_name ? stage_name : "unknown");
}

static void drawing_program_seed_data_roots(DrawingProgramAppContext *ctx) {
    const char *runtime_env;
    if (!ctx) {
        return;
    }
    runtime_env = getenv("DRAWING_PROGRAM_RUNTIME_DIR");
    if (runtime_env && runtime_env[0] != '\0') {
        if (!ctx->runtime_root_cli_override) {
            (void)snprintf(ctx->runtime_root_path, sizeof(ctx->runtime_root_path), "%s", runtime_env);
        }
        if (!ctx->input_root_cli_override) {
            (void)snprintf(ctx->input_root_path, sizeof(ctx->input_root_path), "%s/input", ctx->runtime_root_path);
        }
        if (!ctx->output_root_cli_override) {
            (void)snprintf(ctx->output_root_path, sizeof(ctx->output_root_path), "%s/output", ctx->runtime_root_path);
        }
    }
}

static CoreResult drawing_program_mkdirs_if_needed(const char *dir_path) {
    char buffer[512];
    size_t i;
    size_t len;
    if (!dir_path || dir_path[0] == '\0') {
        return core_result_ok();
    }
    len = strlen(dir_path);
    if (len >= sizeof(buffer)) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "directory path too long" };
    }
    (void)snprintf(buffer, sizeof(buffer), "%s", dir_path);
    for (i = 1u; i < len; ++i) {
        if (buffer[i] != '/') {
            continue;
        }
        buffer[i] = '\0';
        if (buffer[0] != '\0') {
            if (mkdir(buffer, 0775) != 0 && errno != EEXIST) {
                return (CoreResult){ CORE_ERR_IO, "failed to create runtime directory segment" };
            }
        }
        buffer[i] = '/';
    }
    if (mkdir(buffer, 0775) != 0 && errno != EEXIST) {
        return (CoreResult){ CORE_ERR_IO, "failed to create runtime directory" };
    }
    return core_result_ok();
}

static CoreResult drawing_program_ensure_parent_dir(const char *file_path) {
    char buffer[512];
    char *slash;
    if (!file_path || file_path[0] == '\0') {
        return core_result_ok();
    }
    if (strlen(file_path) >= sizeof(buffer)) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "file path too long" };
    }
    (void)snprintf(buffer, sizeof(buffer), "%s", file_path);
    slash = strrchr(buffer, '/');
    if (!slash) {
        return core_result_ok();
    }
    *slash = '\0';
    if (buffer[0] == '\0') {
        return core_result_ok();
    }
    return drawing_program_mkdirs_if_needed(buffer);
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
                                                  &ctx->render_projection);
    if (result.code != CORE_OK) {
        return result;
    }

    ctx->render_frames_projected_total += 1u;
    ctx->render_layers_visible_total += (uint64_t)ctx->render_projection.visible_layer_count;
    if (ctx->render_projection.full_redraw) {
        ctx->render_full_redraw_total += 1u;
    } else if (ctx->render_projection.targeted_redraw) {
        ctx->render_target_redraw_total += 1u;
    }
    ctx->render_last_has_active_layer = ctx->render_projection.has_active_layer;
    ctx->render_last_active_layer_id = ctx->render_projection.active_layer_id;

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
    drawing_program_input_intake((uint64_t)frame_index, total_frames, ctx->headless, raw_out);
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
    ctx->input_events_processed += (uint64_t)raw_out->event_count;
    ctx->input_actions_emitted += (uint64_t)normalized_out->action_count;
    ctx->routed_global_total += (uint64_t)route_out->routed_global_count;
    ctx->routed_pane_total += (uint64_t)route_out->routed_pane_count;
    ctx->routed_fallback_total += (uint64_t)route_out->routed_fallback_count;
    ctx->invalidation_target_total += (uint64_t)invalidation_out->target_invalidation_count;
    ctx->invalidation_full_total += (uint64_t)invalidation_out->full_invalidation_count;
    ctx->invalidation_reason_bits_total += (uint64_t)invalidation_out->invalidation_reason_bits;
    return core_result_ok();
}

/* RS1 RenderDerive phase: derive projection/transforms only. */
static CoreResult drawing_program_frame_render_derive(
    DrawingProgramAppContext *ctx,
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

CoreResult drawing_program_app_bootstrap(DrawingProgramAppContext *ctx, int argc, char **argv) {
    int i;
    if (!ctx) {
        return drawing_program_invalid("null app context");
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->smoke_frames = 1u;
    ctx->persist_enabled = 1u;
    ctx->preset_path = 0;
    ctx->export_json_path = 0;
    ctx->bridge_workspace_preset_path = "workspace_sandbox/data/presets/sketch_layout_v1.pack";
    ctx->pane_host_bounds_width = 1200.0f;
    ctx->pane_host_bounds_height = 800.0f;
    ctx->ui_theme_preset_id = (uint32_t)CORE_THEME_PRESET_DARK_DEFAULT;
    ctx->ui_font_preset_id = (uint32_t)CORE_FONT_PRESET_IDE;
    ctx->ui_left_panel_slot = 0u;
    ctx->ui_right_panel_slot = 0u;
    ctx->ui_active_color_index = drawing_program_color_default_index();
    ctx->ui_font_zoom_step = 0;
    (void)snprintf(ctx->runtime_root_path, sizeof(ctx->runtime_root_path), "data/runtime");
    (void)snprintf(ctx->input_root_path, sizeof(ctx->input_root_path), "data/input");
    (void)snprintf(ctx->output_root_path, sizeof(ctx->output_root_path), "data/output");

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--headless") == 0) {
            ctx->headless = 1u;
            continue;
        }
        if (strcmp(argv[i], "--print-lifecycle") == 0) {
            ctx->print_lifecycle = 1u;
            continue;
        }
        if (strcmp(argv[i], "--smoke-frames") == 0 && i + 1 < argc) {
            unsigned long parsed = strtoul(argv[++i], 0, 10);
            ctx->smoke_frames = (parsed > 0ul) ? (uint32_t)parsed : 1u;
            continue;
        }
        if (strcmp(argv[i], "--preset") == 0 && i + 1 < argc) {
            ctx->preset_path = argv[++i];
            ctx->preset_path_cli_override = 1u;
            continue;
        }
        if (strcmp(argv[i], "--no-persist") == 0) {
            ctx->persist_enabled = 0u;
            continue;
        }
        if (strcmp(argv[i], "--export-json") == 0 && i + 1 < argc) {
            ctx->export_json_path = argv[++i];
            ctx->export_json_requested = 1u;
            continue;
        }
        if (strcmp(argv[i], "--bridge-workspace-preset") == 0 && i + 1 < argc) {
            ctx->bridge_workspace_preset_path = argv[++i];
            ctx->bridge_workspace_check_requested = 1u;
            continue;
        }
        if (strcmp(argv[i], "--bridge-workspace-import") == 0) {
            ctx->bridge_workspace_import_requested = 1u;
            continue;
        }
        if (strcmp(argv[i], "--runtime-root") == 0 && i + 1 < argc) {
            (void)snprintf(ctx->runtime_root_path, sizeof(ctx->runtime_root_path), "%s", argv[++i]);
            ctx->runtime_root_cli_override = 1u;
            continue;
        }
        if (strcmp(argv[i], "--input-root") == 0 && i + 1 < argc) {
            (void)snprintf(ctx->input_root_path, sizeof(ctx->input_root_path), "%s", argv[++i]);
            ctx->input_root_cli_override = 1u;
            continue;
        }
        if (strcmp(argv[i], "--output-root") == 0 && i + 1 < argc) {
            (void)snprintf(ctx->output_root_path, sizeof(ctx->output_root_path), "%s", argv[++i]);
            ctx->output_root_cli_override = 1u;
            continue;
        }
    }

    return core_result_ok();
}

CoreResult drawing_program_app_config_load(DrawingProgramAppContext *ctx) {
    CoreResult result;
    if (!ctx) {
        return drawing_program_invalid("null app context");
    }
    drawing_program_seed_data_roots(ctx);
    result = drawing_program_mkdirs_if_needed(ctx->runtime_root_path);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_mkdirs_if_needed(ctx->input_root_path);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_mkdirs_if_needed(ctx->output_root_path);
    if (result.code != CORE_OK) {
        return result;
    }
    if (!ctx->preset_path_cli_override) {
        (void)snprintf(ctx->preset_path_buffer, sizeof(ctx->preset_path_buffer), "%s/last_session.pack", ctx->runtime_root_path);
        ctx->preset_path = ctx->preset_path_buffer;
    }
    result = drawing_program_ensure_parent_dir(ctx->preset_path);
    if (result.code != CORE_OK) {
        return result;
    }
    if (ctx->export_json_path) {
        result = drawing_program_ensure_parent_dir(ctx->export_json_path);
        if (result.code != CORE_OK) {
            return result;
        }
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
    result = drawing_program_document_init_default(&ctx->document);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_layer_raster_store_init_from_document(&ctx->layer_rasters, &ctx->document);
    if (result.code != CORE_OK) {
        return result;
    }
    drawing_program_editor_state_init(&ctx->editor, &ctx->document);
    drawing_program_history_init(&ctx->history);
    drawing_program_selection_reset(&ctx->selection);
    adapter_result = drawing_program_overlay_adapter_init(ctx);
    if (!adapter_result.ok) {
        CoreResult err = { CORE_ERR_FORMAT, adapter_result.reason };
        drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
        return err;
    }
    ctx->state_seeded = 1u;
    return core_result_ok();
}

CoreResult drawing_program_app_subsystems_init(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return drawing_program_invalid("null app context");
    }
    if (!ctx->state_seeded) {
        return drawing_program_invalid("state must be seeded before subsystem init");
    }
    ctx->subsystems_ready = 1u;
    return core_result_ok();
}

CoreResult drawing_program_runtime_start(DrawingProgramAppContext *ctx) {
    CoreResult result;
    CoreResult load_result;
    uint8_t upgraded_legacy_checker_seed = 0u;
    if (!ctx) {
        return drawing_program_invalid("null app context");
    }
    if (!ctx->subsystems_ready) {
        return drawing_program_invalid("subsystems must be initialized before runtime start");
    }

    load_result = drawing_program_snapshot_load(ctx, ctx->preset_path);
    result = load_result;
    if (drawing_program_trace_ui_state_enabled()) {
        fprintf(stderr,
                "drawing_program trace runtime_start after_load code=%d tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u color=%u leafs=%u\n",
                (int)result.code,
                (unsigned)ctx->editor.active_tool,
                (unsigned)ctx->ui_theme_preset_id,
                (unsigned)ctx->ui_font_preset_id,
                (int)ctx->ui_font_zoom_step,
                (unsigned)ctx->ui_left_panel_slot,
                (unsigned)ctx->ui_right_panel_slot,
                (unsigned)ctx->ui_active_color_index,
                (unsigned)ctx->pane_host.leaf_count);
    }
    ctx->snapshot_loaded_from_preset = (load_result.code == CORE_OK && ctx->pane_host.leaf_count >= 4u) ? 1u : 0u;
    if (!ctx->snapshot_loaded_from_preset) {
        /* Keep scaffold visuals deterministic: fall back to the seeded 4-pane host. */
        (void)drawing_program_pane_host_init(ctx);
        if (ctx->persist_enabled) {
            result = drawing_program_snapshot_save(ctx, ctx->preset_path);
            if (drawing_program_trace_ui_state_enabled()) {
                fprintf(stderr,
                        "drawing_program trace runtime_start fallback_save code=%d path=%s\n",
                        (int)result.code,
                        ctx->preset_path ? ctx->preset_path : "(null)");
            }
        }
    }
    result = drawing_program_document_upgrade_legacy_checker_seed(&ctx->document, &upgraded_legacy_checker_seed);
    if (result.code != CORE_OK) {
        return result;
    }
    if (upgraded_legacy_checker_seed && ctx->persist_enabled) {
        (void)drawing_program_snapshot_save(ctx, ctx->preset_path);
    }
    drawing_program_selection_cancel_transient(&ctx->selection);
    drawing_program_normalize_ui_state(ctx);
    if (drawing_program_trace_ui_state_enabled()) {
        fprintf(stderr,
                "drawing_program trace runtime_start normalized tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u color=%u\n",
                (unsigned)ctx->editor.active_tool,
                (unsigned)ctx->ui_theme_preset_id,
                (unsigned)ctx->ui_font_preset_id,
                (int)ctx->ui_font_zoom_step,
                (unsigned)ctx->ui_left_panel_slot,
                (unsigned)ctx->ui_right_panel_slot,
                (unsigned)ctx->ui_active_color_index);
    }

    if (ctx->bridge_workspace_check_requested) {
        result = drawing_program_snapshot_bridge_check_workspace_preset(ctx->bridge_workspace_preset_path);
        if (result.code != CORE_OK) {
            return result;
        }
        printf("drawing_program workspace preset bridge check OK: %s\n",
               ctx->bridge_workspace_preset_path ? ctx->bridge_workspace_preset_path : "(null)");
    }
    if (ctx->bridge_workspace_import_requested) {
        result = drawing_program_snapshot_bridge_import_workspace_preset(ctx, ctx->bridge_workspace_preset_path);
        if (result.code != CORE_OK) {
            return result;
        }
        printf("drawing_program workspace preset import OK: %s\n",
               ctx->bridge_workspace_preset_path ? ctx->bridge_workspace_preset_path : "(null)");
    }

    ctx->runtime_started = 1u;
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
    if (!ctx->runtime_started) {
        return drawing_program_invalid("runtime must start before run loop");
    }

    frames = ctx->headless ? ctx->smoke_frames : 1u;
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
            ctx->viewport_sample_probe_success_total += 1u;
        }
        result = drawing_program_frame_render_submit(ctx);
        if (result.code != CORE_OK) {
            return result;
        }
        ctx->frame_counter += 1u;
    }

    return core_result_ok();
}

CoreResult drawing_program_app_shutdown(DrawingProgramAppContext *ctx) {
    CoreResult result = core_result_ok();
    if (!ctx) {
        return drawing_program_invalid("null app context");
    }
    if (!ctx->persist_enabled) {
        if (ctx->export_json_requested) {
            result = drawing_program_snapshot_export_debug_json(ctx, ctx->export_json_path);
            if (result.code != CORE_OK) {
                drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
                return result;
            }
        }
        drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
        return core_result_ok();
    }
    result = drawing_program_ensure_parent_dir(ctx->preset_path);
    if (result.code != CORE_OK) {
        drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
        return result;
    }
    result = drawing_program_snapshot_save(ctx, ctx->preset_path);
    if (drawing_program_trace_ui_state_enabled()) {
        fprintf(stderr,
                "drawing_program trace shutdown save_result code=%d path=%s tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u color=%u\n",
                (int)result.code,
                ctx->preset_path ? ctx->preset_path : "(null)",
                (unsigned)ctx->editor.active_tool,
                (unsigned)ctx->ui_theme_preset_id,
                (unsigned)ctx->ui_font_preset_id,
                (int)ctx->ui_font_zoom_step,
                (unsigned)ctx->ui_left_panel_slot,
                (unsigned)ctx->ui_right_panel_slot,
                (unsigned)ctx->ui_active_color_index);
    }
    if (result.code != CORE_OK) {
        fprintf(stderr,
                "drawing_program: snapshot save failed code=%d message=%s path=%s\n",
                (int)result.code,
                result.message ? result.message : "(null)",
                ctx->preset_path ? ctx->preset_path : "(null)");
        drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
        return result;
    }
    if (ctx->export_json_requested) {
        result = drawing_program_snapshot_export_debug_json(ctx, ctx->export_json_path);
        if (result.code != CORE_OK) {
            fprintf(stderr,
                    "drawing_program: snapshot debug export failed code=%d message=%s json=%s\n",
                    (int)result.code,
                    result.message ? result.message : "(null)",
                    ctx->export_json_path ? ctx->export_json_path : "(null)");
            drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
            return result;
        }
        printf("drawing_program snapshot debug export OK: preset=%s json=%s\n",
               ctx->preset_path ? ctx->preset_path : "(null)",
               ctx->export_json_path ? ctx->export_json_path : "(null)");
    }
    drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
    return core_result_ok();
}

CoreResult drawing_program_app_set_pane_host_bounds(DrawingProgramAppContext *ctx,
                                                     float width,
                                                     float height) {
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

int drawing_program_app_main(int argc, char **argv) {
    DrawingProgramAppContext app;
    DrawingProgramAppStage stage = DRAWING_PROGRAM_APP_STAGE_INIT;
    CoreResult result;

    result = drawing_program_app_bootstrap(&app, argc, argv);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: bootstrap failed code=%d message=%s\n", (int)result.code, result.message);
        return 1;
    }
    drawing_program_lifecycle_note(&app, "bootstrap");
    if (!drawing_program_transition_stage(&stage,
                                          DRAWING_PROGRAM_APP_STAGE_INIT,
                                          DRAWING_PROGRAM_APP_STAGE_BOOTSTRAPPED,
                                          "bootstrap")) {
        return 1;
    }

    result = drawing_program_app_config_load(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: config_load failed code=%d message=%s\n", (int)result.code, result.message);
        return 1;
    }
    drawing_program_lifecycle_note(&app, "config_load");
    if (!drawing_program_transition_stage(&stage,
                                          DRAWING_PROGRAM_APP_STAGE_BOOTSTRAPPED,
                                          DRAWING_PROGRAM_APP_STAGE_CONFIG_LOADED,
                                          "config_load")) {
        return 1;
    }

    result = drawing_program_app_state_seed(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: state_seed failed code=%d message=%s\n", (int)result.code, result.message);
        return 1;
    }
    drawing_program_lifecycle_note(&app, "state_seed");
    if (!drawing_program_transition_stage(&stage,
                                          DRAWING_PROGRAM_APP_STAGE_CONFIG_LOADED,
                                          DRAWING_PROGRAM_APP_STAGE_STATE_SEEDED,
                                          "state_seed")) {
        return 1;
    }

    result = drawing_program_app_subsystems_init(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: subsystems_init failed code=%d message=%s\n", (int)result.code, result.message);
        return 1;
    }
    drawing_program_lifecycle_note(&app, "subsystems_init");
    if (!drawing_program_transition_stage(&stage,
                                          DRAWING_PROGRAM_APP_STAGE_STATE_SEEDED,
                                          DRAWING_PROGRAM_APP_STAGE_SUBSYSTEMS_READY,
                                          "subsystems_init")) {
        return 1;
    }

    result = drawing_program_runtime_start(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: runtime_start failed code=%d message=%s\n", (int)result.code, result.message);
        return 1;
    }
    drawing_program_lifecycle_note(&app, "runtime_start");
    if (!drawing_program_transition_stage(&stage,
                                          DRAWING_PROGRAM_APP_STAGE_SUBSYSTEMS_READY,
                                          DRAWING_PROGRAM_APP_STAGE_RUNTIME_STARTED,
                                          "runtime_start")) {
        return 1;
    }

    result = drawing_program_app_run_loop(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: run_loop failed code=%d message=%s\n", (int)result.code, result.message);
        return 1;
    }
    drawing_program_lifecycle_note(&app, "run_loop");
    if (!drawing_program_transition_stage(&stage,
                                          DRAWING_PROGRAM_APP_STAGE_RUNTIME_STARTED,
                                          DRAWING_PROGRAM_APP_STAGE_LOOP_COMPLETED,
                                          "run_loop")) {
        return 1;
    }

    result = drawing_program_app_shutdown(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: shutdown failed code=%d message=%s\n", (int)result.code, result.message);
        return 1;
    }
    drawing_program_lifecycle_note(&app, "shutdown");
    if (!drawing_program_transition_stage(&stage,
                                          DRAWING_PROGRAM_APP_STAGE_LOOP_COMPLETED,
                                          DRAWING_PROGRAM_APP_STAGE_SHUTDOWN_COMPLETED,
                                          "shutdown")) {
        return 1;
    }

    if (app.print_lifecycle) {
        printf("drawing_program lifecycle complete: frames=%llu headless=%u events=%llu actions=%llu "
               "route[g=%llu p=%llu f=%llu] invalidate[target=%llu full=%llu] viewport_probe_ok=%llu "
               "render[frames=%llu visible_layers=%llu full=%llu target=%llu module_calls=%llu]\n",
               (unsigned long long)app.frame_counter,
               (unsigned)app.headless,
               (unsigned long long)app.input_events_processed,
               (unsigned long long)app.input_actions_emitted,
               (unsigned long long)app.routed_global_total,
               (unsigned long long)app.routed_pane_total,
               (unsigned long long)app.routed_fallback_total,
               (unsigned long long)app.invalidation_target_total,
               (unsigned long long)app.invalidation_full_total,
               (unsigned long long)app.viewport_sample_probe_success_total,
               (unsigned long long)app.render_frames_projected_total,
               (unsigned long long)app.render_layers_visible_total,
               (unsigned long long)app.render_full_redraw_total,
               (unsigned long long)app.render_target_redraw_total,
               (unsigned long long)app.render_module_calls_total);
    }

    return 0;
}
