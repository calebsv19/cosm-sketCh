#include "drawing_program_lifecycle_test_support.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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

const char *lifecycle_test_artifact_root(void) {
    const char *configured_root = getenv("DRAWING_PROGRAM_TEST_ARTIFACT_ROOT");
    const char *tmp_root = getenv("TMPDIR");
    static char default_root[512];
    int written = 0;
    if (configured_root && configured_root[0]) {
        return configured_root;
    }
    if (!tmp_root || !tmp_root[0]) {
        tmp_root = "/tmp";
    }
    written = snprintf(default_root, sizeof(default_root), "%s/%s", tmp_root, "drawing_program_lifecycle_tests");
    if (written < 0 || (size_t)written >= sizeof(default_root)) {
        return "/tmp/drawing_program_lifecycle_tests";
    }
    return default_root;
}

int lifecycle_test_artifact_path(char *buffer, size_t buffer_size, const char *leaf_name) {
    const char *root = lifecycle_test_artifact_root();
    int written = 0;
    if (!buffer || buffer_size == 0u || !root || !leaf_name || !leaf_name[0]) {
        return 0;
    }
    if (mkdir(root, 0700) != 0 && errno != EEXIST) {
        fprintf(stderr, "lifecycle_test: failed to create artifact root %s: %s\n", root, strerror(errno));
        return 0;
    }
    written = snprintf(buffer, buffer_size, "%s/%s", root, leaf_name);
    if (written < 0 || (size_t)written >= buffer_size) {
        fprintf(stderr,
                "lifecycle_test: artifact path too long root=%s leaf=%s\n",
                root,
                leaf_name);
        return 0;
    }
    return 1;
}

int lifecycle_test_artifact_child_path(char *buffer,
                                       size_t buffer_size,
                                       const char *parent_path,
                                       const char *leaf_name) {
    int written = 0;
    if (!buffer || buffer_size == 0u || !parent_path || !parent_path[0] || !leaf_name || !leaf_name[0]) {
        return 0;
    }
    written = snprintf(buffer, buffer_size, "%s/%s", parent_path, leaf_name);
    if (written < 0 || (size_t)written >= buffer_size) {
        fprintf(stderr,
                "lifecycle_test: artifact child path too long parent=%s leaf=%s\n",
                parent_path,
                leaf_name);
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
    if (!interaction) {
        return;
    }
    interaction->reflector_drag_active = 0u;
    interaction->reflector_drag_kind = 0u;
    interaction->reflector_drag_index = 0u;
}

void lifecycle_test_cancel_selection_transient(DrawingProgramSelectionState *selection) {
    (void)selection;
}

void lifecycle_test_cancel_all_transient_interactions(DrawingProgramAppContext *ctx,
                                                      VisualCanvasInteractionState *interaction,
                                                      DrawingProgramSelectionState *selection,
                                                      int clear_pan_state) {
    (void)ctx;
    (void)selection;
    (void)clear_pan_state;
    lifecycle_test_cancel_canvas_draw_and_shape(interaction);
}
