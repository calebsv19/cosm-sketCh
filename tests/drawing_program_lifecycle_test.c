#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core_theme.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"

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

static int expect_overlay_error_code(DrawingProgramOverlayAdapterResult result,
                                     DrawingProgramOverlayAdapterErrorCode code,
                                     const char *label) {
    if (result.ok || result.error_code != code) {
        fprintf(stderr, "overlay_test: %s expected error_code=%d got ok=%u code=%d reason=%s\n",
                label,
                (int)code,
                (unsigned)result.ok,
                (int)result.error_code,
                result.reason ? result.reason : "n/a");
        return 0;
    }
    return 1;
}

int main(void) {
    DrawingProgramAppContext ctx;
    DrawingProgramAppContext workflow_ctx;
    uint32_t center_x;
    uint32_t center_y;
    uint32_t workflow_center_x;
    uint32_t workflow_center_y;
    uint8_t workflow_center_value = 0u;
    uint8_t center_before = 0u;
    uint8_t center_after = 0u;
    uint8_t center_undo = 0u;
    uint8_t center_redo = 0u;
    char arg0[] = "drawing_program_test";
    char arg1[] = "--headless";
    char arg2[] = "--smoke-frames";
    char arg3[] = "2";
    char arg4[] = "--no-persist";
    char *argv[] = { arg0, arg1, arg2, arg3, arg4, 0 };
    uint8_t expected_draw_value = drawing_program_color_value_from_index(drawing_program_color_default_index());
    uint8_t expected_eraser_value = drawing_program_color_eraser_value();

    if (!expect_ok(drawing_program_app_bootstrap(&ctx, 5, argv), "bootstrap")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&ctx), "config_load")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&ctx), "state_seed")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_bootstrap(&workflow_ctx, 5, argv), "workflow_bootstrap")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&workflow_ctx), "workflow_config_load")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&workflow_ctx), "workflow_state_seed")) {
        return 1;
    }
    workflow_center_x = workflow_ctx.document.raster_width / 2u;
    workflow_center_y = workflow_ctx.document.raster_height / 2u;
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_ERASER),
                   "workflow_set_tool_eraser")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE),
                   "workflow_stamp_eraser")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document,
                                                        workflow_center_x,
                                                        workflow_center_y,
                                                        &workflow_center_value),
                   "workflow_sample_after_eraser_stamp")) {
        return 1;
    }
    if (workflow_center_value != expected_eraser_value) {
        fprintf(stderr,
                "lifecycle_test: expected workflow eraser stamp to set center sample to %u got=%u\n",
                (unsigned)expected_eraser_value,
                (unsigned)workflow_center_value);
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_BRUSH),
                   "workflow_set_tool_brush")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE),
                   "workflow_stamp_brush")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document,
                                                        workflow_center_x,
                                                        workflow_center_y,
                                                        &workflow_center_value),
                   "workflow_sample_after_brush_stamp")) {
        return 1;
    }
    if (workflow_center_value != expected_draw_value) {
        fprintf(stderr,
                "lifecycle_test: expected workflow brush stamp to set center sample to %u got=%u\n",
                (unsigned)expected_draw_value,
                (unsigned)workflow_center_value);
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY),
                   "workflow_toggle_layer_visibility")) {
        return 1;
    }
    if (workflow_ctx.document.layers[0].visible != 0u) {
        fprintf(stderr, "lifecycle_test: expected workflow toggle to hide active layer\n");
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_UNDO),
                   "workflow_undo")) {
        return 1;
    }
    if (workflow_ctx.document.layers[0].visible != 1u) {
        fprintf(stderr, "lifecycle_test: expected workflow undo to restore active layer visibility\n");
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_REDO),
                   "workflow_redo")) {
        return 1;
    }
    if (workflow_ctx.document.layers[0].visible != 0u) {
        fprintf(stderr, "lifecycle_test: expected workflow redo to hide active layer again\n");
        return 1;
    }
    if (workflow_ctx.tool_switch_total != 2u) {
        fprintf(stderr,
                "lifecycle_test: expected workflow tool switch total=2 got=%llu\n",
                (unsigned long long)workflow_ctx.tool_switch_total);
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
    if (ctx.document.raster_width == 0u ||
        ctx.document.raster_height == 0u ||
        ctx.document.raster_sample_count == 0u) {
        fprintf(stderr, "lifecycle_test: expected seeded raster baseline\n");
        return 1;
    }
    center_x = ctx.document.raster_width / 2u;
    center_y = ctx.document.raster_height / 2u;
    if (!expect_ok(drawing_program_document_sample_read(&ctx.document, center_x, center_y, &center_before),
                   "sample_read_center_before")) {
        return 1;
    }
    if (center_before != expected_eraser_value) {
        fprintf(stderr,
                "lifecycle_test: expected seeded center sample to use eraser/background value %u got=%u\n",
                (unsigned)expected_eraser_value,
                (unsigned)center_before);
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
    if (ctx.pane_host.leaf_count != 4u || ctx.pane_host.module_binding_count != 4u) {
        fprintf(stderr, "lifecycle_test: unexpected pane host seed state leaf_count=%u binding_count=%u\n",
                ctx.pane_host.leaf_count,
                ctx.pane_host.module_binding_count);
        return 1;
    }
    {
        DrawingProgramDocument legacy_doc;
        DrawingProgramDocument legacy_doc_v2;
        uint8_t upgraded = 0u;
        uint8_t upgraded_v2 = 0u;
        uint8_t preserved_sample = 0u;
        uint32_t x;
        uint32_t y;
        memset(&legacy_doc, 0, sizeof(legacy_doc));
        legacy_doc.schema_version = 1u;
        legacy_doc.raster_width = 32u;
        legacy_doc.raster_height = 32u;
        legacy_doc.raster_sample_count = legacy_doc.raster_width * legacy_doc.raster_height;
        for (y = 0u; y < legacy_doc.raster_height; ++y) {
            for (x = 0u; x < legacy_doc.raster_width; ++x) {
                uint32_t idx = y * legacy_doc.raster_width + x;
                legacy_doc.raster_samples[idx] = (((x / 16u) + (y / 16u)) & 1u) ? 44u : 24u;
            }
        }
        legacy_doc.raster_samples[5u * legacy_doc.raster_width + 9u] = 200u;
        if (!expect_ok(drawing_program_document_upgrade_legacy_checker_seed(&legacy_doc, &upgraded),
                       "document_upgrade_legacy_checker_seed")) {
            return 1;
        }
        preserved_sample = legacy_doc.raster_samples[5u * legacy_doc.raster_width + 9u];
        if (upgraded != 1u ||
            legacy_doc.raster_samples[0] != expected_eraser_value ||
            preserved_sample != 200u ||
            legacy_doc.schema_version != 2u) {
            fprintf(stderr,
                    "lifecycle_test: expected checker legacy upgrade to flatten background and preserve edits"
                    " value=%u upgraded=%u sample0=%u preserved=%u schema=%u\n",
                    (unsigned)expected_eraser_value,
                    (unsigned)upgraded,
                    (unsigned)legacy_doc.raster_samples[0],
                    (unsigned)preserved_sample,
                    (unsigned)legacy_doc.schema_version);
            return 1;
        }
        memset(&legacy_doc_v2, 0, sizeof(legacy_doc_v2));
        legacy_doc_v2.schema_version = 2u;
        legacy_doc_v2.raster_width = 32u;
        legacy_doc_v2.raster_height = 32u;
        legacy_doc_v2.raster_sample_count = legacy_doc_v2.raster_width * legacy_doc_v2.raster_height;
        for (y = 0u; y < legacy_doc_v2.raster_height; ++y) {
            for (x = 0u; x < legacy_doc_v2.raster_width; ++x) {
                uint32_t idx = y * legacy_doc_v2.raster_width + x;
                legacy_doc_v2.raster_samples[idx] = (((x / 16u) + (y / 16u)) & 1u) ? 44u : 24u;
            }
        }
        if (!expect_ok(drawing_program_document_upgrade_legacy_checker_seed(&legacy_doc_v2, &upgraded_v2),
                       "document_upgrade_legacy_checker_seed_v2_recovery")) {
            return 1;
        }
        if (upgraded_v2 != 1u || legacy_doc_v2.raster_samples[0] != expected_eraser_value) {
            fprintf(stderr,
                    "lifecycle_test: expected v2 recovery upgrade to flatten checker seed value=%u upgraded=%u sample0=%u\n",
                    (unsigned)expected_eraser_value,
                    (unsigned)upgraded_v2,
                    (unsigned)legacy_doc_v2.raster_samples[0]);
            return 1;
        }
    }
    {
        DrawingProgramInputEventRaw raw = { 0 };
        DrawingProgramInputEventNormalized normalized = { 0 };
        DrawingProgramInputRouteResult route = { 0 };
        DrawingProgramInputInvalidationResult invalidation = { 0 };
        raw.pointer_event_count = 1u;
        raw.event_count = 1u;
        if (!expect_ok(drawing_program_runtime_orchestration_plan_frame(&raw, &normalized, &route, &invalidation),
                       "orchestration_plan_frame")) {
            return 1;
        }
        if (normalized.action_count != 1u || normalized.immediate_action_count != 1u || normalized.queued_action_count != 0u) {
            fprintf(stderr, "lifecycle_test: unexpected orchestration normalization totals action=%u immediate=%u queued=%u\n",
                    normalized.action_count,
                    normalized.immediate_action_count,
                    normalized.queued_action_count);
            return 1;
        }
        if (route.routed_pane_count != 1u || route.routed_global_count != 0u || invalidation.full_invalidate != 0u) {
            fprintf(stderr, "lifecycle_test: unexpected orchestration route/invalidation projection\n");
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_submit_deferred(&ctx, &normalized),
                       "orchestration_submit_deferred_empty")) {
            return 1;
        }
        normalized.queued_action_count = 1u;
        if (drawing_program_runtime_orchestration_submit_deferred(&ctx, &normalized).code == CORE_OK) {
            fprintf(stderr, "lifecycle_test: expected deferred submission guard failure when queued actions are present\n");
            return 1;
        }
    }
    if (!expect_overlay_ok(drawing_program_adapter_overlay_session_begin(&ctx), "overlay_session_begin")) {
        return 1;
    }
    if (!expect_overlay_error_code(drawing_program_adapter_runtime_tick(&ctx),
                                   DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                   "runtime_tick_blocked_in_authoring")) {
        return 1;
    }
    if (!expect_overlay_error_code(drawing_program_adapter_input_route_runtime(&ctx),
                                   DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                   "runtime_input_blocked_in_authoring")) {
        return 1;
    }
    if (!expect_overlay_error_code(drawing_program_adapter_render_runtime_base(&ctx),
                                   DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                   "runtime_render_blocked_in_authoring")) {
        return 1;
    }
    if (!expect_overlay_error_code(drawing_program_adapter_persist_save_session(&ctx),
                                   DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                   "persist_save_blocked_in_authoring")) {
        return 1;
    }
    ctx.export_json_path = "/tmp/drawing_program_overlay_authoring_blocked.json";
    if (!expect_overlay_error_code(drawing_program_adapter_persist_export_debug_json(&ctx),
                                   DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                   "persist_export_blocked_in_authoring")) {
        return 1;
    }
    if (!expect_overlay_ok(drawing_program_adapter_input_route_overlay(&ctx), "overlay_input_allowed_in_authoring")) {
        return 1;
    }
    if (!expect_overlay_ok(drawing_program_adapter_render_overlay_chrome(&ctx), "overlay_render_allowed_in_authoring")) {
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
    if (!expect_overlay_ok(drawing_program_adapter_runtime_tick(&ctx), "runtime_tick_after_authoring")) {
        return 1;
    }
    if (!expect_overlay_ok(drawing_program_adapter_input_route_runtime(&ctx), "runtime_input_after_authoring")) {
        return 1;
    }
    if (!expect_overlay_ok(drawing_program_adapter_render_runtime_base(&ctx), "runtime_render_after_authoring")) {
        return 1;
    }
    ctx.export_json_path = "/tmp/drawing_program_overlay_runtime_export.json";
    if (!expect_overlay_ok(drawing_program_adapter_persist_save_session(&ctx), "persist_save_after_authoring")) {
        return 1;
    }
    if (!expect_overlay_ok(drawing_program_adapter_persist_export_debug_json(&ctx), "persist_export_after_authoring")) {
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
    if (!expect_ok(drawing_program_history_apply_set_layer_visibility(&ctx.history,
                                                                      &ctx.document,
                                                                      1u,
                                                                      0u),
                   "history_apply_set_layer_visibility_noop")) {
        return 1;
    }
    if (ctx.history.count != 1u || ctx.history.cursor != 1u) {
        fprintf(stderr, "lifecycle_test: expected no-op visibility command to avoid history push\n");
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
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &ctx,
                       DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_HISTORY),
                   "workflow_clear_history")) {
        return 1;
    }
    if (ctx.history.count != 0u || ctx.history.cursor != 0u) {
        fprintf(stderr, "lifecycle_test: expected clear history to reset count/cursor\n");
        return 1;
    }
    {
        uint32_t unit_cursor = 0u;
        uint32_t unit_count = 0u;
        uint8_t before_a = 0u;
        uint8_t before_b = 0u;
        uint8_t after_undo_a = 0u;
        uint8_t after_undo_b = 0u;
        uint8_t after_redo_a = 0u;
        uint8_t after_redo_b = 0u;
        uint32_t ax = center_x;
        uint32_t ay = center_y;
        uint32_t bx = (center_x + 1u < ctx.document.raster_width) ? (center_x + 1u) : center_x;
        uint32_t by = center_y;
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, ax, ay, &before_a), "group_seed_read_a")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, bx, by, &before_b), "group_seed_read_b")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_begin_group(&ctx.history), "history_begin_group")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&ctx.history, &ctx.document, ax, ay, 200u),
                       "history_group_set_sample_a")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&ctx.history, &ctx.document, bx, by, 201u),
                       "history_group_set_sample_b")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_end_group(&ctx.history), "history_end_group")) {
            return 1;
        }
        drawing_program_history_query_units(&ctx.history, &unit_cursor, &unit_count);
        if (unit_cursor != 1u || unit_count != 1u) {
            fprintf(stderr, "lifecycle_test: expected grouped history units 1/1 got %u/%u\n", unit_cursor, unit_count);
            return 1;
        }
        if (!expect_ok(drawing_program_history_undo(&ctx.history, &ctx.document), "history_group_undo")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, ax, ay, &after_undo_a), "group_undo_read_a")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, bx, by, &after_undo_b), "group_undo_read_b")) {
            return 1;
        }
        if (after_undo_a != before_a || after_undo_b != before_b) {
            fprintf(stderr, "lifecycle_test: grouped undo did not restore both samples\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_redo(&ctx.history, &ctx.document), "history_group_redo")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, ax, ay, &after_redo_a), "group_redo_read_a")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, bx, by, &after_redo_b), "group_redo_read_b")) {
            return 1;
        }
        if (after_redo_a != 200u || after_redo_b != 201u) {
            fprintf(stderr, "lifecycle_test: grouped redo did not reapply both samples\n");
            return 1;
        }
    }
    {
        CoreResult undo_after_clear = drawing_program_history_undo(&ctx.history, &ctx.document);
        if (undo_after_clear.code != CORE_OK) {
            fprintf(stderr, "lifecycle_test: expected undo after grouped write to succeed\n");
            return 1;
        }
        undo_after_clear = drawing_program_history_undo(&ctx.history, &ctx.document);
        if (undo_after_clear.code != CORE_ERR_NOT_FOUND) {
            fprintf(stderr, "lifecycle_test: expected undo stack exhaustion to return not_found\n");
            return 1;
        }
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
    if (!expect_ok(drawing_program_document_sample_read(&ctx.document, center_x, center_y, &center_after),
                   "sample_read_center_after")) {
        return 1;
    }
    if (center_after != expected_draw_value) {
        fprintf(stderr, "lifecycle_test: expected brush seed to set center sample to %u got=%u\n",
                (unsigned)expected_draw_value,
                (unsigned)center_after);
        return 1;
    }
    if (!expect_ok(drawing_program_history_apply_set_sample_value(&ctx.history,
                                                                  &ctx.document,
                                                                  center_x,
                                                                  center_y,
                                                                  center_after),
                   "history_apply_set_sample_value_noop")) {
        return 1;
    }
    if (ctx.history.count != 1u || ctx.history.cursor != 1u) {
        fprintf(stderr,
                "lifecycle_test: expected no-op sample command to avoid history push count=%u cursor=%u\n",
                ctx.history.count,
                ctx.history.cursor);
        return 1;
    }
    if (ctx.editor.active_tool != DRAWING_PROGRAM_TOOL_SELECT || ctx.tool_switch_total != 1u) {
        fprintf(stderr,
                "lifecycle_test: expected tool switch to SELECT with count=1 got tool=%u switches=%llu\n",
                (unsigned)ctx.editor.active_tool,
                (unsigned long long)ctx.tool_switch_total);
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
    if (ctx.input_events_processed != 4u) {
        fprintf(stderr, "lifecycle_test: expected input_events_processed=4 got=%llu\n",
                (unsigned long long)ctx.input_events_processed);
        return 1;
    }
    if (ctx.input_actions_emitted != 4u) {
        fprintf(stderr, "lifecycle_test: expected input_actions_emitted=4 got=%llu\n",
                (unsigned long long)ctx.input_actions_emitted);
        return 1;
    }
    if (ctx.routed_global_total != 2u || ctx.routed_pane_total != 2u || ctx.routed_fallback_total != 0u) {
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
    if (ctx.render_module_calls_total != 12u ||
        ctx.render_module_canvas_calls_total != 3u ||
        ctx.render_module_palette_calls_total != 3u) {
        fprintf(stderr,
                "lifecycle_test: unexpected module render call totals total=%llu canvas=%llu palette=%llu\n",
                (unsigned long long)ctx.render_module_calls_total,
                (unsigned long long)ctx.render_module_canvas_calls_total,
                (unsigned long long)ctx.render_module_palette_calls_total);
        return 1;
    }
    if (!expect_ok(drawing_program_history_undo(&ctx.history, &ctx.document), "history_undo_brush_seed")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&ctx.document, center_x, center_y, &center_undo),
                   "sample_read_center_after_undo")) {
        return 1;
    }
    if (center_undo != center_before) {
        fprintf(stderr, "lifecycle_test: expected center sample restore on undo before=%u after_undo=%u\n",
                (unsigned)center_before,
                (unsigned)center_undo);
        return 1;
    }
    if (!expect_ok(drawing_program_history_redo(&ctx.history, &ctx.document), "history_redo_brush_seed")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&ctx.document, center_x, center_y, &center_redo),
                   "sample_read_center_after_redo")) {
        return 1;
    }
    if (center_redo != expected_draw_value) {
        fprintf(stderr,
                "lifecycle_test: expected center sample %u after redo got=%u\n",
                (unsigned)expected_draw_value,
                (unsigned)center_redo);
        return 1;
    }
    if (ctx.render_projection.raster_sample_count == 0u ||
        ctx.render_projection.raster_nonzero_count == 0u ||
        ctx.render_projection.raster_hash32 == 0u) {
        fprintf(stderr, "lifecycle_test: expected raster projection baseline populated\n");
        return 1;
    }
    if (ctx.render_canvas_last_raster_hash != ctx.render_projection.raster_hash32 ||
        ctx.render_canvas_last_nonzero_samples != ctx.render_projection.raster_nonzero_count) {
        fprintf(stderr, "lifecycle_test: expected canvas module to consume raster projection fields\n");
        return 1;
    }
    if (!ctx.render_last_has_active_layer || ctx.render_last_active_layer_id != 1u) {
        fprintf(stderr,
                "lifecycle_test: expected active layer projection id=1 has_active=1 got id=%u has_active=%u\n",
                ctx.render_last_active_layer_id,
                (unsigned)ctx.render_last_has_active_layer);
        return 1;
    }
    {
        DrawingProgramAppContext save_ctx;
        DrawingProgramAppContext load_ctx;
        char persist_arg0[] = "drawing_program_persist_roundtrip";
        char persist_arg1[] = "--headless";
        char persist_arg2[] = "--smoke-frames";
        char persist_arg3[] = "1";
        char persist_arg4[] = "--preset";
        char persist_arg5[] = "/tmp/drawing_program_persist_roundtrip.pack";
        char *persist_argv[] = { persist_arg0, persist_arg1, persist_arg2, persist_arg3, persist_arg4, persist_arg5, 0 };
        (void)unlink(persist_arg5);
        if (!expect_ok(drawing_program_app_bootstrap(&save_ctx, 6, persist_argv), "persist_roundtrip_bootstrap_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&save_ctx), "persist_roundtrip_config_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&save_ctx), "persist_roundtrip_state_seed_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_subsystems_init(&save_ctx), "persist_roundtrip_subsystems_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_start(&save_ctx), "persist_roundtrip_runtime_start_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &save_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_PICKER),
                       "persist_roundtrip_set_tool_picker")) {
            return 1;
        }
        save_ctx.ui_theme_preset_id = (uint32_t)CORE_THEME_PRESET_LIGHT_DEFAULT;
        save_ctx.ui_font_zoom_step = 3;
        save_ctx.ui_left_panel_slot = 1u;
        save_ctx.ui_right_panel_slot = 1u;
        save_ctx.ui_active_color_index = 6u;
        if (!expect_ok(drawing_program_app_shutdown(&save_ctx), "persist_roundtrip_shutdown_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&load_ctx, 6, persist_argv), "persist_roundtrip_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&load_ctx), "persist_roundtrip_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&load_ctx), "persist_roundtrip_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_subsystems_init(&load_ctx), "persist_roundtrip_subsystems_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_start(&load_ctx), "persist_roundtrip_runtime_start_load")) {
            return 1;
        }
        if (load_ctx.editor.active_tool != DRAWING_PROGRAM_TOOL_PICKER ||
            load_ctx.ui_theme_preset_id != (uint32_t)CORE_THEME_PRESET_LIGHT_DEFAULT ||
            load_ctx.ui_font_zoom_step != 3 ||
            load_ctx.ui_left_panel_slot != 1u ||
            load_ctx.ui_right_panel_slot != 1u ||
            load_ctx.ui_active_color_index != 6u) {
            fprintf(stderr,
                    "lifecycle_test: persistence roundtrip mismatch tool=%u theme=%u zoom=%d left=%u right=%u color=%u\n",
                    (unsigned)load_ctx.editor.active_tool,
                    (unsigned)load_ctx.ui_theme_preset_id,
                    (int)load_ctx.ui_font_zoom_step,
                    (unsigned)load_ctx.ui_left_panel_slot,
                    (unsigned)load_ctx.ui_right_panel_slot,
                    (unsigned)load_ctx.ui_active_color_index);
            return 1;
        }
        load_ctx.persist_enabled = 0u;
        if (!expect_ok(drawing_program_app_shutdown(&load_ctx), "persist_roundtrip_shutdown_load")) {
            return 1;
        }
    }
    return 0;
}
