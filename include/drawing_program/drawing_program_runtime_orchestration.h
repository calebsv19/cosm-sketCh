#ifndef DRAWING_PROGRAM_RUNTIME_ORCHESTRATION_H
#define DRAWING_PROGRAM_RUNTIME_ORCHESTRATION_H

#include "drawing_program/drawing_program_app_main.h"

#ifdef __cplusplus
extern "C" {
#endif

CoreResult drawing_program_runtime_orchestration_plan_frame(
    const DrawingProgramInputEventRaw *raw,
    DrawingProgramInputEventNormalized *out_normalized,
    DrawingProgramInputRouteResult *out_route,
    DrawingProgramInputInvalidationResult *out_invalidation);

typedef enum DrawingProgramWorkflowControl {
    DRAWING_PROGRAM_WORKFLOW_CONTROL_NONE = 0,
    DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_BRUSH,
    DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_ERASER,
    DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_FILL,
    DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_LINE,
    DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_RECT,
    DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_CIRCLE,
    DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_SELECT,
    DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_MOVE,
    DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_PICKER,
    DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY,
    DRAWING_PROGRAM_WORKFLOW_CONTROL_UNDO,
    DRAWING_PROGRAM_WORKFLOW_CONTROL_REDO,
    DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_HISTORY,
    DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE
} DrawingProgramWorkflowControl;

CoreResult drawing_program_runtime_orchestration_apply_workflow_control(
    DrawingProgramAppContext *ctx,
    DrawingProgramWorkflowControl control);

CoreResult drawing_program_runtime_orchestration_dispatch_immediate(
    DrawingProgramAppContext *ctx,
    const DrawingProgramInputEventNormalized *normalized);

CoreResult drawing_program_runtime_orchestration_submit_deferred(
    DrawingProgramAppContext *ctx,
    const DrawingProgramInputEventNormalized *normalized);

#ifdef __cplusplus
}
#endif

#endif
