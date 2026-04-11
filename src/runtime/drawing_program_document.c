#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_color_model.h"

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
    document->schema_version = 2u;
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

    if (document->raster_sample_count > 0u) {
        memset(document->raster_samples,
               (int)drawing_program_color_eraser_value(),
               (size_t)document->raster_sample_count);
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
