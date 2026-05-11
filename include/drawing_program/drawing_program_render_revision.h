#ifndef DRAWING_PROGRAM_RENDER_REVISION_H
#define DRAWING_PROGRAM_RENDER_REVISION_H

#include <stdint.h>

#include "drawing_program/drawing_program_app_main.h"

#ifdef __cplusplus
extern "C" {
#endif

uint64_t drawing_program_render_revision_compose_active_surface_content(
    const DrawingProgramAppContext *ctx);
void drawing_program_render_revision_refresh(DrawingProgramAppContext *ctx);
void drawing_program_render_revision_mark_layer_opacity_changed(DrawingProgramAppContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
