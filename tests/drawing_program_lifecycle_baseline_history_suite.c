#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core_theme.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_visual_transform_ops.h"
#include "drawing_program_lifecycle_baseline_history_suite.h"
#include "drawing_program_lifecycle_test_support.h"

int drawing_program_lifecycle_run_baseline_history_suite(DrawingProgramAppContext *ctx_ptr,
                                                         uint8_t expected_eraser_value,
                                                         uint32_t *center_x_out,
                                                         uint32_t *center_y_out,
                                                         uint8_t *center_before_out)
{
#define ctx (*ctx_ptr)
    uint32_t center_x = 0u;
    uint32_t center_y = 0u;
    uint8_t center_before = 0u;

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
    if (ctx.layer_rasters.sample_count != ctx.document.raster_sample_count ||
        ctx.layer_rasters.slot_count != ctx.document.layer_count) {
        fprintf(stderr,
                "lifecycle_test: expected seeded layer raster store shape/count sample_count=%u/%u slot_count=%u/%u\n",
                (unsigned)ctx.layer_rasters.sample_count,
                (unsigned)ctx.document.raster_sample_count,
                (unsigned)ctx.layer_rasters.slot_count,
                (unsigned)ctx.document.layer_count);
        return 1;
    }
    {
        uint8_t seeded_layer_sample = 0u;
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&ctx.layer_rasters,
                                                                      ctx.document.layers[0].layer_id,
                                                                      center_x,
                                                                      center_y,
                                                                      &seeded_layer_sample),
                       "seeded_layer_sample_read")) {
            return 1;
        }
        if (seeded_layer_sample != expected_eraser_value) {
            fprintf(stderr,
                    "lifecycle_test: expected seeded layer sample to match eraser value=%u got=%u\n",
                    (unsigned)expected_eraser_value,
                    (unsigned)seeded_layer_sample);
            return 1;
        }
    }
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
    ctx.session.export_json_path = "/tmp/drawing_program_overlay_authoring_blocked.json";
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
    ctx.session.export_json_path = "/tmp/drawing_program_overlay_runtime_export.json";
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
    if (!expect_ok(drawing_program_history_undo(&ctx.history, &ctx.document, &ctx.layer_rasters, &ctx.object_store), "history_undo")) {
        return 1;
    }
    if (ctx.document.layers[0].visible != 1u || ctx.history.cursor != 0u) {
        fprintf(stderr, "lifecycle_test: expected visibility restored after undo\n");
        return 1;
    }
    if (!expect_ok(drawing_program_history_redo(&ctx.history, &ctx.document, &ctx.layer_rasters, &ctx.object_store), "history_redo")) {
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
    if (!expect_ok(drawing_program_document_set_layer_visibility(&ctx.document, 1u, 1u, 0),
                   "restore_base_layer_visibility_for_grouped_sample_test")) {
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
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&ctx.history,
                                                                      &ctx.document,
                                                                      &ctx.layer_rasters,
                                                                      ctx.editor.active_layer_id,
                                                                      ax,
                                                                      ay,
                                                                      200u),
                       "history_group_set_sample_a")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&ctx.history,
                                                                      &ctx.document,
                                                                      &ctx.layer_rasters,
                                                                      ctx.editor.active_layer_id,
                                                                      bx,
                                                                      by,
                                                                      201u),
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
        if (!expect_ok(drawing_program_history_undo(&ctx.history, &ctx.document, &ctx.layer_rasters, &ctx.object_store), "history_group_undo")) {
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
        if (!expect_ok(drawing_program_history_redo(&ctx.history, &ctx.document, &ctx.layer_rasters, &ctx.object_store), "history_group_redo")) {
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
        uint32_t unit_cursor_before = 0u;
        uint32_t unit_count_before = 0u;
        uint32_t unit_cursor_after = 0u;
        uint32_t unit_count_after = 0u;
        uint32_t span_start_x = center_x;
        uint32_t span_start_y = center_y;
        uint32_t span_max = (ctx.document.raster_width > span_start_x) ? (ctx.document.raster_width - span_start_x) : 0u;
        uint32_t span_len = (span_max >= 8u) ? 8u : span_max;
        uint32_t i;
        uint8_t span_seed_value = 0u;
        if (span_len == 0u) {
            fprintf(stderr, "lifecycle_test: invalid span length for span history test\n");
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document,
                                                            span_start_x,
                                                            span_start_y,
                                                            &span_seed_value),
                       "span_seed_read")) {
            return 1;
        }
        for (i = 1u; i < span_len; ++i) {
            if (!expect_ok(drawing_program_history_apply_set_sample_value(&ctx.history,
                                                                          &ctx.document,
                                                                          &ctx.layer_rasters,
                                                                          ctx.editor.active_layer_id,
                                                                          span_start_x + i,
                                                                          span_start_y,
                                                                          span_seed_value),
                           "span_seed_normalize")) {
                return 1;
            }
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &ctx,
                           DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_HISTORY),
                       "span_seed_clear_history")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_begin_group(&ctx.history), "history_begin_group_span")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_span_value(&ctx.history,
                                                                           &ctx.document,
                                                                           &ctx.layer_rasters,
                                                                           ctx.editor.active_layer_id,
                                                                           (span_start_y * ctx.document.raster_width) + span_start_x,
                                                                           span_len,
                                                                           77u),
                       "history_span_apply")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_end_group(&ctx.history), "history_end_group_span")) {
            return 1;
        }
        for (i = 0u; i < span_len; ++i) {
            uint8_t sample = 0u;
            if (!expect_ok(drawing_program_document_sample_read(&ctx.document,
                                                                span_start_x + i,
                                                                span_start_y,
                                                                &sample),
                           "span_read_after_apply")) {
                return 1;
            }
            if (sample != 77u) {
                fprintf(stderr, "lifecycle_test: expected span sample value 77 at offset %u got %u\n", i, sample);
                return 1;
            }
        }
        drawing_program_history_query_units(&ctx.history, &unit_cursor_before, &unit_count_before);
        if (!expect_ok(drawing_program_history_undo(&ctx.history, &ctx.document, &ctx.layer_rasters, &ctx.object_store), "history_span_undo")) {
            return 1;
        }
        for (i = 0u; i < span_len; ++i) {
            uint8_t sample = 0u;
            if (!expect_ok(drawing_program_document_sample_read(&ctx.document,
                                                                span_start_x + i,
                                                                span_start_y,
                                                                &sample),
                           "span_read_after_undo")) {
                return 1;
            }
            if (sample == 77u) {
                fprintf(stderr, "lifecycle_test: span undo did not restore sample at offset %u\n", i);
                return 1;
            }
        }
        if (!expect_ok(drawing_program_history_redo(&ctx.history, &ctx.document, &ctx.layer_rasters, &ctx.object_store), "history_span_redo")) {
            return 1;
        }
        for (i = 0u; i < span_len; ++i) {
            uint8_t sample = 0u;
            if (!expect_ok(drawing_program_document_sample_read(&ctx.document,
                                                                span_start_x + i,
                                                                span_start_y,
                                                                &sample),
                           "span_read_after_redo")) {
                return 1;
            }
            if (sample != 77u) {
                fprintf(stderr, "lifecycle_test: span redo did not reapply sample at offset %u\n", i);
                return 1;
            }
        }
        drawing_program_history_query_units(&ctx.history, &unit_cursor_after, &unit_count_after);
        if (unit_cursor_after < unit_cursor_before || unit_count_after < unit_count_before) {
            fprintf(stderr, "lifecycle_test: span command units regressed unexpectedly (%u/%u -> %u/%u)\n",
                    unit_cursor_before, unit_count_before, unit_cursor_after, unit_count_after);
            return 1;
        }
    }
    {
        CoreResult undo_after_clear = drawing_program_history_undo(&ctx.history, &ctx.document, &ctx.layer_rasters, &ctx.object_store);
        if (undo_after_clear.code != CORE_OK) {
            fprintf(stderr, "lifecycle_test: expected undo after grouped write to succeed\n");
            return 1;
        }
        undo_after_clear = drawing_program_history_undo(&ctx.history, &ctx.document, &ctx.layer_rasters, &ctx.object_store);
        if (undo_after_clear.code != CORE_ERR_NOT_FOUND) {
            fprintf(stderr, "lifecycle_test: expected undo stack exhaustion to return not_found\n");
            return 1;
        }
    }
    if (!expect_ok(drawing_program_runtime_start(&ctx), "runtime_start")) {
        return 1;
    }
    if (center_x_out) {
        *center_x_out = center_x;
    }
    if (center_y_out) {
        *center_y_out = center_y;
    }
    if (center_before_out) {
        *center_before_out = center_before;
    }
    return 0;
#undef ctx
}
