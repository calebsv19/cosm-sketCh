#include "drawing_program/drawing_program_texture_project.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_texture_net.h"

static CoreResult texture_project_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static uint64_t texture_project_next_runtime_epoch(void) {
    static uint64_t next_epoch = 1u;
    uint64_t epoch = next_epoch;
    next_epoch += 1u;
    if (next_epoch == 0u) {
        next_epoch = 1u;
    }
    if (epoch == 0u) {
        epoch = next_epoch;
        next_epoch += 1u;
    }
    return epoch;
}

static uint64_t texture_project_next_content_revision(uint64_t current_revision) {
    uint64_t next_revision = current_revision + 1u;
    if (next_revision == 0u) {
        next_revision = 1u;
    }
    return next_revision;
}

static void texture_project_surface_storage_dispose(DrawingProgramTextureSurfaceStorage *storage) {
    if (!storage) {
        return;
    }
    drawing_program_layer_raster_store_dispose(&storage->layer_rasters);
    free(storage);
}

static int texture_project_surface_index_valid(const DrawingProgramTextureProject *project, uint32_t surface_index) {
    return project && surface_index < project->surface_count;
}

static int texture_project_layer_rasters_match_document(const DrawingProgramDocument *document,
                                                        const DrawingProgramLayerRasterStore *layer_rasters) {
    return document &&
           layer_rasters &&
           layer_rasters->sample_count == document->raster_sample_count &&
           layer_rasters->raster_width == document->raster_width &&
           layer_rasters->raster_height == document->raster_height;
}

static CoreResult texture_project_storage_clone_from_state(DrawingProgramTextureSurfaceStorage **out_storage,
                                                           const DrawingProgramDocument *document,
                                                           const DrawingProgramLayerRasterStore *layer_rasters) {
    DrawingProgramTextureSurfaceStorage *storage = 0;
    CoreResult result;
    uint32_t layer_i;
    if (!out_storage || !document) {
        return texture_project_invalid("invalid texture storage clone request");
    }
    storage = (DrawingProgramTextureSurfaceStorage *)calloc(1u, sizeof(*storage));
    if (!storage) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate texture surface storage" };
    }
    storage->document = *document;
    result = drawing_program_layer_raster_store_init_from_document(&storage->layer_rasters, &storage->document);
    if (result.code != CORE_OK) {
        texture_project_surface_storage_dispose(storage);
        return result;
    }
    if (texture_project_layer_rasters_match_document(document, layer_rasters)) {
        for (layer_i = 0u; layer_i < document->layer_count; ++layer_i) {
            const uint32_t layer_id = document->layers[layer_i].layer_id;
            const DrawingProgramRasterSample *samples = 0;
            uint32_t sample_count = 0u;
            result = drawing_program_layer_raster_store_export_layer(layer_rasters, layer_id, &samples, &sample_count);
            if (result.code != CORE_OK || !samples || sample_count != document->raster_sample_count) {
                if (layer_i == 0u) {
                    samples = document->raster_samples;
                    sample_count = document->raster_sample_count;
                } else {
                    continue;
                }
            }
            result = drawing_program_layer_raster_store_import_layer(&storage->layer_rasters,
                                                                     layer_id,
                                                                     samples,
                                                                     sample_count);
            if (result.code != CORE_OK) {
                texture_project_surface_storage_dispose(storage);
                return result;
            }
        }
    }
    *out_storage = storage;
    return core_result_ok();
}

static uint8_t texture_project_samples_any_nonempty(const DrawingProgramRasterSample *samples, uint32_t sample_count) {
    DrawingProgramRasterSample empty_sample = drawing_program_color_eraser_value();
    uint32_t i;
    if (!samples || sample_count == 0u) {
        return 0u;
    }
    for (i = 0u; i < sample_count; ++i) {
        if (samples[i] != empty_sample) {
            return 1u;
        }
    }
    return 0u;
}

static uint8_t texture_project_storage_has_visible_content(const DrawingProgramTextureSurfaceStorage *storage) {
    uint32_t layer_i;
    if (!storage) {
        return 0u;
    }
    if (texture_project_samples_any_nonempty(storage->document.raster_samples, storage->document.raster_sample_count)) {
        return 1u;
    }
    for (layer_i = 1u; layer_i < storage->document.layer_count; ++layer_i) {
        const uint32_t layer_id = storage->document.layers[layer_i].layer_id;
        const DrawingProgramRasterSample *samples = 0;
        uint32_t sample_count = 0u;
        if (drawing_program_layer_raster_store_export_layer(&storage->layer_rasters,
                                                            layer_id,
                                                            &samples,
                                                            &sample_count)
                .code != CORE_OK) {
            continue;
        }
        if (texture_project_samples_any_nonempty(samples, sample_count)) {
            return 1u;
        }
    }
    return 0u;
}

static void texture_project_surface_seed_layer_opacity_defaults(DrawingProgramTextureSurface *surface) {
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

static void texture_project_surface_seed_layer_role_defaults(DrawingProgramTextureSurface *surface) {
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

static void texture_project_surface_seed_layer_material_intent_defaults(
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

static void texture_project_surface_capture_layer_opacity(
    DrawingProgramTextureSurface *surface,
    const uint32_t *layer_opacity_layer_ids,
    const uint8_t *layer_opacity_values,
    uint32_t layer_opacity_entry_count) {
    uint32_t i;
    if (!surface || !surface->storage) {
        return;
    }
    texture_project_surface_seed_layer_opacity_defaults(surface);
    if (!layer_opacity_layer_ids || !layer_opacity_values || layer_opacity_entry_count == 0u) {
        return;
    }
    for (i = 0u; i < layer_opacity_entry_count; ++i) {
        uint32_t source_layer_id = layer_opacity_layer_ids[i];
        uint8_t opacity = layer_opacity_values[i];
        uint32_t j;
        if (opacity > 100u) {
            opacity = 100u;
        }
        for (j = 0u; j < surface->layer_opacity_entry_count; ++j) {
            if (surface->layer_opacity_layer_ids[j] == source_layer_id) {
                surface->layer_opacity_values[j] = opacity;
                break;
            }
        }
    }
}

static void texture_project_surface_capture_layer_roles(
    DrawingProgramTextureSurface *surface,
    const uint32_t *layer_role_layer_ids,
    const uint8_t *layer_role_values,
    uint32_t layer_role_entry_count) {
    uint32_t i;
    if (!surface || !surface->storage) {
        return;
    }
    texture_project_surface_seed_layer_role_defaults(surface);
    if (!layer_role_layer_ids || !layer_role_values || layer_role_entry_count == 0u) {
        return;
    }
    for (i = 0u; i < layer_role_entry_count; ++i) {
        uint32_t source_layer_id = layer_role_layer_ids[i];
        uint8_t role_kind = layer_role_values[i];
        uint32_t j;
        for (j = 0u; j < surface->layer_role_entry_count; ++j) {
            if (surface->layer_role_layer_ids[j] == source_layer_id) {
                surface->layer_role_values[j] = role_kind;
                break;
            }
        }
    }
}

static void texture_project_surface_capture_layer_material_intents(
    DrawingProgramTextureSurface *surface,
    const uint32_t *layer_material_intent_layer_ids,
    const uint8_t *layer_material_intent_values,
    uint32_t layer_material_intent_entry_count,
    uint32_t overlay_material_intent_kind) {
    uint32_t i;
    if (!surface || !surface->storage) {
        return;
    }
    texture_project_surface_seed_layer_material_intent_defaults(surface, overlay_material_intent_kind);
    if (!layer_material_intent_layer_ids || !layer_material_intent_values ||
        layer_material_intent_entry_count == 0u) {
        return;
    }
    for (i = 0u; i < layer_material_intent_entry_count; ++i) {
        uint32_t source_layer_id = layer_material_intent_layer_ids[i];
        uint8_t intent_kind = layer_material_intent_values[i];
        uint32_t j;
        for (j = 0u; j < surface->layer_material_intent_entry_count; ++j) {
            if (surface->layer_material_intent_layer_ids[j] == source_layer_id) {
                surface->layer_material_intent_values[j] = intent_kind;
                break;
            }
        }
    }
}

static CoreResult texture_project_surface_storage_init_with_shape(DrawingProgramTextureSurfaceStorage **out_storage,
                                                                  uint32_t logical_width,
                                                                  uint32_t logical_height,
                                                                  uint32_t sample_density) {
    DrawingProgramTextureSurfaceStorage *storage = 0;
    CoreResult result;
    if (!out_storage) {
        return texture_project_invalid("invalid texture storage init request");
    }
    storage = (DrawingProgramTextureSurfaceStorage *)calloc(1u, sizeof(*storage));
    if (!storage) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate texture surface storage" };
    }
    result = drawing_program_document_init_with_shape(&storage->document,
                                                      logical_width,
                                                      logical_height,
                                                      sample_density);
    if (result.code != CORE_OK) {
        texture_project_surface_storage_dispose(storage);
        return result;
    }
    result = drawing_program_layer_raster_store_init_from_document(&storage->layer_rasters, &storage->document);
    if (result.code != CORE_OK) {
        texture_project_surface_storage_dispose(storage);
        return result;
    }
    *out_storage = storage;
    return core_result_ok();
}

static CoreResult texture_project_ensure_surface_capacity(DrawingProgramTextureProject *project,
                                                          uint32_t required_capacity) {
    DrawingProgramTextureSurface *next_surfaces = 0;
    uint32_t next_capacity = 0u;
    if (!project || required_capacity == 0u) {
        return texture_project_invalid("invalid texture project capacity request");
    }
    if (required_capacity <= project->surface_capacity) {
        return core_result_ok();
    }
    next_capacity = project->surface_capacity > 0u ? project->surface_capacity : 2u;
    while (next_capacity < required_capacity) {
        if (next_capacity > (UINT32_MAX / 2u)) {
            return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "texture project surface capacity overflow" };
        }
        next_capacity *= 2u;
    }
    next_surfaces = (DrawingProgramTextureSurface *)realloc(project->surfaces,
                                                            (size_t)next_capacity * sizeof(*next_surfaces));
    if (!next_surfaces) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to grow texture project surfaces" };
    }
    memset(next_surfaces + project->surface_capacity,
           0,
           (size_t)(next_capacity - project->surface_capacity) * sizeof(*next_surfaces));
    project->surfaces = next_surfaces;
    project->surface_capacity = next_capacity;
    return core_result_ok();
}

static CoreResult texture_project_append_surface(DrawingProgramTextureProject *project,
                                                 const char *surface_name,
                                                 uint32_t face_role,
                                                 uint32_t quality_preset,
                                                 DrawingProgramTextureSurfaceStorage *storage,
                                                 uint32_t *out_surface_index) {
    DrawingProgramTextureSurface *surface = 0;
    CoreResult result;
    uint32_t index = 0u;
    if (!project || !storage) {
        return texture_project_invalid("invalid texture project append surface request");
    }
    result = texture_project_ensure_surface_capacity(project, project->surface_count + 1u);
    if (result.code != CORE_OK) {
        texture_project_surface_storage_dispose(storage);
        return result;
    }
    index = project->surface_count;
    surface = &project->surfaces[index];
    memset(surface, 0, sizeof(*surface));
    surface->surface_id = project->next_surface_id > 0u ? project->next_surface_id : 1u;
    surface->face_role = face_role;
    surface->quality_preset = quality_preset;
    surface->content_revision = 1u;
    surface->is_blank = texture_project_storage_has_visible_content(storage) ? 0u : 1u;
    surface->resize_locked = surface->is_blank ? 0u : 1u;
    drawing_program_texture_surface_semantic_clear(&surface->semantic);
    surface->storage = storage;
    drawing_program_canvas_reflection_surface_seed_defaults(surface, &storage->document);
    texture_project_surface_seed_layer_opacity_defaults(surface);
    texture_project_surface_seed_layer_role_defaults(surface);
    texture_project_surface_seed_layer_material_intent_defaults(surface,
                                                                project->overlay_material_intent_kind);
    if (surface_name && surface_name[0] != '\0') {
        (void)snprintf(surface->name, sizeof(surface->name), "%s", surface_name);
    } else {
        (void)snprintf(surface->name, sizeof(surface->name), "Surface %u", (unsigned)(index + 1u));
    }
    project->surface_count += 1u;
    project->next_surface_id = surface->surface_id + 1u;
    if (project->next_surface_id == 0u) {
        project->next_surface_id = 1u;
    }
    if (out_surface_index) {
        *out_surface_index = index;
    }
    return core_result_ok();
}

void drawing_program_texture_project_dispose(DrawingProgramTextureProject *project) {
    uint32_t i;
    if (!project) {
        return;
    }
    for (i = 0u; i < project->surface_count; ++i) {
        texture_project_surface_storage_dispose(project->surfaces[i].storage);
        project->surfaces[i].storage = 0;
    }
    free(project->surfaces);
    memset(project, 0, sizeof(*project));
}

CoreResult drawing_program_texture_project_init_empty(DrawingProgramTextureProject *project) {
    if (!project) {
        return texture_project_invalid("invalid texture project init request");
    }
    drawing_program_texture_project_dispose(project);
    project->schema_version = DRAWING_PROGRAM_TEXTURE_PROJECT_SCHEMA_VERSION;
    project->net_layout_kind = DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_NONE;
    project->quality_preset = DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD;
    project->export_intent_kind = DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_FLATTENED_ONLY;
    project->overlay_material_intent_kind = DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_NONE;
    project->next_surface_id = 1u;
    project->runtime_cache_epoch = texture_project_next_runtime_epoch();
    return core_result_ok();
}

CoreResult drawing_program_texture_project_init_single_surface(
    DrawingProgramTextureProject *project,
    const DrawingProgramDocument *document,
    const DrawingProgramLayerRasterStore *layer_rasters,
    const char *surface_name,
    uint32_t quality_preset) {
    DrawingProgramTextureSurfaceStorage *storage = 0;
    CoreResult result;
    if (!project || !document) {
        return texture_project_invalid("invalid single-surface texture project init request");
    }
    result = drawing_program_texture_project_init_empty(project);
    if (result.code != CORE_OK) {
        return result;
    }
    project->quality_preset = quality_preset;
    project->net_layout_kind = DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_NONE;
    result = texture_project_storage_clone_from_state(&storage, document, layer_rasters);
    if (result.code != CORE_OK) {
        return result;
    }
    return texture_project_append_surface(project,
                                          surface_name,
                                          DRAWING_PROGRAM_TEXTURE_FACE_ROLE_UNSPECIFIED,
                                          quality_preset,
                                          storage,
                                          0);
}

CoreResult drawing_program_texture_project_add_surface(
    DrawingProgramTextureProject *project,
    const char *surface_name,
    uint32_t logical_width,
    uint32_t logical_height,
    uint32_t sample_density,
    uint32_t face_role,
    uint32_t quality_preset,
    uint32_t *out_surface_index) {
    DrawingProgramTextureSurfaceStorage *storage = 0;
    CoreResult result;
    if (!project) {
        return texture_project_invalid("invalid add texture surface request");
    }
    result = texture_project_surface_storage_init_with_shape(&storage,
                                                             logical_width,
                                                             logical_height,
                                                             sample_density);
    if (result.code != CORE_OK) {
        return result;
    }
    return texture_project_append_surface(project,
                                          surface_name,
                                          face_role,
                                          quality_preset,
                                          storage,
                                          out_surface_index);
}

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
    uint32_t layer_material_intent_entry_count) {
    DrawingProgramTextureSurfaceStorage *storage = 0;
    CoreResult result;
    DrawingProgramTextureSurface *surface = 0;
    uint32_t opacity_layer_ids_copy[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t opacity_values_copy[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t role_layer_ids_copy[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t role_values_copy[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t material_intent_layer_ids_copy[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t material_intent_values_copy[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t safe_opacity_entry_count = layer_opacity_entry_count;
    uint32_t safe_role_entry_count = layer_role_entry_count;
    uint32_t safe_material_intent_entry_count = layer_material_intent_entry_count;
    if (!texture_project_surface_index_valid(project, surface_index) || !document) {
        return texture_project_invalid("invalid texture surface capture request");
    }
    memset(opacity_layer_ids_copy, 0, sizeof(opacity_layer_ids_copy));
    memset(opacity_values_copy, 0, sizeof(opacity_values_copy));
    memset(role_layer_ids_copy, 0, sizeof(role_layer_ids_copy));
    memset(role_values_copy, 0, sizeof(role_values_copy));
    memset(material_intent_layer_ids_copy, 0, sizeof(material_intent_layer_ids_copy));
    memset(material_intent_values_copy, 0, sizeof(material_intent_values_copy));
    if (safe_opacity_entry_count > DRAWING_PROGRAM_MAX_LAYERS) {
        safe_opacity_entry_count = DRAWING_PROGRAM_MAX_LAYERS;
    }
    if (safe_role_entry_count > DRAWING_PROGRAM_MAX_LAYERS) {
        safe_role_entry_count = DRAWING_PROGRAM_MAX_LAYERS;
    }
    if (safe_material_intent_entry_count > DRAWING_PROGRAM_MAX_LAYERS) {
        safe_material_intent_entry_count = DRAWING_PROGRAM_MAX_LAYERS;
    }
    if (layer_opacity_layer_ids && layer_opacity_values && safe_opacity_entry_count > 0u) {
        memcpy(opacity_layer_ids_copy,
               layer_opacity_layer_ids,
               (size_t)safe_opacity_entry_count * sizeof(opacity_layer_ids_copy[0]));
        memcpy(opacity_values_copy,
               layer_opacity_values,
               (size_t)safe_opacity_entry_count * sizeof(opacity_values_copy[0]));
    } else {
        safe_opacity_entry_count = 0u;
    }
    if (layer_role_layer_ids && layer_role_values && safe_role_entry_count > 0u) {
        memcpy(role_layer_ids_copy,
               layer_role_layer_ids,
               (size_t)safe_role_entry_count * sizeof(role_layer_ids_copy[0]));
        memcpy(role_values_copy,
               layer_role_values,
               (size_t)safe_role_entry_count * sizeof(role_values_copy[0]));
    } else {
        safe_role_entry_count = 0u;
    }
    if (layer_material_intent_layer_ids && layer_material_intent_values &&
        safe_material_intent_entry_count > 0u) {
        memcpy(material_intent_layer_ids_copy,
               layer_material_intent_layer_ids,
               (size_t)safe_material_intent_entry_count * sizeof(material_intent_layer_ids_copy[0]));
        memcpy(material_intent_values_copy,
               layer_material_intent_values,
               (size_t)safe_material_intent_entry_count * sizeof(material_intent_values_copy[0]));
    } else {
        safe_material_intent_entry_count = 0u;
    }
    result = texture_project_storage_clone_from_state(&storage, document, layer_rasters);
    if (result.code != CORE_OK) {
        return result;
    }
    surface = &project->surfaces[surface_index];
    texture_project_surface_storage_dispose(surface->storage);
    surface->storage = storage;
    surface->quality_preset = project->quality_preset;
    surface->content_revision = texture_project_next_content_revision(surface->content_revision);
    surface->is_blank = texture_project_storage_has_visible_content(storage) ? 0u : 1u;
    surface->resize_locked = surface->is_blank ? 0u : 1u;
    drawing_program_canvas_reflection_surface_clamp_to_document(surface, document);
    texture_project_surface_capture_layer_opacity(surface,
                                                  opacity_layer_ids_copy,
                                                  opacity_values_copy,
                                                  safe_opacity_entry_count);
    texture_project_surface_capture_layer_roles(surface,
                                                role_layer_ids_copy,
                                                role_values_copy,
                                                safe_role_entry_count);
    texture_project_surface_capture_layer_material_intents(surface,
                                                           material_intent_layer_ids_copy,
                                                           material_intent_values_copy,
                                                           safe_material_intent_entry_count,
                                                           project->overlay_material_intent_kind);
    return core_result_ok();
}

CoreResult drawing_program_texture_project_set_surface_semantic(
    DrawingProgramTextureProject *project,
    uint32_t surface_index,
    const DrawingProgramTextureSurfaceSemantic *semantic) {
    DrawingProgramTextureSurface *surface = 0;
    if (!texture_project_surface_index_valid(project, surface_index) || !semantic) {
        return texture_project_invalid("invalid texture surface semantic set request");
    }
    surface = &project->surfaces[surface_index];
    surface->semantic = *semantic;
    return core_result_ok();
}

CoreResult drawing_program_texture_project_set_surface_layout_offset(
    DrawingProgramTextureProject *project,
    uint32_t surface_index,
    float offset_x,
    float offset_y) {
    DrawingProgramTextureSurface *surface = 0;
    if (!texture_project_surface_index_valid(project, surface_index)) {
        return texture_project_invalid("invalid texture surface layout offset set request");
    }
    surface = &project->surfaces[surface_index];
    surface->layout_offset_x = offset_x;
    surface->layout_offset_y = offset_y;
    return core_result_ok();
}

CoreResult drawing_program_texture_project_reset_surface_layout_offset(
    DrawingProgramTextureProject *project,
    uint32_t surface_index) {
    return drawing_program_texture_project_set_surface_layout_offset(project, surface_index, 0.0f, 0.0f);
}

void drawing_program_texture_project_reset_all_surface_layout_offsets(
    DrawingProgramTextureProject *project) {
    uint32_t i;
    if (!project) {
        return;
    }
    for (i = 0u; i < project->surface_count; ++i) {
        project->surfaces[i].layout_offset_x = 0.0f;
        project->surfaces[i].layout_offset_y = 0.0f;
    }
}

void drawing_program_texture_project_collect_surface_layer_opacity_by_index(
    const DrawingProgramTextureProject *project,
    uint32_t surface_index,
    uint8_t *out_opacity,
    uint32_t out_count) {
    const DrawingProgramTextureSurface *surface = 0;
    uint32_t i;
    if (!out_opacity) {
        return;
    }
    for (i = 0u; i < out_count; ++i) {
        out_opacity[i] = 100u;
    }
    if (!texture_project_surface_index_valid(project, surface_index)) {
        return;
    }
    surface = &project->surfaces[surface_index];
    if (!surface->storage) {
        return;
    }
    for (i = 0u; i < out_count && i < surface->storage->document.layer_count; ++i) {
        uint32_t layer_id = surface->storage->document.layers[i].layer_id;
        uint32_t j;
        for (j = 0u; j < surface->layer_opacity_entry_count; ++j) {
            if (surface->layer_opacity_layer_ids[j] == layer_id) {
                uint8_t opacity = surface->layer_opacity_values[j];
                out_opacity[i] = (opacity > 100u) ? 100u : opacity;
                break;
            }
        }
    }
}

void drawing_program_texture_project_collect_surface_layer_role_by_index(
    const DrawingProgramTextureProject *project,
    uint32_t surface_index,
    uint8_t *out_role_kind,
    uint32_t out_count) {
    const DrawingProgramTextureSurface *surface = 0;
    uint32_t i;
    if (!out_role_kind) {
        return;
    }
    for (i = 0u; i < out_count; ++i) {
        out_role_kind[i] = (uint8_t)DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM;
    }
    if (!texture_project_surface_index_valid(project, surface_index)) {
        return;
    }
    surface = &project->surfaces[surface_index];
    if (!surface->storage) {
        return;
    }
    for (i = 0u; i < out_count && i < surface->storage->document.layer_count; ++i) {
        uint32_t layer_id = surface->storage->document.layers[i].layer_id;
        uint32_t j;
        for (j = 0u; j < surface->layer_role_entry_count; ++j) {
            if (surface->layer_role_layer_ids[j] == layer_id) {
                out_role_kind[i] = surface->layer_role_values[j];
                break;
            }
        }
    }
}

void drawing_program_texture_project_collect_surface_layer_material_intent_by_index(
    const DrawingProgramTextureProject *project,
    uint32_t surface_index,
    uint8_t *out_intent_kind,
    uint32_t out_count) {
    const DrawingProgramTextureSurface *surface = 0;
    uint32_t i;
    if (!out_intent_kind) {
        return;
    }
    for (i = 0u; i < out_count; ++i) {
        out_intent_kind[i] = (uint8_t)DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE;
    }
    if (!texture_project_surface_index_valid(project, surface_index)) {
        return;
    }
    surface = &project->surfaces[surface_index];
    if (!surface->storage) {
        return;
    }
    for (i = 0u; i < out_count && i < surface->storage->document.layer_count; ++i) {
        uint32_t layer_id = surface->storage->document.layers[i].layer_id;
        uint32_t j;
        for (j = 0u; j < surface->layer_material_intent_entry_count; ++j) {
            if (surface->layer_material_intent_layer_ids[j] == layer_id) {
                out_intent_kind[i] = surface->layer_material_intent_values[j];
                break;
            }
        }
    }
}

void drawing_program_texture_project_set_surface_layer_role(
    DrawingProgramTextureProject *project,
    uint32_t surface_index,
    uint32_t layer_id,
    uint32_t role_kind) {
    DrawingProgramTextureSurface *surface = 0;
    uint32_t i;
    if (!texture_project_surface_index_valid(project, surface_index) || layer_id == 0u) {
        return;
    }
    surface = &project->surfaces[surface_index];
    if (!surface->storage) {
        return;
    }
    if (surface->layer_role_entry_count == 0u) {
        texture_project_surface_seed_layer_role_defaults(surface);
    }
    for (i = 0u; i < surface->layer_role_entry_count; ++i) {
        if (surface->layer_role_layer_ids[i] == layer_id) {
            surface->layer_role_values[i] = (uint8_t)role_kind;
            return;
        }
    }
    if (surface->layer_role_entry_count >= DRAWING_PROGRAM_MAX_LAYERS) {
        return;
    }
    surface->layer_role_layer_ids[surface->layer_role_entry_count] = layer_id;
    surface->layer_role_values[surface->layer_role_entry_count] = (uint8_t)role_kind;
    surface->layer_role_entry_count += 1u;
}

void drawing_program_texture_project_set_surface_layer_material_intent(
    DrawingProgramTextureProject *project,
    uint32_t surface_index,
    uint32_t layer_id,
    uint32_t intent_kind) {
    DrawingProgramTextureSurface *surface = 0;
    uint32_t i;
    if (!texture_project_surface_index_valid(project, surface_index) || layer_id == 0u) {
        return;
    }
    surface = &project->surfaces[surface_index];
    if (!surface->storage) {
        return;
    }
    if (surface->layer_material_intent_entry_count == 0u) {
        texture_project_surface_seed_layer_material_intent_defaults(
            surface, project->overlay_material_intent_kind);
    }
    for (i = 0u; i < surface->layer_material_intent_entry_count; ++i) {
        if (surface->layer_material_intent_layer_ids[i] == layer_id) {
            surface->layer_material_intent_values[i] = (uint8_t)intent_kind;
            return;
        }
    }
    if (surface->layer_material_intent_entry_count >= DRAWING_PROGRAM_MAX_LAYERS) {
        return;
    }
    surface->layer_material_intent_layer_ids[surface->layer_material_intent_entry_count] = layer_id;
    surface->layer_material_intent_values[surface->layer_material_intent_entry_count] =
        (uint8_t)intent_kind;
    surface->layer_material_intent_entry_count += 1u;
}

CoreResult drawing_program_texture_project_remove_surface(
    DrawingProgramTextureProject *project,
    uint32_t surface_index) {
    uint32_t move_count = 0u;
    if (!texture_project_surface_index_valid(project, surface_index)) {
        return texture_project_invalid("invalid texture surface remove request");
    }
    texture_project_surface_storage_dispose(project->surfaces[surface_index].storage);
    project->surfaces[surface_index].storage = 0;
    if (surface_index + 1u < project->surface_count) {
        move_count = project->surface_count - surface_index - 1u;
        memmove(&project->surfaces[surface_index],
                &project->surfaces[surface_index + 1u],
                (size_t)move_count * sizeof(project->surfaces[0]));
    }
    project->surface_count -= 1u;
    memset(&project->surfaces[project->surface_count], 0, sizeof(project->surfaces[0]));
    if (project->surface_count == 0u) {
        project->active_surface_index = 0u;
    } else if (project->active_surface_index >= project->surface_count) {
        project->active_surface_index = project->surface_count - 1u;
    }
    return core_result_ok();
}

CoreResult drawing_program_texture_project_clone_surface(
    const DrawingProgramTextureProject *project,
    uint32_t surface_index,
    DrawingProgramDocument *out_document,
    DrawingProgramLayerRasterStore *out_layer_rasters) {
    DrawingProgramTextureSurfaceStorage *storage = 0;
    CoreResult result;
    if (!texture_project_surface_index_valid(project, surface_index) || !out_document || !out_layer_rasters) {
        return texture_project_invalid("invalid texture surface clone request");
    }
    storage = project->surfaces[surface_index].storage;
    if (!storage) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "texture surface storage missing" };
    }
    memset(out_document, 0, sizeof(*out_document));
    drawing_program_layer_raster_store_dispose(out_layer_rasters);
    result = texture_project_storage_clone_from_state(&storage, &storage->document, &storage->layer_rasters);
    if (result.code != CORE_OK) {
        return result;
    }
    *out_document = storage->document;
    *out_layer_rasters = storage->layer_rasters;
    free(storage);
    return core_result_ok();
}

CoreResult drawing_program_texture_project_select_active_surface(
    DrawingProgramTextureProject *project,
    uint32_t surface_index) {
    if (!texture_project_surface_index_valid(project, surface_index)) {
        return texture_project_invalid("invalid active texture surface selection");
    }
    project->active_surface_index = surface_index;
    return core_result_ok();
}

const DrawingProgramTextureSurface *drawing_program_texture_project_surface_at(
    const DrawingProgramTextureProject *project,
    uint32_t surface_index) {
    if (!texture_project_surface_index_valid(project, surface_index)) {
        return 0;
    }
    return &project->surfaces[surface_index];
}

DrawingProgramTextureSurface *drawing_program_texture_project_surface_at_mut(
    DrawingProgramTextureProject *project,
    uint32_t surface_index) {
    if (!texture_project_surface_index_valid(project, surface_index)) {
        return 0;
    }
    return &project->surfaces[surface_index];
}

void drawing_program_texture_project_refresh_surface_flags(
    DrawingProgramTextureProject *project,
    uint32_t surface_index) {
    DrawingProgramTextureSurface *surface = 0;
    if (!texture_project_surface_index_valid(project, surface_index)) {
        return;
    }
    surface = &project->surfaces[surface_index];
    surface->is_blank = texture_project_storage_has_visible_content(surface->storage) ? 0u : 1u;
    surface->resize_locked = surface->is_blank ? 0u : 1u;
}

uint32_t drawing_program_texture_project_default_sample_density(uint32_t quality_preset) {
    switch ((DrawingProgramTextureQualityPreset)quality_preset) {
        case DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_HIGH:
            return 2u;
        case DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_ULTRA:
            return 4u;
        case DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_CUSTOM:
        case DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD:
        default:
            return 1u;
    }
}

const char *drawing_program_texture_project_face_role_name(uint32_t face_role) {
    switch ((DrawingProgramTextureFaceRole)face_role) {
        case DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT: return "FRONT";
        case DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BACK: return "BACK";
        case DRAWING_PROGRAM_TEXTURE_FACE_ROLE_LEFT: return "LEFT";
        case DRAWING_PROGRAM_TEXTURE_FACE_ROLE_RIGHT: return "RIGHT";
        case DRAWING_PROGRAM_TEXTURE_FACE_ROLE_TOP: return "TOP";
        case DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BOTTOM: return "BOTTOM";
        case DRAWING_PROGRAM_TEXTURE_FACE_ROLE_UNSPECIFIED:
        default: return "SURFACE";
    }
}

const char *drawing_program_texture_project_quality_preset_name(uint32_t quality_preset) {
    switch ((DrawingProgramTextureQualityPreset)quality_preset) {
        case DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_HIGH: return "HIGH";
        case DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_ULTRA: return "ULTRA";
        case DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_CUSTOM: return "CUSTOM";
        case DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD:
        default: return "STANDARD";
    }
}
