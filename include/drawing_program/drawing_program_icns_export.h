#ifndef DRAWING_PROGRAM_ICNS_EXPORT_H
#define DRAWING_PROGRAM_ICNS_EXPORT_H

#include <stddef.h>

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

CoreResult drawing_program_icns_export_default_path(const struct DrawingProgramAppContext *ctx,
                                                    char *out_path,
                                                    size_t out_cap);
CoreResult drawing_program_icns_export_current_document(const struct DrawingProgramAppContext *ctx,
                                                        const char *icns_path);

#ifdef __cplusplus
}
#endif

#endif
