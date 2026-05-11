#ifndef DRAWING_PROGRAM_TEXTURE_PROJECT_SESSION_H
#define DRAWING_PROGRAM_TEXTURE_PROJECT_SESSION_H

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

CoreResult drawing_program_texture_project_session_init_from_current_document(
    struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_texture_project_session_seed_blank(
    struct DrawingProgramAppContext *ctx,
    uint32_t logical_width,
    uint32_t logical_height,
    uint32_t quality_preset);
CoreResult drawing_program_texture_project_session_commit_active_surface(
    struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_texture_project_session_select_surface(
    struct DrawingProgramAppContext *ctx,
    uint32_t surface_index);
CoreResult drawing_program_texture_project_session_import_scene_object(
    struct DrawingProgramAppContext *ctx,
    const char *scene_json_path,
    const char *object_id,
    uint32_t quality_preset);
CoreResult drawing_program_texture_project_session_add_surface(
    struct DrawingProgramAppContext *ctx,
    const char *surface_name,
    uint32_t logical_width,
    uint32_t logical_height,
    uint32_t sample_density,
    uint32_t face_role,
    uint32_t quality_preset,
    uint32_t *out_surface_index);
CoreResult drawing_program_texture_project_session_add_surface_from_active(
    struct DrawingProgramAppContext *ctx,
    const char *surface_name,
    uint32_t *out_surface_index);
CoreResult drawing_program_texture_project_session_duplicate_active_surface(
    struct DrawingProgramAppContext *ctx,
    uint32_t *out_surface_index);
CoreResult drawing_program_texture_project_session_delete_active_surface(
    struct DrawingProgramAppContext *ctx,
    uint32_t *out_surface_index);
CoreResult drawing_program_texture_project_session_resize_active_blank_surface(
    struct DrawingProgramAppContext *ctx,
    uint32_t logical_width,
    uint32_t logical_height);
CoreResult drawing_program_texture_project_session_move_surface(
    struct DrawingProgramAppContext *ctx,
    uint32_t surface_index,
    float offset_x,
    float offset_y);
CoreResult drawing_program_texture_project_session_reset_object_layout(
    struct DrawingProgramAppContext *ctx);
void drawing_program_texture_project_session_sync_scene_selection_from_project(
    struct DrawingProgramAppContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
