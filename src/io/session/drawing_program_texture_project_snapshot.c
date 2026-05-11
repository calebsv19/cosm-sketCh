#include "drawing_program_snapshot_internal.h"

#include <stdlib.h>
#include <string.h>

#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_texture_project.h"

enum {
    DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V1 = 1u,
    DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V2 = 2u,
    DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V3 = 3u,
    DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V4 = 4u,
    DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V5 = 5u,
    DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V6 = 6u,
    DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V7 = 7u,
    DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V8 = 8u,
    DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V9 = 9u,
    DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V10 = 10u,
    DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V11 = 11u,
    DRAWING_PROGRAM_TEXTURE_SURFACE_DOCUMENT_CHUNK_VERSION_V1 = 1u,
    DRAWING_PROGRAM_TEXTURE_SURFACE_LAYER_CHUNK_VERSION_V2 = 2u
};

typedef struct DrawingProgramTextureProjectChunkHeaderV1 {
    uint32_t version;
    uint32_t primitive_kind;
    uint32_t quality_preset;
    uint32_t export_binding_kind;
    uint32_t surface_count;
    uint32_t active_surface_index;
    uint32_t next_surface_id;
    uint32_t reserved0;
    char source_scene_id[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
    char source_object_id[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
} DrawingProgramTextureProjectChunkHeaderV1;

typedef struct DrawingProgramTextureProjectChunkHeaderV3 {
    uint32_t version;
    uint32_t primitive_kind;
    uint32_t quality_preset;
    uint32_t export_binding_kind;
    uint32_t surface_count;
    uint32_t active_surface_index;
    uint32_t next_surface_id;
    uint32_t reserved0;
    char source_scene_id[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
    char source_object_id[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
    char source_scene_path[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
} DrawingProgramTextureProjectChunkHeaderV3;

typedef struct DrawingProgramTextureProjectChunkHeaderV4 {
    uint32_t version;
    uint32_t primitive_kind;
    uint32_t net_layout_kind;
    uint32_t quality_preset;
    uint32_t export_binding_kind;
    uint32_t surface_count;
    uint32_t active_surface_index;
    uint32_t next_surface_id;
    char source_scene_id[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
    char source_object_id[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
    char source_scene_path[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
} DrawingProgramTextureProjectChunkHeaderV4;

typedef struct DrawingProgramTextureProjectChunkHeaderV6 {
    uint32_t version;
    uint32_t primitive_kind;
    uint32_t net_layout_kind;
    uint32_t quality_preset;
    uint32_t export_binding_kind;
    uint32_t export_intent_kind;
    uint32_t surface_count;
    uint32_t active_surface_index;
    uint32_t next_surface_id;
    char source_scene_id[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
    char source_object_id[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
    char source_scene_path[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
} DrawingProgramTextureProjectChunkHeaderV6;

typedef struct DrawingProgramTextureProjectChunkHeaderV7 {
    uint32_t version;
    uint32_t primitive_kind;
    uint32_t net_layout_kind;
    uint32_t quality_preset;
    uint32_t export_binding_kind;
    uint32_t export_intent_kind;
    uint32_t overlay_material_intent_kind;
    uint32_t surface_count;
    uint32_t active_surface_index;
    uint32_t next_surface_id;
    char source_scene_id[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
    char source_object_id[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
    char source_scene_path[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
} DrawingProgramTextureProjectChunkHeaderV7;

typedef DrawingProgramTextureProjectChunkHeaderV7 DrawingProgramTextureProjectChunkHeaderV8;
typedef DrawingProgramTextureProjectChunkHeaderV7 DrawingProgramTextureProjectChunkHeaderV9;
typedef DrawingProgramTextureProjectChunkHeaderV7 DrawingProgramTextureProjectChunkHeaderV10;
typedef DrawingProgramTextureProjectChunkHeaderV7 DrawingProgramTextureProjectChunkHeaderV11;

typedef struct DrawingProgramTextureProjectSurfaceRecordV1 {
    uint32_t surface_id;
    uint32_t face_role;
    uint32_t quality_preset;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    char name[DRAWING_PROGRAM_TEXTURE_PROJECT_NAME_CAPACITY];
} DrawingProgramTextureProjectSurfaceRecordV1;

typedef struct DrawingProgramTextureProjectSurfaceRecordV2 {
    uint32_t surface_id;
    uint32_t face_role;
    uint32_t quality_preset;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    uint8_t is_blank;
    uint8_t resize_locked;
    uint8_t user_created;
    uint8_t reserved0;
    char name[DRAWING_PROGRAM_TEXTURE_PROJECT_NAME_CAPACITY];
} DrawingProgramTextureProjectSurfaceRecordV2;

typedef struct DrawingProgramTextureProjectSurfaceRecordV3 {
    uint32_t surface_id;
    uint32_t face_role;
    uint32_t quality_preset;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    uint8_t is_blank;
    uint8_t resize_locked;
    uint8_t user_created;
    uint8_t reserved0;
    uint32_t net_layout_kind;
    uint32_t net_slot;
    uint32_t orientation;
    uint8_t corner_ids[DRAWING_PROGRAM_TEXTURE_NET_FACE_CORNER_COUNT];
    uint8_t edge_ids[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    uint8_t adjacent_face_roles[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    uint8_t reserved1[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    char name[DRAWING_PROGRAM_TEXTURE_PROJECT_NAME_CAPACITY];
} DrawingProgramTextureProjectSurfaceRecordV3;

typedef struct DrawingProgramTextureProjectSurfaceRecordV4 {
    uint32_t surface_id;
    uint32_t face_role;
    uint32_t quality_preset;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    float layout_offset_x;
    float layout_offset_y;
    uint8_t is_blank;
    uint8_t resize_locked;
    uint8_t user_created;
    uint8_t reserved0;
    uint32_t net_layout_kind;
    uint32_t net_slot;
    uint32_t orientation;
    uint8_t corner_ids[DRAWING_PROGRAM_TEXTURE_NET_FACE_CORNER_COUNT];
    uint8_t edge_ids[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    uint8_t adjacent_face_roles[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    uint8_t reserved1[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    char name[DRAWING_PROGRAM_TEXTURE_PROJECT_NAME_CAPACITY];
} DrawingProgramTextureProjectSurfaceRecordV4;

typedef struct DrawingProgramTextureProjectSurfaceRecordV5 {
    uint32_t surface_id;
    uint32_t face_role;
    uint32_t quality_preset;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    float layout_offset_x;
    float layout_offset_y;
    uint32_t reflection_center_x;
    uint32_t reflection_center_y;
    uint8_t is_blank;
    uint8_t resize_locked;
    uint8_t user_created;
    uint8_t reflection_horizontal;
    uint8_t reflection_vertical;
    uint8_t reserved0;
    uint8_t reserved1;
    uint8_t reserved2;
    uint32_t net_layout_kind;
    uint32_t net_slot;
    uint32_t orientation;
    uint8_t corner_ids[DRAWING_PROGRAM_TEXTURE_NET_FACE_CORNER_COUNT];
    uint8_t edge_ids[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    uint8_t adjacent_face_roles[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    uint8_t reserved3[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    char name[DRAWING_PROGRAM_TEXTURE_PROJECT_NAME_CAPACITY];
} DrawingProgramTextureProjectSurfaceRecordV5;

typedef struct DrawingProgramTextureProjectSurfaceRecordV6 {
    uint32_t surface_id;
    uint32_t face_role;
    uint32_t quality_preset;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    float layout_offset_x;
    float layout_offset_y;
    uint32_t reflection_center_x;
    uint32_t reflection_center_y;
    uint8_t is_blank;
    uint8_t resize_locked;
    uint8_t user_created;
    uint8_t reflection_horizontal;
    uint8_t reflection_vertical;
    uint8_t layer_opacity_entry_count;
    uint8_t reserved0;
    uint32_t net_layout_kind;
    uint32_t net_slot;
    uint32_t orientation;
    uint8_t corner_ids[DRAWING_PROGRAM_TEXTURE_NET_FACE_CORNER_COUNT];
    uint8_t edge_ids[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    uint8_t adjacent_face_roles[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    uint8_t reserved3[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    char name[DRAWING_PROGRAM_TEXTURE_PROJECT_NAME_CAPACITY];
    uint8_t layer_opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t layer_opacity_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
} DrawingProgramTextureProjectSurfaceRecordV6;

typedef struct DrawingProgramTextureProjectSurfaceRecordV7 {
    uint32_t surface_id;
    uint32_t face_role;
    uint32_t quality_preset;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    float layout_offset_x;
    float layout_offset_y;
    uint32_t reflection_center_x;
    uint32_t reflection_center_y;
    uint8_t is_blank;
    uint8_t resize_locked;
    uint8_t user_created;
    uint8_t reflection_horizontal;
    uint8_t reflection_vertical;
    uint8_t layer_opacity_entry_count;
    uint8_t layer_role_entry_count;
    uint32_t net_layout_kind;
    uint32_t net_slot;
    uint32_t orientation;
    uint8_t corner_ids[DRAWING_PROGRAM_TEXTURE_NET_FACE_CORNER_COUNT];
    uint8_t edge_ids[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    uint8_t adjacent_face_roles[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    uint8_t reserved3[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    char name[DRAWING_PROGRAM_TEXTURE_PROJECT_NAME_CAPACITY];
    uint8_t layer_opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t layer_opacity_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t layer_role_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t layer_role_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
} DrawingProgramTextureProjectSurfaceRecordV7;

typedef struct DrawingProgramTextureProjectSurfaceRecordV8 {
    uint32_t surface_id;
    uint32_t face_role;
    uint32_t quality_preset;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    float layout_offset_x;
    float layout_offset_y;
    uint32_t reflection_center_x;
    uint32_t reflection_center_y;
    uint8_t is_blank;
    uint8_t resize_locked;
    uint8_t user_created;
    uint8_t reflection_horizontal;
    uint8_t reflection_vertical;
    uint8_t layer_opacity_entry_count;
    uint8_t layer_role_entry_count;
    uint8_t layer_material_intent_entry_count;
    uint32_t net_layout_kind;
    uint32_t net_slot;
    uint32_t orientation;
    uint8_t corner_ids[DRAWING_PROGRAM_TEXTURE_NET_FACE_CORNER_COUNT];
    uint8_t edge_ids[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    uint8_t adjacent_face_roles[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    uint8_t reserved3[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    char name[DRAWING_PROGRAM_TEXTURE_PROJECT_NAME_CAPACITY];
    uint8_t layer_opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t layer_opacity_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t layer_role_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t layer_role_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t layer_material_intent_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t layer_material_intent_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
} DrawingProgramTextureProjectSurfaceRecordV8;

typedef struct DrawingProgramTextureSurfaceDocumentChunkV1 {
    uint32_t version;
    DrawingProgramDocument document;
} DrawingProgramTextureSurfaceDocumentChunkV1;

static CoreResult texture_project_snapshot_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static int texture_project_document_bounds_valid(const DrawingProgramDocument *document) {
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

static void texture_project_snapshot_seed_surface_layer_opacity_defaults(
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

static void texture_project_snapshot_seed_surface_layer_role_defaults(
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

static void texture_project_snapshot_seed_surface_layer_material_intent_defaults(
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

static CoreResult texture_project_snapshot_write_surface_layer_chunk(
    CorePackWriter *writer,
    const DrawingProgramTextureSurfaceStorage *storage) {
    uint64_t header_bytes;
    uint64_t layer_bytes;
    uint64_t payload_size;
    uint8_t *payload = 0;
    uint8_t *cursor = 0;
    uint32_t i;
    const DrawingProgramDocument *document = 0;
    const DrawingProgramLayerRasterStore *layer_rasters = 0;
    uint32_t sample_count = 0u;
    uint64_t sample_bytes = 0u;
    if (!writer || !storage) {
        return texture_project_snapshot_invalid("invalid texture surface layer chunk write request");
    }
    document = &storage->document;
    layer_rasters = &storage->layer_rasters;
    if (!texture_project_document_bounds_valid(document)) {
        return texture_project_snapshot_invalid("invalid texture surface document while writing layers");
    }
    sample_count = document->raster_sample_count;
    sample_bytes = (uint64_t)sample_count * (uint64_t)sizeof(DrawingProgramRasterSample);
    header_bytes = (uint64_t)(sizeof(uint32_t) * 5u);
    layer_bytes = (uint64_t)(sizeof(uint32_t) * 2u) + sample_bytes;
    payload_size = header_bytes + (layer_bytes * (uint64_t)document->layer_count);
    payload = (uint8_t *)malloc((size_t)payload_size);
    if (!payload) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate texture surface layer payload" };
    }
    cursor = payload;
    memcpy(cursor, &(uint32_t){ DRAWING_PROGRAM_TEXTURE_SURFACE_LAYER_CHUNK_VERSION_V2 }, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &document->raster_width, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &document->raster_height, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &document->raster_sample_count, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &document->layer_count, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    for (i = 0u; i < document->layer_count; ++i) {
        const uint32_t layer_id = document->layers[i].layer_id;
        const DrawingProgramRasterSample *samples = 0;
        uint32_t exported_sample_count = 0u;
        CoreResult export_result = { CORE_ERR_NOT_FOUND, "texture layer raster export unavailable" };
        memcpy(cursor, &layer_id, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        memcpy(cursor, &sample_count, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        if (i == 0u) {
            memcpy(cursor, document->raster_samples, (size_t)sample_bytes);
            cursor += sample_bytes;
            continue;
        }
        export_result = drawing_program_layer_raster_store_export_layer(layer_rasters,
                                                                        layer_id,
                                                                        &samples,
                                                                        &exported_sample_count);
        if (export_result.code == CORE_OK &&
            samples &&
            exported_sample_count == sample_count) {
            memcpy(cursor, samples, (size_t)sample_bytes);
        } else {
            uint32_t sample_i;
            DrawingProgramRasterSample *write_samples = (DrawingProgramRasterSample *)cursor;
            DrawingProgramRasterSample empty_sample = drawing_program_color_eraser_value();
            for (sample_i = 0u; sample_i < sample_count; ++sample_i) {
                write_samples[sample_i] = empty_sample;
            }
        }
        cursor += sample_bytes;
    }
    {
        CoreResult result = core_pack_writer_add_chunk(writer, "DTSR", payload, payload_size);
        free(payload);
        return result;
    }
}

static CoreResult texture_project_snapshot_apply_surface_layer_chunk(
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
            (uint64_t)(end - cursor) < ((uint64_t)entry_sample_count * (uint64_t)sizeof(DrawingProgramRasterSample))) {
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
            if (storage->document.layer_count > 0u &&
                layer_id == storage->document.layers[0].layer_id) {
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

CoreResult drawing_program_texture_project_snapshot_write(
    CorePackWriter *writer,
    const DrawingProgramTextureProject *project) {
    DrawingProgramTextureProjectChunkHeaderV11 header;
    DrawingProgramTextureProjectSurfaceRecordV8 *records = 0;
    CoreResult result;
    uint32_t i;
    if (!writer || !project || project->surface_count == 0u) {
        return texture_project_snapshot_invalid("invalid texture project snapshot write request");
    }
    memset(&header, 0, sizeof(header));
    header.version = DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V11;
    header.primitive_kind = project->primitive_kind;
    header.net_layout_kind = project->net_layout_kind;
    header.quality_preset = project->quality_preset;
    header.export_binding_kind = project->export_binding_kind;
    header.export_intent_kind = project->export_intent_kind;
    header.overlay_material_intent_kind = project->overlay_material_intent_kind;
    header.surface_count = project->surface_count;
    header.active_surface_index = project->active_surface_index;
    header.next_surface_id = project->next_surface_id;
    memcpy(header.source_scene_id, project->source_scene_id, sizeof(header.source_scene_id));
    memcpy(header.source_object_id, project->source_object_id, sizeof(header.source_object_id));
    memcpy(header.source_scene_path, project->source_scene_path, sizeof(header.source_scene_path));
    records = (DrawingProgramTextureProjectSurfaceRecordV8 *)calloc(project->surface_count, sizeof(*records));
    if (!records) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate texture project surface records" };
    }
    for (i = 0u; i < project->surface_count; ++i) {
        const DrawingProgramTextureSurface *surface = drawing_program_texture_project_surface_at(project, i);
        const DrawingProgramDocument *document = 0;
        DrawingProgramTextureSurfaceDocumentChunkV1 document_chunk;
        if (!surface || !surface->storage) {
            free(records);
            return (CoreResult){ CORE_ERR_FORMAT, "texture project surface missing storage" };
        }
        document = &surface->storage->document;
        if (!texture_project_document_bounds_valid(document)) {
            free(records);
            return (CoreResult){ CORE_ERR_FORMAT, "texture project surface document invalid" };
        }
        records[i].surface_id = surface->surface_id;
        records[i].face_role = surface->face_role;
        records[i].quality_preset = surface->quality_preset;
        records[i].logical_width = document->logical_width;
        records[i].logical_height = document->logical_height;
        records[i].sample_density = document->sample_density;
        records[i].layout_offset_x = surface->layout_offset_x;
        records[i].layout_offset_y = surface->layout_offset_y;
        records[i].reflection_center_x = surface->reflection_center_x;
        records[i].reflection_center_y = surface->reflection_center_y;
        records[i].is_blank = surface->is_blank;
        records[i].resize_locked = surface->resize_locked;
        records[i].user_created = surface->user_created;
        records[i].reflection_horizontal = surface->reflection_horizontal;
        records[i].reflection_vertical = surface->reflection_vertical;
        records[i].layer_opacity_entry_count = surface->layer_opacity_entry_count;
        records[i].layer_role_entry_count = surface->layer_role_entry_count;
        records[i].layer_material_intent_entry_count = surface->layer_material_intent_entry_count;
        records[i].net_layout_kind = surface->semantic.layout_kind;
        records[i].net_slot = surface->semantic.net_slot;
        records[i].orientation = surface->semantic.orientation;
        memcpy(records[i].corner_ids, surface->semantic.corner_ids, sizeof(records[i].corner_ids));
        memcpy(records[i].edge_ids, surface->semantic.edge_ids, sizeof(records[i].edge_ids));
        memcpy(records[i].adjacent_face_roles,
               surface->semantic.adjacent_face_roles,
               sizeof(records[i].adjacent_face_roles));
        memcpy(records[i].name, surface->name, sizeof(records[i].name));
        memcpy(records[i].layer_opacity_values,
               surface->layer_opacity_values,
               sizeof(records[i].layer_opacity_values));
        memcpy(records[i].layer_opacity_layer_ids,
               surface->layer_opacity_layer_ids,
               sizeof(records[i].layer_opacity_layer_ids));
        memcpy(records[i].layer_role_values,
               surface->layer_role_values,
               sizeof(records[i].layer_role_values));
        memcpy(records[i].layer_role_layer_ids,
               surface->layer_role_layer_ids,
               sizeof(records[i].layer_role_layer_ids));
        memcpy(records[i].layer_material_intent_values,
               surface->layer_material_intent_values,
               sizeof(records[i].layer_material_intent_values));
        memcpy(records[i].layer_material_intent_layer_ids,
               surface->layer_material_intent_layer_ids,
               sizeof(records[i].layer_material_intent_layer_ids));
        memset(&document_chunk, 0, sizeof(document_chunk));
        document_chunk.version = DRAWING_PROGRAM_TEXTURE_SURFACE_DOCUMENT_CHUNK_VERSION_V1;
        document_chunk.document = *document;
        result = core_pack_writer_add_chunk(writer, "DTSD", &document_chunk, (uint64_t)sizeof(document_chunk));
        if (result.code != CORE_OK) {
            free(records);
            return result;
        }
        result = texture_project_snapshot_write_surface_layer_chunk(writer, surface->storage);
        if (result.code != CORE_OK) {
            free(records);
            return result;
        }
    }
    {
        const uint64_t payload_size =
            (uint64_t)sizeof(header) + ((uint64_t)project->surface_count * (uint64_t)sizeof(*records));
        uint8_t *payload = (uint8_t *)malloc((size_t)payload_size);
        if (!payload) {
            free(records);
            return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate texture project root payload" };
        }
        memcpy(payload, &header, sizeof(header));
        memcpy(payload + sizeof(header), records, (size_t)project->surface_count * sizeof(*records));
        result = core_pack_writer_add_chunk(writer, "DPTP", payload, payload_size);
        free(payload);
    }
    free(records);
    return result;
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
    const DrawingProgramTextureProjectSurfaceRecordV1 *records_v1 = 0;
    const DrawingProgramTextureProjectSurfaceRecordV2 *records_v2 = 0;
    const DrawingProgramTextureProjectSurfaceRecordV3 *records_v3 = 0;
    const DrawingProgramTextureProjectSurfaceRecordV4 *records_v4 = 0;
    const DrawingProgramTextureProjectSurfaceRecordV5 *records_v5 = 0;
    const DrawingProgramTextureProjectSurfaceRecordV6 *records_v6 = 0;
    const DrawingProgramTextureProjectSurfaceRecordV7 *records_v7 = 0;
    const DrawingProgramTextureProjectSurfaceRecordV8 *records_v8 = 0;
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
    if (is_v11) {
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
    expected_root_size = is_v11 ? expected_v11_size
                               : (is_v10 ? expected_v10_size
                               : (is_v9 ? expected_v9_size
                               : (is_v8 ? expected_v8_size
                               : (is_v7 ? expected_v7_size
                               : (is_v6 ? expected_v6_size
                               : (is_v5 ? expected_v5_size
                                        : (is_v4 ? expected_v4_size
                                                 : (is_v3 ? expected_v3_size
                                                          : (is_v2 ? expected_v2_size : expected_v1_size)))))))));
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
        header_v11.version != DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V11) ||
        (root_surface_count == 0u) ||
        (root_active_surface_index >= root_surface_count) ||
        root_chunk.size != expected_root_size) {
        free(root_data);
        return (CoreResult){ CORE_ERR_FORMAT, "texture project root chunk invalid" };
    }
    if (is_v11) {
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
    for (i = 0u;
         i < root_surface_count;
         ++i) {
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
        if (result.code != CORE_OK || document_chunk.version != DRAWING_PROGRAM_TEXTURE_SURFACE_DOCUMENT_CHUNK_VERSION_V1 ||
            !texture_project_document_bounds_valid(&document_chunk.document)) {
            drawing_program_texture_project_dispose(project);
            free(root_data);
            return (CoreResult){ CORE_ERR_FORMAT, "texture surface document payload invalid" };
        }
        uint32_t record_logical_width =
            is_v11 ? records_v8[i].logical_width
                  : (is_v10 ? records_v7[i].logical_width
                  : (is_v9 ? records_v6[i].logical_width
                  : (is_v8 ? records_v5[i].logical_width
                  : ((is_v7 || is_v6 || is_v5) ? records_v4[i].logical_width
                  : (is_v4 ? records_v3[i].logical_width
                           : ((is_v2 || is_v3) ? records_v2[i].logical_width : records_v1[i].logical_width))))));
        uint32_t record_logical_height =
            is_v11 ? records_v8[i].logical_height
                  : (is_v10 ? records_v7[i].logical_height
                  : (is_v9 ? records_v6[i].logical_height
                  : (is_v8 ? records_v5[i].logical_height
                  : ((is_v7 || is_v6 || is_v5) ? records_v4[i].logical_height
                  : (is_v4 ? records_v3[i].logical_height
                           : ((is_v2 || is_v3) ? records_v2[i].logical_height : records_v1[i].logical_height))))));
        uint32_t record_sample_density =
            is_v11 ? records_v8[i].sample_density
                  : (is_v10 ? records_v7[i].sample_density
                  : (is_v9 ? records_v6[i].sample_density
                  : (is_v8 ? records_v5[i].sample_density
                  : ((is_v7 || is_v6 || is_v5) ? records_v4[i].sample_density
                  : (is_v4 ? records_v3[i].sample_density
                           : ((is_v2 || is_v3) ? records_v2[i].sample_density : records_v1[i].sample_density))))));
        uint32_t record_face_role =
            is_v11 ? records_v8[i].face_role
                  : (is_v10 ? records_v7[i].face_role
                  : (is_v9 ? records_v6[i].face_role
                  : (is_v8 ? records_v5[i].face_role
                  : ((is_v7 || is_v6 || is_v5) ? records_v4[i].face_role
                  : (is_v4 ? records_v3[i].face_role
                           : ((is_v2 || is_v3) ? records_v2[i].face_role : records_v1[i].face_role))))));
        uint32_t record_quality_preset =
            is_v11 ? records_v8[i].quality_preset
                  : (is_v10 ? records_v7[i].quality_preset
                  : (is_v9 ? records_v6[i].quality_preset
                  : (is_v8 ? records_v5[i].quality_preset
                  : ((is_v7 || is_v6 || is_v5) ? records_v4[i].quality_preset
                  : (is_v4 ? records_v3[i].quality_preset
                           : ((is_v2 || is_v3) ? records_v2[i].quality_preset : records_v1[i].quality_preset))))));
        uint32_t record_surface_id =
            is_v11 ? records_v8[i].surface_id
                  : (is_v10 ? records_v7[i].surface_id
                  : (is_v9 ? records_v6[i].surface_id
                  : (is_v8 ? records_v5[i].surface_id
                  : ((is_v7 || is_v6 || is_v5) ? records_v4[i].surface_id
                  : (is_v4 ? records_v3[i].surface_id
                           : ((is_v2 || is_v3) ? records_v2[i].surface_id : records_v1[i].surface_id))))));
        const char *record_name =
            is_v11 ? records_v8[i].name
                  : (is_v10 ? records_v7[i].name
                  : (is_v9 ? records_v6[i].name
                  : (is_v8 ? records_v5[i].name
                  : ((is_v7 || is_v6 || is_v5) ? records_v4[i].name
                  : (is_v4 ? records_v3[i].name : ((is_v2 || is_v3) ? records_v2[i].name : records_v1[i].name))))));
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
        if (is_v11) {
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
        if (is_v11) {
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
        if (!((is_v11 || is_v10 || is_v9) && surface->layer_opacity_entry_count > 0u)) {
            texture_project_snapshot_seed_surface_layer_opacity_defaults(surface);
        }
        if (!((is_v11 || is_v10) && surface->layer_role_entry_count > 0u)) {
            texture_project_snapshot_seed_surface_layer_role_defaults(surface);
        }
        if (!(is_v11 && surface->layer_material_intent_entry_count > 0u)) {
            texture_project_snapshot_seed_surface_layer_material_intent_defaults(
                surface, project->overlay_material_intent_kind);
        }
        memcpy(surface->name, record_name, sizeof(surface->name));
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
