#pragma once
#include <stdbool.h>
#include "drawing_program/drawing_program_app_main.h"
typedef struct SketchControl SketchControl;
SketchControl *sketch_control_start(const char *path, bool allow_write, const char *output_root);
bool sketch_control_poll(SketchControl *control, DrawingProgramAppContext *app, bool busy);
void sketch_control_stop(SketchControl *control);
/* Domain dispatch is owner-thread only; used by transport and contained tests. */
char *sketch_control_dispatch(SketchControl *control, DrawingProgramAppContext *app, const char *request, bool busy, bool *changed);
