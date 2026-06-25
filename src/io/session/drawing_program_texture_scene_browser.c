#include "drawing_program/drawing_program_texture_scene_browser.h"

#include <dirent.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_scene.h"
#include "drawing_program_texture_scene_file_guard.h"

static CoreResult texture_scene_browser_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static CoreResult texture_scene_browser_context_error(CoreError code,
                                                      const char *path,
                                                      const char *field,
                                                      const char *detail) {
    static char message[512];
    (void)snprintf(message,
                   sizeof(message),
                   "texture scene browser failure path=%s field=%s detail=%s",
                   path ? path : "(null)",
                   field ? field : "(unknown)",
                   detail ? detail : "unknown");
    return (CoreResult){ code, message };
}

static int texture_scene_browser_has_json_extension(const char *name) {
    const char *dot = 0;
    if (!name || name[0] == '\0') {
        return 0;
    }
    dot = strrchr(name, '.');
    return (dot && strcmp(dot, ".json") == 0) ? 1 : 0;
}

static CoreResult texture_scene_browser_join_path(char *out,
                                                  size_t out_cap,
                                                  const char *root_path,
                                                  const char *file_name) {
    int written;
    if (!out || out_cap == 0u || !root_path || !file_name || root_path[0] == '\0' || file_name[0] == '\0') {
        return texture_scene_browser_invalid("invalid scene browser path join request");
    }
    written = snprintf(out, out_cap, "%s/%s", root_path, file_name);
    if (written < 0 || (size_t)written >= out_cap) {
        return texture_scene_browser_context_error(
            CORE_ERR_INVALID_ARG, root_path, "scene_file", "scene browser path too long");
    }
    return core_result_ok();
}

static int texture_scene_browser_get_required_string(json_object *object,
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

static int texture_scene_browser_get_required_number(json_object *object,
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

static int texture_scene_browser_scene_sort_compare(const void *lhs, const void *rhs) {
    const DrawingProgramTextureSceneFileEntry *left = (const DrawingProgramTextureSceneFileEntry *)lhs;
    const DrawingProgramTextureSceneFileEntry *right = (const DrawingProgramTextureSceneFileEntry *)rhs;
    int by_scene = strcmp(left->scene_id, right->scene_id);
    if (by_scene != 0) {
        return by_scene;
    }
    return strcmp(left->file_name, right->file_name);
}

static int texture_scene_browser_object_sort_compare(const void *lhs, const void *rhs) {
    const DrawingProgramTextureSceneObjectEntry *left = (const DrawingProgramTextureSceneObjectEntry *)lhs;
    const DrawingProgramTextureSceneObjectEntry *right = (const DrawingProgramTextureSceneObjectEntry *)rhs;
    return strcmp(left->object_id, right->object_id);
}

static int texture_scene_browser_parse_scene_root(json_object *root,
                                                  json_object **out_objects,
                                                  const char **out_scene_id) {
    json_object *objects = 0;
    if (!root || !json_object_is_type(root, json_type_object) ||
        !texture_scene_browser_get_required_string(root, "scene_id", out_scene_id) ||
        !json_object_object_get_ex(root, "objects", &objects) ||
        !objects ||
        !json_object_is_type(objects, json_type_array)) {
        return 0;
    }
    *out_objects = objects;
    return 1;
}

static int texture_scene_browser_supported_object_kind(const char *object_type, CoreSceneObjectKind *out_kind) {
    if (!object_type || !out_kind) {
        return 0;
    }
    if (core_scene_object_kind_parse(object_type, out_kind).code != CORE_OK) {
        return 0;
    }
    return (*out_kind == CORE_SCENE_OBJECT_KIND_PLANE_PRIMITIVE ||
            *out_kind == CORE_SCENE_OBJECT_KIND_RECT_PRISM_PRIMITIVE)
               ? 1
               : 0;
}

static int texture_scene_browser_fill_object_entry(json_object *object_json,
                                                   DrawingProgramTextureSceneObjectEntry *entry) {
    json_object *primitive = 0;
    const char *object_id = 0;
    const char *object_type = 0;
    CoreSceneObjectKind kind = CORE_SCENE_OBJECT_KIND_UNKNOWN;
    double width = 0.0;
    double height = 0.0;
    double depth = 0.0;
    int written = 0;
    if (!object_json ||
        !entry ||
        !texture_scene_browser_get_required_string(object_json, "object_id", &object_id) ||
        !texture_scene_browser_get_required_string(object_json, "object_type", &object_type) ||
        !texture_scene_browser_supported_object_kind(object_type, &kind) ||
        !json_object_object_get_ex(object_json, "primitive", &primitive) ||
        !primitive ||
        !json_object_is_type(primitive, json_type_object)) {
        return 0;
    }
    memset(entry, 0, sizeof(*entry));
    if (drawing_program_texture_scene_file_guard_copy_text(entry->object_id,
                                                           sizeof(entry->object_id),
                                                           object_id,
                                                           0,
                                                           "object_id")
                .code != CORE_OK ||
        drawing_program_texture_scene_file_guard_copy_text(entry->object_type,
                                                           sizeof(entry->object_type),
                                                           object_type,
                                                           0,
                                                           "object_type")
                .code != CORE_OK) {
        return 0;
    }
    if (kind == CORE_SCENE_OBJECT_KIND_PLANE_PRIMITIVE &&
        texture_scene_browser_get_required_number(primitive, "width", &width) &&
        texture_scene_browser_get_required_number(primitive, "height", &height)) {
        written = snprintf(entry->summary,
                           sizeof(entry->summary),
                           "%s plane %.2fx%.2f",
                           entry->object_id,
                           width,
                           height);
        return (written >= 0 && (size_t)written < sizeof(entry->summary)) ? 1 : 0;
    }
    if (kind == CORE_SCENE_OBJECT_KIND_RECT_PRISM_PRIMITIVE &&
        texture_scene_browser_get_required_number(primitive, "width", &width) &&
        texture_scene_browser_get_required_number(primitive, "height", &height) &&
        texture_scene_browser_get_required_number(primitive, "depth", &depth)) {
        written = snprintf(entry->summary,
                           sizeof(entry->summary),
                           "%s prism %.2fx%.2fx%.2f",
                           entry->object_id,
                           width,
                           height,
                           depth);
        return (written >= 0 && (size_t)written < sizeof(entry->summary)) ? 1 : 0;
    }
    return 0;
}

CoreResult drawing_program_texture_scene_browser_list_scene_files(
    const char *scene_root_path,
    DrawingProgramTextureSceneFileEntry *out_entries,
    uint32_t entry_capacity,
    uint32_t *out_entry_count) {
    DIR *dir = 0;
    struct dirent *dir_entry = 0;
    uint32_t count = 0u;
    if (!scene_root_path || scene_root_path[0] == '\0' || !out_entry_count) {
        return texture_scene_browser_invalid("invalid scene browser root request");
    }
    *out_entry_count = 0u;
    if (!out_entries || entry_capacity == 0u) {
        return texture_scene_browser_invalid("invalid scene browser output buffer");
    }
    dir = opendir(scene_root_path);
    if (!dir) {
        return texture_scene_browser_context_error(
            CORE_ERR_IO, scene_root_path, "scene_root", "failed to open scene authored root");
    }
    while ((dir_entry = readdir(dir)) != 0) {
        json_object *root = 0;
        json_object *objects = 0;
        const char *scene_id = 0;
        char scene_path[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_PATH_CAPACITY];
        DrawingProgramTextureSceneFileEntry *entry = 0;
        if (dir_entry->d_name[0] == '.' || !texture_scene_browser_has_json_extension(dir_entry->d_name)) {
            continue;
        }
        if (count >= entry_capacity) {
            break;
        }
        if (texture_scene_browser_join_path(scene_path,
                                            sizeof(scene_path),
                                            scene_root_path,
                                            dir_entry->d_name)
                .code != CORE_OK ||
            drawing_program_texture_scene_file_guard_check_json_file(scene_path).code != CORE_OK) {
            continue;
        }
        root = json_object_from_file(scene_path);
        if (!root) {
            continue;
        }
        if (!texture_scene_browser_parse_scene_root(root, &objects, &scene_id)) {
            json_object_put(root);
            continue;
        }
        if (drawing_program_texture_scene_file_guard_check_object_count(
                json_object_array_length(objects), scene_path, "objects")
                .code != CORE_OK) {
            json_object_put(root);
            continue;
        }
        entry = &out_entries[count];
        memset(entry, 0, sizeof(*entry));
        if (drawing_program_texture_scene_file_guard_copy_text(entry->scene_id,
                                                               sizeof(entry->scene_id),
                                                               scene_id,
                                                               scene_path,
                                                               "scene_id")
                    .code != CORE_OK ||
            drawing_program_texture_scene_file_guard_copy_text(entry->file_name,
                                                               sizeof(entry->file_name),
                                                               dir_entry->d_name,
                                                               scene_path,
                                                               "file_name")
                    .code != CORE_OK ||
            drawing_program_texture_scene_file_guard_copy_text(entry->scene_path,
                                                               sizeof(entry->scene_path),
                                                               scene_path,
                                                               scene_path,
                                                               "scene_path")
                    .code != CORE_OK) {
            json_object_put(root);
            continue;
        }
        json_object_put(root);
        count += 1u;
    }
    closedir(dir);
    qsort(out_entries, count, sizeof(out_entries[0]), texture_scene_browser_scene_sort_compare);
    *out_entry_count = count;
    return core_result_ok();
}

CoreResult drawing_program_texture_scene_browser_list_supported_objects(
    const char *scene_json_path,
    DrawingProgramTextureSceneObjectEntry *out_entries,
    uint32_t entry_capacity,
    uint32_t *out_entry_count,
    char *out_scene_id,
    size_t out_scene_id_capacity) {
    json_object *root = 0;
    json_object *objects = 0;
    const char *scene_id = 0;
    uint32_t count = 0u;
    size_t object_count = 0u;
    size_t i;
    if (!scene_json_path || scene_json_path[0] == '\0' || !out_entries || entry_capacity == 0u || !out_entry_count) {
        return texture_scene_browser_invalid("invalid scene object browser request");
    }
    *out_entry_count = 0u;
    if (out_scene_id && out_scene_id_capacity > 0u) {
        out_scene_id[0] = '\0';
    }
    {
        CoreResult guard_result = drawing_program_texture_scene_file_guard_check_json_file(scene_json_path);
        if (guard_result.code != CORE_OK) {
            return texture_scene_browser_context_error(
                guard_result.code, scene_json_path, "scene_file", guard_result.message);
        }
    }
    root = json_object_from_file(scene_json_path);
    if (!root) {
        return texture_scene_browser_context_error(
            CORE_ERR_FORMAT, scene_json_path, "scene", "failed to parse authored scene JSON");
    }
    if (!texture_scene_browser_parse_scene_root(root, &objects, &scene_id)) {
        json_object_put(root);
        return texture_scene_browser_context_error(
            CORE_ERR_FORMAT, scene_json_path, "scene_id|objects", "scene JSON is missing scene root fields");
    }
    {
        CoreResult guard_result =
            drawing_program_texture_scene_file_guard_check_object_count(json_object_array_length(objects),
                                                                        scene_json_path,
                                                                        "objects");
        if (guard_result.code != CORE_OK) {
            json_object_put(root);
            return texture_scene_browser_context_error(
                guard_result.code, scene_json_path, "objects", guard_result.message);
        }
    }
    if (out_scene_id && out_scene_id_capacity > 0u) {
        CoreResult copy_result = drawing_program_texture_scene_file_guard_copy_text(out_scene_id,
                                                                                   out_scene_id_capacity,
                                                                                   scene_id,
                                                                                   scene_json_path,
                                                                                   "scene_id");
        if (copy_result.code != CORE_OK) {
            json_object_put(root);
            return texture_scene_browser_context_error(
                copy_result.code, scene_json_path, "scene_id", copy_result.message);
        }
    }
    object_count = json_object_array_length(objects);
    for (i = 0u; i < object_count && count < entry_capacity; ++i) {
        json_object *object_json = json_object_array_get_idx(objects, i);
        if (texture_scene_browser_fill_object_entry(object_json, &out_entries[count])) {
            count += 1u;
        }
    }
    json_object_put(root);
    qsort(out_entries, count, sizeof(out_entries[0]), texture_scene_browser_object_sort_compare);
    *out_entry_count = count;
    return core_result_ok();
}
