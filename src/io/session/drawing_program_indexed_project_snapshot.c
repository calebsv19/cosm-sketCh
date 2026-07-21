#include "drawing_program_texture_project_snapshot_internal.h"

#include <stdlib.h>
#include <string.h>

enum {
    DRAWING_PROGRAM_INDEXED_PROFILE_CHUNK_VERSION_V1 = 1u,
    DRAWING_PROGRAM_INDEXED_LAYER_CHUNK_VERSION_V1 = 1u
};

typedef struct DrawingProgramIndexedProfileChunkV1 {
    uint32_t version;
    DrawingProgramIndexedTilesetProfile profile;
} DrawingProgramIndexedProfileChunkV1;

typedef struct DrawingProgramIndexedLayerChunkHeaderV1 {
    uint32_t version;
    uint32_t width;
    uint32_t height;
    uint32_t index_count;
    uint32_t slot_count;
} DrawingProgramIndexedLayerChunkHeaderV1;

static CoreResult indexed_snapshot_format(const char *message) {
    CoreResult result = { CORE_ERR_FORMAT, message };
    return result;
}

CoreResult drawing_program_indexed_project_snapshot_write(
    CorePackWriter *writer,
    const DrawingProgramTextureProject *project) {
    DrawingProgramIndexedProfileChunkV1 profile_chunk;
    DrawingProgramIndexedLayerChunkHeaderV1 layer_header;
    uint8_t *layer_payload = 0;
    uint64_t layer_payload_size;
    CoreResult result;
    if (!writer || !project ||
        drawing_program_texture_project_validate_indexed_atlas(project).code != CORE_OK) {
        return indexed_snapshot_format("invalid indexed project snapshot write request");
    }
    memset(&profile_chunk, 0, sizeof(profile_chunk));
    profile_chunk.version = DRAWING_PROGRAM_INDEXED_PROFILE_CHUNK_VERSION_V1;
    profile_chunk.profile = project->indexed_profile;
    result = core_pack_writer_add_chunk(writer, "DPIP", &profile_chunk, (uint64_t)sizeof(profile_chunk));
    if (result.code != CORE_OK) {
        return result;
    }
    memset(&layer_header, 0, sizeof(layer_header));
    layer_header.version = DRAWING_PROGRAM_INDEXED_LAYER_CHUNK_VERSION_V1;
    layer_header.width = project->indexed_raster.width;
    layer_header.height = project->indexed_raster.height;
    layer_header.index_count = project->indexed_raster.index_count;
    layer_header.slot_count = project->indexed_raster.slot_count;
    layer_payload_size = (uint64_t)sizeof(layer_header) + (uint64_t)layer_header.index_count;
    if (layer_payload_size > (uint64_t)SIZE_MAX) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "indexed layer snapshot size overflow" };
    }
    layer_payload = (uint8_t *)malloc((size_t)layer_payload_size);
    if (!layer_payload) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate indexed layer snapshot" };
    }
    memcpy(layer_payload, &layer_header, sizeof(layer_header));
    memcpy(layer_payload + sizeof(layer_header),
           project->indexed_raster.indices,
           (size_t)layer_header.index_count);
    result = core_pack_writer_add_chunk(writer, "DPIL", layer_payload, layer_payload_size);
    free(layer_payload);
    return result;
}

CoreResult drawing_program_indexed_project_snapshot_load(
    CorePackReader *reader,
    DrawingProgramTextureProject *project) {
    CorePackChunkInfo profile_info;
    CorePackChunkInfo layer_info;
    DrawingProgramIndexedProfileChunkV1 profile_chunk;
    DrawingProgramIndexedLayerChunkHeaderV1 layer_header;
    DrawingProgramIndexedLayerRaster next_raster;
    uint8_t *layer_payload = 0;
    CoreResult result;
    if (!reader || !project) {
        return indexed_snapshot_format("invalid indexed project snapshot load request");
    }
    memset(&profile_info, 0, sizeof(profile_info));
    memset(&layer_info, 0, sizeof(layer_info));
    memset(&profile_chunk, 0, sizeof(profile_chunk));
    memset(&layer_header, 0, sizeof(layer_header));
    memset(&next_raster, 0, sizeof(next_raster));
    result = core_pack_reader_find_chunk(reader, "DPIP", 0u, &profile_info);
    if (result.code != CORE_OK || profile_info.size != (uint64_t)sizeof(profile_chunk)) {
        return indexed_snapshot_format("indexed profile chunk missing or invalid");
    }
    result = core_pack_reader_read_chunk_data(reader,
                                              &profile_info,
                                              &profile_chunk,
                                              (uint64_t)sizeof(profile_chunk));
    if (result.code != CORE_OK ||
        profile_chunk.version != DRAWING_PROGRAM_INDEXED_PROFILE_CHUNK_VERSION_V1 ||
        drawing_program_indexed_tileset_profile_validate(&profile_chunk.profile).code != CORE_OK) {
        return indexed_snapshot_format("indexed profile chunk contract invalid");
    }
    result = core_pack_reader_find_chunk(reader, "DPIL", 0u, &layer_info);
    if (result.code != CORE_OK || layer_info.size < (uint64_t)sizeof(layer_header) ||
        layer_info.size > (uint64_t)SIZE_MAX) {
        return indexed_snapshot_format("indexed layer chunk missing or invalid");
    }
    layer_payload = (uint8_t *)malloc((size_t)layer_info.size);
    if (!layer_payload) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate indexed layer load buffer" };
    }
    result = core_pack_reader_read_chunk_data(reader, &layer_info, layer_payload, layer_info.size);
    if (result.code != CORE_OK) {
        free(layer_payload);
        return result;
    }
    memcpy(&layer_header, layer_payload, sizeof(layer_header));
    if (layer_header.version != DRAWING_PROGRAM_INDEXED_LAYER_CHUNK_VERSION_V1 ||
        layer_header.width != profile_chunk.profile.atlas_width ||
        layer_header.height != profile_chunk.profile.atlas_height ||
        layer_header.slot_count != profile_chunk.profile.slot_count ||
        layer_info.size != (uint64_t)sizeof(layer_header) + (uint64_t)layer_header.index_count) {
        free(layer_payload);
        return indexed_snapshot_format("indexed layer chunk shape invalid");
    }
    result = drawing_program_indexed_layer_raster_import(
        &next_raster,
        layer_header.width,
        layer_header.height,
        layer_header.slot_count,
        layer_payload + sizeof(layer_header),
        layer_header.index_count);
    free(layer_payload);
    if (result.code != CORE_OK) {
        return result;
    }
    drawing_program_indexed_layer_raster_dispose(&project->indexed_raster);
    drawing_program_indexed_history_clear(&project->indexed_history);
    project->indexed_profile = profile_chunk.profile;
    project->indexed_raster = next_raster;
    project->profile_kind = DRAWING_PROGRAM_TEXTURE_PROJECT_PROFILE_INDEXED_ATLAS_V1;
    result = drawing_program_texture_project_validate_indexed_atlas(project);
    if (result.code != CORE_OK) {
        drawing_program_indexed_layer_raster_dispose(&project->indexed_raster);
        project->profile_kind = DRAWING_PROGRAM_TEXTURE_PROJECT_PROFILE_STANDARD_RGBA;
        drawing_program_indexed_tileset_profile_clear(&project->indexed_profile);
        return result;
    }
    return core_result_ok();
}
