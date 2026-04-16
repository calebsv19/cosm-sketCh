#ifndef DRAWING_PROGRAM_VISUAL_PANEL_RENDER_COMMON_H
#define DRAWING_PROGRAM_VISUAL_PANEL_RENDER_COMMON_H

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_visual_panel_render.h"

#ifdef __cplusplus
extern "C" {
#endif

int drawing_program_visual_panel_ui_hovered(const VisualPanelUiState *ui,
                                            SDL_Rect rect,
                                            const DrawingProgramVisualPanelRenderHooks *hooks);
void drawing_program_visual_panel_draw_tab_button(SDL_Renderer *renderer,
                                                  SDL_Rect clip_rect,
                                                  SDL_Rect rect,
                                                  const char *label,
                                                  SDL_Color fill,
                                                  SDL_Color fill_hover,
                                                  SDL_Color fill_active,
                                                  SDL_Color border,
                                                  SDL_Color text,
                                                  int text_scale,
                                                  int selected,
                                                  int hovered,
                                                  const DrawingProgramVisualPanelRenderHooks *hooks);
const char *drawing_program_visual_shape_target_mode_name(uint8_t mode);
const char *drawing_program_visual_select_mode_name(uint8_t mode);

#ifdef __cplusplus
}
#endif

#endif
