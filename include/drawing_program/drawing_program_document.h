#ifndef DRAWING_PROGRAM_DOCUMENT_H
#define DRAWING_PROGRAM_DOCUMENT_H

#include <stdint.h>

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_MAX_LAYERS 16u
#define DRAWING_PROGRAM_LAYER_NAME_CAPACITY 32u
#define DRAWING_PROGRAM_MAX_RASTER_SAMPLES 1048576u

typedef struct DrawingProgramLayer {
    uint32_t layer_id;
    char name[DRAWING_PROGRAM_LAYER_NAME_CAPACITY];
    uint8_t visible;
    uint8_t locked;
} DrawingProgramLayer;

typedef struct DrawingProgramDocument {
    uint32_t schema_version;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    uint32_t layer_count;
    uint32_t next_layer_id;
    uint32_t raster_width;
    uint32_t raster_height;
    uint32_t raster_sample_count;
    DrawingProgramLayer layers[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t raster_samples[DRAWING_PROGRAM_MAX_RASTER_SAMPLES];
} DrawingProgramDocument;

CoreResult drawing_program_document_init_default(DrawingProgramDocument *document);
CoreResult drawing_program_document_set_layer_visibility(DrawingProgramDocument *document,
                                                         uint32_t layer_id,
                                                         uint8_t visible,
                                                         uint8_t *out_previous_visibility);
CoreResult drawing_program_document_set_layer_locked(DrawingProgramDocument *document,
                                                     uint32_t layer_id,
                                                     uint8_t locked,
                                                     uint8_t *out_previous_locked);
CoreResult drawing_program_document_layer_index_for_id(const DrawingProgramDocument *document,
                                                       uint32_t layer_id,
                                                       uint32_t *out_layer_index);
CoreResult drawing_program_document_add_layer(DrawingProgramDocument *document,
                                              const char *name,
                                              uint32_t *out_layer_id);
CoreResult drawing_program_document_remove_layer(DrawingProgramDocument *document,
                                                 uint32_t layer_id,
                                                 uint32_t *out_removed_index);
CoreResult drawing_program_document_move_layer(DrawingProgramDocument *document,
                                               uint32_t layer_id,
                                               int32_t direction,
                                               uint32_t *out_new_index);
CoreResult drawing_program_document_sample_read(const DrawingProgramDocument *document,
                                                uint32_t sample_x,
                                                uint32_t sample_y,
                                                uint8_t *out_value);
CoreResult drawing_program_document_sample_write(DrawingProgramDocument *document,
                                                 uint32_t sample_x,
                                                 uint32_t sample_y,
                                                 uint8_t value,
                                                 uint8_t *out_previous_value);
CoreResult drawing_program_document_upgrade_legacy_checker_seed(DrawingProgramDocument *document,
                                                                uint8_t *out_upgraded);

#ifdef __cplusplus
}
#endif

#endif
