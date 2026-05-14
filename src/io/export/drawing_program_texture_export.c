#include "drawing_program/drawing_program_texture_export.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <json-c/json.h>

#include "core_authored_texture.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_export_image.h"
#include "drawing_program/drawing_program_render_domain.h"
#include "drawing_program/drawing_program_texture_layer_material_intent.h"
#include "drawing_program/drawing_program_texture_net.h"
#include "drawing_program/drawing_program_texture_overlay_material_intent.h"
#include "drawing_program/drawing_program_texture_project.h"
#include "drawing_program/drawing_program_texture_project_session.h"
#include "drawing_program/drawing_program_texture_layer_role.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"

typedef enum DrawingProgramTextureExportLaneKind {
    DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_FLATTENED = 0u,
    DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_BASE = 1u,
    DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_OVERLAY = 2u
} DrawingProgramTextureExportLaneKind;

static CoreResult texture_export_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static CoreResult texture_export_io_error(const char *message) {
    CoreResult r = { CORE_ERR_IO, message };
    return r;
}

static CoreResult texture_export_mkdirs_if_needed(const char *dir_path) {
    char buffer[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    size_t i;
    size_t len;
    if (!dir_path || dir_path[0] == '\0') {
        return core_result_ok();
    }
    len = strlen(dir_path);
    if (len >= sizeof(buffer)) {
        return texture_export_invalid("texture export directory path too long");
    }
    (void)snprintf(buffer, sizeof(buffer), "%s", dir_path);
    for (i = 1u; i < len; ++i) {
        if (buffer[i] != '/') {
            continue;
        }
        buffer[i] = '\0';
        if (buffer[0] != '\0') {
            if (mkdir(buffer, 0775) != 0 && errno != EEXIST) {
                return texture_export_io_error("failed to create texture export directory segment");
            }
        }
        buffer[i] = '/';
    }
    if (mkdir(buffer, 0775) != 0 && errno != EEXIST) {
        return texture_export_io_error("failed to create texture export directory");
    }
    return core_result_ok();
}

static const char *texture_export_face_role_file_stem(uint32_t face_role) {
    switch ((DrawingProgramTextureFaceRole)face_role) {
        case DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT: return "front";
        case DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BACK: return "back";
        case DRAWING_PROGRAM_TEXTURE_FACE_ROLE_LEFT: return "left";
        case DRAWING_PROGRAM_TEXTURE_FACE_ROLE_RIGHT: return "right";
        case DRAWING_PROGRAM_TEXTURE_FACE_ROLE_TOP: return "top";
        case DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BOTTOM: return "bottom";
        case DRAWING_PROGRAM_TEXTURE_FACE_ROLE_UNSPECIFIED:
        default: return "surface";
    }
}

static int texture_export_layer_included_for_lane(uint32_t layer_role_kind,
                                                  uint32_t export_intent_kind,
                                                  uint32_t lane_kind) {
    if (export_intent_kind != DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_BASE_PLUS_OVERLAY ||
        lane_kind == DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_FLATTENED) {
        return 1;
    }
    if (lane_kind == DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_OVERLAY) {
        return drawing_program_texture_layer_role_prefers_overlay(layer_role_kind);
    }
    return drawing_program_texture_layer_role_prefers_overlay(layer_role_kind) ? 0 : 1;
}

static int texture_export_layer_contributes_to_lane(const DrawingProgramLayer *layer,
                                                    uint8_t opacity_percent,
                                                    uint8_t layer_role_kind,
                                                    uint32_t export_intent_kind,
                                                    uint32_t lane_kind) {
    if (!layer || !layer->visible) {
        return 0;
    }
    if (opacity_percent == 0u) {
        return 0;
    }
    return texture_export_layer_included_for_lane(layer_role_kind,
                                                  export_intent_kind,
                                                  lane_kind);
}

static const char *texture_export_lane_file_suffix(uint32_t lane_kind) {
    switch ((DrawingProgramTextureExportLaneKind)lane_kind) {
        case DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_BASE: return "_base";
        case DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_OVERLAY: return "_overlay";
        case DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_FLATTENED:
        default: return "";
    }
}

static void texture_export_add_u8_array(json_object *object,
                                        const char *key,
                                        const uint8_t *values,
                                        size_t count) {
    json_object *array = 0;
    size_t i;
    if (!object || !key || !values) {
        return;
    }
    array = json_object_new_array();
    if (!array) {
        return;
    }
    for (i = 0u; i < count; ++i) {
        json_object_array_add(array, json_object_new_int((int)values[i]));
    }
    json_object_object_add(object, key, array);
}

static void texture_export_add_layer_material_intent_stable_ids(json_object *object,
                                                                const uint8_t *values,
                                                                size_t count) {
    json_object *array = 0;
    size_t i;
    if (!object || !values) {
        return;
    }
    array = json_object_new_array();
    if (!array) {
        return;
    }
    for (i = 0u; i < count; ++i) {
        json_object_array_add(array,
                              json_object_new_string(
                                  drawing_program_texture_layer_material_intent_stable_id(values[i])));
    }
    json_object_object_add(object, "layer_material_intent_stable_ids", array);
}

static void texture_export_add_material_intent_summary(json_object *object,
                                                       const char *key,
                                                       uint8_t material_intent_kind) {
    if (!object || !key ||
        material_intent_kind == DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE) {
        return;
    }
    json_object_object_add(object,
                           key,
                           json_object_new_string(
                               drawing_program_texture_layer_material_intent_stable_id(
                                   material_intent_kind)));
}

static uint32_t texture_export_collect_layer_material_intents_for_lane(
    const DrawingProgramDocument *document,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    const uint8_t *layer_role_kind_by_index,
    uint32_t layer_role_count,
    const uint8_t *layer_material_intent_kind_by_index,
    uint32_t layer_material_intent_count,
    uint32_t export_intent_kind,
    uint32_t lane_kind,
    uint8_t *out_values,
    uint32_t out_capacity) {
    uint32_t layer_index = 0u;
    uint32_t out_count = 0u;
    if (!document || !out_values || out_capacity == 0u) {
        return 0u;
    }
    for (layer_index = 0u; layer_index < document->layer_count && out_count < out_capacity;
         ++layer_index) {
        const DrawingProgramLayer *layer = &document->layers[layer_index];
        uint8_t opacity = 100u;
        uint8_t role_kind = (uint8_t)DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM;
        uint8_t material_intent_kind =
            (uint8_t)DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE;
        if (layer_opacity_percent && layer_index < layer_opacity_count) {
            opacity = layer_opacity_percent[layer_index];
            if (opacity > 100u) {
                opacity = 100u;
            }
        }
        if (layer_role_kind_by_index && layer_index < layer_role_count) {
            role_kind = layer_role_kind_by_index[layer_index];
        }
        if (!texture_export_layer_contributes_to_lane(layer,
                                                      opacity,
                                                      role_kind,
                                                      export_intent_kind,
                                                      lane_kind)) {
            continue;
        }
        if (layer_material_intent_kind_by_index && layer_index < layer_material_intent_count) {
            material_intent_kind = layer_material_intent_kind_by_index[layer_index];
        }
        out_values[out_count++] = material_intent_kind;
    }
    return out_count;
}

static void texture_export_collect_material_intent_summary_for_lane(
    const DrawingProgramDocument *document,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    const uint8_t *layer_role_kind_by_index,
    uint32_t layer_role_count,
    const uint8_t *layer_material_intent_kind_by_index,
    uint32_t layer_material_intent_count,
    uint32_t export_intent_kind,
    uint32_t lane_kind,
    uint8_t *out_base_material_intent_kind,
    uint8_t *out_overlay_material_intent_kind) {
    uint32_t layer_index = 0u;
    if (out_base_material_intent_kind) {
        *out_base_material_intent_kind =
            (uint8_t)DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE;
    }
    if (out_overlay_material_intent_kind) {
        *out_overlay_material_intent_kind =
            (uint8_t)DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE;
    }
    if (!document) {
        return;
    }
    for (layer_index = 0u; layer_index < document->layer_count; ++layer_index) {
        const DrawingProgramLayer *layer = &document->layers[layer_index];
        uint8_t opacity = 100u;
        uint8_t role_kind = (uint8_t)DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM;
        uint8_t material_intent_kind =
            (uint8_t)DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE;
        if (layer_opacity_percent && layer_index < layer_opacity_count) {
            opacity = layer_opacity_percent[layer_index];
            if (opacity > 100u) {
                opacity = 100u;
            }
        }
        if (layer_role_kind_by_index && layer_index < layer_role_count) {
            role_kind = layer_role_kind_by_index[layer_index];
        }
        if (!texture_export_layer_contributes_to_lane(layer,
                                                      opacity,
                                                      role_kind,
                                                      export_intent_kind,
                                                      lane_kind)) {
            continue;
        }
        if (layer_material_intent_kind_by_index && layer_index < layer_material_intent_count) {
            material_intent_kind = layer_material_intent_kind_by_index[layer_index];
        }
        if (drawing_program_texture_layer_material_intent_is_base_family(material_intent_kind)) {
            if (out_base_material_intent_kind) {
                *out_base_material_intent_kind = material_intent_kind;
            }
        } else if (drawing_program_texture_layer_material_intent_is_overlay_family(
                       material_intent_kind)) {
            if (out_overlay_material_intent_kind) {
                *out_overlay_material_intent_kind = material_intent_kind;
            }
        }
    }
}

static void texture_export_add_adjacent_face_roles(json_object *object,
                                                   const uint8_t *adjacent_face_roles,
                                                   size_t count) {
    json_object *array = 0;
    size_t i;
    if (!object || !adjacent_face_roles) {
        return;
    }
    array = json_object_new_array();
    if (!array) {
        return;
    }
    for (i = 0u; i < count; ++i) {
        json_object_array_add(array,
                              json_object_new_string(
                                  drawing_program_texture_project_face_role_name(adjacent_face_roles[i])));
    }
    json_object_object_add(object, "adjacent_face_roles", array);
}

static void texture_export_sanitize_component(const char *input, char *out, size_t out_cap) {
    size_t in_i = 0u;
    size_t out_i = 0u;
    int last_was_separator = 0;
    if (!out || out_cap == 0u) {
        return;
    }
    out[0] = '\0';
    if (!input || input[0] == '\0') {
        (void)snprintf(out, out_cap, "texture_project");
        return;
    }
    for (in_i = 0u; input[in_i] != '\0' && out_i + 1u < out_cap; ++in_i) {
        unsigned char c = (unsigned char)input[in_i];
        int is_safe =
            ((c >= (unsigned char)'a' && c <= (unsigned char)'z') ||
             (c >= (unsigned char)'A' && c <= (unsigned char)'Z') ||
             (c >= (unsigned char)'0' && c <= (unsigned char)'9'));
        if (is_safe) {
            out[out_i++] = (char)c;
            last_was_separator = 0;
            continue;
        }
        if (c == (unsigned char)'_' || c == (unsigned char)'-') {
            out[out_i++] = (char)c;
            last_was_separator = 0;
            continue;
        }
        if (!last_was_separator) {
            out[out_i++] = '_';
            last_was_separator = 1;
        }
    }
    while (out_i > 0u && (out[out_i - 1u] == '_' || out[out_i - 1u] == '-')) {
        out_i -= 1u;
    }
    out[out_i] = '\0';
    if (out[0] == '\0') {
        (void)snprintf(out, out_cap, "texture_project");
    }
}

static CoreResult texture_export_project_stem(const DrawingProgramAppContext *ctx,
                                              char *out_stem,
                                              size_t out_cap) {
    char base_name[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx || !out_stem || out_cap == 0u) {
        return texture_export_invalid("invalid texture export project stem request");
    }
    if (ctx->texture_project.source_object_id[0] != '\0') {
        texture_export_sanitize_component(ctx->texture_project.source_object_id, out_stem, out_cap);
        return core_result_ok();
    }
    result = drawing_program_export_image_default_basename(ctx, base_name, sizeof(base_name));
    if (result.code != CORE_OK) {
        return result;
    }
    texture_export_sanitize_component(base_name, out_stem, out_cap);
    return core_result_ok();
}

static CoreResult texture_export_compose_surface_rgba(const DrawingProgramDocument *document,
                                                      const DrawingProgramLayerRasterStore *layer_rasters,
                                                      const uint8_t *layer_opacity_percent,
                                                      uint32_t layer_opacity_count,
                                                      const uint8_t *layer_role_kind_by_index,
                                                      uint32_t layer_role_count,
                                                      uint32_t export_intent_kind,
                                                      uint32_t lane_kind,
                                                      uint8_t **out_rgba,
                                                      uint32_t *out_width,
                                                      uint32_t *out_height) {
    DrawingProgramRasterSample *samples = 0;
    uint8_t *rgba = 0;
    uint64_t rgba_capacity = 0u;
    uint32_t sample_index;
    if (!document || !layer_rasters || !out_rgba || !out_width || !out_height) {
        return texture_export_invalid("invalid texture surface compose request");
    }
    *out_rgba = 0;
    *out_width = 0u;
    *out_height = 0u;
    if (document->raster_sample_count == 0u || document->raster_sample_count > DRAWING_PROGRAM_MAX_RASTER_SAMPLES) {
        return texture_export_invalid("invalid texture surface raster sample count");
    }
    samples = core_alloc((size_t)document->raster_sample_count * sizeof(*samples));
    if (!samples) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate texture export sample buffer" };
    }
    rgba_capacity = (uint64_t)document->raster_sample_count * 4u;
    if (rgba_capacity == 0u || rgba_capacity > SIZE_MAX) {
        core_free(samples);
        return texture_export_invalid("invalid texture export RGBA capacity");
    }
    rgba = core_alloc((size_t)rgba_capacity);
    if (!rgba) {
        core_free(samples);
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate texture export RGBA buffer" };
    }
    for (sample_index = 0u; sample_index < document->raster_sample_count; ++sample_index) {
        DrawingProgramRasterSample composed = drawing_program_color_eraser_value();
        uint32_t layer_index;
        for (layer_index = 0u; layer_index < document->layer_count; ++layer_index) {
            const DrawingProgramLayer *layer = &document->layers[layer_index];
            const DrawingProgramRasterSample *layer_samples = 0;
            uint32_t exported_sample_count = 0u;
            uint8_t opacity = 100u;
            uint8_t role_kind = (uint8_t)DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM;
            DrawingProgramRasterSample sample = drawing_program_color_eraser_value();
            if (layer_role_kind_by_index && layer_index < layer_role_count) {
                role_kind = layer_role_kind_by_index[layer_index];
            }
            if (layer_opacity_percent && layer_index < layer_opacity_count) {
                opacity = layer_opacity_percent[layer_index];
                if (opacity > 100u) {
                    opacity = 100u;
                }
            }
            if (!texture_export_layer_contributes_to_lane(layer,
                                                          opacity,
                                                          role_kind,
                                                          export_intent_kind,
                                                          lane_kind)) {
                continue;
            }
            if (drawing_program_layer_raster_store_export_layer(layer_rasters,
                                                                layer->layer_id,
                                                                &layer_samples,
                                                                &exported_sample_count)
                    .code == CORE_OK &&
                layer_samples &&
                exported_sample_count == document->raster_sample_count) {
                sample = layer_samples[sample_index];
            } else if (layer_index == 0u) {
                sample = document->raster_samples[sample_index];
            }
            if (drawing_program_color_sample_is_transparent(sample)) {
                continue;
            }
            if (opacity >= 100u && drawing_program_color_sample_is_transparent(composed)) {
                composed = sample;
            } else {
                composed = drawing_program_color_blend_samples(composed, sample, opacity);
            }
        }
        samples[sample_index] = composed;
    }
    for (sample_index = 0u; sample_index < document->raster_sample_count; ++sample_index) {
        uint8_t r = 0u;
        uint8_t g = 0u;
        uint8_t b = 0u;
        uint8_t a = 255u;
        drawing_program_color_rgba_from_sample(samples[sample_index], &r, &g, &b, &a);
        rgba[(size_t)sample_index * 4u + 0u] = r;
        rgba[(size_t)sample_index * 4u + 1u] = g;
        rgba[(size_t)sample_index * 4u + 2u] = b;
        rgba[(size_t)sample_index * 4u + 3u] = a;
    }
    core_free(samples);
    *out_rgba = rgba;
    *out_width = document->raster_width;
    *out_height = document->raster_height;
    return core_result_ok();
}

static json_object *texture_export_build_surface_json(const DrawingProgramTextureSurface *surface,
                                                      const DrawingProgramDocument *document,
                                                      const char *png_name,
                                                      const uint8_t *layer_material_intent_kind_by_index,
                                                      uint32_t layer_material_intent_count,
                                                      uint8_t base_material_intent_kind,
                                                      uint8_t overlay_material_intent_kind) {
    json_object *surface_json = 0;
    if (!surface || !surface->storage || !document || !png_name) {
        return 0;
    }
    surface_json = json_object_new_object();
    if (!surface_json) {
        return 0;
    }
    json_object_object_add(surface_json, "surface_id", json_object_new_int((int)surface->surface_id));
    json_object_object_add(surface_json, "name", json_object_new_string(surface->name));
    json_object_object_add(surface_json,
                           "face_role",
                           json_object_new_string(drawing_program_texture_project_face_role_name(surface->face_role)));
    json_object_object_add(surface_json,
                           "quality_preset",
                           json_object_new_string(
                               drawing_program_texture_project_quality_preset_name(surface->quality_preset)));
    json_object_object_add(surface_json,
                           "net_layout_kind",
                           json_object_new_string(
                               drawing_program_texture_net_layout_kind_name(surface->semantic.layout_kind)));
    json_object_object_add(surface_json,
                           "net_slot",
                           json_object_new_string(
                               drawing_program_texture_net_slot_name(surface->semantic.net_slot)));
    json_object_object_add(surface_json,
                           "orientation",
                           json_object_new_string(
                               drawing_program_texture_net_orientation_name(surface->semantic.orientation)));
    json_object_object_add(surface_json, "logical_width", json_object_new_int((int)document->logical_width));
    json_object_object_add(surface_json, "logical_height", json_object_new_int((int)document->logical_height));
    json_object_object_add(surface_json, "sample_density", json_object_new_int((int)document->sample_density));
    json_object_object_add(surface_json, "raster_width", json_object_new_int((int)document->raster_width));
    json_object_object_add(surface_json, "raster_height", json_object_new_int((int)document->raster_height));
    texture_export_add_u8_array(surface_json,
                                "corner_ids",
                                surface->semantic.corner_ids,
                                DRAWING_PROGRAM_TEXTURE_NET_FACE_CORNER_COUNT);
    texture_export_add_u8_array(surface_json,
                                "edge_ids",
                                surface->semantic.edge_ids,
                                DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT);
    texture_export_add_adjacent_face_roles(surface_json,
                                           surface->semantic.adjacent_face_roles,
                                           DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT);
    json_object_object_add(surface_json,
                           "layout_offset_x",
                           json_object_new_double((double)surface->layout_offset_x));
    json_object_object_add(surface_json,
                           "layout_offset_y",
                           json_object_new_double((double)surface->layout_offset_y));
    if (layer_material_intent_kind_by_index) {
        texture_export_add_layer_material_intent_stable_ids(surface_json,
                                                            layer_material_intent_kind_by_index,
                                                            layer_material_intent_count);
    }
    texture_export_add_material_intent_summary(surface_json,
                                               "base_material_intent_kind",
                                               base_material_intent_kind);
    texture_export_add_material_intent_summary(surface_json,
                                               "overlay_material_intent_kind",
                                               overlay_material_intent_kind);
    json_object_object_add(surface_json, "file_name", json_object_new_string(png_name));
    return surface_json;
}

CoreResult drawing_program_texture_export_default_directory(const DrawingProgramAppContext *ctx,
                                                            char *out_path,
                                                            size_t out_cap) {
    char project_stem[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
    CoreResult result;
    if (!ctx || !out_path || out_cap == 0u) {
        return texture_export_invalid("invalid texture export default directory request");
    }
    result = texture_export_project_stem(ctx, project_stem, sizeof(project_stem));
    if (result.code != CORE_OK) {
        return result;
    }
    (void)snprintf(out_path, out_cap, "%s/%s_texture_set", ctx->session.output_root_path, project_stem);
    return core_result_ok();
}

CoreResult drawing_program_texture_export_current_project(DrawingProgramAppContext *ctx,
                                                          const char *dir_path) {
    char export_dir[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    char project_stem[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
    char manifest_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    json_object *manifest = 0;
    json_object *base_surfaces = 0;
    json_object *overlay_surfaces = 0;
    CoreResult result;
    uint32_t i;
    uint32_t emitted_output_kind = 0u;
    if (!ctx) {
        return texture_export_invalid("invalid texture export request");
    }
    if (ctx->texture_project.surface_count == 0u) {
        return texture_export_invalid("texture export requires at least one surface");
    }
    result = drawing_program_texture_project_session_commit_active_surface(ctx);
    if (result.code != CORE_OK) {
        return result;
    }
    result = texture_export_project_stem(ctx, project_stem, sizeof(project_stem));
    if (result.code != CORE_OK) {
        return result;
    }
    if (dir_path && dir_path[0] != '\0') {
        if (strlen(dir_path) >= sizeof(export_dir)) {
            return texture_export_invalid("texture export directory path too long");
        }
        (void)snprintf(export_dir, sizeof(export_dir), "%s", dir_path);
    } else {
        result = drawing_program_texture_export_default_directory(ctx, export_dir, sizeof(export_dir));
        if (result.code != CORE_OK) {
            return result;
        }
    }
    result = texture_export_mkdirs_if_needed(export_dir);
    if (result.code != CORE_OK) {
        return result;
    }

    ctx->texture_project.export_binding_kind = DRAWING_PROGRAM_TEXTURE_BINDING_KIND_SEPARATE_FACES;
    emitted_output_kind = drawing_program_texture_export_intent_emitted_output_kind(
        ctx->texture_project.export_intent_kind);
    manifest = json_object_new_object();
    base_surfaces = json_object_new_array();
    overlay_surfaces = json_object_new_array();
    if (!manifest || !base_surfaces || !overlay_surfaces) {
        if (manifest) {
            json_object_put(manifest);
        }
        if (base_surfaces) {
            json_object_put(base_surfaces);
        }
        if (overlay_surfaces) {
            json_object_put(overlay_surfaces);
        }
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate texture export manifest" };
    }
    json_object_object_add(manifest,
                           "schema_version",
                           json_object_new_int((int)CORE_AUTHORED_TEXTURE_SCHEMA_V5));
    json_object_object_add(manifest,
                           "export_binding_kind",
                           json_object_new_string(
                               core_authored_texture_binding_kind_name(
                                   (CoreAuthoredTextureBindingKind)
                                       ctx->texture_project.export_binding_kind)));
    json_object_object_add(manifest,
                           "export_intent_kind",
                           json_object_new_string(
                               drawing_program_texture_export_intent_kind_name(
                                   ctx->texture_project.export_intent_kind)));
    json_object_object_add(manifest,
                           "emitted_output_kind",
                           json_object_new_string(
                               core_authored_texture_output_kind_name(
                                   (CoreAuthoredTextureOutputKind)emitted_output_kind)));
    json_object_object_add(manifest,
                           "overlay_material_intent_kind",
                           json_object_new_string(
                               drawing_program_texture_overlay_material_intent_kind_name(
                                   ctx->texture_project.overlay_material_intent_kind)));
    json_object_object_add(manifest,
                           "primitive_kind",
                           json_object_new_string(
                               core_authored_texture_primitive_kind_name(
                                   (CoreAuthoredTexturePrimitiveKind)
                                       ctx->texture_project.primitive_kind)));
    json_object_object_add(manifest,
                           "quality_preset",
                           json_object_new_string(
                               drawing_program_texture_project_quality_preset_name(ctx->texture_project.quality_preset)));
    json_object_object_add(manifest,
                           "source_scene_id",
                           json_object_new_string(ctx->texture_project.source_scene_id));
    json_object_object_add(manifest,
                           "source_object_id",
                           json_object_new_string(ctx->texture_project.source_object_id));
    json_object_object_add(manifest, "base_surface_count", json_object_new_int((int)ctx->texture_project.surface_count));
    json_object_object_add(manifest, "base_surfaces", base_surfaces);
    if (emitted_output_kind == CORE_AUTHORED_TEXTURE_OUTPUT_KIND_BASE_PLUS_OVERLAY) {
        json_object_object_add(manifest,
                               "overlay_surface_count",
                               json_object_new_int((int)ctx->texture_project.surface_count));
        json_object_object_add(manifest, "overlay_surfaces", overlay_surfaces);
    } else {
        json_object_put(overlay_surfaces);
        overlay_surfaces = 0;
    }

    for (i = 0u; i < ctx->texture_project.surface_count; ++i) {
        const DrawingProgramTextureSurface *surface =
            drawing_program_texture_project_surface_at(&ctx->texture_project, i);
        const DrawingProgramDocument *document = 0;
        const DrawingProgramLayerRasterStore *layer_rasters = 0;
        char png_name[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        char base_png_name[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        char overlay_png_name[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        char png_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        char overlay_png_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        const char *face_stem = 0;
        uint8_t *rgba = 0;
        uint32_t width = 0u;
        uint32_t height = 0u;
        json_object *base_surface_json = 0;
        json_object *overlay_surface_json = 0;
        uint8_t surface_layer_opacity[DRAWING_PROGRAM_MAX_LAYERS];
        uint8_t surface_layer_roles[DRAWING_PROGRAM_MAX_LAYERS];
        uint8_t surface_layer_material_intents[DRAWING_PROGRAM_MAX_LAYERS];
        uint8_t base_lane_layer_material_intents[DRAWING_PROGRAM_MAX_LAYERS];
        uint8_t overlay_lane_layer_material_intents[DRAWING_PROGRAM_MAX_LAYERS];
        const uint8_t *layer_opacity_percent = 0;
        const uint8_t *layer_role_kind_by_index = 0;
        const uint8_t *layer_material_intent_kind_by_index = 0;
        uint32_t layer_opacity_count = 0u;
        uint32_t layer_role_count = 0u;
        uint32_t layer_material_intent_count = 0u;
        uint32_t base_lane_layer_material_intent_count = 0u;
        uint32_t overlay_lane_layer_material_intent_count = 0u;
        uint8_t base_lane_base_material_intent_kind =
            (uint8_t)DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE;
        uint8_t base_lane_overlay_material_intent_kind =
            (uint8_t)DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE;
        uint8_t overlay_lane_base_material_intent_kind =
            (uint8_t)DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE;
        uint8_t overlay_lane_overlay_material_intent_kind =
            (uint8_t)DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE;
        if (!surface || !surface->storage) {
            json_object_put(manifest);
            return (CoreResult){ CORE_ERR_FORMAT, "texture export surface storage missing" };
        }
        face_stem = texture_export_face_role_file_stem(surface->face_role);
        if (surface->face_role == DRAWING_PROGRAM_TEXTURE_FACE_ROLE_UNSPECIFIED) {
            (void)snprintf(png_name, sizeof(png_name), "%s_surface_%02u%s.png", project_stem, (unsigned)(i + 1u),
                           texture_export_lane_file_suffix(emitted_output_kind == CORE_AUTHORED_TEXTURE_OUTPUT_KIND_BASE_PLUS_OVERLAY
                                                               ? DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_BASE
                                                               : DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_FLATTENED));
            if (emitted_output_kind == CORE_AUTHORED_TEXTURE_OUTPUT_KIND_BASE_PLUS_OVERLAY) {
                (void)snprintf(base_png_name, sizeof(base_png_name), "%s_surface_%02u%s.png", project_stem, (unsigned)(i + 1u),
                               texture_export_lane_file_suffix(DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_BASE));
            } else {
                (void)snprintf(base_png_name, sizeof(base_png_name), "%s", png_name);
            }
            (void)snprintf(overlay_png_name, sizeof(overlay_png_name), "%s_surface_%02u%s.png", project_stem, (unsigned)(i + 1u),
                           texture_export_lane_file_suffix(DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_OVERLAY));
        } else {
            (void)snprintf(png_name, sizeof(png_name), "%s_%s%s.png", project_stem, face_stem,
                           texture_export_lane_file_suffix(emitted_output_kind == CORE_AUTHORED_TEXTURE_OUTPUT_KIND_BASE_PLUS_OVERLAY
                                                               ? DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_BASE
                                                               : DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_FLATTENED));
            if (emitted_output_kind == CORE_AUTHORED_TEXTURE_OUTPUT_KIND_BASE_PLUS_OVERLAY) {
                (void)snprintf(base_png_name, sizeof(base_png_name), "%s_%s%s.png", project_stem, face_stem,
                               texture_export_lane_file_suffix(DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_BASE));
            } else {
                (void)snprintf(base_png_name, sizeof(base_png_name), "%s", png_name);
            }
            (void)snprintf(overlay_png_name, sizeof(overlay_png_name), "%s_%s%s.png", project_stem, face_stem,
                           texture_export_lane_file_suffix(DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_OVERLAY));
        }
        (void)snprintf(png_path, sizeof(png_path), "%s/%s", export_dir, png_name);
        (void)snprintf(overlay_png_path, sizeof(overlay_png_path), "%s/%s", export_dir, overlay_png_name);
        document = &surface->storage->document;
        layer_rasters = &surface->storage->layer_rasters;
        memset(surface_layer_opacity, 0, sizeof(surface_layer_opacity));
        memset(surface_layer_roles, 0, sizeof(surface_layer_roles));
        memset(surface_layer_material_intents, 0, sizeof(surface_layer_material_intents));
        memset(base_lane_layer_material_intents, 0, sizeof(base_lane_layer_material_intents));
        memset(overlay_lane_layer_material_intents, 0, sizeof(overlay_lane_layer_material_intents));
        drawing_program_texture_project_collect_surface_layer_opacity_by_index(&ctx->texture_project,
                                                                               i,
                                                                               surface_layer_opacity,
                                                                               document->layer_count);
        drawing_program_texture_project_collect_surface_layer_role_by_index(&ctx->texture_project,
                                                                            i,
                                                                            surface_layer_roles,
                                                                            document->layer_count);
        drawing_program_texture_project_collect_surface_layer_material_intent_by_index(
            &ctx->texture_project,
            i,
            surface_layer_material_intents,
            document->layer_count);
        layer_opacity_percent = surface_layer_opacity;
        layer_role_kind_by_index = surface_layer_roles;
        layer_material_intent_kind_by_index = surface_layer_material_intents;
        layer_opacity_count = document->layer_count;
        layer_role_count = document->layer_count;
        layer_material_intent_count = document->layer_count;
        base_lane_layer_material_intent_count =
            texture_export_collect_layer_material_intents_for_lane(
                document,
                layer_opacity_percent,
                layer_opacity_count,
                layer_role_kind_by_index,
                layer_role_count,
                layer_material_intent_kind_by_index,
                layer_material_intent_count,
                ctx->texture_project.export_intent_kind,
                emitted_output_kind == CORE_AUTHORED_TEXTURE_OUTPUT_KIND_BASE_PLUS_OVERLAY
                    ? DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_BASE
                    : DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_FLATTENED,
                base_lane_layer_material_intents,
                DRAWING_PROGRAM_MAX_LAYERS);
        overlay_lane_layer_material_intent_count =
            texture_export_collect_layer_material_intents_for_lane(
                document,
                layer_opacity_percent,
                layer_opacity_count,
                layer_role_kind_by_index,
                layer_role_count,
                layer_material_intent_kind_by_index,
                layer_material_intent_count,
                ctx->texture_project.export_intent_kind,
                DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_OVERLAY,
                overlay_lane_layer_material_intents,
                DRAWING_PROGRAM_MAX_LAYERS);
        texture_export_collect_material_intent_summary_for_lane(
            document,
            layer_opacity_percent,
            layer_opacity_count,
            layer_role_kind_by_index,
            layer_role_count,
            layer_material_intent_kind_by_index,
            layer_material_intent_count,
            ctx->texture_project.export_intent_kind,
            emitted_output_kind == CORE_AUTHORED_TEXTURE_OUTPUT_KIND_BASE_PLUS_OVERLAY
                ? DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_BASE
                : DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_FLATTENED,
            &base_lane_base_material_intent_kind,
            &base_lane_overlay_material_intent_kind);
        texture_export_collect_material_intent_summary_for_lane(
            document,
            layer_opacity_percent,
            layer_opacity_count,
            layer_role_kind_by_index,
            layer_role_count,
            layer_material_intent_kind_by_index,
            layer_material_intent_count,
            ctx->texture_project.export_intent_kind,
            DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_OVERLAY,
            &overlay_lane_base_material_intent_kind,
            &overlay_lane_overlay_material_intent_kind);
        result = texture_export_compose_surface_rgba(document,
                                                     layer_rasters,
                                                     layer_opacity_percent,
                                                     layer_opacity_count,
                                                     layer_role_kind_by_index,
                                                     layer_role_count,
                                                     ctx->texture_project.export_intent_kind,
                                                     emitted_output_kind == CORE_AUTHORED_TEXTURE_OUTPUT_KIND_BASE_PLUS_OVERLAY
                                                         ? DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_BASE
                                                         : DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_FLATTENED,
                                                     &rgba,
                                                     &width,
                                                     &height);
        if (result.code != CORE_OK) {
            json_object_put(manifest);
            core_free(rgba);
            return result;
        }
        result = drawing_program_export_image_write_png_rgba(png_path, rgba, width, height);
        core_free(rgba);
        rgba = 0;
        if (result.code != CORE_OK) {
            json_object_put(manifest);
            return result;
        }
        if (emitted_output_kind == CORE_AUTHORED_TEXTURE_OUTPUT_KIND_BASE_PLUS_OVERLAY) {
            result = texture_export_compose_surface_rgba(document,
                                                         layer_rasters,
                                                         layer_opacity_percent,
                                                         layer_opacity_count,
                                                         layer_role_kind_by_index,
                                                         layer_role_count,
                                                         ctx->texture_project.export_intent_kind,
                                                         DRAWING_PROGRAM_TEXTURE_EXPORT_LANE_KIND_OVERLAY,
                                                         &rgba,
                                                         &width,
                                                         &height);
            if (result.code != CORE_OK) {
                json_object_put(manifest);
                core_free(rgba);
                return result;
            }
            result = drawing_program_export_image_write_png_rgba(overlay_png_path, rgba, width, height);
            core_free(rgba);
            rgba = 0;
            if (result.code != CORE_OK) {
                json_object_put(manifest);
                return result;
            }
        }
        base_surface_json = texture_export_build_surface_json(surface,
                                                              document,
                                                              base_png_name,
                                                              base_lane_layer_material_intents,
                                                              base_lane_layer_material_intent_count,
                                                              base_lane_base_material_intent_kind,
                                                              base_lane_overlay_material_intent_kind);
        if (!base_surface_json) {
            json_object_put(manifest);
            return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate texture export base surface manifest" };
        }
        json_object_array_add(base_surfaces, base_surface_json);
        if (overlay_surfaces) {
            overlay_surface_json = texture_export_build_surface_json(surface,
                                                                     document,
                                                                     overlay_png_name,
                                                                     overlay_lane_layer_material_intents,
                                                                     overlay_lane_layer_material_intent_count,
                                                                     overlay_lane_base_material_intent_kind,
                                                                     overlay_lane_overlay_material_intent_kind);
            if (!overlay_surface_json) {
                json_object_put(manifest);
                return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate texture export overlay surface manifest" };
            }
            json_object_array_add(overlay_surfaces, overlay_surface_json);
        }
    }

    (void)snprintf(manifest_path, sizeof(manifest_path), "%s/%s_texture_manifest.json", export_dir, project_stem);
    if (json_object_to_file_ext(manifest_path, manifest, JSON_C_TO_STRING_PRETTY) != 0) {
        json_object_put(manifest);
        return texture_export_io_error("failed to write texture export manifest");
    }
    json_object_put(manifest);
    return core_result_ok();
}
