#ifndef DRAWING_PROGRAM_VISUAL_INPUT_SUPPORT_H
#define DRAWING_PROGRAM_VISUAL_INPUT_SUPPORT_H

#include "drawing_program/drawing_program_visual_input_handlers.h"

#ifdef __cplusplus
extern "C" {
#endif

CoreResult path_draft_commit(DrawingProgramAppContext *ctx,
                             VisualCanvasInteractionState *interaction,
                             uint8_t closed);
void path_draft_reset(VisualCanvasInteractionState *interaction);
int path_draft_pop_point(VisualCanvasInteractionState *interaction);

#ifdef __cplusplus
}
#endif

#endif
