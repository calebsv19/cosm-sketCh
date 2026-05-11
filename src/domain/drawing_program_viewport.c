#include "drawing_program/drawing_program_viewport.h"

#include "core_viewport2d.h"

#include <math.h>
#include <string.h>

enum {
    DRAWING_PROGRAM_VIEWPORT_DEFAULT_MIN_ZOOM_SCALED = 25,
    DRAWING_PROGRAM_VIEWPORT_DEFAULT_MAX_ZOOM_SCALED = 800
};

static const float DRAWING_PROGRAM_VIEWPORT_BASE_PIXEL_SIZE = 0.75f;

static float viewport_clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float drawing_program_viewport_default_min_zoom(void) {
    return (float)DRAWING_PROGRAM_VIEWPORT_DEFAULT_MIN_ZOOM_SCALED / 100.0f;
}

static float drawing_program_viewport_default_max_zoom(void) {
    return (float)DRAWING_PROGRAM_VIEWPORT_DEFAULT_MAX_ZOOM_SCALED / 100.0f;
}

static int drawing_program_viewport_frame_valid(DrawingProgramViewportFrame frame) {
    return isfinite(frame.x) && isfinite(frame.y) && isfinite(frame.width) && isfinite(frame.height) &&
           frame.width > 0.0f && frame.height > 0.0f;
}

static int drawing_program_viewport_document_valid(const DrawingProgramDocument *document) {
    return document && document->raster_width > 0u && document->raster_height > 0u;
}

static float drawing_program_viewport_resolved_min_zoom(const DrawingProgramViewportState *viewport) {
    if (viewport && isfinite(viewport->min_zoom) && viewport->min_zoom > 0.0f) {
        return viewport->min_zoom;
    }
    return drawing_program_viewport_default_min_zoom();
}

static float drawing_program_viewport_resolved_max_zoom(const DrawingProgramViewportState *viewport) {
    float min_zoom = drawing_program_viewport_resolved_min_zoom(viewport);
    if (viewport && isfinite(viewport->max_zoom) && viewport->max_zoom >= min_zoom) {
        return viewport->max_zoom;
    }
    return drawing_program_viewport_default_max_zoom();
}

static float drawing_program_viewport_content_width(const DrawingProgramDocument *document) {
    return (float)document->raster_width * DRAWING_PROGRAM_VIEWPORT_BASE_PIXEL_SIZE;
}

static float drawing_program_viewport_content_height(const DrawingProgramDocument *document) {
    return (float)document->raster_height * DRAWING_PROGRAM_VIEWPORT_BASE_PIXEL_SIZE;
}

static float drawing_program_viewport_fit_padding(DrawingProgramViewportFrame frame) {
    float min_dim = frame.width < frame.height ? frame.width : frame.height;
    float padding = min_dim * 0.05f;
    if (!isfinite(padding) || padding < 0.0f) {
        padding = 0.0f;
    }
    padding = viewport_clampf(padding, 16.0f, 48.0f);
    if ((padding * 2.0f) >= frame.width || (padding * 2.0f) >= frame.height) {
        padding = 0.0f;
    }
    return padding;
}

static CoreResult drawing_program_viewport_state_to_core(const DrawingProgramViewportState *viewport,
                                                         const DrawingProgramDocument *document,
                                                         DrawingProgramViewportFrame frame,
                                                         CoreViewport2D *out_core,
                                                         float *out_content_width,
                                                         float *out_content_height) {
    float zoom;
    float content_width;
    float content_height;
    if (!viewport || !out_core || !drawing_program_viewport_document_valid(document) ||
        !drawing_program_viewport_frame_valid(frame)) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "viewport core bridge requires valid state, document, and frame" };
    }

    content_width = drawing_program_viewport_content_width(document);
    content_height = drawing_program_viewport_content_height(document);
    if (!isfinite(content_width) || !isfinite(content_height) || content_width <= 0.0f || content_height <= 0.0f) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "viewport content dimensions must be positive" };
    }

    zoom = drawing_program_viewport_clamp_zoom(viewport->zoom);
    out_core->min_zoom = drawing_program_viewport_resolved_min_zoom(viewport);
    out_core->max_zoom = drawing_program_viewport_resolved_max_zoom(viewport);
    out_core->zoom = core_viewport2d_clamp_zoom(out_core, zoom);
    out_core->pan_x = (frame.x + (frame.width * 0.5f)) + viewport->pan_x - ((content_width * out_core->zoom) * 0.5f);
    out_core->pan_y = (frame.y + (frame.height * 0.5f)) + viewport->pan_y - ((content_height * out_core->zoom) * 0.5f);

    if (out_content_width) {
        *out_content_width = content_width;
    }
    if (out_content_height) {
        *out_content_height = content_height;
    }
    return core_viewport2d_validate(out_core);
}

static CoreResult drawing_program_viewport_state_from_core(DrawingProgramViewportState *viewport,
                                                           DrawingProgramViewportFrame frame,
                                                           const CoreViewport2D *core,
                                                           float content_width,
                                                           float content_height) {
    if (!viewport || !core || !drawing_program_viewport_frame_valid(frame) ||
        !isfinite(content_width) || !isfinite(content_height) ||
        content_width <= 0.0f || content_height <= 0.0f) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "viewport core bridge writeback requires valid inputs" };
    }
    viewport->pan_x = core->pan_x - (frame.x + (frame.width * 0.5f)) + ((content_width * core->zoom) * 0.5f);
    viewport->pan_y = core->pan_y - (frame.y + (frame.height * 0.5f)) + ((content_height * core->zoom) * 0.5f);
    viewport->zoom = drawing_program_viewport_clamp_zoom(core->zoom);
    viewport->min_zoom = drawing_program_viewport_resolved_min_zoom(core);
    viewport->max_zoom = drawing_program_viewport_resolved_max_zoom(core);
    return core_result_ok();
}

static int drawing_program_viewport_content_to_sample(const DrawingProgramDocument *document,
                                                      float content_x,
                                                      float content_y,
                                                      DrawingProgramSamplePoint *out_sample) {
    uint32_t sample_x;
    uint32_t sample_y;
    if (!drawing_program_viewport_document_valid(document) || !out_sample ||
        !isfinite(content_x) || !isfinite(content_y)) {
        return 0;
    }
    if (content_x < 0.0f || content_y < 0.0f ||
        content_x >= drawing_program_viewport_content_width(document) ||
        content_y >= drawing_program_viewport_content_height(document)) {
        return 0;
    }
    sample_x = (uint32_t)floorf(content_x / DRAWING_PROGRAM_VIEWPORT_BASE_PIXEL_SIZE);
    sample_y = (uint32_t)floorf(content_y / DRAWING_PROGRAM_VIEWPORT_BASE_PIXEL_SIZE);
    if (sample_x >= document->raster_width || sample_y >= document->raster_height) {
        return 0;
    }
    out_sample->x = sample_x;
    out_sample->y = sample_y;
    return 1;
}

void drawing_program_viewport_state_init(DrawingProgramViewportState *viewport) {
    if (!viewport) {
        return;
    }
    viewport->pan_x = 0.0f;
    viewport->pan_y = 0.0f;
    viewport->zoom = 1.0f;
    viewport->min_zoom = drawing_program_viewport_default_min_zoom();
    viewport->max_zoom = drawing_program_viewport_default_max_zoom();
}

void drawing_program_viewport_reset(DrawingProgramViewportState *viewport) {
    drawing_program_viewport_state_init(viewport);
}

float drawing_program_viewport_clamp_zoom(float zoom) {
    if (!isfinite(zoom)) {
        return 1.0f;
    }
    return viewport_clampf(zoom,
                           drawing_program_viewport_default_min_zoom(),
                           drawing_program_viewport_default_max_zoom());
}

float drawing_program_viewport_base_pixel_size(void) {
    return DRAWING_PROGRAM_VIEWPORT_BASE_PIXEL_SIZE;
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

bool drawing_program_viewport_measure_sheet_in_frame(const DrawingProgramViewportState *viewport,
                                                     const DrawingProgramDocument *document,
                                                     DrawingProgramViewportFrame frame,
                                                     float *out_sheet_x,
                                                     float *out_sheet_y,
                                                     float *out_sheet_w,
                                                     float *out_sheet_h,
                                                     float *out_pixel_size) {
    CoreViewport2D core;
    float content_width = 0.0f;
    float content_height = 0.0f;
    CoreResult result = drawing_program_viewport_state_to_core(
        viewport, document, frame, &core, &content_width, &content_height);
    if (result.code != CORE_OK) {
        return false;
    }
    if (out_sheet_x) {
        *out_sheet_x = core.pan_x;
    }
    if (out_sheet_y) {
        *out_sheet_y = core.pan_y;
    }
    if (out_sheet_w) {
        *out_sheet_w = content_width * core.zoom;
    }
    if (out_sheet_h) {
        *out_sheet_h = content_height * core.zoom;
    }
    if (out_pixel_size) {
        *out_pixel_size = DRAWING_PROGRAM_VIEWPORT_BASE_PIXEL_SIZE * core.zoom;
    }
    return true;
}

bool drawing_program_viewport_pan_in_frame(DrawingProgramViewportState *viewport,
                                           const DrawingProgramDocument *document,
                                           DrawingProgramViewportFrame frame,
                                           float delta_x,
                                           float delta_y) {
    CoreViewport2D core;
    float content_width = 0.0f;
    float content_height = 0.0f;
    CoreResult result = drawing_program_viewport_state_to_core(
        viewport, document, frame, &core, &content_width, &content_height);
    if (result.code != CORE_OK) {
        return false;
    }
    result = core_viewport2d_pan_by(&core, delta_x, delta_y);
    if (result.code != CORE_OK) {
        return false;
    }
    return drawing_program_viewport_state_from_core(viewport, frame, &core, content_width, content_height).code == CORE_OK;
}

bool drawing_program_viewport_reset_to_fit_in_frame(DrawingProgramViewportState *viewport,
                                                    const DrawingProgramDocument *document,
                                                    DrawingProgramViewportFrame frame) {
    CoreViewport2D core;
    DrawingProgramViewportFrame fit_frame = frame;
    float content_width;
    float content_height;
    float padding;
    CoreResult result;
    if (!viewport || !drawing_program_viewport_document_valid(document) || !drawing_program_viewport_frame_valid(frame)) {
        return false;
    }
    content_width = drawing_program_viewport_content_width(document);
    content_height = drawing_program_viewport_content_height(document);
    if (!isfinite(content_width) || !isfinite(content_height) || content_width <= 0.0f || content_height <= 0.0f) {
        return false;
    }

    padding = drawing_program_viewport_fit_padding(frame);
    fit_frame.x += padding;
    fit_frame.y += padding;
    fit_frame.width -= padding * 2.0f;
    fit_frame.height -= padding * 2.0f;
    if (!drawing_program_viewport_frame_valid(fit_frame)) {
        fit_frame = frame;
    }

    (void)core_viewport2d_init(&core);
    core.min_zoom = drawing_program_viewport_resolved_min_zoom(viewport);
    core.max_zoom = drawing_program_viewport_resolved_max_zoom(viewport);
    core.zoom = core_viewport2d_clamp_zoom(&core, viewport->zoom);
    result = core_viewport2d_reset_to_fit(&core,
                                          fit_frame.width,
                                          fit_frame.height,
                                          content_width,
                                          content_height);
    if (result.code != CORE_OK) {
        return false;
    }
    core.pan_x += fit_frame.x;
    core.pan_y += fit_frame.y;
    return drawing_program_viewport_state_from_core(viewport, fit_frame, &core, content_width, content_height).code ==
           CORE_OK;
}

bool drawing_program_viewport_zoom_at_screen_anchor_in_frame(DrawingProgramViewportState *viewport,
                                                             const DrawingProgramDocument *document,
                                                             DrawingProgramViewportFrame frame,
                                                             float screen_x,
                                                             float screen_y,
                                                             float zoom_factor) {
    CoreViewport2D core;
    float content_width = 0.0f;
    float content_height = 0.0f;
    CoreResult result = drawing_program_viewport_state_to_core(
        viewport, document, frame, &core, &content_width, &content_height);
    if (result.code != CORE_OK) {
        return false;
    }
    result = core_viewport2d_zoom_at_screen_anchor(&core, screen_x, screen_y, zoom_factor);
    if (result.code != CORE_OK) {
        return false;
    }
    return drawing_program_viewport_state_from_core(viewport, frame, &core, content_width, content_height).code == CORE_OK;
}

bool drawing_program_viewport_screen_to_sample_in_frame(const DrawingProgramViewportState *viewport,
                                                        const DrawingProgramDocument *document,
                                                        DrawingProgramViewportFrame frame,
                                                        DrawingProgramScreenPoint screen,
                                                        DrawingProgramSamplePoint *out_sample) {
    CoreViewport2D core;
    float content_x = 0.0f;
    float content_y = 0.0f;
    CoreResult result = drawing_program_viewport_state_to_core(viewport, document, frame, &core, 0, 0);
    if (result.code != CORE_OK) {
        return false;
    }
    result = core_viewport2d_screen_to_content(&core, screen.x, screen.y, &content_x, &content_y);
    if (result.code != CORE_OK) {
        return false;
    }
    return drawing_program_viewport_content_to_sample(document, content_x, content_y, out_sample) != 0;
}

bool drawing_program_viewport_screen_to_sample_clamped_in_frame(const DrawingProgramViewportState *viewport,
                                                                const DrawingProgramDocument *document,
                                                                DrawingProgramViewportFrame frame,
                                                                DrawingProgramScreenPoint screen,
                                                                DrawingProgramSamplePoint *out_sample) {
    CoreViewport2D core;
    float content_width = 0.0f;
    float content_height = 0.0f;
    float content_x = 0.0f;
    float content_y = 0.0f;
    CoreResult result = drawing_program_viewport_state_to_core(
        viewport, document, frame, &core, &content_width, &content_height);
    if (result.code != CORE_OK || !out_sample) {
        return false;
    }
    result = core_viewport2d_screen_to_content(&core, screen.x, screen.y, &content_x, &content_y);
    if (result.code != CORE_OK) {
        return false;
    }
    content_x = viewport_clampf(content_x, 0.0f, content_width - 0.0001f);
    content_y = viewport_clampf(content_y, 0.0f, content_height - 0.0001f);
    return drawing_program_viewport_content_to_sample(document, content_x, content_y, out_sample) != 0;
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
