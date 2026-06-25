#include "drawing_program_texture_scene_file_guard.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static CoreResult texture_scene_file_guard_error(CoreError code,
                                                 const char *scene_json_path,
                                                 const char *field,
                                                 const char *detail) {
    static char message[512];
    (void)snprintf(message,
                   sizeof(message),
                   "texture scene file guard failure path=%s field=%s detail=%s",
                   scene_json_path ? scene_json_path : "(null)",
                   field ? field : "(unknown)",
                   detail ? detail : "unknown");
    return (CoreResult){ code, message };
}

CoreResult drawing_program_texture_scene_file_guard_check_json_file(const char *scene_json_path) {
    struct stat st;
    if (!scene_json_path || scene_json_path[0] == '\0') {
        return texture_scene_file_guard_error(
            CORE_ERR_INVALID_ARG, scene_json_path, "scene_file", "scene JSON path is empty");
    }
    if (stat(scene_json_path, &st) != 0) {
        return texture_scene_file_guard_error(
            CORE_ERR_IO, scene_json_path, "scene_file", "failed to inspect scene JSON file");
    }
    if (!S_ISREG(st.st_mode)) {
        return texture_scene_file_guard_error(
            CORE_ERR_IO, scene_json_path, "scene_file", "scene JSON path is not a regular file");
    }
    if (st.st_size < 0 || (unsigned long long)st.st_size > DRAWING_PROGRAM_TEXTURE_SCENE_FILE_MAX_BYTES) {
        return texture_scene_file_guard_error(
            CORE_ERR_INVALID_ARG, scene_json_path, "scene_file", "scene JSON exceeds maximum supported size");
    }
    return core_result_ok();
}

CoreResult drawing_program_texture_scene_file_guard_check_object_count(size_t object_count,
                                                                       const char *scene_json_path,
                                                                       const char *field) {
    if (object_count > DRAWING_PROGRAM_TEXTURE_SCENE_FILE_MAX_OBJECTS) {
        return texture_scene_file_guard_error(
            CORE_ERR_INVALID_ARG, scene_json_path, field ? field : "objects", "scene object count exceeds maximum");
    }
    return core_result_ok();
}

int drawing_program_texture_scene_file_guard_text_is_bounded(const char *value, size_t out_cap) {
    size_t i;
    if (!value || value[0] == '\0' || out_cap == 0u) {
        return 0;
    }
    for (i = 0u; value[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)value[i];
        if (i + 1u >= out_cap) {
            return 0;
        }
        if (c < (unsigned char)' ' || c == (unsigned char)0x7f) {
            return 0;
        }
    }
    return 1;
}

CoreResult drawing_program_texture_scene_file_guard_copy_text(char *out,
                                                              size_t out_cap,
                                                              const char *value,
                                                              const char *scene_json_path,
                                                              const char *field) {
    if (!out || out_cap == 0u || !drawing_program_texture_scene_file_guard_text_is_bounded(value, out_cap)) {
        return texture_scene_file_guard_error(
            CORE_ERR_INVALID_ARG, scene_json_path, field, "scene text field is empty, too long, or contains controls");
    }
    (void)snprintf(out, out_cap, "%s", value);
    return core_result_ok();
}
