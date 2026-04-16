#ifndef DRAWING_PROGRAM_LIFECYCLE_BASELINE_HISTORY_SUITE_H
#define DRAWING_PROGRAM_LIFECYCLE_BASELINE_HISTORY_SUITE_H

#include "drawing_program/drawing_program_app_main.h"

int drawing_program_lifecycle_run_baseline_history_suite(DrawingProgramAppContext *ctx,
                                                         uint8_t expected_eraser_value,
                                                         uint32_t *center_x_out,
                                                         uint32_t *center_y_out,
                                                         uint8_t *center_before_out);

#endif
