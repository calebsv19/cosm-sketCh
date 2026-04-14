#ifndef DRAWING_PROGRAM_VISUAL_PANE_BINDINGS_H
#define DRAWING_PROGRAM_VISUAL_PANE_BINDINGS_H

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_app_main.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t drawing_program_visual_module_type_for_pane(const DrawingProgramAppContext *ctx, uint32_t pane_node_id);
int drawing_program_visual_set_module_type_for_pane(DrawingProgramAppContext *ctx,
                                                     uint32_t pane_node_id,
                                                     uint32_t module_type_id);
int drawing_program_visual_pane_rect_for_module_type(const DrawingProgramAppContext *ctx,
                                                      uint32_t module_type_id,
                                                      SDL_Rect *out_rect);
int drawing_program_visual_point_in_rect(SDL_Rect rect, int x, int y);

#ifdef __cplusplus
}
#endif

#endif
