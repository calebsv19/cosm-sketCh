#ifndef DRAWING_PROGRAM_LAYER_RASTER_H
#define DRAWING_PROGRAM_LAYER_RASTER_H

#include <stdint.h>

#include "core_base.h"
#include "drawing_program/drawing_program_document.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DrawingProgramLayerRasterStore {
    uint32_t raster_width;
    uint32_t raster_height;
    uint32_t sample_count;
    uint32_t slot_capacity;
    uint32_t slot_count;
    uint32_t slot_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t *slot_samples;
} DrawingProgramLayerRasterStore;

CoreResult drawing_program_layer_raster_store_init_from_document(
    DrawingProgramLayerRasterStore *store,
    const DrawingProgramDocument *document);
void drawing_program_layer_raster_store_dispose(DrawingProgramLayerRasterStore *store);
CoreResult drawing_program_layer_raster_store_sync_document_layers(
    DrawingProgramLayerRasterStore *store,
    const DrawingProgramDocument *document);
CoreResult drawing_program_layer_raster_store_seed_from_legacy_surface(
    DrawingProgramLayerRasterStore *store,
    const DrawingProgramDocument *document);
CoreResult drawing_program_layer_raster_store_sample_read(
    const DrawingProgramLayerRasterStore *store,
    uint32_t layer_id,
    uint32_t sample_x,
    uint32_t sample_y,
    uint8_t *out_value);
CoreResult drawing_program_layer_raster_store_sample_write(
    DrawingProgramLayerRasterStore *store,
    uint32_t layer_id,
    uint32_t sample_x,
    uint32_t sample_y,
    uint8_t value,
    uint8_t *out_previous_value);
CoreResult drawing_program_layer_raster_store_export_layer(
    const DrawingProgramLayerRasterStore *store,
    uint32_t layer_id,
    const uint8_t **out_samples,
    uint32_t *out_sample_count);
CoreResult drawing_program_layer_raster_store_import_layer(
    DrawingProgramLayerRasterStore *store,
    uint32_t layer_id,
    const uint8_t *samples,
    uint32_t sample_count);

#ifdef __cplusplus
}
#endif

#endif
