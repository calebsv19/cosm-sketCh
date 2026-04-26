#ifndef DRAWING_PROGRAM_APP_POST_LOAD_H
#define DRAWING_PROGRAM_APP_POST_LOAD_H

#include "drawing_program/drawing_program_app_main.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawing_program_app_rearm_after_document_swap(DrawingProgramAppContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
