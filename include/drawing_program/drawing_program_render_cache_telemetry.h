#ifndef DRAWING_PROGRAM_RENDER_CACHE_TELEMETRY_H
#define DRAWING_PROGRAM_RENDER_CACHE_TELEMETRY_H

#include <stdint.h>

#include "drawing_program/drawing_program_visual_surface_cache.h"

struct DrawingProgramAppContext;

void drawing_program_render_cache_note_surface_sync(
    struct DrawingProgramAppContext *ctx,
    const DrawingProgramVisualSurfaceCacheTelemetry *telemetry,
    uint8_t active_surface);
void drawing_program_render_cache_note_queue_step(
    struct DrawingProgramAppContext *ctx,
    const DrawingProgramVisualSurfaceCacheTelemetry *telemetry);

#endif
