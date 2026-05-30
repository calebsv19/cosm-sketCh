#include "drawing_program_visual_canvas_stroke_ops_internal.h"

#include "drawing_program/drawing_program_color_model.h"

enum {
    DRAWING_PROGRAM_SHAPE_HISTORY_UNIT_DELTA_LIMIT =
        DRAWING_PROGRAM_HISTORY_DELTA_BLOCK_FLUSH_CAPACITY * 8u
};

CoreResult drawing_program_visual_layer_sample_read_local(const DrawingProgramAppContext *ctx,
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

CoreResult drawing_program_visual_record_raster_history_sample_change_local(
    DrawingProgramAppContext *ctx,
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
    result = drawing_program_visual_layer_sample_read_local(ctx, layer_id, sample_x, sample_y, &previous_value);
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

CoreResult drawing_program_visual_write_sample_without_history_on_layer_local(
    DrawingProgramAppContext *ctx,
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
