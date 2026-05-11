#ifndef DRAWING_PROGRAM_TEXTURE_CANVAS_RESIZE_H
#define DRAWING_PROGRAM_TEXTURE_CANVAS_RESIZE_H

#include <stdint.h>

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_visual_state.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

int drawing_program_texture_canvas_resize_handle_rect_for_surface(
    const struct DrawingProgramAppContext *ctx,
    SDL_Rect pane_rect,
    uint32_t surface_index,
    SDL_Rect *out_rect);
int drawing_program_texture_canvas_resize_hit_test_handle(
    const struct DrawingProgramAppContext *ctx,
    SDL_Rect pane_rect,
    int sx,
    int sy,
    uint32_t *out_surface_index);
int drawing_program_texture_canvas_resize_begin(
    struct DrawingProgramAppContext *ctx,
    SDL_Rect pane_rect,
    VisualCanvasInteractionState *interaction,
    uint32_t surface_index,
    int sx,
    int sy);
int drawing_program_texture_canvas_resize_update(
    struct DrawingProgramAppContext *ctx,
    VisualCanvasInteractionState *interaction,
    int sx,
    int sy);
void drawing_program_texture_canvas_resize_end(VisualCanvasInteractionState *interaction);

#ifdef __cplusplus
}
#endif

#endif
