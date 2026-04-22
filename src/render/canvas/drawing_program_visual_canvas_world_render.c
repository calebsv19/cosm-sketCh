#include "drawing_program/drawing_program_visual_canvas_world_render.h"

#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_resources.h"
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

static int drawing_program_visual_rect_equals(SDL_Rect a, SDL_Rect b) {
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

static int drawing_program_visual_color_equals(SDL_Color a, SDL_Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

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

static int drawing_program_visual_canvas_world_draw_backdrop_cached(
    SDL_Renderer *renderer,
    SDL_Rect pane_rect,
    SDL_Rect clip_sheet,
    SDL_Color world_grid_minor,
    SDL_Color world_grid_major,
    SDL_Color sheet_fill) {
    SDL_Texture *previous_target = 0;
    SDL_Rect local_clip_sheet;
    SDL_Rect local_pane = {0, 0, 0, 0};
    int cache_needs_redraw = 0;
    int px;
    int py;

    if (!renderer || pane_rect.w <= 0 || pane_rect.h <= 0) {
        return 0;
    }

    local_clip_sheet = clip_sheet;
    local_clip_sheet.x -= pane_rect.x;
    local_clip_sheet.y -= pane_rect.y;
    local_pane.w = pane_rect.w;
    local_pane.h = pane_rect.h;

    if (g_canvas_world_backdrop.renderer_owner != renderer ||
        g_canvas_world_backdrop.texture_w != pane_rect.w ||
        g_canvas_world_backdrop.texture_h != pane_rect.h) {
        drawing_program_visual_canvas_world_backdrop_cache_reset();
    }

    if (!g_canvas_world_backdrop.texture) {
        g_canvas_world_backdrop.texture = SDL_CreateTexture(renderer,
                                                            SDL_PIXELFORMAT_RGBA8888,
                                                            SDL_TEXTUREACCESS_TARGET,
                                                            pane_rect.w,
                                                            pane_rect.h);
        if (!g_canvas_world_backdrop.texture) {
            return 0;
        }
        g_canvas_world_backdrop.renderer_owner = renderer;
        g_canvas_world_backdrop.texture_w = pane_rect.w;
        g_canvas_world_backdrop.texture_h = pane_rect.h;
        g_canvas_world_backdrop.valid = 0u;
        (void)SDL_SetTextureBlendMode(g_canvas_world_backdrop.texture, SDL_BLENDMODE_BLEND);
    }

    cache_needs_redraw =
        (!g_canvas_world_backdrop.valid ||
         !drawing_program_visual_rect_equals(g_canvas_world_backdrop.local_clip_sheet, local_clip_sheet) ||
         !drawing_program_visual_color_equals(g_canvas_world_backdrop.world_grid_minor, world_grid_minor) ||
         !drawing_program_visual_color_equals(g_canvas_world_backdrop.world_grid_major, world_grid_major) ||
         !drawing_program_visual_color_equals(g_canvas_world_backdrop.sheet_fill, sheet_fill));
    if (cache_needs_redraw) {
        previous_target = SDL_GetRenderTarget(renderer);
        if (SDL_SetRenderTarget(renderer, g_canvas_world_backdrop.texture) != 0) {
            (void)SDL_SetRenderTarget(renderer, previous_target);
            return 0;
        }

        SDL_SetRenderDrawColor(renderer, 0u, 0u, 0u, 0u);
        (void)SDL_RenderClear(renderer);
        (void)SDL_RenderSetClipRect(renderer, &local_pane);
        SDL_SetRenderDrawColor(renderer, world_grid_minor.r, world_grid_minor.g, world_grid_minor.b, 255u);
        for (px = 0; px < local_pane.w; px += 24) {
            (void)SDL_RenderDrawLine(renderer, px, 0, px, local_pane.h);
        }
        for (py = 0; py < local_pane.h; py += 24) {
            (void)SDL_RenderDrawLine(renderer, 0, py, local_pane.w, py);
        }
        SDL_SetRenderDrawColor(renderer, world_grid_major.r, world_grid_major.g, world_grid_major.b, 255u);
        for (px = 0; px < local_pane.w; px += 96) {
            (void)SDL_RenderDrawLine(renderer, px, 0, px, local_pane.h);
        }
        for (py = 0; py < local_pane.h; py += 96) {
            (void)SDL_RenderDrawLine(renderer, 0, py, local_pane.w, py);
        }
        SDL_SetRenderDrawColor(renderer, sheet_fill.r, sheet_fill.g, sheet_fill.b, sheet_fill.a);
        (void)SDL_RenderFillRect(renderer, &local_clip_sheet);
        (void)SDL_RenderSetClipRect(renderer, 0);
        (void)SDL_SetRenderTarget(renderer, previous_target);

        g_canvas_world_backdrop.local_clip_sheet = local_clip_sheet;
        g_canvas_world_backdrop.world_grid_minor = world_grid_minor;
        g_canvas_world_backdrop.world_grid_major = world_grid_major;
        g_canvas_world_backdrop.sheet_fill = sheet_fill;
        g_canvas_world_backdrop.valid = 1u;
    }

    return SDL_RenderCopy(renderer, g_canvas_world_backdrop.texture, 0, &pane_rect) == 0 ? 1 : 0;
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
    SDL_Color world_grid_minor = { 42u, 48u, 66u, 255u };
    SDL_Color world_grid_major = { 52u, 58u, 78u, 255u };
    SDL_Color sheet_border = { 210u, 210u, 220u, 255u };
    SDL_Color sheet_fill = { 244u, 244u, 248u, 255u };
    SDL_Rect draw_sheet;
    SDL_Rect clip_sheet;
    if (!renderer || !ctx || !hooks || !hooks->compute_canvas_sheet_metrics || !hooks->draw_selection_overlay ||
        !hooks->draw_object_overlay || !hooks->draw_shape_preview_overlay) {
        return;
    }
    if (ctx->document.raster_width == 0u ||
        ctx->document.raster_height == 0u ||
        ctx->document.raster_sample_count == 0u) {
        return;
    }
    hooks->compute_canvas_sheet_metrics(ctx, pane_rect, &metrics);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_SURFACE_1, &world_grid_minor);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_SURFACE_2, &world_grid_major);
    world_grid_minor = sdl_color_shift_by_luma(world_grid_minor, 8);
    world_grid_major = sdl_color_shift_by_luma(world_grid_major, 16);

    draw_sheet = metrics.sheet_rect;
    clip_sheet = draw_sheet;
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
        (void)SDL_RenderSetClipRect(renderer, 0);
        return;
    }
    if (!drawing_program_visual_canvas_world_draw_backdrop_cached(renderer,
                                                                  pane_rect,
                                                                  clip_sheet,
                                                                  world_grid_minor,
                                                                  world_grid_major,
                                                                  sheet_fill)) {
        drawing_program_visual_canvas_world_draw_backdrop_immediate(renderer,
                                                                    pane_rect,
                                                                    clip_sheet,
                                                                    world_grid_minor,
                                                                    world_grid_major,
                                                                    sheet_fill);
    }

    drawing_program_visual_collect_layer_opacity_by_index(ctx, layer_opacity, DRAWING_PROGRAM_MAX_LAYERS);
    if (drawing_program_visual_canvas_texture_sync_with_signature(
            renderer,
            &ctx->document,
            &ctx->layer_rasters,
            layer_opacity,
            DRAWING_PROGRAM_MAX_LAYERS,
            ctx->runtime.render_canvas_last_raster_hash,
            ctx->runtime.render_canvas_last_nonzero_samples) &&
        drawing_program_visual_canvas_texture_get()) {
        (void)SDL_RenderSetClipRect(renderer, &clip_sheet);
        (void)SDL_RenderCopy(renderer, drawing_program_visual_canvas_texture_get(), 0, &draw_sheet);
    }
    (void)SDL_RenderSetClipRect(renderer, &pane_rect);
    SDL_SetRenderDrawColor(renderer, sheet_border.r, sheet_border.g, sheet_border.b, sheet_border.a);
    (void)SDL_RenderDrawRect(renderer, &draw_sheet);
    hooks->draw_object_overlay(renderer, pane_rect, ctx, theme, &metrics, interaction, ui);
    hooks->draw_selection_overlay(renderer, pane_rect, ctx, theme, &metrics, selection);
    hooks->draw_shape_preview_overlay(renderer, pane_rect, ctx, theme, &metrics, interaction, ui);
    (void)SDL_RenderSetClipRect(renderer, 0);
}

void drawing_program_visual_canvas_world_backdrop_cache_shutdown(void) {
    drawing_program_visual_canvas_world_backdrop_cache_reset();
}
