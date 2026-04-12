#include "drawing_program/drawing_program_selection.h"

#include <stdlib.h>
#include <string.h>

#include "drawing_program/drawing_program_color_model.h"

static int32_t selection_clamp_i32(int32_t value, int32_t min_value, int32_t max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static uint8_t selection_seeded_background_sample(void) {
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

static CoreResult selection_sample_read_from_active_layer(const DrawingProgramDocument *document,
                                                          const DrawingProgramLayerRasterStore *layer_rasters,
                                                          uint32_t active_layer_id,
                                                          uint32_t sample_x,
                                                          uint32_t sample_y,
                                                          uint8_t *out_value) {
    CoreResult result;
    if (!document || !out_value || active_layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid active-layer selection sample read request" };
    }
    if (layer_rasters &&
        layer_rasters->sample_count == document->raster_sample_count &&
        layer_rasters->raster_width == document->raster_width &&
        layer_rasters->raster_height == document->raster_height) {
        result = drawing_program_layer_raster_store_sample_read(layer_rasters,
                                                                active_layer_id,
                                                                sample_x,
                                                                sample_y,
                                                                out_value);
        if (result.code == CORE_OK) {
            return core_result_ok();
        }
    }
    return drawing_program_document_sample_read(document, sample_x, sample_y, out_value);
}

void drawing_program_selection_reset(DrawingProgramSelectionState *selection) {
    if (!selection) {
        return;
    }
    memset(selection, 0, sizeof(*selection));
}

void drawing_program_selection_cancel_transient(DrawingProgramSelectionState *selection) {
    if (!selection) {
        return;
    }
    selection->selecting = 0u;
    selection->moving = 0u;
    selection->offset_x = 0;
    selection->offset_y = 0;
}

void drawing_program_clipboard_reset(DrawingProgramClipboardState *clipboard) {
    if (!clipboard) {
        return;
    }
    memset(clipboard, 0, sizeof(*clipboard));
}

uint8_t drawing_program_selection_mask_at(const DrawingProgramSelectionState *selection,
                                          uint32_t local_x,
                                          uint32_t local_y) {
    uint32_t index;
    if (!selection || selection->width == 0u || selection->height == 0u) {
        return 0u;
    }
    if (local_x >= selection->width || local_y >= selection->height) {
        return 0u;
    }
    index = local_y * selection->width + local_x;
    if (index >= DRAWING_PROGRAM_SELECTION_MAX_AREA) {
        return 0u;
    }
    return selection->payload_mask[index];
}

uint8_t drawing_program_selection_value_at(const DrawingProgramSelectionState *selection,
                                           uint32_t local_x,
                                           uint32_t local_y) {
    uint32_t index;
    if (!selection || selection->width == 0u || selection->height == 0u) {
        return 0u;
    }
    if (local_x >= selection->width || local_y >= selection->height) {
        return 0u;
    }
    index = local_y * selection->width + local_x;
    if (index >= DRAWING_PROGRAM_SELECTION_MAX_AREA) {
        return 0u;
    }
    return selection->payload_value[index];
}

int drawing_program_selection_capture_from_rect(const DrawingProgramDocument *document,
                                                const DrawingProgramLayerRasterStore *layer_rasters,
                                                uint32_t active_layer_id,
                                                DrawingProgramSelectionState *selection,
                                                int32_t x0,
                                                int32_t y0,
                                                uint32_t width,
                                                uint32_t height) {
    int32_t min_x;
    int32_t min_y;
    int32_t max_x;
    int32_t max_y;
    uint32_t clipped_w;
    uint32_t clipped_h;
    uint32_t x;
    uint32_t y;
    if (!document || !selection || width == 0u || height == 0u) {
        drawing_program_selection_reset(selection);
        return 0;
    }
    min_x = x0;
    min_y = y0;
    max_x = x0 + (int32_t)width - 1;
    max_y = y0 + (int32_t)height - 1;
    if (max_x < 0 || max_y < 0 ||
        min_x >= (int32_t)document->raster_width ||
        min_y >= (int32_t)document->raster_height) {
        drawing_program_selection_reset(selection);
        return 0;
    }
    min_x = selection_clamp_i32(min_x, 0, (int32_t)document->raster_width - 1);
    min_y = selection_clamp_i32(min_y, 0, (int32_t)document->raster_height - 1);
    max_x = selection_clamp_i32(max_x, 0, (int32_t)document->raster_width - 1);
    max_y = selection_clamp_i32(max_y, 0, (int32_t)document->raster_height - 1);
    if (max_x < min_x || max_y < min_y) {
        drawing_program_selection_reset(selection);
        return 0;
    }
    clipped_w = (uint32_t)(max_x - min_x + 1);
    clipped_h = (uint32_t)(max_y - min_y + 1);
    if (clipped_w > DRAWING_PROGRAM_SELECTION_MAX_WIDTH) {
        clipped_w = DRAWING_PROGRAM_SELECTION_MAX_WIDTH;
    }
    if (clipped_h > DRAWING_PROGRAM_SELECTION_MAX_HEIGHT) {
        clipped_h = DRAWING_PROGRAM_SELECTION_MAX_HEIGHT;
    }
    memset(selection->payload_mask, 0, sizeof(selection->payload_mask));
    memset(selection->payload_value, 0, sizeof(selection->payload_value));
    selection->has_payload = 1u;
    selection->selecting = 0u;
    selection->moving = 0u;
    selection->layer_id = active_layer_id;
    selection->origin_x = (uint32_t)min_x;
    selection->origin_y = (uint32_t)min_y;
    selection->width = clipped_w;
    selection->height = clipped_h;
    selection->offset_x = 0;
    selection->offset_y = 0;
    selection->payload_count = 0u;
    for (y = 0u; y < clipped_h; ++y) {
        for (x = 0u; x < clipped_w; ++x) {
            uint32_t index = y * clipped_w + x;
            uint8_t sample = 0u;
            uint32_t sx = (uint32_t)min_x + x;
            uint32_t sy = (uint32_t)min_y + y;
            if (index >= DRAWING_PROGRAM_SELECTION_MAX_AREA) {
                continue;
            }
            if (selection_sample_read_from_active_layer(document,
                                                        layer_rasters,
                                                        active_layer_id,
                                                        sx,
                                                        sy,
                                                        &sample)
                    .code != CORE_OK) {
                continue;
            }
            if (sample == selection_seeded_background_sample()) {
                continue;
            }
            selection->payload_mask[index] = 1u;
            selection->payload_value[index] = sample;
            selection->payload_count += 1u;
        }
    }
    if (selection->payload_count == 0u) {
        drawing_program_selection_reset(selection);
        return 0;
    }
    return 1;
}

int drawing_program_selection_add_from_rect(const DrawingProgramDocument *document,
                                            const DrawingProgramLayerRasterStore *layer_rasters,
                                            uint32_t active_layer_id,
                                            DrawingProgramSelectionState *selection,
                                            int32_t x0,
                                            int32_t y0,
                                            uint32_t width,
                                            uint32_t height) {
    DrawingProgramSelectionState incoming;
    uint32_t min_x;
    uint32_t min_y;
    uint32_t max_x;
    uint32_t max_y;
    uint32_t merged_w;
    uint32_t merged_h;
    uint32_t merged_area;
    uint8_t *merged_mask = 0;
    uint8_t *merged_value = 0;
    uint32_t merged_payload_count = 0u;
    uint32_t x;
    uint32_t y;
    if (!document || !selection || width == 0u || height == 0u) {
        return 0;
    }
    drawing_program_selection_reset(&incoming);
    if (!drawing_program_selection_capture_from_rect(document,
                                                     layer_rasters,
                                                     active_layer_id,
                                                     &incoming,
                                                     x0,
                                                     y0,
                                                     width,
                                                     height)) {
        /* additive-empty capture should preserve current payload */
        if (selection->has_payload) {
            selection->selecting = 0u;
            selection->moving = 0u;
            selection->offset_x = 0;
            selection->offset_y = 0;
            return 1;
        }
        drawing_program_selection_reset(selection);
        return 0;
    }
    if (!selection->has_payload ||
        selection->payload_count == 0u ||
        selection->width == 0u ||
        selection->height == 0u ||
        selection->layer_id != incoming.layer_id) {
        *selection = incoming;
        selection->selecting = 0u;
        selection->moving = 0u;
        selection->offset_x = 0;
        selection->offset_y = 0;
        return 1;
    }

    min_x = (selection->origin_x < incoming.origin_x) ? selection->origin_x : incoming.origin_x;
    min_y = (selection->origin_y < incoming.origin_y) ? selection->origin_y : incoming.origin_y;
    max_x = ((selection->origin_x + selection->width) > (incoming.origin_x + incoming.width))
                ? (selection->origin_x + selection->width - 1u)
                : (incoming.origin_x + incoming.width - 1u);
    max_y = ((selection->origin_y + selection->height) > (incoming.origin_y + incoming.height))
                ? (selection->origin_y + selection->height - 1u)
                : (incoming.origin_y + incoming.height - 1u);
    merged_w = max_x - min_x + 1u;
    merged_h = max_y - min_y + 1u;
    if (merged_w > DRAWING_PROGRAM_SELECTION_MAX_WIDTH ||
        merged_h > DRAWING_PROGRAM_SELECTION_MAX_HEIGHT) {
        /* preserve prior selection when requested union exceeds bounded v1 capacity */
        selection->selecting = 0u;
        selection->moving = 0u;
        selection->offset_x = 0;
        selection->offset_y = 0;
        return 0;
    }
    merged_area = merged_w * merged_h;
    if (merged_area == 0u || merged_area > DRAWING_PROGRAM_SELECTION_MAX_AREA) {
        selection->selecting = 0u;
        selection->moving = 0u;
        selection->offset_x = 0;
        selection->offset_y = 0;
        return 0;
    }
    merged_mask = (uint8_t *)calloc(merged_area, sizeof(uint8_t));
    merged_value = (uint8_t *)calloc(merged_area, sizeof(uint8_t));
    if (!merged_mask || !merged_value) {
        free(merged_mask);
        free(merged_value);
        selection->selecting = 0u;
        selection->moving = 0u;
        selection->offset_x = 0;
        selection->offset_y = 0;
        return 0;
    }

    for (y = 0u; y < selection->height; ++y) {
        for (x = 0u; x < selection->width; ++x) {
            uint32_t src_index = y * selection->width + x;
            uint32_t dx;
            uint32_t dy;
            uint32_t dst_index;
            if (src_index >= DRAWING_PROGRAM_SELECTION_MAX_AREA || !selection->payload_mask[src_index]) {
                continue;
            }
            dx = selection->origin_x + x - min_x;
            dy = selection->origin_y + y - min_y;
            dst_index = dy * merged_w + dx;
            if (dst_index >= merged_area) {
                continue;
            }
            if (!merged_mask[dst_index]) {
                merged_mask[dst_index] = 1u;
                merged_payload_count += 1u;
            }
            merged_value[dst_index] = selection->payload_value[src_index];
        }
    }
    for (y = 0u; y < incoming.height; ++y) {
        for (x = 0u; x < incoming.width; ++x) {
            uint32_t src_index = y * incoming.width + x;
            uint32_t dx;
            uint32_t dy;
            uint32_t dst_index;
            if (src_index >= DRAWING_PROGRAM_SELECTION_MAX_AREA || !incoming.payload_mask[src_index]) {
                continue;
            }
            dx = incoming.origin_x + x - min_x;
            dy = incoming.origin_y + y - min_y;
            dst_index = dy * merged_w + dx;
            if (dst_index >= merged_area) {
                continue;
            }
            if (!merged_mask[dst_index]) {
                merged_mask[dst_index] = 1u;
                merged_payload_count += 1u;
            }
            /* incoming marquee region overwrites value when both overlap */
            merged_value[dst_index] = incoming.payload_value[src_index];
        }
    }

    memset(selection->payload_mask, 0, sizeof(selection->payload_mask));
    memset(selection->payload_value, 0, sizeof(selection->payload_value));
    memcpy(selection->payload_mask, merged_mask, merged_area);
    memcpy(selection->payload_value, merged_value, merged_area);
    selection->has_payload = (merged_payload_count > 0u) ? 1u : 0u;
    selection->selecting = 0u;
    selection->moving = 0u;
    selection->layer_id = incoming.layer_id;
    selection->origin_x = min_x;
    selection->origin_y = min_y;
    selection->width = merged_w;
    selection->height = merged_h;
    selection->offset_x = 0;
    selection->offset_y = 0;
    selection->payload_count = merged_payload_count;

    free(merged_mask);
    free(merged_value);
    if (selection->payload_count == 0u) {
        drawing_program_selection_reset(selection);
        return 0;
    }
    return 1;
}

int drawing_program_selection_subtract_from_rect(const DrawingProgramDocument *document,
                                                 const DrawingProgramLayerRasterStore *layer_rasters,
                                                 uint32_t active_layer_id,
                                                 DrawingProgramSelectionState *selection,
                                                 int32_t x0,
                                                 int32_t y0,
                                                 uint32_t width,
                                                 uint32_t height) {
    DrawingProgramSelectionState incoming;
    uint32_t x;
    uint32_t y;
    uint32_t min_local_x = 0u;
    uint32_t min_local_y = 0u;
    uint32_t max_local_x = 0u;
    uint32_t max_local_y = 0u;
    uint8_t found = 0u;
    uint32_t new_w;
    uint32_t new_h;
    uint32_t new_area;
    uint8_t *new_mask = 0;
    uint8_t *new_value = 0;
    uint32_t new_payload_count = 0u;
    if (!document || !selection || width == 0u || height == 0u) {
        return 0;
    }
    if (!selection->has_payload ||
        selection->payload_count == 0u ||
        selection->width == 0u ||
        selection->height == 0u) {
        drawing_program_selection_reset(selection);
        return 0;
    }
    drawing_program_selection_reset(&incoming);
    if (!drawing_program_selection_capture_from_rect(document,
                                                     layer_rasters,
                                                     active_layer_id,
                                                     &incoming,
                                                     x0,
                                                     y0,
                                                     width,
                                                     height)) {
        selection->selecting = 0u;
        selection->moving = 0u;
        selection->offset_x = 0;
        selection->offset_y = 0;
        return 1;
    }
    if (incoming.layer_id != selection->layer_id) {
        selection->selecting = 0u;
        selection->moving = 0u;
        selection->offset_x = 0;
        selection->offset_y = 0;
        return 1;
    }
    for (y = 0u; y < selection->height; ++y) {
        for (x = 0u; x < selection->width; ++x) {
            uint32_t src_index = y * selection->width + x;
            uint32_t global_x = selection->origin_x + x;
            uint32_t global_y = selection->origin_y + y;
            if (src_index >= DRAWING_PROGRAM_SELECTION_MAX_AREA || !selection->payload_mask[src_index]) {
                continue;
            }
            if (global_x >= incoming.origin_x &&
                global_x < (incoming.origin_x + incoming.width) &&
                global_y >= incoming.origin_y &&
                global_y < (incoming.origin_y + incoming.height)) {
                uint32_t incoming_local_x = global_x - incoming.origin_x;
                uint32_t incoming_local_y = global_y - incoming.origin_y;
                uint32_t incoming_index = incoming_local_y * incoming.width + incoming_local_x;
                if (incoming_index < DRAWING_PROGRAM_SELECTION_MAX_AREA &&
                    incoming.payload_mask[incoming_index]) {
                    continue;
                }
            }
            if (!found) {
                min_local_x = x;
                min_local_y = y;
                max_local_x = x;
                max_local_y = y;
                found = 1u;
            } else {
                if (x < min_local_x) {
                    min_local_x = x;
                }
                if (y < min_local_y) {
                    min_local_y = y;
                }
                if (x > max_local_x) {
                    max_local_x = x;
                }
                if (y > max_local_y) {
                    max_local_y = y;
                }
            }
        }
    }
    if (!found) {
        drawing_program_selection_reset(selection);
        return 0;
    }
    new_w = max_local_x - min_local_x + 1u;
    new_h = max_local_y - min_local_y + 1u;
    new_area = new_w * new_h;
    if (new_w == 0u || new_h == 0u || new_area > DRAWING_PROGRAM_SELECTION_MAX_AREA) {
        drawing_program_selection_reset(selection);
        return 0;
    }
    new_mask = (uint8_t *)calloc(new_area, sizeof(uint8_t));
    new_value = (uint8_t *)calloc(new_area, sizeof(uint8_t));
    if (!new_mask || !new_value) {
        free(new_mask);
        free(new_value);
        return 0;
    }
    for (y = min_local_y; y <= max_local_y; ++y) {
        for (x = min_local_x; x <= max_local_x; ++x) {
            uint32_t src_index = y * selection->width + x;
            uint32_t global_x = selection->origin_x + x;
            uint32_t global_y = selection->origin_y + y;
            uint32_t dst_x = x - min_local_x;
            uint32_t dst_y = y - min_local_y;
            uint32_t dst_index = dst_y * new_w + dst_x;
            if (src_index >= DRAWING_PROGRAM_SELECTION_MAX_AREA || !selection->payload_mask[src_index]) {
                continue;
            }
            if (global_x >= incoming.origin_x &&
                global_x < (incoming.origin_x + incoming.width) &&
                global_y >= incoming.origin_y &&
                global_y < (incoming.origin_y + incoming.height)) {
                uint32_t incoming_local_x = global_x - incoming.origin_x;
                uint32_t incoming_local_y = global_y - incoming.origin_y;
                uint32_t incoming_index = incoming_local_y * incoming.width + incoming_local_x;
                if (incoming_index < DRAWING_PROGRAM_SELECTION_MAX_AREA &&
                    incoming.payload_mask[incoming_index]) {
                    continue;
                }
            }
            if (dst_index >= new_area) {
                continue;
            }
            new_mask[dst_index] = 1u;
            new_value[dst_index] = selection->payload_value[src_index];
            new_payload_count += 1u;
        }
    }
    if (new_payload_count == 0u) {
        free(new_mask);
        free(new_value);
        drawing_program_selection_reset(selection);
        return 0;
    }
    memset(selection->payload_mask, 0, sizeof(selection->payload_mask));
    memset(selection->payload_value, 0, sizeof(selection->payload_value));
    memcpy(selection->payload_mask, new_mask, new_area);
    memcpy(selection->payload_value, new_value, new_area);
    selection->origin_x += min_local_x;
    selection->origin_y += min_local_y;
    selection->width = new_w;
    selection->height = new_h;
    selection->payload_count = new_payload_count;
    selection->has_payload = 1u;
    selection->selecting = 0u;
    selection->moving = 0u;
    selection->offset_x = 0;
    selection->offset_y = 0;
    free(new_mask);
    free(new_value);
    return 1;
}

int drawing_program_selection_capture_from_marquee(const DrawingProgramDocument *document,
                                                   const DrawingProgramLayerRasterStore *layer_rasters,
                                                   uint32_t active_layer_id,
                                                   DrawingProgramSelectionState *selection) {
    uint32_t min_x;
    uint32_t min_y;
    uint32_t max_x;
    uint32_t max_y;
    uint32_t width;
    uint32_t height;
    if (!document || !selection || !selection->selecting) {
        return 0;
    }
    min_x = (selection->marquee_start_x < selection->marquee_end_x)
                ? selection->marquee_start_x
                : selection->marquee_end_x;
    min_y = (selection->marquee_start_y < selection->marquee_end_y)
                ? selection->marquee_start_y
                : selection->marquee_end_y;
    max_x = (selection->marquee_start_x > selection->marquee_end_x)
                ? selection->marquee_start_x
                : selection->marquee_end_x;
    max_y = (selection->marquee_start_y > selection->marquee_end_y)
                ? selection->marquee_start_y
                : selection->marquee_end_y;
    width = max_x - min_x + 1u;
    height = max_y - min_y + 1u;
    return drawing_program_selection_capture_from_rect(document,
                                                       layer_rasters,
                                                       active_layer_id,
                                                       selection,
                                                       (int32_t)min_x,
                                                       (int32_t)min_y,
                                                       width,
                                                       height);
}

int drawing_program_selection_add_from_marquee(const DrawingProgramDocument *document,
                                               const DrawingProgramLayerRasterStore *layer_rasters,
                                               uint32_t active_layer_id,
                                               DrawingProgramSelectionState *selection) {
    uint32_t min_x;
    uint32_t min_y;
    uint32_t max_x;
    uint32_t max_y;
    uint32_t width;
    uint32_t height;
    if (!document || !selection || !selection->selecting) {
        return 0;
    }
    min_x = (selection->marquee_start_x < selection->marquee_end_x)
                ? selection->marquee_start_x
                : selection->marquee_end_x;
    min_y = (selection->marquee_start_y < selection->marquee_end_y)
                ? selection->marquee_start_y
                : selection->marquee_end_y;
    max_x = (selection->marquee_start_x > selection->marquee_end_x)
                ? selection->marquee_start_x
                : selection->marquee_end_x;
    max_y = (selection->marquee_start_y > selection->marquee_end_y)
                ? selection->marquee_start_y
                : selection->marquee_end_y;
    width = max_x - min_x + 1u;
    height = max_y - min_y + 1u;
    return drawing_program_selection_add_from_rect(document,
                                                   layer_rasters,
                                                   active_layer_id,
                                                   selection,
                                                   (int32_t)min_x,
                                                   (int32_t)min_y,
                                                   width,
                                                   height);
}

int drawing_program_selection_subtract_from_marquee(const DrawingProgramDocument *document,
                                                    const DrawingProgramLayerRasterStore *layer_rasters,
                                                    uint32_t active_layer_id,
                                                    DrawingProgramSelectionState *selection) {
    uint32_t min_x;
    uint32_t min_y;
    uint32_t max_x;
    uint32_t max_y;
    uint32_t width;
    uint32_t height;
    if (!document || !selection || !selection->selecting) {
        return 0;
    }
    min_x = (selection->marquee_start_x < selection->marquee_end_x)
                ? selection->marquee_start_x
                : selection->marquee_end_x;
    min_y = (selection->marquee_start_y < selection->marquee_end_y)
                ? selection->marquee_start_y
                : selection->marquee_end_y;
    max_x = (selection->marquee_start_x > selection->marquee_end_x)
                ? selection->marquee_start_x
                : selection->marquee_end_x;
    max_y = (selection->marquee_start_y > selection->marquee_end_y)
                ? selection->marquee_start_y
                : selection->marquee_end_y;
    width = max_x - min_x + 1u;
    height = max_y - min_y + 1u;
    return drawing_program_selection_subtract_from_rect(document,
                                                        layer_rasters,
                                                        active_layer_id,
                                                        selection,
                                                        (int32_t)min_x,
                                                        (int32_t)min_y,
                                                        width,
                                                        height);
}

int drawing_program_selection_contains_sample(const DrawingProgramSelectionState *selection,
                                              uint32_t sample_x,
                                              uint32_t sample_y) {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
    int32_t local_x;
    int32_t local_y;
    if (!selection || !selection->has_payload || selection->width == 0u || selection->height == 0u) {
        return 0;
    }
    left = (int32_t)selection->origin_x + selection->offset_x;
    top = (int32_t)selection->origin_y + selection->offset_y;
    right = left + (int32_t)selection->width;
    bottom = top + (int32_t)selection->height;
    if (!((int32_t)sample_x >= left &&
          (int32_t)sample_y >= top &&
          (int32_t)sample_x < right &&
          (int32_t)sample_y < bottom)) {
        return 0;
    }
    local_x = (int32_t)sample_x - left;
    local_y = (int32_t)sample_y - top;
    if (local_x < 0 || local_y < 0) {
        return 0;
    }
    if ((uint32_t)local_x >= selection->width || (uint32_t)local_y >= selection->height) {
        return 0;
    }
    return drawing_program_selection_mask_at(selection, (uint32_t)local_x, (uint32_t)local_y) ? 1 : 0;
}

int drawing_program_selection_begin_move(const DrawingProgramSelectionState *selection,
                                         uint32_t sample_x,
                                         uint32_t sample_y) {
    return drawing_program_selection_contains_sample(selection, sample_x, sample_y);
}

void drawing_program_selection_begin_marquee(DrawingProgramSelectionState *selection,
                                             uint32_t sample_x,
                                             uint32_t sample_y) {
    if (!selection) {
        return;
    }
    selection->selecting = 1u;
    selection->moving = 0u;
    selection->marquee_start_x = sample_x;
    selection->marquee_start_y = sample_y;
    selection->marquee_end_x = sample_x;
    selection->marquee_end_y = sample_y;
}

void drawing_program_selection_begin_move_tracking(DrawingProgramSelectionState *selection,
                                                   uint32_t sample_x,
                                                   uint32_t sample_y) {
    if (!selection) {
        return;
    }
    selection->moving = 1u;
    selection->selecting = 0u;
    selection->move_anchor_sample_x = sample_x;
    selection->move_anchor_sample_y = sample_y;
    selection->move_anchor_offset_x = selection->offset_x;
    selection->move_anchor_offset_y = selection->offset_y;
}

void drawing_program_selection_update_move_offset(DrawingProgramSelectionState *selection,
                                                  uint32_t sample_x,
                                                  uint32_t sample_y) {
    int32_t dx;
    int32_t dy;
    if (!selection) {
        return;
    }
    dx = (int32_t)sample_x - (int32_t)selection->move_anchor_sample_x;
    dy = (int32_t)sample_y - (int32_t)selection->move_anchor_sample_y;
    selection->offset_x = selection->move_anchor_offset_x + dx;
    selection->offset_y = selection->move_anchor_offset_y + dy;
}

CoreResult drawing_program_selection_commit_move(DrawingProgramDocument *document,
                                                 DrawingProgramLayerRasterStore *layer_rasters,
                                                 uint32_t active_layer_id,
                                                 DrawingProgramHistory *history,
                                                 DrawingProgramSelectionState *selection) {
    uint8_t background = selection_seeded_background_sample();
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
            if (!drawing_program_selection_mask_at(selection, x, y)) {
                continue;
            }
            if (sx >= document->raster_width || sy >= document->raster_height) {
                continue;
            }
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
            uint8_t value;
            if (!drawing_program_selection_mask_at(selection, x, y)) {
                continue;
            }
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
    if (!drawing_program_selection_capture_from_rect(document,
                                                     layer_rasters,
                                                     target_layer_id,
                                                     selection,
                                                     target_origin_x,
                                                     target_origin_y,
                                                     selection->width,
                                                     selection->height)) {
        drawing_program_selection_reset(selection);
    }
    return core_result_ok();
}

CoreResult drawing_program_selection_delete_payload(DrawingProgramDocument *document,
                                                    DrawingProgramLayerRasterStore *layer_rasters,
                                                    uint32_t active_layer_id,
                                                    DrawingProgramHistory *history,
                                                    DrawingProgramSelectionState *selection) {
    uint32_t x;
    uint32_t y;
    uint8_t background = selection_seeded_background_sample();
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
            if (!drawing_program_selection_mask_at(selection, x, y)) {
                continue;
            }
            sx = selection->origin_x + x;
            sy = selection->origin_y + y;
            if (sx >= document->raster_width || sy >= document->raster_height) {
                continue;
            }
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
    uint8_t background;
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
            if (!drawing_program_selection_mask_at(selection, x, y)) {
                continue;
            }
            sx = selection->origin_x + x;
            sy = selection->origin_y + y;
            if (sx >= document->raster_width || sy >= document->raster_height) {
                continue;
            }
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
            if (index >= DRAWING_PROGRAM_SELECTION_MAX_AREA || !clipboard->payload_mask[index]) {
                continue;
            }
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

int drawing_program_selection_select_all(const DrawingProgramDocument *document,
                                         const DrawingProgramLayerRasterStore *layer_rasters,
                                         uint32_t active_layer_id,
                                         DrawingProgramSelectionState *selection) {
    uint32_t x;
    uint32_t y;
    uint32_t min_x = 0u;
    uint32_t min_y = 0u;
    uint32_t max_x = 0u;
    uint32_t max_y = 0u;
    uint8_t found = 0u;
    uint8_t background;
    if (!document || !selection || document->raster_width == 0u || document->raster_height == 0u) {
        drawing_program_selection_reset(selection);
        return 0;
    }
    background = selection_seeded_background_sample();
    for (y = 0u; y < document->raster_height; ++y) {
        for (x = 0u; x < document->raster_width; ++x) {
            uint8_t sample = background;
            if (selection_sample_read_from_active_layer(document,
                                                        layer_rasters,
                                                        active_layer_id,
                                                        x,
                                                        y,
                                                        &sample)
                    .code != CORE_OK) {
                continue;
            }
            if (sample == background) {
                continue;
            }
            if (!found) {
                min_x = x;
                min_y = y;
                max_x = x;
                max_y = y;
                found = 1u;
                continue;
            }
            if (x < min_x) {
                min_x = x;
            }
            if (y < min_y) {
                min_y = y;
            }
            if (x > max_x) {
                max_x = x;
            }
            if (y > max_y) {
                max_y = y;
            }
        }
    }
    if (!found) {
        drawing_program_selection_reset(selection);
        return 0;
    }
    return drawing_program_selection_capture_from_rect(document,
                                                       layer_rasters,
                                                       active_layer_id,
                                                       selection,
                                                       (int32_t)min_x,
                                                       (int32_t)min_y,
                                                       max_x - min_x + 1u,
                                                       max_y - min_y + 1u);
}

const char *drawing_program_selection_scope_policy_label(void) {
    return "ACTIVE_LAYER_ONLY";
}
