#ifndef DRAWING_PROGRAM_SESSION_PREFS_H
#define DRAWING_PROGRAM_SESSION_PREFS_H

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

CoreResult drawing_program_session_prefs_load(struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_session_prefs_save(const struct DrawingProgramAppContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
