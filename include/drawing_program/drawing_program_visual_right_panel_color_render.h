#ifndef DRAWING_PROGRAM_VISUAL_RIGHT_PANEL_COLOR_RENDER_H
#define DRAWING_PROGRAM_VISUAL_RIGHT_PANEL_COLOR_RENDER_H

#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_layout_color.h"
#include "drawing_program/drawing_program_visual_panel_render.h"
#include "drawing_program/drawing_program_visual_theme.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawing_program_visual_render_right_panel_color_tab(SDL_Renderer *renderer,
                                                         SDL_Rect rect,
                                                         int y,
                                                         const DrawingProgramAppContext *ctx,
                                                         const VisualPanelUiState *ui,
                                                         VisualPaneLayoutMetrics m,
                                                         VisualThemePalette p,
                                                         const DrawingProgramVisualPanelRenderHooks *hooks);

#ifdef __cplusplus
}
#endif

#endif
