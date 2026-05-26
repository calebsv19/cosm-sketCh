#ifndef DRAWING_PROGRAM_PROJECT_STATE_INTERNAL_H
#define DRAWING_PROGRAM_PROJECT_STATE_INTERNAL_H

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_project_state.h"

CoreResult drawing_program_project_invalid(const char *message);
CoreResult drawing_program_project_slot_path_missing(const char *message);
void drawing_program_project_trim_dir_path(char *path);
int drawing_program_project_path_has_pack_extension(const char *path);
void drawing_program_project_sanitize_component(const char *input, char *out, size_t out_cap);
void drawing_program_project_scene_stem_from_path(const char *path, char *out_stem, size_t out_cap);
CoreResult drawing_program_project_mkdirs_if_needed(const char *dir_path);
CoreResult drawing_program_project_ensure_parent_dir(const char *file_path);
CoreResult drawing_program_project_copy_parent_dir(const char *file_path, char *out_dir, size_t out_cap);
CoreResult drawing_program_project_normalize_path(const char *path,
                                                  uint8_t append_pack_extension,
                                                  char *out_path,
                                                  size_t out_cap);
void drawing_program_project_state_copy_current_path(struct DrawingProgramAppContext *ctx, const char *path);

#endif
