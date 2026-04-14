#ifndef DRAWING_PROGRAM_VISUAL_CANVAS_COORDS_H
#define DRAWING_PROGRAM_VISUAL_CANVAS_COORDS_H

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_visual_state.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawing_program_visual_compute_canvas_sheet_metrics(const DrawingProgramAppContext *ctx,
                                                         SDL_Rect pane_rect,
                                                         VisualCanvasSheetMetrics *out_metrics);
int drawing_program_visual_screen_to_canvas_sample(const DrawingProgramAppContext *ctx,
                                                   SDL_Rect pane_rect,
                                                   int sx,
                                                   int sy,
                                                   uint32_t *out_sample_x,
                                                   uint32_t *out_sample_y);
int drawing_program_visual_screen_to_canvas_sample_clamped(const DrawingProgramAppContext *ctx,
                                                           SDL_Rect pane_rect,
                                                           int sx,
                                                           int sy,
                                                           uint32_t *out_sample_x,
                                                           uint32_t *out_sample_y);

#ifdef __cplusplus
}
#endif

#endif
