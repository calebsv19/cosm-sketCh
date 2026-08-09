#ifndef DRAWING_PROGRAM_INDEXED_TILE_CANVASES_H
#define DRAWING_PROGRAM_INDEXED_TILE_CANVASES_H

#include <stdint.h>

#include "core_base.h"
#include "drawing_program/drawing_program_indexed_cells.h"
#include "drawing_program/drawing_program_indexed_layer_raster.h"
#include "drawing_program/drawing_program_indexed_tileset_profile.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_INDEXED_TILE_CANVAS_PIXEL_CAPACITY 256u

/* A tile canvas is the authoring owner for exactly one fixed logical tile. */
typedef struct DrawingProgramIndexedTileCanvas {
    uint32_t width;
    uint32_t height;
    uint8_t indices[DRAWING_PROGRAM_INDEXED_TILE_CANVAS_PIXEL_CAPACITY];
} DrawingProgramIndexedTileCanvas;

typedef struct DrawingProgramIndexedTileCanvasTable {
    uint32_t count;
    uint32_t slot_count;
    DrawingProgramIndexedTileCanvas canvases[DRAWING_PROGRAM_INDEXED_CELL_CAPACITY];
} DrawingProgramIndexedTileCanvasTable;

void drawing_program_indexed_tile_canvas_table_clear(
    DrawingProgramIndexedTileCanvasTable *table);
CoreResult drawing_program_indexed_tile_canvas_table_validate(
    const DrawingProgramIndexedTileCanvasTable *canvases,
    const DrawingProgramIndexedCellTable *cells,
    const DrawingProgramIndexedTilesetProfile *profile);
CoreResult drawing_program_indexed_tile_canvas_table_slice_atlas(
    DrawingProgramIndexedTileCanvasTable *out_canvases,
    const DrawingProgramIndexedCellTable *cells,
    const DrawingProgramIndexedTilesetProfile *profile,
    const DrawingProgramIndexedLayerRaster *atlas);
CoreResult drawing_program_indexed_tile_canvas_table_compose_atlas(
    const DrawingProgramIndexedTileCanvasTable *canvases,
    const DrawingProgramIndexedCellTable *cells,
    const DrawingProgramIndexedTilesetProfile *profile,
    DrawingProgramIndexedLayerRaster *out_atlas);
CoreResult drawing_program_indexed_tile_canvas_read(
    const DrawingProgramIndexedTileCanvasTable *canvases,
    uint32_t cell_index,
    uint32_t x,
    uint32_t y,
    uint8_t *out_index);
CoreResult drawing_program_indexed_tile_canvas_write(
    DrawingProgramIndexedTileCanvasTable *canvases,
    uint32_t cell_index,
    uint32_t x,
    uint32_t y,
    uint8_t index);
int drawing_program_indexed_tile_canvas_find_atlas_coordinate(
    const DrawingProgramIndexedCellTable *cells,
    uint32_t atlas_x,
    uint32_t atlas_y,
    uint32_t *out_cell_index,
    uint32_t *out_local_x,
    uint32_t *out_local_y);

#ifdef __cplusplus
}
#endif

#endif
