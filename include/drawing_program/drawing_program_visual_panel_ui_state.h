#ifndef DRAWING_PROGRAM_VISUAL_PANEL_UI_STATE_H
#define DRAWING_PROGRAM_VISUAL_PANEL_UI_STATE_H

#include "drawing_program/drawing_program_visual_state.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawing_program_visual_panel_ui_disarm_canvas_delete(VisualPanelUiState *ui);
void drawing_program_visual_panel_ui_arm_canvas_delete(DrawingProgramAppContext *ctx, VisualPanelUiState *ui);
int drawing_program_visual_panel_ui_canvas_delete_is_confirmable(const DrawingProgramAppContext *ctx,
                                                                 const VisualPanelUiState *ui);
int drawing_program_visual_panel_ui_canvas_delete_is_waiting_for_min_confirm_delay(
    const DrawingProgramAppContext *ctx,
    const VisualPanelUiState *ui);
void drawing_program_visual_panel_ui_disarm_reflection_center_pick(VisualPanelUiState *ui);
void drawing_program_visual_panel_ui_arm_reflection_center_pick(VisualPanelUiState *ui);
void drawing_program_visual_panel_ui_toggle_reflection_center_pick(VisualPanelUiState *ui);
void drawing_program_visual_panel_ui_disarm_right_canvas_transients(VisualPanelUiState *ui);

#ifdef __cplusplus
}
#endif

#endif
