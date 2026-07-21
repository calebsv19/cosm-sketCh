#ifndef DRAWING_PROGRAM_INDEXED_TILESET_PROFILE_H
#define DRAWING_PROGRAM_INDEXED_TILESET_PROFILE_H

#include <stdint.h>

#include "core_authored_texture.h"
#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_INDEXED_TILESET_ID_CAPACITY 64u
#define DRAWING_PROGRAM_INDEXED_SLOT_ID_CAPACITY 64u
#define DRAWING_PROGRAM_INDEXED_SLOT_CAPACITY 32u

typedef struct DrawingProgramIndexedPaletteSlot {
    char id[DRAWING_PROGRAM_INDEXED_SLOT_ID_CAPACITY];
    CoreAuthoredTextureRgba8 source_rgba;
    CoreAuthoredTextureRgba8 preview_rgba;
} DrawingProgramIndexedPaletteSlot;

typedef struct DrawingProgramIndexedTilesetProfile {
    uint32_t contract_revision;
    uint32_t atlas_width;
    uint32_t atlas_height;
    uint32_t logical_cell_width;
    uint32_t logical_cell_height;
    uint32_t slot_count;
    uint32_t transparent_slot_index;
    char tileset_id[DRAWING_PROGRAM_INDEXED_TILESET_ID_CAPACITY];
    DrawingProgramIndexedPaletteSlot slots[DRAWING_PROGRAM_INDEXED_SLOT_CAPACITY];
} DrawingProgramIndexedTilesetProfile;

void drawing_program_indexed_tileset_profile_clear(
    DrawingProgramIndexedTilesetProfile *profile);
CoreResult drawing_program_indexed_tileset_profile_validate(
    const DrawingProgramIndexedTilesetProfile *profile);

#ifdef __cplusplus
}
#endif

#endif
