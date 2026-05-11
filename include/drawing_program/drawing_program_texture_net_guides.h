#ifndef DRAWING_PROGRAM_TEXTURE_NET_GUIDES_H
#define DRAWING_PROGRAM_TEXTURE_NET_GUIDES_H

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_texture_project.h"
#include "drawing_program/drawing_program_visual_state.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *drawing_program_ui_canvas_guide_mode_name(uint8_t guide_mode);

void drawing_program_visual_draw_texture_net_guides(SDL_Renderer *renderer,
                                                    SDL_Rect pane_rect,
                                                    const DrawingProgramTextureSurface *surface,
                                                    const VisualCanvasSheetMetrics *metrics,
                                                    uint8_t guide_mode);

#ifdef __cplusplus
}
#endif

#endif
