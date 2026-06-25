#ifndef DRAWING_PROGRAM_TEXTURE_SCENE_FILE_GUARD_H
#define DRAWING_PROGRAM_TEXTURE_SCENE_FILE_GUARD_H

#include <stddef.h>

#include "core_base.h"

enum {
    DRAWING_PROGRAM_TEXTURE_SCENE_FILE_MAX_BYTES = 1024u * 1024u,
    DRAWING_PROGRAM_TEXTURE_SCENE_FILE_MAX_OBJECTS = 1024u
};

CoreResult drawing_program_texture_scene_file_guard_check_json_file(const char *scene_json_path);
CoreResult drawing_program_texture_scene_file_guard_check_object_count(size_t object_count,
                                                                       const char *scene_json_path,
                                                                       const char *field);
CoreResult drawing_program_texture_scene_file_guard_copy_text(char *out,
                                                              size_t out_cap,
                                                              const char *value,
                                                              const char *scene_json_path,
                                                              const char *field);
int drawing_program_texture_scene_file_guard_text_is_bounded(const char *value, size_t out_cap);

#endif
