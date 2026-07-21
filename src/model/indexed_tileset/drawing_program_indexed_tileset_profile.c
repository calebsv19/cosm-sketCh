#include "drawing_program/drawing_program_indexed_tileset_profile.h"

#include <string.h>

static CoreResult indexed_profile_invalid(const char *message) {
    CoreResult result = { CORE_ERR_INVALID_ARG, message };
    return result;
}

void drawing_program_indexed_tileset_profile_clear(
    DrawingProgramIndexedTilesetProfile *profile) {
    if (profile) {
        memset(profile, 0, sizeof(*profile));
    }
}

CoreResult drawing_program_indexed_tileset_profile_validate(
    const DrawingProgramIndexedTilesetProfile *profile) {
    CoreAuthoredTextureIndexedSlot slots[DRAWING_PROGRAM_INDEXED_SLOT_CAPACITY];
    CoreAuthoredTexturePaletteEntry entries[DRAWING_PROGRAM_INDEXED_SLOT_CAPACITY];
    CoreAuthoredTextureIndexedPaletteContract contract;
    uint32_t i;
    if (!profile ||
        profile->contract_revision != CORE_AUTHORED_TEXTURE_INDEXED_CONTRACT_REVISION_V1 ||
        !core_authored_texture_identifier_validate(profile->tileset_id) ||
        profile->atlas_width == 0u || profile->atlas_height == 0u ||
        profile->logical_cell_width == 0u || profile->logical_cell_height == 0u ||
        profile->atlas_width % profile->logical_cell_width != 0u ||
        profile->atlas_height % profile->logical_cell_height != 0u ||
        profile->slot_count == 0u ||
        profile->slot_count > DRAWING_PROGRAM_INDEXED_SLOT_CAPACITY ||
        profile->transparent_slot_index >= profile->slot_count) {
        return indexed_profile_invalid("invalid indexed tileset profile shape");
    }
    memset(slots, 0, sizeof(slots));
    memset(entries, 0, sizeof(entries));
    for (i = 0u; i < profile->slot_count; ++i) {
        slots[i].id = profile->slots[i].id;
        slots[i].source_rgba = profile->slots[i].source_rgba;
        entries[i].slot_id = profile->slots[i].id;
        entries[i].rgba = profile->slots[i].preview_rgba;
    }
    memset(&contract, 0, sizeof(contract));
    contract.revision = profile->contract_revision;
    contract.slots = slots;
    contract.slot_count = profile->slot_count;
    contract.entries = entries;
    contract.entry_count = profile->slot_count;
    if (!core_authored_texture_indexed_palette_validate(&contract)) {
        return indexed_profile_invalid("indexed tileset palette contract invalid");
    }
    if (profile->slots[profile->transparent_slot_index].source_rgba.a != 0u ||
        profile->slots[profile->transparent_slot_index].preview_rgba.a != 0u) {
        return indexed_profile_invalid("indexed transparent slot must have zero alpha");
    }
    return core_result_ok();
}
