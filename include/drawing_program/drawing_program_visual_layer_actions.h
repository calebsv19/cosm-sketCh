#ifndef DRAWING_PROGRAM_VISUAL_LAYER_ACTIONS_H
#define DRAWING_PROGRAM_VISUAL_LAYER_ACTIONS_H

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_visual_layer_roles.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawing_program_visual_apply_workflow_control_if_valid(DrawingProgramAppContext *ctx,
                                                            DrawingProgramWorkflowControl control);
void drawing_program_visual_apply_layer_rename_auto(DrawingProgramAppContext *ctx);
void drawing_program_visual_apply_layer_duplicate_active(DrawingProgramAppContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
