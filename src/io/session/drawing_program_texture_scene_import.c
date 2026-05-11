#include "drawing_program/drawing_program_texture_scene_import.h"

#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

static CoreResult texture_scene_import_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static int texture_scene_import_get_required_string(json_object *object,
                                                    const char *key,
                                                    const char **out_value) {
    json_object *value = 0;
    if (!object || !key || !out_value) {
        return 0;
    }
    if (!json_object_object_get_ex(object, key, &value) ||
        !value ||
        !json_object_is_type(value, json_type_string)) {
        return 0;
    }
    *out_value = json_object_get_string(value);
    return (*out_value && (*out_value)[0] != '\0') ? 1 : 0;
}

static int texture_scene_import_get_optional_bool(json_object *object,
                                                  const char *key,
                                                  bool *out_value) {
    json_object *value = 0;
    if (!object || !key || !out_value) {
        return 0;
    }
    if (!json_object_object_get_ex(object, key, &value)) {
        return 1;
    }
    if (!value || !json_object_is_type(value, json_type_boolean)) {
        return 0;
    }
    *out_value = json_object_get_boolean(value) ? true : false;
    return 1;
}

static int texture_scene_import_get_required_number(json_object *object,
                                                    const char *key,
                                                    double *out_value) {
    json_object *value = 0;
    if (!object || !key || !out_value) {
        return 0;
    }
    if (!json_object_object_get_ex(object, key, &value) ||
        !value ||
        !(json_object_is_type(value, json_type_double) || json_object_is_type(value, json_type_int))) {
        return 0;
    }
    *out_value = json_object_get_double(value);
    return 1;
}

static int texture_scene_import_get_required_object(json_object *object,
                                                    const char *key,
                                                    json_object **out_child) {
    json_object *value = 0;
    if (!object || !key || !out_child) {
        return 0;
    }
    if (!json_object_object_get_ex(object, key, &value) ||
        !value ||
        !json_object_is_type(value, json_type_object)) {
        return 0;
    }
    *out_child = value;
    return 1;
}

static int texture_scene_import_parse_vec3(json_object *object, CoreObjectVec3 *out_vec) {
    if (!out_vec ||
        !texture_scene_import_get_required_number(object, "x", &out_vec->x) ||
        !texture_scene_import_get_required_number(object, "y", &out_vec->y) ||
        !texture_scene_import_get_required_number(object, "z", &out_vec->z)) {
        return 0;
    }
    return 1;
}

static int texture_scene_import_parse_frame(json_object *object, CoreSceneFrame3 *out_frame) {
    json_object *origin = 0;
    json_object *axis_u = 0;
    json_object *axis_v = 0;
    json_object *normal = 0;
    if (!out_frame ||
        !texture_scene_import_get_required_object(object, "origin", &origin) ||
        !texture_scene_import_get_required_object(object, "axis_u", &axis_u) ||
        !texture_scene_import_get_required_object(object, "axis_v", &axis_v) ||
        !texture_scene_import_get_required_object(object, "normal", &normal) ||
        !texture_scene_import_parse_vec3(origin, &out_frame->origin) ||
        !texture_scene_import_parse_vec3(axis_u, &out_frame->axis_u) ||
        !texture_scene_import_parse_vec3(axis_v, &out_frame->axis_v) ||
        !texture_scene_import_parse_vec3(normal, &out_frame->normal)) {
        return 0;
    }
    return 1;
}

static int texture_scene_import_apply_locked_plane(CoreSceneObjectContract *contract, const char *locked_plane_text) {
    if (!contract || !locked_plane_text || locked_plane_text[0] == '\0') {
        return 0;
    }
    if (strcmp(locked_plane_text, "xy") == 0) {
        return core_object_set_plane_lock(&contract->object, CORE_OBJECT_PLANE_XY).code == CORE_OK;
    }
    if (strcmp(locked_plane_text, "yz") == 0) {
        return core_object_set_plane_lock(&contract->object, CORE_OBJECT_PLANE_YZ).code == CORE_OK;
    }
    if (strcmp(locked_plane_text, "xz") == 0) {
        return core_object_set_plane_lock(&contract->object, CORE_OBJECT_PLANE_XZ).code == CORE_OK;
    }
    return 0;
}

static CoreResult texture_scene_import_parse_plane(json_object *object_json,
                                                   json_object *primitive_json,
                                                   CoreSceneObjectContract *out_contract) {
    json_object *frame = 0;
    const char *dimensional_mode = 0;
    const char *locked_plane = 0;
    CoreResult result;
    if (!object_json || !primitive_json || !out_contract) {
        return texture_scene_import_invalid("invalid plane import request");
    }
    if (!texture_scene_import_get_required_string(object_json, "dimensional_mode", &dimensional_mode) ||
        strcmp(dimensional_mode, "plane_locked") != 0) {
        return (CoreResult){ CORE_ERR_FORMAT, "plane primitive dimensional_mode must be plane_locked" };
    }
    if (!texture_scene_import_get_required_string(object_json, "locked_plane", &locked_plane) ||
        !texture_scene_import_apply_locked_plane(out_contract, locked_plane)) {
        return (CoreResult){ CORE_ERR_FORMAT, "plane primitive locked_plane missing or invalid" };
    }
    if (!texture_scene_import_get_required_number(primitive_json, "width", &out_contract->plane_primitive.width) ||
        !texture_scene_import_get_required_number(primitive_json, "height", &out_contract->plane_primitive.height) ||
        !texture_scene_import_get_required_object(primitive_json, "frame", &frame) ||
        !texture_scene_import_parse_frame(frame, &out_contract->plane_primitive.frame) ||
        !texture_scene_import_get_optional_bool(primitive_json,
                                                "lock_to_construction_plane",
                                                &out_contract->plane_primitive.lock_to_construction_plane) ||
        !texture_scene_import_get_optional_bool(primitive_json,
                                                "lock_to_bounds",
                                                &out_contract->plane_primitive.lock_to_bounds)) {
        return (CoreResult){ CORE_ERR_FORMAT, "plane primitive payload is missing required fields" };
    }
    out_contract->has_plane_primitive = true;
    result = core_scene_object_contract_validate(out_contract);
    return result;
}

static CoreResult texture_scene_import_parse_rect_prism(json_object *object_json,
                                                        json_object *primitive_json,
                                                        CoreSceneObjectContract *out_contract) {
    json_object *frame = 0;
    const char *dimensional_mode = 0;
    CoreResult result;
    (void)object_json;
    if (!primitive_json || !out_contract) {
        return texture_scene_import_invalid("invalid rect-prism import request");
    }
    if (!texture_scene_import_get_required_string(object_json, "dimensional_mode", &dimensional_mode) ||
        strcmp(dimensional_mode, "full_3d") != 0) {
        return (CoreResult){ CORE_ERR_FORMAT, "rect prism dimensional_mode must be full_3d" };
    }
    if (!texture_scene_import_get_required_number(primitive_json, "width", &out_contract->rect_prism_primitive.width) ||
        !texture_scene_import_get_required_number(primitive_json, "height", &out_contract->rect_prism_primitive.height) ||
        !texture_scene_import_get_required_number(primitive_json, "depth", &out_contract->rect_prism_primitive.depth) ||
        !texture_scene_import_get_required_object(primitive_json, "frame", &frame) ||
        !texture_scene_import_parse_frame(frame, &out_contract->rect_prism_primitive.frame) ||
        !texture_scene_import_get_optional_bool(primitive_json,
                                                "lock_to_construction_plane",
                                                &out_contract->rect_prism_primitive.lock_to_construction_plane) ||
        !texture_scene_import_get_optional_bool(primitive_json,
                                                "lock_to_bounds",
                                                &out_contract->rect_prism_primitive.lock_to_bounds)) {
        return (CoreResult){ CORE_ERR_FORMAT, "rect prism primitive payload is missing required fields" };
    }
    out_contract->has_rect_prism_primitive = true;
    result = core_scene_object_contract_validate(out_contract);
    return result;
}

CoreResult drawing_program_texture_scene_import_load_object(
    const char *scene_json_path,
    const char *object_id,
    CoreSceneRootContract *out_scene_root,
    CoreSceneObjectContract *out_scene_object) {
    json_object *root = 0;
    json_object *objects = 0;
    const char *scene_id = 0;
    size_t object_count = 0u;
    size_t i;
    CoreResult result;
    if (!scene_json_path || scene_json_path[0] == '\0' || !object_id || object_id[0] == '\0' ||
        !out_scene_root || !out_scene_object) {
        return texture_scene_import_invalid("invalid scene texture object import request");
    }
    core_scene_root_contract_init(out_scene_root);
    core_scene_object_contract_init(out_scene_object);
    root = json_object_from_file(scene_json_path);
    if (!root || !json_object_is_type(root, json_type_object)) {
        if (root) {
            json_object_put(root);
        }
        return (CoreResult){ CORE_ERR_FORMAT, "failed to parse scene JSON" };
    }
    if (texture_scene_import_get_required_string(root, "scene_id", &scene_id)) {
        (void)core_scene_root_contract_set_scene_id(out_scene_root, scene_id);
    }
    if (!json_object_object_get_ex(root, "objects", &objects) ||
        !objects ||
        !json_object_is_type(objects, json_type_array)) {
        json_object_put(root);
        return (CoreResult){ CORE_ERR_FORMAT, "scene JSON is missing objects array" };
    }
    object_count = json_object_array_length(objects);
    for (i = 0u; i < object_count; ++i) {
        json_object *object_json = json_object_array_get_idx(objects, i);
        json_object *primitive_json = 0;
        const char *candidate_object_id = 0;
        const char *object_type = 0;
        const char *primitive_kind = 0;
        CoreSceneObjectKind kind = CORE_SCENE_OBJECT_KIND_UNKNOWN;
        if (!object_json || !json_object_is_type(object_json, json_type_object) ||
            !texture_scene_import_get_required_string(object_json, "object_id", &candidate_object_id) ||
            strcmp(candidate_object_id, object_id) != 0) {
            continue;
        }
        if (!texture_scene_import_get_required_string(object_json, "object_type", &object_type) ||
            core_scene_object_kind_parse(object_type, &kind).code != CORE_OK) {
            json_object_put(root);
            return (CoreResult){ CORE_ERR_FORMAT, "scene object_type is missing or unsupported" };
        }
        result = core_scene_object_contract_prepare(out_scene_object, candidate_object_id, kind);
        if (result.code != CORE_OK) {
            json_object_put(root);
            return result;
        }
        if (!texture_scene_import_get_required_object(object_json, "primitive", &primitive_json) ||
            !texture_scene_import_get_required_string(primitive_json, "kind", &primitive_kind) ||
            strcmp(primitive_kind, object_type) != 0) {
            json_object_put(root);
            return (CoreResult){ CORE_ERR_FORMAT, "scene primitive payload missing or mismatched" };
        }
        switch (kind) {
            case CORE_SCENE_OBJECT_KIND_PLANE_PRIMITIVE:
                result = texture_scene_import_parse_plane(object_json, primitive_json, out_scene_object);
                json_object_put(root);
                return result;
            case CORE_SCENE_OBJECT_KIND_RECT_PRISM_PRIMITIVE:
                result = texture_scene_import_parse_rect_prism(object_json, primitive_json, out_scene_object);
                json_object_put(root);
                return result;
            default:
                json_object_put(root);
                return (CoreResult){ CORE_ERR_INVALID_ARG, "scene object kind is not supported for texture import" };
        }
    }
    json_object_put(root);
    return (CoreResult){ CORE_ERR_NOT_FOUND, "scene object_id was not found in scene JSON" };
}
