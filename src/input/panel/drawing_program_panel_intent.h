#ifndef DRAWING_PROGRAM_PANEL_INTENT_H
#define DRAWING_PROGRAM_PANEL_INTENT_H
#include "drawing_program/drawing_program_ui_controls.h"
#include "drawing_program/drawing_program_visual_input_handlers.h"
#include "drawing_program/drawing_program_visual_layout.h"
/* A command has meaning, never a fabricated pointer position. Spatial intents
 * serve continuous controls and retained pointer/domain-test entry points. */
typedef struct DrawingProgramPanelIntent {
    KitUiSurfaceKey key;
    int x, y;
    int semantic;
} DrawingProgramPanelIntent;
static inline int drawing_program_panel_intent_matches(
    DrawingProgramPanelIntent intent, uint32_t op, uint64_t identity,
    SDL_Rect rect, const DrawingProgramVisualInputHandlersHooks *hooks) {
    if (intent.semantic)
        return op && intent.key.domain == op && intent.key.value == identity;
    return hooks && hooks->point_in_rect &&
        hooks->point_in_rect(rect, intent.x, intent.y);
}
void drawing_program_visual_input_handle_left_panel_click_payload_intent(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    DrawingProgramPanelIntent intent,
    DrawingProgramSelectionState *selection,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks);
void drawing_program_visual_input_handle_right_panel_click_payload_intent(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    DrawingProgramPanelIntent intent,
    DrawingProgramSelectionState *selection,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks);
int drawing_program_visual_input_handle_right_canvas_workspace_mode_payload_intent(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    VisualPaneLayoutMetrics metrics,
    DrawingProgramPanelIntent intent,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks);
int drawing_program_visual_input_handle_right_color_panel_click_payload_intent(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    DrawingProgramPanelIntent intent,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks);
int drawing_program_visual_input_handle_indexed_asset_intent(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    DrawingProgramPanelIntent intent,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks);
int drawing_program_visual_input_handle_right_file_tab_payload_intent(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    DrawingProgramPanelIntent intent,
    DrawingProgramSelectionState *selection,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks);
int drawing_program_visual_input_handle_right_asset_tab_payload_intent(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    DrawingProgramPanelIntent intent,
    DrawingProgramSelectionState *selection,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks);
int drawing_program_visual_input_handle_right_export_tab_payload_intent(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    DrawingProgramPanelIntent intent,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks);
#endif
