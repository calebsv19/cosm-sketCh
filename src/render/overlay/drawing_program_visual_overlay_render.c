#include "drawing_program/drawing_program_visual_overlay_render.h"

#include <math.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_object_geometry.h"
#include "drawing_program/drawing_program_visual_overlay_shared.h"
#include "drawing_program/drawing_program_visual_theme.h"

static int sample_center_to_screen(const VisualCanvasSheetMetrics *metrics,
                                   const DrawingProgramAppContext *ctx,
                                   uint32_t sample_x,
                                   uint32_t sample_y,
                                   int *out_x,
                                   int *out_y) {
    float fx;
    float fy;
    if (!metrics || !ctx || !out_x || !out_y) {
        return 0;
    }
    if (sample_x >= ctx->document.raster_width || sample_y >= ctx->document.raster_height) {
        return 0;
    }
    fx = (float)metrics->sheet_rect.x + (((float)sample_x + 0.5f) * metrics->pixel_size);
    fy = (float)metrics->sheet_rect.y + (((float)sample_y + 0.5f) * metrics->pixel_size);
    *out_x = (int)fx;
    *out_y = (int)fy;
    return 1;
}

static int sample_point_center_to_screen(const VisualCanvasSheetMetrics *metrics,
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

static int preview_stroke_width_pixels(const VisualCanvasSheetMetrics *metrics, uint32_t stroke_width_samples) {
    int stroke_px;
    if (stroke_width_samples < 1u) {
        stroke_width_samples = 1u;
    }
    if (!metrics || metrics->pixel_size <= 0.0f) {
        return (int)stroke_width_samples;
    }
    stroke_px = (int)lroundf((float)stroke_width_samples * metrics->pixel_size);
    if (stroke_px < 1) {
        stroke_px = 1;
    }
    if (stroke_px > 128) {
        stroke_px = 128;
    }
    return stroke_px;
}

static void draw_preview_thick_line(SDL_Renderer *renderer,
                                    int x0,
                                    int y0,
                                    int x1,
                                    int y1,
                                    int stroke_px,
                                    SDL_Color color,
                                    uint8_t alpha) {
    float dx;
    float dy;
    float len;
    float nx;
    float ny;
    int i;
    int start;
    if (!renderer) {
        return;
    }
    if (stroke_px < 1) {
        stroke_px = 1;
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
    if (stroke_px == 1) {
        (void)SDL_RenderDrawLine(renderer, x0, y0, x1, y1);
        return;
    }
    dx = (float)(x1 - x0);
    dy = (float)(y1 - y0);
    len = sqrtf((dx * dx) + (dy * dy));
    if (len <= 0.0001f) {
        SDL_Rect dot = { x0 - (stroke_px / 2), y0 - (stroke_px / 2), stroke_px, stroke_px };
        (void)SDL_RenderFillRect(renderer, &dot);
        return;
    }
    nx = -dy / len;
    ny = dx / len;
    start = -(stroke_px / 2);
    for (i = 0; i < stroke_px; ++i) {
        float off = (float)(start + i);
        int ox = (int)lroundf(nx * off);
        int oy = (int)lroundf(ny * off);
        (void)SDL_RenderDrawLine(renderer, x0 + ox, y0 + oy, x1 + ox, y1 + oy);
    }
}

static void draw_preview_thick_rect_outline(SDL_Renderer *renderer,
                                            SDL_Rect rect,
                                            int stroke_px,
                                            SDL_Color color,
                                            uint8_t alpha) {
    int i;
    if (!renderer) {
        return;
    }
    if (stroke_px < 1) {
        stroke_px = 1;
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
    for (i = 0; i < stroke_px; ++i) {
        SDL_Rect ring = { rect.x + i, rect.y + i, rect.w - (2 * i), rect.h - (2 * i) };
        if (ring.w <= 0 || ring.h <= 0) {
            break;
        }
        (void)SDL_RenderDrawRect(renderer, &ring);
    }
}

static void draw_preview_circle_fill(SDL_Renderer *renderer,
                                     int center_x,
                                     int center_y,
                                     int radius_px,
                                     SDL_Color color,
                                     uint8_t alpha) {
    int y;
    if (!renderer) {
        return;
    }
    if (radius_px < 1) {
        radius_px = 1;
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
    for (y = -radius_px; y <= radius_px; ++y) {
        float row = (float)((radius_px * radius_px) - (y * y));
        int span = (row > 0.0f) ? (int)floorf(sqrtf(row)) : 0;
        (void)SDL_RenderDrawLine(renderer, center_x - span, center_y + y, center_x + span, center_y + y);
    }
}

static void draw_preview_circle_outline(SDL_Renderer *renderer,
                                        int center_x,
                                        int center_y,
                                        int radius_px,
                                        int stroke_px,
                                        SDL_Color color,
                                        uint8_t alpha) {
    int s;
    const int segments = 64;
    const float two_pi = 6.28318530718f;
    if (!renderer) {
        return;
    }
    if (stroke_px < 1) {
        stroke_px = 1;
    }
    if (radius_px < 1) {
        radius_px = 1;
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
    for (s = 0; s < stroke_px; ++s) {
        float radius = (float)radius_px - (float)(stroke_px / 2) + (float)s;
        int i;
        if (radius < 1.0f) {
            continue;
        }
        for (i = 0; i < segments; ++i) {
            float t0 = ((float)i / (float)segments) * two_pi;
            float t1 = ((float)(i + 1) / (float)segments) * two_pi;
            int x0 = center_x + (int)(cosf(t0) * radius);
            int y0 = center_y + (int)(sinf(t0) * radius);
            int x1 = center_x + (int)(cosf(t1) * radius);
            int y1 = center_y + (int)(sinf(t1) * radius);
            (void)SDL_RenderDrawLine(renderer, x0, y0, x1, y1);
        }
    }
}

static void draw_preview_point_handle(SDL_Renderer *renderer,
                                      int center_x,
                                      int center_y,
                                      int half_size,
                                      SDL_Color fill_color,
                                      SDL_Color border_color,
                                      uint8_t alpha_fill,
                                      uint8_t alpha_border) {
    SDL_Rect r;
    if (!renderer) {
        return;
    }
    if (half_size < 2) {
        half_size = 2;
    }
    if (half_size > 10) {
        half_size = 10;
    }
    r.x = center_x - half_size;
    r.y = center_y - half_size;
    r.w = (half_size * 2) + 1;
    r.h = (half_size * 2) + 1;
    SDL_SetRenderDrawColor(renderer, fill_color.r, fill_color.g, fill_color.b, alpha_fill);
    (void)SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, border_color.b, alpha_border);
    (void)SDL_RenderDrawRect(renderer, &r);
}

static void draw_preview_bezier_handle_node(SDL_Renderer *renderer,
                                            const VisualCanvasSheetMetrics *metrics,
                                            int32_t anchor_sample_x,
                                            int32_t anchor_sample_y,
                                            int32_t handle_dx,
                                            int32_t handle_dy,
                                            SDL_Color line_color,
                                            SDL_Color fill_color,
                                            SDL_Color border_color,
                                            int half_size) {
    int anchor_sx = 0;
    int anchor_sy = 0;
    int handle_sx = 0;
    int handle_sy = 0;
    if (!renderer || !metrics) {
        return;
    }
    if (handle_dx == 0 && handle_dy == 0) {
        return;
    }
    if (!sample_point_center_to_screen(metrics, anchor_sample_x, anchor_sample_y, &anchor_sx, &anchor_sy) ||
        !sample_point_center_to_screen(
            metrics, anchor_sample_x + handle_dx, anchor_sample_y + handle_dy, &handle_sx, &handle_sy)) {
        return;
    }
    draw_preview_thick_line(renderer, anchor_sx, anchor_sy, handle_sx, handle_sy, 1, line_color, 220u);
    draw_preview_point_handle(renderer, handle_sx, handle_sy, half_size, fill_color, border_color, 240u, 255u);
}

static void draw_preview_bezier_handles(SDL_Renderer *renderer,
                                        const VisualCanvasSheetMetrics *metrics,
                                        const DrawingProgramPathPoint *point,
                                        int is_active_drag_point,
                                        int half_size) {
    SDL_Color linked_line = { 86u, 162u, 255u, 255u };
    SDL_Color linked_fill = { 236u, 246u, 255u, 255u };
    SDL_Color linked_border = { 40u, 112u, 255u, 255u };
    SDL_Color active_line = { 255u, 196u, 84u, 255u };
    SDL_Color active_fill = { 255u, 228u, 166u, 255u };
    SDL_Color active_border = { 212u, 128u, 28u, 255u };
    SDL_Color line = is_active_drag_point ? active_line : linked_line;
    SDL_Color fill = is_active_drag_point ? active_fill : linked_fill;
    SDL_Color border = is_active_drag_point ? active_border : linked_border;
    if (!renderer || !metrics || !point || !drawing_program_object_path_point_is_bezier_active(point)) {
        return;
    }
    draw_preview_bezier_handle_node(renderer,
                                    metrics,
                                    point->x,
                                    point->y,
                                    point->handle_in_dx,
                                    point->handle_in_dy,
                                    line,
                                    fill,
                                    border,
                                    half_size);
    draw_preview_bezier_handle_node(renderer,
                                    metrics,
                                    point->x,
                                    point->y,
                                    point->handle_out_dx,
                                    point->handle_out_dy,
                                    line,
                                    fill,
                                    border,
                                    half_size);
}

static void draw_preview_path_polyline(SDL_Renderer *renderer,
                                       const VisualCanvasSheetMetrics *metrics,
                                       const double *flat_xy,
                                       uint16_t flat_count,
                                       uint8_t closed,
                                       int stroke_px,
                                       SDL_Color color,
                                       uint8_t alpha) {
    uint32_t i;
    if (!renderer || !metrics || !flat_xy || flat_count < 2u) {
        return;
    }
    if (stroke_px > 3) {
        stroke_px = 3;
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
    for (i = 1u; i < flat_count; ++i) {
        int x0 = (int)lround((double)metrics->sheet_rect.x + (flat_xy[(i - 1u) * 2u] * (double)metrics->pixel_size));
        int y0 = (int)lround((double)metrics->sheet_rect.y + (flat_xy[((i - 1u) * 2u) + 1u] * (double)metrics->pixel_size));
        int x1 = (int)lround((double)metrics->sheet_rect.x + (flat_xy[i * 2u] * (double)metrics->pixel_size));
        int y1 = (int)lround((double)metrics->sheet_rect.y + (flat_xy[(i * 2u) + 1u] * (double)metrics->pixel_size));
        draw_preview_thick_line(renderer, x0, y0, x1, y1, stroke_px, color, alpha);
    }
    if (closed && flat_count > 2u) {
        int x0 = (int)lround((double)metrics->sheet_rect.x + (flat_xy[(flat_count - 1u) * 2u] * (double)metrics->pixel_size));
        int y0 = (int)lround((double)metrics->sheet_rect.y +
                             (flat_xy[((flat_count - 1u) * 2u) + 1u] * (double)metrics->pixel_size));
        int x1 = (int)lround((double)metrics->sheet_rect.x + (flat_xy[0] * (double)metrics->pixel_size));
        int y1 = (int)lround((double)metrics->sheet_rect.y + (flat_xy[1] * (double)metrics->pixel_size));
        draw_preview_thick_line(renderer, x0, y0, x1, y1, stroke_px, color, alpha);
    }
}

static void draw_preview_path_draft(SDL_Renderer *renderer,
                                    const DrawingProgramAppContext *ctx,
                                    const VisualCanvasSheetMetrics *metrics,
                                    const VisualCanvasInteractionState *interaction,
                                    const VisualPanelUiState *ui,
                                    const DrawingProgramVisualShapePreviewHooks *hooks,
                                    SDL_Color accent) {
    uint32_t i;
    uint16_t point_count;
    uint8_t active_color_index;
    int stroke_px;
    int handle_half = 4;
    SDL_Color point_fill = { 20u, 24u, 34u, 255u };
    SDL_Color preview_color = { 0u, 0u, 0u, 255u };
    if (!renderer || !ctx || !metrics || !interaction || !hooks) {
        return;
    }
    if (!interaction->path_draft_active || interaction->path_draft_point_count == 0u) {
        return;
    }
    point_count = interaction->path_draft_point_count;
    active_color_index = drawing_program_color_index_clamp(ctx->ui.active_color_index);
    drawing_program_color_rgb_from_index(
        active_color_index, &preview_color.r, &preview_color.g, &preview_color.b);
    stroke_px = preview_stroke_width_pixels(metrics, hooks->tool_shape_stroke_width(ctx));
    handle_half = (int)lroundf(metrics->pixel_size * 0.6f);
    if (handle_half < 3) {
        handle_half = 3;
    }
    if (handle_half > 8) {
        handle_half = 8;
    }
    if (interaction->path_preview_flatten_valid) {
        draw_preview_path_polyline(renderer,
                                   metrics,
                                   interaction->path_preview_flatten_xy,
                                   interaction->path_preview_flatten_point_count,
                                   interaction->path_draft_closed,
                                   stroke_px,
                                   preview_color,
                                   235u);
    }
    for (i = 0u; i < (uint32_t)point_count; ++i) {
        int cx = 0;
        int cy = 0;
        if (!sample_point_center_to_screen(metrics,
                                           interaction->path_draft_points[i].x,
                                           interaction->path_draft_points[i].y,
                                           &cx,
                                           &cy)) {
            continue;
        }
        draw_preview_bezier_handles(renderer,
                                    metrics,
                                    &interaction->path_draft_points[i],
                                    interaction->path_draft_drag_active &&
                                        interaction->path_draft_drag_point_index == i,
                                    handle_half - 1);
        draw_preview_point_handle(renderer, cx, cy, handle_half, point_fill, accent, 255u, 255u);
    }
    if (point_count > 0u && ui && ui->mouse_known && ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_PATH &&
        !interaction->path_draft_drag_active) {
        int last_x = 0;
        int last_y = 0;
        int preview_x = 0;
        int preview_y = 0;
        if (sample_center_to_screen(metrics,
                                    ctx,
                                    (uint32_t)interaction->path_draft_points[point_count - 1u].x,
                                    (uint32_t)interaction->path_draft_points[point_count - 1u].y,
                                    &last_x,
                                    &last_y) &&
            sample_center_to_screen(metrics,
                                    ctx,
                                    interaction->path_preview_sample_x,
                                    interaction->path_preview_sample_y,
                                    &preview_x,
                                    &preview_y) &&
            (preview_x != last_x || preview_y != last_y)) {
            draw_preview_thick_line(renderer, last_x, last_y, preview_x, preview_y, stroke_px, preview_color, 170u);
        }
    }
}

void drawing_program_visual_draw_shape_preview_overlay(
    SDL_Renderer *renderer,
    SDL_Rect pane_rect,
    const DrawingProgramAppContext *ctx,
    const CoreThemePreset *theme,
    const VisualCanvasSheetMetrics *metrics,
    const VisualCanvasInteractionState *interaction,
    const VisualPanelUiState *ui,
    const DrawingProgramVisualShapePreviewHooks *hooks) {
    DrawingProgramToolKind tool;
    uint32_t end_x;
    uint32_t end_y;
    SDL_Color accent = { 120u, 160u, 220u, 255u };
    if (!renderer || !ctx || !metrics || !interaction || !hooks || !hooks->tool_uses_shape_commit ||
        !hooks->tool_shape_mode || !hooks->tool_shape_stroke_width || !hooks->shape_mode_includes_fill ||
        !hooks->shape_mode_includes_outline || !hooks->screen_to_canvas_sample) {
        return;
    }
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_ACCENT_PRIMARY, &accent);
    (void)SDL_RenderSetClipRect(renderer, &pane_rect);
    if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_PATH && interaction->path_draft_active) {
        draw_preview_path_draft(renderer, ctx, metrics, interaction, ui, hooks, accent);
    }
    if (!interaction->shape_active) {
        (void)SDL_RenderSetClipRect(renderer, 0);
        return;
    }
    tool = (DrawingProgramToolKind)interaction->shape_tool;
    if (!hooks->tool_uses_shape_commit(tool)) {
        (void)SDL_RenderSetClipRect(renderer, 0);
        return;
    }
    end_x = interaction->shape_start_sample_x;
    end_y = interaction->shape_start_sample_y;
    if (ui && ui->mouse_known) {
        (void)hooks->screen_to_canvas_sample(ctx, pane_rect, ui->mouse_x, ui->mouse_y, &end_x, &end_y);
    }
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_LINE: {
            int x0 = 0;
            int y0 = 0;
            int x1 = 0;
            int y1 = 0;
            int stroke_px = preview_stroke_width_pixels(metrics, hooks->tool_shape_stroke_width(ctx));
            if (sample_center_to_screen(metrics,
                                        ctx,
                                        interaction->shape_start_sample_x,
                                        interaction->shape_start_sample_y,
                                        &x0,
                                        &y0) &&
                sample_center_to_screen(metrics, ctx, end_x, end_y, &x1, &y1)) {
                draw_preview_thick_line(renderer, x0, y0, x1, y1, stroke_px, accent, 230u);
            }
            break;
        }
        case DRAWING_PROGRAM_TOOL_RECT: {
            uint32_t min_x = (interaction->shape_start_sample_x < end_x) ? interaction->shape_start_sample_x : end_x;
            uint32_t min_y = (interaction->shape_start_sample_y < end_y) ? interaction->shape_start_sample_y : end_y;
            uint32_t max_x = (interaction->shape_start_sample_x > end_x) ? interaction->shape_start_sample_x : end_x;
            uint32_t max_y = (interaction->shape_start_sample_y > end_y) ? interaction->shape_start_sample_y : end_y;
            SDL_Rect rect = { 0, 0, 0, 0 };
            uint32_t w = (max_x - min_x) + 1u;
            uint32_t h = (max_y - min_y) + 1u;
            uint8_t mode = hooks->tool_shape_mode(ctx);
            int stroke_px = preview_stroke_width_pixels(metrics, hooks->tool_shape_stroke_width(ctx));
            if (drawing_program_visual_overlay_sample_rect_to_screen_rect(
                    metrics, (int32_t)min_x, (int32_t)min_y, w, h, &rect)) {
                if (hooks->shape_mode_includes_fill(tool, mode)) {
                    SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 36u);
                    (void)SDL_RenderFillRect(renderer, &rect);
                }
                if (hooks->shape_mode_includes_outline(tool, mode)) {
                    draw_preview_thick_rect_outline(renderer, rect, stroke_px, accent, 230u);
                }
            }
            break;
        }
        case DRAWING_PROGRAM_TOOL_CIRCLE: {
            int cx = 0;
            int cy = 0;
            int32_t dx = (int32_t)end_x - (int32_t)interaction->shape_start_sample_x;
            int32_t dy = (int32_t)end_y - (int32_t)interaction->shape_start_sample_y;
            int32_t r_samples = (dx < 0 ? -dx : dx);
            int32_t ay = (dy < 0 ? -dy : dy);
            int radius_px;
            int stroke_px = preview_stroke_width_pixels(metrics, hooks->tool_shape_stroke_width(ctx));
            uint8_t mode = hooks->tool_shape_mode(ctx);
            if (ay > r_samples) {
                r_samples = ay;
            }
            if (!sample_center_to_screen(metrics,
                                         ctx,
                                         interaction->shape_start_sample_x,
                                         interaction->shape_start_sample_y,
                                         &cx,
                                         &cy)) {
                break;
            }
            radius_px = (int)lroundf((((float)r_samples) + 0.5f) * metrics->pixel_size);
            if (radius_px < 1) {
                radius_px = 1;
            }
            if (hooks->shape_mode_includes_fill(tool, mode)) {
                draw_preview_circle_fill(renderer, cx, cy, radius_px, accent, 36u);
            }
            if (hooks->shape_mode_includes_outline(tool, mode)) {
                draw_preview_circle_outline(renderer, cx, cy, radius_px, stroke_px, accent, 230u);
            }
            break;
        }
        default:
            break;
    }
    (void)SDL_RenderSetClipRect(renderer, 0);
}
