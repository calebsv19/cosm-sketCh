#include "drawing_program/drawing_program_selection.h"

#include <string.h>

#include "drawing_program/drawing_program_color_model.h"

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

CoreResult drawing_program_selection_commit_move(DrawingProgramDocument *document,
                                                 DrawingProgramLayerRasterStore *layer_rasters,
                                                 uint32_t active_layer_id,
                                                 DrawingProgramHistory *history,
                                                 DrawingProgramSelectionState *selection) {
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
    result = drawing_program_history_begin_group(history);
    if (result.code != CORE_OK) {
        return result;
    }
    for (y = 0u; y < selection->height; ++y) {
        for (x = 0u; x < selection->width; ++x) {
            uint32_t sx = selection->origin_x + x;
            uint32_t sy = selection->origin_y + y;
            if (!drawing_program_selection_mask_at(selection, x, y)) continue;
            if (sx >= document->raster_width || sy >= document->raster_height) continue;
            result = drawing_program_history_apply_set_sample_value(history,
                                                                    document,
                                                                    layer_rasters,
                                                                    target_layer_id,
                                                                    sx,
                                                                    sy,
                                                                    background);
            if (result.code != CORE_OK) {
                (void)drawing_program_history_end_group(history);
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
            result = drawing_program_history_apply_set_sample_value(history,
                                                                    document,
                                                                    layer_rasters,
                                                                    target_layer_id,
                                                                    (uint32_t)dx,
                                                                    (uint32_t)dy,
                                                                    value);
            if (result.code != CORE_OK) {
                (void)drawing_program_history_end_group(history);
                return result;
            }
        }
    }
    result = drawing_program_history_end_group(history);
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
    result = drawing_program_history_begin_group(history);
    if (result.code != CORE_OK) {
        return result;
    }
    for (y = 0u; y < selection->height; ++y) {
        for (x = 0u; x < selection->width; ++x) {
            uint32_t sx;
            uint32_t sy;
            if (!drawing_program_selection_mask_at(selection, x, y)) continue;
            sx = selection->origin_x + x;
            sy = selection->origin_y + y;
            if (sx >= document->raster_width || sy >= document->raster_height) continue;
            result = drawing_program_history_apply_set_sample_value(history,
                                                                    document,
                                                                    layer_rasters,
                                                                    target_layer_id,
                                                                    sx,
                                                                    sy,
                                                                    background);
            if (result.code != CORE_OK) {
                (void)drawing_program_history_end_group(history);
                return result;
            }
        }
    }
    result = drawing_program_history_end_group(history);
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
    result = drawing_program_history_begin_group(history);
    if (result.code != CORE_OK) {
        return result;
    }
    for (y = 0u; y < selection->height; ++y) {
        for (x = 0u; x < selection->width; ++x) {
            uint32_t sx;
            uint32_t sy;
            result = core_result_ok();
            if (!drawing_program_selection_mask_at(selection, x, y)) continue;
            sx = selection->origin_x + x;
            sy = selection->origin_y + y;
            if (sx >= document->raster_width || sy >= document->raster_height) continue;
            result = drawing_program_history_apply_set_sample_value(history,
                                                                    document,
                                                                    layer_rasters,
                                                                    target_layer_id,
                                                                    sx,
                                                                    sy,
                                                                    background);
            if (result.code != CORE_OK) {
                (void)drawing_program_history_end_group(history);
                return result;
            }
        }
    }
    result = drawing_program_history_end_group(history);
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
    result = drawing_program_history_begin_group(history);
    if (result.code != CORE_OK) {
        return result;
    }
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
            result = drawing_program_history_apply_set_sample_value(history,
                                                                    document,
                                                                    layer_rasters,
                                                                    target_layer_id,
                                                                    (uint32_t)dx,
                                                                    (uint32_t)dy,
                                                                    clipboard->payload_value[index]);
            if (result.code != CORE_OK) {
                (void)drawing_program_history_end_group(history);
                return result;
            }
        }
    }
    result = drawing_program_history_end_group(history);
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
