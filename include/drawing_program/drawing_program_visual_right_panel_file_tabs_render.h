#ifndef DRAWING_PROGRAM_VISUAL_RIGHT_PANEL_FILE_TABS_RENDER_H
#define DRAWING_PROGRAM_VISUAL_RIGHT_PANEL_FILE_TABS_RENDER_H

#include "drawing_program/drawing_program_visual_right_panel_render.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_theme.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawing_program_visual_render_right_file_tab(SDL_Renderer *renderer,
                                                  SDL_Rect rect,
                                                  const DrawingProgramAppContext *ctx,
                                                  const VisualPanelUiState *ui,
                                                  VisualPaneLayoutMetrics m,
                                                  VisualThemePalette p,
                                                  const DrawingProgramVisualPanelRenderHooks *hooks);

void drawing_program_visual_render_right_asset_tab(SDL_Renderer *renderer,
                                                   SDL_Rect rect,
                                                   const DrawingProgramAppContext *ctx,
                                                   const VisualPanelUiState *ui,
                                                   VisualPaneLayoutMetrics m,
                                                   VisualThemePalette p,
                                                   const DrawingProgramVisualPanelRenderHooks *hooks);

void drawing_program_visual_render_right_export_tab(SDL_Renderer *renderer,
                                                    SDL_Rect rect,
                                                    const DrawingProgramAppContext *ctx,
                                                    const VisualPanelUiState *ui,
                                                    VisualPaneLayoutMetrics m,
                                                    VisualThemePalette p,
                                                    const DrawingProgramVisualPanelRenderHooks *hooks);

#ifdef __cplusplus
}
#endif

#endif
