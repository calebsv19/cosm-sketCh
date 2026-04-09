#include <stdio.h>
#include <unistd.h>

#include "drawing_program/drawing_program_app_main.h"

static int expect_ok(CoreResult result, const char *label) {
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: %s failed code=%d message=%s\n", label, (int)result.code, result.message);
        return 0;
    }
    return 1;
}

static int expect_overlay_ok(DrawingProgramOverlayAdapterResult result, const char *label) {
    if (!result.ok) {
        fprintf(stderr, "overlay_test: %s failed code=%d reason=%s\n",
                label, (int)result.error_code, result.reason ? result.reason : "n/a");
        return 0;
    }
    return 1;
}

int main(void) {
    DrawingProgramAppContext ctx;
    char arg0[] = "drawing_program_test";
    char arg1[] = "--headless";
    char arg2[] = "--smoke-frames";
    char arg3[] = "2";
    char *argv[] = { arg0, arg1, arg2, arg3, 0 };

    if (!expect_ok(drawing_program_app_bootstrap(&ctx, 4, argv), "bootstrap")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&ctx), "config_load")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&ctx), "state_seed")) {
        return 1;
    }
    {
        DrawingProgramViewportTransform transform;
        DrawingProgramScreenPoint screen = { 10.0f, 20.0f };
        DrawingProgramCanvasPoint canvas;
        DrawingProgramSamplePoint sample;
        DrawingProgramCanvasPoint sample_center;
        drawing_program_viewport_transform_from_state(&ctx.editor, &ctx.document, &transform);
        canvas = drawing_program_screen_to_canvas(transform, screen);
        if (canvas.x != 10.0f || canvas.y != 20.0f) {
            fprintf(stderr, "viewport_test: expected identity screen->canvas mapping got %.3f %.3f\n",
                    (double)canvas.x, (double)canvas.y);
            return 1;
        }
        if (!drawing_program_screen_to_sample(transform, screen, &sample) || sample.x != 10u || sample.y != 20u) {
            fprintf(stderr, "viewport_test: expected sample (10,20)\n");
            return 1;
        }
        sample_center = drawing_program_sample_to_canvas_center(transform, sample);
        if (sample_center.x <= 10.0f || sample_center.x >= 11.0f ||
            sample_center.y <= 20.0f || sample_center.y >= 21.0f) {
            fprintf(stderr, "viewport_test: expected sample center inside pixel cell\n");
            return 1;
        }
        ctx.editor.viewport.pan_x = 50.0f;
        ctx.editor.viewport.pan_y = 25.0f;
        ctx.editor.viewport.zoom = 2.0f;
        drawing_program_viewport_transform_from_state(&ctx.editor, &ctx.document, &transform);
        canvas = drawing_program_screen_to_canvas(transform, (DrawingProgramScreenPoint){ 150.0f, 125.0f });
        if (canvas.x != 50.0f || canvas.y != 50.0f) {
            fprintf(stderr, "viewport_test: expected pan/zoom mapped canvas (50,50), got %.3f %.3f\n",
                    (double)canvas.x, (double)canvas.y);
            return 1;
        }
    }
    if (ctx.document.layer_count != 1u || ctx.document.layers[0].layer_id != 1u) {
        fprintf(stderr, "lifecycle_test: unexpected document seed state layer_count=%u\n", ctx.document.layer_count);
        return 1;
    }
    if (ctx.editor.active_tool != DRAWING_PROGRAM_TOOL_BRUSH || ctx.editor.active_layer_id != 1u) {
        fprintf(stderr, "lifecycle_test: unexpected editor seed state tool=%u layer=%u\n",
                (unsigned)ctx.editor.active_tool,
                (unsigned)ctx.editor.active_layer_id);
        return 1;
    }
    if (ctx.history.count != 0u || ctx.history.cursor != 0u) {
        fprintf(stderr, "lifecycle_test: expected empty history at seed\n");
        return 1;
    }
    if (ctx.pane_host.leaf_count != 2u || ctx.pane_host.module_binding_count != 2u) {
        fprintf(stderr, "lifecycle_test: unexpected pane host seed state leaf_count=%u binding_count=%u\n",
                ctx.pane_host.leaf_count,
                ctx.pane_host.module_binding_count);
        return 1;
    }
    if (!expect_overlay_ok(drawing_program_adapter_overlay_session_begin(&ctx), "overlay_session_begin")) {
        return 1;
    }
    if (!expect_overlay_ok(drawing_program_adapter_snapshot_stage_draft(&ctx), "snapshot_stage_draft")) {
        return 1;
    }
    {
        DrawingProgramOverlayAdapterResult dirty_exit = drawing_program_adapter_overlay_session_end(&ctx);
        if (dirty_exit.ok ||
            dirty_exit.error_code != DRAWING_PROGRAM_OVERLAY_ADAPTER_DIRTY_EXIT_REQUIRES_APPLY_OR_CANCEL) {
            fprintf(stderr, "overlay_test: expected dirty-exit guard rejection\n");
            return 1;
        }
    }
    if (!expect_overlay_ok(drawing_program_adapter_snapshot_commit_draft(&ctx), "snapshot_commit_draft")) {
        return 1;
    }
    if (ctx.overlay_adapter.lifecycle_state != DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE ||
        ctx.pane_host.layout_state.mode != CORE_LAYOUT_MODE_RUNTIME) {
        fprintf(stderr, "overlay_test: expected runtime active after commit\n");
        return 1;
    }
    if (!expect_overlay_ok(drawing_program_adapter_overlay_session_begin(&ctx), "overlay_session_begin_2")) {
        return 1;
    }
    if (!expect_overlay_ok(drawing_program_adapter_snapshot_stage_draft(&ctx), "snapshot_stage_draft_2")) {
        return 1;
    }
    if (!expect_overlay_ok(drawing_program_adapter_snapshot_discard_draft(&ctx), "snapshot_discard_draft")) {
        return 1;
    }
    if (ctx.overlay_adapter.lifecycle_state != DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE ||
        ctx.pane_host.layout_state.mode != CORE_LAYOUT_MODE_RUNTIME) {
        fprintf(stderr, "overlay_test: expected runtime active after discard\n");
        return 1;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&ctx), "subsystems_init")) {
        return 1;
    }
    if (!expect_ok(drawing_program_history_apply_set_layer_visibility(&ctx.history,
                                                                      &ctx.document,
                                                                      1u,
                                                                      0u),
                   "history_apply_set_layer_visibility")) {
        return 1;
    }
    if (ctx.document.layers[0].visible != 0u || ctx.history.count != 1u || ctx.history.cursor != 1u) {
        fprintf(stderr, "lifecycle_test: expected one applied visibility command\n");
        return 1;
    }
    if (!expect_ok(drawing_program_history_undo(&ctx.history, &ctx.document), "history_undo")) {
        return 1;
    }
    if (ctx.document.layers[0].visible != 1u || ctx.history.cursor != 0u) {
        fprintf(stderr, "lifecycle_test: expected visibility restored after undo\n");
        return 1;
    }
    if (!expect_ok(drawing_program_history_redo(&ctx.history, &ctx.document), "history_redo")) {
        return 1;
    }
    if (ctx.document.layers[0].visible != 0u || ctx.history.cursor != 1u) {
        fprintf(stderr, "lifecycle_test: expected visibility re-applied after redo\n");
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&ctx), "runtime_start")) {
        return 1;
    }
    {
        const char *pack_path = "/tmp/drawing_program_test_snapshot.pack";
        const char *json_path = "/tmp/drawing_program_test_snapshot.json";
        if (!expect_ok(drawing_program_snapshot_save(&ctx, pack_path), "snapshot_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&ctx, pack_path), "snapshot_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_export_debug_json(&ctx, json_path), "snapshot_export_json")) {
            return 1;
        }
        if (access(json_path, F_OK) != 0) {
            fprintf(stderr, "lifecycle_test: expected debug json output at %s\n", json_path);
            return 1;
        }
    }
    if (!expect_ok(drawing_program_app_run_loop(&ctx), "run_loop")) {
        return 1;
    }
    if (ctx.viewport_sample_probe_success_total != 2u) {
        fprintf(stderr, "lifecycle_test: expected viewport probe success count 2 got=%llu\n",
                (unsigned long long)ctx.viewport_sample_probe_success_total);
        return 1;
    }
    if (!expect_ok(drawing_program_app_shutdown(&ctx), "shutdown")) {
        return 1;
    }
    if (ctx.frame_counter != 2u) {
        fprintf(stderr, "lifecycle_test: expected frame_counter=2 got=%llu\n",
                (unsigned long long)ctx.frame_counter);
        return 1;
    }
    if (ctx.input_events_processed != 3u) {
        fprintf(stderr, "lifecycle_test: expected input_events_processed=3 got=%llu\n",
                (unsigned long long)ctx.input_events_processed);
        return 1;
    }
    if (ctx.input_actions_emitted != 3u) {
        fprintf(stderr, "lifecycle_test: expected input_actions_emitted=3 got=%llu\n",
                (unsigned long long)ctx.input_actions_emitted);
        return 1;
    }
    if (ctx.routed_global_total != 1u || ctx.routed_pane_total != 2u || ctx.routed_fallback_total != 0u) {
        fprintf(stderr,
                "lifecycle_test: unexpected route totals g=%llu p=%llu f=%llu\n",
                (unsigned long long)ctx.routed_global_total,
                (unsigned long long)ctx.routed_pane_total,
                (unsigned long long)ctx.routed_fallback_total);
        return 1;
    }
    if (ctx.invalidation_target_total != 1u || ctx.invalidation_full_total != 1u) {
        fprintf(stderr,
                "lifecycle_test: unexpected invalidation totals target=%llu full=%llu\n",
                (unsigned long long)ctx.invalidation_target_total,
                (unsigned long long)ctx.invalidation_full_total);
        return 1;
    }
    if (ctx.render_frames_projected_total != 2u ||
        ctx.render_layers_visible_total != 2u ||
        ctx.render_full_redraw_total != 1u ||
        ctx.render_target_redraw_total != 1u) {
        fprintf(stderr,
                "lifecycle_test: unexpected render projection totals frames=%llu visible=%llu full=%llu target=%llu\n",
                (unsigned long long)ctx.render_frames_projected_total,
                (unsigned long long)ctx.render_layers_visible_total,
                (unsigned long long)ctx.render_full_redraw_total,
                (unsigned long long)ctx.render_target_redraw_total);
        return 1;
    }
    if (ctx.render_module_calls_total != 4u ||
        ctx.render_module_canvas_calls_total != 2u ||
        ctx.render_module_palette_calls_total != 2u) {
        fprintf(stderr,
                "lifecycle_test: unexpected module render call totals total=%llu canvas=%llu palette=%llu\n",
                (unsigned long long)ctx.render_module_calls_total,
                (unsigned long long)ctx.render_module_canvas_calls_total,
                (unsigned long long)ctx.render_module_palette_calls_total);
        return 1;
    }
    if (!ctx.render_last_has_active_layer || ctx.render_last_active_layer_id != 1u) {
        fprintf(stderr,
                "lifecycle_test: expected active layer projection id=1 has_active=1 got id=%u has_active=%u\n",
                ctx.render_last_active_layer_id,
                (unsigned)ctx.render_last_has_active_layer);
        return 1;
    }
    return 0;
}
