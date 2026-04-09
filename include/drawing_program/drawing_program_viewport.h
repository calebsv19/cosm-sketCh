#ifndef DRAWING_PROGRAM_VIEWPORT_H
#define DRAWING_PROGRAM_VIEWPORT_H

#include <stdbool.h>
#include <stdint.h>

#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_editor_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DrawingProgramScreenPoint {
    float x;
    float y;
} DrawingProgramScreenPoint;

typedef struct DrawingProgramViewportPoint {
    float x;
    float y;
} DrawingProgramViewportPoint;

typedef struct DrawingProgramCanvasPoint {
    float x;
    float y;
} DrawingProgramCanvasPoint;

typedef struct DrawingProgramSamplePoint {
    uint32_t x;
    uint32_t y;
} DrawingProgramSamplePoint;

typedef struct DrawingProgramViewportTransform {
    float pan_x;
    float pan_y;
    float zoom;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
} DrawingProgramViewportTransform;

float drawing_program_viewport_clamp_zoom(float zoom);
void drawing_program_viewport_transform_from_state(const DrawingProgramEditorState *editor,
                                                   const DrawingProgramDocument *document,
                                                   DrawingProgramViewportTransform *out_transform);

DrawingProgramViewportPoint drawing_program_screen_to_viewport(DrawingProgramViewportTransform transform,
                                                               DrawingProgramScreenPoint screen);
DrawingProgramScreenPoint drawing_program_viewport_to_screen(DrawingProgramViewportTransform transform,
                                                             DrawingProgramViewportPoint viewport);
DrawingProgramCanvasPoint drawing_program_viewport_to_canvas(DrawingProgramViewportTransform transform,
                                                             DrawingProgramViewportPoint viewport);
DrawingProgramViewportPoint drawing_program_canvas_to_viewport(DrawingProgramViewportTransform transform,
                                                               DrawingProgramCanvasPoint canvas);
DrawingProgramCanvasPoint drawing_program_screen_to_canvas(DrawingProgramViewportTransform transform,
                                                           DrawingProgramScreenPoint screen);
DrawingProgramScreenPoint drawing_program_canvas_to_screen(DrawingProgramViewportTransform transform,
                                                           DrawingProgramCanvasPoint canvas);
bool drawing_program_canvas_to_sample(DrawingProgramViewportTransform transform,
                                      DrawingProgramCanvasPoint canvas,
                                      DrawingProgramSamplePoint *out_sample);
bool drawing_program_screen_to_sample(DrawingProgramViewportTransform transform,
                                      DrawingProgramScreenPoint screen,
                                      DrawingProgramSamplePoint *out_sample);
DrawingProgramCanvasPoint drawing_program_sample_to_canvas_center(DrawingProgramViewportTransform transform,
                                                                  DrawingProgramSamplePoint sample);

#ifdef __cplusplus
}
#endif

#endif
