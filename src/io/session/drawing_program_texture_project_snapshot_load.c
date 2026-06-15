#include "drawing_program_texture_project_snapshot_internal.h"

#include <stdlib.h>
#include <string.h>

CoreResult texture_project_snapshot_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

int texture_project_document_bounds_valid(const DrawingProgramDocument *document) {
    return document &&
           document->logical_width > 0u &&
           document->logical_height > 0u &&
           document->sample_density > 0u &&
           document->layer_count > 0u &&
           document->layer_count <= DRAWING_PROGRAM_MAX_LAYERS &&
           document->raster_width > 0u &&
           document->raster_height > 0u &&
           document->raster_sample_count > 0u &&
           document->raster_sample_count <= DRAWING_PROGRAM_MAX_RASTER_SAMPLES &&
           document->raster_width == document->logical_width * document->sample_density &&
           document->raster_height == document->logical_height * document->sample_density &&
           document->raster_sample_count == document->raster_width * document->raster_height;
}

void texture_project_snapshot_seed_surface_layer_opacity_defaults(
    DrawingProgramTextureSurface *surface) {
    uint32_t i;
    if (!surface || !surface->storage) {
        return;
    }
    surface->layer_opacity_entry_count = 0u;
    memset(surface->layer_opacity_values, 0, sizeof(surface->layer_opacity_values));
    memset(surface->layer_opacity_layer_ids, 0, sizeof(surface->layer_opacity_layer_ids));
    for (i = 0u;
         i < surface->storage->document.layer_count && i < DRAWING_PROGRAM_MAX_LAYERS;
         ++i) {
        surface->layer_opacity_layer_ids[i] = surface->storage->document.layers[i].layer_id;
        surface->layer_opacity_values[i] = 100u;
        surface->layer_opacity_entry_count += 1u;
    }
}

void texture_project_snapshot_seed_surface_layer_role_defaults(
    DrawingProgramTextureSurface *surface) {
    uint32_t i;
    if (!surface || !surface->storage) {
        return;
    }
    surface->layer_role_entry_count = 0u;
    memset(surface->layer_role_values, 0, sizeof(surface->layer_role_values));
    memset(surface->layer_role_layer_ids, 0, sizeof(surface->layer_role_layer_ids));
    for (i = 0u;
         i < surface->storage->document.layer_count && i < DRAWING_PROGRAM_MAX_LAYERS;
         ++i) {
        const DrawingProgramLayer *layer = &surface->storage->document.layers[i];
        surface->layer_role_layer_ids[i] = layer->layer_id;
        surface->layer_role_values[i] =
            (uint8_t)drawing_program_texture_layer_role_detect_name(layer->name);
        surface->layer_role_entry_count += 1u;
    }
}

void texture_project_snapshot_seed_surface_layer_material_intent_defaults(
    DrawingProgramTextureSurface *surface,
    uint32_t overlay_material_intent_kind) {
    uint32_t i;
    uint32_t legacy_overlay_intent =
        drawing_program_texture_layer_material_intent_from_legacy_overlay_kind(
            overlay_material_intent_kind);
    if (!surface || !surface->storage) {
        return;
    }
    surface->layer_material_intent_entry_count = 0u;
    memset(surface->layer_material_intent_values, 0, sizeof(surface->layer_material_intent_values));
    memset(surface->layer_material_intent_layer_ids, 0, sizeof(surface->layer_material_intent_layer_ids));
    for (i = 0u;
         i < surface->storage->document.layer_count && i < DRAWING_PROGRAM_MAX_LAYERS;
         ++i) {
        const DrawingProgramLayer *layer = &surface->storage->document.layers[i];
        uint32_t role_kind = drawing_program_texture_layer_role_detect_name(layer->name);
        uint32_t intent_kind = drawing_program_texture_layer_material_intent_default_for_role(role_kind);
        if (legacy_overlay_intent != DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE &&
            drawing_program_texture_layer_role_prefers_overlay(role_kind)) {
            intent_kind = legacy_overlay_intent;
        }
        surface->layer_material_intent_layer_ids[i] = layer->layer_id;
        surface->layer_material_intent_values[i] = (uint8_t)intent_kind;
        surface->layer_material_intent_entry_count += 1u;
    }
}

CoreResult texture_project_snapshot_apply_surface_layer_chunk(
    DrawingProgramTextureSurfaceStorage *storage,
    const void *chunk_data,
    uint64_t chunk_size) {
    const uint8_t *cursor = (const uint8_t *)chunk_data;
    const uint8_t *end = cursor + chunk_size;
    uint32_t version = 0u;
    uint32_t raster_width = 0u;
    uint32_t raster_height = 0u;
    uint32_t sample_count = 0u;
    uint32_t layer_count = 0u;
    uint32_t i;
    if (!storage || !cursor || chunk_size < (uint64_t)(sizeof(uint32_t) * 5u)) {
        return texture_project_snapshot_invalid("invalid texture surface layer chunk payload");
    }
    memcpy(&version, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&raster_width, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&raster_height, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&sample_count, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&layer_count, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    if (version != DRAWING_PROGRAM_TEXTURE_SURFACE_LAYER_CHUNK_VERSION_V2) {
        return (CoreResult){ CORE_ERR_FORMAT, "unsupported texture surface layer chunk version" };
    }
    if (raster_width != storage->document.raster_width ||
        raster_height != storage->document.raster_height ||
        sample_count != storage->document.raster_sample_count ||
        layer_count > DRAWING_PROGRAM_MAX_LAYERS) {
        return (CoreResult){ CORE_ERR_FORMAT, "texture surface layer chunk shape mismatch" };
    }
    for (i = 0u; i < layer_count; ++i) {
        uint32_t layer_id = 0u;
        uint32_t entry_sample_count = 0u;
        if ((uint64_t)(end - cursor) < (uint64_t)(sizeof(uint32_t) * 2u)) {
            return (CoreResult){ CORE_ERR_FORMAT, "texture surface layer chunk truncated header" };
        }
        memcpy(&layer_id, cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        memcpy(&entry_sample_count, cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        if (entry_sample_count != sample_count ||
            (uint64_t)(end - cursor) <
                ((uint64_t)entry_sample_count * (uint64_t)sizeof(DrawingProgramRasterSample))) {
            return (CoreResult){ CORE_ERR_FORMAT, "texture surface layer chunk truncated samples" };
        }
        if (drawing_program_document_layer_index_for_id(&storage->document, layer_id, &(uint32_t){ 0u }).code == CORE_OK) {
            CoreResult import_result = drawing_program_layer_raster_store_import_layer(&storage->layer_rasters,
                                                                                       layer_id,
                                                                                       (const DrawingProgramRasterSample *)cursor,
                                                                                       entry_sample_count);
            if (import_result.code != CORE_OK) {
                return import_result;
            }
            if (layer_id == drawing_program_layer_raster_legacy_surface_layer_id(&storage->document)) {
                memcpy(storage->document.raster_samples,
                       cursor,
                       (size_t)entry_sample_count * sizeof(storage->document.raster_samples[0]));
            }
        }
        cursor += (uint64_t)entry_sample_count * (uint64_t)sizeof(DrawingProgramRasterSample);
    }
    if (cursor != end) {
        return (CoreResult){ CORE_ERR_FORMAT, "texture surface layer chunk trailing bytes" };
    }
    return core_result_ok();
}

CoreResult drawing_program_texture_project_snapshot_load(
    DrawingProgramTextureProject *project,
    CorePackReader *reader,
    uint8_t *out_found) {
    CorePackChunkInfo root_chunk;
    DrawingProgramTextureProjectChunkHeaderV1 header;
    DrawingProgramTextureProjectChunkHeaderV3 header_v3;
    DrawingProgramTextureProjectChunkHeaderV4 header_v4;
    DrawingProgramTextureProjectChunkHeaderV4 header_v5;
    DrawingProgramTextureProjectChunkHeaderV6 header_v6;
    DrawingProgramTextureProjectChunkHeaderV7 header_v7;
    DrawingProgramTextureProjectChunkHeaderV8 header_v8;
    DrawingProgramTextureProjectChunkHeaderV9 header_v9;
    DrawingProgramTextureProjectChunkHeaderV10 header_v10;
    DrawingProgramTextureProjectChunkHeaderV11 header_v11;
    DrawingProgramTextureProjectChunkHeaderV12 header_v12;
    const DrawingProgramTextureProjectSurfaceRecordV1 *records_v1 = 0;
    const DrawingProgramTextureProjectSurfaceRecordV2 *records_v2 = 0;
    const DrawingProgramTextureProjectSurfaceRecordV3 *records_v3 = 0;
    const DrawingProgramTextureProjectSurfaceRecordV4 *records_v4 = 0;
    const DrawingProgramTextureProjectSurfaceRecordV5 *records_v5 = 0;
    const DrawingProgramTextureProjectSurfaceRecordV6 *records_v6 = 0;
    const DrawingProgramTextureProjectSurfaceRecordV7 *records_v7 = 0;
    const DrawingProgramTextureProjectSurfaceRecordV8 *records_v8 = 0;
    const DrawingProgramTextureProjectSurfaceRecordV9 *records_v9 = 0;
    uint8_t *root_data = 0;
    CoreResult result;
    uint32_t i;
    uint64_t expected_v1_size = 0u;
    uint64_t expected_v2_size = 0u;
    uint64_t expected_v3_size = 0u;
    uint64_t expected_v4_size = 0u;
    uint64_t expected_v5_size = 0u;
    uint64_t expected_v6_size = 0u;
    uint64_t expected_v7_size = 0u;
    uint64_t expected_v8_size = 0u;
    uint64_t expected_v9_size = 0u;
    uint64_t expected_v10_size = 0u;
    uint64_t expected_v11_size = 0u;
    uint64_t expected_v12_size = 0u;
    uint64_t expected_root_size = 0u;
    uint8_t is_v2 = 0u;
    uint8_t is_v3 = 0u;
    uint8_t is_v4 = 0u;
    uint8_t is_v5 = 0u;
    uint8_t is_v6 = 0u;
    uint8_t is_v7 = 0u;
    uint8_t is_v8 = 0u;
    uint8_t is_v9 = 0u;
    uint8_t is_v10 = 0u;
    uint8_t is_v11 = 0u;
    uint8_t is_v12 = 0u;
    uint32_t root_surface_count = 0u;
    uint32_t root_active_surface_index = 0u;
    uint32_t root_primitive_kind = 0u;
    uint32_t root_net_layout_kind = 0u;
    uint32_t root_quality_preset = 0u;
    uint32_t root_export_binding_kind = 0u;
    uint32_t root_export_intent_kind = DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_FLATTENED_ONLY;
    uint32_t root_overlay_material_intent_kind = DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_NONE;
    uint32_t root_next_surface_id = 1u;
    const char *root_source_scene_id = 0;
    const char *root_source_object_id = 0;
    const char *root_source_scene_path = 0;
    if (out_found) {
        *out_found = 0u;
    }
    if (!project || !reader) {
        return texture_project_snapshot_invalid("invalid texture project snapshot load request");
    }
    memset(&root_chunk, 0, sizeof(root_chunk));
    result = core_pack_reader_find_chunk(reader, "DPTP", 0u, &root_chunk);
    if (result.code == CORE_ERR_NOT_FOUND) {
        return core_result_ok();
    }
    if (result.code != CORE_OK) {
        return result;
    }
    if (root_chunk.size < (uint64_t)sizeof(header)) {
        return (CoreResult){ CORE_ERR_FORMAT, "texture project root chunk too small" };
    }
    root_data = (uint8_t *)malloc((size_t)root_chunk.size);
    if (!root_data) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate texture project root buffer" };
    }
    result = core_pack_reader_read_chunk_data(reader, &root_chunk, root_data, root_chunk.size);
    if (result.code != CORE_OK) {
        free(root_data);
        return result;
    }
    memcpy(&header, root_data, sizeof(header));
    memset(&header_v3, 0, sizeof(header_v3));
    memset(&header_v4, 0, sizeof(header_v4));
    memset(&header_v5, 0, sizeof(header_v5));
    memset(&header_v6, 0, sizeof(header_v6));
    memset(&header_v7, 0, sizeof(header_v7));
    memset(&header_v8, 0, sizeof(header_v8));
    memset(&header_v9, 0, sizeof(header_v9));
    memset(&header_v10, 0, sizeof(header_v10));
    memset(&header_v11, 0, sizeof(header_v11));
    memset(&header_v12, 0, sizeof(header_v12));
    if (root_chunk.size >= (uint64_t)sizeof(header_v12)) {
        memcpy(&header_v12, root_data, sizeof(header_v12));
    }
    if (root_chunk.size >= (uint64_t)sizeof(header_v11)) {
        memcpy(&header_v11, root_data, sizeof(header_v11));
    }
    if (root_chunk.size >= (uint64_t)sizeof(header_v10)) {
        memcpy(&header_v10, root_data, sizeof(header_v10));
    }
    if (root_chunk.size >= (uint64_t)sizeof(header_v9)) {
        memcpy(&header_v9, root_data, sizeof(header_v9));
    }
    if (root_chunk.size >= (uint64_t)sizeof(header_v8)) {
        memcpy(&header_v8, root_data, sizeof(header_v8));
    }
    if (root_chunk.size >= (uint64_t)sizeof(header_v7)) {
        memcpy(&header_v7, root_data, sizeof(header_v7));
    }
    if (root_chunk.size >= (uint64_t)sizeof(header_v6)) {
        memcpy(&header_v6, root_data, sizeof(header_v6));
    }
    if (root_chunk.size >= (uint64_t)sizeof(header_v5)) {
        memcpy(&header_v5, root_data, sizeof(header_v5));
    }
    if (root_chunk.size >= (uint64_t)sizeof(header_v4)) {
        memcpy(&header_v4, root_data, sizeof(header_v4));
    }
    if (root_chunk.size >= (uint64_t)sizeof(header_v3)) {
        memcpy(&header_v3, root_data, sizeof(header_v3));
    } else {
        memcpy(&header_v3, &header, sizeof(header));
    }
    expected_v1_size = (uint64_t)sizeof(header) +
                       ((uint64_t)header.surface_count *
                        (uint64_t)sizeof(DrawingProgramTextureProjectSurfaceRecordV1));
    expected_v2_size = (uint64_t)sizeof(header) +
                       ((uint64_t)header.surface_count *
                        (uint64_t)sizeof(DrawingProgramTextureProjectSurfaceRecordV2));
    expected_v3_size = (uint64_t)sizeof(header_v3) +
                       ((uint64_t)header_v3.surface_count *
                        (uint64_t)sizeof(DrawingProgramTextureProjectSurfaceRecordV2));
    expected_v4_size = (uint64_t)sizeof(header_v4) +
                       ((uint64_t)header_v4.surface_count *
                        (uint64_t)sizeof(DrawingProgramTextureProjectSurfaceRecordV3));
    expected_v5_size = (uint64_t)sizeof(header_v5) +
                       ((uint64_t)header_v5.surface_count *
                        (uint64_t)sizeof(DrawingProgramTextureProjectSurfaceRecordV4));
    expected_v6_size = (uint64_t)sizeof(header_v6) +
                       ((uint64_t)header_v6.surface_count *
                        (uint64_t)sizeof(DrawingProgramTextureProjectSurfaceRecordV4));
    expected_v7_size = (uint64_t)sizeof(header_v7) +
                       ((uint64_t)header_v7.surface_count *
                        (uint64_t)sizeof(DrawingProgramTextureProjectSurfaceRecordV4));
    expected_v8_size = (uint64_t)sizeof(header_v8) +
                       ((uint64_t)header_v8.surface_count *
                        (uint64_t)sizeof(DrawingProgramTextureProjectSurfaceRecordV5));
    expected_v9_size = (uint64_t)sizeof(header_v9) +
                       ((uint64_t)header_v9.surface_count *
                        (uint64_t)sizeof(DrawingProgramTextureProjectSurfaceRecordV6));
    expected_v10_size = (uint64_t)sizeof(header_v10) +
                        ((uint64_t)header_v10.surface_count *
                         (uint64_t)sizeof(DrawingProgramTextureProjectSurfaceRecordV7));
    expected_v11_size = (uint64_t)sizeof(header_v11) +
                        ((uint64_t)header_v11.surface_count *
                         (uint64_t)sizeof(DrawingProgramTextureProjectSurfaceRecordV8));
    expected_v12_size = (uint64_t)sizeof(header_v12) +
                        ((uint64_t)header_v12.surface_count *
                         (uint64_t)sizeof(DrawingProgramTextureProjectSurfaceRecordV9));
    is_v2 = (header.version == DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V2) ? 1u : 0u;
    is_v3 = (header_v3.version == DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V3) ? 1u : 0u;
    is_v4 = (header_v4.version == DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V4) ? 1u : 0u;
    is_v5 = (header_v5.version == DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V5) ? 1u : 0u;
    is_v6 = (header_v6.version == DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V6) ? 1u : 0u;
    is_v7 = (header_v7.version == DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V7) ? 1u : 0u;
    is_v8 = (header_v8.version == DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V8) ? 1u : 0u;
    is_v9 = (header_v9.version == DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V9) ? 1u : 0u;
    is_v10 = (header_v10.version == DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V10) ? 1u : 0u;
    is_v11 = (header_v11.version == DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V11) ? 1u : 0u;
    is_v12 = (header_v12.version == DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V12) ? 1u : 0u;
    if (is_v12) {
        root_surface_count = header_v12.surface_count;
        root_active_surface_index = header_v12.active_surface_index;
        root_primitive_kind = header_v12.primitive_kind;
        root_net_layout_kind = header_v12.net_layout_kind;
        root_quality_preset = header_v12.quality_preset;
        root_export_binding_kind = header_v12.export_binding_kind;
        root_export_intent_kind = header_v12.export_intent_kind;
        root_overlay_material_intent_kind = header_v12.overlay_material_intent_kind;
        root_next_surface_id = header_v12.next_surface_id;
        root_source_scene_id = header_v12.source_scene_id;
        root_source_object_id = header_v12.source_object_id;
        root_source_scene_path = header_v12.source_scene_path;
    } else if (is_v11) {
        root_surface_count = header_v11.surface_count;
        root_active_surface_index = header_v11.active_surface_index;
        root_primitive_kind = header_v11.primitive_kind;
        root_net_layout_kind = header_v11.net_layout_kind;
        root_quality_preset = header_v11.quality_preset;
        root_export_binding_kind = header_v11.export_binding_kind;
        root_export_intent_kind = header_v11.export_intent_kind;
        root_overlay_material_intent_kind = header_v11.overlay_material_intent_kind;
        root_next_surface_id = header_v11.next_surface_id;
        root_source_scene_id = header_v11.source_scene_id;
        root_source_object_id = header_v11.source_object_id;
        root_source_scene_path = header_v11.source_scene_path;
    } else if (is_v10) {
        root_surface_count = header_v10.surface_count;
        root_active_surface_index = header_v10.active_surface_index;
        root_primitive_kind = header_v10.primitive_kind;
        root_net_layout_kind = header_v10.net_layout_kind;
        root_quality_preset = header_v10.quality_preset;
        root_export_binding_kind = header_v10.export_binding_kind;
        root_export_intent_kind = header_v10.export_intent_kind;
        root_overlay_material_intent_kind = header_v10.overlay_material_intent_kind;
        root_next_surface_id = header_v10.next_surface_id;
        root_source_scene_id = header_v10.source_scene_id;
        root_source_object_id = header_v10.source_object_id;
        root_source_scene_path = header_v10.source_scene_path;
    } else if (is_v9) {
        root_surface_count = header_v9.surface_count;
        root_active_surface_index = header_v9.active_surface_index;
        root_primitive_kind = header_v9.primitive_kind;
        root_net_layout_kind = header_v9.net_layout_kind;
        root_quality_preset = header_v9.quality_preset;
        root_export_binding_kind = header_v9.export_binding_kind;
        root_export_intent_kind = header_v9.export_intent_kind;
        root_overlay_material_intent_kind = header_v9.overlay_material_intent_kind;
        root_next_surface_id = header_v9.next_surface_id;
        root_source_scene_id = header_v9.source_scene_id;
        root_source_object_id = header_v9.source_object_id;
        root_source_scene_path = header_v9.source_scene_path;
    } else if (is_v8) {
        root_surface_count = header_v8.surface_count;
        root_active_surface_index = header_v8.active_surface_index;
        root_primitive_kind = header_v8.primitive_kind;
        root_net_layout_kind = header_v8.net_layout_kind;
        root_quality_preset = header_v8.quality_preset;
        root_export_binding_kind = header_v8.export_binding_kind;
        root_export_intent_kind = header_v8.export_intent_kind;
        root_overlay_material_intent_kind = header_v8.overlay_material_intent_kind;
        root_next_surface_id = header_v8.next_surface_id;
        root_source_scene_id = header_v8.source_scene_id;
        root_source_object_id = header_v8.source_object_id;
        root_source_scene_path = header_v8.source_scene_path;
    } else if (is_v7) {
        root_surface_count = header_v7.surface_count;
        root_active_surface_index = header_v7.active_surface_index;
        root_primitive_kind = header_v7.primitive_kind;
        root_net_layout_kind = header_v7.net_layout_kind;
        root_quality_preset = header_v7.quality_preset;
        root_export_binding_kind = header_v7.export_binding_kind;
        root_export_intent_kind = header_v7.export_intent_kind;
        root_overlay_material_intent_kind = header_v7.overlay_material_intent_kind;
        root_next_surface_id = header_v7.next_surface_id;
        root_source_scene_id = header_v7.source_scene_id;
        root_source_object_id = header_v7.source_object_id;
        root_source_scene_path = header_v7.source_scene_path;
    } else if (is_v6) {
        root_surface_count = header_v6.surface_count;
        root_active_surface_index = header_v6.active_surface_index;
        root_primitive_kind = header_v6.primitive_kind;
        root_net_layout_kind = header_v6.net_layout_kind;
        root_quality_preset = header_v6.quality_preset;
        root_export_binding_kind = header_v6.export_binding_kind;
        root_export_intent_kind = header_v6.export_intent_kind;
        root_next_surface_id = header_v6.next_surface_id;
        root_source_scene_id = header_v6.source_scene_id;
        root_source_object_id = header_v6.source_object_id;
        root_source_scene_path = header_v6.source_scene_path;
    } else if (is_v5) {
        root_surface_count = header_v5.surface_count;
        root_active_surface_index = header_v5.active_surface_index;
        root_primitive_kind = header_v5.primitive_kind;
        root_net_layout_kind = header_v5.net_layout_kind;
        root_quality_preset = header_v5.quality_preset;
        root_export_binding_kind = header_v5.export_binding_kind;
        root_next_surface_id = header_v5.next_surface_id;
        root_source_scene_id = header_v5.source_scene_id;
        root_source_object_id = header_v5.source_object_id;
        root_source_scene_path = header_v5.source_scene_path;
    } else if (is_v4) {
        root_surface_count = header_v4.surface_count;
        root_active_surface_index = header_v4.active_surface_index;
        root_primitive_kind = header_v4.primitive_kind;
        root_net_layout_kind = header_v4.net_layout_kind;
        root_quality_preset = header_v4.quality_preset;
        root_export_binding_kind = header_v4.export_binding_kind;
        root_next_surface_id = header_v4.next_surface_id;
        root_source_scene_id = header_v4.source_scene_id;
        root_source_object_id = header_v4.source_object_id;
        root_source_scene_path = header_v4.source_scene_path;
    } else if (is_v3) {
        root_surface_count = header_v3.surface_count;
        root_active_surface_index = header_v3.active_surface_index;
        root_primitive_kind = header_v3.primitive_kind;
        root_net_layout_kind = drawing_program_texture_net_default_layout_kind(header_v3.primitive_kind);
        root_quality_preset = header_v3.quality_preset;
        root_export_binding_kind = header_v3.export_binding_kind;
        root_next_surface_id = header_v3.next_surface_id;
        root_source_scene_id = header_v3.source_scene_id;
        root_source_object_id = header_v3.source_object_id;
        root_source_scene_path = header_v3.source_scene_path;
    } else {
        root_surface_count = header.surface_count;
        root_active_surface_index = header.active_surface_index;
        root_primitive_kind = header.primitive_kind;
        root_net_layout_kind = drawing_program_texture_net_default_layout_kind(header.primitive_kind);
        root_quality_preset = header.quality_preset;
        root_export_binding_kind = header.export_binding_kind;
        root_next_surface_id = header.next_surface_id;
        root_source_scene_id = header.source_scene_id;
        root_source_object_id = header.source_object_id;
        root_source_scene_path = "";
    }
    expected_root_size = is_v12 ? expected_v12_size
                               : (is_v11 ? expected_v11_size
                               : (is_v10 ? expected_v10_size
                               : (is_v9 ? expected_v9_size
                               : (is_v8 ? expected_v8_size
                               : (is_v7 ? expected_v7_size
                               : (is_v6 ? expected_v6_size
                               : (is_v5 ? expected_v5_size
                                        : (is_v4 ? expected_v4_size
                                                 : (is_v3 ? expected_v3_size
                                                          : (is_v2 ? expected_v2_size : expected_v1_size))))))))));
    if ((header.version != DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V1 &&
        header.version != DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V2 &&
        header_v3.version != DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V3 &&
        header_v4.version != DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V4 &&
        header_v5.version != DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V5 &&
        header_v6.version != DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V6 &&
        header_v7.version != DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V7 &&
        header_v8.version != DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V8 &&
        header_v9.version != DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V9 &&
        header_v10.version != DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V10 &&
        header_v11.version != DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V11 &&
        header_v12.version != DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V12) ||
        (root_surface_count == 0u) ||
        (root_active_surface_index >= root_surface_count) ||
        root_chunk.size != expected_root_size) {
        free(root_data);
        return (CoreResult){ CORE_ERR_FORMAT, "texture project root chunk invalid" };
    }
    if (is_v12) {
        records_v9 = (const DrawingProgramTextureProjectSurfaceRecordV9 *)(root_data + sizeof(header_v12));
    } else if (is_v11) {
        records_v8 = (const DrawingProgramTextureProjectSurfaceRecordV8 *)(root_data + sizeof(header_v11));
    } else if (is_v10) {
        records_v7 = (const DrawingProgramTextureProjectSurfaceRecordV7 *)(root_data + sizeof(header_v10));
    } else if (is_v9) {
        records_v6 = (const DrawingProgramTextureProjectSurfaceRecordV6 *)(root_data + sizeof(header_v9));
    } else if (is_v8) {
        records_v5 = (const DrawingProgramTextureProjectSurfaceRecordV5 *)(root_data + sizeof(header_v8));
    } else if (is_v7) {
        records_v4 = (const DrawingProgramTextureProjectSurfaceRecordV4 *)(root_data + sizeof(header_v7));
    } else if (is_v6) {
        records_v4 = (const DrawingProgramTextureProjectSurfaceRecordV4 *)(root_data + sizeof(header_v6));
    } else if (is_v5) {
        records_v4 = (const DrawingProgramTextureProjectSurfaceRecordV4 *)(root_data + sizeof(header_v5));
    } else if (is_v4) {
        records_v3 = (const DrawingProgramTextureProjectSurfaceRecordV3 *)(root_data + sizeof(header_v4));
    } else if (is_v3) {
        records_v2 = (const DrawingProgramTextureProjectSurfaceRecordV2 *)(root_data + sizeof(header_v3));
    } else if (is_v2) {
        records_v2 = (const DrawingProgramTextureProjectSurfaceRecordV2 *)(root_data + sizeof(header));
    } else {
        records_v1 = (const DrawingProgramTextureProjectSurfaceRecordV1 *)(root_data + sizeof(header));
    }
    result = drawing_program_texture_project_init_empty(project);
    if (result.code != CORE_OK) {
        free(root_data);
        return result;
    }
    project->primitive_kind = root_primitive_kind;
    project->net_layout_kind = root_net_layout_kind;
    project->quality_preset = root_quality_preset;
    project->export_binding_kind = root_export_binding_kind;
    project->export_intent_kind = root_export_intent_kind;
    project->overlay_material_intent_kind = root_overlay_material_intent_kind;
    project->active_surface_index = root_active_surface_index;
    project->next_surface_id = root_next_surface_id > 0u ? root_next_surface_id : 1u;
    memcpy(project->source_scene_id, root_source_scene_id, sizeof(project->source_scene_id));
    memcpy(project->source_object_id, root_source_object_id, sizeof(project->source_object_id));
    memset(project->source_scene_path, 0, sizeof(project->source_scene_path));
    if (root_source_scene_path) {
        strncpy(project->source_scene_path,
                root_source_scene_path,
                sizeof(project->source_scene_path) - 1u);
    }
    for (i = 0u; i < root_surface_count; ++i) {
        CorePackChunkInfo document_chunk_info;
        CorePackChunkInfo layer_chunk_info;
        DrawingProgramTextureSurfaceDocumentChunkV1 document_chunk;
        DrawingProgramTextureSurfaceStorage *storage = 0;
        DrawingProgramTextureSurface *surface = 0;
        uint32_t surface_index = 0u;
        uint8_t *layer_chunk_data = 0;
        memset(&document_chunk_info, 0, sizeof(document_chunk_info));
        memset(&layer_chunk_info, 0, sizeof(layer_chunk_info));
        result = core_pack_reader_find_chunk(reader, "DTSD", i, &document_chunk_info);
        if (result.code != CORE_OK || document_chunk_info.size != (uint64_t)sizeof(document_chunk)) {
            drawing_program_texture_project_dispose(project);
            free(root_data);
            return (CoreResult){ CORE_ERR_FORMAT, "texture surface document chunk missing or invalid" };
        }
        result = core_pack_reader_read_chunk_data(reader, &document_chunk_info, &document_chunk, sizeof(document_chunk));
        if (result.code != CORE_OK ||
            document_chunk.version != DRAWING_PROGRAM_TEXTURE_SURFACE_DOCUMENT_CHUNK_VERSION_V1 ||
            !texture_project_document_bounds_valid(&document_chunk.document)) {
            drawing_program_texture_project_dispose(project);
            free(root_data);
            return (CoreResult){ CORE_ERR_FORMAT, "texture surface document payload invalid" };
        }
        {
            uint32_t record_logical_width =
                is_v12 ? records_v9[i].logical_width
                      : (is_v11 ? records_v8[i].logical_width
                      : (is_v10 ? records_v7[i].logical_width
                      : (is_v9 ? records_v6[i].logical_width
                      : (is_v8 ? records_v5[i].logical_width
                      : ((is_v7 || is_v6 || is_v5) ? records_v4[i].logical_width
                      : (is_v4 ? records_v3[i].logical_width
                               : ((is_v2 || is_v3) ? records_v2[i].logical_width : records_v1[i].logical_width)))))));
            uint32_t record_logical_height =
                is_v12 ? records_v9[i].logical_height
                      : (is_v11 ? records_v8[i].logical_height
                      : (is_v10 ? records_v7[i].logical_height
                      : (is_v9 ? records_v6[i].logical_height
                      : (is_v8 ? records_v5[i].logical_height
                      : ((is_v7 || is_v6 || is_v5) ? records_v4[i].logical_height
                      : (is_v4 ? records_v3[i].logical_height
                               : ((is_v2 || is_v3) ? records_v2[i].logical_height : records_v1[i].logical_height)))))));
            uint32_t record_sample_density =
                is_v12 ? records_v9[i].sample_density
                      : (is_v11 ? records_v8[i].sample_density
                      : (is_v10 ? records_v7[i].sample_density
                      : (is_v9 ? records_v6[i].sample_density
                      : (is_v8 ? records_v5[i].sample_density
                      : ((is_v7 || is_v6 || is_v5) ? records_v4[i].sample_density
                      : (is_v4 ? records_v3[i].sample_density
                               : ((is_v2 || is_v3) ? records_v2[i].sample_density : records_v1[i].sample_density)))))));
            uint32_t record_face_role =
                is_v12 ? records_v9[i].face_role
                      : (is_v11 ? records_v8[i].face_role
                      : (is_v10 ? records_v7[i].face_role
                      : (is_v9 ? records_v6[i].face_role
                      : (is_v8 ? records_v5[i].face_role
                      : ((is_v7 || is_v6 || is_v5) ? records_v4[i].face_role
                      : (is_v4 ? records_v3[i].face_role
                               : ((is_v2 || is_v3) ? records_v2[i].face_role : records_v1[i].face_role)))))));
            uint32_t record_quality_preset =
                is_v12 ? records_v9[i].quality_preset
                      : (is_v11 ? records_v8[i].quality_preset
                      : (is_v10 ? records_v7[i].quality_preset
                      : (is_v9 ? records_v6[i].quality_preset
                      : (is_v8 ? records_v5[i].quality_preset
                      : ((is_v7 || is_v6 || is_v5) ? records_v4[i].quality_preset
                      : (is_v4 ? records_v3[i].quality_preset
                               : ((is_v2 || is_v3) ? records_v2[i].quality_preset : records_v1[i].quality_preset)))))));
            uint32_t record_surface_id =
                is_v12 ? records_v9[i].surface_id
                      : (is_v11 ? records_v8[i].surface_id
                      : (is_v10 ? records_v7[i].surface_id
                      : (is_v9 ? records_v6[i].surface_id
                      : (is_v8 ? records_v5[i].surface_id
                      : ((is_v7 || is_v6 || is_v5) ? records_v4[i].surface_id
                      : (is_v4 ? records_v3[i].surface_id
                               : ((is_v2 || is_v3) ? records_v2[i].surface_id : records_v1[i].surface_id)))))));
            const char *record_name =
                is_v12 ? records_v9[i].name
                      : (is_v11 ? records_v8[i].name
                      : (is_v10 ? records_v7[i].name
                      : (is_v9 ? records_v6[i].name
                      : (is_v8 ? records_v5[i].name
                      : ((is_v7 || is_v6 || is_v5) ? records_v4[i].name
                      : (is_v4 ? records_v3[i].name
                               : ((is_v2 || is_v3) ? records_v2[i].name : records_v1[i].name)))))));
            if (record_logical_width != document_chunk.document.logical_width ||
                record_logical_height != document_chunk.document.logical_height ||
                record_sample_density != document_chunk.document.sample_density) {
                drawing_program_texture_project_dispose(project);
                free(root_data);
                return (CoreResult){ CORE_ERR_FORMAT, "texture surface record/document mismatch" };
            }
            result = drawing_program_texture_project_add_surface(project,
                                                                 record_name,
                                                                 record_logical_width,
                                                                 record_logical_height,
                                                                 record_sample_density,
                                                                 record_face_role,
                                                                 record_quality_preset,
                                                                 &surface_index);
            if (result.code != CORE_OK) {
                drawing_program_texture_project_dispose(project);
                free(root_data);
                return result;
            }
            surface = drawing_program_texture_project_surface_at_mut(project, surface_index);
            if (!surface || !surface->storage) {
                drawing_program_texture_project_dispose(project);
                free(root_data);
                return (CoreResult){ CORE_ERR_FORMAT, "texture surface storage allocation failed" };
            }
            storage = surface->storage;
            storage->document = document_chunk.document;
            drawing_program_layer_raster_store_dispose(&storage->layer_rasters);
            result = drawing_program_layer_raster_store_init_from_document(&storage->layer_rasters, &storage->document);
            if (result.code != CORE_OK) {
                drawing_program_texture_project_dispose(project);
                free(root_data);
                return result;
            }
            surface->surface_id = record_surface_id;
            surface->face_role = record_face_role;
            surface->quality_preset = record_quality_preset;
            surface->content_revision = 1u;
            if (is_v12) {
                surface->layout_offset_x = records_v9[i].layout_offset_x;
                surface->layout_offset_y = records_v9[i].layout_offset_y;
                surface->reflection_state = records_v9[i].reflection_state;
                surface->reflection_center_x = records_v9[i].reflection_center_x;
                surface->reflection_center_y = records_v9[i].reflection_center_y;
                surface->is_blank = records_v9[i].is_blank;
                surface->resize_locked = records_v9[i].resize_locked;
                surface->user_created = records_v9[i].user_created;
                surface->reflection_horizontal = records_v9[i].reflection_horizontal;
                surface->reflection_vertical = records_v9[i].reflection_vertical;
                surface->layer_opacity_entry_count = records_v9[i].layer_opacity_entry_count;
                surface->layer_role_entry_count = records_v9[i].layer_role_entry_count;
                surface->layer_material_intent_entry_count = records_v9[i].layer_material_intent_entry_count;
                memcpy(surface->layer_opacity_values,
                       records_v9[i].layer_opacity_values,
                       sizeof(surface->layer_opacity_values));
                memcpy(surface->layer_opacity_layer_ids,
                       records_v9[i].layer_opacity_layer_ids,
                       sizeof(surface->layer_opacity_layer_ids));
                memcpy(surface->layer_role_values,
                       records_v9[i].layer_role_values,
                       sizeof(surface->layer_role_values));
                memcpy(surface->layer_role_layer_ids,
                       records_v9[i].layer_role_layer_ids,
                       sizeof(surface->layer_role_layer_ids));
                memcpy(surface->layer_material_intent_values,
                       records_v9[i].layer_material_intent_values,
                       sizeof(surface->layer_material_intent_values));
                memcpy(surface->layer_material_intent_layer_ids,
                       records_v9[i].layer_material_intent_layer_ids,
                       sizeof(surface->layer_material_intent_layer_ids));
            } else if (is_v11) {
                surface->layout_offset_x = records_v8[i].layout_offset_x;
                surface->layout_offset_y = records_v8[i].layout_offset_y;
                surface->reflection_center_x = records_v8[i].reflection_center_x;
                surface->reflection_center_y = records_v8[i].reflection_center_y;
                surface->is_blank = records_v8[i].is_blank;
                surface->resize_locked = records_v8[i].resize_locked;
                surface->user_created = records_v8[i].user_created;
                surface->reflection_horizontal = records_v8[i].reflection_horizontal;
                surface->reflection_vertical = records_v8[i].reflection_vertical;
                surface->layer_opacity_entry_count = records_v8[i].layer_opacity_entry_count;
                surface->layer_role_entry_count = records_v8[i].layer_role_entry_count;
                surface->layer_material_intent_entry_count = records_v8[i].layer_material_intent_entry_count;
                memcpy(surface->layer_opacity_values,
                       records_v8[i].layer_opacity_values,
                       sizeof(surface->layer_opacity_values));
                memcpy(surface->layer_opacity_layer_ids,
                       records_v8[i].layer_opacity_layer_ids,
                       sizeof(surface->layer_opacity_layer_ids));
                memcpy(surface->layer_role_values,
                       records_v8[i].layer_role_values,
                       sizeof(surface->layer_role_values));
                memcpy(surface->layer_role_layer_ids,
                       records_v8[i].layer_role_layer_ids,
                       sizeof(surface->layer_role_layer_ids));
                memcpy(surface->layer_material_intent_values,
                       records_v8[i].layer_material_intent_values,
                       sizeof(surface->layer_material_intent_values));
                memcpy(surface->layer_material_intent_layer_ids,
                       records_v8[i].layer_material_intent_layer_ids,
                       sizeof(surface->layer_material_intent_layer_ids));
            } else if (is_v10) {
                surface->layout_offset_x = records_v7[i].layout_offset_x;
                surface->layout_offset_y = records_v7[i].layout_offset_y;
                surface->reflection_center_x = records_v7[i].reflection_center_x;
                surface->reflection_center_y = records_v7[i].reflection_center_y;
                surface->is_blank = records_v7[i].is_blank;
                surface->resize_locked = records_v7[i].resize_locked;
                surface->user_created = records_v7[i].user_created;
                surface->reflection_horizontal = records_v7[i].reflection_horizontal;
                surface->reflection_vertical = records_v7[i].reflection_vertical;
                surface->layer_opacity_entry_count = records_v7[i].layer_opacity_entry_count;
                surface->layer_role_entry_count = records_v7[i].layer_role_entry_count;
                memcpy(surface->layer_opacity_values,
                       records_v7[i].layer_opacity_values,
                       sizeof(surface->layer_opacity_values));
                memcpy(surface->layer_opacity_layer_ids,
                       records_v7[i].layer_opacity_layer_ids,
                       sizeof(surface->layer_opacity_layer_ids));
                memcpy(surface->layer_role_values,
                       records_v7[i].layer_role_values,
                       sizeof(surface->layer_role_values));
                memcpy(surface->layer_role_layer_ids,
                       records_v7[i].layer_role_layer_ids,
                       sizeof(surface->layer_role_layer_ids));
            } else if (is_v9) {
                surface->layout_offset_x = records_v6[i].layout_offset_x;
                surface->layout_offset_y = records_v6[i].layout_offset_y;
                surface->reflection_center_x = records_v6[i].reflection_center_x;
                surface->reflection_center_y = records_v6[i].reflection_center_y;
                surface->is_blank = records_v6[i].is_blank;
                surface->resize_locked = records_v6[i].resize_locked;
                surface->user_created = records_v6[i].user_created;
                surface->reflection_horizontal = records_v6[i].reflection_horizontal;
                surface->reflection_vertical = records_v6[i].reflection_vertical;
                surface->layer_opacity_entry_count = records_v6[i].layer_opacity_entry_count;
                memcpy(surface->layer_opacity_values,
                       records_v6[i].layer_opacity_values,
                       sizeof(surface->layer_opacity_values));
                memcpy(surface->layer_opacity_layer_ids,
                       records_v6[i].layer_opacity_layer_ids,
                       sizeof(surface->layer_opacity_layer_ids));
            } else if (is_v8) {
                surface->layout_offset_x = records_v5[i].layout_offset_x;
                surface->layout_offset_y = records_v5[i].layout_offset_y;
                surface->reflection_center_x = records_v5[i].reflection_center_x;
                surface->reflection_center_y = records_v5[i].reflection_center_y;
                surface->is_blank = records_v5[i].is_blank;
                surface->resize_locked = records_v5[i].resize_locked;
                surface->user_created = records_v5[i].user_created;
                surface->reflection_horizontal = records_v5[i].reflection_horizontal;
                surface->reflection_vertical = records_v5[i].reflection_vertical;
            } else if (is_v7 || is_v6 || is_v5) {
                surface->layout_offset_x = records_v4[i].layout_offset_x;
                surface->layout_offset_y = records_v4[i].layout_offset_y;
                surface->is_blank = records_v4[i].is_blank;
                surface->resize_locked = records_v4[i].resize_locked;
                surface->user_created = records_v4[i].user_created;
            } else if (is_v4) {
                surface->is_blank = records_v3[i].is_blank;
                surface->resize_locked = records_v3[i].resize_locked;
                surface->user_created = records_v3[i].user_created;
            } else if (is_v2 || is_v3) {
                surface->is_blank = records_v2[i].is_blank;
                surface->resize_locked = records_v2[i].resize_locked;
                surface->user_created = records_v2[i].user_created;
            }
            if (!is_v12) {
                drawing_program_reflection_state_seed_crosshair(&surface->reflection_state,
                                                                surface->reflection_center_x,
                                                                surface->reflection_center_y);
                drawing_program_reflection_state_set_crosshair_enabled(&surface->reflection_state,
                                                                       surface->reflection_horizontal,
                                                                       surface->reflection_vertical);
            }
            if (is_v12) {
                surface->semantic.layout_kind = records_v9[i].net_layout_kind;
                surface->semantic.net_slot = records_v9[i].net_slot;
                surface->semantic.orientation = records_v9[i].orientation;
                memcpy(surface->semantic.corner_ids, records_v9[i].corner_ids, sizeof(surface->semantic.corner_ids));
                memcpy(surface->semantic.edge_ids, records_v9[i].edge_ids, sizeof(surface->semantic.edge_ids));
                memcpy(surface->semantic.adjacent_face_roles,
                       records_v9[i].adjacent_face_roles,
                       sizeof(surface->semantic.adjacent_face_roles));
            } else if (is_v11) {
                surface->semantic.layout_kind = records_v8[i].net_layout_kind;
                surface->semantic.net_slot = records_v8[i].net_slot;
                surface->semantic.orientation = records_v8[i].orientation;
                memcpy(surface->semantic.corner_ids, records_v8[i].corner_ids, sizeof(surface->semantic.corner_ids));
                memcpy(surface->semantic.edge_ids, records_v8[i].edge_ids, sizeof(surface->semantic.edge_ids));
                memcpy(surface->semantic.adjacent_face_roles,
                       records_v8[i].adjacent_face_roles,
                       sizeof(surface->semantic.adjacent_face_roles));
            } else if (is_v10) {
                surface->semantic.layout_kind = records_v7[i].net_layout_kind;
                surface->semantic.net_slot = records_v7[i].net_slot;
                surface->semantic.orientation = records_v7[i].orientation;
                memcpy(surface->semantic.corner_ids, records_v7[i].corner_ids, sizeof(surface->semantic.corner_ids));
                memcpy(surface->semantic.edge_ids, records_v7[i].edge_ids, sizeof(surface->semantic.edge_ids));
                memcpy(surface->semantic.adjacent_face_roles,
                       records_v7[i].adjacent_face_roles,
                       sizeof(surface->semantic.adjacent_face_roles));
            } else if (is_v9) {
                surface->semantic.layout_kind = records_v6[i].net_layout_kind;
                surface->semantic.net_slot = records_v6[i].net_slot;
                surface->semantic.orientation = records_v6[i].orientation;
                memcpy(surface->semantic.corner_ids, records_v6[i].corner_ids, sizeof(surface->semantic.corner_ids));
                memcpy(surface->semantic.edge_ids, records_v6[i].edge_ids, sizeof(surface->semantic.edge_ids));
                memcpy(surface->semantic.adjacent_face_roles,
                       records_v6[i].adjacent_face_roles,
                       sizeof(surface->semantic.adjacent_face_roles));
            } else if (is_v8) {
                surface->semantic.layout_kind = records_v5[i].net_layout_kind;
                surface->semantic.net_slot = records_v5[i].net_slot;
                surface->semantic.orientation = records_v5[i].orientation;
                memcpy(surface->semantic.corner_ids, records_v5[i].corner_ids, sizeof(surface->semantic.corner_ids));
                memcpy(surface->semantic.edge_ids, records_v5[i].edge_ids, sizeof(surface->semantic.edge_ids));
                memcpy(surface->semantic.adjacent_face_roles,
                       records_v5[i].adjacent_face_roles,
                       sizeof(surface->semantic.adjacent_face_roles));
            } else if (is_v7 || is_v6 || is_v5) {
                surface->semantic.layout_kind = records_v4[i].net_layout_kind;
                surface->semantic.net_slot = records_v4[i].net_slot;
                surface->semantic.orientation = records_v4[i].orientation;
                memcpy(surface->semantic.corner_ids, records_v4[i].corner_ids, sizeof(surface->semantic.corner_ids));
                memcpy(surface->semantic.edge_ids, records_v4[i].edge_ids, sizeof(surface->semantic.edge_ids));
                memcpy(surface->semantic.adjacent_face_roles,
                       records_v4[i].adjacent_face_roles,
                       sizeof(surface->semantic.adjacent_face_roles));
            } else if (is_v4) {
                surface->semantic.layout_kind = records_v3[i].net_layout_kind;
                surface->semantic.net_slot = records_v3[i].net_slot;
                surface->semantic.orientation = records_v3[i].orientation;
                memcpy(surface->semantic.corner_ids, records_v3[i].corner_ids, sizeof(surface->semantic.corner_ids));
                memcpy(surface->semantic.edge_ids, records_v3[i].edge_ids, sizeof(surface->semantic.edge_ids));
                memcpy(surface->semantic.adjacent_face_roles,
                       records_v3[i].adjacent_face_roles,
                       sizeof(surface->semantic.adjacent_face_roles));
            } else {
                CoreResult semantic_result =
                    drawing_program_texture_net_seed_surface_semantic(project->primitive_kind, record_face_role, &surface->semantic);
                if (semantic_result.code != CORE_OK) {
                    drawing_program_texture_surface_semantic_clear(&surface->semantic);
                }
            }
            drawing_program_canvas_reflection_surface_clamp_to_document(surface, &storage->document);
            if (!((is_v12 || is_v11 || is_v10 || is_v9) && surface->layer_opacity_entry_count > 0u)) {
                texture_project_snapshot_seed_surface_layer_opacity_defaults(surface);
            }
            if (!((is_v12 || is_v11 || is_v10) && surface->layer_role_entry_count > 0u)) {
                texture_project_snapshot_seed_surface_layer_role_defaults(surface);
            }
            if (!((is_v12 || is_v11) && surface->layer_material_intent_entry_count > 0u)) {
                texture_project_snapshot_seed_surface_layer_material_intent_defaults(
                    surface, project->overlay_material_intent_kind);
            }
            memcpy(surface->name, record_name, sizeof(surface->name));
        }
        result = core_pack_reader_find_chunk(reader, "DTSR", i, &layer_chunk_info);
        if (result.code == CORE_OK) {
            layer_chunk_data = (uint8_t *)malloc((size_t)layer_chunk_info.size);
            if (!layer_chunk_data) {
                drawing_program_texture_project_dispose(project);
                free(root_data);
                return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate texture surface layer buffer" };
            }
            result = core_pack_reader_read_chunk_data(reader, &layer_chunk_info, layer_chunk_data, layer_chunk_info.size);
            if (result.code == CORE_OK) {
                result = texture_project_snapshot_apply_surface_layer_chunk(storage,
                                                                           layer_chunk_data,
                                                                           layer_chunk_info.size);
            }
            free(layer_chunk_data);
            if (result.code != CORE_OK) {
                drawing_program_texture_project_dispose(project);
                free(root_data);
                return result;
            }
        }
        if (!is_v2) {
            drawing_program_texture_project_refresh_surface_flags(project, surface_index);
        }
    }
    free(root_data);
    if (out_found) {
        *out_found = 1u;
    }
    return core_result_ok();
}
