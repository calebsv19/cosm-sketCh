#ifndef DRAWING_PROGRAM_VISUAL_TOOL_OPTIONS_H
#define DRAWING_PROGRAM_VISUAL_TOOL_OPTIONS_H

#include <stddef.h>
#include <stdint.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_visual_state.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t drawing_program_visual_tool_count(void);
DrawingProgramToolKind drawing_program_visual_tool_at(uint32_t index);
DrawingProgramWorkflowControl drawing_program_visual_workflow_control_for_tool(DrawingProgramToolKind tool);
uint32_t drawing_program_visual_tool_option_count(const DrawingProgramAppContext *ctx, DrawingProgramToolKind tool);
uint32_t drawing_program_visual_tool_option_kind_for_index_raw(const DrawingProgramAppContext *ctx,
                                                               DrawingProgramToolKind tool,
                                                               uint32_t index);
int drawing_program_visual_tool_option_is_action_button_raw(uint32_t option_kind_raw);
int drawing_program_visual_tool_option_is_select_delete_raw(uint32_t option_kind_raw);
void drawing_program_visual_tool_option_adjust_raw(DrawingProgramAppContext *ctx, uint32_t option_kind_raw, int delta);
const char *drawing_program_visual_tool_option_label_raw(uint32_t option_kind_raw);
void drawing_program_visual_tool_option_value_text_raw(const DrawingProgramAppContext *ctx,
                                                       uint32_t option_kind_raw,
                                                       char *out_text,
                                                       size_t out_cap);
uint8_t drawing_program_visual_clamp_left_slot(uint8_t slot);
uint8_t drawing_program_visual_clamp_right_slot(uint8_t slot);
void drawing_program_visual_sync_panel_ui_from_app(const DrawingProgramAppContext *ctx, VisualPanelUiState *ui);

#ifdef __cplusplus
}
#endif

#endif
