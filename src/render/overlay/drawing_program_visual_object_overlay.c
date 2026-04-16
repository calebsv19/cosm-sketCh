#include "drawing_program/drawing_program_visual_overlay_render.h"

#include <math.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_overlay_shared.h"
#include "drawing_program/drawing_program_visual_theme.h"

static int object_style_includes_fill(uint8_t style_mode) {
    return (style_mode == 1u || style_mode == 2u) ? 1 : 0;
}

static int object_style_includes_outline(uint8_t style_mode) {
    return (style_mode == 1u) ? 0 : 1;
}

static int object_selection_contains(const DrawingProgramAppContext *ctx, uint32_t object_id) {
    if (!ctx || object_id == 0u) {
        return 0;
    }
    return drawing_program_object_selection_contains(&ctx->object_selection, object_id);
}

static int object_overlay_screen_to_canvas_sample(const DrawingProgramAppContext *ctx,
                                                  const VisualCanvasSheetMetrics *metrics,
                                                  int screen_x,
                                                  int screen_y,
                                                  uint32_t *out_sample_x,
                                                  uint32_t *out_sample_y) {
    int rel_x;
    int rel_y;
    int sample_x;
    int sample_y;
    if (!ctx || !metrics || !out_sample_x || !out_sample_y || metrics->pixel_size <= 0.0f) {
        return 0;
    }
    if (screen_x < metrics->sheet_rect.x || screen_y < metrics->sheet_rect.y ||
        screen_x >= (metrics->sheet_rect.x + metrics->sheet_rect.w) ||
        screen_y >= (metrics->sheet_rect.y + metrics->sheet_rect.h)) {
        return 0;
    }
    rel_x = screen_x - metrics->sheet_rect.x;
    rel_y = screen_y - metrics->sheet_rect.y;
    sample_x = (int)((float)rel_x / metrics->pixel_size);
    sample_y = (int)((float)rel_y / metrics->pixel_size);
    if (sample_x < 0 || sample_y < 0 || sample_x >= (int)ctx->document.raster_width ||
        sample_y >= (int)ctx->document.raster_height) {
        return 0;
    }
    *out_sample_x = (uint32_t)sample_x;
    *out_sample_y = (uint32_t)sample_y;
    return 1;
}

static int draw_object_sample_cell(SDL_Renderer *renderer,
                                   const VisualCanvasSheetMetrics *metrics,
                                   int32_t sample_x,
                                   int32_t sample_y,
                                   SDL_Color color,
                                   uint8_t alpha) {
    SDL_Rect sample_rect;
    if (!renderer || !metrics) {
        return 0;
    }
    if (!drawing_program_visual_overlay_sample_rect_to_screen_rect(metrics, sample_x, sample_y, 1u, 1u, &sample_rect)) {
        return 0;
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
    (void)SDL_RenderFillRect(renderer, &sample_rect);
    return 1;
}

static void draw_object_rect(SDL_Renderer *renderer,
                             const DrawingProgramAppContext *ctx,
                             const VisualCanvasSheetMetrics *metrics,
                             const DrawingProgramObjectRecord *object,
                             int32_t origin_x,
                             int32_t origin_y,
                             uint8_t alpha) {
    uint32_t x;
    uint32_t y;
    uint32_t stroke_width = object->stroke_width == 0u ? 1u : (uint32_t)object->stroke_width;
    int has_fill = object_style_includes_fill(object->style_mode);
    int has_outline = object_style_includes_outline(object->style_mode);
    SDL_Color stroke_color = { 0u, 0u, 0u, 255u };
    SDL_Color fill_color = { 0u, 0u, 0u, 255u };
    if (!renderer || !ctx || !metrics || !object || object->width == 0u || object->height == 0u) {
        return;
    }
    (void)drawing_program_color_rgb_from_index(drawing_program_color_index_clamp(object->stroke_color_index),
                                               &stroke_color.r,
                                               &stroke_color.g,
                                               &stroke_color.b);
    (void)drawing_program_color_rgb_from_index(drawing_program_color_index_clamp(object->fill_color_index),
                                               &fill_color.r,
                                               &fill_color.g,
                                               &fill_color.b);
    for (y = 0u; y < object->height; ++y) {
        for (x = 0u; x < object->width; ++x) {
            int32_t sample_x = origin_x + (int32_t)x;
            int32_t sample_y = origin_y + (int32_t)y;
            int on_outline = 0;
            if (sample_x < 0 || sample_y < 0 ||
                sample_x >= (int32_t)ctx->document.raster_width ||
                sample_y >= (int32_t)ctx->document.raster_height) {
                continue;
            }
            if (has_outline) {
                on_outline = (x < stroke_width || y < stroke_width || x >= (object->width - stroke_width) ||
                              y >= (object->height - stroke_width))
                                 ? 1
                                 : 0;
            }
            if (on_outline) {
                (void)draw_object_sample_cell(renderer, metrics, sample_x, sample_y, stroke_color, alpha);
            } else if (has_fill) {
                (void)draw_object_sample_cell(renderer, metrics, sample_x, sample_y, fill_color, alpha);
            }
        }
    }
}

static void draw_object_ellipse(SDL_Renderer *renderer,
                                const DrawingProgramAppContext *ctx,
                                const VisualCanvasSheetMetrics *metrics,
                                const DrawingProgramObjectRecord *object,
                                int32_t origin_x,
                                int32_t origin_y,
                                uint8_t alpha) {
    int32_t min_x;
    int32_t min_y;
    int32_t max_x;
    int32_t max_y;
    int32_t x;
    int32_t y;
    uint32_t stroke_width = object->stroke_width == 0u ? 1u : (uint32_t)object->stroke_width;
    int has_fill = object_style_includes_fill(object->style_mode);
    int has_outline = object_style_includes_outline(object->style_mode);
    SDL_Color stroke_color = { 0u, 0u, 0u, 255u };
    SDL_Color fill_color = { 0u, 0u, 0u, 255u };
    double rx;
    double ry;
    double inner_rx;
    double inner_ry;
    double cx;
    double cy;
    if (!renderer || !ctx || !metrics || !object || object->width == 0u || object->height == 0u) {
        return;
    }
    (void)drawing_program_color_rgb_from_index(drawing_program_color_index_clamp(object->stroke_color_index),
                                               &stroke_color.r,
                                               &stroke_color.g,
                                               &stroke_color.b);
    (void)drawing_program_color_rgb_from_index(drawing_program_color_index_clamp(object->fill_color_index),
                                               &fill_color.r,
                                               &fill_color.g,
                                               &fill_color.b);
    rx = ((double)object->width) * 0.5;
    ry = ((double)object->height) * 0.5;
    if (rx <= 0.0 || ry <= 0.0) {
        return;
    }
    inner_rx = rx - (double)stroke_width;
    inner_ry = ry - (double)stroke_width;
    if (inner_rx < 0.0) {
        inner_rx = 0.0;
    }
    if (inner_ry < 0.0) {
        inner_ry = 0.0;
    }
    cx = (double)origin_x + rx;
    cy = (double)origin_y + ry;
    min_x = origin_x;
    min_y = origin_y;
    max_x = origin_x + (int32_t)object->width - 1;
    max_y = origin_y + (int32_t)object->height - 1;
    for (y = min_y; y <= max_y; ++y) {
        for (x = min_x; x <= max_x; ++x) {
            double dx;
            double dy;
            double outer_norm;
            int inside_outer;
            int inside_inner = 0;
            int on_outline = 0;
            if (x < 0 || y < 0 || x >= (int32_t)ctx->document.raster_width || y >= (int32_t)ctx->document.raster_height) {
                continue;
            }
            dx = ((double)x + 0.5) - cx;
            dy = ((double)y + 0.5) - cy;
            outer_norm = ((dx * dx) / (rx * rx)) + ((dy * dy) / (ry * ry));
            inside_outer = (outer_norm <= 1.0) ? 1 : 0;
            if (!inside_outer) {
                continue;
            }
            if (has_outline && inner_rx > 0.0 && inner_ry > 0.0) {
                double inner_norm = ((dx * dx) / (inner_rx * inner_rx)) + ((dy * dy) / (inner_ry * inner_ry));
                inside_inner = (inner_norm <= 1.0) ? 1 : 0;
            }
            on_outline = has_outline && !inside_inner;
            if (on_outline) {
                (void)draw_object_sample_cell(renderer, metrics, x, y, stroke_color, alpha);
            } else if (has_fill) {
                (void)draw_object_sample_cell(renderer, metrics, x, y, fill_color, alpha);
            }
        }
    }
}

static double path_segment_distance_sq(double px,
                                       double py,
                                       double x0,
                                       double y0,
                                       double x1,
                                       double y1) {
    double dx = x1 - x0;
    double dy = y1 - y0;
    double len_sq = (dx * dx) + (dy * dy);
    double t;
    double rx;
    double ry;
    if (len_sq <= 0.0) {
        rx = px - x0;
        ry = py - y0;
        return (rx * rx) + (ry * ry);
    }
    t = (((px - x0) * dx) + ((py - y0) * dy)) / len_sq;
    if (t < 0.0) {
        t = 0.0;
    } else if (t > 1.0) {
        t = 1.0;
    }
    rx = px - (x0 + (t * dx));
    ry = py - (y0 + (t * dy));
    return (rx * rx) + (ry * ry);
}

static int draw_object_path_contains_fill(const DrawingProgramObjectRecord *object,
                                          int32_t delta_x,
                                          int32_t delta_y,
                                          double px,
                                          double py) {
    uint32_t i;
    uint32_t j;
    int inside = 0;
    uint32_t point_count;
    if (!object || !object->path_closed) {
        return 0;
    }
    point_count = (uint32_t)object->path_point_count;
    if (point_count < 3u || point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
        return 0;
    }
    for (i = 0u, j = point_count - 1u; i < point_count; j = i++) {
        const double xi = (double)(object->path_points[i].x + delta_x) + 0.5;
        const double yi = (double)(object->path_points[i].y + delta_y) + 0.5;
        const double xj = (double)(object->path_points[j].x + delta_x) + 0.5;
        const double yj = (double)(object->path_points[j].y + delta_y) + 0.5;
        const int y_cross = ((yi > py) != (yj > py));
        if (!y_cross) {
            continue;
        }
        if (fabs(yj - yi) < 1e-6) {
            continue;
        }
        if (px < (((xj - xi) * (py - yi)) / (yj - yi)) + xi) {
            inside = !inside;
        }
    }
    return inside;
}

static int draw_object_path_contains_outline(const DrawingProgramObjectRecord *object,
                                             int32_t delta_x,
                                             int32_t delta_y,
                                             double px,
                                             double py) {
    uint32_t point_count;
    uint32_t segment_count;
    uint32_t i;
    double half_width;
    double threshold_sq;
    if (!object) {
        return 0;
    }
    point_count = (uint32_t)object->path_point_count;
    if (point_count < 2u || point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
        return 0;
    }
    segment_count = point_count - 1u;
    if (object->path_closed) {
        segment_count += 1u;
    }
    if (segment_count == 0u) {
        return 0;
    }
    half_width = ((double)(object->stroke_width == 0u ? 1u : object->stroke_width)) * 0.5;
    if (half_width < 0.6) {
        half_width = 0.6;
    }
    threshold_sq = half_width * half_width;
    for (i = 0u; i < segment_count; ++i) {
        uint32_t a = i;
        uint32_t b = (i + 1u < point_count) ? (i + 1u) : 0u;
        double d_sq = path_segment_distance_sq(px,
                                               py,
                                               (double)(object->path_points[a].x + delta_x) + 0.5,
                                               (double)(object->path_points[a].y + delta_y) + 0.5,
                                               (double)(object->path_points[b].x + delta_x) + 0.5,
                                               (double)(object->path_points[b].y + delta_y) + 0.5);
        if (d_sq <= threshold_sq) {
            return 1;
        }
    }
    return 0;
}

static void draw_object_path(SDL_Renderer *renderer,
                             const DrawingProgramAppContext *ctx,
                             const VisualCanvasSheetMetrics *metrics,
                             const DrawingProgramObjectRecord *object,
                             int32_t origin_x,
                             int32_t origin_y,
                             uint8_t alpha) {
    int32_t min_x;
    int32_t min_y;
    int32_t max_x;
    int32_t max_y;
    int32_t x;
    int32_t y;
    int32_t delta_x;
    int32_t delta_y;
    int has_fill;
    int has_outline;
    SDL_Color stroke_color = { 0u, 0u, 0u, 255u };
    SDL_Color fill_color = { 0u, 0u, 0u, 255u };
    if (!renderer || !ctx || !metrics || !object || object->path_point_count < 2u) {
        return;
    }
    has_fill = object_style_includes_fill(object->style_mode) ? 1 : 0;
    has_outline = object_style_includes_outline(object->style_mode) ? 1 : 0;
    delta_x = origin_x - object->origin_x;
    delta_y = origin_y - object->origin_y;
    min_x = origin_x;
    min_y = origin_y;
    max_x = origin_x + (int32_t)object->width - 1;
    max_y = origin_y + (int32_t)object->height - 1;
    (void)drawing_program_color_rgb_from_index(drawing_program_color_index_clamp(object->stroke_color_index),
                                               &stroke_color.r,
                                               &stroke_color.g,
                                               &stroke_color.b);
    (void)drawing_program_color_rgb_from_index(drawing_program_color_index_clamp(object->fill_color_index),
                                               &fill_color.r,
                                               &fill_color.g,
                                               &fill_color.b);
    for (y = min_y; y <= max_y; ++y) {
        for (x = min_x; x <= max_x; ++x) {
            double px;
            double py;
            int on_outline = 0;
            int inside_fill = 0;
            if (x < 0 || y < 0 || x >= (int32_t)ctx->document.raster_width || y >= (int32_t)ctx->document.raster_height) {
                continue;
            }
            px = (double)x + 0.5;
            py = (double)y + 0.5;
            if (has_outline) {
                on_outline = draw_object_path_contains_outline(object, delta_x, delta_y, px, py);
            }
            if (has_fill) {
                inside_fill = draw_object_path_contains_fill(object, delta_x, delta_y, px, py);
            }
            if (on_outline) {
                (void)draw_object_sample_cell(renderer, metrics, x, y, stroke_color, alpha);
            } else if (inside_fill) {
                (void)draw_object_sample_cell(renderer, metrics, x, y, fill_color, alpha);
            }
        }
    }
}

static void draw_selected_path_point_handles(SDL_Renderer *renderer,
                                             const DrawingProgramAppContext *ctx,
                                             const VisualCanvasSheetMetrics *metrics,
                                             const DrawingProgramObjectRecord *object,
                                             const VisualCanvasInteractionState *interaction,
                                             int show_handles) {
    uint32_t i;
    if (!renderer || !ctx || !metrics || !object || !show_handles ||
        object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
        object->path_point_count == 0u) {
        return;
    }
    for (i = 0u; i < (uint32_t)object->path_point_count; ++i) {
        int32_t sample_x = object->path_points[i].x;
        int32_t sample_y = object->path_points[i].y;
        SDL_Rect point_rect;
        SDL_Rect point_inner;
        SDL_Color border_color = { 40u, 112u, 255u, 255u };
        SDL_Color fill_color = { 236u, 246u, 255u, 255u };
        SDL_Color cross_color = { 24u, 86u, 214u, 255u };
        SDL_Rect cross_h;
        SDL_Rect cross_v;
        int min_handle_size = 10;
        if (interaction &&
            interaction->object_move_active &&
            !interaction->object_path_point_move_active) {
            sample_x += interaction->object_move_offset_x;
            sample_y += interaction->object_move_offset_y;
        }
        if (interaction &&
            interaction->object_path_point_move_active &&
            interaction->object_path_point_object_id == object->object_id &&
            interaction->object_path_point_index == i) {
            sample_x += interaction->object_path_point_offset_x;
            sample_y += interaction->object_path_point_offset_y;
            border_color = (SDL_Color){ 255u, 196u, 84u, 255u };
            fill_color = (SDL_Color){ 255u, 228u, 166u, 255u };
            cross_color = (SDL_Color){ 212u, 128u, 28u, 255u };
            min_handle_size = 12;
        }
        if (!drawing_program_visual_overlay_sample_rect_to_screen_rect(metrics, sample_x, sample_y, 1u, 1u, &point_rect)) {
            continue;
        }
        if (point_rect.w < min_handle_size) {
            int grow_x = (min_handle_size - point_rect.w + 1) / 2;
            point_rect.x -= grow_x;
            point_rect.w += grow_x * 2;
        }
        if (point_rect.h < min_handle_size) {
            int grow_y = (min_handle_size - point_rect.h + 1) / 2;
            point_rect.y -= grow_y;
            point_rect.h += grow_y * 2;
        }
        point_inner = point_rect;
        if (point_inner.w > 2) {
            point_inner.x += 1;
            point_inner.w -= 2;
        }
        if (point_inner.h > 2) {
            point_inner.y += 1;
            point_inner.h -= 2;
        }
        if (point_rect.w > 2 && point_rect.h > 2) {
            SDL_Rect outer = { point_rect.x - 1, point_rect.y - 1, point_rect.w + 2, point_rect.h + 2 };
            SDL_SetRenderDrawColor(renderer, 16u, 26u, 52u, 220u);
            (void)SDL_RenderDrawRect(renderer, &outer);
        }
        SDL_SetRenderDrawColor(renderer, fill_color.r, fill_color.g, fill_color.b, 230u);
        (void)SDL_RenderFillRect(renderer, &point_inner);
        SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, border_color.b, 255u);
        (void)SDL_RenderDrawRect(renderer, &point_rect);
        cross_h = (SDL_Rect){ point_rect.x + 2, point_rect.y + (point_rect.h / 2), point_rect.w - 4, 1 };
        cross_v = (SDL_Rect){ point_rect.x + (point_rect.w / 2), point_rect.y + 2, 1, point_rect.h - 4 };
        if (cross_h.w > 0 && cross_h.h > 0) {
            SDL_SetRenderDrawColor(renderer, cross_color.r, cross_color.g, cross_color.b, 230u);
            (void)SDL_RenderFillRect(renderer, &cross_h);
        }
        if (cross_v.w > 0 && cross_v.h > 0) {
            SDL_SetRenderDrawColor(renderer, cross_color.r, cross_color.g, cross_color.b, 230u);
            (void)SDL_RenderFillRect(renderer, &cross_v);
        }
    }
}

static int path_point_screen_center(const VisualCanvasSheetMetrics *metrics,
                                    int32_t sample_x,
                                    int32_t sample_y,
                                    int *out_x,
                                    int *out_y) {
    SDL_Rect sample_rect;
    if (!metrics || !out_x || !out_y) {
        return 0;
    }
    if (!drawing_program_visual_overlay_sample_rect_to_screen_rect(metrics, sample_x, sample_y, 1u, 1u, &sample_rect)) {
        return 0;
    }
    *out_x = sample_rect.x + (sample_rect.w / 2);
    *out_y = sample_rect.y + (sample_rect.h / 2);
    return 1;
}

static void draw_active_path_point_edge_preview(SDL_Renderer *renderer,
                                                const VisualCanvasSheetMetrics *metrics,
                                                const DrawingProgramObjectRecord *object,
                                                const VisualCanvasInteractionState *interaction,
                                                int32_t draw_origin_x,
                                                int32_t draw_origin_y) {
    uint32_t point_index;
    int32_t delta_x;
    int32_t delta_y;
    int32_t moved_x;
    int32_t moved_y;
    int curr_x = 0;
    int curr_y = 0;
    SDL_Color hint = { 88u, 148u, 255u, 255u };
    SDL_Color shadow = { 10u, 20u, 46u, 255u };
    uint32_t prev_index = UINT32_MAX;
    uint32_t next_index = UINT32_MAX;
    if (!renderer || !metrics || !object || !interaction ||
        object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
        object->path_point_count < 2u ||
        !interaction->object_path_point_move_active ||
        interaction->object_path_point_object_id != object->object_id) {
        return;
    }
    point_index = interaction->object_path_point_index;
    if (point_index >= object->path_point_count) {
        return;
    }
    delta_x = draw_origin_x - object->origin_x;
    delta_y = draw_origin_y - object->origin_y;
    moved_x = object->path_points[point_index].x + delta_x + interaction->object_path_point_offset_x;
    moved_y = object->path_points[point_index].y + delta_y + interaction->object_path_point_offset_y;
    if (!path_point_screen_center(metrics, moved_x, moved_y, &curr_x, &curr_y)) {
        return;
    }
    if (point_index > 0u) {
        prev_index = point_index - 1u;
    } else if (object->path_closed && object->path_point_count > 2u) {
        prev_index = (uint32_t)object->path_point_count - 1u;
    }
    if ((point_index + 1u) < object->path_point_count) {
        next_index = point_index + 1u;
    } else if (object->path_closed && object->path_point_count > 2u) {
        next_index = 0u;
    }
    if (prev_index != UINT32_MAX) {
        int px = 0;
        int py = 0;
        int32_t sx = object->path_points[prev_index].x + delta_x;
        int32_t sy = object->path_points[prev_index].y + delta_y;
        if (path_point_screen_center(metrics, sx, sy, &px, &py)) {
            SDL_SetRenderDrawColor(renderer, shadow.r, shadow.g, shadow.b, 220u);
            (void)SDL_RenderDrawLine(renderer, curr_x + 1, curr_y + 1, px + 1, py + 1);
            SDL_SetRenderDrawColor(renderer, hint.r, hint.g, hint.b, 245u);
            (void)SDL_RenderDrawLine(renderer, curr_x, curr_y, px, py);
        }
    }
    if (next_index != UINT32_MAX) {
        int nx = 0;
        int ny = 0;
        int32_t sx = object->path_points[next_index].x + delta_x;
        int32_t sy = object->path_points[next_index].y + delta_y;
        if (path_point_screen_center(metrics, sx, sy, &nx, &ny)) {
            SDL_SetRenderDrawColor(renderer, shadow.r, shadow.g, shadow.b, 220u);
            (void)SDL_RenderDrawLine(renderer, curr_x + 1, curr_y + 1, nx + 1, ny + 1);
            SDL_SetRenderDrawColor(renderer, hint.r, hint.g, hint.b, 245u);
            (void)SDL_RenderDrawLine(renderer, curr_x, curr_y, nx, ny);
        }
    }
}

void drawing_program_visual_draw_object_overlay(SDL_Renderer *renderer,
                                                SDL_Rect pane_rect,
                                                const DrawingProgramAppContext *ctx,
                                                const CoreThemePreset *theme,
                                                const VisualCanvasSheetMetrics *metrics,
                                                const VisualCanvasInteractionState *interaction,
                                                const VisualPanelUiState *ui) {
    SDL_Color outline_color = { 116u, 126u, 148u, 255u };
    SDL_Color selected_outline = { 72u, 134u, 255u, 255u };
    SDL_Color hover_outline = { 128u, 176u, 255u, 255u };
    uint32_t visible_indices[DRAWING_PROGRAM_MAX_OBJECTS];
    uint32_t hover_object_id = 0u;
    uint32_t count;
    uint32_t i;
    if (!renderer || !ctx || !metrics) {
        return;
    }
    (void)theme;
    outline_color = sdl_color_shift_by_luma(selected_outline, -96);
    if (outline_color.a == 0u) {
        outline_color.a = 255u;
    }
    if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_SELECT && ui && ui->mouse_known) {
        uint32_t sample_x = 0u;
        uint32_t sample_y = 0u;
        if (object_overlay_screen_to_canvas_sample(ctx, metrics, ui->mouse_x, ui->mouse_y, &sample_x, &sample_y)) {
            (void)drawing_program_object_store_hit_test_topmost(
                &ctx->object_store, &ctx->document, sample_x, sample_y, &hover_object_id, 0);
        }
    }
    count = drawing_program_object_store_collect_visible_indices(
        &ctx->object_store, &ctx->document, visible_indices, DRAWING_PROGRAM_MAX_OBJECTS);
    if (count == 0u) {
        return;
    }
    (void)SDL_RenderSetClipRect(renderer, &pane_rect);
    for (i = 0u; i < count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        uint32_t index = visible_indices[i];
        const DrawingProgramObjectRecord *object = &ctx->object_store.objects[index];
        uint32_t layer_index = 0u;
        uint8_t layer_alpha_percent;
        uint8_t alpha;
        int selected;
        int hovered;
        int32_t draw_origin_x;
        int32_t draw_origin_y;
        SDL_Rect bounds_rect;
        if (drawing_program_document_layer_index_for_id(&ctx->document, object->layer_id, &layer_index).code != CORE_OK) {
            continue;
        }
        if (!ctx->document.layers[layer_index].visible) {
            continue;
        }
        layer_alpha_percent = drawing_program_visual_layer_opacity_get(ctx, object->layer_id);
        if (layer_alpha_percent == 0u) {
            continue;
        }
        alpha = (uint8_t)(((uint32_t)layer_alpha_percent * 255u) / 100u);
        selected = object_selection_contains(ctx, object->object_id);
        hovered = (hover_object_id == object->object_id) ? 1 : 0;
        draw_origin_x = object->origin_x;
        draw_origin_y = object->origin_y;
        if (selected && interaction && interaction->object_move_active) {
            draw_origin_x += interaction->object_move_offset_x;
            draw_origin_y += interaction->object_move_offset_y;
        }
        if (object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_ELLIPSE) {
            draw_object_ellipse(renderer, ctx, metrics, object, draw_origin_x, draw_origin_y, alpha);
        } else if (object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
            draw_object_path(renderer, ctx, metrics, object, draw_origin_x, draw_origin_y, alpha);
            draw_active_path_point_edge_preview(
                renderer, metrics, object, interaction, draw_origin_x, draw_origin_y);
        } else {
            draw_object_rect(renderer, ctx, metrics, object, draw_origin_x, draw_origin_y, alpha);
        }
        draw_selected_path_point_handles(renderer,
                                         ctx,
                                         metrics,
                                         object,
                                         interaction,
                                         selected || (hovered && ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE));
        if (drawing_program_visual_overlay_sample_rect_to_screen_rect(
                metrics, draw_origin_x, draw_origin_y, object->width, object->height, &bounds_rect)) {
            SDL_Color border_color = selected ? selected_outline : outline_color;
            uint8_t border_alpha = selected ? 255u : 190u;
            if (hovered && !selected) {
                border_color = selected_outline;
                border_alpha = 210u;
            }
            if (selected) {
                SDL_Rect outer2 = { bounds_rect.x - 2, bounds_rect.y - 2, bounds_rect.w + 4, bounds_rect.h + 4 };
                SDL_Rect outer1 = { bounds_rect.x - 1, bounds_rect.y - 1, bounds_rect.w + 2, bounds_rect.h + 2 };
                SDL_Color shadow = { 18u, 32u, 66u, 255u };
                SDL_SetRenderDrawColor(renderer, shadow.r, shadow.g, shadow.b, 220u);
                (void)SDL_RenderDrawRect(renderer, &outer2);
                (void)SDL_RenderDrawRect(renderer, &outer1);
                SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, border_color.b, 255u);
                (void)SDL_RenderDrawRect(renderer, &bounds_rect);
                if (bounds_rect.w > 2 && bounds_rect.h > 2) {
                    SDL_Rect inner1 = { bounds_rect.x + 1, bounds_rect.y + 1, bounds_rect.w - 2, bounds_rect.h - 2 };
                    SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, border_color.b, 220u);
                    (void)SDL_RenderDrawRect(renderer, &inner1);
                }
                if (bounds_rect.w > 4 && bounds_rect.h > 4) {
                    SDL_Rect inner2 = { bounds_rect.x + 2, bounds_rect.y + 2, bounds_rect.w - 4, bounds_rect.h - 4 };
                    SDL_SetRenderDrawColor(renderer, shadow.r, shadow.g, shadow.b, 170u);
                    (void)SDL_RenderDrawRect(renderer, &inner2);
                }
            } else {
                SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, border_color.b, border_alpha);
                (void)SDL_RenderDrawRect(renderer, &bounds_rect);
            }
            if (hovered && !selected && bounds_rect.w > 2 && bounds_rect.h > 2) {
                SDL_Rect hover_outer = { bounds_rect.x - 1, bounds_rect.y - 1, bounds_rect.w + 2, bounds_rect.h + 2 };
                SDL_SetRenderDrawColor(renderer, hover_outline.r, hover_outline.g, hover_outline.b, 170u);
                (void)SDL_RenderDrawRect(renderer, &hover_outer);
            }
        }
    }
    (void)SDL_RenderSetClipRect(renderer, 0);
}
