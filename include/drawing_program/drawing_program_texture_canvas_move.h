#ifndef DRAWING_PROGRAM_TEXTURE_CANVAS_MOVE_H
#define DRAWING_PROGRAM_TEXTURE_CANVAS_MOVE_H

#include <stdint.h>

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_visual_state.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

int drawing_program_texture_canvas_move_begin(
    const struct DrawingProgramAppContext *ctx,
    SDL_Rect pane_rect,
    VisualCanvasInteractionState *interaction,
    uint32_t surface_index,
    int sx,
    int sy);
int drawing_program_texture_canvas_move_update(
    struct DrawingProgramAppContext *ctx,
    VisualCanvasInteractionState *interaction,
    int sx,
    int sy);
void drawing_program_texture_canvas_move_end(VisualCanvasInteractionState *interaction);

#ifdef __cplusplus
}
#endif

#endif
