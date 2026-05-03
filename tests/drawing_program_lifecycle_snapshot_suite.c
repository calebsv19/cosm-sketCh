#include "drawing_program_lifecycle_snapshot_suite.h"

#include "drawing_program_lifecycle_snapshot_suite_internal.h"

int drawing_program_lifecycle_run_snapshot_suite(DrawingProgramAppContext *ctx) {
    if (drawing_program_lifecycle_run_snapshot_shell_suite(ctx) != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_snapshot_layer_suite() != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_snapshot_object_suite() != 0) {
        return 1;
    }
    return 0;
}
