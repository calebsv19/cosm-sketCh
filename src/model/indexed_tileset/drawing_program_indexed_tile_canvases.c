#include "drawing_program/drawing_program_indexed_tile_canvases.h"

#include <string.h>

static CoreResult indexed_tile_canvases_invalid(const char *message) {
    return (CoreResult){ CORE_ERR_INVALID_ARG, message };
}

void drawing_program_indexed_tile_canvas_table_clear(
    DrawingProgramIndexedTileCanvasTable *table) {
    if (table) memset(table, 0, sizeof(*table));
}

CoreResult drawing_program_indexed_tile_canvas_table_validate(
    const DrawingProgramIndexedTileCanvasTable *canvases,
    const DrawingProgramIndexedCellTable *cells,
    const DrawingProgramIndexedTilesetProfile *profile) {
    uint32_t i;
    uint32_t pixel_count;
    if (!canvases || !cells || !profile ||
        drawing_program_indexed_tileset_profile_validate(profile).code != CORE_OK ||
        drawing_program_indexed_cell_table_validate(cells, profile->atlas_width, profile->atlas_height,
                                                    profile->logical_cell_width,
                                                    profile->logical_cell_height).code != CORE_OK ||
        canvases->count != cells->count || canvases->count == 0u ||
        canvases->count > DRAWING_PROGRAM_INDEXED_CELL_CAPACITY ||
        canvases->slot_count != profile->slot_count) {
        return indexed_tile_canvases_invalid("indexed tile canvas table contract invalid");
    }
    pixel_count = profile->logical_cell_width * profile->logical_cell_height;
    if (pixel_count > DRAWING_PROGRAM_INDEXED_TILE_CANVAS_PIXEL_CAPACITY) {
        return indexed_tile_canvases_invalid("indexed tile canvas capacity exceeded");
    }
    for (i = 0u; i < canvases->count; ++i) {
        const DrawingProgramIndexedTileCanvas *canvas = &canvases->canvases[i];
        uint32_t p;
        if (cells->cells[i].width != profile->logical_cell_width ||
            cells->cells[i].height != profile->logical_cell_height ||
            canvas->width != profile->logical_cell_width ||
            canvas->height != profile->logical_cell_height) {
            return indexed_tile_canvases_invalid("indexed tile canvas geometry must remain fixed");
        }
        for (p = 0u; p < pixel_count; ++p) {
            if ((uint32_t)canvas->indices[p] >= canvases->slot_count) {
                return indexed_tile_canvases_invalid("indexed tile canvas slot out of range");
            }
        }
    }
    return core_result_ok();
}

int drawing_program_indexed_tile_canvas_find_atlas_coordinate(
    const DrawingProgramIndexedCellTable *cells,
    uint32_t atlas_x,
    uint32_t atlas_y,
    uint32_t *out_cell_index,
    uint32_t *out_local_x,
    uint32_t *out_local_y) {
    uint32_t i;
    if (!cells || !out_cell_index || !out_local_x || !out_local_y) return 0;
    for (i = 0u; i < cells->count; ++i) {
        const DrawingProgramIndexedCell *cell = &cells->cells[i];
        if (atlas_x >= cell->x && atlas_x < cell->x + cell->width &&
            atlas_y >= cell->y && atlas_y < cell->y + cell->height) {
            *out_cell_index = i;
            *out_local_x = atlas_x - cell->x;
            *out_local_y = atlas_y - cell->y;
            return 1;
        }
    }
    return 0;
}

CoreResult drawing_program_indexed_tile_canvas_table_slice_atlas(
    DrawingProgramIndexedTileCanvasTable *out_canvases,
    const DrawingProgramIndexedCellTable *cells,
    const DrawingProgramIndexedTilesetProfile *profile,
    const DrawingProgramIndexedLayerRaster *atlas) {
    DrawingProgramIndexedTileCanvasTable next;
    uint32_t i;
    if (!out_canvases || !cells || !profile || !atlas ||
        drawing_program_indexed_layer_raster_validate(atlas).code != CORE_OK ||
        atlas->width != profile->atlas_width || atlas->height != profile->atlas_height ||
        atlas->slot_count != profile->slot_count ||
        drawing_program_indexed_cell_table_validate(cells, profile->atlas_width, profile->atlas_height,
                                                    profile->logical_cell_width,
                                                    profile->logical_cell_height).code != CORE_OK) {
        return indexed_tile_canvases_invalid("cannot slice indexed atlas into tile canvases");
    }
    memset(&next, 0, sizeof(next));
    next.count = cells->count;
    next.slot_count = profile->slot_count;
    for (i = 0u; i < cells->count; ++i) {
        const DrawingProgramIndexedCell *cell = &cells->cells[i];
        DrawingProgramIndexedTileCanvas *canvas = &next.canvases[i];
        uint32_t y;
        canvas->width = cell->width;
        canvas->height = cell->height;
        for (y = 0u; y < cell->height; ++y) {
            memcpy(&canvas->indices[y * cell->width],
                   &atlas->indices[(cell->y + y) * atlas->width + cell->x],
                   cell->width);
        }
    }
    *out_canvases = next;
    return core_result_ok();
}

CoreResult drawing_program_indexed_tile_canvas_table_compose_atlas(
    const DrawingProgramIndexedTileCanvasTable *canvases,
    const DrawingProgramIndexedCellTable *cells,
    const DrawingProgramIndexedTilesetProfile *profile,
    DrawingProgramIndexedLayerRaster *out_atlas) {
    DrawingProgramIndexedLayerRaster next;
    uint32_t i;
    CoreResult result;
    if (!out_atlas || drawing_program_indexed_tile_canvas_table_validate(canvases, cells, profile).code != CORE_OK) {
        return indexed_tile_canvases_invalid("cannot compose indexed tile canvases");
    }
    memset(&next, 0, sizeof(next));
    result = drawing_program_indexed_layer_raster_init(&next, profile->atlas_width, profile->atlas_height,
                                                       profile->slot_count, profile->transparent_slot_index);
    if (result.code != CORE_OK) return result;
    for (i = 0u; i < cells->count; ++i) {
        const DrawingProgramIndexedCell *cell = &cells->cells[i];
        const DrawingProgramIndexedTileCanvas *canvas = &canvases->canvases[i];
        uint32_t y;
        for (y = 0u; y < cell->height; ++y) {
            memcpy(&next.indices[(cell->y + y) * next.width + cell->x],
                   &canvas->indices[y * cell->width], cell->width);
        }
    }
    drawing_program_indexed_layer_raster_dispose(out_atlas);
    *out_atlas = next;
    return core_result_ok();
}

CoreResult drawing_program_indexed_tile_canvas_read(
    const DrawingProgramIndexedTileCanvasTable *canvases,
    uint32_t cell_index, uint32_t x, uint32_t y, uint8_t *out_index) {
    const DrawingProgramIndexedTileCanvas *canvas;
    if (!canvases || !out_index || cell_index >= canvases->count) {
        return indexed_tile_canvases_invalid("indexed tile canvas read invalid");
    }
    canvas = &canvases->canvases[cell_index];
    if (x >= canvas->width || y >= canvas->height) return indexed_tile_canvases_invalid("indexed tile canvas coordinate invalid");
    *out_index = canvas->indices[y * canvas->width + x];
    return core_result_ok();
}

CoreResult drawing_program_indexed_tile_canvas_write(
    DrawingProgramIndexedTileCanvasTable *canvases,
    uint32_t cell_index, uint32_t x, uint32_t y, uint8_t index) {
    DrawingProgramIndexedTileCanvas *canvas;
    if (!canvases || cell_index >= canvases->count || (uint32_t)index >= canvases->slot_count) {
        return indexed_tile_canvases_invalid("indexed tile canvas write invalid");
    }
    canvas = &canvases->canvases[cell_index];
    if (x >= canvas->width || y >= canvas->height) return indexed_tile_canvases_invalid("indexed tile canvas coordinate invalid");
    canvas->indices[y * canvas->width + x] = index;
    return core_result_ok();
}
