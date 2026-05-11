#include "drawing_program/drawing_program_selection.h"

#include <string.h>

#include "drawing_program/drawing_program_color_model.h"

enum {
    DRAWING_PROGRAM_SELECTION_HISTORY_UNIT_DELTA_LIMIT =
        DRAWING_PROGRAM_HISTORY_DELTA_BLOCK_FLUSH_CAPACITY * 8u
};

typedef struct DrawingProgramSelectionHistoryBatch {
    uint32_t layer_id;
    uint32_t pending_delta_count;
    uint32_t group_delta_count;
    uint8_t group_open;
    DrawingProgramHistoryRasterDeltaEntry pending_deltas[DRAWING_PROGRAM_HISTORY_DELTA_BLOCK_FLUSH_CAPACITY];
} DrawingProgramSelectionHistoryBatch;

static DrawingProgramRasterSample selection_seeded_background_sample(void) {
    return drawing_program_color_eraser_value();
}

static uint32_t selection_resolve_target_layer_id(const DrawingProgramDocument *document,
                                                  const DrawingProgramSelectionState *selection,
                                                  uint32_t active_layer_id) {
    uint32_t ignored_index = 0u;
    if (!document || document->layer_count == 0u) {
        return 0u;
    }
    if (selection && selection->layer_id != 0u &&
        drawing_program_document_layer_index_for_id(document, selection->layer_id, &ignored_index).code == CORE_OK) {
        return selection->layer_id;
    }
    if (active_layer_id != 0u &&
        drawing_program_document_layer_index_for_id(document, active_layer_id, &ignored_index).code == CORE_OK) {
        return active_layer_id;
    }
    return 0u;
}

static int selection_history_layer_store_usable(const DrawingProgramDocument *document,
                                                const DrawingProgramLayerRasterStore *layer_rasters) {
    return document && layer_rasters &&
           layer_rasters->raster_width == document->raster_width &&
           layer_rasters->raster_height == document->raster_height &&
           layer_rasters->sample_count == document->raster_sample_count;
}

static CoreResult selection_history_sample_read(const DrawingProgramDocument *document,
                                                const DrawingProgramLayerRasterStore *layer_rasters,
                                                uint32_t layer_id,
                                                uint32_t sample_x,
                                                uint32_t sample_y,
                                                DrawingProgramRasterSample *out_value) {
    if (!document || !out_value) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid selection history sample read request" };
    }
    if (selection_history_layer_store_usable(document, layer_rasters) && layer_id != 0u) {
        return drawing_program_layer_raster_store_raster_sample_read(
            layer_rasters, layer_id, sample_x, sample_y, out_value);
    }
    return drawing_program_document_raster_sample_read(document, sample_x, sample_y, out_value);
}

static int selection_history_find_pending_delta(const DrawingProgramSelectionHistoryBatch *batch,
                                                uint32_t sample_index) {
    uint32_t i;
    if (!batch) {
        return -1;
    }
    for (i = 0u; i < batch->pending_delta_count; ++i) {
        if (batch->pending_deltas[i].sample_index == sample_index) {
            return (int)i;
        }
    }
    return -1;
}

static void selection_history_remove_pending_delta(DrawingProgramSelectionHistoryBatch *batch,
                                                   uint32_t pending_index) {
    if (!batch || pending_index >= batch->pending_delta_count) {
        return;
    }
    batch->pending_delta_count -= 1u;
    if (pending_index != batch->pending_delta_count) {
        batch->pending_deltas[pending_index] = batch->pending_deltas[batch->pending_delta_count];
    }
}

static CoreResult selection_history_begin_group_if_needed(DrawingProgramHistory *history,
                                                          DrawingProgramSelectionHistoryBatch *batch) {
    if (!history || !batch) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid selection history begin request" };
    }
    if (batch->group_open) {
        return core_result_ok();
    }
    {
        CoreResult result = drawing_program_history_begin_group(history);
        if (result.code != CORE_OK) {
            return result;
        }
    }
    batch->group_open = 1u;
    return core_result_ok();
}

static CoreResult selection_history_rotate_group_if_needed(DrawingProgramHistory *history,
                                                           DrawingProgramSelectionHistoryBatch *batch) {
    CoreResult result;
    if (!history || !batch) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid selection history rotate request" };
    }
    if (!batch->group_open ||
        batch->group_delta_count < DRAWING_PROGRAM_SELECTION_HISTORY_UNIT_DELTA_LIMIT) {
        return core_result_ok();
    }
    result = drawing_program_history_end_group(history);
    if (result.code != CORE_OK) {
        return result;
    }
    batch->group_open = 0u;
    batch->group_delta_count = 0u;
    return selection_history_begin_group_if_needed(history, batch);
}

static CoreResult selection_history_flush_pending(DrawingProgramDocument *document,
                                                  DrawingProgramLayerRasterStore *layer_rasters,
                                                  DrawingProgramHistory *history,
                                                  DrawingProgramSelectionHistoryBatch *batch) {
    CoreResult result;
    uint32_t flushed_count;
    if (!document || !history || !batch) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid selection history flush request" };
    }
    if (batch->pending_delta_count == 0u) {
        return core_result_ok();
    }
    result = selection_history_begin_group_if_needed(history, batch);
    if (result.code != CORE_OK) {
        return result;
    }
    flushed_count = batch->pending_delta_count;
    result = drawing_program_history_apply_raster_delta_block(history,
                                                              document,
                                                              layer_rasters,
                                                              batch->layer_id,
                                                              batch->pending_deltas,
                                                              batch->pending_delta_count);
    if (result.code != CORE_OK) {
        return result;
    }
    batch->pending_delta_count = 0u;
    batch->group_delta_count += flushed_count;
    return selection_history_rotate_group_if_needed(history, batch);
}

static CoreResult selection_history_finish(DrawingProgramDocument *document,
                                           DrawingProgramLayerRasterStore *layer_rasters,
                                           DrawingProgramHistory *history,
                                           DrawingProgramSelectionHistoryBatch *batch) {
    CoreResult result = core_result_ok();
    if (!document || !history || !batch) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid selection history finish request" };
    }
    result = selection_history_flush_pending(document, layer_rasters, history, batch);
    if (result.code != CORE_OK) {
        if (batch->group_open) {
            (void)drawing_program_history_end_group(history);
            batch->group_open = 0u;
        }
        return result;
    }
    if (batch->group_open) {
        result = drawing_program_history_end_group(history);
        batch->group_open = 0u;
    }
    return result;
}

static CoreResult selection_history_record_sample(DrawingProgramDocument *document,
                                                  DrawingProgramLayerRasterStore *layer_rasters,
                                                  DrawingProgramHistory *history,
                                                  DrawingProgramSelectionHistoryBatch *batch,
                                                  uint32_t sample_x,
                                                  uint32_t sample_y,
                                                  DrawingProgramRasterSample value) {
    uint32_t sample_index;
    int existing_index;
    DrawingProgramRasterSample previous_value = drawing_program_color_eraser_value();
    CoreResult result;
    if (!document || !history || !batch) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid selection history record request" };
    }
    if (sample_x >= document->raster_width || sample_y >= document->raster_height) {
        return core_result_ok();
    }
    sample_index = (sample_y * document->raster_width) + sample_x;
    existing_index = selection_history_find_pending_delta(batch, sample_index);
    if (existing_index >= 0) {
        DrawingProgramHistoryRasterDeltaEntry *entry = &batch->pending_deltas[existing_index];
        entry->new_sample_value = drawing_program_color_normalize_input_sample(value);
        if (entry->new_sample_value == entry->previous_sample_value) {
            selection_history_remove_pending_delta(batch, (uint32_t)existing_index);
        }
        return core_result_ok();
    }
    result = selection_history_sample_read(document,
                                           layer_rasters,
                                           batch->layer_id,
                                           sample_x,
                                           sample_y,
                                           &previous_value);
    if (result.code != CORE_OK) {
        return result;
    }
    value = drawing_program_color_normalize_input_sample(value);
    if (previous_value == value) {
        return core_result_ok();
    }
    if (batch->pending_delta_count >= DRAWING_PROGRAM_HISTORY_DELTA_BLOCK_FLUSH_CAPACITY) {
        result = selection_history_flush_pending(document, layer_rasters, history, batch);
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

CoreResult drawing_program_selection_commit_move(DrawingProgramDocument *document,
                                                 DrawingProgramLayerRasterStore *layer_rasters,
                                                 uint32_t active_layer_id,
                                                 DrawingProgramHistory *history,
                                                 DrawingProgramSelectionState *selection) {
    DrawingProgramSelectionHistoryBatch history_batch;
    DrawingProgramRasterSample background = selection_seeded_background_sample();
    uint32_t x;
    uint32_t y;
    int32_t target_origin_x;
    int32_t target_origin_y;
    uint32_t target_layer_id = 0u;
    CoreResult result;
    if (!document || !history || !selection || !selection->has_payload) {
        return core_result_ok();
    }
    target_layer_id = selection_resolve_target_layer_id(document, selection, active_layer_id);
    if (target_layer_id == 0u) {
        drawing_program_selection_reset(selection);
        return core_result_ok();
    }
    {
        int32_t min_offset_x;
        int32_t max_offset_x;
        int32_t min_offset_y;
        int32_t max_offset_y;
        if (selection->width == 0u || selection->height == 0u ||
            document->raster_width == 0u || document->raster_height == 0u) {
            selection->offset_x = 0;
            selection->offset_y = 0;
        } else {
            min_offset_x = -(int32_t)selection->origin_x;
            min_offset_y = -(int32_t)selection->origin_y;
            max_offset_x = (int32_t)document->raster_width - (int32_t)(selection->origin_x + selection->width);
            max_offset_y = (int32_t)document->raster_height - (int32_t)(selection->origin_y + selection->height);
            if (selection->offset_x < min_offset_x) selection->offset_x = min_offset_x;
            if (selection->offset_x > max_offset_x) selection->offset_x = max_offset_x;
            if (selection->offset_y < min_offset_y) selection->offset_y = min_offset_y;
            if (selection->offset_y > max_offset_y) selection->offset_y = max_offset_y;
        }
    }
    if (selection->offset_x == 0 && selection->offset_y == 0) {
        selection->moving = 0u;
        return core_result_ok();
    }
    memset(&history_batch, 0, sizeof(history_batch));
    history_batch.layer_id = target_layer_id;
    for (y = 0u; y < selection->height; ++y) {
        for (x = 0u; x < selection->width; ++x) {
            uint32_t sx = selection->origin_x + x;
            uint32_t sy = selection->origin_y + y;
            if (!drawing_program_selection_mask_at(selection, x, y)) continue;
            if (sx >= document->raster_width || sy >= document->raster_height) continue;
            result = selection_history_record_sample(document,
                                                     layer_rasters,
                                                     history,
                                                     &history_batch,
                                                     sx,
                                                     sy,
                                                     background);
            if (result.code != CORE_OK) {
                (void)selection_history_finish(document, layer_rasters, history, &history_batch);
                return result;
            }
        }
    }
    for (y = 0u; y < selection->height; ++y) {
        for (x = 0u; x < selection->width; ++x) {
            int32_t dx;
            int32_t dy;
            DrawingProgramRasterSample value;
            if (!drawing_program_selection_mask_at(selection, x, y)) continue;
            dx = (int32_t)selection->origin_x + selection->offset_x + (int32_t)x;
            dy = (int32_t)selection->origin_y + selection->offset_y + (int32_t)y;
            if (dx < 0 || dy < 0 ||
                dx >= (int32_t)document->raster_width ||
                dy >= (int32_t)document->raster_height) {
                continue;
            }
            value = drawing_program_selection_value_at(selection, x, y);
            result = selection_history_record_sample(document,
                                                     layer_rasters,
                                                     history,
                                                     &history_batch,
                                                     (uint32_t)dx,
                                                     (uint32_t)dy,
                                                     value);
            if (result.code != CORE_OK) {
                (void)selection_history_finish(document, layer_rasters, history, &history_batch);
                return result;
            }
        }
    }
    result = selection_history_finish(document, layer_rasters, history, &history_batch);
    if (result.code != CORE_OK) {
        return result;
    }
    target_origin_x = (int32_t)selection->origin_x + selection->offset_x;
    target_origin_y = (int32_t)selection->origin_y + selection->offset_y;
    selection->moving = 0u;
    selection->offset_x = 0;
    selection->offset_y = 0;
    if (target_origin_x < 0 || target_origin_y < 0 ||
        target_origin_x >= (int32_t)document->raster_width ||
        target_origin_y >= (int32_t)document->raster_height) {
        drawing_program_selection_reset(selection);
        return core_result_ok();
    }
    selection->origin_x = (uint32_t)target_origin_x;
    selection->origin_y = (uint32_t)target_origin_y;
    selection->layer_id = target_layer_id;
    return core_result_ok();
}

CoreResult drawing_program_selection_delete_payload(DrawingProgramDocument *document,
                                                    DrawingProgramLayerRasterStore *layer_rasters,
                                                    uint32_t active_layer_id,
                                                    DrawingProgramHistory *history,
                                                    DrawingProgramSelectionState *selection) {
    DrawingProgramSelectionHistoryBatch history_batch;
    uint32_t x;
    uint32_t y;
    DrawingProgramRasterSample background = selection_seeded_background_sample();
    uint32_t target_layer_id = 0u;
    CoreResult result;
    if (!document || !history || !selection || !selection->has_payload) {
        return core_result_ok();
    }
    target_layer_id = selection_resolve_target_layer_id(document, selection, active_layer_id);
    if (target_layer_id == 0u) {
        drawing_program_selection_reset(selection);
        return core_result_ok();
    }
    memset(&history_batch, 0, sizeof(history_batch));
    history_batch.layer_id = target_layer_id;
    for (y = 0u; y < selection->height; ++y) {
        for (x = 0u; x < selection->width; ++x) {
            uint32_t sx;
            uint32_t sy;
            if (!drawing_program_selection_mask_at(selection, x, y)) continue;
            sx = selection->origin_x + x;
            sy = selection->origin_y + y;
            if (sx >= document->raster_width || sy >= document->raster_height) continue;
            result = selection_history_record_sample(document,
                                                     layer_rasters,
                                                     history,
                                                     &history_batch,
                                                     sx,
                                                     sy,
                                                     background);
            if (result.code != CORE_OK) {
                (void)selection_history_finish(document, layer_rasters, history, &history_batch);
                return result;
            }
        }
    }
    result = selection_history_finish(document, layer_rasters, history, &history_batch);
    if (result.code != CORE_OK) {
        return result;
    }
    drawing_program_selection_reset(selection);
    return core_result_ok();
}

int drawing_program_selection_copy_payload(const DrawingProgramSelectionState *selection,
                                           DrawingProgramClipboardState *clipboard) {
    if (!selection || !clipboard) {
        return 0;
    }
    drawing_program_clipboard_reset(clipboard);
    if (!selection->has_payload ||
        selection->width == 0u ||
        selection->height == 0u ||
        selection->payload_count == 0u) {
        return 0;
    }
    clipboard->has_payload = 1u;
    clipboard->source_layer_id = selection->layer_id;
    clipboard->width = selection->width;
    clipboard->height = selection->height;
    clipboard->payload_count = selection->payload_count;
    memcpy(clipboard->payload_mask, selection->payload_mask, sizeof(clipboard->payload_mask));
    memcpy(clipboard->payload_value, selection->payload_value, sizeof(clipboard->payload_value));
    return 1;
}

CoreResult drawing_program_selection_cut_to_clipboard(DrawingProgramDocument *document,
                                                      DrawingProgramLayerRasterStore *layer_rasters,
                                                      uint32_t active_layer_id,
                                                      DrawingProgramHistory *history,
                                                      DrawingProgramSelectionState *selection,
                                                      DrawingProgramClipboardState *clipboard) {
    DrawingProgramSelectionHistoryBatch history_batch;
    uint32_t x;
    uint32_t y;
    DrawingProgramRasterSample background;
    CoreResult result;
    uint32_t target_layer_id = 0u;
    if (!document || !history || !selection || !clipboard) {
        return core_result_ok();
    }
    if (!drawing_program_selection_copy_payload(selection, clipboard)) {
        return core_result_ok();
    }
    target_layer_id = selection_resolve_target_layer_id(document, selection, active_layer_id);
    if (target_layer_id == 0u) {
        drawing_program_selection_reset(selection);
        return core_result_ok();
    }
    background = selection_seeded_background_sample();
    memset(&history_batch, 0, sizeof(history_batch));
    history_batch.layer_id = target_layer_id;
    for (y = 0u; y < selection->height; ++y) {
        for (x = 0u; x < selection->width; ++x) {
            uint32_t sx;
            uint32_t sy;
            result = core_result_ok();
            if (!drawing_program_selection_mask_at(selection, x, y)) continue;
            sx = selection->origin_x + x;
            sy = selection->origin_y + y;
            if (sx >= document->raster_width || sy >= document->raster_height) continue;
            result = selection_history_record_sample(document,
                                                     layer_rasters,
                                                     history,
                                                     &history_batch,
                                                     sx,
                                                     sy,
                                                     background);
            if (result.code != CORE_OK) {
                (void)selection_history_finish(document, layer_rasters, history, &history_batch);
                return result;
            }
        }
    }
    result = selection_history_finish(document, layer_rasters, history, &history_batch);
    if (result.code != CORE_OK) {
        return result;
    }
    drawing_program_selection_reset(selection);
    return core_result_ok();
}

CoreResult drawing_program_selection_paste_from_clipboard(DrawingProgramDocument *document,
                                                          DrawingProgramLayerRasterStore *layer_rasters,
                                                          uint32_t active_layer_id,
                                                          DrawingProgramHistory *history,
                                                          DrawingProgramSelectionState *selection,
                                                          const DrawingProgramClipboardState *clipboard,
                                                          int32_t origin_x,
                                                          int32_t origin_y) {
    DrawingProgramSelectionHistoryBatch history_batch;
    uint32_t x;
    uint32_t y;
    CoreResult result;
    uint32_t target_layer_id = 0u;
    if (!document || !history || !selection || !clipboard) {
        return core_result_ok();
    }
    if (!clipboard->has_payload ||
        clipboard->width == 0u ||
        clipboard->height == 0u ||
        clipboard->payload_count == 0u) {
        return core_result_ok();
    }
    target_layer_id = active_layer_id;
    if (target_layer_id == 0u) {
        target_layer_id = clipboard->source_layer_id;
    }
    if (target_layer_id == 0u) {
        return core_result_ok();
    }
    {
        uint32_t ignored_index = 0u;
        if (drawing_program_document_layer_index_for_id(document, target_layer_id, &ignored_index).code != CORE_OK) {
            return core_result_ok();
        }
    }
    memset(&history_batch, 0, sizeof(history_batch));
    history_batch.layer_id = target_layer_id;
    for (y = 0u; y < clipboard->height; ++y) {
        for (x = 0u; x < clipboard->width; ++x) {
            uint32_t index = y * clipboard->width + x;
            int32_t dx = origin_x + (int32_t)x;
            int32_t dy = origin_y + (int32_t)y;
            if (index >= DRAWING_PROGRAM_SELECTION_MAX_AREA || !clipboard->payload_mask[index]) continue;
            if (dx < 0 || dy < 0 ||
                dx >= (int32_t)document->raster_width ||
                dy >= (int32_t)document->raster_height) {
                continue;
            }
            result = selection_history_record_sample(document,
                                                     layer_rasters,
                                                     history,
                                                     &history_batch,
                                                     (uint32_t)dx,
                                                     (uint32_t)dy,
                                                     clipboard->payload_value[index]);
            if (result.code != CORE_OK) {
                (void)selection_history_finish(document, layer_rasters, history, &history_batch);
                return result;
            }
        }
    }
    result = selection_history_finish(document, layer_rasters, history, &history_batch);
    if (result.code != CORE_OK) {
        return result;
    }
    if (!drawing_program_selection_capture_from_rect(document,
                                                     layer_rasters,
                                                     target_layer_id,
                                                     selection,
                                                     origin_x,
                                                     origin_y,
                                                     clipboard->width,
                                                     clipboard->height)) {
        drawing_program_selection_reset(selection);
    }
    return core_result_ok();
}
