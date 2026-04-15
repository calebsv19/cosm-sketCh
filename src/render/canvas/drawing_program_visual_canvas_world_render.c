#include "drawing_program/drawing_program_visual_canvas_world_render.h"

#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_resources.h"
#include "drawing_program/drawing_program_visual_theme.h"

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
    int px;
    int py;
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

    SDL_SetRenderDrawColor(renderer, sheet_fill.r, sheet_fill.g, sheet_fill.b, sheet_fill.a);
    (void)SDL_RenderFillRect(renderer, &clip_sheet);

    drawing_program_visual_collect_layer_opacity_by_index(ctx, layer_opacity, DRAWING_PROGRAM_MAX_LAYERS);
    if (drawing_program_visual_canvas_texture_sync(renderer,
                                                   &ctx->document,
                                                   &ctx->layer_rasters,
                                                   layer_opacity,
                                                   DRAWING_PROGRAM_MAX_LAYERS) &&
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
