#include "drawing_program/drawing_program_viewport.h"

#include <math.h>
#include <string.h>

static float viewport_clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

float drawing_program_viewport_clamp_zoom(float zoom) {
    if (!isfinite(zoom)) {
        return 1.0f;
    }
    return viewport_clampf(zoom, 0.125f, 64.0f);
}

void drawing_program_viewport_transform_from_state(const DrawingProgramEditorState *editor,
                                                   const DrawingProgramDocument *document,
                                                   DrawingProgramViewportTransform *out_transform) {
    if (!out_transform) {
        return;
    }
    memset(out_transform, 0, sizeof(*out_transform));
    out_transform->zoom = 1.0f;
    out_transform->sample_density = 1u;
    if (editor) {
        out_transform->pan_x = editor->viewport.pan_x;
        out_transform->pan_y = editor->viewport.pan_y;
        out_transform->zoom = drawing_program_viewport_clamp_zoom(editor->viewport.zoom);
    }
    if (document) {
        out_transform->logical_width = document->logical_width;
        out_transform->logical_height = document->logical_height;
        out_transform->sample_density = document->sample_density > 0u ? document->sample_density : 1u;
    }
}

DrawingProgramViewportPoint drawing_program_screen_to_viewport(DrawingProgramViewportTransform transform,
                                                               DrawingProgramScreenPoint screen) {
    DrawingProgramViewportPoint viewport;
    viewport.x = screen.x - transform.pan_x;
    viewport.y = screen.y - transform.pan_y;
    return viewport;
}

DrawingProgramScreenPoint drawing_program_viewport_to_screen(DrawingProgramViewportTransform transform,
                                                             DrawingProgramViewportPoint viewport) {
    DrawingProgramScreenPoint screen;
    screen.x = viewport.x + transform.pan_x;
    screen.y = viewport.y + transform.pan_y;
    return screen;
}

DrawingProgramCanvasPoint drawing_program_viewport_to_canvas(DrawingProgramViewportTransform transform,
                                                             DrawingProgramViewportPoint viewport) {
    DrawingProgramCanvasPoint canvas;
    float zoom = drawing_program_viewport_clamp_zoom(transform.zoom);
    canvas.x = viewport.x / zoom;
    canvas.y = viewport.y / zoom;
    return canvas;
}

DrawingProgramViewportPoint drawing_program_canvas_to_viewport(DrawingProgramViewportTransform transform,
                                                               DrawingProgramCanvasPoint canvas) {
    DrawingProgramViewportPoint viewport;
    float zoom = drawing_program_viewport_clamp_zoom(transform.zoom);
    viewport.x = canvas.x * zoom;
    viewport.y = canvas.y * zoom;
    return viewport;
}

DrawingProgramCanvasPoint drawing_program_screen_to_canvas(DrawingProgramViewportTransform transform,
                                                           DrawingProgramScreenPoint screen) {
    DrawingProgramViewportPoint viewport = drawing_program_screen_to_viewport(transform, screen);
    return drawing_program_viewport_to_canvas(transform, viewport);
}

DrawingProgramScreenPoint drawing_program_canvas_to_screen(DrawingProgramViewportTransform transform,
                                                           DrawingProgramCanvasPoint canvas) {
    DrawingProgramViewportPoint viewport = drawing_program_canvas_to_viewport(transform, canvas);
    return drawing_program_viewport_to_screen(transform, viewport);
}

bool drawing_program_canvas_to_sample(DrawingProgramViewportTransform transform,
                                      DrawingProgramCanvasPoint canvas,
                                      DrawingProgramSamplePoint *out_sample) {
    float max_x;
    float max_y;
    float sample_x_f;
    float sample_y_f;
    uint32_t sample_x;
    uint32_t sample_y;
    if (!out_sample || transform.logical_width == 0u || transform.logical_height == 0u) {
        return false;
    }
    if (!isfinite(canvas.x) || !isfinite(canvas.y)) {
        return false;
    }

    max_x = (float)transform.logical_width;
    max_y = (float)transform.logical_height;
    if (canvas.x < 0.0f || canvas.y < 0.0f || canvas.x >= max_x || canvas.y >= max_y) {
        return false;
    }

    sample_x_f = canvas.x * (float)transform.sample_density;
    sample_y_f = canvas.y * (float)transform.sample_density;
    sample_x = (uint32_t)floorf(sample_x_f);
    sample_y = (uint32_t)floorf(sample_y_f);
    if (sample_x >= transform.logical_width * transform.sample_density ||
        sample_y >= transform.logical_height * transform.sample_density) {
        return false;
    }
    out_sample->x = sample_x;
    out_sample->y = sample_y;
    return true;
}

bool drawing_program_screen_to_sample(DrawingProgramViewportTransform transform,
                                      DrawingProgramScreenPoint screen,
                                      DrawingProgramSamplePoint *out_sample) {
    DrawingProgramCanvasPoint canvas = drawing_program_screen_to_canvas(transform, screen);
    return drawing_program_canvas_to_sample(transform, canvas, out_sample);
}

DrawingProgramCanvasPoint drawing_program_sample_to_canvas_center(DrawingProgramViewportTransform transform,
                                                                  DrawingProgramSamplePoint sample) {
    DrawingProgramCanvasPoint canvas;
    float density = (float)(transform.sample_density > 0u ? transform.sample_density : 1u);
    canvas.x = ((float)sample.x + 0.5f) / density;
    canvas.y = ((float)sample.y + 0.5f) / density;
    return canvas;
}
