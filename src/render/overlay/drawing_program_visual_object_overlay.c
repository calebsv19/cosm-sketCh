#include "drawing_program/drawing_program_visual_overlay_render.h"

#include <string.h>

#include "drawing_program_visual_object_overlay_internal.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
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
            drawing_program_visual_object_overlay_draw_ellipse(
                renderer, ctx, metrics, object, draw_origin_x, draw_origin_y, alpha);
        } else if (object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
            drawing_program_visual_object_overlay_draw_path(
                renderer, ctx, metrics, object, interaction, draw_origin_x, draw_origin_y, alpha);
            drawing_program_visual_object_overlay_draw_active_path_point_edge_preview(
                renderer, metrics, object, interaction, draw_origin_x, draw_origin_y);
            drawing_program_visual_object_overlay_draw_selected_bezier_point_handles(
                renderer, ctx, metrics, object, interaction, draw_origin_x, draw_origin_y);
        } else {
            drawing_program_visual_object_overlay_draw_rect(
                renderer, ctx, metrics, object, draw_origin_x, draw_origin_y, alpha);
        }
        drawing_program_visual_object_overlay_draw_selected_path_point_handles(
            renderer,
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
