#include "drawing_program/drawing_program_texture_project_template.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_texture_net.h"

typedef struct DrawingProgramTextureTemplateSurfaceSpec {
    const char *name;
    uint32_t face_role;
    double width_units;
    double height_units;
} DrawingProgramTextureTemplateSurfaceSpec;

static CoreResult texture_template_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static double texture_template_base_pixels_per_unit(uint32_t quality_preset) {
    switch (quality_preset) {
        case DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD: return 128.0;
        case DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_HIGH: return 256.0;
        case DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_ULTRA: return 384.0;
        case DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_CUSTOM: return 256.0;
        default: return 128.0;
    }
}

static uint32_t texture_template_scaled_dimension(double units, double pixels_per_unit) {
    double scaled = 0.0;
    if (!(units > 0.0) || !(pixels_per_unit > 0.0)) {
        return 1u;
    }
    scaled = floor((units * pixels_per_unit) + 0.5);
    if (!(scaled >= 1.0)) {
        return 1u;
    }
    if (scaled > 65535.0) {
        return 65535u;
    }
    return (uint32_t)scaled;
}

static double texture_template_resolve_pixels_per_unit(const DrawingProgramTextureTemplateSurfaceSpec *surfaces,
                                                       uint32_t surface_count,
                                                       uint32_t quality_preset) {
    double base = texture_template_base_pixels_per_unit(quality_preset);
    double limit = base;
    uint32_t i;
    if (!surfaces || surface_count == 0u) {
        return base;
    }
    for (i = 0u; i < surface_count; ++i) {
        double width_units = surfaces[i].width_units;
        double height_units = surfaces[i].height_units;
        double area_units = width_units * height_units;
        double max_for_surface = 0.0;
        if (!(width_units > 0.0) || !(height_units > 0.0) || !(area_units > 0.0)) {
            continue;
        }
        max_for_surface = sqrt((double)DRAWING_PROGRAM_MAX_RASTER_SAMPLES / area_units);
        if (max_for_surface < limit) {
            limit = max_for_surface;
        }
    }
    if (!(limit >= 1.0)) {
        limit = 1.0;
    }
    if (limit > base) {
        limit = base;
    }
    return floor(limit);
}

static CoreResult texture_template_append_surface(DrawingProgramTextureProject *project,
                                                  const DrawingProgramTextureTemplateSurfaceSpec *spec,
                                                  uint32_t primitive_kind,
                                                  double pixels_per_unit,
                                                  uint32_t quality_preset) {
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t surface_index = 0u;
    DrawingProgramTextureSurfaceSemantic semantic;
    CoreResult result;
    if (!project || !spec) {
        return texture_template_invalid("invalid texture template surface append request");
    }
    logical_width = texture_template_scaled_dimension(spec->width_units, pixels_per_unit);
    logical_height = texture_template_scaled_dimension(spec->height_units, pixels_per_unit);
    result = drawing_program_texture_project_add_surface(project,
                                                         spec->name,
                                                         logical_width,
                                                         logical_height,
                                                         1u,
                                                         spec->face_role,
                                                         quality_preset,
                                                         &surface_index);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_texture_net_seed_surface_semantic(primitive_kind, spec->face_role, &semantic);
    if (result.code != CORE_OK) {
        return result;
    }
    return drawing_program_texture_project_set_surface_semantic(project, surface_index, &semantic);
}

CoreResult drawing_program_texture_project_init_from_scene_object(
    DrawingProgramTextureProject *project,
    const CoreSceneRootContract *scene_root,
    const CoreSceneObjectContract *scene_object,
    uint32_t quality_preset) {
    DrawingProgramTextureTemplateSurfaceSpec surfaces[6];
    uint32_t surface_count = 0u;
    uint32_t primitive_kind = DRAWING_PROGRAM_TEXTURE_PRIMITIVE_KIND_NONE;
    double pixels_per_unit = 1.0;
    CoreResult result;
    uint32_t i;
    if (!project || !scene_object) {
        return texture_template_invalid("invalid scene-object texture template request");
    }
    result = core_scene_object_contract_validate(scene_object);
    if (result.code != CORE_OK) {
        return result;
    }
    memset(surfaces, 0, sizeof(surfaces));
    switch (scene_object->kind) {
        case CORE_SCENE_OBJECT_KIND_PLANE_PRIMITIVE:
            primitive_kind = DRAWING_PROGRAM_TEXTURE_PRIMITIVE_KIND_PLANE;
            surfaces[0].name = "Front";
            surfaces[0].face_role = DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT;
            surfaces[0].width_units = scene_object->plane_primitive.width;
            surfaces[0].height_units = scene_object->plane_primitive.height;
            surface_count = 1u;
            break;
        case CORE_SCENE_OBJECT_KIND_RECT_PRISM_PRIMITIVE:
            primitive_kind = DRAWING_PROGRAM_TEXTURE_PRIMITIVE_KIND_RECT_PRISM;
            surfaces[0].name = "Front";
            surfaces[0].face_role = DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT;
            surfaces[0].width_units = scene_object->rect_prism_primitive.width;
            surfaces[0].height_units = scene_object->rect_prism_primitive.height;
            surfaces[1].name = "Back";
            surfaces[1].face_role = DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BACK;
            surfaces[1].width_units = scene_object->rect_prism_primitive.width;
            surfaces[1].height_units = scene_object->rect_prism_primitive.height;
            surfaces[2].name = "Left";
            surfaces[2].face_role = DRAWING_PROGRAM_TEXTURE_FACE_ROLE_LEFT;
            surfaces[2].width_units = scene_object->rect_prism_primitive.depth;
            surfaces[2].height_units = scene_object->rect_prism_primitive.height;
            surfaces[3].name = "Right";
            surfaces[3].face_role = DRAWING_PROGRAM_TEXTURE_FACE_ROLE_RIGHT;
            surfaces[3].width_units = scene_object->rect_prism_primitive.depth;
            surfaces[3].height_units = scene_object->rect_prism_primitive.height;
            surfaces[4].name = "Top";
            surfaces[4].face_role = DRAWING_PROGRAM_TEXTURE_FACE_ROLE_TOP;
            surfaces[4].width_units = scene_object->rect_prism_primitive.width;
            surfaces[4].height_units = scene_object->rect_prism_primitive.depth;
            surfaces[5].name = "Bottom";
            surfaces[5].face_role = DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BOTTOM;
            surfaces[5].width_units = scene_object->rect_prism_primitive.width;
            surfaces[5].height_units = scene_object->rect_prism_primitive.depth;
            surface_count = 6u;
            break;
        default:
            return (CoreResult){ CORE_ERR_INVALID_ARG, "scene object kind is not supported for texture template import" };
    }

    result = drawing_program_texture_project_init_empty(project);
    if (result.code != CORE_OK) {
        return result;
    }
    project->primitive_kind = primitive_kind;
    project->net_layout_kind = drawing_program_texture_net_default_layout_kind(primitive_kind);
    project->quality_preset = quality_preset;
    project->export_binding_kind = DRAWING_PROGRAM_TEXTURE_BINDING_KIND_SEPARATE_FACES;
    if (scene_root) {
        (void)snprintf(project->source_scene_id, sizeof(project->source_scene_id), "%s", scene_root->scene_id);
    }
    (void)snprintf(project->source_object_id, sizeof(project->source_object_id), "%s", scene_object->object.object_id);

    pixels_per_unit = texture_template_resolve_pixels_per_unit(surfaces, surface_count, quality_preset);
    for (i = 0u; i < surface_count; ++i) {
        result = texture_template_append_surface(project,
                                                 &surfaces[i],
                                                 primitive_kind,
                                                 pixels_per_unit,
                                                 quality_preset);
        if (result.code != CORE_OK) {
            drawing_program_texture_project_dispose(project);
            return result;
        }
    }
    project->active_surface_index = 0u;
    return core_result_ok();
}
