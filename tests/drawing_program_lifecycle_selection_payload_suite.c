#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program_lifecycle_selection_payload_suite.h"
#include "drawing_program_lifecycle_test_support.h"

int drawing_program_lifecycle_run_selection_payload_suite(DrawingProgramAppContext *workflow_ctx_ptr,
                                                          DrawingProgramClipboardState *workflow_clipboard_ptr,
                                                          char **argv,
                                                          uint32_t workflow_center_x,
                                                          uint32_t workflow_center_y,
                                                          uint8_t expected_draw_value,
                                                          uint8_t expected_eraser_value)
{
    uint8_t workflow_center_value = 0u;
#define workflow_ctx (*workflow_ctx_ptr)
#define workflow_clipboard (*workflow_clipboard_ptr)

    {
        uint32_t marquee_w = 300u;
        uint32_t marquee_h = 140u;
        int32_t marquee_x = 0;
        int32_t marquee_y = 0;
        if (workflow_ctx.document.raster_width <= marquee_w || workflow_ctx.document.raster_height <= marquee_h) {
            fprintf(stderr,
                    "lifecycle_test: raster too small for marquee regression %ux%u raster=%ux%u\n",
                    (unsigned)marquee_w,
                    (unsigned)marquee_h,
                    (unsigned)workflow_ctx.document.raster_width,
                    (unsigned)workflow_ctx.document.raster_height);
            return 1;
        }
        marquee_x = (int32_t)((workflow_ctx.document.raster_width - marquee_w) / 2u);
        marquee_y = (int32_t)((workflow_ctx.document.raster_height - marquee_h) / 2u);
        if (!drawing_program_selection_capture_from_rect(&workflow_ctx.document,
                                                         &workflow_ctx.layer_rasters,
                                                         workflow_ctx.editor.active_layer_id,
                                                         &workflow_ctx.selection,
                                                         marquee_x,
                                                         marquee_y,
                                                         marquee_w,
                                                         marquee_h)) {
            fprintf(stderr,
                    "lifecycle_test: expected marquee-size selection capture to succeed for %ux%u\n",
                    (unsigned)marquee_w,
                    (unsigned)marquee_h);
            return 1;
        }
        if (workflow_ctx.selection.width != marquee_w || workflow_ctx.selection.height != marquee_h) {
            fprintf(stderr,
                    "lifecycle_test: expected marquee selection size=%ux%u got=%ux%u\n",
                    (unsigned)marquee_w,
                    (unsigned)marquee_h,
                    (unsigned)workflow_ctx.selection.width,
                    (unsigned)workflow_ctx.selection.height);
            return 1;
        }
    }
    {
        uint32_t p1x = workflow_center_x;
        uint32_t p1y = workflow_center_y;
        uint32_t p2x = workflow_center_x + 7u;
        uint32_t p2y = workflow_center_y + 5u;
        uint32_t min_x;
        uint32_t min_y;
        uint32_t max_x;
        uint32_t max_y;
        uint32_t local1_index;
        uint32_t local2_index;
        if (p2x >= workflow_ctx.document.raster_width) {
            p2x = workflow_ctx.document.raster_width - 1u;
        }
        if (p2y >= workflow_ctx.document.raster_height) {
            p2y = workflow_ctx.document.raster_height - 1u;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                       &workflow_ctx.document,
                                                                       &workflow_ctx.layer_rasters,
                                                                       workflow_ctx.editor.active_layer_id,
                                                                       p2x,
                                                                       p2y,
                                                                       expected_draw_value),
                       "selection_additive_seed_second_sample")) {
            return 1;
        }
        drawing_program_selection_reset(&workflow_ctx.selection);
        if (!drawing_program_selection_capture_from_rect(&workflow_ctx.document,
                                                         &workflow_ctx.layer_rasters,
                                                         workflow_ctx.editor.active_layer_id,
                                                         &workflow_ctx.selection,
                                                         (int32_t)p1x,
                                                         (int32_t)p1y,
                                                         1u,
                                                         1u)) {
            fprintf(stderr, "lifecycle_test: expected base 1x1 capture for additive selection\n");
            return 1;
        }
        if (!drawing_program_selection_add_from_rect(&workflow_ctx.document,
                                                     &workflow_ctx.layer_rasters,
                                                     workflow_ctx.editor.active_layer_id,
                                                     &workflow_ctx.selection,
                                                     (int32_t)p2x,
                                                     (int32_t)p2y,
                                                     1u,
                                                     1u)) {
            fprintf(stderr, "lifecycle_test: expected additive marquee capture to succeed\n");
            return 1;
        }
        if (!workflow_ctx.selection.has_payload || workflow_ctx.selection.payload_count != 2u) {
            fprintf(stderr,
                    "lifecycle_test: expected additive selection payload_count=2 got=%u\n",
                    (unsigned)workflow_ctx.selection.payload_count);
            return 1;
        }
        min_x = (p1x < p2x) ? p1x : p2x;
        min_y = (p1y < p2y) ? p1y : p2y;
        max_x = (p1x > p2x) ? p1x : p2x;
        max_y = (p1y > p2y) ? p1y : p2y;
        if (workflow_ctx.selection.origin_x != min_x ||
            workflow_ctx.selection.origin_y != min_y ||
            workflow_ctx.selection.width != (max_x - min_x + 1u) ||
            workflow_ctx.selection.height != (max_y - min_y + 1u)) {
            fprintf(stderr,
                    "lifecycle_test: additive selection bounds mismatch origin=%u,%u size=%ux%u expected_origin=%u,%u expected_size=%ux%u\n",
                    (unsigned)workflow_ctx.selection.origin_x,
                    (unsigned)workflow_ctx.selection.origin_y,
                    (unsigned)workflow_ctx.selection.width,
                    (unsigned)workflow_ctx.selection.height,
                    (unsigned)min_x,
                    (unsigned)min_y,
                    (unsigned)(max_x - min_x + 1u),
                    (unsigned)(max_y - min_y + 1u));
            return 1;
        }
        local1_index = (p1y - min_y) * workflow_ctx.selection.width + (p1x - min_x);
        local2_index = (p2y - min_y) * workflow_ctx.selection.width + (p2x - min_x);
        if (local1_index >= DRAWING_PROGRAM_SELECTION_MAX_AREA ||
            local2_index >= DRAWING_PROGRAM_SELECTION_MAX_AREA ||
            workflow_ctx.selection.payload_mask[local1_index] == 0u ||
            workflow_ctx.selection.payload_mask[local2_index] == 0u) {
            fprintf(stderr, "lifecycle_test: additive selection missing one of expected payload points\n");
            return 1;
        }
        if (max_x > min_x) {
            uint32_t gap_x = min_x + 1u;
            uint32_t gap_y = min_y;
            uint32_t gap_index = (gap_y - min_y) * workflow_ctx.selection.width + (gap_x - min_x);
            if (gap_index < DRAWING_PROGRAM_SELECTION_MAX_AREA &&
                workflow_ctx.selection.payload_mask[gap_index] != 0u &&
                !(gap_x == p1x && gap_y == p1y) &&
                !(gap_x == p2x && gap_y == p2y)) {
                fprintf(stderr,
                        "lifecycle_test: additive selection unexpectedly filled interior gap at local index=%u\n",
                        (unsigned)gap_index);
                return 1;
            }
        }
        if (!drawing_program_selection_add_from_rect(&workflow_ctx.document,
                                                     &workflow_ctx.layer_rasters,
                                                     workflow_ctx.editor.active_layer_id,
                                                     &workflow_ctx.selection,
                                                     -32,
                                                     -32,
                                                     1u,
                                                     1u) ||
            workflow_ctx.selection.payload_count != 2u) {
            fprintf(stderr, "lifecycle_test: additive empty marquee should preserve current selection payload\n");
            return 1;
        }
        if (!drawing_program_selection_capture_from_rect(&workflow_ctx.document,
                                                         &workflow_ctx.layer_rasters,
                                                         workflow_ctx.editor.active_layer_id,
                                                         &workflow_ctx.selection,
                                                         (int32_t)p1x,
                                                         (int32_t)p1y,
                                                         1u,
                                                         1u)) {
            fprintf(stderr, "lifecycle_test: expected plain marquee replace capture to succeed\n");
            return 1;
        }
        if (!workflow_ctx.selection.has_payload ||
            workflow_ctx.selection.payload_count != 1u ||
            workflow_ctx.selection.origin_x != p1x ||
            workflow_ctx.selection.origin_y != p1y ||
            workflow_ctx.selection.width != 1u ||
            workflow_ctx.selection.height != 1u ||
            workflow_ctx.selection.payload_mask[0] == 0u) {
            fprintf(stderr,
                    "lifecycle_test: expected plain marquee replace to clear prior additive payload and keep only replace rect\n");
            return 1;
        }
        drawing_program_selection_reset(&workflow_ctx.selection);
        if (!drawing_program_selection_capture_from_rect(&workflow_ctx.document,
                                                         &workflow_ctx.layer_rasters,
                                                         workflow_ctx.editor.active_layer_id,
                                                         &workflow_ctx.selection,
                                                         (int32_t)p1x,
                                                         (int32_t)p1y,
                                                         1u,
                                                         1u)) {
            fprintf(stderr, "lifecycle_test: expected base 1x1 recapture for subtractive regression\n");
            return 1;
        }
        if (!drawing_program_selection_add_from_rect(&workflow_ctx.document,
                                                     &workflow_ctx.layer_rasters,
                                                     workflow_ctx.editor.active_layer_id,
                                                     &workflow_ctx.selection,
                                                     (int32_t)p2x,
                                                     (int32_t)p2y,
                                                     1u,
                                                     1u)) {
            fprintf(stderr, "lifecycle_test: expected additive recapture before subtractive regression\n");
            return 1;
        }
        if (!drawing_program_selection_subtract_from_rect(&workflow_ctx.document,
                                                          &workflow_ctx.layer_rasters,
                                                          workflow_ctx.editor.active_layer_id,
                                                          &workflow_ctx.selection,
                                                          (int32_t)p2x,
                                                          (int32_t)p2y,
                                                          1u,
                                                          1u)) {
            fprintf(stderr, "lifecycle_test: expected subtractive selection capture to succeed\n");
            return 1;
        }
        if (!workflow_ctx.selection.has_payload ||
            workflow_ctx.selection.payload_count != 1u ||
            workflow_ctx.selection.origin_x != p1x ||
            workflow_ctx.selection.origin_y != p1y ||
            workflow_ctx.selection.width != 1u ||
            workflow_ctx.selection.height != 1u ||
            workflow_ctx.selection.payload_mask[0] == 0u) {
            fprintf(stderr,
                    "lifecycle_test: subtractive selection did not collapse to remaining payload origin=%u,%u size=%ux%u count=%u\n",
                    (unsigned)workflow_ctx.selection.origin_x,
                    (unsigned)workflow_ctx.selection.origin_y,
                    (unsigned)workflow_ctx.selection.width,
                    (unsigned)workflow_ctx.selection.height,
                    (unsigned)workflow_ctx.selection.payload_count);
            return 1;
        }
        if (drawing_program_selection_subtract_from_rect(&workflow_ctx.document,
                                                         &workflow_ctx.layer_rasters,
                                                         workflow_ctx.editor.active_layer_id,
                                                         &workflow_ctx.selection,
                                                         (int32_t)p1x,
                                                         (int32_t)p1y,
                                                         1u,
                                                         1u)) {
            fprintf(stderr, "lifecycle_test: subtractive selection should clear and return 0 on final payload removal\n");
            return 1;
        }
        if (workflow_ctx.selection.has_payload || workflow_ctx.selection.payload_count != 0u) {
            fprintf(stderr, "lifecycle_test: subtractive final removal should reset selection state\n");
            return 1;
        }
    }
    {
        uint32_t p1x = workflow_center_x + 12u;
        uint32_t p1y = workflow_center_y + 12u;
        uint32_t p2x = p1x + 3u;
        uint32_t p2y = p1y + 1u;
        uint32_t first_local_index;
        uint32_t second_local_index;
        DrawingProgramRasterSample color_a = drawing_program_color_value_from_rgba(17u, 89u, 203u, 255u);
        DrawingProgramRasterSample color_b = drawing_program_color_value_from_rgba(231u, 41u, 77u, 255u);
        DrawingProgramRasterSample selected_a = drawing_program_color_eraser_value();
        DrawingProgramRasterSample selected_b = drawing_program_color_eraser_value();
        DrawingProgramRasterSample remaining = drawing_program_color_eraser_value();
        if (p2x >= workflow_ctx.document.raster_width || p2y >= workflow_ctx.document.raster_height) {
            fprintf(stderr, "lifecycle_test: true-color selection regression coordinates out of bounds\n");
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "selection_true_color_clear_canvas")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                      &workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      workflow_ctx.editor.active_layer_id,
                                                                      p1x,
                                                                      p1y,
                                                                      color_a),
                       "selection_true_color_seed_a")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                      &workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      workflow_ctx.editor.active_layer_id,
                                                                      p2x,
                                                                      p2y,
                                                                      color_b),
                       "selection_true_color_seed_b")) {
            return 1;
        }
        drawing_program_selection_reset(&workflow_ctx.selection);
        if (!drawing_program_selection_capture_from_rect(&workflow_ctx.document,
                                                         &workflow_ctx.layer_rasters,
                                                         workflow_ctx.editor.active_layer_id,
                                                         &workflow_ctx.selection,
                                                         (int32_t)p1x,
                                                         (int32_t)p1y,
                                                         1u,
                                                         1u)) {
            fprintf(stderr, "lifecycle_test: expected true-color base capture to succeed\n");
            return 1;
        }
        if (!drawing_program_selection_add_from_rect(&workflow_ctx.document,
                                                     &workflow_ctx.layer_rasters,
                                                     workflow_ctx.editor.active_layer_id,
                                                     &workflow_ctx.selection,
                                                     (int32_t)p2x,
                                                     (int32_t)p2y,
                                                     1u,
                                                     1u)) {
            fprintf(stderr, "lifecycle_test: expected true-color additive capture to succeed\n");
            return 1;
        }
        first_local_index = (p1y - workflow_ctx.selection.origin_y) * workflow_ctx.selection.width +
                            (p1x - workflow_ctx.selection.origin_x);
        second_local_index = (p2y - workflow_ctx.selection.origin_y) * workflow_ctx.selection.width +
                             (p2x - workflow_ctx.selection.origin_x);
        if (first_local_index >= DRAWING_PROGRAM_SELECTION_MAX_AREA ||
            second_local_index >= DRAWING_PROGRAM_SELECTION_MAX_AREA) {
            fprintf(stderr, "lifecycle_test: true-color additive indices out of bounds\n");
            return 1;
        }
        selected_a = drawing_program_selection_value_at(&workflow_ctx.selection,
                                                        p1x - workflow_ctx.selection.origin_x,
                                                        p1y - workflow_ctx.selection.origin_y);
        selected_b = drawing_program_selection_value_at(&workflow_ctx.selection,
                                                        p2x - workflow_ctx.selection.origin_x,
                                                        p2y - workflow_ctx.selection.origin_y);
        if (selected_a != color_a || selected_b != color_b) {
            fprintf(stderr,
                    "lifecycle_test: expected true-color additive selection payload to preserve packed samples "
                    "got a=%u b=%u expected_a=%u expected_b=%u\n",
                    (unsigned)selected_a,
                    (unsigned)selected_b,
                    (unsigned)color_a,
                    (unsigned)color_b);
            return 1;
        }
        if (!drawing_program_selection_subtract_from_rect(&workflow_ctx.document,
                                                          &workflow_ctx.layer_rasters,
                                                          workflow_ctx.editor.active_layer_id,
                                                          &workflow_ctx.selection,
                                                          (int32_t)p1x,
                                                          (int32_t)p1y,
                                                          1u,
                                                          1u)) {
            fprintf(stderr, "lifecycle_test: expected true-color subtractive capture to preserve remaining payload\n");
            return 1;
        }
        if (!workflow_ctx.selection.has_payload ||
            workflow_ctx.selection.payload_count != 1u ||
            workflow_ctx.selection.origin_x != p2x ||
            workflow_ctx.selection.origin_y != p2y ||
            workflow_ctx.selection.width != 1u ||
            workflow_ctx.selection.height != 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected true-color subtractive collapse to remaining payload origin=%u,%u size=%ux%u count=%u\n",
                    (unsigned)workflow_ctx.selection.origin_x,
                    (unsigned)workflow_ctx.selection.origin_y,
                    (unsigned)workflow_ctx.selection.width,
                    (unsigned)workflow_ctx.selection.height,
                    (unsigned)workflow_ctx.selection.payload_count);
            return 1;
        }
        remaining = drawing_program_selection_value_at(&workflow_ctx.selection, 0u, 0u);
        if (remaining != color_b) {
            fprintf(stderr,
                    "lifecycle_test: expected true-color subtractive collapse to preserve remaining packed sample "
                    "got=%u expected=%u\n",
                    (unsigned)remaining,
                    (unsigned)color_b);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "selection_true_color_restore_clear_canvas")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                      &workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      workflow_ctx.editor.active_layer_id,
                                                                      workflow_center_x,
                                                                      workflow_center_y,
                                                                      expected_draw_value),
                       "selection_true_color_restore_center_seed")) {
            return 1;
        }
        drawing_program_selection_reset(&workflow_ctx.selection);
    }
    if (!drawing_program_selection_select_all(&workflow_ctx.document,
                                              &workflow_ctx.layer_rasters,
                                              workflow_ctx.editor.active_layer_id,
                                              &workflow_ctx.selection)) {
        fprintf(stderr, "lifecycle_test: expected select-all to capture non-background payload\n");
        return 1;
    }
    if (!workflow_ctx.selection.has_payload ||
        workflow_ctx.selection.payload_count == 0u ||
        workflow_ctx.selection.width == 0u ||
        workflow_ctx.selection.height == 0u) {
        fprintf(stderr, "lifecycle_test: expected selection payload after select-all\n");
        return 1;
    }
    {
        uint32_t origin_x_before = workflow_ctx.selection.origin_x;
        uint32_t origin_y_before = workflow_ctx.selection.origin_y;
        uint8_t moved_from = 0u;
        uint8_t moved_to = 0u;
        workflow_ctx.selection.offset_x = 1;
        workflow_ctx.selection.offset_y = 0;
        if (!expect_ok(drawing_program_selection_commit_move(&workflow_ctx.document,
                                                             &workflow_ctx.layer_rasters,
                                                             workflow_ctx.editor.active_layer_id,
                                                             &workflow_ctx.history,
                                                             &workflow_ctx.selection),
                       "selection_commit_move_nudge_right")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document,
                                                            workflow_center_x,
                                                            workflow_center_y,
                                                            &moved_from),
                       "selection_nudge_sample_from")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document,
                                                            workflow_center_x + 1u,
                                                            workflow_center_y,
                                                            &moved_to),
                       "selection_nudge_sample_to")) {
            return 1;
        }
        if (moved_from != expected_eraser_value || moved_to != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected selection nudge to move pixel right from=%u to=%u\n",
                    (unsigned)moved_from,
                    (unsigned)moved_to);
            return 1;
        }
        if (!workflow_ctx.selection.has_payload ||
            workflow_ctx.selection.origin_x != origin_x_before + 1u ||
            workflow_ctx.selection.origin_y != origin_y_before) {
            fprintf(stderr,
                    "lifecycle_test: expected selection bounds to follow nudge origin_before=%u,%u origin_after=%u,%u\n",
                    (unsigned)origin_x_before,
                    (unsigned)origin_y_before,
                    (unsigned)workflow_ctx.selection.origin_x,
                    (unsigned)workflow_ctx.selection.origin_y);
            return 1;
        }
    }
    {
        uint8_t moved_from = 0u;
        uint8_t moved_to = 0u;
        uint32_t move_dx = 3u;
        uint32_t move_dy = 2u;
        uint32_t seed_x = workflow_center_x;
        uint32_t seed_y = workflow_center_y;
        if (seed_x + move_dx >= workflow_ctx.document.raster_width) {
            seed_x = workflow_ctx.document.raster_width - move_dx - 1u;
        }
        if (seed_y + move_dy >= workflow_ctx.document.raster_height) {
            seed_y = workflow_ctx.document.raster_height - move_dy - 1u;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "selection_move_tracking_clear_canvas")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                      &workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      workflow_ctx.editor.active_layer_id,
                                                                      seed_x,
                                                                      seed_y,
                                                                      expected_draw_value),
                       "selection_move_tracking_seed")) {
            return 1;
        }
        if (!drawing_program_selection_capture_from_rect(&workflow_ctx.document,
                                                         &workflow_ctx.layer_rasters,
                                                         workflow_ctx.editor.active_layer_id,
                                                         &workflow_ctx.selection,
                                                         (int32_t)seed_x,
                                                         (int32_t)seed_y,
                                                         1u,
                                                         1u)) {
            fprintf(stderr, "lifecycle_test: expected selection capture for move-tracking regression\n");
            return 1;
        }
        drawing_program_selection_begin_move_tracking(&workflow_ctx.selection, seed_x, seed_y);
        drawing_program_selection_update_move_offset(&workflow_ctx.selection, seed_x + move_dx, seed_y + move_dy);
        if (!expect_ok(drawing_program_selection_commit_move(&workflow_ctx.document,
                                                             &workflow_ctx.layer_rasters,
                                                             workflow_ctx.editor.active_layer_id,
                                                             &workflow_ctx.history,
                                                             &workflow_ctx.selection),
                       "selection_move_tracking_commit")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, seed_x, seed_y, &moved_from),
                       "selection_move_tracking_probe_from")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document,
                                                            seed_x + move_dx,
                                                            seed_y + move_dy,
                                                            &moved_to),
                       "selection_move_tracking_probe_to")) {
            return 1;
        }
        if (moved_from != expected_eraser_value || moved_to != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected move-tracking commit to move sample from=%u to=%u got_from=%u got_to=%u\n",
                    (unsigned)expected_eraser_value,
                    (unsigned)expected_draw_value,
                    (unsigned)moved_from,
                    (unsigned)moved_to);
            return 1;
        }
    }
    if (!expect_ok(drawing_program_snapshot_save(&workflow_ctx, "/tmp/drawing_program_selection_roundtrip.pack"),
                   "snapshot_save_selection_roundtrip")) {
        return 1;
    }
    {
        static DrawingProgramAppContext selection_load_ctx;
        if (!expect_ok(drawing_program_app_bootstrap(&selection_load_ctx, 5, argv), "selection_load_bootstrap")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&selection_load_ctx), "selection_load_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&selection_load_ctx), "selection_load_state_seed")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_subsystems_init(&selection_load_ctx), "selection_load_subsystems_init")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_start(&selection_load_ctx), "selection_load_runtime_start")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&selection_load_ctx, "/tmp/drawing_program_selection_roundtrip.pack"),
                       "snapshot_load_selection_roundtrip")) {
            return 1;
        }
        if (!selection_load_ctx.selection.has_payload ||
            selection_load_ctx.selection.payload_count == 0u ||
            selection_load_ctx.selection.width == 0u ||
            selection_load_ctx.selection.height == 0u) {
            fprintf(stderr, "lifecycle_test: expected loaded snapshot to restore selection payload\n");
            return 1;
        }
    }
    (void)unlink("/tmp/drawing_program_selection_roundtrip.pack");
    drawing_program_selection_reset(&workflow_ctx.selection);
    if (workflow_ctx.selection.has_payload || workflow_ctx.selection.payload_count != 0u) {
        fprintf(stderr, "lifecycle_test: expected clear selection to remove payload\n");
        return 1;
    }
    if (!drawing_program_selection_select_all(&workflow_ctx.document,
                                              &workflow_ctx.layer_rasters,
                                              workflow_ctx.editor.active_layer_id,
                                              &workflow_ctx.selection)) {
        fprintf(stderr, "lifecycle_test: expected select-all to capture payload for clipboard seed\n");
        return 1;
    }
    if (!drawing_program_selection_copy_payload(&workflow_ctx.selection, &workflow_clipboard) ||
        !workflow_clipboard.has_payload ||
        workflow_clipboard.payload_count == 0u) {
        fprintf(stderr, "lifecycle_test: expected clipboard copy to capture selection payload\n");
        return 1;
    }
    if (!expect_ok(drawing_program_selection_cut_to_clipboard(&workflow_ctx.document,
                                                              &workflow_ctx.layer_rasters,
                                                              workflow_ctx.editor.active_layer_id,
                                                              &workflow_ctx.history,
                                                              &workflow_ctx.selection,
                                                              &workflow_clipboard),
                   "selection_cut_to_clipboard")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document,
                                                        workflow_center_x + 1u,
                                                        workflow_center_y,
                                                        &workflow_center_value),
                   "workflow_sample_after_cut")) {
        return 1;
    }
    if (workflow_center_value != expected_eraser_value) {
        fprintf(stderr,
                "lifecycle_test: expected cut to clear moved payload sample to background=%u got=%u\n",
                (unsigned)expected_eraser_value,
                (unsigned)workflow_center_value);
        return 1;
    }
    if (!expect_ok(drawing_program_selection_paste_from_clipboard(&workflow_ctx.document,
                                                                  &workflow_ctx.layer_rasters,
                                                                  workflow_ctx.editor.active_layer_id,
                                                                  &workflow_ctx.history,
                                                                  &workflow_ctx.selection,
                                                                  &workflow_clipboard,
                                                                  (int32_t)workflow_center_x + 2,
                                                                  (int32_t)workflow_center_y),
                   "selection_paste_from_clipboard")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document,
                                                        workflow_center_x + 2u,
                                                        workflow_center_y,
                                                        &workflow_center_value),
                   "workflow_sample_after_paste")) {
        return 1;
    }
    if (workflow_center_value != expected_draw_value) {
        fprintf(stderr,
                "lifecycle_test: expected paste to place sample value=%u got=%u\n",
                (unsigned)expected_draw_value,
                (unsigned)workflow_center_value);
        return 1;
    }
    {
        uint32_t layer_id = workflow_ctx.editor.active_layer_id;
        uint8_t seeded_value = drawing_program_color_value_from_index(4u);
        uint8_t probe = 0u;
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "selection_delete_payload_clear_canvas")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                      &workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      layer_id,
                                                                      14u,
                                                                      14u,
                                                                      seeded_value),
                       "selection_delete_payload_seed")) {
            return 1;
        }
        if (!drawing_program_selection_capture_from_rect(&workflow_ctx.document,
                                                         &workflow_ctx.layer_rasters,
                                                         layer_id,
                                                         &workflow_ctx.selection,
                                                         14,
                                                         14,
                                                         1u,
                                                         1u)) {
            fprintf(stderr, "lifecycle_test: expected delete payload capture to succeed\n");
            return 1;
        }
        if (!expect_ok(drawing_program_selection_delete_payload(&workflow_ctx.document,
                                                                &workflow_ctx.layer_rasters,
                                                                layer_id,
                                                                &workflow_ctx.history,
                                                                &workflow_ctx.selection),
                       "selection_delete_payload_commit")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, 14u, 14u, &probe),
                       "selection_delete_payload_probe")) {
            return 1;
        }
        if (probe != expected_eraser_value) {
            fprintf(stderr,
                    "lifecycle_test: expected selection delete to clear sample to background=%u got=%u\n",
                    (unsigned)expected_eraser_value,
                    (unsigned)probe);
            return 1;
        }
        if (workflow_ctx.selection.has_payload || workflow_ctx.selection.payload_count != 0u) {
            fprintf(stderr, "lifecycle_test: expected selection delete to reset active payload state\n");
            return 1;
        }
    }
    {
        uint32_t layer_id = workflow_ctx.editor.active_layer_id;
        uint8_t value_left = drawing_program_color_value_from_index(0u);
        uint8_t value_right = drawing_program_color_value_from_index(2u);
        uint8_t value_hole = drawing_program_color_value_from_index(5u);
        uint8_t probe = 0u;
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "selection_move_mask_semantics_clear_canvas")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                      &workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      layer_id,
                                                                      10u,
                                                                      10u,
                                                                      value_left),
                       "selection_move_mask_semantics_seed_left")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                      &workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      layer_id,
                                                                      12u,
                                                                      10u,
                                                                      value_right),
                       "selection_move_mask_semantics_seed_right")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                      &workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      layer_id,
                                                                      13u,
                                                                      10u,
                                                                      value_hole),
                       "selection_move_mask_semantics_seed_hole_keep")) {
            return 1;
        }
        if (!drawing_program_selection_capture_from_rect(&workflow_ctx.document,
                                                         &workflow_ctx.layer_rasters,
                                                         layer_id,
                                                         &workflow_ctx.selection,
                                                         10,
                                                         10,
                                                         3u,
                                                         1u)) {
            fprintf(stderr, "lifecycle_test: expected masked move selection capture\n");
            return 1;
        }
        if (drawing_program_selection_begin_move(&workflow_ctx.selection, 11u, 10u)) {
            fprintf(stderr, "lifecycle_test: expected move begin to reject transparent hole hit\n");
            return 1;
        }
        if (!drawing_program_selection_begin_move(&workflow_ctx.selection, 10u, 10u)) {
            fprintf(stderr, "lifecycle_test: expected move begin to accept payload hit\n");
            return 1;
        }
        workflow_ctx.selection.offset_x = 2;
        workflow_ctx.selection.offset_y = 0;
        if (!expect_ok(drawing_program_selection_commit_move(&workflow_ctx.document,
                                                             &workflow_ctx.layer_rasters,
                                                             layer_id,
                                                             &workflow_ctx.history,
                                                             &workflow_ctx.selection),
                       "selection_move_mask_semantics_commit")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, 10u, 10u, &probe),
                       "selection_move_mask_semantics_src_left")) {
            return 1;
        }
        if (probe != expected_eraser_value) {
            fprintf(stderr,
                    "lifecycle_test: expected source-left clear to background=%u got=%u\n",
                    (unsigned)expected_eraser_value,
                    (unsigned)probe);
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, 12u, 10u, &probe),
                       "selection_move_mask_semantics_dst_overlap")) {
            return 1;
        }
        if (probe != value_left) {
            fprintf(stderr,
                    "lifecycle_test: expected overlap destination value=%u got=%u\n",
                    (unsigned)value_left,
                    (unsigned)probe);
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, 13u, 10u, &probe),
                       "selection_move_mask_semantics_hole_preserve")) {
            return 1;
        }
        if (probe != value_hole) {
            fprintf(stderr,
                    "lifecycle_test: expected transparent-hole destination preserve=%u got=%u\n",
                    (unsigned)value_hole,
                    (unsigned)probe);
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, 14u, 10u, &probe),
                       "selection_move_mask_semantics_dst_right")) {
            return 1;
        }
        if (probe != value_right) {
            fprintf(stderr,
                    "lifecycle_test: expected destination-right value=%u got=%u\n",
                    (unsigned)value_right,
                    (unsigned)probe);
            return 1;
        }
        if (!workflow_ctx.selection.has_payload ||
            workflow_ctx.selection.origin_x != 12u ||
            workflow_ctx.selection.origin_y != 10u ||
            workflow_ctx.selection.width != 3u ||
            workflow_ctx.selection.height != 1u ||
            workflow_ctx.selection.payload_count != 2u ||
            drawing_program_selection_mask_at(&workflow_ctx.selection, 0u, 0u) != 1u ||
            drawing_program_selection_mask_at(&workflow_ctx.selection, 1u, 0u) != 0u ||
            drawing_program_selection_mask_at(&workflow_ctx.selection, 2u, 0u) != 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected sparse selection payload preserved after move "
                    "(origin=12,10 size=3x1 count=2 mask=1/0/1)\n");
            return 1;
        }
    }
    {
        uint32_t layer_id = workflow_ctx.editor.active_layer_id;
        uint8_t seed_a = drawing_program_color_value_from_index(1u);
        uint8_t seed_b = drawing_program_color_value_from_index(6u);
        uint8_t probe = 0u;
        uint32_t max_x = workflow_ctx.document.raster_width - 1u;
        uint32_t max_y = workflow_ctx.document.raster_height - 1u;
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "selection_move_clamp_clear_canvas")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                      &workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      layer_id,
                                                                      1u,
                                                                      1u,
                                                                      seed_a),
                       "selection_move_clamp_seed_a")) {
            return 1;
        }
        if (!drawing_program_selection_capture_from_rect(&workflow_ctx.document,
                                                         &workflow_ctx.layer_rasters,
                                                         layer_id,
                                                         &workflow_ctx.selection,
                                                         1,
                                                         1,
                                                         1u,
                                                         1u)) {
            fprintf(stderr, "lifecycle_test: expected move-clamp capture at 1,1\n");
            return 1;
        }
        workflow_ctx.selection.offset_x = -100;
        workflow_ctx.selection.offset_y = -100;
        if (!expect_ok(drawing_program_selection_commit_move(&workflow_ctx.document,
                                                             &workflow_ctx.layer_rasters,
                                                             layer_id,
                                                             &workflow_ctx.history,
                                                             &workflow_ctx.selection),
                       "selection_move_clamp_commit_to_min")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, 0u, 0u, &probe),
                       "selection_move_clamp_min_probe_dst")) {
            return 1;
        }
        if (probe != seed_a) {
            fprintf(stderr,
                    "lifecycle_test: expected clamped min destination value=%u got=%u\n",
                    (unsigned)seed_a,
                    (unsigned)probe);
            return 1;
        }
        if (!workflow_ctx.selection.has_payload ||
            workflow_ctx.selection.origin_x != 0u ||
            workflow_ctx.selection.origin_y != 0u) {
            fprintf(stderr,
                    "lifecycle_test: expected clamped selection origin at min bound 0,0 got=%u,%u\n",
                    (unsigned)workflow_ctx.selection.origin_x,
                    (unsigned)workflow_ctx.selection.origin_y);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "selection_move_clamp_clear_canvas_again")) {
            return 1;
        }
        if (max_x < 1u || max_y < 1u) {
            fprintf(stderr, "lifecycle_test: raster too small for max-bound move clamp check\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                      &workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      layer_id,
                                                                      max_x - 1u,
                                                                      max_y - 1u,
                                                                      seed_b),
                       "selection_move_clamp_seed_b")) {
            return 1;
        }
        if (!drawing_program_selection_capture_from_rect(&workflow_ctx.document,
                                                         &workflow_ctx.layer_rasters,
                                                         layer_id,
                                                         &workflow_ctx.selection,
                                                         (int32_t)max_x - 1,
                                                         (int32_t)max_y - 1,
                                                         1u,
                                                         1u)) {
            fprintf(stderr, "lifecycle_test: expected move-clamp capture near max bound\n");
            return 1;
        }
        workflow_ctx.selection.offset_x = 100;
        workflow_ctx.selection.offset_y = 100;
        if (!expect_ok(drawing_program_selection_commit_move(&workflow_ctx.document,
                                                             &workflow_ctx.layer_rasters,
                                                             layer_id,
                                                             &workflow_ctx.history,
                                                             &workflow_ctx.selection),
                       "selection_move_clamp_commit_to_max")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, max_x, max_y, &probe),
                       "selection_move_clamp_max_probe_dst")) {
            return 1;
        }
        if (probe != seed_b) {
            fprintf(stderr,
                    "lifecycle_test: expected clamped max destination value=%u got=%u\n",
                    (unsigned)seed_b,
                    (unsigned)probe);
            return 1;
        }
        if (!workflow_ctx.selection.has_payload ||
            workflow_ctx.selection.origin_x != max_x ||
            workflow_ctx.selection.origin_y != max_y) {
            fprintf(stderr,
                    "lifecycle_test: expected clamped selection origin at max bound %u,%u got=%u,%u\n",
                    (unsigned)max_x,
                    (unsigned)max_y,
                    (unsigned)workflow_ctx.selection.origin_x,
                    (unsigned)workflow_ctx.selection.origin_y);
            return 1;
        }
    }
#undef workflow_clipboard
#undef workflow_ctx
    return 0;
}
