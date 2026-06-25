#ifndef DRAWING_PROGRAM_APP_POST_LOAD_H
#define DRAWING_PROGRAM_APP_POST_LOAD_H

#include "drawing_program/drawing_program_app_main.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawing_program_app_rearm_after_document_swap(DrawingProgramAppContext *ctx);
void drawing_program_app_rearm_after_snapshot_load(DrawingProgramAppContext *ctx,
                                                   DrawingProgramSelectionState *selection,
                                                   int preserve_project_clean_state);

#ifdef __cplusplus
}
#endif

#endif
