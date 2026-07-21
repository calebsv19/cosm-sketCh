#ifndef DRAWING_PROGRAM_INDEXED_LAYER_RASTER_H
#define DRAWING_PROGRAM_INDEXED_LAYER_RASTER_H

#include <stdint.h>

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DrawingProgramIndexedLayerRaster {
    uint32_t width;
    uint32_t height;
    uint32_t index_count;
    uint32_t slot_count;
    uint8_t *indices;
} DrawingProgramIndexedLayerRaster;

void drawing_program_indexed_layer_raster_dispose(
    DrawingProgramIndexedLayerRaster *raster);
CoreResult drawing_program_indexed_layer_raster_init(
    DrawingProgramIndexedLayerRaster *raster,
    uint32_t width,
    uint32_t height,
    uint32_t slot_count,
    uint8_t fill_index);
CoreResult drawing_program_indexed_layer_raster_import(
    DrawingProgramIndexedLayerRaster *raster,
    uint32_t width,
    uint32_t height,
    uint32_t slot_count,
    const uint8_t *indices,
    uint32_t index_count);
CoreResult drawing_program_indexed_layer_raster_clone(
    DrawingProgramIndexedLayerRaster *out_raster,
    const DrawingProgramIndexedLayerRaster *source);
CoreResult drawing_program_indexed_layer_raster_validate(
    const DrawingProgramIndexedLayerRaster *raster);
CoreResult drawing_program_indexed_layer_raster_read(
    const DrawingProgramIndexedLayerRaster *raster,
    uint32_t x,
    uint32_t y,
    uint8_t *out_index);
CoreResult drawing_program_indexed_layer_raster_write(
    DrawingProgramIndexedLayerRaster *raster,
    uint32_t x,
    uint32_t y,
    uint8_t index);

#ifdef __cplusplus
}
#endif

#endif
