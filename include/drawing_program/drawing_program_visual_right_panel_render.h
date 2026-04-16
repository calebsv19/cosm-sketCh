#ifndef DRAWING_PROGRAM_VISUAL_RIGHT_PANEL_RENDER_H
#define DRAWING_PROGRAM_VISUAL_RIGHT_PANEL_RENDER_H

#include "drawing_program/drawing_program_visual_panel_render.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawing_program_visual_render_right_panel_chrome(SDL_Renderer *renderer,
                                                      SDL_Rect rect,
                                                      const DrawingProgramAppContext *ctx,
                                                      const CoreThemePreset *theme,
                                                      const VisualPanelUiState *ui,
                                                      const VisualSelectionState *selection,
                                                      const VisualCanvasInteractionState *interaction,
                                                      const DrawingProgramVisualPanelRenderHooks *hooks);

#ifdef __cplusplus
}
#endif

#endif
