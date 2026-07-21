#ifndef DRAWING_PROGRAM_VISUAL_INDEXED_CANVAS_H
#define DRAWING_PROGRAM_VISUAL_INDEXED_CANVAS_H

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_visual_state.h"

void drawing_program_visual_draw_indexed_canvas(
    SDL_Renderer *renderer,
    SDL_Rect pane_rect,
    const DrawingProgramAppContext *ctx,
    const VisualCanvasSheetMetrics *metrics,
    int (*draw_bitmap_text)(SDL_Renderer *, SDL_Rect, int, int, const char *, SDL_Color, int));

#endif
