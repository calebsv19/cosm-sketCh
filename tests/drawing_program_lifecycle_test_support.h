#ifndef DRAWING_PROGRAM_LIFECYCLE_TEST_SUPPORT_H
#define DRAWING_PROGRAM_LIFECYCLE_TEST_SUPPORT_H

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_visual_input_handlers.h"

extern uint32_t g_test_apply_workflow_calls;
extern DrawingProgramWorkflowControl g_test_last_workflow_control;
extern uint32_t g_test_object_nudge_calls;
extern uint32_t g_test_selection_nudge_calls;

int expect_ok(CoreResult result, const char *label);
int expect_overlay_ok(DrawingProgramOverlayAdapterResult result, const char *label);
int expect_overlay_error_code(DrawingProgramOverlayAdapterResult result,
                              DrawingProgramOverlayAdapterErrorCode code,
                              const char *label);

void lifecycle_test_reset_input_handler_counters(void);
void lifecycle_test_apply_workflow_control(DrawingProgramAppContext *ctx,
                                           DrawingProgramWorkflowControl control);
CoreResult lifecycle_test_nudge_object_move(DrawingProgramAppContext *ctx,
                                            VisualCanvasInteractionState *interaction,
                                            int32_t dx,
                                            int32_t dy);
CoreResult lifecycle_test_nudge_selection_move(DrawingProgramAppContext *ctx,
                                               VisualCanvasInteractionState *interaction,
                                               DrawingProgramSelectionState *selection,
                                               int32_t dx,
                                               int32_t dy);
void lifecycle_test_cancel_canvas_draw_and_shape(VisualCanvasInteractionState *interaction);
void lifecycle_test_cancel_selection_transient(DrawingProgramSelectionState *selection);
void lifecycle_test_cancel_all_transient_interactions(DrawingProgramAppContext *ctx,
                                                      VisualCanvasInteractionState *interaction,
                                                      DrawingProgramSelectionState *selection,
                                                      int clear_pan_state);

#endif
