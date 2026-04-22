#ifndef DRAWING_PROGRAM_APP_VISUAL_RUNTIME_SUPPORT_H
#define DRAWING_PROGRAM_APP_VISUAL_RUNTIME_SUPPORT_H

#include <SDL2/SDL.h>

#include "core_theme.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_visual_input_handlers.h"
#include "drawing_program/drawing_program_visual_state.h"

const DrawingProgramVisualInputHandlersHooks *drawing_program_visual_input_handlers_hooks(void);

int drawing_program_visual_draw_debug_frame(
    SDL_Window *window,
    SDL_Renderer *renderer,
    const DrawingProgramAppContext *ctx,
    const CoreThemePreset *theme,
    const VisualPanelUiState *ui,
    const VisualSelectionState *selection,
    const VisualCanvasInteractionState *interaction);

#endif
