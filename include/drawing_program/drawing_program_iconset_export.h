#ifndef DRAWING_PROGRAM_ICONSET_EXPORT_H
#define DRAWING_PROGRAM_ICONSET_EXPORT_H

#include <stddef.h>

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

CoreResult drawing_program_iconset_export_default_path(const struct DrawingProgramAppContext *ctx,
                                                       char *out_path,
                                                       size_t out_cap);
CoreResult drawing_program_iconset_export_current_document(const struct DrawingProgramAppContext *ctx,
                                                           const char *iconset_dir_path);

#ifdef __cplusplus
}
#endif

#endif
