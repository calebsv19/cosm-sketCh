#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_history.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_ui_color_state.h"
#include "drawing_program/drawing_program_visual_canvas_draw_action_ops.h"
#include "drawing_program/drawing_program_visual_canvas_stroke_ops.h"
#include "drawing_program/drawing_program_visual_shape_ops.h"
#include "drawing_program/drawing_program_visual_transform_ops.h"
#include "drawing_program_lifecycle_runtime_ui_suite.h"
#include "drawing_program_lifecycle_test_support.h"

CoreResult drawing_program_visual_apply_canvas_stamp_square_on_layer(DrawingProgramAppContext *ctx,
                                                                     uint32_t layer_id,
                                                                     int32_t sample_x,
                                                                     int32_t sample_y,
                                                                     DrawingProgramRasterSample value,
                                                                     uint32_t stroke_width,
                                                                     uint8_t hardness_percent);

static int runtime_ui_screen_to_canvas_sample(const DrawingProgramAppContext *ctx,
                                              SDL_Rect pane_rect,
                                              int sx,
                                              int sy,
                                              uint32_t *out_sample_x,
                                              uint32_t *out_sample_y) {
    (void)ctx;
    (void)pane_rect;
    if (!out_sample_x || !out_sample_y || sx < 0 || sy < 0) {
        return 0;
    }
    *out_sample_x = (uint32_t)sx;
    *out_sample_y = (uint32_t)sy;
    return 1;
}

static int runtime_ui_active_layer_query(const DrawingProgramAppContext *ctx,
                                         uint32_t *out_layer_id,
                                         uint32_t *out_index,
                                         uint8_t *out_visible,
                                         uint8_t *out_locked) {
    CoreResult result = drawing_program_runtime_orchestration_resolve_active_layer(
        ctx, out_layer_id, out_index, out_visible, out_locked);
    return (result.code == CORE_OK) ? 1 : 0;
}

static int runtime_ui_active_layer_allows_edits(const DrawingProgramAppContext *ctx) {
    uint8_t visible = 0u;
    uint8_t locked = 0u;
    if (!runtime_ui_active_layer_query(ctx, 0, 0, &visible, &locked)) {
        return 0;
    }
    return (visible && !locked) ? 1 : 0;
}

static const DrawingProgramVisualCanvasDrawActionOpsHooks *runtime_ui_draw_hooks(void) {
    static const DrawingProgramVisualCanvasDrawActionOpsHooks hooks = {
        .screen_to_canvas_sample = runtime_ui_screen_to_canvas_sample,
        .active_layer_allows_edits_visual = runtime_ui_active_layer_allows_edits,
        .active_layer_query = runtime_ui_active_layer_query,
        .sample_value_for_tool = drawing_program_visual_sample_value_for_tool,
        .tool_brush_radius_samples = drawing_program_visual_tool_brush_radius_samples,
        .tool_brush_spacing_samples = drawing_program_visual_tool_brush_spacing_samples,
        .tool_brush_hardness_percent = drawing_program_visual_tool_brush_hardness_percent,
        .seeded_background_sample_for_coord = drawing_program_visual_seeded_background_sample_for_coord,
        .begin_canvas_history_group = drawing_program_visual_begin_canvas_history_group,
        .end_canvas_history_group = drawing_program_visual_end_canvas_history_group,
        .apply_canvas_stamp_square_on_layer = drawing_program_visual_apply_canvas_stamp_square_on_layer,
        .apply_canvas_direct_stroke_stamp_square_on_layer =
            drawing_program_visual_apply_canvas_direct_stroke_stamp_square_on_layer
    };
    return &hooks;
}

int drawing_program_lifecycle_run_runtime_ui_suite(DrawingProgramAppContext *ctx_ptr,
                                                   uint32_t center_x,
                                                   uint32_t center_y,
                                                   uint8_t expected_draw_value) {
    uint8_t center_after = 0u;
    uint8_t center_before_runtime = 0u;
    uint8_t center_undo = 0u;
    uint8_t center_redo = 0u;
#define ctx (*ctx_ptr)

    if (!expect_ok(drawing_program_document_sample_read(&ctx.document, center_x, center_y, &center_before_runtime),
                   "sample_read_center_before_runtime_ui")) {
        return 1;
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
    if (ctx.editor.active_tool != DRAWING_PROGRAM_TOOL_SELECT || ctx.runtime.tool_switch_total != 1u) {
        fprintf(stderr,
                "lifecycle_test: expected tool switch to SELECT with count=1 got tool=%u switches=%llu\n",
                (unsigned)ctx.editor.active_tool,
                (unsigned long long)ctx.runtime.tool_switch_total);
        return 1;
    }
    if (ctx.runtime.viewport_sample_probe_success_total != 2u) {
        fprintf(stderr, "lifecycle_test: expected viewport probe success count 2 got=%llu\n",
                (unsigned long long)ctx.runtime.viewport_sample_probe_success_total);
        return 1;
    }
    if (ctx.runtime.frame_counter != 2u) {
        fprintf(stderr, "lifecycle_test: expected frame_counter=2 got=%llu\n",
                (unsigned long long)ctx.runtime.frame_counter);
        return 1;
    }
    if (ctx.runtime.input_events_processed != 4u) {
        fprintf(stderr, "lifecycle_test: expected input_events_processed=4 got=%llu\n",
                (unsigned long long)ctx.runtime.input_events_processed);
        return 1;
    }
    if (ctx.runtime.input_actions_emitted != 4u) {
        fprintf(stderr, "lifecycle_test: expected input_actions_emitted=4 got=%llu\n",
                (unsigned long long)ctx.runtime.input_actions_emitted);
        return 1;
    }
    if (ctx.runtime.routed_global_total != 2u ||
        ctx.runtime.routed_pane_total != 2u ||
        ctx.runtime.routed_fallback_total != 0u) {
        fprintf(stderr,
                "lifecycle_test: unexpected route totals g=%llu p=%llu f=%llu\n",
                (unsigned long long)ctx.runtime.routed_global_total,
                (unsigned long long)ctx.runtime.routed_pane_total,
                (unsigned long long)ctx.runtime.routed_fallback_total);
        return 1;
    }
    if (ctx.runtime.invalidation_target_total != 1u || ctx.runtime.invalidation_full_total != 1u) {
        fprintf(stderr,
                "lifecycle_test: unexpected invalidation totals target=%llu full=%llu\n",
                (unsigned long long)ctx.runtime.invalidation_target_total,
                (unsigned long long)ctx.runtime.invalidation_full_total);
        return 1;
    }
    if (ctx.runtime.render_frames_projected_total != 2u ||
        ctx.runtime.render_layers_visible_total != 2u ||
        ctx.runtime.render_full_redraw_total != 1u ||
        ctx.runtime.render_target_redraw_total != 1u) {
        fprintf(stderr,
                "lifecycle_test: unexpected render projection totals frames=%llu visible=%llu full=%llu target=%llu\n",
                (unsigned long long)ctx.runtime.render_frames_projected_total,
                (unsigned long long)ctx.runtime.render_layers_visible_total,
                (unsigned long long)ctx.runtime.render_full_redraw_total,
                (unsigned long long)ctx.runtime.render_target_redraw_total);
        return 1;
    }
    if (ctx.runtime.render_module_calls_total != 12u ||
        ctx.runtime.render_module_canvas_calls_total != 3u ||
        ctx.runtime.render_module_palette_calls_total != 3u) {
        fprintf(stderr,
                "lifecycle_test: unexpected module render call totals total=%llu canvas=%llu palette=%llu\n",
                (unsigned long long)ctx.runtime.render_module_calls_total,
                (unsigned long long)ctx.runtime.render_module_canvas_calls_total,
                (unsigned long long)ctx.runtime.render_module_palette_calls_total);
        return 1;
    }
    if (!expect_ok(drawing_program_history_undo(&ctx.history, &ctx.document, &ctx.layer_rasters, &ctx.object_store),
                   "history_undo_brush_seed")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&ctx.document, center_x, center_y, &center_undo),
                   "sample_read_center_after_undo")) {
        return 1;
    }
    if (center_undo != center_before_runtime) {
        fprintf(stderr, "lifecycle_test: expected center sample restore on undo before_runtime=%u after_undo=%u\n",
                (unsigned)center_before_runtime,
                (unsigned)center_undo);
        return 1;
    }
    if (!expect_ok(drawing_program_history_redo(&ctx.history, &ctx.document, &ctx.layer_rasters, &ctx.object_store),
                   "history_redo_brush_seed")) {
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
    if (ctx.runtime.render_projection.raster_sample_count == 0u ||
        ctx.runtime.render_projection.visible_layer_count == 0u ||
        ctx.runtime.render_active_surface_content_revision == 0u) {
        fprintf(stderr, "lifecycle_test: expected raster projection baseline populated\n");
        return 1;
    }
    if (ctx.runtime.render_layer_opacity_revision == 0u) {
        fprintf(stderr, "lifecycle_test: expected layer opacity revision baseline populated\n");
        return 1;
    }
    if (!ctx.runtime.render_last_has_active_layer || ctx.runtime.render_last_active_layer_id != 1u) {
        fprintf(stderr,
                "lifecycle_test: expected active layer projection id=1 has_active=1 got id=%u has_active=%u\n",
                ctx.runtime.render_last_active_layer_id,
                (unsigned)ctx.runtime.render_last_has_active_layer);
        return 1;
    }
    {
        uint8_t sample = 0u;
        uint8_t bg =
            drawing_program_color_legacy_sample_from_sample(drawing_program_color_eraser_value());
        drawing_program_ui_color_load_active_paint_from_swatch(&ctx, drawing_program_color_default_index());
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "s3_shape_mode_clear_canvas_fill_rect")) {
            return 1;
        }
        ctx.ui.tool_shape_mode = 1u;
        ctx.ui.tool_shape_stroke_width = 1u;
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
        ctx.ui.tool_shape_mode = 0u;
        ctx.ui.tool_shape_stroke_width = 1u;
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
        ctx.ui.tool_shape_mode = 0u;
        ctx.ui.tool_shape_stroke_width = 3u;
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
        drawing_program_history_clear(&ctx.history);
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "s3_shape_history_clear_canvas_long_line")) {
            return 1;
        }
        {
            uint32_t history_units = 0u;
            uint32_t history_total_units = 0u;
            uint32_t line_start_x = 24u;
            uint32_t line_end_x = 0u;
            if (ctx.document.raster_width <= 96u) {
                fprintf(stderr, "lifecycle_test: expected canvas width for long line history regression\n");
                return 1;
            }
            line_end_x = ctx.document.raster_width - 24u;
            ctx.ui.tool_shape_mode = 0u;
            ctx.ui.tool_shape_stroke_width = 16u;
            if (!expect_ok(drawing_program_app_shape_commit_samples(&ctx,
                                                                    DRAWING_PROGRAM_TOOL_LINE,
                                                                    line_start_x,
                                                                    48u,
                                                                    line_end_x,
                                                                    48u),
                           "s3_shape_history_long_line_commit")) {
                return 1;
            }
            if (!expect_ok(drawing_program_document_sample_read(&ctx.document,
                                                                (line_start_x + line_end_x) / 2u,
                                                                48u,
                                                                &sample),
                           "s3_shape_history_long_line_probe")) {
                return 1;
            }
            if (sample != expected_draw_value) {
                fprintf(stderr,
                        "lifecycle_test: expected long line history sample=%u got=%u\n",
                        (unsigned)expected_draw_value,
                        (unsigned)sample);
                return 1;
            }
            drawing_program_history_query_units(&ctx.history, &history_units, &history_total_units);
            if (history_units != 1u || history_total_units != 1u || ctx.history.count <= 3u) {
                fprintf(stderr,
                        "lifecycle_test: expected long line delta batching to keep one undo unit and multiple commands cursor=%u total=%u count=%u\n",
                        (unsigned)history_units,
                        (unsigned)history_total_units,
                        (unsigned)ctx.history.count);
                return 1;
            }
            if (!expect_ok(drawing_program_history_undo(&ctx.history,
                                                        &ctx.document,
                                                        &ctx.layer_rasters,
                                                        &ctx.object_store),
                           "s3_shape_history_long_line_undo")) {
                return 1;
            }
            if (!expect_ok(drawing_program_document_sample_read(&ctx.document,
                                                                (line_start_x + line_end_x) / 2u,
                                                                48u,
                                                                &sample),
                           "s3_shape_history_long_line_probe_after_undo")) {
                return 1;
            }
            if (sample != bg) {
                fprintf(stderr,
                        "lifecycle_test: expected long line undo to restore background=%u got=%u\n",
                        (unsigned)bg,
                        (unsigned)sample);
                return 1;
            }
        }
        drawing_program_history_clear(&ctx.history);
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "s3_shape_history_clear_canvas_large_rect")) {
            return 1;
        }
        {
            uint32_t history_units_before = 0u;
            uint32_t history_units_after = 0u;
            uint32_t history_total_units = 0u;
            uint32_t undo_steps = 0u;
            uint32_t rect_x0 = 24u;
            uint32_t rect_y0 = 24u;
            uint32_t rect_x1 = rect_x0 + 255u;
            uint32_t rect_y1 = rect_y0 + 159u;
            if (ctx.document.raster_width <= rect_x1 + 1u ||
                ctx.document.raster_height <= rect_y1 + 1u) {
                fprintf(stderr,
                        "lifecycle_test: expected canvas size for large rect history regression raster=%ux%u need>%ux%u\n",
                        (unsigned)ctx.document.raster_width,
                        (unsigned)ctx.document.raster_height,
                        (unsigned)rect_x1,
                        (unsigned)rect_y1);
                return 1;
            }
            ctx.ui.tool_shape_mode = 1u;
            ctx.ui.tool_shape_stroke_width = 1u;
            drawing_program_history_query_units(&ctx.history, &history_units_before, 0);
            if (!expect_ok(drawing_program_app_shape_commit_samples(&ctx,
                                                                    DRAWING_PROGRAM_TOOL_RECT,
                                                                    rect_x0,
                                                                    rect_y0,
                                                                    rect_x1,
                                                                    rect_y1),
                           "s3_shape_history_large_rect_commit")) {
                return 1;
            }
            if (!expect_ok(drawing_program_document_sample_read(&ctx.document, rect_x0 + 10u, rect_y0 + 10u, &sample),
                           "s3_shape_history_large_rect_probe")) {
                return 1;
            }
            if (sample != expected_draw_value) {
                fprintf(stderr,
                        "lifecycle_test: expected large rect fill sample=%u got=%u\n",
                        (unsigned)expected_draw_value,
                        (unsigned)sample);
                return 1;
            }
            drawing_program_history_query_units(&ctx.history, &history_units_after, &history_total_units);
            if (history_units_after <= history_units_before + 1u) {
                fprintf(stderr,
                        "lifecycle_test: expected large rect fill to split undo units before=%u after=%u total=%u\n",
                        (unsigned)history_units_before,
                        (unsigned)history_units_after,
                        (unsigned)history_total_units);
                return 1;
            }
            while (history_units_after > history_units_before) {
                if (!expect_ok(drawing_program_history_undo(&ctx.history,
                                                            &ctx.document,
                                                            &ctx.layer_rasters,
                                                            &ctx.object_store),
                               "s3_shape_history_large_rect_undo")) {
                    return 1;
                }
                undo_steps += 1u;
                drawing_program_history_query_units(&ctx.history, &history_units_after, 0);
            }
            if (undo_steps < 2u) {
                fprintf(stderr,
                        "lifecycle_test: expected large rect undo to require multiple units got=%u\n",
                        (unsigned)undo_steps);
                return 1;
            }
            if (!expect_ok(drawing_program_document_sample_read(&ctx.document, rect_x0 + 10u, rect_y0 + 10u, &sample),
                           "s3_shape_history_large_rect_probe_after_undo")) {
                return 1;
            }
            if (sample != bg) {
                fprintf(stderr,
                        "lifecycle_test: expected large rect undo to restore background=%u got=%u\n",
                        (unsigned)bg,
                        (unsigned)sample);
                return 1;
            }
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "s3_reflection_clear_canvas_brush")) {
            return 1;
        }
        {
            VisualCanvasInteractionState reflection_draw_state;
            memset(&reflection_draw_state, 0, sizeof(reflection_draw_state));
            ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_BRUSH;
            ctx.ui.tool_brush_size = 1u;
            ctx.ui.tool_brush_spacing = 1u;
            ctx.ui.tool_brush_hardness = 100u;
            ctx.editor.symmetry_horizontal = 1u;
            ctx.editor.symmetry_vertical = 1u;
            if (!drawing_program_canvas_reflection_set_active_center(&ctx, 20u, 20u)) {
                fprintf(stderr, "lifecycle_test: expected reflection center set to succeed for brush mirror test\n");
                return 1;
            }
            drawing_program_canvas_reflection_sync_active_surface_from_editor(&ctx);
            if (!expect_ok(drawing_program_visual_apply_canvas_draw_at_screen(&ctx,
                                                                              (SDL_Rect){ 0, 0, 128, 128 },
                                                                              18,
                                                                              19,
                                                                              &reflection_draw_state,
                                                                              runtime_ui_draw_hooks()),
                           "s3_reflection_brush_apply")) {
                return 1;
            }
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 18u, 19u, &sample),
                       "s3_reflection_brush_source_sample")) {
            return 1;
        }
        if (sample != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected reflection brush source=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)sample);
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 22u, 19u, &sample),
                       "s3_reflection_brush_vertical_sample")) {
            return 1;
        }
        if (sample != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected reflection brush vertical=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)sample);
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 18u, 21u, &sample),
                       "s3_reflection_brush_horizontal_sample")) {
            return 1;
        }
        if (sample != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected reflection brush horizontal=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)sample);
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 22u, 21u, &sample),
                       "s3_reflection_brush_quad_sample")) {
            return 1;
        }
        if (sample != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected reflection brush quad=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)sample);
            return 1;
        }
        drawing_program_history_clear(&ctx.history);
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "s3_reflection_clear_canvas_brush_history")) {
            return 1;
        }
        drawing_program_history_clear(&ctx.history);
        {
            VisualCanvasInteractionState reflection_span_state;
            memset(&reflection_span_state, 0, sizeof(reflection_span_state));
            ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_BRUSH;
            ctx.ui.tool_brush_size = 2u;
            ctx.ui.tool_brush_spacing = 1u;
            ctx.ui.tool_brush_hardness = 100u;
            ctx.editor.symmetry_horizontal = 1u;
            ctx.editor.symmetry_vertical = 1u;
            if (!drawing_program_canvas_reflection_set_active_center(&ctx, 40u, 40u)) {
                fprintf(stderr, "lifecycle_test: expected reflection center set to succeed for brush history test\n");
                return 1;
            }
            drawing_program_canvas_reflection_sync_active_surface_from_editor(&ctx);
            drawing_program_visual_begin_canvas_history_group(&ctx);
            if (!expect_ok(drawing_program_visual_apply_canvas_draw_at_screen(&ctx,
                                                                              (SDL_Rect){ 0, 0, 128, 128 },
                                                                              30,
                                                                              31,
                                                                              &reflection_span_state,
                                                                              runtime_ui_draw_hooks()),
                           "s3_reflection_brush_span_apply")) {
                return 1;
            }
            if (!expect_ok(drawing_program_visual_flush_direct_stroke_history(
                               &ctx, &reflection_span_state, reflection_span_state.direct_stroke_history_layer_id),
                           "s3_reflection_brush_span_flush")) {
                return 1;
            }
            drawing_program_visual_end_canvas_history_group(&ctx);
        }
        if (ctx.history.count != 3u) {
            fprintf(stderr,
                    "lifecycle_test: expected reflected hard-brush delta command count=3 got=%u\n",
                    (unsigned)ctx.history.count);
            return 1;
        }
        drawing_program_history_clear(&ctx.history);
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "s3_reflection_clear_canvas_long_stroke")) {
            return 1;
        }
        {
            VisualCanvasInteractionState long_stroke_state;
            uint32_t history_units = 0u;
            uint32_t history_total_units = 0u;
            uint32_t start_x = 8u;
            uint32_t start_y = 24u;
            uint32_t long_steps =
                (ctx.document.raster_width > (start_x + 270u)) ? 270u : 0u;
            uint32_t step = 0u;
            memset(&long_stroke_state, 0, sizeof(long_stroke_state));
            if (long_steps == 0u) {
                fprintf(stderr,
                        "lifecycle_test: expected canvas width to support long-stroke chunking regression\n");
                return 1;
            }
            ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_BRUSH;
            ctx.ui.tool_brush_size = 1u;
            ctx.ui.tool_brush_spacing = 1u;
            ctx.ui.tool_brush_hardness = 100u;
            ctx.editor.symmetry_horizontal = 0u;
            ctx.editor.symmetry_vertical = 0u;
            drawing_program_visual_begin_canvas_history_group(&ctx);
            for (step = 0u; step <= long_steps; ++step) {
                if (!expect_ok(drawing_program_visual_apply_canvas_draw_at_screen(&ctx,
                                                                                  (SDL_Rect){ 0, 0, 512, 512 },
                                                                                  (int)(start_x + step),
                                                                                  (int)start_y,
                                                                                  &long_stroke_state,
                                                                                  runtime_ui_draw_hooks()),
                               "s3_long_stroke_chunk_apply")) {
                    return 1;
                }
            }
            if (!expect_ok(drawing_program_visual_flush_direct_stroke_history(
                               &ctx, &long_stroke_state, long_stroke_state.direct_stroke_history_layer_id),
                           "s3_long_stroke_chunk_flush")) {
                return 1;
            }
            drawing_program_visual_end_canvas_history_group(&ctx);
            drawing_program_history_query_units(&ctx.history, &history_units, &history_total_units);
            if (history_units != 2u || history_total_units != 2u) {
                fprintf(stderr,
                        "lifecycle_test: expected long stroke chunking to create 2 undo units got cursor=%u total=%u\n",
                        (unsigned)history_units,
                        (unsigned)history_total_units);
                return 1;
            }
        }
        drawing_program_history_clear(&ctx.history);
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "s3_reflection_clear_canvas_rect")) {
            return 1;
        }
        ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_SELECT;
        ctx.ui.tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
        ctx.ui.tool_shape_mode = 2u;
        ctx.ui.tool_shape_stroke_width = 1u;
        ctx.editor.symmetry_horizontal = 1u;
        ctx.editor.symmetry_vertical = 1u;
        if (!drawing_program_canvas_reflection_set_active_center(&ctx, 20u, 20u)) {
            fprintf(stderr, "lifecycle_test: expected reflection center set to succeed for rect mirror test\n");
            return 1;
        }
        drawing_program_canvas_reflection_sync_active_surface_from_editor(&ctx);
        if (!expect_ok(drawing_program_app_shape_commit_samples(&ctx,
                                                                DRAWING_PROGRAM_TOOL_RECT,
                                                                12u,
                                                                12u,
                                                                14u,
                                                                14u),
                       "s3_reflection_rect_commit")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 13u, 13u, &sample),
                       "s3_reflection_rect_source_fill")) {
            return 1;
        }
        if (sample != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected reflection rect source=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)sample);
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 27u, 13u, &sample),
                       "s3_reflection_rect_vertical_fill")) {
            return 1;
        }
        if (sample != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected reflection rect vertical=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)sample);
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 13u, 27u, &sample),
                       "s3_reflection_rect_horizontal_fill")) {
            return 1;
        }
        if (sample != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected reflection rect horizontal=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)sample);
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 27u, 27u, &sample),
                       "s3_reflection_rect_quad_fill")) {
            return 1;
        }
        if (sample != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected reflection rect quad=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)sample);
            return 1;
        }
        drawing_program_object_store_reset(&ctx.object_store);
        drawing_program_object_selection_reset(&ctx.object_selection);
        ctx.ui.tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_OBJECT;
        ctx.ui.tool_shape_mode = 2u;
        ctx.ui.tool_shape_stroke_width = 2u;
        ctx.editor.symmetry_horizontal = 0u;
        ctx.editor.symmetry_vertical = 1u;
        if (!drawing_program_canvas_reflection_set_active_center(&ctx, 20u, 20u)) {
            fprintf(stderr, "lifecycle_test: expected reflection center set to succeed for object mirror test\n");
            return 1;
        }
        drawing_program_canvas_reflection_sync_active_surface_from_editor(&ctx);
        if (!expect_ok(drawing_program_app_shape_commit_samples(&ctx,
                                                                DRAWING_PROGRAM_TOOL_RECT,
                                                                14u,
                                                                14u,
                                                                22u,
                                                                18u),
                       "s3_shape_target_rect_object_commit")) {
            return 1;
        }
        if (ctx.object_store.object_count != 2u ||
            ctx.object_selection.count != 2u ||
            ctx.object_selection.active_object_id == 0u) {
            fprintf(stderr,
                    "lifecycle_test: expected reflected rect object target to create two selected objects count=%u sel=%u active=%u\n",
                    (unsigned)ctx.object_store.object_count,
                    (unsigned)ctx.object_selection.count,
                    (unsigned)ctx.object_selection.active_object_id);
            return 1;
        }
        {
            uint32_t rect_hits = 0u;
            uint32_t object_index = 0u;
            for (object_index = 0u; object_index < ctx.object_store.object_count; ++object_index) {
                const DrawingProgramObjectRecord *object = &ctx.object_store.objects[object_index];
                if (object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT &&
                    object->width == 9u &&
                    object->height == 5u &&
                    ((object->origin_x == 14 && object->origin_y == 14) ||
                     (object->origin_x == 18 && object->origin_y == 14))) {
                    rect_hits += 1u;
                }
            }
            if (rect_hits != 2u) {
                fprintf(stderr,
                        "lifecycle_test: expected reflected rect objects at 14,14 and 18,14 hits=%u\n",
                        (unsigned)rect_hits);
                return 1;
            }
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 18u, 16u, &sample),
                       "s3_shape_target_rect_object_canvas_sample")) {
            return 1;
        }
        if (sample != bg) {
            fprintf(stderr,
                    "lifecycle_test: expected rect object target to avoid direct raster write background=%u got=%u\n",
                    (unsigned)bg,
                    (unsigned)sample);
            return 1;
        }
        drawing_program_object_store_reset(&ctx.object_store);
        drawing_program_object_selection_reset(&ctx.object_selection);
        ctx.editor.symmetry_horizontal = 0u;
        ctx.editor.symmetry_vertical = 0u;
        drawing_program_canvas_reflection_sync_active_surface_from_editor(&ctx);
        if (!expect_ok(drawing_program_app_shape_commit_samples(&ctx,
                                                                DRAWING_PROGRAM_TOOL_CIRCLE,
                                                                32u,
                                                                32u,
                                                                36u,
                                                                38u),
                       "s3_shape_target_circle_object_commit")) {
            return 1;
        }
        if (ctx.object_store.object_count != 1u ||
            ctx.object_selection.count != 1u ||
            ctx.object_selection.active_object_id == 0u) {
            fprintf(stderr,
                    "lifecycle_test: expected circle object target to create one selected object count=%u sel=%u active=%u\n",
                    (unsigned)ctx.object_store.object_count,
                    (unsigned)ctx.object_selection.count,
                    (unsigned)ctx.object_selection.active_object_id);
            return 1;
        }
        {
            const DrawingProgramObjectRecord *object =
                drawing_program_object_store_get_by_id(&ctx.object_store, ctx.object_selection.active_object_id);
            if (!object ||
                object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_ELLIPSE ||
                object->width == 0u ||
                object->height == 0u) {
                fprintf(stderr,
                        "lifecycle_test: expected circle object target ellipse geometry type=%u size=%ux%u\n",
                        object ? (unsigned)object->type : 0u,
                        object ? (unsigned)object->width : 0u,
                        object ? (unsigned)object->height : 0u);
                return 1;
            }
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 32u, 32u, &sample),
                       "s3_shape_target_circle_object_canvas_sample")) {
            return 1;
        }
        if (sample != bg) {
            fprintf(stderr,
                    "lifecycle_test: expected circle object target to avoid direct raster write background=%u got=%u\n",
                    (unsigned)bg,
                    (unsigned)sample);
            return 1;
        }
        ctx.ui.tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
    }

    if (!expect_ok(drawing_program_app_shutdown(&ctx), "shutdown")) {
        return 1;
    }

#undef ctx
    return 0;
}
