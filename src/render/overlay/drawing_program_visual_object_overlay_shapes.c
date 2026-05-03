#include "drawing_program_visual_object_overlay_internal.h"

void drawing_program_visual_object_overlay_draw_rect(
    SDL_Renderer *renderer,
    const DrawingProgramAppContext *ctx,
    const VisualCanvasSheetMetrics *metrics,
    const DrawingProgramObjectRecord *object,
    int32_t origin_x,
    int32_t origin_y,
    uint8_t alpha) {
    uint32_t x;
    uint32_t y;
    uint32_t stroke_width = object->stroke_width == 0u ? 1u : (uint32_t)object->stroke_width;
    int has_fill = drawing_program_visual_object_style_includes_fill(object->style_mode);
    int has_outline = drawing_program_visual_object_style_includes_outline(object->style_mode);
    SDL_Color stroke_color;
    SDL_Color fill_color;
    if (!renderer || !ctx || !metrics || !object || object->width == 0u || object->height == 0u) {
        return;
    }
    stroke_color = drawing_program_visual_object_overlay_color_from_sample(object->stroke_color_value);
    fill_color = drawing_program_visual_object_overlay_color_from_sample(object->fill_color_value);
    for (y = 0u; y < object->height; ++y) {
        for (x = 0u; x < object->width; ++x) {
            int32_t sample_x = origin_x + (int32_t)x;
            int32_t sample_y = origin_y + (int32_t)y;
            int on_outline = 0;
            if (sample_x < 0 || sample_y < 0 ||
                sample_x >= (int32_t)ctx->document.raster_width ||
                sample_y >= (int32_t)ctx->document.raster_height) {
                continue;
            }
            if (has_outline) {
                on_outline = (x < stroke_width || y < stroke_width || x >= (object->width - stroke_width) ||
                              y >= (object->height - stroke_width))
                                 ? 1
                                 : 0;
            }
            if (on_outline) {
                (void)drawing_program_visual_object_overlay_draw_sample_cell(
                    renderer, metrics, sample_x, sample_y, stroke_color, alpha);
            } else if (has_fill) {
                (void)drawing_program_visual_object_overlay_draw_sample_cell(
                    renderer, metrics, sample_x, sample_y, fill_color, alpha);
            }
        }
    }
}

void drawing_program_visual_object_overlay_draw_ellipse(
    SDL_Renderer *renderer,
    const DrawingProgramAppContext *ctx,
    const VisualCanvasSheetMetrics *metrics,
    const DrawingProgramObjectRecord *object,
    int32_t origin_x,
    int32_t origin_y,
    uint8_t alpha) {
    int32_t min_x;
    int32_t min_y;
    int32_t max_x;
    int32_t max_y;
    int32_t x;
    int32_t y;
    uint32_t stroke_width = object->stroke_width == 0u ? 1u : (uint32_t)object->stroke_width;
    int has_fill = drawing_program_visual_object_style_includes_fill(object->style_mode);
    int has_outline = drawing_program_visual_object_style_includes_outline(object->style_mode);
    SDL_Color stroke_color;
    SDL_Color fill_color;
    double rx;
    double ry;
    double inner_rx;
    double inner_ry;
    double cx;
    double cy;
    if (!renderer || !ctx || !metrics || !object || object->width == 0u || object->height == 0u) {
        return;
    }
    stroke_color = drawing_program_visual_object_overlay_color_from_sample(object->stroke_color_value);
    fill_color = drawing_program_visual_object_overlay_color_from_sample(object->fill_color_value);
    rx = ((double)object->width) * 0.5;
    ry = ((double)object->height) * 0.5;
    if (rx <= 0.0 || ry <= 0.0) {
        return;
    }
    inner_rx = rx - (double)stroke_width;
    inner_ry = ry - (double)stroke_width;
    if (inner_rx < 0.0) {
        inner_rx = 0.0;
    }
    if (inner_ry < 0.0) {
        inner_ry = 0.0;
    }
    cx = (double)origin_x + rx;
    cy = (double)origin_y + ry;
    min_x = origin_x;
    min_y = origin_y;
    max_x = origin_x + (int32_t)object->width - 1;
    max_y = origin_y + (int32_t)object->height - 1;
    for (y = min_y; y <= max_y; ++y) {
        for (x = min_x; x <= max_x; ++x) {
            double dx;
            double dy;
            double outer_norm;
            int inside_outer;
            int inside_inner = 0;
            int on_outline = 0;
            if (x < 0 || y < 0 || x >= (int32_t)ctx->document.raster_width || y >= (int32_t)ctx->document.raster_height) {
                continue;
            }
            dx = ((double)x + 0.5) - cx;
            dy = ((double)y + 0.5) - cy;
            outer_norm = ((dx * dx) / (rx * rx)) + ((dy * dy) / (ry * ry));
            inside_outer = (outer_norm <= 1.0) ? 1 : 0;
            if (!inside_outer) {
                continue;
            }
            if (has_outline && inner_rx > 0.0 && inner_ry > 0.0) {
                double inner_norm = ((dx * dx) / (inner_rx * inner_rx)) + ((dy * dy) / (inner_ry * inner_ry));
                inside_inner = (inner_norm <= 1.0) ? 1 : 0;
            }
            on_outline = has_outline && !inside_inner;
            if (on_outline) {
                (void)drawing_program_visual_object_overlay_draw_sample_cell(renderer, metrics, x, y, stroke_color, alpha);
            } else if (has_fill) {
                (void)drawing_program_visual_object_overlay_draw_sample_cell(renderer, metrics, x, y, fill_color, alpha);
            }
        }
    }
}
