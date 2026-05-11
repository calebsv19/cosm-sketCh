#include "drawing_program/drawing_program_visual_canvas_action_ops.h"

#include <string.h>

#include "drawing_program/drawing_program_ui_color_state.h"

static CoreResult visual_fill_enqueue_seed(uint32_t *queue,
                                           uint32_t queue_capacity,
                                           uint32_t *tail,
                                           uint8_t *queued_mask,
                                           uint32_t sample_index) {
    if (!queue || !tail || !queued_mask) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid fill queue request" };
    }
    if (queued_mask[sample_index]) {
        return core_result_ok();
    }
    if (*tail >= queue_capacity) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "fill queue exhausted" };
    }
    queued_mask[sample_index] = 1u;
    queue[*tail] = sample_index;
    *tail += 1u;
    return core_result_ok();
}

static CoreResult visual_fill_flush_pending_deltas(DrawingProgramAppContext *ctx,
                                                   uint32_t layer_id,
                                                   DrawingProgramHistoryRasterDeltaEntry *pending_deltas,
                                                   uint32_t *pending_delta_count) {
    CoreResult result;
    if (!ctx || !pending_deltas || !pending_delta_count) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid fill delta flush request" };
    }
    if (*pending_delta_count == 0u) {
        return core_result_ok();
    }
    result = drawing_program_history_apply_raster_delta_block(&ctx->history,
                                                              &ctx->document,
                                                              &ctx->layer_rasters,
                                                              layer_id,
                                                              pending_deltas,
                                                              *pending_delta_count);
    if (result.code != CORE_OK) {
        return result;
    }
    *pending_delta_count = 0u;
    return core_result_ok();
}

CoreResult drawing_program_visual_apply_canvas_picker_at_screen(DrawingProgramAppContext *ctx,
                                                                SDL_Rect pane_rect,
                                                                int sx,
                                                                int sy,
                                                                const DrawingProgramVisualCanvasActionOpsHooks *hooks) {
    uint32_t sample_x = 0u;
    uint32_t sample_y = 0u;
    DrawingProgramRasterSample sample = drawing_program_color_eraser_value();
    CoreResult result;
    if (!ctx || !hooks || !hooks->screen_to_canvas_sample || !hooks->visible_sample_read_visual) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid picker action request" };
    }
    if (!hooks->screen_to_canvas_sample(ctx, pane_rect, sx, sy, &sample_x, &sample_y)) {
        return core_result_ok();
    }
    result = hooks->visible_sample_read_visual(ctx, sample_x, sample_y, &sample);
    if (result.code != CORE_OK) {
        return result;
    }
    if (drawing_program_color_sample_is_transparent(sample)) {
        return core_result_ok();
    }
    drawing_program_ui_color_set_active_paint_sample(ctx, sample, 1u);
    return core_result_ok();
}

CoreResult drawing_program_visual_apply_canvas_fill_at_screen(DrawingProgramAppContext *ctx,
                                                              SDL_Rect pane_rect,
                                                              int sx,
                                                              int sy,
                                                              const DrawingProgramVisualCanvasActionOpsHooks *hooks) {
    static uint32_t queue[DRAWING_PROGRAM_MAX_RASTER_SAMPLES];
    static uint8_t queued_mask[DRAWING_PROGRAM_MAX_RASTER_SAMPLES];
    static uint8_t processed_mask[DRAWING_PROGRAM_MAX_RASTER_SAMPLES];
    static DrawingProgramRasterSample active_layer_snapshot[DRAWING_PROGRAM_MAX_RASTER_SAMPLES];
    static DrawingProgramHistoryRasterDeltaEntry
        pending_deltas[DRAWING_PROGRAM_HISTORY_DELTA_BLOCK_FLUSH_CAPACITY];
    const DrawingProgramRasterSample *active_layer_samples = 0;
    uint32_t start_x = 0u;
    uint32_t start_y = 0u;
    uint32_t width;
    uint32_t height;
    uint32_t start_index;
    uint32_t active_layer_sample_count = 0u;
    uint32_t active_layer_id = 0u;
    uint32_t head = 0u;
    uint32_t tail = 0u;
    uint32_t pending_delta_count = 0u;
    uint8_t group_open = 0u;
    DrawingProgramRasterSample target = drawing_program_color_eraser_value();
    DrawingProgramRasterSample replacement = drawing_program_color_eraser_value();
    uint8_t tolerance_setting = 0u;
    CoreResult result = core_result_ok();
    CoreResult end_group_result;
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
    memcpy(active_layer_snapshot,
           active_layer_samples,
           (size_t)ctx->document.raster_sample_count * sizeof(active_layer_snapshot[0]));
    result = hooks->active_layer_sample_read_visual(ctx, start_x, start_y, &target);
    if (result.code != CORE_OK) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "fill start sample read failed" };
    }
    if (target == replacement) {
        return core_result_ok();
    }
    result = drawing_program_history_begin_group(&ctx->history);
    if (result.code != CORE_OK) {
        return result;
    }
    group_open = 1u;
    memset(queued_mask, 0, ctx->document.raster_sample_count * sizeof(queued_mask[0]));
    memset(processed_mask, 0, ctx->document.raster_sample_count * sizeof(processed_mask[0]));
    start_index = (start_y * width) + start_x;
    result = visual_fill_enqueue_seed(queue,
                                      ctx->document.raster_sample_count,
                                      &tail,
                                      queued_mask,
                                      start_index);
    if (result.code != CORE_OK) {
        goto cleanup;
    }
    while (head < tail) {
        uint32_t idx = queue[head++];
        uint32_t x = idx % width;
        uint32_t y = idx / width;
        uint32_t row_start = y * width;
        uint32_t left = x;
        uint32_t right = x;
        uint32_t span_x = 0u;
        DrawingProgramRasterSample current = active_layer_snapshot[idx];
        if (processed_mask[idx] ||
            !hooks->fill_sample_matches_tolerance(current, target, tolerance_setting)) {
            continue;
        }
        while (left > 0u &&
               !processed_mask[row_start + left - 1u] &&
               hooks->fill_sample_matches_tolerance(active_layer_snapshot[row_start + left - 1u],
                                                    target,
                                                    tolerance_setting)) {
            left -= 1u;
        }
        while (right + 1u < width &&
               !processed_mask[row_start + right + 1u] &&
               hooks->fill_sample_matches_tolerance(active_layer_snapshot[row_start + right + 1u],
                                                    target,
                                                    tolerance_setting)) {
            right += 1u;
        }
        for (span_x = left; span_x <= right; ++span_x) {
            processed_mask[row_start + span_x] = 1u;
            pending_deltas[pending_delta_count].sample_index = row_start + span_x;
            pending_deltas[pending_delta_count].previous_sample_value =
                active_layer_snapshot[row_start + span_x];
            pending_deltas[pending_delta_count].new_sample_value = replacement;
            pending_delta_count += 1u;
            if (pending_delta_count >= DRAWING_PROGRAM_HISTORY_DELTA_BLOCK_FLUSH_CAPACITY) {
                result = visual_fill_flush_pending_deltas(
                    ctx, active_layer_id, pending_deltas, &pending_delta_count);
                if (result.code != CORE_OK) {
                    goto cleanup;
                }
            }
        }
        if (y > 0u) {
            uint32_t above_row_start = row_start - width;
            span_x = left;
            while (span_x <= right) {
                uint32_t probe_index = above_row_start + span_x;
                if (!processed_mask[probe_index] &&
                    !queued_mask[probe_index] &&
                    hooks->fill_sample_matches_tolerance(active_layer_snapshot[probe_index],
                                                         target,
                                                         tolerance_setting)) {
                    result = visual_fill_enqueue_seed(queue,
                                                      ctx->document.raster_sample_count,
                                                      &tail,
                                                      queued_mask,
                                                      probe_index);
                    if (result.code != CORE_OK) {
                        goto cleanup;
                    }
                    while (span_x <= right) {
                        probe_index = above_row_start + span_x;
                        if (processed_mask[probe_index] ||
                            !hooks->fill_sample_matches_tolerance(active_layer_snapshot[probe_index],
                                                                  target,
                                                                  tolerance_setting)) {
                            break;
                        }
                        span_x += 1u;
                    }
                } else {
                    span_x += 1u;
                }
            }
        }
        if (y + 1u < height) {
            uint32_t below_row_start = row_start + width;
            span_x = left;
            while (span_x <= right) {
                uint32_t probe_index = below_row_start + span_x;
                if (!processed_mask[probe_index] &&
                    !queued_mask[probe_index] &&
                    hooks->fill_sample_matches_tolerance(active_layer_snapshot[probe_index],
                                                         target,
                                                         tolerance_setting)) {
                    result = visual_fill_enqueue_seed(queue,
                                                      ctx->document.raster_sample_count,
                                                      &tail,
                                                      queued_mask,
                                                      probe_index);
                    if (result.code != CORE_OK) {
                        goto cleanup;
                    }
                    while (span_x <= right) {
                        probe_index = below_row_start + span_x;
                        if (processed_mask[probe_index] ||
                            !hooks->fill_sample_matches_tolerance(active_layer_snapshot[probe_index],
                                                                  target,
                                                                  tolerance_setting)) {
                            break;
                        }
                        span_x += 1u;
                    }
                } else {
                    span_x += 1u;
                }
            }
        }
    }
    if (pending_delta_count > 0u) {
        result = visual_fill_flush_pending_deltas(ctx, active_layer_id, pending_deltas, &pending_delta_count);
    }

cleanup:
    if (group_open) {
        end_group_result = drawing_program_history_end_group(&ctx->history);
        if (result.code == CORE_OK && end_group_result.code != CORE_OK) {
            result = end_group_result;
        }
    }
    return result;
}
