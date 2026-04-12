#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_color_model.h"

#include <stdio.h>
#include <string.h>

static int drawing_program_document_validate_shape(uint32_t logical_width,
                                                   uint32_t logical_height,
                                                   uint32_t sample_density,
                                                   uint32_t *out_raster_width,
                                                   uint32_t *out_raster_height,
                                                   uint32_t *out_raster_sample_count) {
    uint64_t raster_w;
    uint64_t raster_h;
    uint64_t total;
    if (!out_raster_width || !out_raster_height || !out_raster_sample_count) {
        return 0;
    }
    if (logical_width == 0u || logical_height == 0u || sample_density == 0u) {
        return 0;
    }
    raster_w = (uint64_t)logical_width * (uint64_t)sample_density;
    raster_h = (uint64_t)logical_height * (uint64_t)sample_density;
    total = raster_w * raster_h;
    if (raster_w == 0u || raster_h == 0u || total == 0u) {
        return 0;
    }
    if (raster_w > (uint64_t)UINT32_MAX || raster_h > (uint64_t)UINT32_MAX) {
        return 0;
    }
    if (total > (uint64_t)DRAWING_PROGRAM_MAX_RASTER_SAMPLES) {
        return 0;
    }
    *out_raster_width = (uint32_t)raster_w;
    *out_raster_height = (uint32_t)raster_h;
    *out_raster_sample_count = (uint32_t)total;
    return 1;
}

static CoreResult drawing_program_document_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static CoreResult drawing_program_document_find_layer_index_internal(const DrawingProgramDocument *document,
                                                                     uint32_t layer_id,
                                                                     uint32_t *out_layer_index) {
    uint32_t i;
    if (!document || !out_layer_index || layer_id == 0u) {
        return drawing_program_document_invalid("invalid layer index query");
    }
    for (i = 0u; i < document->layer_count; ++i) {
        if (document->layers[i].layer_id == layer_id) {
            *out_layer_index = i;
            return core_result_ok();
        }
    }
    return (CoreResult){ CORE_ERR_NOT_FOUND, "layer not found" };
}

CoreResult drawing_program_document_init_with_shape(DrawingProgramDocument *document,
                                                    uint32_t logical_width,
                                                    uint32_t logical_height,
                                                    uint32_t sample_density) {
    uint32_t raster_width = 0u;
    uint32_t raster_height = 0u;
    uint32_t raster_sample_count = 0u;
    if (!document) {
        return drawing_program_document_invalid("null document");
    }
    if (!drawing_program_document_validate_shape(logical_width,
                                                 logical_height,
                                                 sample_density,
                                                 &raster_width,
                                                 &raster_height,
                                                 &raster_sample_count)) {
        return drawing_program_document_invalid("invalid or oversized document shape");
    }

    memset(document, 0, sizeof(*document));
    document->schema_version = 2u;
    document->logical_width = logical_width;
    document->logical_height = logical_height;
    document->sample_density = sample_density;
    document->raster_width = raster_width;
    document->raster_height = raster_height;
    document->raster_sample_count = raster_sample_count;
    document->next_layer_id = 2u;
    document->layer_count = 1u;

    document->layers[0].layer_id = 1u;
    (void)snprintf(document->layers[0].name,
                   sizeof(document->layers[0].name),
                   "%s",
                   "Base Layer");
    document->layers[0].visible = 1u;
    document->layers[0].locked = 0u;

    if (document->raster_sample_count > 0u) {
        memset(document->raster_samples,
               (int)drawing_program_color_eraser_value(),
               (size_t)document->raster_sample_count);
    }
    return core_result_ok();
}

CoreResult drawing_program_document_init_default(DrawingProgramDocument *document) {
    return drawing_program_document_init_with_shape(document, 512u, 512u, 1u);
}

CoreResult drawing_program_document_set_layer_visibility(DrawingProgramDocument *document,
                                                         uint32_t layer_id,
                                                         uint8_t visible,
                                                         uint8_t *out_previous_visibility) {
    uint32_t index = 0u;
    CoreResult result;
    if (!document || layer_id == 0u) {
        return drawing_program_document_invalid("invalid layer visibility request");
    }
    result = drawing_program_document_find_layer_index_internal(document, layer_id, &index);
    if (result.code != CORE_OK) {
        return result;
    }
    if (out_previous_visibility) {
        *out_previous_visibility = document->layers[index].visible;
    }
    document->layers[index].visible = visible ? 1u : 0u;
    return core_result_ok();
}

CoreResult drawing_program_document_set_layer_locked(DrawingProgramDocument *document,
                                                     uint32_t layer_id,
                                                     uint8_t locked,
                                                     uint8_t *out_previous_locked) {
    uint32_t index = 0u;
    CoreResult result;
    if (!document || layer_id == 0u) {
        return drawing_program_document_invalid("invalid layer lock request");
    }
    result = drawing_program_document_find_layer_index_internal(document, layer_id, &index);
    if (result.code != CORE_OK) {
        return result;
    }
    if (out_previous_locked) {
        *out_previous_locked = document->layers[index].locked;
    }
    document->layers[index].locked = locked ? 1u : 0u;
    return core_result_ok();
}

CoreResult drawing_program_document_layer_index_for_id(const DrawingProgramDocument *document,
                                                       uint32_t layer_id,
                                                       uint32_t *out_layer_index) {
    return drawing_program_document_find_layer_index_internal(document, layer_id, out_layer_index);
}

CoreResult drawing_program_document_add_layer(DrawingProgramDocument *document,
                                              const char *name,
                                              uint32_t *out_layer_id) {
    DrawingProgramLayer *layer = 0;
    uint32_t next_index = 0u;
    uint32_t layer_id = 0u;
    uint32_t probe_count = 0u;
    if (!document) {
        return drawing_program_document_invalid("invalid add layer request");
    }
    if (document->layer_count >= DRAWING_PROGRAM_MAX_LAYERS) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "layer capacity reached" };
    }
    if (document->next_layer_id == 0u) {
        document->next_layer_id = 1u;
    }
    layer_id = document->next_layer_id;
    while (probe_count < DRAWING_PROGRAM_MAX_LAYERS) {
        CoreResult exists;
        uint32_t ignored_index = 0u;
        exists = drawing_program_document_find_layer_index_internal(document, layer_id, &ignored_index);
        if (exists.code == CORE_ERR_NOT_FOUND) {
            break;
        }
        layer_id += 1u;
        if (layer_id == 0u) {
            layer_id = 1u;
        }
        probe_count += 1u;
    }
    if (probe_count >= DRAWING_PROGRAM_MAX_LAYERS) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "unable to allocate unique layer id" };
    }
    next_index = document->layer_count;
    layer = &document->layers[next_index];
    memset(layer, 0, sizeof(*layer));
    layer->layer_id = layer_id;
    if (name && name[0] != '\0') {
        (void)snprintf(layer->name, sizeof(layer->name), "%s", name);
    } else {
        (void)snprintf(layer->name, sizeof(layer->name), "Layer %u", layer_id);
    }
    layer->visible = 1u;
    layer->locked = 0u;
    document->layer_count += 1u;
    document->next_layer_id = layer_id + 1u;
    if (document->next_layer_id == 0u) {
        document->next_layer_id = 1u;
    }
    if (out_layer_id) {
        *out_layer_id = layer_id;
    }
    return core_result_ok();
}

CoreResult drawing_program_document_remove_layer(DrawingProgramDocument *document,
                                                 uint32_t layer_id,
                                                 uint32_t *out_removed_index) {
    uint32_t index = 0u;
    uint32_t i;
    CoreResult result;
    if (!document || layer_id == 0u) {
        return drawing_program_document_invalid("invalid remove layer request");
    }
    if (document->layer_count <= 1u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "cannot remove the last remaining layer" };
    }
    result = drawing_program_document_find_layer_index_internal(document, layer_id, &index);
    if (result.code != CORE_OK) {
        return result;
    }
    if (out_removed_index) {
        *out_removed_index = index;
    }
    for (i = index + 1u; i < document->layer_count; ++i) {
        document->layers[i - 1u] = document->layers[i];
    }
    memset(&document->layers[document->layer_count - 1u], 0, sizeof(document->layers[document->layer_count - 1u]));
    document->layer_count -= 1u;
    return core_result_ok();
}

CoreResult drawing_program_document_move_layer(DrawingProgramDocument *document,
                                               uint32_t layer_id,
                                               int32_t direction,
                                               uint32_t *out_new_index) {
    uint32_t index = 0u;
    uint32_t target = 0u;
    CoreResult result;
    DrawingProgramLayer temp;
    if (!document || layer_id == 0u) {
        return drawing_program_document_invalid("invalid layer move request");
    }
    result = drawing_program_document_find_layer_index_internal(document, layer_id, &index);
    if (result.code != CORE_OK) {
        return result;
    }
    target = index;
    if (direction < 0) {
        if (index > 0u) {
            target = index - 1u;
        }
    } else if (direction > 0) {
        if (index + 1u < document->layer_count) {
            target = index + 1u;
        }
    } else {
        if (out_new_index) {
            *out_new_index = index;
        }
        return core_result_ok();
    }
    if (target != index) {
        temp = document->layers[index];
        document->layers[index] = document->layers[target];
        document->layers[target] = temp;
    }
    if (out_new_index) {
        *out_new_index = target;
    }
    return core_result_ok();
}

CoreResult drawing_program_document_sample_read(const DrawingProgramDocument *document,
                                                uint32_t sample_x,
                                                uint32_t sample_y,
                                                uint8_t *out_value) {
    uint32_t idx;
    if (!document || !out_value) {
        return drawing_program_document_invalid("invalid sample read request");
    }
    if (sample_x >= document->raster_width || sample_y >= document->raster_height) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "sample coordinate out of bounds" };
    }
    idx = (sample_y * document->raster_width) + sample_x;
    if (idx >= document->raster_sample_count) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "sample index out of bounds" };
    }
    *out_value = document->raster_samples[idx];
    return core_result_ok();
}

CoreResult drawing_program_document_sample_write(DrawingProgramDocument *document,
                                                 uint32_t sample_x,
                                                 uint32_t sample_y,
                                                 uint8_t value,
                                                 uint8_t *out_previous_value) {
    uint32_t idx;
    if (!document) {
        return drawing_program_document_invalid("invalid sample write request");
    }
    if (sample_x >= document->raster_width || sample_y >= document->raster_height) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "sample coordinate out of bounds" };
    }
    idx = (sample_y * document->raster_width) + sample_x;
    if (idx >= document->raster_sample_count) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "sample index out of bounds" };
    }
    if (out_previous_value) {
        *out_previous_value = document->raster_samples[idx];
    }
    document->raster_samples[idx] = value;
    return core_result_ok();
}

CoreResult drawing_program_document_upgrade_legacy_checker_seed(DrawingProgramDocument *document,
                                                                uint8_t *out_upgraded) {
    uint32_t x;
    uint32_t y;
    uint64_t sample_total = 0u;
    uint64_t checker_match_count = 0u;
    uint32_t required_match_percent = 75u;
    uint8_t upgraded_by_conversion = 0u;
    uint8_t upgraded = 0u;
    if (!document) {
        return drawing_program_document_invalid("invalid legacy checker upgrade request");
    }
    if (document->schema_version >= 2u) {
        /* Recovery lane: tolerate prior bad schema tagging only for near-perfect checker signature. */
        required_match_percent = 95u;
    }
    if (document->raster_sample_count == 0u || document->raster_width == 0u || document->raster_height == 0u) {
        document->schema_version = 2u;
        if (out_upgraded) {
            *out_upgraded = 0u;
        }
        return core_result_ok();
    }
    for (y = 0u; y < document->raster_height; ++y) {
        for (x = 0u; x < document->raster_width; ++x) {
            uint32_t idx = (y * document->raster_width) + x;
            uint8_t expected = (((x / 16u) + (y / 16u)) & 1u) ? 44u : 24u;
            if (idx >= document->raster_sample_count) {
                break;
            }
            sample_total += 1u;
            if (document->raster_samples[idx] == expected) {
                checker_match_count += 1u;
            }
        }
    }
    if (sample_total > 0u && (checker_match_count * 100u) >= (sample_total * (uint64_t)required_match_percent)) {
        for (y = 0u; y < document->raster_height; ++y) {
            for (x = 0u; x < document->raster_width; ++x) {
                uint32_t idx = (y * document->raster_width) + x;
                uint8_t expected = (((x / 16u) + (y / 16u)) & 1u) ? 44u : 24u;
                if (idx >= document->raster_sample_count) {
                    break;
                }
                if (document->raster_samples[idx] == expected) {
                    document->raster_samples[idx] = drawing_program_color_eraser_value();
                    upgraded_by_conversion = 1u;
                }
            }
        }
    }
    if (upgraded_by_conversion) {
        document->schema_version = 2u;
    }
    upgraded = upgraded_by_conversion;
    if (out_upgraded) {
        *out_upgraded = upgraded;
    }
    return core_result_ok();
}
