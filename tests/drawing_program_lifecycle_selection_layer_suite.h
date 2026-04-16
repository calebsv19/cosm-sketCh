#ifndef DRAWING_PROGRAM_LIFECYCLE_SELECTION_LAYER_SUITE_H
#define DRAWING_PROGRAM_LIFECYCLE_SELECTION_LAYER_SUITE_H

#include "drawing_program/drawing_program_app_main.h"

int drawing_program_lifecycle_run_selection_layer_suite(DrawingProgramAppContext *workflow_ctx,
                                                        DrawingProgramClipboardState *workflow_clipboard,
                                                        char **argv,
                                                        uint32_t workflow_center_x,
                                                        uint32_t workflow_center_y,
                                                        uint8_t expected_draw_value,
                                                        uint8_t expected_eraser_value);

#endif
