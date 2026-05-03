#include "drawing_program_visual_object_overlay_internal.h"

#include <string.h>

#include "drawing_program/drawing_program_object_geometry.h"

static int draw_object_path_contains_fill(const DrawingProgramObjectRecord *object,
                                          int32_t delta_x,
                                          int32_t delta_y,
                                          double px,
                                          double py) {
    double flat_xy[DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS * 24u * 2u];
    uint32_t flat_count = 0u;
    CoreResult result;
    if (!object || !object->path_closed) {
        return 0;
    }
    result = drawing_program_object_path_flatten_points(
        object, delta_x, delta_y, flat_xy, DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS * 24u, &flat_count);
    if (result.code != CORE_OK) {
        return 0;
    }
    return drawing_program_object_path_flattened_contains_fill(flat_xy, flat_count, px, py);
}

static int draw_object_path_contains_outline(const DrawingProgramObjectRecord *object,
                                             int32_t delta_x,
                                             int32_t delta_y,
                                             double px,
                                             double py) {
    double flat_xy[DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS * 24u * 2u];
    uint32_t flat_count = 0u;
    double half_width;
    double threshold_sq;
    CoreResult result;
    if (!object) {
        return 0;
    }
    result = drawing_program_object_path_flatten_points(
        object, delta_x, delta_y, flat_xy, DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS * 24u, &flat_count);
    if (result.code != CORE_OK) {
        return 0;
    }
    half_width = ((double)(object->stroke_width == 0u ? 1u : object->stroke_width)) * 0.5;
    if (half_width < 0.6) {
        half_width = 0.6;
    }
    threshold_sq = half_width * half_width;
    return drawing_program_object_path_flattened_contains_outline(
        flat_xy, flat_count, object->path_closed, px, py, threshold_sq);
}

void drawing_program_visual_object_overlay_draw_path(
    SDL_Renderer *renderer,
    const DrawingProgramAppContext *ctx,
    const VisualCanvasSheetMetrics *metrics,
    const DrawingProgramObjectRecord *object,
    const VisualCanvasInteractionState *interaction,
    int32_t origin_x,
    int32_t origin_y,
    uint8_t alpha) {
    int32_t min_x;
    int32_t min_y;
    int32_t max_x;
    int32_t max_y;
    int32_t x;
    int32_t y;
    int32_t move_delta_x;
    int32_t move_delta_y;
    int has_fill;
    int has_outline;
    int has_flattened_outline = 0;
    int has_flattened_fill = 0;
    CoreResult flatten_result = core_result_ok();
    double flat_xy[DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS * 24u * 2u];
    uint32_t flat_count = 0u;
    double half_width = 0.0;
    double threshold_sq = 0.0;
    DrawingProgramObjectRecord preview_object;
    SDL_Color stroke_color = { 0u, 0u, 0u, 255u };
    SDL_Color fill_color = { 0u, 0u, 0u, 255u };
    if (!renderer || !ctx || !metrics || !object || object->path_point_count < 2u) {
        return;
    }
    preview_object = *object;
    if (interaction &&
        interaction->object_path_point_move_active &&
        interaction->object_path_point_object_id == object->object_id &&
        interaction->object_path_point_index < preview_object.path_point_count) {
        preview_object.path_points[interaction->object_path_point_index].x += interaction->object_path_point_offset_x;
        preview_object.path_points[interaction->object_path_point_index].y += interaction->object_path_point_offset_y;
    }
    if (interaction &&
        interaction->object_path_handle_move_active &&
        interaction->object_path_handle_object_id == object->object_id &&
        interaction->object_path_handle_point_index < preview_object.path_point_count) {
        DrawingProgramPathPoint *point = &preview_object.path_points[interaction->object_path_handle_point_index];
        if (interaction->object_path_handle_kind == (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_IN) {
            point->handle_in_dx = interaction->object_path_handle_dx;
            point->handle_in_dy = interaction->object_path_handle_dy;
            if (point->handle_linked) {
                point->handle_out_dx = -interaction->object_path_handle_dx;
                point->handle_out_dy = -interaction->object_path_handle_dy;
            }
        } else if (interaction->object_path_handle_kind == (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_OUT) {
            point->handle_out_dx = interaction->object_path_handle_dx;
            point->handle_out_dy = interaction->object_path_handle_dy;
            if (point->handle_linked) {
                point->handle_in_dx = -interaction->object_path_handle_dx;
                point->handle_in_dy = -interaction->object_path_handle_dy;
            }
        }
    }
    {
        DrawingProgramPathPayload payload;
        int32_t bounds_origin_x = 0;
        int32_t bounds_origin_y = 0;
        uint32_t bounds_width = 0u;
        uint32_t bounds_height = 0u;
        memset(&payload, 0, sizeof(payload));
        payload.point_count = preview_object.path_point_count;
        payload.closed = preview_object.path_closed;
        memcpy(payload.points,
               preview_object.path_points,
               (size_t)payload.point_count * sizeof(payload.points[0]));
        if (drawing_program_object_path_payload_bounds(
                &payload, &bounds_origin_x, &bounds_origin_y, &bounds_width, &bounds_height).code == CORE_OK) {
            preview_object.origin_x = bounds_origin_x;
            preview_object.origin_y = bounds_origin_y;
            preview_object.width = bounds_width;
            preview_object.height = bounds_height;
        }
    }
    has_fill = drawing_program_visual_object_style_includes_fill(object->style_mode) ? 1 : 0;
    has_outline = drawing_program_visual_object_style_includes_outline(object->style_mode) ? 1 : 0;
    move_delta_x = origin_x - object->origin_x;
    move_delta_y = origin_y - object->origin_y;
    if (has_outline || has_fill) {
        flatten_result = drawing_program_object_path_flatten_points(
            &preview_object, move_delta_x, move_delta_y, flat_xy, DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS * 24u, &flat_count);
        if (flatten_result.code == CORE_OK) {
            if (has_outline) {
                half_width = ((double)(preview_object.stroke_width == 0u ? 1u : preview_object.stroke_width)) * 0.5;
                if (half_width < 0.6) {
                    half_width = 0.6;
                }
                threshold_sq = half_width * half_width;
                has_flattened_outline = 1;
            }
            if (has_fill && preview_object.path_closed) {
                has_flattened_fill = 1;
            }
        }
    }
    min_x = preview_object.origin_x + move_delta_x;
    min_y = preview_object.origin_y + move_delta_y;
    max_x = min_x + (int32_t)preview_object.width - 1;
    max_y = min_y + (int32_t)preview_object.height - 1;
    stroke_color = drawing_program_visual_object_overlay_color_from_sample(preview_object.stroke_color_value);
    fill_color = drawing_program_visual_object_overlay_color_from_sample(preview_object.fill_color_value);
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
                if (has_flattened_outline) {
                    on_outline = drawing_program_object_path_flattened_contains_outline(
                        flat_xy, flat_count, preview_object.path_closed, px, py, threshold_sq);
                } else {
                    on_outline = draw_object_path_contains_outline(
                        &preview_object, move_delta_x, move_delta_y, px, py);
                }
            }
            if (has_fill) {
                if (has_flattened_fill) {
                    inside_fill = drawing_program_object_path_flattened_contains_fill(flat_xy, flat_count, px, py);
                } else {
                    inside_fill = draw_object_path_contains_fill(
                        &preview_object, move_delta_x, move_delta_y, px, py);
                }
            }
            if (on_outline) {
                (void)drawing_program_visual_object_overlay_draw_sample_cell(renderer, metrics, x, y, stroke_color, alpha);
            } else if (inside_fill) {
                (void)drawing_program_visual_object_overlay_draw_sample_cell(renderer, metrics, x, y, fill_color, alpha);
            }
        }
    }
}

void drawing_program_visual_object_overlay_draw_selected_path_point_handles(
    SDL_Renderer *renderer,
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
        const DrawingProgramPathPoint *point = &object->path_points[i];
        SDL_Rect point_rect;
        SDL_Rect point_inner;
        SDL_Color border_color = { 40u, 112u, 255u, 255u };
        SDL_Color fill_color = { 236u, 246u, 255u, 255u };
        SDL_Color cross_color = { 24u, 86u, 214u, 255u };
        SDL_Rect cross_h;
        SDL_Rect cross_v;
        int min_handle_size = 10;
        int is_selected_point = (ctx->object_selection.selected_path_point_active &&
                                 ctx->object_selection.selected_path_point_object_id == object->object_id &&
                                 ctx->object_selection.selected_path_point_index == i)
                                    ? 1
                                    : 0;
        if (interaction &&
            interaction->object_move_active &&
            !interaction->object_path_point_move_active) {
            sample_x += interaction->object_move_offset_x;
            sample_y += interaction->object_move_offset_y;
        }
        if (is_selected_point) {
            border_color = (SDL_Color){ 0u, 148u, 255u, 255u };
            fill_color = (SDL_Color){ 222u, 241u, 255u, 255u };
            cross_color = (SDL_Color){ 0u, 110u, 214u, 255u };
            min_handle_size = 12;
            if (point->bezier_enabled) {
                if (point->handle_linked) {
                    border_color = (SDL_Color){ 0u, 166u, 255u, 255u };
                    fill_color = (SDL_Color){ 214u, 244u, 255u, 255u };
                    cross_color = (SDL_Color){ 0u, 114u, 226u, 255u };
                } else {
                    border_color = (SDL_Color){ 168u, 76u, 224u, 255u };
                    fill_color = (SDL_Color){ 248u, 226u, 255u, 255u };
                    cross_color = (SDL_Color){ 116u, 40u, 176u, 255u };
                }
                min_handle_size = 14;
            }
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
            if (is_selected_point) {
                SDL_Rect outer2 = { point_rect.x - 3, point_rect.y - 3, point_rect.w + 6, point_rect.h + 6 };
                SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, border_color.b, 210u);
                (void)SDL_RenderDrawRect(renderer, &outer2);
            }
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

static int draw_bezier_handle_node(SDL_Renderer *renderer,
                                   const VisualCanvasSheetMetrics *metrics,
                                   int32_t anchor_sample_x,
                                   int32_t anchor_sample_y,
                                   int32_t handle_dx,
                                   int32_t handle_dy,
                                   const SDL_Color *line_color,
                                   const SDL_Color *line_shadow,
                                   const SDL_Color *fill_color,
                                   const SDL_Color *border_color) {
    const int min_handle_size = 10;
    int anchor_sx = 0;
    int anchor_sy = 0;
    int handle_sx = 0;
    int handle_sy = 0;
    SDL_Rect rect;
    SDL_Rect inner;
    SDL_Rect outer;
    if (!renderer || !metrics || !line_color || !line_shadow || !fill_color || !border_color) {
        return 0;
    }
    if (handle_dx == 0 && handle_dy == 0) {
        return 0;
    }
    if (!path_point_screen_center(metrics, anchor_sample_x, anchor_sample_y, &anchor_sx, &anchor_sy) ||
        !path_point_screen_center(metrics,
                                  anchor_sample_x + handle_dx,
                                  anchor_sample_y + handle_dy,
                                  &handle_sx,
                                  &handle_sy)) {
        return 0;
    }
    SDL_SetRenderDrawColor(renderer, line_shadow->r, line_shadow->g, line_shadow->b, line_shadow->a);
    (void)SDL_RenderDrawLine(renderer, anchor_sx + 1, anchor_sy + 1, handle_sx + 1, handle_sy + 1);
    SDL_SetRenderDrawColor(renderer, line_color->r, line_color->g, line_color->b, line_color->a);
    (void)SDL_RenderDrawLine(renderer, anchor_sx, anchor_sy, handle_sx, handle_sy);
    rect.x = handle_sx - (min_handle_size / 2);
    rect.y = handle_sy - (min_handle_size / 2);
    rect.w = min_handle_size;
    rect.h = min_handle_size;
    inner = rect;
    outer = rect;
    if (inner.w > 2) {
        inner.x += 1;
        inner.w -= 2;
    }
    if (inner.h > 2) {
        inner.y += 1;
        inner.h -= 2;
    }
    outer.x -= 1;
    outer.y -= 1;
    outer.w += 2;
    outer.h += 2;
    SDL_SetRenderDrawColor(renderer, 16u, 26u, 52u, 220u);
    (void)SDL_RenderDrawRect(renderer, &outer);
    SDL_SetRenderDrawColor(renderer, fill_color->r, fill_color->g, fill_color->b, fill_color->a);
    (void)SDL_RenderFillRect(renderer, &inner);
    SDL_SetRenderDrawColor(renderer, border_color->r, border_color->g, border_color->b, border_color->a);
    (void)SDL_RenderDrawRect(renderer, &rect);
    return 1;
}

void drawing_program_visual_object_overlay_draw_selected_bezier_point_handles(
    SDL_Renderer *renderer,
    const DrawingProgramAppContext *ctx,
    const VisualCanvasSheetMetrics *metrics,
    const DrawingProgramObjectRecord *object,
    const VisualCanvasInteractionState *interaction,
    int32_t draw_origin_x,
    int32_t draw_origin_y) {
    const DrawingProgramPathPoint *source_point = 0;
    DrawingProgramPathPoint point;
    int32_t delta_x = 0;
    int32_t delta_y = 0;
    int32_t anchor_x = 0;
    int32_t anchor_y = 0;
    uint16_t point_index = 0u;
    if (!renderer || !ctx || !metrics || !object ||
        object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
        !ctx->object_selection.selected_path_point_active ||
        ctx->object_selection.selected_path_point_object_id != object->object_id ||
        ctx->object_selection.selected_path_point_index >= object->path_point_count) {
        return;
    }
    point_index = ctx->object_selection.selected_path_point_index;
    source_point = &object->path_points[point_index];
    if (!source_point->bezier_enabled) {
        return;
    }
    point = *source_point;
    if (interaction &&
        interaction->object_path_handle_move_active &&
        interaction->object_path_handle_object_id == object->object_id &&
        interaction->object_path_handle_point_index == point_index) {
        if (interaction->object_path_handle_kind == (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_IN) {
            point.handle_in_dx = interaction->object_path_handle_dx;
            point.handle_in_dy = interaction->object_path_handle_dy;
            if (point.handle_linked) {
                point.handle_out_dx = -interaction->object_path_handle_dx;
                point.handle_out_dy = -interaction->object_path_handle_dy;
            }
        } else if (interaction->object_path_handle_kind == (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_OUT) {
            point.handle_out_dx = interaction->object_path_handle_dx;
            point.handle_out_dy = interaction->object_path_handle_dy;
            if (point.handle_linked) {
                point.handle_in_dx = -interaction->object_path_handle_dx;
                point.handle_in_dy = -interaction->object_path_handle_dy;
            }
        }
    }
    delta_x = draw_origin_x - object->origin_x;
    delta_y = draw_origin_y - object->origin_y;
    anchor_x = point.x + delta_x;
    anchor_y = point.y + delta_y;
    if (interaction &&
        interaction->object_path_point_move_active &&
        interaction->object_path_point_object_id == object->object_id &&
        interaction->object_path_point_index == point_index) {
        anchor_x += interaction->object_path_point_offset_x;
        anchor_y += interaction->object_path_point_offset_y;
    }
    {
        const SDL_Color linked_line_color = { 86u, 162u, 255u, 245u };
        const SDL_Color linked_line_shadow = { 10u, 20u, 46u, 220u };
        const SDL_Color linked_handle_fill = { 236u, 246u, 255u, 240u };
        const SDL_Color linked_handle_border = { 40u, 112u, 255u, 255u };
        const SDL_Color unlinked_in_line_color = { 218u, 116u, 255u, 245u };
        const SDL_Color unlinked_in_line_shadow = { 44u, 18u, 62u, 220u };
        const SDL_Color unlinked_in_fill = { 252u, 232u, 255u, 240u };
        const SDL_Color unlinked_in_border = { 168u, 76u, 224u, 255u };
        const SDL_Color unlinked_out_line_color = { 96u, 214u, 168u, 245u };
        const SDL_Color unlinked_out_line_shadow = { 14u, 42u, 34u, 220u };
        const SDL_Color unlinked_out_fill = { 226u, 255u, 244u, 240u };
        const SDL_Color unlinked_out_border = { 44u, 156u, 118u, 255u };
        const SDL_Color active_fill = { 255u, 228u, 166u, 245u };
        const SDL_Color active_border = { 212u, 128u, 28u, 255u };
        const SDL_Color *in_line = point.handle_linked ? &linked_line_color : &unlinked_in_line_color;
        const SDL_Color *in_shadow = point.handle_linked ? &linked_line_shadow : &unlinked_in_line_shadow;
        const SDL_Color *in_fill = point.handle_linked ? &linked_handle_fill : &unlinked_in_fill;
        const SDL_Color *in_border = point.handle_linked ? &linked_handle_border : &unlinked_in_border;
        const SDL_Color *out_line = point.handle_linked ? &linked_line_color : &unlinked_out_line_color;
        const SDL_Color *out_shadow = point.handle_linked ? &linked_line_shadow : &unlinked_out_line_shadow;
        const SDL_Color *out_fill = point.handle_linked ? &linked_handle_fill : &unlinked_out_fill;
        const SDL_Color *out_border = point.handle_linked ? &linked_handle_border : &unlinked_out_border;
        if (interaction &&
            interaction->object_path_handle_move_active &&
            interaction->object_path_handle_object_id == object->object_id &&
            interaction->object_path_handle_point_index == point_index) {
            if (interaction->object_path_handle_kind == (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_IN) {
                in_fill = &active_fill;
                in_border = &active_border;
            } else if (interaction->object_path_handle_kind == (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_OUT) {
                out_fill = &active_fill;
                out_border = &active_border;
            }
        }
        (void)draw_bezier_handle_node(renderer,
                                      metrics,
                                      anchor_x,
                                      anchor_y,
                                      point.handle_in_dx,
                                      point.handle_in_dy,
                                      in_line,
                                      in_shadow,
                                      in_fill,
                                      in_border);
        (void)draw_bezier_handle_node(renderer,
                                      metrics,
                                      anchor_x,
                                      anchor_y,
                                      point.handle_out_dx,
                                      point.handle_out_dy,
                                      out_line,
                                      out_shadow,
                                      out_fill,
                                      out_border);
    }
}

void drawing_program_visual_object_overlay_draw_active_path_point_edge_preview(
    SDL_Renderer *renderer,
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
