#include "drawing_program/drawing_program_visual_shape_ops.h"

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
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

uint8_t drawing_program_visual_clamp_setting_u8(uint8_t value, uint8_t min_v, uint8_t max_v) {
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

uint8_t drawing_program_visual_sample_value_for_tool(const DrawingProgramAppContext *ctx, DrawingProgramToolKind tool) {
    uint8_t color_index = drawing_program_color_default_index();
    if (ctx) {
        color_index = drawing_program_color_index_clamp(ctx->ui_active_color_index);
    }
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
        default:
            return drawing_program_color_value_from_index(color_index);
    }
}

uint32_t drawing_program_visual_tool_brush_radius_samples(const DrawingProgramAppContext *ctx, DrawingProgramToolKind tool) {
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_BRUSH:
            return (uint32_t)drawing_program_visual_clamp_setting_u8(ctx ? ctx->ui_tool_brush_size : 2u, 1u, 16u);
        case DRAWING_PROGRAM_TOOL_ERASER:
            return (uint32_t)drawing_program_visual_clamp_setting_u8(ctx ? ctx->ui_tool_eraser_size : 4u, 1u, 16u);
        default:
            return 0u;
    }
}

uint32_t drawing_program_visual_tool_brush_spacing_samples(const DrawingProgramAppContext *ctx,
                                                           DrawingProgramToolKind tool,
                                                           uint32_t radius) {
    uint32_t spacing = 1u;
    if (tool == DRAWING_PROGRAM_TOOL_BRUSH && ctx) {
        spacing = (uint32_t)drawing_program_visual_clamp_setting_u8(ctx->ui_tool_brush_spacing, 1u, 16u);
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
        return drawing_program_visual_clamp_setting_u8(ctx->ui_tool_brush_hardness, 1u, 100u);
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
    return drawing_program_visual_clamp_setting_u8(ctx ? ctx->ui_tool_shape_mode : 0u, 0u, 2u);
}

uint32_t drawing_program_visual_tool_shape_stroke_width(const DrawingProgramAppContext *ctx) {
    return (uint32_t)drawing_program_visual_clamp_setting_u8(ctx ? ctx->ui_tool_shape_stroke_width : 1u, 1u, 16u);
}

uint8_t drawing_program_visual_tool_fill_tolerance_setting(const DrawingProgramAppContext *ctx) {
    return drawing_program_visual_clamp_setting_u8(ctx ? ctx->ui_tool_fill_tolerance : 0u,
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

int drawing_program_visual_fill_sample_matches_tolerance(uint8_t sample, uint8_t target, uint8_t tolerance_setting) {
    int diff = (int)sample - (int)target;
    uint8_t threshold = drawing_program_visual_fill_tolerance_sample_delta(tolerance_setting);
    if (diff < 0) {
        diff = -diff;
    }
    return (diff <= (int)threshold) ? 1 : 0;
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

uint8_t drawing_program_visual_color_index_for_sample(uint8_t sample) {
    uint8_t best_index = drawing_program_color_default_index();
    int best_dist = 256;
    uint8_t i;
    for (i = 0u; i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++i) {
        int palette_value = (int)drawing_program_color_value_from_index(i);
        int d = palette_value - (int)sample;
        if (d < 0) {
            d = -d;
        }
        if (d < best_dist) {
            best_dist = d;
            best_index = i;
        }
    }
    return best_index;
}

uint8_t drawing_program_visual_seeded_background_sample_for_coord(const DrawingProgramDocument *document,
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
    uint8_t value;
    uint8_t mode;
    uint32_t stroke_width;
    uint32_t active_layer_id = 0u;
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
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_LINE:
            return drawing_program_visual_apply_canvas_line_between_samples_on_layer(ctx,
                                                                                     active_layer_id,
                                                                                     start_x,
                                                                                     start_y,
                                                                                     end_x,
                                                                                     end_y,
                                                                                     value,
                                                                                     stroke_width,
                                                                                     100u);
        case DRAWING_PROGRAM_TOOL_RECT: {
            CoreResult result = core_result_ok();
            if (drawing_program_visual_shape_mode_includes_fill(tool, mode)) {
                result = drawing_program_visual_apply_canvas_rect_fill_between_samples_on_layer(ctx,
                                                                                                active_layer_id,
                                                                                                start_x,
                                                                                                start_y,
                                                                                                end_x,
                                                                                                end_y,
                                                                                                value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
            if (drawing_program_visual_shape_mode_includes_outline(tool, mode)) {
                result = drawing_program_visual_apply_canvas_rect_outline_between_samples_on_layer(ctx,
                                                                                                   active_layer_id,
                                                                                                   start_x,
                                                                                                   start_y,
                                                                                                   end_x,
                                                                                                   end_y,
                                                                                                   value,
                                                                                                   stroke_width);
            }
            return result;
        }
        case DRAWING_PROGRAM_TOOL_CIRCLE: {
            CoreResult result = core_result_ok();
            if (drawing_program_visual_shape_mode_includes_fill(tool, mode)) {
                result = drawing_program_visual_apply_canvas_circle_fill_between_samples_on_layer(ctx,
                                                                                                  active_layer_id,
                                                                                                  start_x,
                                                                                                  start_y,
                                                                                                  end_x,
                                                                                                  end_y,
                                                                                                  value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
            if (drawing_program_visual_shape_mode_includes_outline(tool, mode)) {
                result = drawing_program_visual_apply_canvas_circle_outline_between_samples_on_layer(ctx,
                                                                                                     active_layer_id,
                                                                                                     start_x,
                                                                                                     start_y,
                                                                                                     end_x,
                                                                                                     end_y,
                                                                                                     value,
                                                                                                     stroke_width);
            }
            return result;
        }
        default:
            return core_result_ok();
    }
}
