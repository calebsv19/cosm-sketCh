#ifndef DRAWING_PROGRAM_CANVAS_REFLECTION_H
#define DRAWING_PROGRAM_CANVAS_REFLECTION_H

#include <stdint.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_texture_project.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY 4u

typedef struct DrawingProgramCanvasReflectionPoint {
    int32_t x;
    int32_t y;
} DrawingProgramCanvasReflectionPoint;

typedef struct DrawingProgramCanvasReflectionSegment {
    int32_t start_x;
    int32_t start_y;
    int32_t end_x;
    int32_t end_y;
} DrawingProgramCanvasReflectionSegment;

void drawing_program_canvas_reflection_default_center_for_document(const DrawingProgramDocument *document,
                                                                   uint32_t *out_x,
                                                                   uint32_t *out_y);
void drawing_program_canvas_reflection_surface_seed_defaults(DrawingProgramTextureSurface *surface,
                                                             const DrawingProgramDocument *document);
void drawing_program_canvas_reflection_surface_clamp_to_document(DrawingProgramTextureSurface *surface,
                                                                 const DrawingProgramDocument *document);
void drawing_program_canvas_reflection_sync_editor_from_active_surface(DrawingProgramAppContext *ctx);
void drawing_program_canvas_reflection_sync_active_surface_from_editor(DrawingProgramAppContext *ctx);
int drawing_program_canvas_reflection_active_center(const DrawingProgramAppContext *ctx,
                                                    uint32_t *out_x,
                                                    uint32_t *out_y);
void drawing_program_canvas_reflection_reset_active_center(DrawingProgramAppContext *ctx);
int drawing_program_canvas_reflection_set_active_center(DrawingProgramAppContext *ctx,
                                                        uint32_t sample_x,
                                                        uint32_t sample_y);
int drawing_program_canvas_reflection_enabled(const DrawingProgramAppContext *ctx);
uint32_t drawing_program_canvas_reflection_collect_points(
    const DrawingProgramAppContext *ctx,
    int32_t sample_x,
    int32_t sample_y,
    DrawingProgramCanvasReflectionPoint out_points[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY]);
uint32_t drawing_program_canvas_reflection_collect_segments(
    const DrawingProgramAppContext *ctx,
    int32_t start_x,
    int32_t start_y,
    int32_t end_x,
    int32_t end_y,
    DrawingProgramCanvasReflectionSegment out_segments[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY]);

#ifdef __cplusplus
}
#endif

#endif
