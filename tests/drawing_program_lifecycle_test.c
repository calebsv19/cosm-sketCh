#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core_pack.h"
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

static int write_legacy_snapshot_without_layer_chunk(const char *source_pack_path,
                                                     const char *output_pack_path) {
    CorePackReader reader;
    CorePackWriter writer;
    CorePackChunkInfo dps2_chunk;
    CorePackChunkInfo dpui_chunk;
    CoreResult result;
    uint8_t *dps2_data = 0;
    uint8_t *dpui_data = 0;
    int has_dpui = 0;
    memset(&dps2_chunk, 0, sizeof(dps2_chunk));
    memset(&dpui_chunk, 0, sizeof(dpui_chunk));
    result = core_pack_reader_open(source_pack_path, &reader);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed to open source snapshot %s (%s)\n",
                source_pack_path,
                result.message ? result.message : "unknown");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPS2", 0u, &dps2_chunk);
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing DPS2 chunk\n");
        return 0;
    }
    dps2_data = (uint8_t *)malloc((size_t)dps2_chunk.size);
    if (!dps2_data) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to allocate DPS2 copy buffer\n");
        return 0;
    }
    result = core_pack_reader_read_chunk_data(&reader, &dps2_chunk, dps2_data, dps2_chunk.size);
    if (result.code != CORE_OK) {
        free(dps2_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to read DPS2 chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPUI", 0u, &dpui_chunk);
    if (result.code == CORE_OK) {
        dpui_data = (uint8_t *)malloc((size_t)dpui_chunk.size);
        if (!dpui_data) {
            free(dps2_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPUI copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dpui_chunk, dpui_data, dpui_chunk.size);
        if (result.code != CORE_OK) {
            free(dpui_data);
            free(dps2_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPUI chunk\n");
            return 0;
        }
        has_dpui = 1;
    }
    (void)core_pack_reader_close(&reader);

    result = core_pack_writer_open(output_pack_path, &writer);
    if (result.code != CORE_OK) {
        free(dpui_data);
        free(dps2_data);
        fprintf(stderr, "lifecycle_test: failed to open legacy snapshot output %s\n", output_pack_path);
        return 0;
    }
    result = core_pack_writer_add_chunk(&writer, "DPS2", dps2_data, dps2_chunk.size);
    if (result.code == CORE_OK && has_dpui) {
        result = core_pack_writer_add_chunk(&writer, "DPUI", dpui_data, dpui_chunk.size);
    }
    if (result.code == CORE_OK) {
        result = core_pack_writer_close(&writer);
    } else {
        (void)core_pack_writer_close(&writer);
    }
    free(dpui_data);
    free(dps2_data);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed to write legacy snapshot output chunk set\n");
        return 0;
    }
    return 1;
}

int main(void) {
    DrawingProgramAppContext ctx;
    DrawingProgramAppContext workflow_ctx;
    static DrawingProgramAppContext size_ctx;
    DrawingProgramClipboardState workflow_clipboard;
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
    char arg5[] = "--canvas-size";
    char arg6[] = "640x360";
    char *argv[] = { arg0, arg1, arg2, arg3, arg4, 0 };
    char *size_argv[] = { arg0, arg1, arg2, arg3, arg4, arg5, arg6, 0 };
    uint8_t expected_draw_value = drawing_program_color_value_from_index(drawing_program_color_default_index());
    uint8_t expected_eraser_value = drawing_program_color_eraser_value();
    drawing_program_clipboard_reset(&workflow_clipboard);

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
    if (!expect_ok(drawing_program_app_bootstrap(&size_ctx, 7, size_argv), "size_bootstrap")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&size_ctx), "size_config_load")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&size_ctx), "size_state_seed")) {
        return 1;
    }
    if (size_ctx.document.logical_width != 640u ||
        size_ctx.document.logical_height != 360u ||
        size_ctx.document.raster_width != 640u ||
        size_ctx.document.raster_height != 360u) {
        fprintf(stderr,
                "lifecycle_test: expected non-square canvas seed 640x360 got logical=%ux%u raster=%ux%u\n",
                (unsigned)size_ctx.document.logical_width,
                (unsigned)size_ctx.document.logical_height,
                (unsigned)size_ctx.document.raster_width,
                (unsigned)size_ctx.document.raster_height);
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
    if (!expect_ok(drawing_program_snapshot_save(&workflow_ctx, "/tmp/drawing_program_selection_roundtrip.pack"),
                   "snapshot_save_selection_roundtrip")) {
        return 1;
    }
    {
        DrawingProgramAppContext selection_load_ctx;
        if (!expect_ok(drawing_program_app_bootstrap(&selection_load_ctx, 5, argv), "selection_load_bootstrap")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&selection_load_ctx), "selection_load_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&selection_load_ctx), "selection_load_state_seed")) {
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
    {
        uint32_t base_layer_id = workflow_ctx.document.layers[0].layer_id;
        uint32_t new_layer_id = 0u;
        uint8_t sample_before_lock_stamp = 0u;
        uint8_t sample_after_lock_stamp = 0u;
        uint8_t base_before_active_stamp = 0u;
        uint8_t base_after_active_stamp = 0u;
        uint8_t active_before_visible_stamp = 0u;
        uint8_t active_after_visible_stamp = 0u;
        uint8_t active_before_hidden_stamp = 0u;
        uint8_t active_after_hidden_stamp = 0u;
        uint32_t resolved_active_layer_id = 0u;
        uint32_t resolved_active_layer_index = 0u;
        uint8_t resolved_active_visible = 0u;
        uint8_t resolved_active_locked = 0u;
        uint32_t policy_seed_x = workflow_center_x;
        uint32_t policy_seed_y = workflow_center_y + 2u;
        uint8_t policy_base_seed_value = 64u;
        uint8_t policy_active_seed_value = 192u;
        uint8_t policy_read_value = 0u;
        uint32_t clear_seed_x = workflow_center_x + 4u;
        uint32_t clear_seed_y = workflow_center_y + 4u;
        uint8_t clear_after_value = 0u;
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_ADD_LAYER),
                       "workflow_add_layer")) {
            return 1;
        }
        if (workflow_ctx.document.layer_count != 2u) {
            fprintf(stderr,
                    "lifecycle_test: expected layer_count=2 after add, got %u\n",
                    workflow_ctx.document.layer_count);
            return 1;
        }
        new_layer_id = workflow_ctx.editor.active_layer_id;
        if (new_layer_id == 0u || workflow_ctx.document.layers[1].layer_id != new_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected active layer to be newly added appended layer id=%u index1=%u\n",
                    (unsigned)new_layer_id,
                    (unsigned)workflow_ctx.document.layers[1].layer_id);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_resolve_active_layer(&workflow_ctx,
                                                                                   &resolved_active_layer_id,
                                                                                   &resolved_active_layer_index,
                                                                                   &resolved_active_visible,
                                                                                   &resolved_active_locked),
                       "workflow_resolve_active_layer")) {
            return 1;
        }
        if (resolved_active_layer_id != new_layer_id ||
            resolved_active_layer_index >= workflow_ctx.document.layer_count ||
            workflow_ctx.document.layers[resolved_active_layer_index].layer_id != resolved_active_layer_id ||
            resolved_active_visible != workflow_ctx.document.layers[resolved_active_layer_index].visible ||
            resolved_active_locked != workflow_ctx.document.layers[resolved_active_layer_index].locked) {
            fprintf(stderr,
                    "lifecycle_test: expected active-layer resolve to return coherent id/index mapping "
                    "(id=%u index=%u mapped=%u visible=%u locked=%u)\n",
                    (unsigned)resolved_active_layer_id,
                    (unsigned)resolved_active_layer_index,
                    (unsigned)((resolved_active_layer_index < workflow_ctx.document.layer_count)
                                   ? workflow_ctx.document.layers[resolved_active_layer_index].layer_id
                                   : 0u),
                    (unsigned)resolved_active_visible,
                    (unsigned)resolved_active_locked);
            return 1;
        }
        if (clear_seed_x >= workflow_ctx.document.raster_width || clear_seed_y >= workflow_ctx.document.raster_height) {
            fprintf(stderr, "lifecycle_test: clear-canvas probe coordinate out of bounds\n");
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY),
                       "workflow_clear_canvas_seed_hide_active")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_LOCK),
                       "workflow_clear_canvas_seed_lock_active")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "workflow_clear_canvas")) {
            return 1;
        }
        if (workflow_ctx.document.layers[1].visible != 1u || workflow_ctx.document.layers[1].locked != 0u) {
            fprintf(stderr,
                    "lifecycle_test: expected clear canvas to restore active layer editable state visible=%u locked=%u\n",
                    (unsigned)workflow_ctx.document.layers[1].visible,
                    (unsigned)workflow_ctx.document.layers[1].locked);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE),
                       "workflow_clear_canvas_stamp_after_clear")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                      &workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      clear_seed_x,
                                                                      clear_seed_y,
                                                                      expected_draw_value),
                       "workflow_clear_canvas_seed_active_probe")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      clear_seed_x,
                                                                      clear_seed_y,
                                                                      &clear_after_value),
                       "workflow_clear_canvas_probe_after_write")) {
            return 1;
        }
        if (clear_after_value != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected draw write after clear canvas value=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)clear_after_value);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_LOCK),
                       "workflow_toggle_layer_lock_on")) {
            return 1;
        }
        if (workflow_ctx.document.layers[1].locked != 1u) {
            fprintf(stderr, "lifecycle_test: expected active layer lock on after toggle\n");
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document,
                                                            workflow_center_x,
                                                            workflow_center_y,
                                                            &sample_before_lock_stamp),
                       "workflow_sample_before_lock_stamp")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE),
                       "workflow_stamp_locked_layer")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document,
                                                            workflow_center_x,
                                                            workflow_center_y,
                                                            &sample_after_lock_stamp),
                       "workflow_sample_after_lock_stamp")) {
            return 1;
        }
        if (sample_before_lock_stamp != sample_after_lock_stamp) {
            fprintf(stderr,
                    "lifecycle_test: expected locked layer stamp to no-op before=%u after=%u\n",
                    (unsigned)sample_before_lock_stamp,
                    (unsigned)sample_after_lock_stamp);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_LOCK),
                       "workflow_toggle_layer_lock_off")) {
            return 1;
        }
        if (workflow_ctx.document.layers[1].locked != 0u) {
            fprintf(stderr, "lifecycle_test: expected active layer lock off after second toggle\n");
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      base_layer_id,
                                                                      workflow_center_x,
                                                                      workflow_center_y,
                                                                      &base_before_active_stamp),
                       "workflow_base_sample_before_active_stamp")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      workflow_center_x,
                                                                      workflow_center_y,
                                                                      &active_before_visible_stamp),
                       "workflow_active_sample_before_visible_stamp")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE),
                       "workflow_stamp_visible_active_layer")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      base_layer_id,
                                                                      workflow_center_x,
                                                                      workflow_center_y,
                                                                      &base_after_active_stamp),
                       "workflow_base_sample_after_active_stamp")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      workflow_center_x,
                                                                      workflow_center_y,
                                                                      &active_after_visible_stamp),
                       "workflow_active_sample_after_visible_stamp")) {
            return 1;
        }
        if (base_before_active_stamp != base_after_active_stamp ||
            active_after_visible_stamp != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected active-layer stamp to update only active layer "
                    "(base %u->%u active %u->%u expected_active=%u)\n",
                    (unsigned)base_before_active_stamp,
                    (unsigned)base_after_active_stamp,
                    (unsigned)active_before_visible_stamp,
                    (unsigned)active_after_visible_stamp,
                    (unsigned)expected_draw_value);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY),
                       "workflow_toggle_active_layer_visibility_off_for_stamp_noop")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      workflow_center_x,
                                                                      workflow_center_y,
                                                                      &active_before_hidden_stamp),
                       "workflow_active_sample_before_hidden_stamp")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE),
                       "workflow_stamp_hidden_active_layer_noop")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      workflow_center_x,
                                                                      workflow_center_y,
                                                                      &active_after_hidden_stamp),
                       "workflow_active_sample_after_hidden_stamp")) {
            return 1;
        }
        if (active_before_hidden_stamp != active_after_hidden_stamp) {
            fprintf(stderr,
                    "lifecycle_test: expected hidden-layer stamp no-op before=%u after=%u\n",
                    (unsigned)active_before_hidden_stamp,
                    (unsigned)active_after_hidden_stamp);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY),
                       "workflow_toggle_active_layer_visibility_on_restore")) {
            return 1;
        }
        {
            DrawingProgramRenderInvalidation render_invalidation = {0};
            DrawingProgramRenderFrameProjection projection_visible = {0};
            DrawingProgramRenderFrameProjection projection_hidden = {0};
            uint8_t previous_visible = 0u;
            if (!expect_ok(drawing_program_document_sample_write(&workflow_ctx.document,
                                                                 workflow_center_x,
                                                                 workflow_center_y,
                                                                 104u,
                                                                 0),
                           "workflow_compose_seed_legacy_surface")) {
                return 1;
            }
            if (!expect_ok(drawing_program_layer_raster_store_sample_write(&workflow_ctx.layer_rasters,
                                                                           new_layer_id,
                                                                           workflow_center_x,
                                                                           workflow_center_y,
                                                                           200u,
                                                                           0),
                           "workflow_compose_seed_layer_surface")) {
                return 1;
            }
            if (!expect_ok(drawing_program_render_project_frame(&workflow_ctx.document,
                                                                &workflow_ctx.layer_rasters,
                                                                &workflow_ctx.editor,
                                                                &render_invalidation,
                                                                &projection_visible),
                           "workflow_compose_projection_visible")) {
                return 1;
            }
            if (!expect_ok(drawing_program_document_set_layer_visibility(&workflow_ctx.document,
                                                                         new_layer_id,
                                                                         0u,
                                                                         &previous_visible),
                           "workflow_compose_hide_new_layer")) {
                return 1;
            }
            if (!expect_ok(drawing_program_render_project_frame(&workflow_ctx.document,
                                                                &workflow_ctx.layer_rasters,
                                                                &workflow_ctx.editor,
                                                                &render_invalidation,
                                                                &projection_hidden),
                           "workflow_compose_projection_hidden")) {
                return 1;
            }
            if (projection_visible.raster_hash32 == projection_hidden.raster_hash32 ||
                projection_visible.visible_layer_count == projection_hidden.visible_layer_count) {
                fprintf(stderr,
                        "lifecycle_test: expected layer visibility to change composited projection hash/visible-count "
                        "visible(hash=%u vis=%u) hidden(hash=%u vis=%u)\n",
                        (unsigned)projection_visible.raster_hash32,
                        (unsigned)projection_visible.visible_layer_count,
                        (unsigned)projection_hidden.raster_hash32,
                        (unsigned)projection_hidden.visible_layer_count);
                return 1;
            }
            if (!expect_ok(drawing_program_document_set_layer_visibility(&workflow_ctx.document,
                                                                         new_layer_id,
                                                                         previous_visible,
                                                                         0),
                           "workflow_compose_restore_new_layer_visibility")) {
                return 1;
            }
        }
        if (policy_seed_y >= workflow_ctx.document.raster_height ||
            (policy_seed_x + 3u) >= workflow_ctx.document.raster_width) {
            fprintf(stderr, "lifecycle_test: layered selection policy seed position out of bounds\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                      &workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      base_layer_id,
                                                                      policy_seed_x,
                                                                      policy_seed_y,
                                                                      policy_base_seed_value),
                       "workflow_policy_seed_base_layer")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                      &workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      policy_seed_x + 1u,
                                                                      policy_seed_y,
                                                                      policy_active_seed_value),
                       "workflow_policy_seed_active_layer")) {
            return 1;
        }
        if (!drawing_program_selection_capture_from_rect(&workflow_ctx.document,
                                                        &workflow_ctx.layer_rasters,
                                                        new_layer_id,
                                                        &workflow_ctx.selection,
                                                        (int32_t)policy_seed_x,
                                                        (int32_t)policy_seed_y,
                                                        2u,
                                                        1u)) {
            fprintf(stderr, "lifecycle_test: expected layered selection capture on active layer\n");
            return 1;
        }
        if (!workflow_ctx.selection.has_payload ||
            workflow_ctx.selection.layer_id != new_layer_id ||
            workflow_ctx.selection.payload_count != 1u ||
            drawing_program_selection_mask_at(&workflow_ctx.selection, 0u, 0u) != 0u ||
            drawing_program_selection_mask_at(&workflow_ctx.selection, 1u, 0u) != 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected active-layer-only selection payload "
                    "(layer=%u payload=%u mask0=%u mask1=%u)\n",
                    (unsigned)workflow_ctx.selection.layer_id,
                    (unsigned)workflow_ctx.selection.payload_count,
                    (unsigned)drawing_program_selection_mask_at(&workflow_ctx.selection, 0u, 0u),
                    (unsigned)drawing_program_selection_mask_at(&workflow_ctx.selection, 1u, 0u));
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_set_active_layer_id(&workflow_ctx, base_layer_id),
                       "workflow_policy_set_active_base_before_cut")) {
            return 1;
        }
        if (!expect_ok(drawing_program_selection_cut_to_clipboard(&workflow_ctx.document,
                                                                  &workflow_ctx.layer_rasters,
                                                                  workflow_ctx.editor.active_layer_id,
                                                                  &workflow_ctx.history,
                                                                  &workflow_ctx.selection,
                                                                  &workflow_clipboard),
                       "workflow_policy_cut_selection_layer_binding")) {
            return 1;
        }
        if (!workflow_clipboard.has_payload || workflow_clipboard.source_layer_id != new_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected clipboard source layer=%u has_payload=%u got_layer=%u\n",
                    (unsigned)new_layer_id,
                    (unsigned)workflow_clipboard.has_payload,
                    (unsigned)workflow_clipboard.source_layer_id);
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      base_layer_id,
                                                                      policy_seed_x,
                                                                      policy_seed_y,
                                                                      &policy_read_value),
                       "workflow_policy_base_after_cut")) {
            return 1;
        }
        if (policy_read_value != policy_base_seed_value) {
            fprintf(stderr,
                    "lifecycle_test: expected base layer seed unchanged after cut expected=%u got=%u\n",
                    (unsigned)policy_base_seed_value,
                    (unsigned)policy_read_value);
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      policy_seed_x + 1u,
                                                                      policy_seed_y,
                                                                      &policy_read_value),
                       "workflow_policy_active_after_cut")) {
            return 1;
        }
        if (policy_read_value != expected_eraser_value) {
            fprintf(stderr,
                    "lifecycle_test: expected active-layer cut clear expected=%u got=%u\n",
                    (unsigned)expected_eraser_value,
                    (unsigned)policy_read_value);
            return 1;
        }
        if (!expect_ok(drawing_program_selection_paste_from_clipboard(&workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      workflow_ctx.editor.active_layer_id,
                                                                      &workflow_ctx.history,
                                                                      &workflow_ctx.selection,
                                                                      &workflow_clipboard,
                                                                      (int32_t)policy_seed_x + 2,
                                                                      (int32_t)policy_seed_y),
                       "workflow_policy_paste_to_active_layer")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      base_layer_id,
                                                                      policy_seed_x + 3u,
                                                                      policy_seed_y,
                                                                      &policy_read_value),
                       "workflow_policy_base_after_paste")) {
            return 1;
        }
        if (policy_read_value != policy_active_seed_value) {
            fprintf(stderr,
                    "lifecycle_test: expected paste target on active(base) layer expected=%u got=%u\n",
                    (unsigned)policy_active_seed_value,
                    (unsigned)policy_read_value);
            return 1;
        }
        if (!workflow_ctx.selection.has_payload || workflow_ctx.selection.layer_id != base_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected pasted selection to bind to active target layer=%u got=%u\n",
                    (unsigned)base_layer_id,
                    (unsigned)workflow_ctx.selection.layer_id);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_set_active_layer_id(&workflow_ctx, new_layer_id),
                       "workflow_policy_restore_active_new_before_cycle")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_PREV),
                       "workflow_select_layer_prev")) {
            return 1;
        }
        if (workflow_ctx.editor.active_layer_id != base_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected prev layer to select base id=%u got=%u\n",
                    (unsigned)base_layer_id,
                    (unsigned)workflow_ctx.editor.active_layer_id);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_NEXT),
                       "workflow_select_layer_next")) {
            return 1;
        }
        if (workflow_ctx.editor.active_layer_id != new_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected next layer to select added id=%u got=%u\n",
                    (unsigned)new_layer_id,
                    (unsigned)workflow_ctx.editor.active_layer_id);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_DOWN),
                       "workflow_move_active_layer_down")) {
            return 1;
        }
        if (workflow_ctx.document.layers[0].layer_id != new_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected move down to place active layer at index0 id=%u got=%u\n",
                    (unsigned)new_layer_id,
                    (unsigned)workflow_ctx.document.layers[0].layer_id);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_UP),
                       "workflow_move_active_layer_up")) {
            return 1;
        }
        if (workflow_ctx.document.layers[1].layer_id != new_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected move up to restore active layer at index1 id=%u got=%u\n",
                    (unsigned)new_layer_id,
                    (unsigned)workflow_ctx.document.layers[1].layer_id);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_set_active_layer_id(&workflow_ctx, base_layer_id),
                       "workflow_set_active_layer_id")) {
            return 1;
        }
        if (workflow_ctx.editor.active_layer_id != base_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected set_active_layer_id to target base id=%u got=%u\n",
                    (unsigned)base_layer_id,
                    (unsigned)workflow_ctx.editor.active_layer_id);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_set_active_layer_id(&workflow_ctx, new_layer_id),
                       "workflow_set_active_layer_new_before_delete")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_DELETE_ACTIVE_LAYER),
                       "workflow_delete_active_layer")) {
            return 1;
        }
        if (workflow_ctx.document.layer_count != 1u ||
            workflow_ctx.document.layers[0].layer_id != base_layer_id ||
            workflow_ctx.editor.active_layer_id != base_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected delete active layer to retain base only "
                    "(count=%u base=%u active=%u expected_base=%u)\n",
                    (unsigned)workflow_ctx.document.layer_count,
                    (unsigned)workflow_ctx.document.layers[0].layer_id,
                    (unsigned)workflow_ctx.editor.active_layer_id,
                    (unsigned)base_layer_id);
            return 1;
        }
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
    if (!expect_ok(drawing_program_history_undo(&ctx.history, &ctx.document, &ctx.layer_rasters), "history_undo")) {
        return 1;
    }
    if (ctx.document.layers[0].visible != 1u || ctx.history.cursor != 0u) {
        fprintf(stderr, "lifecycle_test: expected visibility restored after undo\n");
        return 1;
    }
    if (!expect_ok(drawing_program_history_redo(&ctx.history, &ctx.document, &ctx.layer_rasters), "history_redo")) {
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
        if (!expect_ok(drawing_program_history_undo(&ctx.history, &ctx.document, &ctx.layer_rasters), "history_group_undo")) {
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
        if (!expect_ok(drawing_program_history_redo(&ctx.history, &ctx.document, &ctx.layer_rasters), "history_group_redo")) {
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
        if (!expect_ok(drawing_program_history_undo(&ctx.history, &ctx.document, &ctx.layer_rasters), "history_span_undo")) {
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
        if (!expect_ok(drawing_program_history_redo(&ctx.history, &ctx.document, &ctx.layer_rasters), "history_span_redo")) {
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
        CoreResult undo_after_clear = drawing_program_history_undo(&ctx.history, &ctx.document, &ctx.layer_rasters);
        if (undo_after_clear.code != CORE_OK) {
            fprintf(stderr, "lifecycle_test: expected undo after grouped write to succeed\n");
            return 1;
        }
        undo_after_clear = drawing_program_history_undo(&ctx.history, &ctx.document, &ctx.layer_rasters);
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
    {
        DrawingProgramAppContext layer_save_ctx;
        DrawingProgramAppContext layer_load_ctx;
        DrawingProgramAppContext legacy_load_ctx;
        char layer_arg0[] = "drawing_program_layer_snapshot_roundtrip";
        char layer_arg1[] = "--headless";
        char layer_arg2[] = "--smoke-frames";
        char layer_arg3[] = "1";
        char layer_arg4[] = "--preset";
        char layer_arg5[] = "/tmp/drawing_program_layer_roundtrip.pack";
        char *layer_argv[] = { layer_arg0, layer_arg1, layer_arg2, layer_arg3, layer_arg4, layer_arg5, 0 };
        const char *legacy_pack_path = "/tmp/drawing_program_layer_roundtrip_legacy.pack";
        uint32_t layer_sample_x = 13u;
        uint32_t layer_sample_y = 17u;
        uint8_t layer_sample_value = 173u;
        uint8_t loaded_layer_sample = 0u;
        uint8_t migrated_layer_sample = 0u;
        uint8_t document_seed_sample = 131u;
        uint32_t new_layer_id = 0u;
        (void)unlink(layer_arg5);
        (void)unlink(legacy_pack_path);
        if (!expect_ok(drawing_program_app_bootstrap(&layer_save_ctx, 6, layer_argv), "layer_snapshot_bootstrap_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&layer_save_ctx), "layer_snapshot_config_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&layer_save_ctx), "layer_snapshot_state_seed_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_subsystems_init(&layer_save_ctx), "layer_snapshot_subsystems_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_start(&layer_save_ctx), "layer_snapshot_runtime_start_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &layer_save_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_ADD_LAYER),
                       "layer_snapshot_add_layer")) {
            return 1;
        }
        new_layer_id = layer_save_ctx.editor.active_layer_id;
        if (!expect_ok(drawing_program_document_sample_write(&layer_save_ctx.document,
                                                             layer_sample_x,
                                                             layer_sample_y,
                                                             document_seed_sample,
                                                             0),
                       "layer_snapshot_seed_legacy_document_surface")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_write(&layer_save_ctx.layer_rasters,
                                                                       new_layer_id,
                                                                       layer_sample_x,
                                                                       layer_sample_y,
                                                                       layer_sample_value,
                                                                       0),
                       "layer_snapshot_seed_new_layer_surface")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_save(&layer_save_ctx, layer_arg5), "layer_snapshot_save")) {
            return 1;
        }
        if (!write_legacy_snapshot_without_layer_chunk(layer_arg5, legacy_pack_path)) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&layer_load_ctx, 6, layer_argv), "layer_snapshot_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&layer_load_ctx), "layer_snapshot_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&layer_load_ctx), "layer_snapshot_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&layer_load_ctx, layer_arg5), "layer_snapshot_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&layer_load_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      layer_sample_x,
                                                                      layer_sample_y,
                                                                      &loaded_layer_sample),
                       "layer_snapshot_loaded_new_layer_sample")) {
            return 1;
        }
        if (loaded_layer_sample != layer_sample_value) {
            fprintf(stderr,
                    "lifecycle_test: expected layer chunk sample roundtrip=%u got=%u\n",
                    (unsigned)layer_sample_value,
                    (unsigned)loaded_layer_sample);
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&legacy_load_ctx, 6, layer_argv), "legacy_layer_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&legacy_load_ctx), "legacy_layer_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&legacy_load_ctx), "legacy_layer_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&legacy_load_ctx, legacy_pack_path), "legacy_layer_snapshot_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&legacy_load_ctx.layer_rasters,
                                                                      legacy_load_ctx.document.layers[0].layer_id,
                                                                      layer_sample_x,
                                                                      layer_sample_y,
                                                                      &migrated_layer_sample),
                       "legacy_layer_migration_seed_read")) {
            return 1;
        }
        if (migrated_layer_sample != document_seed_sample) {
            fprintf(stderr,
                    "lifecycle_test: expected legacy migration to seed base layer sample=%u got=%u\n",
                    (unsigned)document_seed_sample,
                    (unsigned)migrated_layer_sample);
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&layer_save_ctx), "layer_snapshot_shutdown_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&layer_load_ctx), "layer_snapshot_shutdown_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&legacy_load_ctx), "legacy_layer_shutdown_load")) {
            return 1;
        }
        (void)unlink(layer_arg5);
        (void)unlink(legacy_pack_path);
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
                                                                  &ctx.layer_rasters,
                                                                  ctx.editor.active_layer_id,
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
    if (!expect_ok(drawing_program_history_undo(&ctx.history, &ctx.document, &ctx.layer_rasters), "history_undo_brush_seed")) {
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
    if (!expect_ok(drawing_program_history_redo(&ctx.history, &ctx.document, &ctx.layer_rasters), "history_redo_brush_seed")) {
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
        uint8_t sample = 0u;
        uint8_t bg = drawing_program_color_eraser_value();
        ctx.ui_active_color_index = drawing_program_color_default_index();
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "s3_shape_mode_clear_canvas_fill_rect")) {
            return 1;
        }
        ctx.ui_tool_shape_mode = 1u; /* FILL */
        ctx.ui_tool_shape_stroke_width = 1u;
        if (!expect_ok(drawing_program_app_shape_commit_samples(&ctx,
                                                                DRAWING_PROGRAM_TOOL_RECT,
                                                                8u,
                                                                8u,
                                                                12u,
                                                                12u),
                       "s3_shape_mode_rect_fill_commit")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 10u, 10u, &sample),
                       "s3_shape_mode_rect_fill_center_sample")) {
            return 1;
        }
        if (sample != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected rect fill center=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)sample);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "s3_shape_mode_clear_canvas_outline_rect")) {
            return 1;
        }
        ctx.ui_tool_shape_mode = 0u; /* OUTLINE */
        ctx.ui_tool_shape_stroke_width = 1u;
        if (!expect_ok(drawing_program_app_shape_commit_samples(&ctx,
                                                                DRAWING_PROGRAM_TOOL_RECT,
                                                                8u,
                                                                8u,
                                                                12u,
                                                                12u),
                       "s3_shape_mode_rect_outline_commit")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 10u, 10u, &sample),
                       "s3_shape_mode_rect_outline_center_sample")) {
            return 1;
        }
        if (sample != bg) {
            fprintf(stderr,
                    "lifecycle_test: expected rect outline center background=%u got=%u\n",
                    (unsigned)bg,
                    (unsigned)sample);
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 8u, 10u, &sample),
                       "s3_shape_mode_rect_outline_edge_sample")) {
            return 1;
        }
        if (sample != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected rect outline edge=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)sample);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "s3_shape_mode_clear_canvas_line_width")) {
            return 1;
        }
        ctx.ui_tool_shape_mode = 0u; /* OUTLINE */
        ctx.ui_tool_shape_stroke_width = 3u;
        if (!expect_ok(drawing_program_app_shape_commit_samples(&ctx,
                                                                DRAWING_PROGRAM_TOOL_LINE,
                                                                40u,
                                                                20u,
                                                                40u,
                                                                28u),
                       "s3_shape_mode_line_stroke_commit")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 41u, 24u, &sample),
                       "s3_shape_mode_line_stroke_adjacent_sample")) {
            return 1;
        }
        if (sample != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected thick line adjacent=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)sample);
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 43u, 24u, &sample),
                       "s3_shape_mode_line_stroke_outside_sample")) {
            return 1;
        }
        if (sample != bg) {
            fprintf(stderr,
                    "lifecycle_test: expected thick line outside background=%u got=%u\n",
                    (unsigned)bg,
                    (unsigned)sample);
            return 1;
        }
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
        save_ctx.ui_tool_brush_size = 7u;
        save_ctx.ui_tool_brush_opacity = 65u;
        save_ctx.ui_tool_eraser_size = 5u;
        save_ctx.ui_tool_shape_stroke_width = 4u;
        save_ctx.ui_tool_shape_mode = 2u;
        save_ctx.ui_tool_fill_tolerance = 3u;
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
            load_ctx.ui_active_color_index != 6u ||
            load_ctx.ui_tool_brush_size != 7u ||
            load_ctx.ui_tool_brush_opacity != 65u ||
            load_ctx.ui_tool_eraser_size != 5u ||
            load_ctx.ui_tool_shape_stroke_width != 4u ||
            load_ctx.ui_tool_shape_mode != 2u ||
            load_ctx.ui_tool_fill_tolerance != 3u) {
            fprintf(stderr,
                    "lifecycle_test: persistence roundtrip mismatch tool=%u theme=%u zoom=%d left=%u right=%u color=%u brush=%u/%u eraser=%u shape=%u mode=%u fill_tol=%u\n",
                    (unsigned)load_ctx.editor.active_tool,
                    (unsigned)load_ctx.ui_theme_preset_id,
                    (int)load_ctx.ui_font_zoom_step,
                    (unsigned)load_ctx.ui_left_panel_slot,
                    (unsigned)load_ctx.ui_right_panel_slot,
                    (unsigned)load_ctx.ui_active_color_index,
                    (unsigned)load_ctx.ui_tool_brush_size,
                    (unsigned)load_ctx.ui_tool_brush_opacity,
                    (unsigned)load_ctx.ui_tool_eraser_size,
                    (unsigned)load_ctx.ui_tool_shape_stroke_width,
                    (unsigned)load_ctx.ui_tool_shape_mode,
                    (unsigned)load_ctx.ui_tool_fill_tolerance);
            return 1;
        }
        load_ctx.persist_enabled = 0u;
        if (!expect_ok(drawing_program_app_shutdown(&load_ctx), "persist_roundtrip_shutdown_load")) {
            return 1;
        }
    }
    return 0;
}
