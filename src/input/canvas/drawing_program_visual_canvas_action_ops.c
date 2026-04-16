#include "drawing_program/drawing_program_visual_canvas_action_ops.h"

#include <string.h>

CoreResult drawing_program_visual_apply_canvas_picker_at_screen(DrawingProgramAppContext *ctx,
                                                                SDL_Rect pane_rect,
                                                                int sx,
                                                                int sy,
                                                                const DrawingProgramVisualCanvasActionOpsHooks *hooks) {
    uint32_t sample_x = 0u;
    uint32_t sample_y = 0u;
    uint8_t sample = 0u;
    CoreResult result;
    if (!ctx || !hooks || !hooks->screen_to_canvas_sample || !hooks->color_index_for_sample) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid picker action request" };
    }
    if (!hooks->screen_to_canvas_sample(ctx, pane_rect, sx, sy, &sample_x, &sample_y)) {
        return core_result_ok();
    }
    result = drawing_program_document_sample_read(&ctx->document, sample_x, sample_y, &sample);
    if (result.code != CORE_OK) {
        return result;
    }
    ctx->ui.active_color_index = hooks->color_index_for_sample(sample);
    return core_result_ok();
}

CoreResult drawing_program_visual_apply_canvas_fill_at_screen(DrawingProgramAppContext *ctx,
                                                              SDL_Rect pane_rect,
                                                              int sx,
                                                              int sy,
                                                              const DrawingProgramVisualCanvasActionOpsHooks *hooks) {
    static uint32_t queue[DRAWING_PROGRAM_MAX_RASTER_SAMPLES];
    static uint8_t region_mask[DRAWING_PROGRAM_MAX_RASTER_SAMPLES];
    const uint8_t *active_layer_samples = 0;
    uint32_t start_x = 0u;
    uint32_t start_y = 0u;
    uint32_t width;
    uint32_t height;
    uint32_t start_index;
    uint32_t active_layer_sample_count = 0u;
    uint32_t active_layer_id = 0u;
    uint32_t head = 0u;
    uint32_t tail = 0u;
    uint32_t fill_region_count = 0u;
    uint32_t min_region_y = 0u;
    uint32_t max_region_y = 0u;
    uint8_t target = 0u;
    uint8_t replacement = 0u;
    uint8_t tolerance_setting = 0u;
    CoreResult result;
    if (!ctx || !hooks || !hooks->screen_to_canvas_sample || !hooks->active_layer_allows_edits_visual ||
        !hooks->active_layer_query || !hooks->sample_value_for_tool || !hooks->tool_fill_tolerance_setting ||
        !hooks->fill_sample_matches_tolerance || !hooks->active_layer_sample_read_visual) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid fill action request" };
    }
    if (!hooks->active_layer_allows_edits_visual(ctx)) {
        return core_result_ok();
    }
    if (!hooks->active_layer_query(ctx, &active_layer_id, 0, 0, 0)) {
        return core_result_ok();
    }
    if (active_layer_id == 0u) {
        return core_result_ok();
    }
    if (!hooks->screen_to_canvas_sample(ctx, pane_rect, sx, sy, &start_x, &start_y)) {
        return core_result_ok();
    }
    width = ctx->document.raster_width;
    height = ctx->document.raster_height;
    if (width == 0u || height == 0u || ctx->document.raster_sample_count == 0u) {
        return core_result_ok();
    }
    replacement = hooks->sample_value_for_tool(ctx, ctx->editor.active_tool);
    tolerance_setting = hooks->tool_fill_tolerance_setting(ctx);
    result = drawing_program_layer_raster_store_export_layer(&ctx->layer_rasters,
                                                             active_layer_id,
                                                             &active_layer_samples,
                                                             &active_layer_sample_count);
    if (result.code != CORE_OK ||
        !active_layer_samples ||
        active_layer_sample_count != ctx->document.raster_sample_count) {
        active_layer_samples = ctx->document.raster_samples;
    }
    result = hooks->active_layer_sample_read_visual(ctx, start_x, start_y, &target);
    if (result.code != CORE_OK) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "fill start sample read failed" };
    }
    if (target == replacement) {
        return core_result_ok();
    }
    memset(region_mask, 0, ctx->document.raster_sample_count * sizeof(region_mask[0]));
    start_index = (start_y * width) + start_x;
    queue[tail++] = start_index;
    region_mask[start_index] = 1u;
    fill_region_count = 1u;
    min_region_y = start_y;
    max_region_y = start_y;
    while (head < tail) {
        uint32_t idx = queue[head++];
        uint32_t x = idx % width;
        uint32_t y = idx / width;
        uint8_t current = active_layer_samples[idx];
        if (!hooks->fill_sample_matches_tolerance(current, target, tolerance_setting)) {
            continue;
        }
        if (x > 0u) {
            uint32_t n = idx - 1u;
            if (!region_mask[n] && hooks->fill_sample_matches_tolerance(active_layer_samples[n], target, tolerance_setting)) {
                if (tail >= ctx->document.raster_sample_count) {
                    return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "fill queue exhausted" };
                }
                region_mask[n] = 1u;
                queue[tail++] = n;
                fill_region_count += 1u;
            }
        }
        if (x + 1u < width) {
            uint32_t n = idx + 1u;
            if (!region_mask[n] && hooks->fill_sample_matches_tolerance(active_layer_samples[n], target, tolerance_setting)) {
                if (tail >= ctx->document.raster_sample_count) {
                    return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "fill queue exhausted" };
                }
                region_mask[n] = 1u;
                queue[tail++] = n;
                fill_region_count += 1u;
            }
        }
        if (y > 0u) {
            uint32_t n = idx - width;
            if (!region_mask[n] && hooks->fill_sample_matches_tolerance(active_layer_samples[n], target, tolerance_setting)) {
                if (tail >= ctx->document.raster_sample_count) {
                    return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "fill queue exhausted" };
                }
                region_mask[n] = 1u;
                queue[tail++] = n;
                fill_region_count += 1u;
                if (y - 1u < min_region_y) {
                    min_region_y = y - 1u;
                }
            }
        }
        if (y + 1u < height) {
            uint32_t n = idx + width;
            if (!region_mask[n] && hooks->fill_sample_matches_tolerance(active_layer_samples[n], target, tolerance_setting)) {
                if (tail >= ctx->document.raster_sample_count) {
                    return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "fill queue exhausted" };
                }
                region_mask[n] = 1u;
                queue[tail++] = n;
                fill_region_count += 1u;
                if (y + 1u > max_region_y) {
                    max_region_y = y + 1u;
                }
            }
        }
        if (y < min_region_y) {
            min_region_y = y;
        }
        if (y > max_region_y) {
            max_region_y = y;
        }
    }
    if (fill_region_count == 0u) {
        return core_result_ok();
    }
    {
        uint32_t y;
        for (y = min_region_y; y <= max_region_y && y < height; ++y) {
            uint32_t x = 0u;
            uint32_t row_start = y * width;
            while (x < width) {
                uint32_t span_start_x = x;
                uint32_t span_len = 0u;
                while (x < width && !region_mask[row_start + x]) {
                    x += 1u;
                }
                if (x >= width) {
                    break;
                }
                span_start_x = x;
                while (x < width && region_mask[row_start + x]) {
                    span_len += 1u;
                    x += 1u;
                }
                if (span_len > 0u) {
                    CoreResult span_result = drawing_program_history_apply_set_sample_span_value(&ctx->history,
                                                                                                  &ctx->document,
                                                                                                  &ctx->layer_rasters,
                                                                                                  active_layer_id,
                                                                                                  row_start + span_start_x,
                                                                                                  span_len,
                                                                                                  replacement);
                    if (span_result.code != CORE_OK) {
                        return span_result;
                    }
                }
            }
        }
    }
    return core_result_ok();
}
