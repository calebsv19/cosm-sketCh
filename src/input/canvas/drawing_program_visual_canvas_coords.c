#include "drawing_program/drawing_program_visual_canvas_coords.h"

#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_texture_workspace.h"

void drawing_program_visual_compute_canvas_sheet_metrics(const DrawingProgramAppContext *ctx,
                                                         SDL_Rect pane_rect,
                                                         VisualCanvasSheetMetrics *out_metrics) {
    if (!ctx || !out_metrics) {
        return;
    }
    if (!drawing_program_texture_workspace_active_sheet_metrics(ctx, pane_rect, out_metrics)) {
        out_metrics->pixel_size = 0.0f;
        out_metrics->sheet_rect = (SDL_Rect){0, 0, 0, 0};
    }
}

int drawing_program_visual_screen_to_canvas_sample(const DrawingProgramAppContext *ctx,
                                                   SDL_Rect pane_rect,
                                                   int sx,
                                                   int sy,
                                                   uint32_t *out_sample_x,
                                                   uint32_t *out_sample_y) {
    if (!ctx || !out_sample_x || !out_sample_y) {
        return 0;
    }
    return drawing_program_texture_workspace_screen_to_active_sample(
        ctx, pane_rect, sx, sy, out_sample_x, out_sample_y);
}

int drawing_program_visual_screen_to_canvas_sample_clamped(const DrawingProgramAppContext *ctx,
                                                           SDL_Rect pane_rect,
                                                           int sx,
                                                           int sy,
                                                           uint32_t *out_sample_x,
                                                           uint32_t *out_sample_y) {
    if (!ctx || !out_sample_x || !out_sample_y || ctx->document.raster_width == 0u || ctx->document.raster_height == 0u) {
        return 0;
    }
    return drawing_program_texture_workspace_screen_to_active_sample_clamped(
        ctx, pane_rect, sx, sy, out_sample_x, out_sample_y);
}
