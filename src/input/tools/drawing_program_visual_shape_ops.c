#include "drawing_program/drawing_program_visual_shape_ops.h"

#include <string.h>

#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_ui_color_state.h"
#include "drawing_program/drawing_program_visual_canvas_stroke_ops.h"

static int active_layer_query(const DrawingProgramAppContext *ctx,
                              uint32_t *out_layer_id,
                              uint32_t *out_index,
                              uint8_t *out_visible,
                              uint8_t *out_locked) {
    CoreResult result = drawing_program_runtime_orchestration_resolve_active_layer(ctx,
                                                                                    out_layer_id,
                                                                                    out_index,
                                                                                    out_visible,
                                                                                    out_locked);
    return (result.code == CORE_OK) ? 1 : 0;
}

static int active_layer_allows_edits_visual(const DrawingProgramAppContext *ctx) {
    uint8_t visible = 0u;
    uint8_t locked = 0u;
    if (!active_layer_query(ctx, 0, 0, &visible, &locked)) {
        return 0;
    }
    return (visible && !locked) ? 1 : 0;
}

static uint8_t shape_target_mode(const DrawingProgramAppContext *ctx) {
    return drawing_program_visual_clamp_setting_u8(
        ctx ? ctx->ui.tool_shape_target_mode : (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL,
        (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL,
        (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_OBJECT);
}

static void rect_bounds_from_samples(int32_t start_x,
                                     int32_t start_y,
                                     int32_t end_x,
                                     int32_t end_y,
                                     int32_t *out_origin_x,
                                     int32_t *out_origin_y,
                                     uint32_t *out_width,
                                     uint32_t *out_height) {
    int32_t min_x = (start_x < end_x) ? start_x : end_x;
    int32_t min_y = (start_y < end_y) ? start_y : end_y;
    int32_t max_x = (start_x > end_x) ? start_x : end_x;
    int32_t max_y = (start_y > end_y) ? start_y : end_y;
    if (out_origin_x) {
        *out_origin_x = min_x;
    }
    if (out_origin_y) {
        *out_origin_y = min_y;
    }
    if (out_width) {
        *out_width = (uint32_t)((max_x - min_x) + 1);
    }
    if (out_height) {
        *out_height = (uint32_t)((max_y - min_y) + 1);
    }
}

static void circle_bounds_from_samples(int32_t center_x,
                                       int32_t center_y,
                                       int32_t edge_x,
                                       int32_t edge_y,
                                       int32_t *out_origin_x,
                                       int32_t *out_origin_y,
                                       uint32_t *out_width,
                                       uint32_t *out_height) {
    int32_t cx = center_x;
    int32_t cy = center_y;
    int32_t rx = (int32_t)edge_x - cx;
    int32_t ry = (int32_t)edge_y - cy;
    int32_t r = (rx < 0) ? -rx : rx;
    int32_t ay = (ry < 0) ? -ry : ry;
    if (ay > r) {
        r = ay;
    }
    if (r < 0) {
        r = 0;
    }
    if (out_origin_x) {
        *out_origin_x = cx - r;
    }
    if (out_origin_y) {
        *out_origin_y = cy - r;
    }
    if (out_width) {
        *out_width = (uint32_t)(r * 2 + 1);
    }
    if (out_height) {
        *out_height = (uint32_t)(r * 2 + 1);
    }
}

static CoreResult add_shape_object(DrawingProgramAppContext *ctx,
                                   DrawingProgramToolKind tool,
                                   uint32_t active_layer_id,
                                   int32_t start_x,
                                   int32_t start_y,
                                   int32_t end_x,
                                   int32_t end_y,
                                   uint8_t mode,
                                   uint32_t stroke_width,
                                   uint32_t *out_object_id) {
    DrawingProgramObjectRecord seed;
    DrawingProgramRasterSample color_value;
    CoreResult add_result;
    memset(&seed, 0, sizeof(seed));
    if (!ctx || active_layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid object-shape commit request" };
    }
    color_value = drawing_program_ui_color_active_paint_sample_value(ctx);
    seed.layer_id = active_layer_id;
    seed.visible = 1u;
    seed.locked = 0u;
    seed.stroke_color_value = color_value;
    seed.fill_color_value = color_value;
    seed.stroke_width = drawing_program_visual_clamp_setting_u8((uint8_t)stroke_width, 1u, 16u);
    seed.style_mode = drawing_program_visual_clamp_setting_u8(mode, 0u, 2u);
    if (tool == DRAWING_PROGRAM_TOOL_RECT) {
        seed.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT;
        rect_bounds_from_samples(
            start_x, start_y, end_x, end_y, &seed.origin_x, &seed.origin_y, &seed.width, &seed.height);
    } else if (tool == DRAWING_PROGRAM_TOOL_CIRCLE) {
        seed.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_ELLIPSE;
        circle_bounds_from_samples(
            start_x, start_y, end_x, end_y, &seed.origin_x, &seed.origin_y, &seed.width, &seed.height);
    } else {
        return core_result_ok();
    }
    if (seed.width == 0u || seed.height == 0u) {
        return core_result_ok();
    }
    add_result = drawing_program_object_store_add(&ctx->object_store, &seed, out_object_id);
    if (add_result.code != CORE_OK || !out_object_id || *out_object_id == 0u) {
        return (add_result.code == CORE_OK)
                   ? (CoreResult){ CORE_ERR_FORMAT, "failed to allocate shape object id" }
                   : add_result;
    }
    return core_result_ok();
}

uint8_t drawing_program_visual_clamp_setting_u8(uint8_t value, uint8_t min_v, uint8_t max_v) {
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

DrawingProgramRasterSample drawing_program_visual_sample_value_for_tool(
    const DrawingProgramAppContext *ctx,
    DrawingProgramToolKind tool) {
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_ERASER:
            return drawing_program_color_eraser_value();
        case DRAWING_PROGRAM_TOOL_BRUSH:
        case DRAWING_PROGRAM_TOOL_FILL:
        case DRAWING_PROGRAM_TOOL_LINE:
        case DRAWING_PROGRAM_TOOL_RECT:
        case DRAWING_PROGRAM_TOOL_CIRCLE:
        case DRAWING_PROGRAM_TOOL_SELECT:
        case DRAWING_PROGRAM_TOOL_MOVE:
        case DRAWING_PROGRAM_TOOL_PICKER:
        case DRAWING_PROGRAM_TOOL_PATH:
        default:
            return drawing_program_ui_color_active_paint_sample_value(ctx);
    }
}

uint32_t drawing_program_visual_tool_brush_radius_samples(const DrawingProgramAppContext *ctx, DrawingProgramToolKind tool) {
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_BRUSH:
            return (uint32_t)drawing_program_visual_clamp_setting_u8(ctx ? ctx->ui.tool_brush_size : 2u, 1u, 16u);
        case DRAWING_PROGRAM_TOOL_ERASER:
            return (uint32_t)drawing_program_visual_clamp_setting_u8(ctx ? ctx->ui.tool_eraser_size : 4u, 1u, 16u);
        default:
            return 0u;
    }
}

uint32_t drawing_program_visual_tool_brush_spacing_samples(const DrawingProgramAppContext *ctx,
                                                           DrawingProgramToolKind tool,
                                                           uint32_t radius) {
    uint32_t spacing = 1u;
    if (tool == DRAWING_PROGRAM_TOOL_BRUSH && ctx) {
        spacing = (uint32_t)drawing_program_visual_clamp_setting_u8(ctx->ui.tool_brush_spacing, 1u, 16u);
    } else if (radius > 0u) {
        spacing = (radius / 2u) + 1u;
    }
    if (spacing < 1u) {
        spacing = 1u;
    }
    return spacing;
}

uint8_t drawing_program_visual_tool_brush_hardness_percent(const DrawingProgramAppContext *ctx,
                                                           DrawingProgramToolKind tool) {
    if (tool == DRAWING_PROGRAM_TOOL_BRUSH && ctx) {
        return drawing_program_visual_clamp_setting_u8(ctx->ui.tool_brush_hardness, 1u, 100u);
    }
    return 100u;
}

int drawing_program_visual_tool_uses_direct_sample_stroke(DrawingProgramToolKind tool) {
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_BRUSH:
        case DRAWING_PROGRAM_TOOL_ERASER:
            return 1;
        default:
            return 0;
    }
}

int drawing_program_visual_tool_uses_shape_commit(DrawingProgramToolKind tool) {
    return (tool == DRAWING_PROGRAM_TOOL_LINE ||
            tool == DRAWING_PROGRAM_TOOL_RECT ||
            tool == DRAWING_PROGRAM_TOOL_CIRCLE)
               ? 1
               : 0;
}

uint8_t drawing_program_visual_tool_shape_mode(const DrawingProgramAppContext *ctx) {
    return drawing_program_visual_clamp_setting_u8(ctx ? ctx->ui.tool_shape_mode : 0u, 0u, 2u);
}

uint32_t drawing_program_visual_tool_shape_stroke_width(const DrawingProgramAppContext *ctx) {
    return (uint32_t)drawing_program_visual_clamp_setting_u8(ctx ? ctx->ui.tool_shape_stroke_width : 1u, 1u, 16u);
}

uint8_t drawing_program_visual_tool_fill_tolerance_setting(const DrawingProgramAppContext *ctx) {
    return drawing_program_visual_clamp_setting_u8(ctx ? ctx->ui.tool_fill_tolerance : 0u,
                                                   0u,
                                                   (uint8_t)DRAWING_PROGRAM_UI_FILL_TOLERANCE_MAX);
}

uint8_t drawing_program_visual_fill_tolerance_sample_delta(uint8_t tolerance_setting) {
    uint32_t delta = (uint32_t)tolerance_setting * (uint32_t)DRAWING_PROGRAM_UI_FILL_TOLERANCE_SAMPLE_SCALE;
    if (delta > 255u) {
        delta = 255u;
    }
    return (uint8_t)delta;
}

int drawing_program_visual_fill_sample_matches_tolerance(DrawingProgramRasterSample sample,
                                                         DrawingProgramRasterSample target,
                                                         uint8_t tolerance_setting) {
    uint8_t threshold = drawing_program_visual_fill_tolerance_sample_delta(tolerance_setting);
    return (drawing_program_color_sample_distance(sample, target) <= threshold) ? 1 : 0;
}

const char *drawing_program_visual_shape_mode_name(uint8_t mode) {
    switch (drawing_program_visual_clamp_setting_u8(mode, 0u, 2u)) {
        case 0u: return "OUTLINE";
        case 1u: return "FILL";
        case 2u: return "FILL+OUTLINE";
        default: return "OUTLINE";
    }
}

int drawing_program_visual_shape_mode_includes_fill(DrawingProgramToolKind tool, uint8_t mode) {
    if (tool == DRAWING_PROGRAM_TOOL_LINE) {
        return 0;
    }
    return (drawing_program_visual_clamp_setting_u8(mode, 0u, 2u) == 1u ||
            drawing_program_visual_clamp_setting_u8(mode, 0u, 2u) == 2u)
               ? 1
               : 0;
}

int drawing_program_visual_shape_mode_includes_outline(DrawingProgramToolKind tool, uint8_t mode) {
    if (tool == DRAWING_PROGRAM_TOOL_LINE) {
        return 1;
    }
    return (drawing_program_visual_clamp_setting_u8(mode, 0u, 2u) == 1u) ? 0 : 1;
}

DrawingProgramRasterSample drawing_program_visual_seeded_background_sample_for_coord(
    const DrawingProgramDocument *document,
    uint32_t x,
    uint32_t y) {
    (void)document;
    (void)x;
    (void)y;
    return drawing_program_color_eraser_value();
}

CoreResult drawing_program_visual_apply_canvas_shape_commit(DrawingProgramAppContext *ctx,
                                                            DrawingProgramToolKind tool,
                                                            uint32_t start_x,
                                                            uint32_t start_y,
                                                            uint32_t end_x,
                                                            uint32_t end_y) {
    DrawingProgramCanvasReflectionSegment segments[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY];
    DrawingProgramVisualRasterHistoryBatch history_batch;
    DrawingProgramRasterSample value;
    uint8_t mode;
    uint32_t stroke_width;
    uint32_t active_layer_id = 0u;
    uint32_t segment_count = 0u;
    uint32_t segment_index = 0u;
    if (!ctx) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "null app context for shape commit" };
    }
    if (!active_layer_allows_edits_visual(ctx)) {
        return core_result_ok();
    }
    if (!active_layer_query(ctx, &active_layer_id, 0, 0, 0) || active_layer_id == 0u) {
        return core_result_ok();
    }
    value = drawing_program_visual_sample_value_for_tool(ctx, tool);
    mode = drawing_program_visual_tool_shape_mode(ctx);
    stroke_width = drawing_program_visual_tool_shape_stroke_width(ctx);
    segment_count = drawing_program_canvas_reflection_collect_segments(
        ctx, (int32_t)start_x, (int32_t)start_y, (int32_t)end_x, (int32_t)end_y, segments);
    if (segment_count == 0u) {
        segment_count = 1u;
        segments[0].start_x = (int32_t)start_x;
        segments[0].start_y = (int32_t)start_y;
        segments[0].end_x = (int32_t)end_x;
        segments[0].end_y = (int32_t)end_y;
    }
    if (shape_target_mode(ctx) == (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_OBJECT &&
        (tool == DRAWING_PROGRAM_TOOL_RECT || tool == DRAWING_PROGRAM_TOOL_CIRCLE)) {
        drawing_program_selection_reset(&ctx->selection);
        drawing_program_object_selection_reset(&ctx->object_selection);
        for (segment_index = 0u; segment_index < segment_count; ++segment_index) {
            uint32_t object_id = 0u;
            CoreResult object_result = add_shape_object(ctx,
                                                        tool,
                                                        active_layer_id,
                                                        segments[segment_index].start_x,
                                                        segments[segment_index].start_y,
                                                        segments[segment_index].end_x,
                                                        segments[segment_index].end_y,
                                                        mode,
                                                        stroke_width,
                                                        &object_id);
            if (object_result.code != CORE_OK) {
                return object_result;
            }
            if (object_id != 0u) {
                (void)drawing_program_object_selection_add(&ctx->object_selection, object_id);
            }
        }
        return core_result_ok();
    }
    drawing_program_visual_raster_history_batch_init(&history_batch, active_layer_id);
    for (segment_index = 0u; segment_index < segment_count; ++segment_index) {
        CoreResult result = core_result_ok();
        switch (tool) {
            case DRAWING_PROGRAM_TOOL_LINE:
                result = drawing_program_visual_apply_canvas_line_between_samples_with_history_batch(
                    ctx,
                    &history_batch,
                    active_layer_id,
                    segments[segment_index].start_x,
                    segments[segment_index].start_y,
                    segments[segment_index].end_x,
                    segments[segment_index].end_y,
                    value,
                    stroke_width,
                    100u);
                break;
            case DRAWING_PROGRAM_TOOL_RECT:
                if (drawing_program_visual_shape_mode_includes_fill(tool, mode)) {
                    result = drawing_program_visual_apply_canvas_rect_fill_between_samples_with_history_batch(
                        ctx,
                        &history_batch,
                        active_layer_id,
                        segments[segment_index].start_x,
                        segments[segment_index].start_y,
                        segments[segment_index].end_x,
                        segments[segment_index].end_y,
                        value);
                    if (result.code != CORE_OK) {
                        (void)drawing_program_visual_raster_history_batch_finish(ctx, &history_batch);
                        return result;
                    }
                }
                if (drawing_program_visual_shape_mode_includes_outline(tool, mode)) {
                    result = drawing_program_visual_apply_canvas_rect_outline_between_samples_with_history_batch(
                        ctx,
                        &history_batch,
                        active_layer_id,
                        segments[segment_index].start_x,
                        segments[segment_index].start_y,
                        segments[segment_index].end_x,
                        segments[segment_index].end_y,
                        value,
                        stroke_width);
                }
                break;
            case DRAWING_PROGRAM_TOOL_CIRCLE:
                if (drawing_program_visual_shape_mode_includes_fill(tool, mode)) {
                    result = drawing_program_visual_apply_canvas_circle_fill_between_samples_with_history_batch(
                        ctx,
                        &history_batch,
                        active_layer_id,
                        segments[segment_index].start_x,
                        segments[segment_index].start_y,
                        segments[segment_index].end_x,
                        segments[segment_index].end_y,
                        value);
                    if (result.code != CORE_OK) {
                        (void)drawing_program_visual_raster_history_batch_finish(ctx, &history_batch);
                        return result;
                    }
                }
                if (drawing_program_visual_shape_mode_includes_outline(tool, mode)) {
                    result = drawing_program_visual_apply_canvas_circle_outline_between_samples_with_history_batch(
                        ctx,
                        &history_batch,
                        active_layer_id,
                        segments[segment_index].start_x,
                        segments[segment_index].start_y,
                        segments[segment_index].end_x,
                        segments[segment_index].end_y,
                        value,
                        stroke_width);
                }
                break;
            default:
                break;
        }
        if (result.code != CORE_OK) {
            (void)drawing_program_visual_raster_history_batch_finish(ctx, &history_batch);
            return result;
        }
    }
    return drawing_program_visual_raster_history_batch_finish(ctx, &history_batch);
}
