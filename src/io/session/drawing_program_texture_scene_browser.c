#include "drawing_program/drawing_program_texture_scene_browser.h"

#include <dirent.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_scene.h"

static CoreResult texture_scene_browser_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static int texture_scene_browser_has_json_extension(const char *name) {
    const char *dot = 0;
    if (!name || name[0] == '\0') {
        return 0;
    }
    dot = strrchr(name, '.');
    return (dot && strcmp(dot, ".json") == 0) ? 1 : 0;
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
    (void)snprintf(entry->object_id, sizeof(entry->object_id), "%s", object_id);
    (void)snprintf(entry->object_type, sizeof(entry->object_type), "%s", object_type);
    if (kind == CORE_SCENE_OBJECT_KIND_PLANE_PRIMITIVE &&
        texture_scene_browser_get_required_number(primitive, "width", &width) &&
        texture_scene_browser_get_required_number(primitive, "height", &height)) {
        (void)snprintf(entry->summary,
                       sizeof(entry->summary),
                       "%s plane %.2fx%.2f",
                       object_id,
                       width,
                       height);
        return 1;
    }
    if (kind == CORE_SCENE_OBJECT_KIND_RECT_PRISM_PRIMITIVE &&
        texture_scene_browser_get_required_number(primitive, "width", &width) &&
        texture_scene_browser_get_required_number(primitive, "height", &height) &&
        texture_scene_browser_get_required_number(primitive, "depth", &depth)) {
        (void)snprintf(entry->summary,
                       sizeof(entry->summary),
                       "%s prism %.2fx%.2fx%.2f",
                       object_id,
                       width,
                       height,
                       depth);
        return 1;
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
        return (CoreResult){ CORE_ERR_IO, "failed to open scene authored root" };
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
        (void)snprintf(scene_path, sizeof(scene_path), "%s/%s", scene_root_path, dir_entry->d_name);
        root = json_object_from_file(scene_path);
        if (!root) {
            continue;
        }
        if (!texture_scene_browser_parse_scene_root(root, &objects, &scene_id)) {
            json_object_put(root);
            continue;
        }
        entry = &out_entries[count];
        memset(entry, 0, sizeof(*entry));
        (void)snprintf(entry->scene_id, sizeof(entry->scene_id), "%s", scene_id);
        (void)snprintf(entry->file_name, sizeof(entry->file_name), "%s", dir_entry->d_name);
        (void)snprintf(entry->scene_path, sizeof(entry->scene_path), "%s", scene_path);
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
    root = json_object_from_file(scene_json_path);
    if (!root) {
        return (CoreResult){ CORE_ERR_FORMAT, "failed to parse authored scene JSON" };
    }
    if (!texture_scene_browser_parse_scene_root(root, &objects, &scene_id)) {
        json_object_put(root);
        return (CoreResult){ CORE_ERR_FORMAT, "scene JSON is missing scene root fields" };
    }
    if (out_scene_id && out_scene_id_capacity > 0u) {
        (void)snprintf(out_scene_id, out_scene_id_capacity, "%s", scene_id);
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
