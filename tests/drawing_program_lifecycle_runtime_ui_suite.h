#ifndef DRAWING_PROGRAM_LIFECYCLE_RUNTIME_UI_SUITE_H
#define DRAWING_PROGRAM_LIFECYCLE_RUNTIME_UI_SUITE_H

#include "drawing_program/drawing_program_app_main.h"

int drawing_program_lifecycle_run_runtime_ui_suite(DrawingProgramAppContext *ctx,
                                                   uint32_t center_x,
                                                   uint32_t center_y,
                                                   uint8_t center_before,
                                                   uint8_t expected_draw_value);

#endif
