#ifndef DRAWING_PROGRAM_TEXTURE_PROJECT_SNAPSHOT_INTERNAL_H
#define DRAWING_PROGRAM_TEXTURE_PROJECT_SNAPSHOT_INTERNAL_H

#include "drawing_program_snapshot_internal.h"

#include "drawing_program/drawing_program_canvas_reflection.h"

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
    DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V12 = 12u,
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
typedef DrawingProgramTextureProjectChunkHeaderV7 DrawingProgramTextureProjectChunkHeaderV12;

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

typedef struct DrawingProgramTextureProjectSurfaceRecordV9 {
    uint32_t surface_id;
    uint32_t face_role;
    uint32_t quality_preset;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    float layout_offset_x;
    float layout_offset_y;
    DrawingProgramReflectionState reflection_state;
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
} DrawingProgramTextureProjectSurfaceRecordV9;

typedef struct DrawingProgramTextureSurfaceDocumentChunkV1 {
    uint32_t version;
    DrawingProgramDocument document;
} DrawingProgramTextureSurfaceDocumentChunkV1;

CoreResult texture_project_snapshot_invalid(const char *message);
int texture_project_document_bounds_valid(const DrawingProgramDocument *document);
void texture_project_snapshot_seed_surface_layer_opacity_defaults(
    DrawingProgramTextureSurface *surface);
void texture_project_snapshot_seed_surface_layer_role_defaults(
    DrawingProgramTextureSurface *surface);
void texture_project_snapshot_seed_surface_layer_material_intent_defaults(
    DrawingProgramTextureSurface *surface,
    uint32_t overlay_material_intent_kind);
CoreResult texture_project_snapshot_write_surface_layer_chunk(
    CorePackWriter *writer,
    const DrawingProgramTextureSurfaceStorage *storage);
CoreResult texture_project_snapshot_apply_surface_layer_chunk(
    DrawingProgramTextureSurfaceStorage *storage,
    const void *chunk_data,
    uint64_t chunk_size);

#endif
