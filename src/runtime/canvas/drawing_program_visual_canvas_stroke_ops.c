#include "drawing_program/drawing_program_visual_canvas_stroke_ops.h"

#include <math.h>

#include "drawing_program/drawing_program_color_model.h"

enum {
    DRAWING_PROGRAM_SHAPE_HISTORY_UNIT_DELTA_LIMIT =
        DRAWING_PROGRAM_HISTORY_DELTA_BLOCK_FLUSH_CAPACITY * 8u
};

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

static int raster_history_find_pending_delta_local(const DrawingProgramVisualRasterHistoryBatch *batch,
                                                   uint32_t sample_index) {
    int32_t i;
    if (!batch) {
        return -1;
    }
    for (i = (int32_t)batch->pending_delta_count - 1; i >= 0; --i) {
        if (batch->pending_deltas[i].sample_index == sample_index) {
            return (int)i;
        }
    }
    return -1;
}

static void raster_history_remove_pending_delta_local(DrawingProgramVisualRasterHistoryBatch *batch,
                                                      uint32_t pending_index) {
    if (!batch || pending_index >= batch->pending_delta_count) {
        return;
    }
    batch->pending_delta_count -= 1u;
    if (pending_index != batch->pending_delta_count) {
        batch->pending_deltas[pending_index] = batch->pending_deltas[batch->pending_delta_count];
    }
}

void drawing_program_visual_raster_history_batch_init(DrawingProgramVisualRasterHistoryBatch *batch,
                                                      uint32_t layer_id) {
    if (!batch) {
        return;
    }
    batch->layer_id = layer_id;
    batch->pending_delta_count = 0u;
    batch->group_delta_count = 0u;
    batch->group_open = 0u;
}

static CoreResult raster_history_begin_group_if_needed_local(DrawingProgramAppContext *ctx,
                                                             DrawingProgramVisualRasterHistoryBatch *batch) {
    if (!ctx || !batch) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid raster-history begin request" };
    }
    if (batch->group_open) {
        return core_result_ok();
    }
    {
        CoreResult result = drawing_program_history_begin_group(&ctx->history);
        if (result.code != CORE_OK) {
            return result;
        }
    }
    batch->group_open = 1u;
    return core_result_ok();
}

static CoreResult raster_history_rotate_group_if_needed_local(DrawingProgramAppContext *ctx,
                                                              DrawingProgramVisualRasterHistoryBatch *batch) {
    CoreResult result;
    if (!ctx || !batch) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid raster-history rotate request" };
    }
    if (!batch->group_open || batch->group_delta_count < DRAWING_PROGRAM_SHAPE_HISTORY_UNIT_DELTA_LIMIT) {
        return core_result_ok();
    }
    result = drawing_program_history_end_group(&ctx->history);
    if (result.code != CORE_OK) {
        return result;
    }
    batch->group_open = 0u;
    batch->group_delta_count = 0u;
    return raster_history_begin_group_if_needed_local(ctx, batch);
}

static CoreResult raster_history_flush_pending_local(DrawingProgramAppContext *ctx,
                                                     DrawingProgramVisualRasterHistoryBatch *batch) {
    CoreResult result;
    uint32_t flushed_count;
    if (!ctx || !batch) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid raster-history flush request" };
    }
    if (batch->pending_delta_count == 0u) {
        return core_result_ok();
    }
    result = raster_history_begin_group_if_needed_local(ctx, batch);
    if (result.code != CORE_OK) {
        return result;
    }
    flushed_count = batch->pending_delta_count;
    result = drawing_program_history_apply_raster_delta_block(&ctx->history,
                                                              &ctx->document,
                                                              &ctx->layer_rasters,
                                                              batch->layer_id,
                                                              batch->pending_deltas,
                                                              batch->pending_delta_count);
    if (result.code != CORE_OK) {
        return result;
    }
    batch->pending_delta_count = 0u;
    batch->group_delta_count += flushed_count;
    return raster_history_rotate_group_if_needed_local(ctx, batch);
}

CoreResult drawing_program_visual_raster_history_batch_finish(DrawingProgramAppContext *ctx,
                                                              DrawingProgramVisualRasterHistoryBatch *batch) {
    CoreResult result = core_result_ok();
    if (!ctx || !batch) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid raster-history finish request" };
    }
    result = raster_history_flush_pending_local(ctx, batch);
    if (result.code != CORE_OK) {
        if (batch->group_open) {
            (void)drawing_program_history_end_group(&ctx->history);
            batch->group_open = 0u;
        }
        return result;
    }
    if (batch->group_open) {
        result = drawing_program_history_end_group(&ctx->history);
        batch->group_open = 0u;
    }
    return result;
}

static CoreResult record_raster_history_sample_change_local(DrawingProgramAppContext *ctx,
                                                            DrawingProgramVisualRasterHistoryBatch *batch,
                                                            uint32_t layer_id,
                                                            uint32_t sample_x,
                                                            uint32_t sample_y,
                                                            DrawingProgramRasterSample value) {
    DrawingProgramRasterSample previous_value = drawing_program_color_eraser_value();
    uint32_t sample_index;
    int existing_index;
    CoreResult result;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid raster-history sample record request" };
    }
    if (!batch) {
        return apply_sample_if_changed_on_layer_local(ctx, layer_id, sample_x, sample_y, value);
    }
    result = layer_sample_read_visual_local(ctx, layer_id, sample_x, sample_y, &previous_value);
    if (result.code != CORE_OK) {
        return result;
    }
    value = drawing_program_color_normalize_input_sample(value);
    if (previous_value == value) {
        return core_result_ok();
    }
    sample_index = (sample_y * ctx->document.raster_width) + sample_x;
    existing_index = raster_history_find_pending_delta_local(batch, sample_index);
    if (existing_index >= 0) {
        DrawingProgramHistoryRasterDeltaEntry *entry = &batch->pending_deltas[existing_index];
        entry->new_sample_value = value;
        if (entry->new_sample_value == entry->previous_sample_value) {
            raster_history_remove_pending_delta_local(batch, (uint32_t)existing_index);
        }
        return core_result_ok();
    }
    if (batch->pending_delta_count >= DRAWING_PROGRAM_HISTORY_DELTA_BLOCK_FLUSH_CAPACITY) {
        result = raster_history_flush_pending_local(ctx, batch);
        if (result.code != CORE_OK) {
            return result;
        }
    }
    batch->pending_deltas[batch->pending_delta_count].sample_index = sample_index;
    batch->pending_deltas[batch->pending_delta_count].previous_sample_value = previous_value;
    batch->pending_deltas[batch->pending_delta_count].new_sample_value = value;
    batch->pending_delta_count += 1u;
    return core_result_ok();
}

static CoreResult compose_visible_sample_on_layer_local(const DrawingProgramAppContext *ctx,
                                                        uint32_t sample_x,
                                                        uint32_t sample_y,
                                                        DrawingProgramRasterSample *out_value) {
    uint32_t i;
    DrawingProgramRasterSample composed = drawing_program_color_eraser_value();
    if (!ctx || !out_value) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid visible sample compose request" };
    }
    for (i = 0u; i < ctx->document.layer_count; ++i) {
        DrawingProgramRasterSample sample = drawing_program_color_eraser_value();
        if (!ctx->document.layers[i].visible) {
            continue;
        }
        if (drawing_program_layer_raster_store_raster_sample_read(&ctx->layer_rasters,
                                                                  ctx->document.layers[i].layer_id,
                                                                  sample_x,
                                                                  sample_y,
                                                                  &sample).code == CORE_OK &&
            !drawing_program_color_sample_is_transparent(sample)) {
            composed = sample;
        }
    }
    *out_value = composed;
    return core_result_ok();
}

static CoreResult write_sample_without_history_on_layer_local(DrawingProgramAppContext *ctx,
                                                              uint32_t layer_id,
                                                              uint32_t sample_x,
                                                              uint32_t sample_y,
                                                              DrawingProgramRasterSample value) {
    DrawingProgramRasterSample composed = drawing_program_color_eraser_value();
    CoreResult result;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid non-history sample write request" };
    }
    result = drawing_program_layer_raster_store_sample_write(
        &ctx->layer_rasters, layer_id, sample_x, sample_y, value, 0);
    if (result.code != CORE_OK) {
        return result;
    }
    result = compose_visible_sample_on_layer_local(ctx, sample_x, sample_y, &composed);
    if (result.code != CORE_OK) {
        return result;
    }
    return drawing_program_document_sample_write(&ctx->document, sample_x, sample_y, composed, 0);
}

static int direct_stroke_find_pending_delta_local(const VisualCanvasInteractionState *interaction,
                                                  uint32_t sample_index) {
    int32_t i;
    if (!interaction) {
        return -1;
    }
    for (i = (int32_t)interaction->direct_stroke_pending_delta_count - 1; i >= 0; --i) {
        if (interaction->direct_stroke_pending_deltas[i].sample_index == sample_index) {
            return (int)i;
        }
    }
    return -1;
}

static CoreResult apply_hard_row_span_on_layer_local(DrawingProgramAppContext *ctx,
                                                     DrawingProgramVisualRasterHistoryBatch *history_batch,
                                                     uint32_t layer_id,
                                                     int32_t sample_y,
                                                     int32_t start_x,
                                                     int32_t end_x,
                                                     DrawingProgramRasterSample value) {
    int32_t clamped_start_x;
    int32_t clamped_end_x;
    int32_t segment_start_x;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid hard-row span request" };
    }
    if (sample_y < 0 || (uint32_t)sample_y >= ctx->document.raster_height) {
        return core_result_ok();
    }
    if (start_x > end_x) {
        int32_t swap = start_x;
        start_x = end_x;
        end_x = swap;
    }
    clamped_start_x = start_x;
    if (clamped_start_x < 0) {
        clamped_start_x = 0;
    }
    clamped_end_x = end_x;
    if (clamped_end_x >= (int32_t)ctx->document.raster_width) {
        clamped_end_x = (int32_t)ctx->document.raster_width - 1;
    }
    if (clamped_start_x > clamped_end_x) {
        return core_result_ok();
    }
    if (history_batch) {
        for (segment_start_x = clamped_start_x; segment_start_x <= clamped_end_x; ++segment_start_x) {
            CoreResult result = record_raster_history_sample_change_local(
                ctx, history_batch, layer_id, (uint32_t)segment_start_x, (uint32_t)sample_y, value);
            if (result.code != CORE_OK) {
                return result;
            }
        }
        return core_result_ok();
    }
    segment_start_x = clamped_start_x;
    while (segment_start_x <= clamped_end_x) {
        DrawingProgramRasterSample prev = drawing_program_color_eraser_value();
        int32_t segment_end_x = segment_start_x;
        CoreResult result = layer_sample_read_visual_local(
            ctx, layer_id, (uint32_t)segment_start_x, (uint32_t)sample_y, &prev);
        if (result.code != CORE_OK) {
            return result;
        }
        while (segment_end_x < clamped_end_x) {
            DrawingProgramRasterSample probe = drawing_program_color_eraser_value();
            result = layer_sample_read_visual_local(
                ctx, layer_id, (uint32_t)(segment_end_x + 1), (uint32_t)sample_y, &probe);
            if (result.code != CORE_OK) {
                return result;
            }
            if (probe != prev) {
                break;
            }
            segment_end_x += 1;
        }
        result = drawing_program_history_apply_set_sample_span_value(
            &ctx->history,
            &ctx->document,
            &ctx->layer_rasters,
            layer_id,
            ((uint32_t)sample_y * ctx->document.raster_width) + (uint32_t)segment_start_x,
            (uint32_t)(segment_end_x - segment_start_x + 1),
            value);
        if (result.code != CORE_OK) {
            return result;
        }
        segment_start_x = segment_end_x + 1;
    }
    return core_result_ok();
}

CoreResult drawing_program_visual_flush_direct_stroke_history(DrawingProgramAppContext *ctx,
                                                              VisualCanvasInteractionState *interaction,
                                                              uint32_t layer_id) {
    if (!ctx || !interaction) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid direct-stroke history flush request" };
    }
    if (interaction->direct_stroke_pending_delta_count == 0u) {
        interaction->direct_stroke_history_layer_id = 0u;
        return core_result_ok();
    }
    if (layer_id == 0u) {
        layer_id = interaction->direct_stroke_history_layer_id;
    }
    if (layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "missing direct-stroke history layer id" };
    }
    {
        CoreResult result = drawing_program_history_apply_raster_delta_block(
            &ctx->history,
            &ctx->document,
            &ctx->layer_rasters,
            layer_id,
            interaction->direct_stroke_pending_deltas,
            interaction->direct_stroke_pending_delta_count);
        if (result.code != CORE_OK) {
            return result;
        }
    }
    interaction->direct_stroke_pending_delta_count = 0u;
    interaction->direct_stroke_history_layer_id = 0u;
    return core_result_ok();
}

static CoreResult record_direct_stroke_sample_change_local(DrawingProgramAppContext *ctx,
                                                           VisualCanvasInteractionState *interaction,
                                                           uint32_t layer_id,
                                                           uint32_t sample_x,
                                                           uint32_t sample_y,
                                                           DrawingProgramRasterSample value) {
    DrawingProgramRasterSample previous_value = drawing_program_color_eraser_value();
    uint32_t sample_index;
    int existing_index;
    CoreResult result;
    if (!ctx || !interaction || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid direct-stroke sample record request" };
    }
    result = layer_sample_read_visual_local(ctx, layer_id, sample_x, sample_y, &previous_value);
    if (result.code != CORE_OK) {
        return result;
    }
    if (previous_value == value) {
        return core_result_ok();
    }
    sample_index = (sample_y * ctx->document.raster_width) + sample_x;
    existing_index = direct_stroke_find_pending_delta_local(interaction, sample_index);
    if (existing_index < 0 &&
        interaction->direct_stroke_pending_delta_count >= DRAWING_PROGRAM_DIRECT_STROKE_PENDING_DELTA_CAPACITY) {
        result = drawing_program_visual_flush_direct_stroke_history(ctx, interaction, layer_id);
        if (result.code != CORE_OK) {
            return result;
        }
        existing_index = -1;
    }
    if (interaction->direct_stroke_history_layer_id == 0u) {
        interaction->direct_stroke_history_layer_id = layer_id;
    } else if (interaction->direct_stroke_history_layer_id != layer_id) {
        result = drawing_program_visual_flush_direct_stroke_history(
            ctx, interaction, interaction->direct_stroke_history_layer_id);
        if (result.code != CORE_OK) {
            return result;
        }
        interaction->direct_stroke_history_layer_id = layer_id;
        existing_index = -1;
    }
    if (existing_index >= 0) {
        interaction->direct_stroke_pending_deltas[existing_index].new_sample_value = value;
    } else {
        DrawingProgramHistoryRasterDeltaEntry *entry =
            &interaction->direct_stroke_pending_deltas[interaction->direct_stroke_pending_delta_count++];
        entry->sample_index = sample_index;
        entry->previous_sample_value = previous_value;
        entry->new_sample_value = value;
    }
    return write_sample_without_history_on_layer_local(ctx, layer_id, sample_x, sample_y, value);
}

static DrawingProgramRasterSample blend_sample_value_u8_local(DrawingProgramRasterSample dst,
                                                              DrawingProgramRasterSample src,
                                                              uint8_t opacity_percent) {
    return drawing_program_color_blend_samples(dst, src, clamp_percent_u8_local(opacity_percent));
}

static CoreResult apply_canvas_stamp_square_on_layer_with_history_batch_local(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *history_batch,
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
            CoreResult result = apply_hard_row_span_on_layer_local(
                ctx,
                history_batch,
                layer_id,
                sample_y + y,
                sample_x + start_offset,
                sample_x + end_offset,
                value);
            if (result.code != CORE_OK) {
                return result;
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
                    CoreResult result = record_raster_history_sample_change_local(
                        ctx, history_batch, layer_id, (uint32_t)tx, (uint32_t)ty, out_value);
                    if (result.code != CORE_OK) {
                        return result;
                    }
                }
            }
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_visual_apply_canvas_stamp_square_on_layer(DrawingProgramAppContext *ctx,
                                                                     uint32_t layer_id,
                                                                     int32_t sample_x,
                                                                     int32_t sample_y,
                                                                     DrawingProgramRasterSample value,
                                                                     uint32_t stroke_width,
                                                                     uint8_t hardness_percent) {
    return apply_canvas_stamp_square_on_layer_with_history_batch_local(
        ctx, 0, layer_id, sample_x, sample_y, value, stroke_width, hardness_percent);
}

CoreResult drawing_program_visual_apply_canvas_direct_stroke_stamp_square_on_layer(
    DrawingProgramAppContext *ctx,
    VisualCanvasInteractionState *interaction,
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
    if (!ctx || !interaction || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid direct-stroke stamp request" };
    }
    if (stroke_width < 1u) {
        stroke_width = 1u;
    }
    hardness_percent = clamp_setting_u8_local(hardness_percent, 1u, 100u);
    start_offset = -((int32_t)stroke_width / 2);
    end_offset = start_offset + (int32_t)stroke_width - 1;
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
            DrawingProgramRasterSample out_value = value;
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
            if (opacity < 100u) {
                DrawingProgramRasterSample current = drawing_program_color_eraser_value();
                (void)layer_sample_read_visual_local(ctx, layer_id, (uint32_t)tx, (uint32_t)ty, &current);
                out_value = blend_sample_value_u8_local(current, value, opacity);
            }
            {
                CoreResult result = record_direct_stroke_sample_change_local(
                    ctx, interaction, layer_id, (uint32_t)tx, (uint32_t)ty, out_value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
    }
    return core_result_ok();
}

static CoreResult apply_canvas_line_between_samples_with_history_batch_local(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *history_batch,
    uint32_t layer_id,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    DrawingProgramRasterSample value,
    uint32_t stroke_width,
    uint8_t hardness_percent) {
    int32_t x = x0;
    int32_t y = y0;
    int32_t tx = x1;
    int32_t ty = y1;
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
            CoreResult stamp_result = apply_canvas_stamp_square_on_layer_with_history_batch_local(
                ctx, history_batch, layer_id, x, y, value, stroke_width, hardness_percent);
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

CoreResult drawing_program_visual_apply_canvas_line_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                              uint32_t layer_id,
                                                                              int32_t x0,
                                                                              int32_t y0,
                                                                              int32_t x1,
                                                                              int32_t y1,
                                                                              DrawingProgramRasterSample value,
                                                                              uint32_t stroke_width,
                                                                              uint8_t hardness_percent) {
    return apply_canvas_line_between_samples_with_history_batch_local(
        ctx, 0, layer_id, x0, y0, x1, y1, value, stroke_width, hardness_percent);
}

CoreResult drawing_program_visual_apply_canvas_line_between_samples_with_history_batch(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *history_batch,
    uint32_t layer_id,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    DrawingProgramRasterSample value,
    uint32_t stroke_width,
    uint8_t hardness_percent) {
    return apply_canvas_line_between_samples_with_history_batch_local(
        ctx, history_batch, layer_id, x0, y0, x1, y1, value, stroke_width, hardness_percent);
}

static CoreResult apply_canvas_rect_fill_between_samples_with_history_batch_local(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *history_batch,
    uint32_t layer_id,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    DrawingProgramRasterSample value) {
    int32_t min_x = (x0 < x1) ? x0 : x1;
    int32_t min_y = (y0 < y1) ? y0 : y1;
    int32_t max_x = (x0 > x1) ? x0 : x1;
    int32_t max_y = (y0 > y1) ? y0 : y1;
    int32_t y;
    int32_t x;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid rect-fill request" };
    }
    if (min_x < 0) {
        min_x = 0;
    }
    if (min_y < 0) {
        min_y = 0;
    }
    if (max_x >= (int32_t)ctx->document.raster_width) {
        max_x = (int32_t)ctx->document.raster_width - 1;
    }
    if (max_y >= (int32_t)ctx->document.raster_height) {
        max_y = (int32_t)ctx->document.raster_height - 1;
    }
    if (min_x > max_x || min_y > max_y) {
        return core_result_ok();
    }
    for (y = min_y; y <= max_y; ++y) {
        for (x = min_x; x <= max_x; ++x) {
            CoreResult result = record_raster_history_sample_change_local(
                ctx, history_batch, layer_id, (uint32_t)x, (uint32_t)y, value);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_visual_apply_canvas_rect_fill_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                                   uint32_t layer_id,
                                                                                   int32_t x0,
                                                                                   int32_t y0,
                                                                                   int32_t x1,
                                                                                   int32_t y1,
                                                                                   DrawingProgramRasterSample value) {
    return apply_canvas_rect_fill_between_samples_with_history_batch_local(
        ctx, 0, layer_id, x0, y0, x1, y1, value);
}

CoreResult drawing_program_visual_apply_canvas_rect_fill_between_samples_with_history_batch(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *history_batch,
    uint32_t layer_id,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    DrawingProgramRasterSample value) {
    return apply_canvas_rect_fill_between_samples_with_history_batch_local(
        ctx, history_batch, layer_id, x0, y0, x1, y1, value);
}

static CoreResult apply_canvas_rect_outline_between_samples_with_history_batch_local(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *history_batch,
    uint32_t layer_id,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    DrawingProgramRasterSample value,
    uint32_t stroke_width) {
    int32_t min_x = (x0 < x1) ? x0 : x1;
    int32_t min_y = (y0 < y1) ? y0 : y1;
    int32_t max_x = (x0 > x1) ? x0 : x1;
    int32_t max_y = (y0 > y1) ? y0 : y1;
    int32_t y;
    int32_t x;
    int32_t rect_width;
    int32_t rect_height;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid rect-outline request" };
    }
    if (stroke_width < 1u) {
        stroke_width = 1u;
    }
    if (min_x < 0) {
        min_x = 0;
    }
    if (min_y < 0) {
        min_y = 0;
    }
    if (max_x >= (int32_t)ctx->document.raster_width) {
        max_x = (int32_t)ctx->document.raster_width - 1;
    }
    if (max_y >= (int32_t)ctx->document.raster_height) {
        max_y = (int32_t)ctx->document.raster_height - 1;
    }
    if (min_x > max_x || min_y > max_y) {
        return core_result_ok();
    }
    rect_width = (max_x - min_x) + 1u;
    rect_height = (max_y - min_y) + 1u;
    if ((int32_t)stroke_width >= rect_width || (int32_t)stroke_width >= rect_height) {
        return apply_canvas_rect_fill_between_samples_with_history_batch_local(
            ctx, history_batch, layer_id, min_x, min_y, max_x, max_y, value);
    }
    for (y = min_y; y <= max_y; ++y) {
        for (x = min_x; x <= max_x; ++x) {
            int32_t left_dist = x - min_x;
            int32_t right_dist = max_x - x;
            int32_t top_dist = y - min_y;
            int32_t bottom_dist = max_y - y;
            if (left_dist < (int32_t)stroke_width ||
                right_dist < (int32_t)stroke_width ||
                top_dist < (int32_t)stroke_width ||
                bottom_dist < (int32_t)stroke_width) {
                CoreResult result = record_raster_history_sample_change_local(
                    ctx, history_batch, layer_id, (uint32_t)x, (uint32_t)y, value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_visual_apply_canvas_rect_outline_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                                      uint32_t layer_id,
                                                                                      int32_t x0,
                                                                                      int32_t y0,
                                                                                      int32_t x1,
                                                                                      int32_t y1,
                                                                                      DrawingProgramRasterSample value,
                                                                                      uint32_t stroke_width) {
    return apply_canvas_rect_outline_between_samples_with_history_batch_local(
        ctx, 0, layer_id, x0, y0, x1, y1, value, stroke_width);
}

CoreResult drawing_program_visual_apply_canvas_rect_outline_between_samples_with_history_batch(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *history_batch,
    uint32_t layer_id,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    DrawingProgramRasterSample value,
    uint32_t stroke_width) {
    return apply_canvas_rect_outline_between_samples_with_history_batch_local(
        ctx, history_batch, layer_id, x0, y0, x1, y1, value, stroke_width);
}

static CoreResult apply_canvas_circle_fill_between_samples_with_history_batch_local(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *history_batch,
    uint32_t layer_id,
    int32_t center_x,
    int32_t center_y,
    int32_t edge_x,
    int32_t edge_y,
    DrawingProgramRasterSample value) {
    int32_t cx = center_x;
    int32_t cy = center_y;
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
        if (center_x < 0 || center_y < 0 ||
            center_x >= (int32_t)ctx->document.raster_width ||
            center_y >= (int32_t)ctx->document.raster_height) {
            return core_result_ok();
        }
        return record_raster_history_sample_change_local(
            ctx, history_batch, layer_id, (uint32_t)center_x, (uint32_t)center_y, value);
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
                CoreResult result = record_raster_history_sample_change_local(
                    ctx, history_batch, layer_id, (uint32_t)x, (uint32_t)y, value);
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
                                                                                     int32_t center_x,
                                                                                     int32_t center_y,
                                                                                     int32_t edge_x,
                                                                                     int32_t edge_y,
                                                                                     DrawingProgramRasterSample value) {
    return apply_canvas_circle_fill_between_samples_with_history_batch_local(
        ctx, 0, layer_id, center_x, center_y, edge_x, edge_y, value);
}

CoreResult drawing_program_visual_apply_canvas_circle_fill_between_samples_with_history_batch(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *history_batch,
    uint32_t layer_id,
    int32_t center_x,
    int32_t center_y,
    int32_t edge_x,
    int32_t edge_y,
    DrawingProgramRasterSample value) {
    return apply_canvas_circle_fill_between_samples_with_history_batch_local(
        ctx, history_batch, layer_id, center_x, center_y, edge_x, edge_y, value);
}

static CoreResult apply_canvas_circle_outline_between_samples_with_history_batch_local(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *history_batch,
    uint32_t layer_id,
    int32_t center_x,
    int32_t center_y,
    int32_t edge_x,
    int32_t edge_y,
    DrawingProgramRasterSample value,
    uint32_t stroke_width) {
    int32_t cx = center_x;
    int32_t cy = center_y;
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
        return apply_canvas_stamp_square_on_layer_with_history_batch_local(
            ctx, history_batch, layer_id, cx, cy, value, stroke_width, 100u);
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
                CoreResult result = record_raster_history_sample_change_local(
                    ctx, history_batch, layer_id, (uint32_t)x, (uint32_t)y, value);
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
                                                                                        int32_t center_x,
                                                                                        int32_t center_y,
                                                                                        int32_t edge_x,
                                                                                        int32_t edge_y,
                                                                                        DrawingProgramRasterSample value,
                                                                                        uint32_t stroke_width) {
    return apply_canvas_circle_outline_between_samples_with_history_batch_local(
        ctx, 0, layer_id, center_x, center_y, edge_x, edge_y, value, stroke_width);
}

CoreResult drawing_program_visual_apply_canvas_circle_outline_between_samples_with_history_batch(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *history_batch,
    uint32_t layer_id,
    int32_t center_x,
    int32_t center_y,
    int32_t edge_x,
    int32_t edge_y,
    DrawingProgramRasterSample value,
    uint32_t stroke_width) {
    return apply_canvas_circle_outline_between_samples_with_history_batch_local(
        ctx, history_batch, layer_id, center_x, center_y, edge_x, edge_y, value, stroke_width);
}
