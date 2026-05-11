#ifndef DRAWING_PROGRAM_TEXTURE_WORKSPACE_H
#define DRAWING_PROGRAM_TEXTURE_WORKSPACE_H

#include <stdint.h>

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_visual_state.h"

#ifdef __cplusplus
extern "C" {
#endif

int drawing_program_texture_workspace_surface_sheet_metrics(const DrawingProgramAppContext *ctx,
                                                            SDL_Rect pane_rect,
                                                            uint32_t surface_index,
                                                            VisualCanvasSheetMetrics *out_metrics);
int drawing_program_texture_workspace_active_sheet_metrics(const DrawingProgramAppContext *ctx,
                                                           SDL_Rect pane_rect,
                                                           VisualCanvasSheetMetrics *out_metrics);
int drawing_program_texture_workspace_hit_test_surface(const DrawingProgramAppContext *ctx,
                                                       SDL_Rect pane_rect,
                                                       int sx,
                                                       int sy,
                                                       uint32_t *out_surface_index);
int drawing_program_texture_workspace_screen_to_active_sample(const DrawingProgramAppContext *ctx,
                                                              SDL_Rect pane_rect,
                                                              int sx,
                                                              int sy,
                                                              uint32_t *out_sample_x,
                                                              uint32_t *out_sample_y);
int drawing_program_texture_workspace_screen_to_active_sample_clamped(const DrawingProgramAppContext *ctx,
                                                                      SDL_Rect pane_rect,
                                                                      int sx,
                                                                      int sy,
                                                                      uint32_t *out_sample_x,
                                                                      uint32_t *out_sample_y);
int drawing_program_texture_workspace_fit_all(DrawingProgramAppContext *ctx, SDL_Rect pane_rect);
int drawing_program_texture_workspace_fit_surface(DrawingProgramAppContext *ctx,
                                                  SDL_Rect pane_rect,
                                                  uint32_t surface_index);

#ifdef __cplusplus
}
#endif

#endif
