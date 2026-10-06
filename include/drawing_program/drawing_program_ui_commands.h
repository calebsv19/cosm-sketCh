#ifndef DRAWING_PROGRAM_UI_COMMANDS_H
#define DRAWING_PROGRAM_UI_COMMANDS_H
#include "drawing_program_ui_controls.h"
#include "drawing_program_visual_input_handlers.h"
/* Consumes recognized content commands, including rejected/stale targets.
 * Only enabled keys from the collected shared surface may execute. */
int drawing_program_ui_command_dispatch(
    DrawingProgramAppContext *app, KitUiSurfaceKey key,
    DrawingProgramSelectionState *selection, VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks);
#endif
