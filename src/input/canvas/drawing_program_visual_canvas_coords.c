#include "drawing_program/drawing_program_visual_canvas_coords.h"

#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_viewport.h"

static DrawingProgramViewportFrame drawing_program_viewport_frame_from_rect(SDL_Rect rect) {
    DrawingProgramViewportFrame frame;
    frame.x = (float)rect.x;
    frame.y = (float)rect.y;
    frame.width = (float)rect.w;
    frame.height = (float)rect.h;
    return frame;
}

void drawing_program_visual_compute_canvas_sheet_metrics(const DrawingProgramAppContext *ctx,
                                                         SDL_Rect pane_rect,
                                                         VisualCanvasSheetMetrics *out_metrics) {
    float sheet_x = 0.0f;
    float sheet_y = 0.0f;
    float sheet_w = 0.0f;
    float sheet_h = 0.0f;
    float pixel_size = 0.0f;
    if (!ctx || !out_metrics) {
        return;
    }
    if (!drawing_program_viewport_measure_sheet_in_frame(&ctx->editor.viewport,
                                                         &ctx->document,
                                                         drawing_program_viewport_frame_from_rect(pane_rect),
                                                         &sheet_x,
                                                         &sheet_y,
                                                         &sheet_w,
                                                         &sheet_h,
                                                         &pixel_size)) {
        out_metrics->pixel_size = 0.0f;
        out_metrics->sheet_rect = (SDL_Rect){0, 0, 0, 0};
        return;
    }
    out_metrics->pixel_size = pixel_size;
    out_metrics->sheet_rect.x = (int)sheet_x;
    out_metrics->sheet_rect.y = (int)sheet_y;
    out_metrics->sheet_rect.w = (int)sheet_w;
    out_metrics->sheet_rect.h = (int)sheet_h;
}

int drawing_program_visual_screen_to_canvas_sample(const DrawingProgramAppContext *ctx,
                                                   SDL_Rect pane_rect,
                                                   int sx,
                                                   int sy,
                                                   uint32_t *out_sample_x,
                                                   uint32_t *out_sample_y) {
    DrawingProgramSamplePoint sample;
    if (!ctx || !out_sample_x || !out_sample_y) {
        return 0;
    }
    if (!drawing_program_viewport_screen_to_sample_in_frame(&ctx->editor.viewport,
                                                            &ctx->document,
                                                            drawing_program_viewport_frame_from_rect(pane_rect),
                                                            (DrawingProgramScreenPoint){(float)sx, (float)sy},
                                                            &sample)) {
        return 0;
    }
    *out_sample_x = sample.x;
    *out_sample_y = sample.y;
    return 1;
}

int drawing_program_visual_screen_to_canvas_sample_clamped(const DrawingProgramAppContext *ctx,
                                                           SDL_Rect pane_rect,
                                                           int sx,
                                                           int sy,
                                                           uint32_t *out_sample_x,
                                                           uint32_t *out_sample_y) {
    DrawingProgramSamplePoint sample;
    if (!ctx || !out_sample_x || !out_sample_y || ctx->document.raster_width == 0u || ctx->document.raster_height == 0u) {
        return 0;
    }
    if (!drawing_program_viewport_screen_to_sample_clamped_in_frame(&ctx->editor.viewport,
                                                                    &ctx->document,
                                                                    drawing_program_viewport_frame_from_rect(pane_rect),
                                                                    (DrawingProgramScreenPoint){(float)sx, (float)sy},
                                                                    &sample)) {
        return 0;
    }
    *out_sample_x = sample.x;
    *out_sample_y = sample.y;
    return 1;
}
