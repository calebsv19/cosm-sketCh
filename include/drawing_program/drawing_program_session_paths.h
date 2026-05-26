#ifndef DRAWING_PROGRAM_SESSION_PATHS_H
#define DRAWING_PROGRAM_SESSION_PATHS_H

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

CoreResult drawing_program_session_paths_configure(struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_session_paths_ensure_parent_dir(const char *file_path);

#ifdef __cplusplus
}
#endif

#endif
