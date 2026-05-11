#include "drawing_program/drawing_program_visual_canvas_world_render.h"

#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_render_zoom_bucket.h"
#include "drawing_program/drawing_program_render_revision.h"
#include "drawing_program/drawing_program_texture_project.h"
#include "drawing_program/drawing_program_texture_canvas_resize.h"
#include "drawing_program/drawing_program_texture_net_guides.h"
#include "drawing_program/drawing_program_texture_workspace.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_resources.h"
#include "drawing_program/drawing_program_visual_surface_cache.h"
#include "drawing_program/drawing_program_visual_theme.h"

typedef struct DrawingProgramVisualCanvasWorldBackdropCache {
    SDL_Renderer *renderer_owner;
    SDL_Texture *texture;
    int texture_w;
    int texture_h;
    SDL_Rect local_clip_sheet;
    SDL_Color world_grid_minor;
    SDL_Color world_grid_major;
    SDL_Color sheet_fill;
    uint8_t valid;
} DrawingProgramVisualCanvasWorldBackdropCache;

static DrawingProgramVisualCanvasWorldBackdropCache g_canvas_world_backdrop = {0};

static void drawing_program_visual_canvas_world_backdrop_cache_reset(void) {
    if (g_canvas_world_backdrop.texture) {
        SDL_DestroyTexture(g_canvas_world_backdrop.texture);
        g_canvas_world_backdrop.texture = 0;
    }
    g_canvas_world_backdrop.renderer_owner = 0;
    g_canvas_world_backdrop.texture_w = 0;
    g_canvas_world_backdrop.texture_h = 0;
    g_canvas_world_backdrop.local_clip_sheet = (SDL_Rect){0, 0, 0, 0};
    g_canvas_world_backdrop.world_grid_minor = (SDL_Color){0u, 0u, 0u, 0u};
    g_canvas_world_backdrop.world_grid_major = (SDL_Color){0u, 0u, 0u, 0u};
    g_canvas_world_backdrop.sheet_fill = (SDL_Color){0u, 0u, 0u, 0u};
    g_canvas_world_backdrop.valid = 0u;
}

static void drawing_program_visual_canvas_world_draw_backdrop_immediate(
    SDL_Renderer *renderer,
    SDL_Rect pane_rect,
    SDL_Rect clip_sheet,
    SDL_Color world_grid_minor,
    SDL_Color world_grid_major,
    SDL_Color sheet_fill) {
    int px;
    int py;
    (void)SDL_RenderSetClipRect(renderer, &pane_rect);
    SDL_SetRenderDrawColor(renderer, world_grid_minor.r, world_grid_minor.g, world_grid_minor.b, 255u);
    for (px = pane_rect.x; px < pane_rect.x + pane_rect.w; px += 24) {
        (void)SDL_RenderDrawLine(renderer, px, pane_rect.y, px, pane_rect.y + pane_rect.h);
    }
    for (py = pane_rect.y; py < pane_rect.y + pane_rect.h; py += 24) {
        (void)SDL_RenderDrawLine(renderer, pane_rect.x, py, pane_rect.x + pane_rect.w, py);
    }
    SDL_SetRenderDrawColor(renderer, world_grid_major.r, world_grid_major.g, world_grid_major.b, 255u);
    for (px = pane_rect.x; px < pane_rect.x + pane_rect.w; px += 96) {
        (void)SDL_RenderDrawLine(renderer, px, pane_rect.y, px, pane_rect.y + pane_rect.h);
    }
    for (py = pane_rect.y; py < pane_rect.y + pane_rect.h; py += 96) {
        (void)SDL_RenderDrawLine(renderer, pane_rect.x, py, pane_rect.x + pane_rect.w, py);
    }
    SDL_SetRenderDrawColor(renderer, sheet_fill.r, sheet_fill.g, sheet_fill.b, sheet_fill.a);
    (void)SDL_RenderFillRect(renderer, &clip_sheet);
    (void)SDL_RenderSetClipRect(renderer, 0);
}

static void drawing_program_visual_canvas_world_note_cache_telemetry(
    DrawingProgramAppContext *ctx,
    const DrawingProgramVisualSurfaceCacheTelemetry *telemetry,
    uint8_t active_surface) {
    if (!ctx || !telemetry) {
        return;
    }
    if (telemetry->cache_hit) {
        ctx->runtime.render_surface_cache_hit_total += 1u;
        if (active_surface) {
            ctx->runtime.render_surface_cache_active_hit_total += 1u;
        } else {
            ctx->runtime.render_surface_cache_inactive_hit_total += 1u;
        }
    }
    if (telemetry->cache_miss) {
        ctx->runtime.render_surface_cache_miss_total += 1u;
        if (active_surface) {
            ctx->runtime.render_surface_cache_active_miss_total += 1u;
        } else {
            ctx->runtime.render_surface_cache_inactive_miss_total += 1u;
        }
    }
    if (telemetry->cache_deferred) {
        ctx->runtime.render_surface_cache_deferred_total += 1u;
    }
    if (telemetry->cache_rebuilt) {
        ctx->runtime.render_surface_cache_rebuild_total += 1u;
    }
    if (telemetry->cache_copy_ready) {
        ctx->runtime.render_surface_cache_copy_total += 1u;
    }
    if (telemetry->cache_unavailable) {
        ctx->runtime.render_surface_cache_unavailable_total += 1u;
    }
    ctx->runtime.render_surface_cache_compose_us_total += (uint64_t)telemetry->cache_compose_us;
    ctx->runtime.render_surface_cache_upload_us_total += (uint64_t)telemetry->cache_upload_us;
    ctx->runtime.render_surface_cache_rebuild_us_total += (uint64_t)telemetry->cache_rebuild_us;
}

uint32_t drawing_program_visual_canvas_world_current_zoom_bucket_percent(const DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return 0u;
    }
    return drawing_program_render_zoom_bucket_percent(ctx->editor.viewport.zoom);
}

static void drawing_program_visual_draw_surface_label(SDL_Renderer *renderer,
                                                      SDL_Rect pane_rect,
                                                      const DrawingProgramTextureSurface *surface,
                                                      const VisualCanvasSheetMetrics *metrics,
                                                      SDL_Color color,
                                                      const DrawingProgramVisualCanvasWorldRenderHooks *hooks) {
    char label[96];
    size_t label_len = 0u;
    int label_scale = 1;
    int glyph_w = 0;
    int glyph_h = 0;
    int plate_w = 0;
    int plate_h = 0;
    SDL_Rect plate_rect;
    SDL_Color plate_fill = { 18u, 22u, 28u, 255u };
    if (!renderer || !surface || !metrics || !hooks || !hooks->draw_bitmap_text) {
        return;
    }
    (void)snprintf(label,
                   sizeof(label),
                   "%s %ux%u",
                   drawing_program_texture_project_face_role_name(surface->face_role),
                   (unsigned)surface->storage->document.raster_width,
                   (unsigned)surface->storage->document.raster_height);
    label_len = strlen(label);
    if (metrics->sheet_rect.w >= 96 && metrics->sheet_rect.h >= 72) {
        label_scale = 2;
    }
    glyph_w = 6 * label_scale;
    glyph_h = 7 * label_scale;
    plate_w = (int)(label_len * (size_t)glyph_w) + 8;
    if (plate_w > metrics->sheet_rect.w - 2) {
        plate_w = metrics->sheet_rect.w - 2;
    }
    if (plate_w < glyph_w + 6) {
        plate_w = glyph_w + 6;
    }
    plate_h = glyph_h + 6;
    if (plate_h > metrics->sheet_rect.h - 2) {
        plate_h = metrics->sheet_rect.h - 2;
    }
    if (plate_h < glyph_h + 2) {
        plate_h = glyph_h + 2;
    }
    plate_rect = (SDL_Rect){ metrics->sheet_rect.x + 1,
                             metrics->sheet_rect.y - plate_h - 2,
                             plate_w,
                             plate_h };
    if (plate_rect.y < pane_rect.y) {
        plate_rect.y = metrics->sheet_rect.y + 1;
    }
    if (plate_rect.x < pane_rect.x) {
        plate_rect.x = pane_rect.x;
    }
    if (plate_rect.y < pane_rect.y) {
        plate_rect.y = pane_rect.y;
    }
    SDL_SetRenderDrawColor(renderer, plate_fill.r, plate_fill.g, plate_fill.b, plate_fill.a);
    (void)SDL_RenderFillRect(renderer, &plate_rect);
    (void)hooks->draw_bitmap_text(renderer,
                                  pane_rect,
                                  plate_rect.x + 4,
                                  plate_rect.y + 3,
                                  label,
                                  color,
                                  label_scale);
}

static void drawing_program_visual_draw_reflection_overlay(SDL_Renderer *renderer,
                                                           SDL_Rect pane_rect,
                                                           const DrawingProgramAppContext *ctx,
                                                           const VisualCanvasSheetMetrics *metrics,
                                                           const VisualPanelUiState *ui) {
    uint32_t center_x = 0u;
    uint32_t center_y = 0u;
    int line_x = 0;
    int line_y = 0;
    SDL_Rect marker = {0, 0, 0, 0};
    if (!renderer || !ctx || !metrics) {
        return;
    }
    if (!drawing_program_canvas_reflection_active_center(ctx, &center_x, &center_y)) {
        return;
    }
    if (!ctx->editor.symmetry_horizontal &&
        !ctx->editor.symmetry_vertical &&
        !(ui && ui->right_canvas_reflection_center_pick_pending)) {
        return;
    }
    line_x = metrics->sheet_rect.x + (int)(((float)center_x + 0.5f) * metrics->pixel_size);
    line_y = metrics->sheet_rect.y + (int)(((float)center_y + 0.5f) * metrics->pixel_size);
    if (ctx->editor.symmetry_vertical) {
        SDL_SetRenderDrawColor(renderer, 244u, 186u, 96u, 255u);
        (void)SDL_RenderDrawLine(renderer, line_x, metrics->sheet_rect.y, line_x, metrics->sheet_rect.y + metrics->sheet_rect.h);
    }
    if (ctx->editor.symmetry_horizontal) {
        SDL_SetRenderDrawColor(renderer, 244u, 186u, 96u, 255u);
        (void)SDL_RenderDrawLine(renderer, metrics->sheet_rect.x, line_y, metrics->sheet_rect.x + metrics->sheet_rect.w, line_y);
    }
    marker.w = (metrics->pixel_size >= 6.0f) ? 7 : 5;
    marker.h = marker.w;
    marker.x = line_x - (marker.w / 2);
    marker.y = line_y - (marker.h / 2);
    SDL_SetRenderDrawColor(renderer, 255u, 238u, 184u, 255u);
    (void)SDL_RenderFillRect(renderer, &marker);
    SDL_SetRenderDrawColor(renderer, 66u, 40u, 12u, 255u);
    (void)SDL_RenderDrawRect(renderer, &marker);
    (void)SDL_RenderSetClipRect(renderer, &pane_rect);
}

void drawing_program_visual_draw_canvas_world_view(
    SDL_Renderer *renderer,
    SDL_Rect pane_rect,
    const DrawingProgramAppContext *ctx,
    const CoreThemePreset *theme,
    const VisualSelectionState *selection,
    const VisualPanelUiState *ui,
    const VisualCanvasInteractionState *interaction,
    const DrawingProgramVisualCanvasWorldRenderHooks *hooks) {
    VisualCanvasSheetMetrics metrics;
    uint8_t layer_opacity[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t surface_index = 0u;
    SDL_Color world_grid_minor = { 42u, 48u, 66u, 255u };
    SDL_Color world_grid_major = { 52u, 58u, 78u, 255u };
    SDL_Color sheet_border = { 210u, 210u, 220u, 255u };
    SDL_Color active_sheet_border = { 244u, 186u, 96u, 255u };
    SDL_Color surface_label = { 226u, 230u, 238u, 255u };
    SDL_Color sheet_fill = { 244u, 244u, 248u, 255u };
    if (!renderer || !ctx || !hooks || !hooks->compute_canvas_sheet_metrics || !hooks->draw_selection_overlay ||
        !hooks->draw_object_overlay || !hooks->draw_shape_preview_overlay || !hooks->draw_bitmap_text) {
        return;
    }
    if (ctx->document.raster_width == 0u ||
        ctx->document.raster_height == 0u ||
        ctx->document.raster_sample_count == 0u) {
        return;
    }
    {
        uint32_t zoom_bucket_percent = drawing_program_visual_canvas_world_current_zoom_bucket_percent(ctx);
        if (zoom_bucket_percent != 0u) {
            if (ctx->runtime.render_zoom_bucket_percent != 0u &&
                ctx->runtime.render_zoom_bucket_percent != zoom_bucket_percent) {
                ((DrawingProgramAppContext *)ctx)->runtime.render_zoom_bucket_switch_total += 1u;
            }
            ((DrawingProgramAppContext *)ctx)->runtime.render_zoom_bucket_percent = zoom_bucket_percent;
        }
    }
    drawing_program_visual_surface_cache_prune_for_project(&ctx->texture_project);
    hooks->compute_canvas_sheet_metrics(ctx, pane_rect, &metrics);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_SURFACE_1, &world_grid_minor);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_SURFACE_2, &world_grid_major);
    world_grid_minor = sdl_color_shift_by_luma(world_grid_minor, 8);
    world_grid_major = sdl_color_shift_by_luma(world_grid_major, 16);
    drawing_program_visual_canvas_world_draw_backdrop_immediate(
        renderer, pane_rect, pane_rect, world_grid_minor, world_grid_major, (SDL_Color){0u, 0u, 0u, 0u});

    drawing_program_visual_collect_layer_opacity_by_index(ctx, layer_opacity, DRAWING_PROGRAM_MAX_LAYERS);
    (void)SDL_RenderSetClipRect(renderer, &pane_rect);

    for (surface_index = 0u; surface_index < ctx->texture_project.surface_count; ++surface_index) {
        const DrawingProgramTextureSurface *surface =
            drawing_program_texture_project_surface_at(&ctx->texture_project, surface_index);
        SDL_Rect clip_sheet;
        SDL_Color border = (surface_index == ctx->texture_project.active_surface_index) ? active_sheet_border : sheet_border;
        if (!surface || !drawing_program_texture_workspace_surface_sheet_metrics(ctx, pane_rect, surface_index, &metrics)) {
            continue;
        }
        clip_sheet = metrics.sheet_rect;
        if (clip_sheet.x < pane_rect.x) {
            clip_sheet.w -= (pane_rect.x - clip_sheet.x);
            clip_sheet.x = pane_rect.x;
        }
        if (clip_sheet.y < pane_rect.y) {
            clip_sheet.h -= (pane_rect.y - clip_sheet.y);
            clip_sheet.y = pane_rect.y;
        }
        if (clip_sheet.x + clip_sheet.w > pane_rect.x + pane_rect.w) {
            clip_sheet.w = pane_rect.x + pane_rect.w - clip_sheet.x;
        }
        if (clip_sheet.y + clip_sheet.h > pane_rect.y + pane_rect.h) {
            clip_sheet.h = pane_rect.y + pane_rect.h - clip_sheet.y;
        }
        if (clip_sheet.w <= 0 || clip_sheet.h <= 0) {
            continue;
        }
        SDL_SetRenderDrawColor(renderer, sheet_fill.r, sheet_fill.g, sheet_fill.b, sheet_fill.a);
        (void)SDL_RenderFillRect(renderer, &clip_sheet);
        if (surface->storage) {
            DrawingProgramVisualSurfaceCacheRequest cache_request;
            DrawingProgramVisualSurfaceCacheTelemetry cache_telemetry;
            const DrawingProgramDocument *cache_document = &surface->storage->document;
            const DrawingProgramLayerRasterStore *cache_layer_rasters = &surface->storage->layer_rasters;
            SDL_Texture *surface_texture = 0;
            uint8_t active_surface = (surface_index == ctx->texture_project.active_surface_index) ? 1u : 0u;
            memset(&cache_request, 0, sizeof(cache_request));
            cache_request.project_epoch = ctx->texture_project.runtime_cache_epoch;
            cache_request.content_revision = surface->content_revision;
            cache_request.layer_opacity_revision = ctx->runtime.render_layer_opacity_revision;
            cache_request.surface_id = surface->surface_id;
            cache_request.zoom_bucket_percent = ctx->runtime.render_zoom_bucket_percent;
            cache_request.active_surface = active_surface;
            if (active_surface) {
                cache_document = &ctx->document;
                cache_layer_rasters = &ctx->layer_rasters;
                cache_request.content_revision =
                    drawing_program_render_revision_compose_active_surface_content(ctx);
                ((DrawingProgramAppContext *)ctx)->runtime.render_active_surface_content_revision =
                    cache_request.content_revision;
            }
            surface_texture = drawing_program_visual_surface_cache_sync(renderer,
                                                                       &cache_request,
                                                                       cache_document,
                                                                       cache_layer_rasters,
                                                                       layer_opacity,
                                                                       DRAWING_PROGRAM_MAX_LAYERS,
                                                                       &cache_telemetry);
            drawing_program_visual_canvas_world_note_cache_telemetry((DrawingProgramAppContext *)ctx,
                                                                     &cache_telemetry,
                                                                     active_surface);
            {
                uint32_t entry_count = drawing_program_visual_surface_cache_entry_count();
                if (entry_count > ctx->runtime.render_surface_cache_entry_peak) {
                    ((DrawingProgramAppContext *)ctx)->runtime.render_surface_cache_entry_peak = entry_count;
                }
            }
            if (surface_texture) {
                (void)SDL_RenderSetClipRect(renderer, &clip_sheet);
                (void)SDL_RenderCopy(renderer, surface_texture, 0, &metrics.sheet_rect);
            }
        }
        (void)SDL_RenderSetClipRect(renderer, &pane_rect);
        SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
        (void)SDL_RenderDrawRect(renderer, &metrics.sheet_rect);
        drawing_program_visual_draw_texture_net_guides(
            renderer, pane_rect, surface, &metrics, ctx->ui.canvas_guide_mode);
        if (ctx->ui.canvas_control_mode == (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT &&
            surface->is_blank &&
            !surface->resize_locked) {
            SDL_Rect handle_rect;
            SDL_Color handle_fill = (surface_index == ctx->texture_project.active_surface_index)
                                        ? (SDL_Color){ 244u, 186u, 96u, 255u }
                                        : (SDL_Color){ 116u, 126u, 154u, 255u };
            if (drawing_program_texture_canvas_resize_handle_rect_for_surface(
                    ctx, pane_rect, surface_index, &handle_rect)) {
                SDL_SetRenderDrawColor(renderer, handle_fill.r, handle_fill.g, handle_fill.b, handle_fill.a);
                (void)SDL_RenderFillRect(renderer, &handle_rect);
                SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
                (void)SDL_RenderDrawRect(renderer, &handle_rect);
            }
        }
        drawing_program_visual_draw_surface_label(
            renderer, pane_rect, surface, &metrics, surface_label, hooks);
    }
    if (!drawing_program_texture_workspace_active_sheet_metrics(ctx, pane_rect, &metrics)) {
        (void)SDL_RenderSetClipRect(renderer, 0);
        return;
    }
    drawing_program_visual_draw_reflection_overlay(renderer, pane_rect, ctx, &metrics, ui);
    hooks->draw_object_overlay(renderer, pane_rect, ctx, theme, &metrics, interaction, ui);
    hooks->draw_selection_overlay(renderer, pane_rect, ctx, theme, &metrics, selection);
    hooks->draw_shape_preview_overlay(renderer, pane_rect, ctx, theme, &metrics, interaction, ui);
    (void)SDL_RenderSetClipRect(renderer, 0);
}

void drawing_program_visual_canvas_world_backdrop_cache_shutdown(void) {
    drawing_program_visual_canvas_world_backdrop_cache_reset();
}
