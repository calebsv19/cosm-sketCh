#ifndef DRAWING_PROGRAM_VISUAL_INPUT_WORKSPACE_VIEW_H
#define DRAWING_PROGRAM_VISUAL_INPUT_WORKSPACE_VIEW_H

#include "drawing_program/drawing_program_runtime_orchestration.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Keep atlas-workspace view consequences owned in one place so panel/canvas
 * actions only own the mutation they trigger, not duplicate fit/reset policy. */
int drawing_program_visual_input_workspace_view_fit_surface(DrawingProgramAppContext *ctx,
                                                            uint32_t surface_index);
int drawing_program_visual_input_workspace_view_fit_all(DrawingProgramAppContext *ctx);
int drawing_program_visual_input_workspace_view_fit_all_or_reset(DrawingProgramAppContext *ctx);
int drawing_program_visual_input_workspace_view_show_canvas_fit_all(DrawingProgramAppContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
