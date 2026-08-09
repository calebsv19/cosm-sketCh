#ifndef DRAWING_PROGRAM_VISUAL_INPUT_INDEXED_ASSET_H
#define DRAWING_PROGRAM_VISUAL_INPUT_INDEXED_ASSET_H

#include <SDL.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_visual_input_handlers.h"
#include "drawing_program/drawing_program_visual_panel_ui_state.h"

int drawing_program_visual_input_handle_indexed_asset(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks);

#endif
