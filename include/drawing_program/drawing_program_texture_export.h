#ifndef DRAWING_PROGRAM_TEXTURE_EXPORT_H
#define DRAWING_PROGRAM_TEXTURE_EXPORT_H

#include <stddef.h>

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

CoreResult drawing_program_texture_export_default_directory(const struct DrawingProgramAppContext *ctx,
                                                            char *out_path,
                                                            size_t out_cap);
CoreResult drawing_program_texture_export_current_project(struct DrawingProgramAppContext *ctx,
                                                          const char *dir_path);

#ifdef __cplusplus
}
#endif

#endif
