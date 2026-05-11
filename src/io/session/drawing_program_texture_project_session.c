#include "drawing_program/drawing_program_texture_project_session.h"

#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_app_post_load.h"
#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_editor_state.h"
#include "drawing_program/drawing_program_history.h"
#include "drawing_program/drawing_program_object_store.h"
#include "drawing_program/drawing_program_project_state.h"
#include "drawing_program/drawing_program_render_revision.h"
#include "drawing_program/drawing_program_selection.h"
#include "drawing_program/drawing_program_texture_project.h"
#include "drawing_program/drawing_program_texture_project_template.h"
#include "drawing_program/drawing_program_texture_scene_import.h"

static CoreResult texture_project_session_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static uint8_t texture_project_session_clamp_percent(uint8_t value) {
    return (value > 100u) ? 100u : value;
}

static void texture_project_session_ui_layer_opacity_set(DrawingProgramAppContext *ctx,
                                                         uint32_t layer_id,
                                                         uint8_t opacity_percent) {
    uint8_t i;
    if (!ctx || layer_id == 0u) {
        return;
    }
    for (i = 0u; i < ctx->ui.layer_opacity_entry_count; ++i) {
        if (ctx->ui.layer_opacity_layer_ids[i] == layer_id) {
            ctx->ui.layer_opacity_values[i] = texture_project_session_clamp_percent(opacity_percent);
            return;
        }
    }
    if (ctx->ui.layer_opacity_entry_count >= DRAWING_PROGRAM_MAX_LAYERS) {
        return;
    }
    ctx->ui.layer_opacity_layer_ids[ctx->ui.layer_opacity_entry_count] = layer_id;
    ctx->ui.layer_opacity_values[ctx->ui.layer_opacity_entry_count] =
        texture_project_session_clamp_percent(opacity_percent);
    ctx->ui.layer_opacity_entry_count += 1u;
}

static void texture_project_session_sync_layer_opacity(DrawingProgramAppContext *ctx) {
    uint8_t compact_count = 0u;
    uint8_t i;
    if (!ctx) {
        return;
    }
    for (i = 0u; i < ctx->ui.layer_opacity_entry_count; ++i) {
        uint32_t layer_id = ctx->ui.layer_opacity_layer_ids[i];
        uint32_t ignored = 0u;
        if (layer_id != 0u &&
            drawing_program_document_layer_index_for_id(&ctx->document, layer_id, &ignored).code == CORE_OK) {
            ctx->ui.layer_opacity_layer_ids[compact_count] = layer_id;
            ctx->ui.layer_opacity_values[compact_count] =
                texture_project_session_clamp_percent(ctx->ui.layer_opacity_values[i]);
            compact_count += 1u;
        }
    }
    ctx->ui.layer_opacity_entry_count = compact_count;
    for (i = 0u; i < ctx->document.layer_count; ++i) {
        texture_project_session_ui_layer_opacity_set(ctx, ctx->document.layers[i].layer_id, 100u);
    }
}

static void texture_project_session_load_surface_layer_opacity(DrawingProgramAppContext *ctx,
                                                               uint32_t surface_index) {
    uint8_t opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t i;
    if (!ctx) {
        return;
    }
    memset(opacity_values, 0, sizeof(opacity_values));
    drawing_program_texture_project_collect_surface_layer_opacity_by_index(&ctx->texture_project,
                                                                           surface_index,
                                                                           opacity_values,
                                                                           DRAWING_PROGRAM_MAX_LAYERS);
    ctx->ui.layer_opacity_entry_count = 0u;
    memset(ctx->ui.layer_opacity_layer_ids, 0, sizeof(ctx->ui.layer_opacity_layer_ids));
    memset(ctx->ui.layer_opacity_values, 0, sizeof(ctx->ui.layer_opacity_values));
    for (i = 0u; i < ctx->document.layer_count && i < DRAWING_PROGRAM_MAX_LAYERS; ++i) {
        texture_project_session_ui_layer_opacity_set(ctx,
                                                     ctx->document.layers[i].layer_id,
                                                     opacity_values[i]);
    }
    drawing_program_render_revision_mark_layer_opacity_changed(ctx);
}

static CoreResult texture_project_session_init_blank_document_from_source(
    DrawingProgramDocument *out_document,
    DrawingProgramLayerRasterStore *out_layer_rasters,
    const DrawingProgramDocument *source_document,
    uint32_t logical_width,
    uint32_t logical_height) {
    uint32_t i;
    CoreResult result;
    if (!out_document || !out_layer_rasters || !source_document) {
        return texture_project_session_invalid("invalid blank texture document init request");
    }
    memset(out_document, 0, sizeof(*out_document));
    drawing_program_layer_raster_store_dispose(out_layer_rasters);
    result = drawing_program_document_init_with_shape(out_document,
                                                      logical_width,
                                                      logical_height,
                                                      source_document->sample_density);
    if (result.code != CORE_OK) {
        return result;
    }
    out_document->layer_count = source_document->layer_count;
    out_document->next_layer_id = source_document->next_layer_id;
    for (i = 0u; i < source_document->layer_count && i < DRAWING_PROGRAM_MAX_LAYERS; ++i) {
        out_document->layers[i] = source_document->layers[i];
    }
    for (; i < DRAWING_PROGRAM_MAX_LAYERS; ++i) {
        memset(&out_document->layers[i], 0, sizeof(out_document->layers[i]));
    }
    result = drawing_program_layer_raster_store_init_from_document(out_layer_rasters, out_document);
    if (result.code != CORE_OK) {
        drawing_program_layer_raster_store_dispose(out_layer_rasters);
        memset(out_document, 0, sizeof(*out_document));
        return result;
    }
    return core_result_ok();
}

static CoreResult texture_project_session_apply_surface(DrawingProgramAppContext *ctx,
                                                        uint32_t surface_index,
                                                        uint8_t commit_current_surface) {
    DrawingProgramDocument next_document;
    DrawingProgramLayerRasterStore next_layer_rasters;
    DrawingProgramEditorState preserved_editor;
    CoreResult result;
    if (!ctx) {
        return texture_project_session_invalid("invalid texture project apply surface request");
    }
    if (commit_current_surface) {
        result = drawing_program_texture_project_session_commit_active_surface(ctx);
        if (result.code != CORE_OK) {
            return result;
        }
    }
    memset(&next_document, 0, sizeof(next_document));
    memset(&next_layer_rasters, 0, sizeof(next_layer_rasters));
    result = drawing_program_texture_project_clone_surface(&ctx->texture_project,
                                                           surface_index,
                                                           &next_document,
                                                           &next_layer_rasters);
    if (result.code != CORE_OK) {
        drawing_program_layer_raster_store_dispose(&next_layer_rasters);
        return result;
    }
    result = drawing_program_texture_project_select_active_surface(&ctx->texture_project, surface_index);
    if (result.code != CORE_OK) {
        drawing_program_layer_raster_store_dispose(&next_layer_rasters);
        return result;
    }
    preserved_editor = ctx->editor;
    drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
    ctx->document = next_document;
    ctx->layer_rasters = next_layer_rasters;
    drawing_program_object_store_reset(&ctx->object_store);
    drawing_program_object_selection_reset(&ctx->object_selection);
    drawing_program_editor_state_init(&ctx->editor, &ctx->document);
    ctx->editor.active_tool = preserved_editor.active_tool;
    ctx->editor.viewport = preserved_editor.viewport;
    if (preserved_editor.active_layer_id != 0u) {
        uint32_t ignored_layer_index = 0u;
        if (drawing_program_document_layer_index_for_id(&ctx->document,
                                                        preserved_editor.active_layer_id,
                                                        &ignored_layer_index).code == CORE_OK) {
            ctx->editor.active_layer_id = preserved_editor.active_layer_id;
        }
    }
    drawing_program_history_init(&ctx->history);
    drawing_program_selection_reset(&ctx->selection);
    drawing_program_clipboard_reset(&ctx->clipboard);
    texture_project_session_sync_layer_opacity(ctx);
    texture_project_session_load_surface_layer_opacity(ctx, surface_index);
    drawing_program_app_rearm_after_document_swap(ctx);
    return core_result_ok();
}

static void texture_project_session_mark_surface_user_created(DrawingProgramAppContext *ctx,
                                                              uint32_t surface_index) {
    DrawingProgramTextureSurface *surface = 0;
    if (!ctx) {
        return;
    }
    surface = drawing_program_texture_project_surface_at_mut(&ctx->texture_project, surface_index);
    if (!surface) {
        return;
    }
    surface->user_created = 1u;
}

void drawing_program_texture_project_session_sync_scene_selection_from_project(
    DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    ctx->session.selected_scene_path[0] = '\0';
    ctx->session.selected_scene_object_id[0] = '\0';
    if (ctx->texture_project.source_scene_path[0] != '\0') {
        (void)snprintf(ctx->session.selected_scene_path,
                       sizeof(ctx->session.selected_scene_path),
                       "%s",
                       ctx->texture_project.source_scene_path);
    }
    if (ctx->texture_project.source_object_id[0] != '\0') {
        (void)snprintf(ctx->session.selected_scene_object_id,
                       sizeof(ctx->session.selected_scene_object_id),
                       "%s",
                       ctx->texture_project.source_object_id);
    }
}

CoreResult drawing_program_texture_project_session_init_from_current_document(
    DrawingProgramAppContext *ctx) {
    DrawingProgramTextureProject next_project;
    CoreResult result;
    if (!ctx) {
        return texture_project_session_invalid("invalid texture project session init request");
    }
    memset(&next_project, 0, sizeof(next_project));
    result = drawing_program_texture_project_init_single_surface(&next_project,
                                                                &ctx->document,
                                                                &ctx->layer_rasters,
                                                                "Surface 1",
                                                                DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD);
    if (result.code != CORE_OK) {
        return result;
    }
    next_project.active_surface_index = 0u;
    drawing_program_texture_project_dispose(&ctx->texture_project);
    ctx->texture_project = next_project;
    return core_result_ok();
}

CoreResult drawing_program_texture_project_session_commit_active_surface(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return texture_project_session_invalid("invalid texture project active surface commit request");
    }
    if (ctx->texture_project.surface_count == 0u) {
        return drawing_program_texture_project_session_init_from_current_document(ctx);
    }
    return drawing_program_texture_project_capture_surface(&ctx->texture_project,
                                                           ctx->texture_project.active_surface_index,
                                                           &ctx->document,
                                                           &ctx->layer_rasters,
                                                           ctx->ui.layer_opacity_layer_ids,
                                                           ctx->ui.layer_opacity_values,
                                                           ctx->ui.layer_opacity_entry_count,
                                                           ctx->texture_project.surfaces[ctx->texture_project.active_surface_index]
                                                               .layer_role_layer_ids,
                                                           ctx->texture_project.surfaces[ctx->texture_project.active_surface_index]
                                                               .layer_role_values,
                                                           ctx->texture_project.surfaces[ctx->texture_project.active_surface_index]
                                                               .layer_role_entry_count,
                                                           ctx->texture_project.surfaces[ctx->texture_project.active_surface_index]
                                                               .layer_material_intent_layer_ids,
                                                           ctx->texture_project.surfaces[ctx->texture_project.active_surface_index]
                                                               .layer_material_intent_values,
                                                           ctx->texture_project.surfaces[ctx->texture_project.active_surface_index]
                                                               .layer_material_intent_entry_count);
}

CoreResult drawing_program_texture_project_session_select_surface(DrawingProgramAppContext *ctx,
                                                                  uint32_t surface_index) {
    return texture_project_session_apply_surface(ctx, surface_index, 1u);
}

CoreResult drawing_program_texture_project_session_import_scene_object(DrawingProgramAppContext *ctx,
                                                                       const char *scene_json_path,
                                                                       const char *object_id,
                                                                       uint32_t quality_preset) {
    DrawingProgramTextureProject next_project;
    CoreSceneRootContract scene_root;
    CoreSceneObjectContract scene_object;
    CoreResult result;
    if (!ctx || !scene_json_path || scene_json_path[0] == '\0' || !object_id || object_id[0] == '\0') {
        return texture_project_session_invalid("invalid scene-object texture import request");
    }
    memset(&next_project, 0, sizeof(next_project));
    core_scene_root_contract_init(&scene_root);
    core_scene_object_contract_init(&scene_object);
    result = drawing_program_texture_scene_import_load_object(scene_json_path,
                                                              object_id,
                                                              &scene_root,
                                                              &scene_object);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_texture_project_init_from_scene_object(&next_project,
                                                                    &scene_root,
                                                                    &scene_object,
                                                                    quality_preset);
    if (result.code != CORE_OK) {
        drawing_program_texture_project_dispose(&next_project);
        return result;
    }
    (void)snprintf(next_project.source_scene_path,
                   sizeof(next_project.source_scene_path),
                   "%s",
                   scene_json_path);
    result = drawing_program_project_state_prepare_texture_import_path(ctx,
                                                                       next_project.source_scene_id,
                                                                       next_project.source_scene_path,
                                                                       next_project.source_object_id);
    if (result.code != CORE_OK) {
        drawing_program_texture_project_dispose(&next_project);
        return result;
    }
    drawing_program_texture_project_dispose(&ctx->texture_project);
    ctx->texture_project = next_project;
    result = texture_project_session_apply_surface(ctx, 0u, 0u);
    if (result.code != CORE_OK) {
        return result;
    }
    drawing_program_texture_project_session_sync_scene_selection_from_project(ctx);
    ctx->session.project_saved_history_count = 0u;
    ctx->session.project_saved_history_cursor = 0u;
    ctx->session.project_has_saved_state = 0u;
    return core_result_ok();
}

CoreResult drawing_program_texture_project_session_seed_blank(DrawingProgramAppContext *ctx,
                                                              uint32_t logical_width,
                                                              uint32_t logical_height,
                                                              uint32_t quality_preset) {
    DrawingProgramTextureProject next_project;
    CoreResult result;
    uint32_t surface_index = 0u;
    uint32_t sample_density = drawing_program_texture_project_default_sample_density(quality_preset);
    if (!ctx) {
        return texture_project_session_invalid("invalid texture project blank seed request");
    }
    memset(&next_project, 0, sizeof(next_project));
    result = drawing_program_texture_project_init_empty(&next_project);
    if (result.code != CORE_OK) {
        return result;
    }
    next_project.quality_preset = quality_preset;
    result = drawing_program_texture_project_add_surface(&next_project,
                                                         "Surface 1",
                                                         logical_width,
                                                         logical_height,
                                                         sample_density,
                                                         DRAWING_PROGRAM_TEXTURE_FACE_ROLE_UNSPECIFIED,
                                                         quality_preset,
                                                         &surface_index);
    if (result.code != CORE_OK) {
        drawing_program_texture_project_dispose(&next_project);
        return result;
    }
    drawing_program_texture_project_dispose(&ctx->texture_project);
    ctx->texture_project = next_project;
    texture_project_session_mark_surface_user_created(ctx, 0u);
    return texture_project_session_apply_surface(ctx, surface_index, 0u);
}

CoreResult drawing_program_texture_project_session_add_surface(DrawingProgramAppContext *ctx,
                                                               const char *surface_name,
                                                               uint32_t logical_width,
                                                               uint32_t logical_height,
                                                               uint32_t sample_density,
                                                               uint32_t face_role,
                                                               uint32_t quality_preset,
                                                               uint32_t *out_surface_index) {
    CoreResult result;
    if (!ctx) {
        return texture_project_session_invalid("invalid add texture surface request");
    }
    result = drawing_program_texture_project_session_commit_active_surface(ctx);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_texture_project_add_surface(&ctx->texture_project,
                                                         surface_name,
                                                         logical_width,
                                                         logical_height,
                                                         sample_density,
                                                         face_role,
                                                         quality_preset,
                                                         out_surface_index);
    if (result.code == CORE_OK && out_surface_index) {
        drawing_program_texture_project_refresh_surface_flags(&ctx->texture_project, *out_surface_index);
    }
    return result;
}

CoreResult drawing_program_texture_project_session_add_surface_from_active(DrawingProgramAppContext *ctx,
                                                                           const char *surface_name,
                                                                           uint32_t *out_surface_index) {
    const DrawingProgramTextureSurface *active_surface = 0;
    uint32_t logical_width = 0u;
    uint32_t logical_height = 0u;
    uint32_t sample_density = 0u;
    uint32_t quality_preset = DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD;
    CoreResult result;
    if (!ctx) {
        return texture_project_session_invalid("invalid add blank surface from active request");
    }
    active_surface =
        drawing_program_texture_project_surface_at(&ctx->texture_project, ctx->texture_project.active_surface_index);
    if (active_surface && active_surface->storage) {
        logical_width = active_surface->storage->document.logical_width;
        logical_height = active_surface->storage->document.logical_height;
        sample_density = active_surface->storage->document.sample_density;
        quality_preset = active_surface->quality_preset;
    } else {
        logical_width = ctx->document.logical_width;
        logical_height = ctx->document.logical_height;
        sample_density = ctx->document.sample_density;
        quality_preset = ctx->texture_project.quality_preset;
    }
    result = drawing_program_texture_project_session_add_surface(ctx,
                                                                 surface_name,
                                                                 logical_width,
                                                                 logical_height,
                                                                 sample_density,
                                                                 DRAWING_PROGRAM_TEXTURE_FACE_ROLE_UNSPECIFIED,
                                                                 quality_preset,
                                                                 out_surface_index);
    if (result.code != CORE_OK) {
        return result;
    }
    if (out_surface_index) {
        texture_project_session_mark_surface_user_created(ctx, *out_surface_index);
    }
    return core_result_ok();
}

CoreResult drawing_program_texture_project_session_duplicate_active_surface(DrawingProgramAppContext *ctx,
                                                                            uint32_t *out_surface_index) {
    const DrawingProgramTextureSurface *active_surface = 0;
    char duplicate_name[DRAWING_PROGRAM_TEXTURE_PROJECT_NAME_CAPACITY];
    uint32_t surface_index = 0u;
    CoreResult result;
    if (!ctx) {
        return texture_project_session_invalid("invalid duplicate active texture surface request");
    }
    active_surface =
        drawing_program_texture_project_surface_at(&ctx->texture_project, ctx->texture_project.active_surface_index);
    if (!active_surface || !active_surface->storage) {
        return texture_project_session_invalid("active texture surface unavailable for duplication");
    }
    (void)snprintf(duplicate_name, sizeof(duplicate_name), "%s Copy", active_surface->name);
    result = drawing_program_texture_project_session_commit_active_surface(ctx);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_texture_project_add_surface(&ctx->texture_project,
                                                         duplicate_name,
                                                         active_surface->storage->document.logical_width,
                                                         active_surface->storage->document.logical_height,
                                                         active_surface->storage->document.sample_density,
                                                         active_surface->face_role,
                                                         active_surface->quality_preset,
                                                         &surface_index);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_texture_project_set_surface_semantic(&ctx->texture_project,
                                                                  surface_index,
                                                                  &active_surface->semantic);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_texture_project_set_surface_layout_offset(&ctx->texture_project,
                                                                       surface_index,
                                                                       active_surface->layout_offset_x,
                                                                       active_surface->layout_offset_y);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_texture_project_capture_surface(&ctx->texture_project,
                                                             surface_index,
                                                             &ctx->document,
                                                             &ctx->layer_rasters,
                                                             ctx->ui.layer_opacity_layer_ids,
                                                             ctx->ui.layer_opacity_values,
                                                             ctx->ui.layer_opacity_entry_count,
                                                             active_surface->layer_role_layer_ids,
                                                             active_surface->layer_role_values,
                                                             active_surface->layer_role_entry_count,
                                                             active_surface->layer_material_intent_layer_ids,
                                                             active_surface->layer_material_intent_values,
                                                             active_surface->layer_material_intent_entry_count);
    if (result.code != CORE_OK) {
        return result;
    }
    {
        DrawingProgramTextureSurface *duplicate_surface =
            drawing_program_texture_project_surface_at_mut(&ctx->texture_project, surface_index);
        if (duplicate_surface) {
            duplicate_surface->reflection_center_x = active_surface->reflection_center_x;
            duplicate_surface->reflection_center_y = active_surface->reflection_center_y;
            duplicate_surface->reflection_horizontal = active_surface->reflection_horizontal;
            duplicate_surface->reflection_vertical = active_surface->reflection_vertical;
            drawing_program_canvas_reflection_surface_clamp_to_document(duplicate_surface, &ctx->document);
        }
    }
    texture_project_session_mark_surface_user_created(ctx, surface_index);
    drawing_program_texture_project_refresh_surface_flags(&ctx->texture_project, surface_index);
    if (out_surface_index) {
        *out_surface_index = surface_index;
    }
    return core_result_ok();
}

CoreResult drawing_program_texture_project_session_delete_active_surface(DrawingProgramAppContext *ctx,
                                                                         uint32_t *out_surface_index) {
    uint32_t delete_index = 0u;
    uint32_t next_index = 0u;
    CoreResult result;
    if (!ctx) {
        return texture_project_session_invalid("invalid delete active texture surface request");
    }
    if (ctx->texture_project.surface_count <= 1u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "cannot delete the last texture surface" };
    }
    delete_index = ctx->texture_project.active_surface_index;
    next_index = (delete_index > 0u) ? (delete_index - 1u) : 0u;
    result = drawing_program_texture_project_remove_surface(&ctx->texture_project, delete_index);
    if (result.code != CORE_OK) {
        return result;
    }
    if (next_index >= ctx->texture_project.surface_count) {
        next_index = ctx->texture_project.surface_count - 1u;
    }
    result = texture_project_session_apply_surface(ctx, next_index, 0u);
    if (result.code != CORE_OK) {
        return result;
    }
    if (out_surface_index) {
        *out_surface_index = next_index;
    }
    return core_result_ok();
}

CoreResult drawing_program_texture_project_session_resize_active_blank_surface(
    DrawingProgramAppContext *ctx,
    uint32_t logical_width,
    uint32_t logical_height) {
    const DrawingProgramTextureSurface *active_surface = 0;
    DrawingProgramDocument next_document;
    DrawingProgramLayerRasterStore next_layer_rasters;
    DrawingProgramEditorState preserved_editor;
    CoreResult result;
    uint32_t preserved_active_layer_id = 0u;
    if (!ctx) {
        return texture_project_session_invalid("invalid resize active blank texture surface request");
    }
    active_surface =
        drawing_program_texture_project_surface_at(&ctx->texture_project, ctx->texture_project.active_surface_index);
    if (!active_surface || !active_surface->storage) {
        return texture_project_session_invalid("active texture surface unavailable for resize");
    }
    if (!active_surface->is_blank || active_surface->resize_locked) {
        return texture_project_session_invalid("active texture surface is not resizable");
    }
    if (logical_width == 0u || logical_height == 0u) {
        return texture_project_session_invalid("invalid resized texture surface shape");
    }
    if (ctx->document.logical_width == logical_width && ctx->document.logical_height == logical_height) {
        return core_result_ok();
    }
    memset(&next_document, 0, sizeof(next_document));
    memset(&next_layer_rasters, 0, sizeof(next_layer_rasters));
    preserved_editor = ctx->editor;
    preserved_active_layer_id = ctx->editor.active_layer_id;
    result = texture_project_session_init_blank_document_from_source(&next_document,
                                                                     &next_layer_rasters,
                                                                     &ctx->document,
                                                                     logical_width,
                                                                     logical_height);
    if (result.code != CORE_OK) {
        return result;
    }
    drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
    ctx->document = next_document;
    ctx->layer_rasters = next_layer_rasters;
    drawing_program_object_store_reset(&ctx->object_store);
    drawing_program_object_selection_reset(&ctx->object_selection);
    drawing_program_history_init(&ctx->history);
    drawing_program_selection_reset(&ctx->selection);
    drawing_program_clipboard_reset(&ctx->clipboard);
    ctx->editor = preserved_editor;
    {
        uint32_t ignored_layer_index = 0u;
        if (preserved_active_layer_id == 0u ||
            drawing_program_document_layer_index_for_id(&ctx->document,
                                                        preserved_active_layer_id,
                                                        &ignored_layer_index).code != CORE_OK) {
            ctx->editor.active_layer_id = (ctx->document.layer_count > 0u) ? ctx->document.layers[0].layer_id : 0u;
        }
    }
    texture_project_session_sync_layer_opacity(ctx);
    drawing_program_app_rearm_after_document_swap(ctx);
    result = drawing_program_texture_project_capture_surface(&ctx->texture_project,
                                                             ctx->texture_project.active_surface_index,
                                                             &ctx->document,
                                                             &ctx->layer_rasters,
                                                             ctx->ui.layer_opacity_layer_ids,
                                                             ctx->ui.layer_opacity_values,
                                                             ctx->ui.layer_opacity_entry_count,
                                                             ctx->texture_project.surfaces[ctx->texture_project.active_surface_index]
                                                                 .layer_role_layer_ids,
                                                             ctx->texture_project.surfaces[ctx->texture_project.active_surface_index]
                                                                 .layer_role_values,
                                                             ctx->texture_project.surfaces[ctx->texture_project.active_surface_index]
                                                                 .layer_role_entry_count,
                                                             ctx->texture_project.surfaces[ctx->texture_project.active_surface_index]
                                                                 .layer_material_intent_layer_ids,
                                                             ctx->texture_project.surfaces[ctx->texture_project.active_surface_index]
                                                                 .layer_material_intent_values,
                                                             ctx->texture_project.surfaces[ctx->texture_project.active_surface_index]
                                                                 .layer_material_intent_entry_count);
    if (result.code != CORE_OK) {
        return result;
    }
    drawing_program_texture_project_refresh_surface_flags(&ctx->texture_project,
                                                          ctx->texture_project.active_surface_index);
    return core_result_ok();
}

CoreResult drawing_program_texture_project_session_move_surface(DrawingProgramAppContext *ctx,
                                                                uint32_t surface_index,
                                                                float offset_x,
                                                                float offset_y) {
    if (!ctx) {
        return texture_project_session_invalid("invalid move texture surface request");
    }
    return drawing_program_texture_project_set_surface_layout_offset(&ctx->texture_project,
                                                                     surface_index,
                                                                     offset_x,
                                                                     offset_y);
}

CoreResult drawing_program_texture_project_session_reset_object_layout(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return texture_project_session_invalid("invalid reset texture object layout request");
    }
    drawing_program_texture_project_reset_all_surface_layout_offsets(&ctx->texture_project);
    return core_result_ok();
}
