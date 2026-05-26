#ifndef DRAWING_PROGRAM_VISUAL_INPUT_PANEL_WORKSPACE_MODES_H
#define DRAWING_PROGRAM_VISUAL_INPUT_PANEL_WORKSPACE_MODES_H

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_visual_input_handlers.h"
#include "drawing_program/drawing_program_visual_layout.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawing_program_visual_input_disarm_right_canvas_workspace_modes(VisualPanelUiState *ui);

int drawing_program_visual_input_handle_right_canvas_workspace_mode_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    VisualPaneLayoutMetrics metrics,
    int x,
    int y,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks);

#ifdef __cplusplus
}
#endif

#endif
