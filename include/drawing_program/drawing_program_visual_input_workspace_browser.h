#ifndef DRAWING_PROGRAM_VISUAL_INPUT_WORKSPACE_BROWSER_H
#define DRAWING_PROGRAM_VISUAL_INPUT_WORKSPACE_BROWSER_H

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_visual_input_handlers.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Atlas-shell canvas clicks that browse, commit, fit, or manipulate workspace
 * sheets should resolve here before the true active-edit tool path runs. */
int drawing_program_visual_input_handle_workspace_canvas_click(
    int click_x,
    int click_y,
    SDL_Rect canvas_pane,
    DrawingProgramAppContext *app,
    VisualCanvasInteractionState *canvas_interaction,
    DrawingProgramSelectionState *selection_state,
    VisualPanelUiState *panel_ui,
    const DrawingProgramVisualInputHandlersHooks *hooks);

#ifdef __cplusplus
}
#endif

#endif
