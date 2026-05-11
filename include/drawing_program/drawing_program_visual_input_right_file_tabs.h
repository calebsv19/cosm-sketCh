#ifndef DRAWING_PROGRAM_VISUAL_INPUT_RIGHT_FILE_TABS_H
#define DRAWING_PROGRAM_VISUAL_INPUT_RIGHT_FILE_TABS_H

#include "drawing_program/drawing_program_visual_input_handlers.h"

#ifdef __cplusplus
extern "C" {
#endif

int drawing_program_visual_input_handle_right_file_tab_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    DrawingProgramSelectionState *selection,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks);

int drawing_program_visual_input_handle_right_asset_tab_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    DrawingProgramSelectionState *selection,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks);

int drawing_program_visual_input_handle_right_export_tab_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks);

int drawing_program_visual_input_handle_right_file_tabs_wheel_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    int wheel_y,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks);

#ifdef __cplusplus
}
#endif

#endif
