#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core_pack.h"
#include "core_theme.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_visual_input_handlers.h"
#include "drawing_program/drawing_program_visual_input_selection_ops.h"
#include "drawing_program/drawing_program_visual_input_support.h"
#include "drawing_program/drawing_program_visual_transform_ops.h"
#include "drawing_program_lifecycle_test_support.h"

#include "drawing_program_lifecycle_snapshot_suite.h"
#include "drawing_program_lifecycle_selection_layer_suite.h"
#include "drawing_program_lifecycle_baseline_history_suite.h"
#include "drawing_program_lifecycle_object_path_suite.h"
#include "drawing_program_lifecycle_runtime_render_suite.h"

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
    char arg0[] = "drawing_program_test";
    char arg1[] = "--headless";
    char arg2[] = "--smoke-frames";
    char arg3[] = "2";
    char arg4[] = "--no-persist";
    char arg5[] = "--canvas-size";
    char arg6[] = "640x360";
    char *argv[] = { arg0, arg1, arg2, arg3, arg4, 0 };
    char *size_argv[] = { arg0, arg1, arg2, arg3, arg4, arg5, arg6, 0 };
    uint8_t expected_draw_value = 0u;
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
    if (!expect_ok(drawing_program_app_subsystems_init(&workflow_ctx), "workflow_subsystems_init")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&workflow_ctx), "workflow_runtime_start")) {
        return 1;
    }
    expected_draw_value = drawing_program_color_value_from_index(
        drawing_program_color_index_clamp(workflow_ctx.ui.active_color_index));
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
    expected_draw_value = drawing_program_color_value_from_index(
        drawing_program_color_index_clamp(workflow_ctx.ui.active_color_index));
    if (workflow_center_value != expected_draw_value) {
        fprintf(stderr,
                "lifecycle_test: expected workflow brush stamp to set center sample to %u got=%u\n",
                (unsigned)expected_draw_value,
                (unsigned)workflow_center_value);
        return 1;
    }
    if (drawing_program_lifecycle_run_selection_layer_suite(&workflow_ctx,
                                                            &workflow_clipboard,
                                                            argv,
                                                            workflow_center_x,
                                                            workflow_center_y,
                                                            expected_draw_value,
                                                            expected_eraser_value) != 0) {
        return 1;
    }
    if (workflow_ctx.runtime.tool_switch_total != 2u) {
        fprintf(stderr,
                "lifecycle_test: expected workflow tool switch total=2 got=%llu\n",
                (unsigned long long)workflow_ctx.runtime.tool_switch_total);
        return 1;
    }
    if (drawing_program_lifecycle_run_baseline_history_suite(&ctx,
                                                             expected_eraser_value,
                                                             &center_x,
                                                             &center_y,
                                                             &center_before) != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_snapshot_suite(&ctx) != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_object_path_suite(&workflow_ctx,
                                                        expected_draw_value,
                                                        expected_eraser_value) != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_runtime_render_suite(&ctx,
                                                           &workflow_ctx,
                                                           center_x,
                                                           center_y,
                                                           center_before,
                                                           expected_draw_value,
                                                           expected_eraser_value) != 0) {
        return 1;
    }
    return 0;
}
