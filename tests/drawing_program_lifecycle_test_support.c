#include "drawing_program_lifecycle_test_support.h"

#include <stdio.h>

uint32_t g_test_apply_workflow_calls = 0u;
DrawingProgramWorkflowControl g_test_last_workflow_control = DRAWING_PROGRAM_WORKFLOW_CONTROL_NONE;
uint32_t g_test_object_nudge_calls = 0u;
uint32_t g_test_selection_nudge_calls = 0u;

int expect_ok(CoreResult result, const char *label) {
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: %s failed code=%d message=%s\n", label, (int)result.code, result.message);
        return 0;
    }
    return 1;
}

int expect_overlay_ok(DrawingProgramOverlayAdapterResult result, const char *label) {
    if (!result.ok) {
        fprintf(stderr, "overlay_test: %s failed code=%d reason=%s\n",
                label, (int)result.error_code, result.reason ? result.reason : "n/a");
        return 0;
    }
    return 1;
}

int expect_overlay_error_code(DrawingProgramOverlayAdapterResult result,
                              DrawingProgramOverlayAdapterErrorCode code,
                              const char *label) {
    if (result.ok || result.error_code != code) {
        fprintf(stderr, "overlay_test: %s expected error_code=%d got ok=%u code=%d reason=%s\n",
                label,
                (int)code,
                (unsigned)result.ok,
                (int)result.error_code,
                result.reason ? result.reason : "n/a");
        return 0;
    }
    return 1;
}

void lifecycle_test_reset_input_handler_counters(void) {
    g_test_apply_workflow_calls = 0u;
    g_test_last_workflow_control = DRAWING_PROGRAM_WORKFLOW_CONTROL_NONE;
    g_test_object_nudge_calls = 0u;
    g_test_selection_nudge_calls = 0u;
}

void lifecycle_test_apply_workflow_control(DrawingProgramAppContext *ctx,
                                           DrawingProgramWorkflowControl control) {
    (void)ctx;
    g_test_apply_workflow_calls += 1u;
    g_test_last_workflow_control = control;
}

CoreResult lifecycle_test_nudge_object_move(DrawingProgramAppContext *ctx,
                                            VisualCanvasInteractionState *interaction,
                                            int32_t dx,
                                            int32_t dy) {
    (void)ctx;
    (void)interaction;
    (void)dx;
    (void)dy;
    g_test_object_nudge_calls += 1u;
    return core_result_ok();
}

CoreResult lifecycle_test_nudge_selection_move(DrawingProgramAppContext *ctx,
                                               VisualCanvasInteractionState *interaction,
                                               DrawingProgramSelectionState *selection,
                                               int32_t dx,
                                               int32_t dy) {
    (void)ctx;
    (void)interaction;
    (void)selection;
    (void)dx;
    (void)dy;
    g_test_selection_nudge_calls += 1u;
    return core_result_ok();
}

void lifecycle_test_cancel_canvas_draw_and_shape(VisualCanvasInteractionState *interaction) {
    (void)interaction;
}

void lifecycle_test_cancel_selection_transient(DrawingProgramSelectionState *selection) {
    (void)selection;
}

void lifecycle_test_cancel_all_transient_interactions(DrawingProgramAppContext *ctx,
                                                      VisualCanvasInteractionState *interaction,
                                                      DrawingProgramSelectionState *selection,
                                                      int clear_pan_state) {
    (void)ctx;
    (void)interaction;
    (void)selection;
    (void)clear_pan_state;
}
