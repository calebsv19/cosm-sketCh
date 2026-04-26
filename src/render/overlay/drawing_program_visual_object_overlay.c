#include "drawing_program/drawing_program_visual_overlay_render.h"

#include <math.h>
#include <string.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_object_geometry.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_overlay_shared.h"
#include "drawing_program/drawing_program_visual_theme.h"

typedef struct DrawingProgramVisualObjectOverlayCache {
    SDL_Renderer *renderer_owner;
    SDL_Texture *texture;
    int texture_w;
    int texture_h;
    uint32_t signature;
    uint8_t valid;
} DrawingProgramVisualObjectOverlayCache;

static DrawingProgramVisualObjectOverlayCache g_object_overlay_cache = {0};

static SDL_Color drawing_program_visual_overlay_color_from_sample(DrawingProgramRasterSample sample) {
    SDL_Color color = {0u, 0u, 0u, 0u};
    drawing_program_color_rgba_from_sample(sample, &color.r, &color.g, &color.b, &color.a);
    return color;
}

static void drawing_program_visual_object_overlay_cache_reset(void) {
    if (g_object_overlay_cache.texture) {
        SDL_DestroyTexture(g_object_overlay_cache.texture);
        g_object_overlay_cache.texture = 0;
    }
    g_object_overlay_cache.renderer_owner = 0;
    g_object_overlay_cache.texture_w = 0;
    g_object_overlay_cache.texture_h = 0;
    g_object_overlay_cache.signature = 0u;
    g_object_overlay_cache.valid = 0u;
}

static uint32_t drawing_program_visual_hash32_append_bytes(uint32_t hash32,
                                                           const void *bytes,
                                                           size_t byte_count) {
    const uint8_t *cursor = (const uint8_t *)bytes;
    size_t i;
    if (!cursor || byte_count == 0u) {
        return hash32;
    }
    for (i = 0u; i < byte_count; ++i) {
        hash32 ^= (uint32_t)cursor[i];
        hash32 *= 16777619u;
    }
    return hash32;
}

static uint32_t drawing_program_visual_object_overlay_signature(
    SDL_Rect pane_rect,
    const DrawingProgramAppContext *ctx,
    const VisualCanvasSheetMetrics *metrics,
    const VisualCanvasInteractionState *interaction,
    uint32_t hover_object_id) {
    uint32_t hash32 = 2166136261u;
    int32_t local_sheet_x = 0;
    int32_t local_sheet_y = 0;
    uint32_t pixel_size_bits = 0u;
    uint32_t object_count = 0u;
    uint32_t layer_count = 0u;
    if (!ctx || !metrics) {
        return 0u;
    }
    local_sheet_x = metrics->sheet_rect.x - pane_rect.x;
    local_sheet_y = metrics->sheet_rect.y - pane_rect.y;
    memcpy(&pixel_size_bits, &metrics->pixel_size, sizeof(pixel_size_bits));
    hash32 = drawing_program_visual_hash32_append_bytes(hash32, &pane_rect.w, sizeof(pane_rect.w));
    hash32 = drawing_program_visual_hash32_append_bytes(hash32, &pane_rect.h, sizeof(pane_rect.h));
    hash32 = drawing_program_visual_hash32_append_bytes(hash32, &local_sheet_x, sizeof(local_sheet_x));
    hash32 = drawing_program_visual_hash32_append_bytes(hash32, &local_sheet_y, sizeof(local_sheet_y));
    hash32 = drawing_program_visual_hash32_append_bytes(hash32, &metrics->sheet_rect.w, sizeof(metrics->sheet_rect.w));
    hash32 = drawing_program_visual_hash32_append_bytes(hash32, &metrics->sheet_rect.h, sizeof(metrics->sheet_rect.h));
    hash32 = drawing_program_visual_hash32_append_bytes(hash32, &pixel_size_bits, sizeof(pixel_size_bits));
    hash32 = drawing_program_visual_hash32_append_bytes(hash32, &hover_object_id, sizeof(hover_object_id));
    hash32 = drawing_program_visual_hash32_append_bytes(hash32, &ctx->editor.active_tool, sizeof(ctx->editor.active_tool));
    hash32 = drawing_program_visual_hash32_append_bytes(
        hash32, &ctx->object_selection, sizeof(ctx->object_selection));
    object_count = ctx->object_store.object_count;
    if (object_count > DRAWING_PROGRAM_MAX_OBJECTS) {
        object_count = DRAWING_PROGRAM_MAX_OBJECTS;
    }
    hash32 = drawing_program_visual_hash32_append_bytes(hash32, &object_count, sizeof(object_count));
    if (object_count > 0u) {
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, ctx->object_store.objects, (size_t)object_count * sizeof(ctx->object_store.objects[0]));
    }
    layer_count = ctx->document.layer_count;
    if (layer_count > DRAWING_PROGRAM_MAX_LAYERS) {
        layer_count = DRAWING_PROGRAM_MAX_LAYERS;
    }
    hash32 = drawing_program_visual_hash32_append_bytes(hash32, &layer_count, sizeof(layer_count));
    if (layer_count > 0u) {
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, ctx->document.layers, (size_t)layer_count * sizeof(ctx->document.layers[0]));
    }
    hash32 = drawing_program_visual_hash32_append_bytes(
        hash32, &ctx->ui.layer_opacity_entry_count, sizeof(ctx->ui.layer_opacity_entry_count));
    hash32 = drawing_program_visual_hash32_append_bytes(
        hash32, ctx->ui.layer_opacity_layer_ids, sizeof(ctx->ui.layer_opacity_layer_ids));
    hash32 = drawing_program_visual_hash32_append_bytes(
        hash32, ctx->ui.layer_opacity_values, sizeof(ctx->ui.layer_opacity_values));
    if (interaction) {
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->drawing_active, sizeof(interaction->drawing_active));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->panning_active, sizeof(interaction->panning_active));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->shape_active, sizeof(interaction->shape_active));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->path_draft_active, sizeof(interaction->path_draft_active));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->path_draft_closed, sizeof(interaction->path_draft_closed));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->path_draft_drag_active, sizeof(interaction->path_draft_drag_active));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->path_draft_point_count, sizeof(interaction->path_draft_point_count));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, interaction->path_draft_points, sizeof(interaction->path_draft_points));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->transform_active, sizeof(interaction->transform_active));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->object_move_active, sizeof(interaction->object_move_active));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->object_move_offset_x, sizeof(interaction->object_move_offset_x));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->object_move_offset_y, sizeof(interaction->object_move_offset_y));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->object_path_point_move_active, sizeof(interaction->object_path_point_move_active));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->object_path_point_object_id, sizeof(interaction->object_path_point_object_id));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->object_path_point_index, sizeof(interaction->object_path_point_index));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->object_path_point_offset_x, sizeof(interaction->object_path_point_offset_x));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->object_path_point_offset_y, sizeof(interaction->object_path_point_offset_y));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->object_path_handle_move_active, sizeof(interaction->object_path_handle_move_active));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->object_path_handle_object_id, sizeof(interaction->object_path_handle_object_id));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->object_path_handle_point_index, sizeof(interaction->object_path_handle_point_index));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->object_path_handle_kind, sizeof(interaction->object_path_handle_kind));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->object_path_handle_dx, sizeof(interaction->object_path_handle_dx));
        hash32 = drawing_program_visual_hash32_append_bytes(
            hash32, &interaction->object_path_handle_dy, sizeof(interaction->object_path_handle_dy));
    }
    return hash32;
}

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
    SDL_Color stroke_color;
    SDL_Color fill_color;
    if (!renderer || !ctx || !metrics || !object || object->width == 0u || object->height == 0u) {
        return;
    }
    stroke_color = drawing_program_visual_overlay_color_from_sample(object->stroke_color_value);
    fill_color = drawing_program_visual_overlay_color_from_sample(object->fill_color_value);
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
    SDL_Color stroke_color;
    SDL_Color fill_color;
    double rx;
    double ry;
    double inner_rx;
    double inner_ry;
    double cx;
    double cy;
    if (!renderer || !ctx || !metrics || !object || object->width == 0u || object->height == 0u) {
        return;
    }
    stroke_color = drawing_program_visual_overlay_color_from_sample(object->stroke_color_value);
    fill_color = drawing_program_visual_overlay_color_from_sample(object->fill_color_value);
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

static void draw_object_path(SDL_Renderer *renderer,
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
    has_fill = object_style_includes_fill(object->style_mode) ? 1 : 0;
    has_outline = object_style_includes_outline(object->style_mode) ? 1 : 0;
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
    stroke_color = drawing_program_visual_overlay_color_from_sample(preview_object.stroke_color_value);
    fill_color = drawing_program_visual_overlay_color_from_sample(preview_object.fill_color_value);
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

static void draw_selected_bezier_point_handles(SDL_Renderer *renderer,
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

static void drawing_program_visual_draw_object_overlay_core(
    SDL_Renderer *renderer,
    SDL_Rect pane_rect,
    const DrawingProgramAppContext *ctx,
    const CoreThemePreset *theme,
    const VisualCanvasSheetMetrics *metrics,
    const VisualCanvasInteractionState *interaction,
    uint32_t hover_object_id) {
    SDL_Color outline_color = { 116u, 126u, 148u, 255u };
    SDL_Color selected_outline = { 72u, 134u, 255u, 255u };
    SDL_Color hover_outline = { 128u, 176u, 255u, 255u };
    uint32_t visible_indices[DRAWING_PROGRAM_MAX_OBJECTS];
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
            draw_object_path(renderer, ctx, metrics, object, interaction, draw_origin_x, draw_origin_y, alpha);
            draw_active_path_point_edge_preview(
                renderer, metrics, object, interaction, draw_origin_x, draw_origin_y);
            draw_selected_bezier_point_handles(
                renderer, ctx, metrics, object, interaction, draw_origin_x, draw_origin_y);
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

void drawing_program_visual_draw_object_overlay(SDL_Renderer *renderer,
                                                SDL_Rect pane_rect,
                                                const DrawingProgramAppContext *ctx,
                                                const CoreThemePreset *theme,
                                                const VisualCanvasSheetMetrics *metrics,
                                                const VisualCanvasInteractionState *interaction,
                                                const VisualPanelUiState *ui) {
    SDL_Texture *previous_target = 0;
    uint32_t hover_object_id = 0u;
    uint32_t signature = 0u;
    VisualCanvasSheetMetrics local_metrics;
    SDL_Rect local_pane_rect;
    if (!renderer || !ctx || !metrics || pane_rect.w <= 0 || pane_rect.h <= 0) {
        return;
    }
    if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_SELECT && ui && ui->mouse_known) {
        uint32_t sample_x = 0u;
        uint32_t sample_y = 0u;
        if (object_overlay_screen_to_canvas_sample(ctx, metrics, ui->mouse_x, ui->mouse_y, &sample_x, &sample_y)) {
            (void)drawing_program_object_store_hit_test_topmost(
                &ctx->object_store, &ctx->document, sample_x, sample_y, &hover_object_id, 0);
        }
    }

    if (g_object_overlay_cache.renderer_owner != renderer ||
        g_object_overlay_cache.texture_w != pane_rect.w ||
        g_object_overlay_cache.texture_h != pane_rect.h) {
        drawing_program_visual_object_overlay_cache_reset();
    }
    if (!g_object_overlay_cache.texture) {
        g_object_overlay_cache.texture = SDL_CreateTexture(renderer,
                                                           SDL_PIXELFORMAT_RGBA8888,
                                                           SDL_TEXTUREACCESS_TARGET,
                                                           pane_rect.w,
                                                           pane_rect.h);
        if (!g_object_overlay_cache.texture) {
            drawing_program_visual_draw_object_overlay_core(
                renderer, pane_rect, ctx, theme, metrics, interaction, hover_object_id);
            return;
        }
        g_object_overlay_cache.renderer_owner = renderer;
        g_object_overlay_cache.texture_w = pane_rect.w;
        g_object_overlay_cache.texture_h = pane_rect.h;
        g_object_overlay_cache.valid = 0u;
        (void)SDL_SetTextureBlendMode(g_object_overlay_cache.texture, SDL_BLENDMODE_BLEND);
    }

    signature = drawing_program_visual_object_overlay_signature(
        pane_rect, ctx, metrics, interaction, hover_object_id);
    if (!g_object_overlay_cache.valid || g_object_overlay_cache.signature != signature) {
        previous_target = SDL_GetRenderTarget(renderer);
        if (SDL_SetRenderTarget(renderer, g_object_overlay_cache.texture) != 0) {
            (void)SDL_SetRenderTarget(renderer, previous_target);
            drawing_program_visual_draw_object_overlay_core(
                renderer, pane_rect, ctx, theme, metrics, interaction, hover_object_id);
            return;
        }
        SDL_SetRenderDrawColor(renderer, 0u, 0u, 0u, 0u);
        (void)SDL_RenderClear(renderer);
        local_pane_rect = (SDL_Rect){0, 0, pane_rect.w, pane_rect.h};
        local_metrics = *metrics;
        local_metrics.sheet_rect.x -= pane_rect.x;
        local_metrics.sheet_rect.y -= pane_rect.y;
        drawing_program_visual_draw_object_overlay_core(
            renderer, local_pane_rect, ctx, theme, &local_metrics, interaction, hover_object_id);
        (void)SDL_SetRenderTarget(renderer, previous_target);
        g_object_overlay_cache.signature = signature;
        g_object_overlay_cache.valid = 1u;
    }
    (void)SDL_RenderCopy(renderer, g_object_overlay_cache.texture, 0, &pane_rect);
}

void drawing_program_visual_object_overlay_cache_shutdown(void) {
    drawing_program_visual_object_overlay_cache_reset();
}
