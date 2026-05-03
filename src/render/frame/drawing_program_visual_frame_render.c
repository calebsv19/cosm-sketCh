#include "drawing_program/drawing_program_visual_frame_render.h"

#include "drawing_program/drawing_program_visual_theme.h"

static void draw_splitter_overlay(SDL_Renderer *renderer,
                                  const DrawingProgramAppContext *ctx,
                                  const CoreThemePreset *theme) {
    CorePaneRect splitter_bounds;
    SDL_Color accent = { 120u, 160u, 220u, 255u };
    SDL_Color border = { 180u, 196u, 228u, 255u };
    SDL_Rect rect;
    int hovered = 0;
    int active = 0;
    uint8_t fill_alpha = 96u;
    uint8_t border_alpha = 180u;

    if (!renderer || !ctx) {
        return;
    }
    if (!drawing_program_pane_host_visible_splitter(ctx, &splitter_bounds, &hovered, &active)) {
        return;
    }
    if (!hovered && !active) {
        return;
    }
    if (theme) {
        (void)resolve_theme_color(theme, CORE_THEME_COLOR_ACCENT_PRIMARY, &accent);
        border = sdl_color_ensure_contrast(accent, accent);
    }
    if (active) {
        fill_alpha = 156u;
        border_alpha = 255u;
    }
    rect.x = (int)splitter_bounds.x;
    rect.y = (int)splitter_bounds.y;
    rect.w = (int)splitter_bounds.width;
    rect.h = (int)splitter_bounds.height;
    if (rect.w < 1) {
        rect.w = 1;
    }
    if (rect.h < 1) {
        rect.h = 1;
    }
    SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, fill_alpha);
    (void)SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border_alpha);
    (void)SDL_RenderDrawRect(renderer, &rect);
}

int drawing_program_visual_draw_frame(SDL_Window *window,
                                      SDL_Renderer *renderer,
                                      const DrawingProgramAppContext *ctx,
                                      const CoreThemePreset *theme,
                                      const VisualPanelUiState *ui,
                                      const VisualSelectionState *selection,
                                      const VisualCanvasInteractionState *interaction,
                                      const DrawingProgramVisualFrameRenderHooks *hooks) {
    int width = 0;
    int height = 0;
    uint32_t i;
    SDL_Color background;
    if (!window || !renderer || !ctx || !hooks || !hooks->module_type_for_pane ||
        !hooks->draw_menu_bar_chrome || !hooks->draw_left_panel_chrome ||
        !hooks->draw_right_panel_chrome || !hooks->draw_canvas_world_view ||
        !hooks->draw_canvas_viewport_chrome) {
        return 0;
    }
    if (ctx->overlay_adapter.lifecycle_state != DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE ||
        ctx->overlay_adapter.runtime_paused) {
        return 0;
    }
    if (ctx->pane_host.leaf_count == 0u) {
        return 0;
    }

    (void)window;
    if (SDL_GetRendererOutputSize(renderer, &width, &height) != 0) {
        return 0;
    }
    if (width <= 0 || height <= 0) {
        return 0;
    }

    if (!theme || resolve_theme_color(theme, CORE_THEME_COLOR_SURFACE_0, &background).code != CORE_OK) {
        background = (SDL_Color){ 18u, 20u, 28u, 255u };
    }
    SDL_SetRenderDrawColor(renderer, background.r, background.g, background.b, background.a);
    if (SDL_RenderClear(renderer) != 0) {
        return 0;
    }

    for (i = 0u; i < ctx->pane_host.leaf_count; ++i) {
        const CorePaneLeafRect *leaf = &ctx->pane_host.leaves[i];
        uint32_t module_type_id = hooks->module_type_for_pane(ctx, (uint32_t)leaf->id);
        SDL_Color fill;
        SDL_Color border;
        SDL_Rect rect;
        module_color(module_type_id, theme, &fill, &border);
        rect.x = (int)(leaf->rect.x);
        rect.y = (int)(leaf->rect.y);
        rect.w = (int)(leaf->rect.width);
        rect.h = (int)(leaf->rect.height);
        if (rect.w < 2) {
            rect.w = 2;
        }
        if (rect.h < 2) {
            rect.h = 2;
        }
        SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
        (void)SDL_RenderFillRect(renderer, &rect);
        if (module_type_id == 3u) {
            hooks->draw_menu_bar_chrome(renderer, rect, ctx, theme);
        } else if (module_type_id == 2u) {
            hooks->draw_left_panel_chrome(renderer, rect, ctx, theme, ui);
        } else if (module_type_id == 4u) {
            hooks->draw_right_panel_chrome(renderer, rect, ctx, theme, ui, selection, interaction);
        } else if (module_type_id == 1u) {
            hooks->draw_canvas_world_view(renderer, rect, ctx, theme, selection, ui, interaction);
            hooks->draw_canvas_viewport_chrome(renderer, rect, ctx, theme);
        }
        SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
        (void)SDL_RenderDrawRect(renderer, &rect);
    }
    draw_splitter_overlay(renderer, ctx, theme);

    return 1;
}
