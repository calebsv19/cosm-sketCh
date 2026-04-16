#ifndef DRAWING_PROGRAM_VISUAL_INPUT_PANEL_CLICKS_H
#define DRAWING_PROGRAM_VISUAL_INPUT_PANEL_CLICKS_H

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_visual_input_handlers.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawing_program_visual_input_handle_left_panel_click_payload(DrawingProgramAppContext *ctx,
                                                                  SDL_Rect rect,
                                                                  int x,
                                                                  int y,
                                                                  DrawingProgramSelectionState *selection,
                                                                  VisualPanelUiState *ui,
                                                                  const DrawingProgramVisualInputHandlersHooks *hooks);

void drawing_program_visual_input_handle_right_panel_click_payload(DrawingProgramAppContext *ctx,
                                                                   SDL_Rect rect,
                                                                   int x,
                                                                   int y,
                                                                   DrawingProgramSelectionState *selection,
                                                                   VisualPanelUiState *ui,
                                                                   const DrawingProgramVisualInputHandlersHooks *hooks);

#ifdef __cplusplus
}
#endif

#endif
