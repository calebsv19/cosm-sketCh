#ifndef DRAWING_PROGRAM_TEXTURE_PROJECT_H
#define DRAWING_PROGRAM_TEXTURE_PROJECT_H

#include <stdint.h>

#include "core_base.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_texture_export_intent.h"
#include "drawing_program/drawing_program_layer_raster.h"
#include "drawing_program/drawing_program_texture_layer_material_intent.h"
#include "drawing_program/drawing_program_texture_layer_role.h"
#include "drawing_program/drawing_program_texture_net.h"
#include "drawing_program/drawing_program_texture_overlay_material_intent.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_TEXTURE_PROJECT_SCHEMA_VERSION 9u
#define DRAWING_PROGRAM_TEXTURE_PROJECT_NAME_CAPACITY 64u
#define DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY 64u
#define DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY 512u

typedef enum DrawingProgramTexturePrimitiveKind {
    DRAWING_PROGRAM_TEXTURE_PRIMITIVE_KIND_NONE = 0u,
    DRAWING_PROGRAM_TEXTURE_PRIMITIVE_KIND_PLANE = 1u,
    DRAWING_PROGRAM_TEXTURE_PRIMITIVE_KIND_RECT_PRISM = 2u
} DrawingProgramTexturePrimitiveKind;

typedef enum DrawingProgramTextureQualityPreset {
    DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD = 0u,
    DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_HIGH = 1u,
    DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_ULTRA = 2u,
    DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_CUSTOM = 3u
} DrawingProgramTextureQualityPreset;

typedef enum DrawingProgramTextureFaceRole {
    DRAWING_PROGRAM_TEXTURE_FACE_ROLE_UNSPECIFIED = 0u,
    DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT = 1u,
    DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BACK = 2u,
    DRAWING_PROGRAM_TEXTURE_FACE_ROLE_LEFT = 3u,
    DRAWING_PROGRAM_TEXTURE_FACE_ROLE_RIGHT = 4u,
    DRAWING_PROGRAM_TEXTURE_FACE_ROLE_TOP = 5u,
    DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BOTTOM = 6u
} DrawingProgramTextureFaceRole;

typedef enum DrawingProgramTextureBindingKind {
    DRAWING_PROGRAM_TEXTURE_BINDING_KIND_NONE = 0u,
    DRAWING_PROGRAM_TEXTURE_BINDING_KIND_SEPARATE_FACES = 1u,
    DRAWING_PROGRAM_TEXTURE_BINDING_KIND_ATLAS = 2u
} DrawingProgramTextureBindingKind;

typedef struct DrawingProgramTextureSurfaceStorage {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
} DrawingProgramTextureSurfaceStorage;

typedef struct DrawingProgramTextureSurface {
    uint32_t surface_id;
    uint32_t face_role;
    uint32_t quality_preset;
    uint64_t content_revision;
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
    char name[DRAWING_PROGRAM_TEXTURE_PROJECT_NAME_CAPACITY];
    uint8_t layer_opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t layer_opacity_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t layer_role_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t layer_role_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t layer_material_intent_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t layer_material_intent_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    DrawingProgramTextureSurfaceSemantic semantic;
    DrawingProgramTextureSurfaceStorage *storage;
} DrawingProgramTextureSurface;

typedef struct DrawingProgramTextureProject {
    uint32_t schema_version;
    uint32_t primitive_kind;
    uint32_t net_layout_kind;
    uint32_t quality_preset;
    uint32_t export_binding_kind;
    uint32_t export_intent_kind;
    uint32_t overlay_material_intent_kind;
    uint32_t surface_count;
    uint32_t surface_capacity;
    uint32_t active_surface_index;
    uint32_t next_surface_id;
    uint64_t runtime_cache_epoch;
    char source_scene_id[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
    char source_object_id[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
    char source_scene_path[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
    DrawingProgramTextureSurface *surfaces;
} DrawingProgramTextureProject;

void drawing_program_texture_project_dispose(DrawingProgramTextureProject *project);
CoreResult drawing_program_texture_project_init_empty(DrawingProgramTextureProject *project);
CoreResult drawing_program_texture_project_init_single_surface(
    DrawingProgramTextureProject *project,
    const DrawingProgramDocument *document,
    const DrawingProgramLayerRasterStore *layer_rasters,
    const char *surface_name,
    uint32_t quality_preset);
CoreResult drawing_program_texture_project_add_surface(
    DrawingProgramTextureProject *project,
    const char *surface_name,
    uint32_t logical_width,
    uint32_t logical_height,
    uint32_t sample_density,
    uint32_t face_role,
    uint32_t quality_preset,
    uint32_t *out_surface_index);
CoreResult drawing_program_texture_project_capture_surface(
    DrawingProgramTextureProject *project,
    uint32_t surface_index,
    const DrawingProgramDocument *document,
    const DrawingProgramLayerRasterStore *layer_rasters,
    const uint32_t *layer_opacity_layer_ids,
    const uint8_t *layer_opacity_values,
    uint32_t layer_opacity_entry_count,
    const uint32_t *layer_role_layer_ids,
    const uint8_t *layer_role_values,
    uint32_t layer_role_entry_count,
    const uint32_t *layer_material_intent_layer_ids,
    const uint8_t *layer_material_intent_values,
    uint32_t layer_material_intent_entry_count);
CoreResult drawing_program_texture_project_remove_surface(
    DrawingProgramTextureProject *project,
    uint32_t surface_index);
CoreResult drawing_program_texture_project_clone_surface(
    const DrawingProgramTextureProject *project,
    uint32_t surface_index,
    DrawingProgramDocument *out_document,
    DrawingProgramLayerRasterStore *out_layer_rasters);
CoreResult drawing_program_texture_project_select_active_surface(
    DrawingProgramTextureProject *project,
    uint32_t surface_index);
const DrawingProgramTextureSurface *drawing_program_texture_project_surface_at(
    const DrawingProgramTextureProject *project,
    uint32_t surface_index);
DrawingProgramTextureSurface *drawing_program_texture_project_surface_at_mut(
    DrawingProgramTextureProject *project,
    uint32_t surface_index);
void drawing_program_texture_project_refresh_surface_flags(
    DrawingProgramTextureProject *project,
    uint32_t surface_index);
CoreResult drawing_program_texture_project_set_surface_semantic(
    DrawingProgramTextureProject *project,
    uint32_t surface_index,
    const DrawingProgramTextureSurfaceSemantic *semantic);
CoreResult drawing_program_texture_project_set_surface_layout_offset(
    DrawingProgramTextureProject *project,
    uint32_t surface_index,
    float offset_x,
    float offset_y);
CoreResult drawing_program_texture_project_reset_surface_layout_offset(
    DrawingProgramTextureProject *project,
    uint32_t surface_index);
void drawing_program_texture_project_reset_all_surface_layout_offsets(
    DrawingProgramTextureProject *project);
void drawing_program_texture_project_collect_surface_layer_opacity_by_index(
    const DrawingProgramTextureProject *project,
    uint32_t surface_index,
    uint8_t *out_opacity,
    uint32_t out_count);
void drawing_program_texture_project_collect_surface_layer_role_by_index(
    const DrawingProgramTextureProject *project,
    uint32_t surface_index,
    uint8_t *out_role_kind,
    uint32_t out_count);
void drawing_program_texture_project_collect_surface_layer_material_intent_by_index(
    const DrawingProgramTextureProject *project,
    uint32_t surface_index,
    uint8_t *out_intent_kind,
    uint32_t out_count);
void drawing_program_texture_project_set_surface_layer_role(
    DrawingProgramTextureProject *project,
    uint32_t surface_index,
    uint32_t layer_id,
    uint32_t role_kind);
void drawing_program_texture_project_set_surface_layer_material_intent(
    DrawingProgramTextureProject *project,
    uint32_t surface_index,
    uint32_t layer_id,
    uint32_t intent_kind);
uint32_t drawing_program_texture_project_default_sample_density(uint32_t quality_preset);
const char *drawing_program_texture_project_face_role_name(uint32_t face_role);
const char *drawing_program_texture_project_quality_preset_name(uint32_t quality_preset);

#ifdef __cplusplus
}
#endif

#endif
