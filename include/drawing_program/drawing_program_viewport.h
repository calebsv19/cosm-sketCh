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

typedef struct DrawingProgramViewportFrame {
    float x;
    float y;
    float width;
    float height;
} DrawingProgramViewportFrame;

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

void drawing_program_viewport_state_init(DrawingProgramViewportState *viewport);
void drawing_program_viewport_reset(DrawingProgramViewportState *viewport);
float drawing_program_viewport_clamp_zoom(float zoom);
float drawing_program_viewport_base_pixel_size(void);
void drawing_program_viewport_transform_from_state(const DrawingProgramEditorState *editor,
                                                   const DrawingProgramDocument *document,
                                                   DrawingProgramViewportTransform *out_transform);
bool drawing_program_viewport_measure_sheet_in_frame(const DrawingProgramViewportState *viewport,
                                                     const DrawingProgramDocument *document,
                                                     DrawingProgramViewportFrame frame,
                                                     float *out_sheet_x,
                                                     float *out_sheet_y,
                                                     float *out_sheet_w,
                                                     float *out_sheet_h,
                                                     float *out_pixel_size);
bool drawing_program_viewport_pan_in_frame(DrawingProgramViewportState *viewport,
                                           const DrawingProgramDocument *document,
                                           DrawingProgramViewportFrame frame,
                                           float delta_x,
                                           float delta_y);
bool drawing_program_viewport_reset_to_fit_in_frame(DrawingProgramViewportState *viewport,
                                                    const DrawingProgramDocument *document,
                                                    DrawingProgramViewportFrame frame);
bool drawing_program_viewport_zoom_at_screen_anchor_in_frame(DrawingProgramViewportState *viewport,
                                                             const DrawingProgramDocument *document,
                                                             DrawingProgramViewportFrame frame,
                                                             float screen_x,
                                                             float screen_y,
                                                             float zoom_factor);
bool drawing_program_viewport_screen_to_sample_in_frame(const DrawingProgramViewportState *viewport,
                                                        const DrawingProgramDocument *document,
                                                        DrawingProgramViewportFrame frame,
                                                        DrawingProgramScreenPoint screen,
                                                        DrawingProgramSamplePoint *out_sample);
bool drawing_program_viewport_screen_to_sample_clamped_in_frame(const DrawingProgramViewportState *viewport,
                                                                const DrawingProgramDocument *document,
                                                                DrawingProgramViewportFrame frame,
                                                                DrawingProgramScreenPoint screen,
                                                                DrawingProgramSamplePoint *out_sample);

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
