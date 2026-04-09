#include "drawing_program/drawing_program_app_main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void drawing_program_input_intake(uint64_t frame_index,
                                         uint32_t total_frames,
                                         DrawingProgramInputEventRaw *out_raw) {
    if (!out_raw) {
        return;
    }
    memset(out_raw, 0, sizeof(*out_raw));
    out_raw->frame_index = frame_index;

    out_raw->pointer_event_count = 1u;
    out_raw->event_count = 1u;

    if (frame_index + 1u >= (uint64_t)total_frames) {
        out_raw->quit_event_count = 1u;
        out_raw->quit_requested = 1u;
        out_raw->event_count += 1u;
    }
}

static void drawing_program_input_normalize(const DrawingProgramInputEventRaw *raw,
                                            DrawingProgramInputEventNormalized *out_normalized) {
    if (!raw || !out_normalized) {
        return;
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
    out_normalized->immediate_action_count = out_normalized->action_count;
    out_normalized->queued_action_count = 0u;
    out_normalized->ignored_action_count = raw->other_event_count;
}

static void drawing_program_input_route(const DrawingProgramInputEventNormalized *normalized,
                                        DrawingProgramInputRouteResult *out_route) {
    if (!normalized || !out_route) {
        return;
    }
    memset(out_route, 0, sizeof(*out_route));
    out_route->target_policy = DRAWING_PROGRAM_INPUT_ROUTE_TARGET_FALLBACK;

    if (normalized->has_quit_action || normalized->has_window_action || normalized->has_keyboard_action) {
        if (normalized->has_quit_action) {
            out_route->routed_global_count += 1u;
        }
        if (normalized->has_window_action) {
            out_route->routed_global_count += 1u;
        }
        if (normalized->has_keyboard_action) {
            out_route->routed_global_count += 1u;
        }
        out_route->target_policy = DRAWING_PROGRAM_INPUT_ROUTE_TARGET_GLOBAL;
        out_route->consumed = 1u;
    }

    if (normalized->has_pointer_action || normalized->has_wheel_action) {
        if (normalized->has_pointer_action) {
            out_route->routed_pane_count += 1u;
        }
        if (normalized->has_wheel_action) {
            out_route->routed_pane_count += 1u;
        }
        if (out_route->target_policy == DRAWING_PROGRAM_INPUT_ROUTE_TARGET_FALLBACK) {
            out_route->target_policy = DRAWING_PROGRAM_INPUT_ROUTE_TARGET_PANE;
        }
        out_route->consumed = 1u;
    }

    if (!out_route->consumed) {
        out_route->routed_fallback_count = normalized->action_count > 0u ? normalized->action_count : 1u;
        out_route->target_policy = DRAWING_PROGRAM_INPUT_ROUTE_TARGET_FALLBACK;
    }
}

static void drawing_program_input_invalidate(const DrawingProgramInputEventNormalized *normalized,
                                             const DrawingProgramInputRouteResult *route,
                                             DrawingProgramInputInvalidationResult *out_invalidation) {
    uint32_t bits = 0u;
    if (!normalized || !route || !out_invalidation) {
        return;
    }
    memset(out_invalidation, 0, sizeof(*out_invalidation));

    if (normalized->has_quit_action) {
        bits |= DRAWING_PROGRAM_INPUT_INVALIDATE_REASON_QUIT;
    }
    if (normalized->has_window_action) {
        bits |= DRAWING_PROGRAM_INPUT_INVALIDATE_REASON_WINDOW;
    }
    if (normalized->has_keyboard_action) {
        bits |= DRAWING_PROGRAM_INPUT_INVALIDATE_REASON_KEYBOARD;
    }
    if (normalized->has_pointer_action) {
        bits |= DRAWING_PROGRAM_INPUT_INVALIDATE_REASON_POINTER;
    }
    if (normalized->has_wheel_action) {
        bits |= DRAWING_PROGRAM_INPUT_INVALIDATE_REASON_WHEEL;
    }

    out_invalidation->invalidation_reason_bits = bits;
    out_invalidation->full_invalidate =
        (normalized->has_quit_action || normalized->has_window_action) ? 1u : 0u;
    if (out_invalidation->full_invalidate) {
        out_invalidation->full_invalidation_count = 1u;
    } else if (route->consumed) {
        out_invalidation->target_invalidation_count = 1u;
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

CoreResult drawing_program_app_bootstrap(DrawingProgramAppContext *ctx, int argc, char **argv) {
    int i;
    if (!ctx) {
        return drawing_program_invalid("null app context");
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->smoke_frames = 1u;
    ctx->preset_path = "data/last_session.pack";
    ctx->export_json_path = 0;
    ctx->bridge_workspace_preset_path = "workspace_sandbox/data/presets/sketch_layout_v1.pack";

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
    }

    return core_result_ok();
}

CoreResult drawing_program_app_config_load(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return drawing_program_invalid("null app context");
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
    drawing_program_editor_state_init(&ctx->editor, &ctx->document);
    drawing_program_history_init(&ctx->history);
    adapter_result = drawing_program_overlay_adapter_init(ctx);
    if (!adapter_result.ok) {
        CoreResult err = { CORE_ERR_FORMAT, adapter_result.reason };
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
    if (!ctx) {
        return drawing_program_invalid("null app context");
    }
    if (!ctx->subsystems_ready) {
        return drawing_program_invalid("subsystems must be initialized before runtime start");
    }

    result = drawing_program_snapshot_load(ctx, ctx->preset_path);
    if (result.code != CORE_OK) {
        (void)drawing_program_snapshot_save(ctx, ctx->preset_path);
    }

    if (ctx->bridge_workspace_check_requested) {
        result = drawing_program_snapshot_bridge_check_workspace_preset(ctx->bridge_workspace_preset_path);
        if (result.code != CORE_OK) {
            return result;
        }
        printf("drawing_program workspace preset bridge check OK: %s\n",
               ctx->bridge_workspace_preset_path ? ctx->bridge_workspace_preset_path : "(null)");
    }

    if (ctx->export_json_requested) {
        result = drawing_program_snapshot_export_debug_json(ctx, ctx->export_json_path);
        if (result.code != CORE_OK) {
            return result;
        }
        printf("drawing_program snapshot debug export OK: preset=%s json=%s\n",
               ctx->preset_path ? ctx->preset_path : "(null)",
               ctx->export_json_path ? ctx->export_json_path : "(null)");
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
        DrawingProgramOverlayAdapterResult adapter_result;
        DrawingProgramScreenPoint screen_probe;
        DrawingProgramSamplePoint sample_probe;
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
        drawing_program_input_intake((uint64_t)i, frames, &raw);
        drawing_program_input_normalize(&raw, &normalized);
        drawing_program_input_route(&normalized, &route);
        drawing_program_input_invalidate(&normalized, &route, &invalidation);
        result = drawing_program_render_project_and_update_counters(ctx, &invalidation);
        if (result.code != CORE_OK) {
            return result;
        }
        drawing_program_viewport_transform_from_state(&ctx->editor, &ctx->document, &viewport_transform);
        screen_probe.x = 100.0f + (float)i;
        screen_probe.y = 100.0f + (float)i;
        if (drawing_program_screen_to_sample(viewport_transform, screen_probe, &sample_probe)) {
            ctx->viewport_sample_probe_success_total += 1u;
        }
        adapter_result = drawing_program_adapter_render_runtime_base(ctx);
        if (!adapter_result.ok) {
            CoreResult err = { CORE_ERR_FORMAT, adapter_result.reason };
            return err;
        }

        ctx->input_events_processed += (uint64_t)raw.event_count;
        ctx->input_actions_emitted += (uint64_t)normalized.action_count;
        ctx->routed_global_total += (uint64_t)route.routed_global_count;
        ctx->routed_pane_total += (uint64_t)route.routed_pane_count;
        ctx->routed_fallback_total += (uint64_t)route.routed_fallback_count;
        ctx->invalidation_target_total += (uint64_t)invalidation.target_invalidation_count;
        ctx->invalidation_full_total += (uint64_t)invalidation.full_invalidation_count;
        ctx->invalidation_reason_bits_total += (uint64_t)invalidation.invalidation_reason_bits;
        ctx->frame_counter += 1u;
    }

    return core_result_ok();
}

CoreResult drawing_program_app_shutdown(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return drawing_program_invalid("null app context");
    }
    (void)drawing_program_snapshot_save(ctx, ctx->preset_path);
    return core_result_ok();
}

int drawing_program_app_main(int argc, char **argv) {
    DrawingProgramAppContext app;
    DrawingProgramAppStage stage = DRAWING_PROGRAM_APP_STAGE_INIT;
    CoreResult result;

    result = drawing_program_app_bootstrap(&app, argc, argv);
    if (result.code != CORE_OK) {
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
