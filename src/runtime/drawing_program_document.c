#include "drawing_program/drawing_program_document.h"

#include <stdio.h>
#include <string.h>

static uint32_t drawing_program_document_raster_count(uint32_t logical_width,
                                                      uint32_t logical_height,
                                                      uint32_t sample_density) {
    uint64_t raster_w = (uint64_t)logical_width * (uint64_t)sample_density;
    uint64_t raster_h = (uint64_t)logical_height * (uint64_t)sample_density;
    uint64_t total = raster_w * raster_h;
    if (total > (uint64_t)DRAWING_PROGRAM_MAX_RASTER_SAMPLES) {
        return DRAWING_PROGRAM_MAX_RASTER_SAMPLES;
    }
    return (uint32_t)total;
}

static CoreResult drawing_program_document_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

CoreResult drawing_program_document_init_default(DrawingProgramDocument *document) {
    if (!document) {
        return drawing_program_document_invalid("null document");
    }

    memset(document, 0, sizeof(*document));
    document->schema_version = 1u;
    document->logical_width = 512u;
    document->logical_height = 512u;
    document->sample_density = 1u;
    document->raster_width = document->logical_width * document->sample_density;
    document->raster_height = document->logical_height * document->sample_density;
    document->raster_sample_count = drawing_program_document_raster_count(document->logical_width,
                                                                          document->logical_height,
                                                                          document->sample_density);
    document->next_layer_id = 2u;
    document->layer_count = 1u;

    document->layers[0].layer_id = 1u;
    (void)snprintf(document->layers[0].name,
                   sizeof(document->layers[0].name),
                   "%s",
                   "Base Layer");
    document->layers[0].visible = 1u;
    document->layers[0].locked = 0u;

    if (document->raster_width > 0u && document->raster_height > 0u && document->raster_sample_count > 0u) {
        uint32_t x;
        uint32_t y;
        for (y = 0u; y < document->raster_height; ++y) {
            for (x = 0u; x < document->raster_width; ++x) {
                uint32_t idx = (y * document->raster_width) + x;
                uint8_t base = ((x / 16u + y / 16u) & 1u) ? 44u : 24u;
                if (idx >= document->raster_sample_count) {
                    break;
                }
                if (x == y || x + y == document->raster_width - 1u) {
                    base = 180u;
                }
                if (x > document->raster_width / 2u - 2u && x < document->raster_width / 2u + 2u) {
                    base = 220u;
                }
                if (y > document->raster_height / 2u - 2u && y < document->raster_height / 2u + 2u) {
                    base = 220u;
                }
                document->raster_samples[idx] = base;
            }
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_document_set_layer_visibility(DrawingProgramDocument *document,
                                                         uint32_t layer_id,
                                                         uint8_t visible,
                                                         uint8_t *out_previous_visibility) {
    uint32_t i;
    if (!document || layer_id == 0u) {
        return drawing_program_document_invalid("invalid layer visibility request");
    }
    for (i = 0u; i < document->layer_count; ++i) {
        if (document->layers[i].layer_id == layer_id) {
            if (out_previous_visibility) {
                *out_previous_visibility = document->layers[i].visible;
            }
            document->layers[i].visible = visible ? 1u : 0u;
            return core_result_ok();
        }
    }
    return (CoreResult){ CORE_ERR_NOT_FOUND, "layer not found" };
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
