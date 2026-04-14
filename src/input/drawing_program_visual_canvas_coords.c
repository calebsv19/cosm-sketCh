#include "drawing_program/drawing_program_visual_canvas_coords.h"

#include "drawing_program/drawing_program_visual_pane_bindings.h"

void drawing_program_visual_compute_canvas_sheet_metrics(const DrawingProgramAppContext *ctx,
                                                         SDL_Rect pane_rect,
                                                         VisualCanvasSheetMetrics *out_metrics) {
    float zoom;
    float base_pixel = 0.75f;
    float pixel_size;
    int sheet_w;
    int sheet_h;
    int cx;
    int cy;
    if (!ctx || !out_metrics) {
        return;
    }
    zoom = ctx->editor.viewport.zoom;
    if (zoom < 0.25f) {
        zoom = 0.25f;
    }
    if (zoom > 8.0f) {
        zoom = 8.0f;
    }
    pixel_size = base_pixel * zoom;
    if (pixel_size < 0.25f) {
        pixel_size = 0.25f;
    }
    if (pixel_size > 16.0f) {
        pixel_size = 16.0f;
    }
    sheet_w = (int)((float)ctx->document.raster_width * pixel_size);
    sheet_h = (int)((float)ctx->document.raster_height * pixel_size);
    cx = pane_rect.x + pane_rect.w / 2 + (int)ctx->editor.viewport.pan_x;
    cy = pane_rect.y + pane_rect.h / 2 + (int)ctx->editor.viewport.pan_y;
    out_metrics->pixel_size = pixel_size;
    out_metrics->sheet_rect.x = cx - sheet_w / 2;
    out_metrics->sheet_rect.y = cy - sheet_h / 2;
    out_metrics->sheet_rect.w = sheet_w;
    out_metrics->sheet_rect.h = sheet_h;
}

int drawing_program_visual_screen_to_canvas_sample(const DrawingProgramAppContext *ctx,
                                                   SDL_Rect pane_rect,
                                                   int sx,
                                                   int sy,
                                                   uint32_t *out_sample_x,
                                                   uint32_t *out_sample_y) {
    VisualCanvasSheetMetrics metrics;
    int local_x;
    int local_y;
    if (!ctx || !out_sample_x || !out_sample_y) {
        return 0;
    }
    drawing_program_visual_compute_canvas_sheet_metrics(ctx, pane_rect, &metrics);
    if (!drawing_program_visual_point_in_rect(metrics.sheet_rect, sx, sy)) {
        return 0;
    }
    local_x = sx - metrics.sheet_rect.x;
    local_y = sy - metrics.sheet_rect.y;
    if (metrics.pixel_size <= 0.0f) {
        return 0;
    }
    *out_sample_x = (uint32_t)((float)local_x / metrics.pixel_size);
    *out_sample_y = (uint32_t)((float)local_y / metrics.pixel_size);
    if (*out_sample_x >= ctx->document.raster_width || *out_sample_y >= ctx->document.raster_height) {
        return 0;
    }
    return 1;
}

int drawing_program_visual_screen_to_canvas_sample_clamped(const DrawingProgramAppContext *ctx,
                                                           SDL_Rect pane_rect,
                                                           int sx,
                                                           int sy,
                                                           uint32_t *out_sample_x,
                                                           uint32_t *out_sample_y) {
    VisualCanvasSheetMetrics metrics;
    int local_x;
    int local_y;
    int32_t sample_x;
    int32_t sample_y;
    if (!ctx || !out_sample_x || !out_sample_y || ctx->document.raster_width == 0u || ctx->document.raster_height == 0u) {
        return 0;
    }
    drawing_program_visual_compute_canvas_sheet_metrics(ctx, pane_rect, &metrics);
    if (metrics.pixel_size <= 0.0f) {
        return 0;
    }
    local_x = sx - metrics.sheet_rect.x;
    local_y = sy - metrics.sheet_rect.y;
    sample_x = (int32_t)((float)local_x / metrics.pixel_size);
    sample_y = (int32_t)((float)local_y / metrics.pixel_size);
    if (sample_x < 0) {
        sample_x = 0;
    }
    if (sample_y < 0) {
        sample_y = 0;
    }
    if ((uint32_t)sample_x >= ctx->document.raster_width) {
        sample_x = (int32_t)ctx->document.raster_width - 1;
    }
    if ((uint32_t)sample_y >= ctx->document.raster_height) {
        sample_y = (int32_t)ctx->document.raster_height - 1;
    }
    *out_sample_x = (uint32_t)sample_x;
    *out_sample_y = (uint32_t)sample_y;
    return 1;
}
