#include "drawing_program/drawing_program_texture_scene_import.h"

#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

#include "core_object.h"
#include "drawing_program_texture_scene_file_guard.h"

static CoreResult texture_scene_import_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static CoreResult texture_scene_import_context_error(CoreError code,
                                                     const char *scene_path,
                                                     const char *object_id,
                                                     const char *primitive_kind,
                                                     const char *field,
                                                     const char *detail) {
    static char message[768];
    (void)snprintf(message,
                   sizeof(message),
                   "texture scene import failure scene=%s object_id=%s primitive=%s field=%s detail=%s",
                   scene_path ? scene_path : "(null)",
                   object_id ? object_id : "(null)",
                   primitive_kind ? primitive_kind : "(unknown)",
                   field ? field : "(unknown)",
                   detail ? detail : "unknown");
    return (CoreResult){ code, message };
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

static CoreResult texture_scene_import_validate_text_field(const char *value,
                                                           size_t out_cap,
                                                           const char *scene_json_path,
                                                           const char *object_id,
                                                           const char *primitive_kind,
                                                           const char *field) {
    if (!drawing_program_texture_scene_file_guard_text_is_bounded(value, out_cap)) {
        return texture_scene_import_context_error(CORE_ERR_INVALID_ARG,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  field,
                                                  "scene text field is empty, too long, or contains controls");
    }
    return core_result_ok();
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
                                                   const char *scene_json_path,
                                                   const char *object_id,
                                                   const char *primitive_kind,
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
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  "dimensional_mode",
                                                  "plane primitive dimensional_mode must be plane_locked");
    }
    if (!texture_scene_import_get_required_string(object_json, "locked_plane", &locked_plane) ||
        !texture_scene_import_apply_locked_plane(out_contract, locked_plane)) {
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  "locked_plane",
                                                  "plane primitive locked_plane missing or invalid");
    }
    if (!texture_scene_import_get_required_number(primitive_json, "width", &out_contract->plane_primitive.width)) {
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  "primitive.width",
                                                  "plane primitive width missing or invalid");
    }
    if (!texture_scene_import_get_required_number(primitive_json, "height", &out_contract->plane_primitive.height)) {
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  "primitive.height",
                                                  "plane primitive height missing or invalid");
    }
    if (!texture_scene_import_get_required_object(primitive_json, "frame", &frame)) {
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  "primitive.frame",
                                                  "plane primitive frame missing or invalid");
    }
    if (!texture_scene_import_parse_frame(frame, &out_contract->plane_primitive.frame)) {
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  "primitive.frame",
                                                  "plane primitive frame payload is missing required vector fields");
    }
    if (!texture_scene_import_get_optional_bool(primitive_json,
                                                "lock_to_construction_plane",
                                                &out_contract->plane_primitive.lock_to_construction_plane)) {
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  "primitive.lock_to_construction_plane",
                                                  "plane primitive lock_to_construction_plane must be boolean");
    }
    if (!texture_scene_import_get_optional_bool(primitive_json,
                                                "lock_to_bounds",
                                                &out_contract->plane_primitive.lock_to_bounds)) {
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  "primitive.lock_to_bounds",
                                                  "plane primitive lock_to_bounds must be boolean");
    }
    out_contract->has_plane_primitive = true;
    result = core_scene_object_contract_validate(out_contract);
    return result;
}

static CoreResult texture_scene_import_parse_rect_prism(json_object *object_json,
                                                        json_object *primitive_json,
                                                        const char *scene_json_path,
                                                        const char *object_id,
                                                        const char *primitive_kind,
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
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  "dimensional_mode",
                                                  "rect prism dimensional_mode must be full_3d");
    }
    if (!texture_scene_import_get_required_number(primitive_json, "width", &out_contract->rect_prism_primitive.width)) {
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  "primitive.width",
                                                  "rect prism primitive width missing or invalid");
    }
    if (!texture_scene_import_get_required_number(primitive_json, "height", &out_contract->rect_prism_primitive.height)) {
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  "primitive.height",
                                                  "rect prism primitive height missing or invalid");
    }
    if (!texture_scene_import_get_required_number(primitive_json, "depth", &out_contract->rect_prism_primitive.depth)) {
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  "primitive.depth",
                                                  "rect prism primitive depth missing or invalid");
    }
    if (!texture_scene_import_get_required_object(primitive_json, "frame", &frame)) {
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  "primitive.frame",
                                                  "rect prism primitive frame missing or invalid");
    }
    if (!texture_scene_import_parse_frame(frame, &out_contract->rect_prism_primitive.frame)) {
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  "primitive.frame",
                                                  "rect prism primitive frame payload is missing required vector fields");
    }
    if (!texture_scene_import_get_optional_bool(primitive_json,
                                                "lock_to_construction_plane",
                                                &out_contract->rect_prism_primitive.lock_to_construction_plane)) {
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  "primitive.lock_to_construction_plane",
                                                  "rect prism primitive lock_to_construction_plane must be boolean");
    }
    if (!texture_scene_import_get_optional_bool(primitive_json,
                                                "lock_to_bounds",
                                                &out_contract->rect_prism_primitive.lock_to_bounds)) {
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  primitive_kind,
                                                  "primitive.lock_to_bounds",
                                                  "rect prism primitive lock_to_bounds must be boolean");
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
    result = drawing_program_texture_scene_file_guard_check_json_file(scene_json_path);
    if (result.code != CORE_OK) {
        return texture_scene_import_context_error(
            result.code, scene_json_path, object_id, 0, "scene_file", result.message);
    }
    root = json_object_from_file(scene_json_path);
    if (!root || !json_object_is_type(root, json_type_object)) {
        if (root) {
            json_object_put(root);
        }
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  0,
                                                  "scene",
                                                  "failed to parse scene JSON");
    }
    if (texture_scene_import_get_required_string(root, "scene_id", &scene_id)) {
        result = texture_scene_import_validate_text_field(scene_id,
                                                          sizeof(out_scene_root->scene_id),
                                                          scene_json_path,
                                                          object_id,
                                                          0,
                                                          "scene_id");
        if (result.code != CORE_OK) {
            json_object_put(root);
            return result;
        }
        result = core_scene_root_contract_set_scene_id(out_scene_root, scene_id);
        if (result.code != CORE_OK) {
            json_object_put(root);
            return texture_scene_import_context_error(
                result.code, scene_json_path, object_id, 0, "scene_id", result.message);
        }
    }
    if (!json_object_object_get_ex(root, "objects", &objects) ||
        !objects ||
        !json_object_is_type(objects, json_type_array)) {
        json_object_put(root);
        return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                  scene_json_path,
                                                  object_id,
                                                  0,
                                                  "objects",
                                                  "scene JSON is missing objects array");
    }
    object_count = json_object_array_length(objects);
    result = drawing_program_texture_scene_file_guard_check_object_count(object_count, scene_json_path, "objects");
    if (result.code != CORE_OK) {
        json_object_put(root);
        return texture_scene_import_context_error(
            result.code, scene_json_path, object_id, 0, "objects", result.message);
    }
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
        result = texture_scene_import_validate_text_field(candidate_object_id,
                                                          sizeof(out_scene_object->object.object_id),
                                                          scene_json_path,
                                                          candidate_object_id,
                                                          0,
                                                          "object_id");
        if (result.code != CORE_OK) {
            json_object_put(root);
            return result;
        }
        if (!texture_scene_import_get_required_string(object_json, "object_type", &object_type) ||
            core_scene_object_kind_parse(object_type, &kind).code != CORE_OK) {
            json_object_put(root);
            return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                      scene_json_path,
                                                      candidate_object_id,
                                                      0,
                                                      "object_type",
                                                      "scene object_type is missing or unsupported");
        }
        result = texture_scene_import_validate_text_field(object_type,
                                                          sizeof(out_scene_object->object.object_type),
                                                          scene_json_path,
                                                          candidate_object_id,
                                                          object_type,
                                                          "object_type");
        if (result.code != CORE_OK) {
            json_object_put(root);
            return result;
        }
        result = core_scene_object_contract_prepare(out_scene_object, candidate_object_id, kind);
        if (result.code != CORE_OK) {
            json_object_put(root);
            return texture_scene_import_context_error(result.code,
                                                      scene_json_path,
                                                      candidate_object_id,
                                                      object_type,
                                                      "object_contract",
                                                      result.message);
        }
        if (!texture_scene_import_get_required_object(object_json, "primitive", &primitive_json) ||
            !texture_scene_import_get_required_string(primitive_json, "kind", &primitive_kind) ||
            strcmp(primitive_kind, object_type) != 0) {
            json_object_put(root);
            return texture_scene_import_context_error(CORE_ERR_FORMAT,
                                                      scene_json_path,
                                                      candidate_object_id,
                                                      primitive_kind ? primitive_kind : object_type,
                                                      "primitive.kind",
                                                      "scene primitive payload missing or mismatched");
        }
        result = texture_scene_import_validate_text_field(primitive_kind,
                                                          sizeof(out_scene_object->object.object_type),
                                                          scene_json_path,
                                                          candidate_object_id,
                                                          primitive_kind,
                                                          "primitive.kind");
        if (result.code != CORE_OK) {
            json_object_put(root);
            return result;
        }
        switch (kind) {
            case CORE_SCENE_OBJECT_KIND_PLANE_PRIMITIVE:
                result = texture_scene_import_parse_plane(object_json,
                                                          primitive_json,
                                                          scene_json_path,
                                                          candidate_object_id,
                                                          primitive_kind,
                                                          out_scene_object);
                json_object_put(root);
                return result;
            case CORE_SCENE_OBJECT_KIND_RECT_PRISM_PRIMITIVE:
                result = texture_scene_import_parse_rect_prism(object_json,
                                                               primitive_json,
                                                               scene_json_path,
                                                               candidate_object_id,
                                                               primitive_kind,
                                                               out_scene_object);
                json_object_put(root);
                return result;
            default:
                json_object_put(root);
                return texture_scene_import_context_error(CORE_ERR_INVALID_ARG,
                                                          scene_json_path,
                                                          candidate_object_id,
                                                          primitive_kind,
                                                          "object_type",
                                                          "scene object kind is not supported for texture import");
        }
    }
    json_object_put(root);
    return texture_scene_import_context_error(CORE_ERR_NOT_FOUND,
                                              scene_json_path,
                                              object_id,
                                              0,
                                              "object_id",
                                              "scene object_id was not found in scene JSON");
}
