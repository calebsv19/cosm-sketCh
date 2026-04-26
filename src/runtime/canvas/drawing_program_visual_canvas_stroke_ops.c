#include "drawing_program/drawing_program_visual_canvas_stroke_ops.h"

#include <math.h>

#include "drawing_program/drawing_program_color_model.h"

static uint8_t clamp_percent_u8_local(uint8_t value) {
    return (value > 100u) ? 100u : value;
}

static uint8_t clamp_setting_u8_local(uint8_t value, uint8_t min_v, uint8_t max_v) {
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

static CoreResult layer_sample_read_visual_local(const DrawingProgramAppContext *ctx,
                                                 uint32_t layer_id,
                                                 uint32_t sample_x,
                                                 uint32_t sample_y,
                                                 DrawingProgramRasterSample *out_value) {
    CoreResult result;
    if (!ctx || !out_value || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid layer sample read request" };
    }
    result = drawing_program_layer_raster_store_raster_sample_read(&ctx->layer_rasters,
                                                            layer_id,
                                                            sample_x,
                                                            sample_y,
                                                            out_value);
    if (result.code == CORE_OK) {
        return core_result_ok();
    }
    return drawing_program_document_raster_sample_read(&ctx->document, sample_x, sample_y, out_value);
}

static CoreResult apply_sample_if_changed_on_layer_local(DrawingProgramAppContext *ctx,
                                                         uint32_t layer_id,
                                                         uint32_t sample_x,
                                                         uint32_t sample_y,
                                                         DrawingProgramRasterSample value) {
    return drawing_program_history_apply_set_sample_value(&ctx->history,
                                                          &ctx->document,
                                                          &ctx->layer_rasters,
                                                          layer_id,
                                                          sample_x,
                                                          sample_y,
                                                          value);
}

static DrawingProgramRasterSample blend_sample_value_u8_local(DrawingProgramRasterSample dst,
                                                              DrawingProgramRasterSample src,
                                                              uint8_t opacity_percent) {
    return drawing_program_color_blend_samples(dst, src, clamp_percent_u8_local(opacity_percent));
}

CoreResult drawing_program_visual_apply_canvas_stamp_square_on_layer(DrawingProgramAppContext *ctx,
                                                                     uint32_t layer_id,
                                                                     int32_t sample_x,
                                                                     int32_t sample_y,
                                                                     DrawingProgramRasterSample value,
                                                                     uint32_t stroke_width,
                                                                     uint8_t hardness_percent) {
    int32_t start_offset;
    int32_t end_offset;
    int32_t y;
    int32_t x;
    float radius_limit;
    float inner_radius;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid stamp request" };
    }
    if (stroke_width < 1u) {
        stroke_width = 1u;
    }
    hardness_percent = clamp_setting_u8_local(hardness_percent, 1u, 100u);
    start_offset = -((int32_t)stroke_width / 2);
    end_offset = start_offset + (int32_t)stroke_width - 1;
    if (hardness_percent >= 100u) {
        for (y = start_offset; y <= end_offset; ++y) {
            for (x = start_offset; x <= end_offset; ++x) {
                int32_t tx = sample_x + x;
                int32_t ty = sample_y + y;
                CoreResult result;
                if (tx < 0 || ty < 0 ||
                    (uint32_t)tx >= ctx->document.raster_width ||
                    (uint32_t)ty >= ctx->document.raster_height) {
                    continue;
                }
                result = apply_sample_if_changed_on_layer_local(ctx,
                                                                layer_id,
                                                                (uint32_t)tx,
                                                                (uint32_t)ty,
                                                                value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
        return core_result_ok();
    }
    radius_limit = ((float)stroke_width * 0.5f);
    if (radius_limit < 0.5f) {
        radius_limit = 0.5f;
    }
    inner_radius = radius_limit * ((float)hardness_percent / 100.0f);
    for (y = start_offset; y <= end_offset; ++y) {
        for (x = start_offset; x <= end_offset; ++x) {
            int32_t tx = sample_x + x;
            int32_t ty = sample_y + y;
            float dx = (float)x + 0.5f;
            float dy = (float)y + 0.5f;
            float d = sqrtf((dx * dx) + (dy * dy));
            uint8_t opacity = 100u;
            if (tx < 0 || ty < 0 ||
                (uint32_t)tx >= ctx->document.raster_width ||
                (uint32_t)ty >= ctx->document.raster_height) {
                continue;
            }
            if (d > radius_limit) {
                continue;
            }
            if (hardness_percent < 100u && d > inner_radius && radius_limit > inner_radius) {
                float edge_t = (radius_limit - d) / (radius_limit - inner_radius);
                int edge_alpha = (int)(edge_t * 100.0f);
                if (edge_alpha < 1) {
                    edge_alpha = 1;
                }
                if (edge_alpha > 100) {
                    edge_alpha = 100;
                }
                opacity = (uint8_t)edge_alpha;
            }
            {
                DrawingProgramRasterSample out_value = value;
                if (opacity < 100u) {
                    DrawingProgramRasterSample current = drawing_program_color_eraser_value();
                    (void)layer_sample_read_visual_local(ctx, layer_id, (uint32_t)tx, (uint32_t)ty, &current);
                    out_value = blend_sample_value_u8_local(current, value, opacity);
                }
                {
                    CoreResult result = apply_sample_if_changed_on_layer_local(ctx,
                                                                               layer_id,
                                                                               (uint32_t)tx,
                                                                               (uint32_t)ty,
                                                                               out_value);
                    if (result.code != CORE_OK) {
                        return result;
                    }
                }
            }
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_visual_apply_canvas_line_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                              uint32_t layer_id,
                                                                              uint32_t x0,
                                                                              uint32_t y0,
                                                                              uint32_t x1,
                                                                              uint32_t y1,
                                                                              DrawingProgramRasterSample value,
                                                                              uint32_t stroke_width,
                                                                              uint8_t hardness_percent) {
    int32_t x = (int32_t)x0;
    int32_t y = (int32_t)y0;
    int32_t tx = (int32_t)x1;
    int32_t ty = (int32_t)y1;
    int32_t dx = (tx > x) ? (tx - x) : (x - tx);
    int32_t sx = (x < tx) ? 1 : -1;
    int32_t dy = -((ty > y) ? (ty - y) : (y - ty));
    int32_t sy = (y < ty) ? 1 : -1;
    int32_t err = dx + dy;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid line request" };
    }
    if (stroke_width < 1u) {
        stroke_width = 1u;
    }
    while (1) {
        if (x >= 0 && y >= 0 &&
            (uint32_t)x < ctx->document.raster_width &&
            (uint32_t)y < ctx->document.raster_height) {
            CoreResult stamp_result = drawing_program_visual_apply_canvas_stamp_square_on_layer(ctx,
                                                                                                 layer_id,
                                                                                                 x,
                                                                                                 y,
                                                                                                 value,
                                                                                                 stroke_width,
                                                                                                 hardness_percent);
            if (stamp_result.code != CORE_OK) {
                return stamp_result;
            }
        }
        if (x == tx && y == ty) {
            break;
        }
        {
            int32_t e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y += sy;
            }
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_visual_apply_canvas_rect_fill_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                                   uint32_t layer_id,
                                                                                   uint32_t x0,
                                                                                   uint32_t y0,
                                                                                   uint32_t x1,
                                                                                   uint32_t y1,
                                                                                   DrawingProgramRasterSample value) {
    uint32_t min_x = (x0 < x1) ? x0 : x1;
    uint32_t min_y = (y0 < y1) ? y0 : y1;
    uint32_t max_x = (x0 > x1) ? x0 : x1;
    uint32_t max_y = (y0 > y1) ? y0 : y1;
    uint32_t y;
    uint32_t x;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid rect-fill request" };
    }
    for (y = min_y; y <= max_y; ++y) {
        for (x = min_x; x <= max_x; ++x) {
            CoreResult result = apply_sample_if_changed_on_layer_local(ctx, layer_id, x, y, value);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_visual_apply_canvas_rect_outline_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                                      uint32_t layer_id,
                                                                                      uint32_t x0,
                                                                                      uint32_t y0,
                                                                                      uint32_t x1,
                                                                                      uint32_t y1,
                                                                                      DrawingProgramRasterSample value,
                                                                                      uint32_t stroke_width) {
    uint32_t min_x = (x0 < x1) ? x0 : x1;
    uint32_t min_y = (y0 < y1) ? y0 : y1;
    uint32_t max_x = (x0 > x1) ? x0 : x1;
    uint32_t max_y = (y0 > y1) ? y0 : y1;
    uint32_t y;
    uint32_t x;
    uint32_t rect_width;
    uint32_t rect_height;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid rect-outline request" };
    }
    if (stroke_width < 1u) {
        stroke_width = 1u;
    }
    rect_width = (max_x - min_x) + 1u;
    rect_height = (max_y - min_y) + 1u;
    if (stroke_width >= rect_width || stroke_width >= rect_height) {
        return drawing_program_visual_apply_canvas_rect_fill_between_samples_on_layer(
            ctx, layer_id, min_x, min_y, max_x, max_y, value);
    }
    for (y = min_y; y <= max_y; ++y) {
        for (x = min_x; x <= max_x; ++x) {
            uint32_t left_dist = x - min_x;
            uint32_t right_dist = max_x - x;
            uint32_t top_dist = y - min_y;
            uint32_t bottom_dist = max_y - y;
            if (left_dist < stroke_width ||
                right_dist < stroke_width ||
                top_dist < stroke_width ||
                bottom_dist < stroke_width) {
                CoreResult result = apply_sample_if_changed_on_layer_local(ctx, layer_id, x, y, value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_visual_apply_canvas_circle_fill_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                                     uint32_t layer_id,
                                                                                     uint32_t center_x,
                                                                                     uint32_t center_y,
                                                                                     uint32_t edge_x,
                                                                                     uint32_t edge_y,
                                                                                     DrawingProgramRasterSample value) {
    int32_t cx = (int32_t)center_x;
    int32_t cy = (int32_t)center_y;
    int32_t rx = (int32_t)edge_x - cx;
    int32_t ry = (int32_t)edge_y - cy;
    int32_t r = (rx < 0 ? -rx : rx);
    int32_t ay = (ry < 0 ? -ry : ry);
    int32_t min_x;
    int32_t max_x;
    int32_t min_y;
    int32_t max_y;
    int32_t y;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid circle-fill request" };
    }
    if (ay > r) {
        r = ay;
    }
    if (r <= 0) {
        return apply_sample_if_changed_on_layer_local(ctx, layer_id, center_x, center_y, value);
    }
    min_x = cx - r;
    max_x = cx + r;
    min_y = cy - r;
    max_y = cy + r;
    for (y = min_y; y <= max_y; ++y) {
        int32_t x;
        if (y < 0 || (uint32_t)y >= ctx->document.raster_height) {
            continue;
        }
        for (x = min_x; x <= max_x; ++x) {
            int32_t dx;
            int32_t dy;
            if (x < 0 || (uint32_t)x >= ctx->document.raster_width) {
                continue;
            }
            dx = x - cx;
            dy = y - cy;
            if ((dx * dx) + (dy * dy) <= (r * r)) {
                CoreResult result = apply_sample_if_changed_on_layer_local(ctx, layer_id, (uint32_t)x, (uint32_t)y, value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_visual_apply_canvas_circle_outline_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                                        uint32_t layer_id,
                                                                                        uint32_t center_x,
                                                                                        uint32_t center_y,
                                                                                        uint32_t edge_x,
                                                                                        uint32_t edge_y,
                                                                                        DrawingProgramRasterSample value,
                                                                                        uint32_t stroke_width) {
    int32_t cx = (int32_t)center_x;
    int32_t cy = (int32_t)center_y;
    int32_t rx = (int32_t)edge_x - cx;
    int32_t ry = (int32_t)edge_y - cy;
    int32_t outer_r = (rx < 0 ? -rx : rx);
    int32_t ay = (ry < 0 ? -ry : ry);
    int32_t inner_r;
    int32_t min_x;
    int32_t max_x;
    int32_t min_y;
    int32_t max_y;
    int32_t y;
    int32_t outer_r2;
    int32_t inner_r2;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid circle-outline request" };
    }
    if (stroke_width < 1u) {
        stroke_width = 1u;
    }
    if (ay > outer_r) {
        outer_r = ay;
    }
    if (outer_r <= 0) {
        return drawing_program_visual_apply_canvas_stamp_square_on_layer(ctx, layer_id, cx, cy, value, stroke_width, 100u);
    }
    inner_r = outer_r - (int32_t)stroke_width + 1;
    if (inner_r < 0) {
        inner_r = 0;
    }
    outer_r2 = outer_r * outer_r;
    inner_r2 = inner_r * inner_r;
    min_x = cx - outer_r;
    max_x = cx + outer_r;
    min_y = cy - outer_r;
    max_y = cy + outer_r;
    for (y = min_y; y <= max_y; ++y) {
        int32_t x;
        if (y < 0 || (uint32_t)y >= ctx->document.raster_height) {
            continue;
        }
        for (x = min_x; x <= max_x; ++x) {
            int32_t dx;
            int32_t dy;
            int32_t d2;
            if (x < 0 || (uint32_t)x >= ctx->document.raster_width) {
                continue;
            }
            dx = x - cx;
            dy = y - cy;
            d2 = (dx * dx) + (dy * dy);
            if (d2 <= outer_r2 && d2 >= inner_r2) {
                CoreResult result = apply_sample_if_changed_on_layer_local(ctx, layer_id, (uint32_t)x, (uint32_t)y, value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
    }
    return core_result_ok();
}
