#include "drawing_program/drawing_program_visual_overlay_shared.h"

#include <math.h>

int drawing_program_visual_overlay_sample_rect_to_screen_rect(const VisualCanvasSheetMetrics *metrics,
                                                              int32_t sample_x,
                                                              int32_t sample_y,
                                                              uint32_t width,
                                                              uint32_t height,
                                                              SDL_Rect *out_rect) {
    double start_x;
    double start_y;
    double end_x;
    double end_y;
    int x;
    int y;
    int w;
    int h;
    if (!metrics || !out_rect || width == 0u || height == 0u) {
        return 0;
    }
    start_x = (double)metrics->sheet_rect.x + ((double)sample_x * (double)metrics->pixel_size);
    start_y = (double)metrics->sheet_rect.y + ((double)sample_y * (double)metrics->pixel_size);
    end_x = (double)metrics->sheet_rect.x + ((double)(sample_x + (int32_t)width) * (double)metrics->pixel_size);
    end_y = (double)metrics->sheet_rect.y + ((double)(sample_y + (int32_t)height) * (double)metrics->pixel_size);
    x = (int)floor(start_x);
    y = (int)floor(start_y);
    w = (int)floor(end_x) - x;
    h = (int)floor(end_y) - y;
    if (w < 1) {
        w = 1;
    }
    if (h < 1) {
        h = 1;
    }
    out_rect->x = x;
    out_rect->y = y;
    out_rect->w = w;
    out_rect->h = h;
    return 1;
}
