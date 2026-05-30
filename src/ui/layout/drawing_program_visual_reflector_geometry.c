#include "drawing_program/drawing_program_visual_reflector_geometry.h"

#include <math.h>
#include <stdlib.h>

static int append_unique_point(SDL_Point *points, int *point_count, int x, int y) {
    int i;
    if (!points || !point_count) {
        return 0;
    }
    for (i = 0; i < *point_count; ++i) {
        if (abs(points[i].x - x) <= 1 && abs(points[i].y - y) <= 1) {
            return 0;
        }
    }
    if (*point_count >= 4) {
        return 0;
    }
    points[*point_count].x = x;
    points[*point_count].y = y;
    *point_count += 1;
    return 1;
}

int drawing_program_visual_reflector_screen_line(const VisualCanvasSheetMetrics *metrics,
                                                 const DrawingProgramReflectorLine *line,
                                                 SDL_Point *out_start,
                                                 SDL_Point *out_end) {
    SDL_Point points[4];
    int point_count = 0;
    double anchor_x;
    double anchor_y;
    double dir_x;
    double dir_y;
    double left;
    double right;
    double top;
    double bottom;
    if (!metrics || !line || !out_start || !out_end || metrics->sheet_rect.w <= 0 || metrics->sheet_rect.h <= 0 ||
        (line->direction_dx == 0 && line->direction_dy == 0)) {
        return 0;
    }
    anchor_x = (double)metrics->sheet_rect.x + (((double)line->anchor_x + 0.5) * (double)metrics->pixel_size);
    anchor_y = (double)metrics->sheet_rect.y + (((double)line->anchor_y + 0.5) * (double)metrics->pixel_size);
    dir_x = (double)line->direction_dx * (double)metrics->pixel_size;
    dir_y = (double)line->direction_dy * (double)metrics->pixel_size;
    left = (double)metrics->sheet_rect.x;
    right = (double)(metrics->sheet_rect.x + metrics->sheet_rect.w - 1);
    top = (double)metrics->sheet_rect.y;
    bottom = (double)(metrics->sheet_rect.y + metrics->sheet_rect.h - 1);
    if (fabs(dir_x) > 0.0001) {
        double y_left = anchor_y + (((left - anchor_x) / dir_x) * dir_y);
        double y_right = anchor_y + (((right - anchor_x) / dir_x) * dir_y);
        if (y_left >= top && y_left <= bottom) {
            append_unique_point(points, &point_count, (int)lround(left), (int)lround(y_left));
        }
        if (y_right >= top && y_right <= bottom) {
            append_unique_point(points, &point_count, (int)lround(right), (int)lround(y_right));
        }
    }
    if (fabs(dir_y) > 0.0001) {
        double x_top = anchor_x + (((top - anchor_y) / dir_y) * dir_x);
        double x_bottom = anchor_x + (((bottom - anchor_y) / dir_y) * dir_x);
        if (x_top >= left && x_top <= right) {
            append_unique_point(points, &point_count, (int)lround(x_top), (int)lround(top));
        }
        if (x_bottom >= left && x_bottom <= right) {
            append_unique_point(points, &point_count, (int)lround(x_bottom), (int)lround(bottom));
        }
    }
    if (point_count < 2) {
        return 0;
    }
    *out_start = points[0];
    *out_end = points[1];
    return 1;
}

int drawing_program_visual_reflector_screen_handles(const VisualCanvasSheetMetrics *metrics,
                                                    const DrawingProgramReflectorLine *line,
                                                    SDL_Point *out_anchor,
                                                    SDL_Point *out_direction) {
    double anchor_x;
    double anchor_y;
    double dir_x;
    double dir_y;
    double length;
    double handle_radius;
    if (!metrics || !line || !out_anchor || !out_direction || (line->direction_dx == 0 && line->direction_dy == 0)) {
        return 0;
    }
    anchor_x = (double)metrics->sheet_rect.x + (((double)line->anchor_x + 0.5) * (double)metrics->pixel_size);
    anchor_y = (double)metrics->sheet_rect.y + (((double)line->anchor_y + 0.5) * (double)metrics->pixel_size);
    dir_x = (double)line->direction_dx;
    dir_y = (double)line->direction_dy;
    length = sqrt((dir_x * dir_x) + (dir_y * dir_y));
    if (length <= 0.0) {
        return 0;
    }
    handle_radius = ((double)metrics->pixel_size * 7.0);
    if (handle_radius < 20.0) {
        handle_radius = 20.0;
    } else if (handle_radius > 56.0) {
        handle_radius = 56.0;
    }
    out_anchor->x = (int)lround(anchor_x);
    out_anchor->y = (int)lround(anchor_y);
    out_direction->x = (int)lround(anchor_x + ((dir_x / length) * handle_radius));
    out_direction->y = (int)lround(anchor_y + ((dir_y / length) * handle_radius));
    return 1;
}
