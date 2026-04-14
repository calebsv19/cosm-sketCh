#ifndef DRAWING_PROGRAM_VISUAL_FRAME_RENDER_H
#define DRAWING_PROGRAM_VISUAL_FRAME_RENDER_H

#include <SDL2/SDL.h>

#include "core_theme.h"
#include "drawing_program/drawing_program_visual_state.h"

typedef struct DrawingProgramVisualFrameRenderHooks {
    uint32_t (*module_type_for_pane)(const DrawingProgramAppContext *ctx, uint32_t pane_node_id);
    void (*draw_menu_bar_chrome)(SDL_Renderer *renderer,
                                 SDL_Rect rect,
                                 const DrawingProgramAppContext *ctx,
                                 const CoreThemePreset *theme);
    void (*draw_left_panel_chrome)(SDL_Renderer *renderer,
                                   SDL_Rect rect,
                                   const DrawingProgramAppContext *ctx,
                                   const CoreThemePreset *theme,
                                   const VisualPanelUiState *ui);
    void (*draw_right_panel_chrome)(SDL_Renderer *renderer,
                                    SDL_Rect rect,
                                    const DrawingProgramAppContext *ctx,
                                    const CoreThemePreset *theme,
                                    const VisualPanelUiState *ui,
                                    const VisualSelectionState *selection,
                                    const VisualCanvasInteractionState *interaction);
    void (*draw_canvas_world_view)(SDL_Renderer *renderer,
                                   SDL_Rect rect,
                                   const DrawingProgramAppContext *ctx,
                                   const CoreThemePreset *theme,
                                   const VisualSelectionState *selection,
                                   const VisualPanelUiState *ui,
                                   const VisualCanvasInteractionState *interaction);
    void (*draw_canvas_viewport_chrome)(SDL_Renderer *renderer,
                                        SDL_Rect rect,
                                        const DrawingProgramAppContext *ctx,
                                        const CoreThemePreset *theme);
} DrawingProgramVisualFrameRenderHooks;

int drawing_program_visual_draw_frame(SDL_Window *window,
                                      SDL_Renderer *renderer,
                                      const DrawingProgramAppContext *ctx,
                                      const CoreThemePreset *theme,
                                      const VisualPanelUiState *ui,
                                      const VisualSelectionState *selection,
                                      const VisualCanvasInteractionState *interaction,
                                      const DrawingProgramVisualFrameRenderHooks *hooks);

#endif
