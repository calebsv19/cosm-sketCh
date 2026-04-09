#ifndef DRAWING_PROGRAM_DOCUMENT_H
#define DRAWING_PROGRAM_DOCUMENT_H

#include <stdint.h>

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_MAX_LAYERS 16u
#define DRAWING_PROGRAM_LAYER_NAME_CAPACITY 32u

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
    DrawingProgramLayer layers[DRAWING_PROGRAM_MAX_LAYERS];
} DrawingProgramDocument;

CoreResult drawing_program_document_init_default(DrawingProgramDocument *document);
CoreResult drawing_program_document_set_layer_visibility(DrawingProgramDocument *document,
                                                         uint32_t layer_id,
                                                         uint8_t visible,
                                                         uint8_t *out_previous_visibility);

#ifdef __cplusplus
}
#endif

#endif
