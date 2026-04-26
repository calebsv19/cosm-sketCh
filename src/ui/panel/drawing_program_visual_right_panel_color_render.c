#include "drawing_program/drawing_program_color_panel_internal.h"

#include <stdio.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_visual_panel_render_common.h"

static void draw_color_swatch(SDL_Renderer *renderer, SDL_Rect rect, uint8_t r, uint8_t g, uint8_t b) {
    if (!renderer) {
        return;
    }
    SDL_SetRenderDrawColor(renderer, r, g, b, 255u);
    (void)SDL_RenderFillRect(renderer, &rect);
}

static void draw_hue_slider(SDL_Renderer *renderer, SDL_Rect rect) {
    int x;
    if (!renderer || rect.w <= 0 || rect.h <= 0) {
        return;
    }
    for (x = 0; x < rect.w; ++x) {
        uint8_t hue = (rect.w > 1) ? (uint8_t)((x * 255) / (rect.w - 1)) : 0u;
        uint8_t r = 0u;
        uint8_t g = 0u;
        uint8_t b = 0u;
        drawing_program_color_hsv_to_rgb(hue, 255u, 255u, &r, &g, &b);
        SDL_SetRenderDrawColor(renderer, r, g, b, 255u);
        (void)SDL_RenderDrawLine(renderer, rect.x + x, rect.y, rect.x + x, rect.y + rect.h - 1);
    }
}

static void draw_sv_grid(SDL_Renderer *renderer, SDL_Rect rect, uint8_t hue) {
    int y_px;
    if (!renderer || rect.w <= 0 || rect.h <= 0) {
        return;
    }
    for (y_px = 0; y_px < rect.h; ++y_px) {
        uint8_t value = (rect.h > 1) ? (uint8_t)(255u - ((y_px * 255) / (rect.h - 1))) : 255u;
        int x;
        for (x = 0; x < rect.w; ++x) {
            uint8_t saturation = (rect.w > 1) ? (uint8_t)((x * 255) / (rect.w - 1)) : 0u;
            uint8_t r = 0u;
            uint8_t g = 0u;
            uint8_t b = 0u;
            drawing_program_color_hsv_to_rgb(hue, saturation, value, &r, &g, &b);
            SDL_SetRenderDrawColor(renderer, r, g, b, 255u);
            (void)SDL_RenderDrawPoint(renderer, rect.x + x, rect.y + y_px);
        }
    }
}

void drawing_program_visual_render_right_panel_color_tab(SDL_Renderer *renderer,
                                                         SDL_Rect rect,
                                                         int y,
                                                         const DrawingProgramAppContext *ctx,
                                                         const VisualPanelUiState *ui,
                                                         VisualPaneLayoutMetrics m,
                                                         VisualThemePalette p,
                                                         const DrawingProgramVisualPanelRenderHooks *hooks) {
    char line[160];
    uint8_t active_color_index;
    uint8_t swatch_r = 0u;
    uint8_t swatch_g = 0u;
    uint8_t swatch_b = 0u;
    SDL_Rect swatch_rect;
    SDL_Rect save_button_rect;
    SDL_Rect hue_rect;
    SDL_Rect sv_rect;
    SDL_Rect marker_rect;
    int recent_label_y;
    int hsv_line_y;
    int preset_label_y;
    uint8_t recent_i;
    uint8_t palette_i;

    if (!renderer || !ctx || !hooks || !hooks->draw_bitmap_text || !hooks->color_index_clamp || !hooks->color_rgb_from_index) {
        return;
    }

    active_color_index = hooks->color_index_clamp(ctx->ui.active_color_index);
    swatch_rect = right_color_active_swatch_rect(rect, m);
    save_button_rect = right_color_save_preset_button_rect(rect, m);
    hue_rect = right_color_hue_slider_rect(rect, m);
    sv_rect = right_color_sv_grid_rect(rect, m);

    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "COLOR SYSTEM", p.text_primary, m.body_scale);
    y += m.line_h;
    (void)snprintf(line,
                   sizeof(line),
                   "ACTIVE PAINT  TARGET PRESET C%u",
                   (unsigned)active_color_index + 1u);
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
    y += m.line_h;
    (void)snprintf(line,
                   sizeof(line),
                   "RGB %u,%u,%u",
                   (unsigned)ctx->ui.active_paint_r,
                   (unsigned)ctx->ui.active_paint_g,
                   (unsigned)ctx->ui.active_paint_b);
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
    swatch_r = ctx->ui.active_paint_r;
    swatch_g = ctx->ui.active_paint_g;
    swatch_b = ctx->ui.active_paint_b;
    draw_color_swatch(renderer, swatch_rect, swatch_r, swatch_g, swatch_b);
    SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
    (void)SDL_RenderDrawRect(renderer, &swatch_rect);
    preset_label_y = swatch_rect.y + swatch_rect.h + m.section_gap;
    hooks->color_rgb_from_index(active_color_index, &swatch_r, &swatch_g, &swatch_b);
    (void)snprintf(line,
                   sizeof(line),
                   "PRESET RGB %u,%u,%u",
                   (unsigned)swatch_r,
                   (unsigned)swatch_g,
                   (unsigned)swatch_b);
    hooks->draw_bitmap_text(renderer,
                            rect,
                            rect.x + m.pad_x,
                            preset_label_y,
                            line,
                            p.text_muted,
                            m.body_scale);
    (void)snprintf(line, sizeof(line), "SAVE PAINT TO C%u", (unsigned)active_color_index + 1u);
    drawing_program_visual_panel_draw_tab_button(renderer,
                                                 rect,
                                                 save_button_rect,
                                                 line,
                                                 p.button_fill,
                                                 p.button_fill_hover,
                                                 p.button_fill_active,
                                                 p.button_border,
                                                 p.text_primary,
                                                 m.body_scale,
                                                 0,
                                                 drawing_program_visual_panel_ui_hovered(ui, save_button_rect, hooks),
                                                 hooks);
    hsv_line_y = save_button_rect.y + save_button_rect.h + m.section_gap;
    (void)snprintf(line,
                   sizeof(line),
                   "H:%u S:%u V:%u",
                   (unsigned)ctx->ui.color_hue,
                   (unsigned)ctx->ui.color_saturation,
                   (unsigned)ctx->ui.color_value);
    hooks->draw_bitmap_text(renderer,
                            rect,
                            rect.x + m.pad_x,
                            hsv_line_y,
                            line,
                            p.text_primary,
                            m.body_scale);

    recent_label_y = hsv_line_y + m.line_h + m.section_gap;
    hooks->draw_bitmap_text(renderer,
                            rect,
                            rect.x + m.pad_x,
                            recent_label_y,
                            "PAINT SLOTS",
                            p.text_primary,
                            m.body_scale);
    for (recent_i = 0u; recent_i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++recent_i) {
        SDL_Rect recent_rect = right_color_recent_swatch_rect(rect, m, recent_i);
        uint8_t rr = ctx->ui.recent_color_rgb[recent_i][0];
        uint8_t rg = ctx->ui.recent_color_rgb[recent_i][1];
        uint8_t rb = ctx->ui.recent_color_rgb[recent_i][2];
        draw_color_swatch(renderer, recent_rect, rr, rg, rb);
        SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
        (void)SDL_RenderDrawRect(renderer, &recent_rect);
        if (recent_i == ctx->ui.selected_recent_color_index) {
            SDL_Rect inner = { recent_rect.x + 1, recent_rect.y + 1, recent_rect.w - 2, recent_rect.h - 2 };
            SDL_SetRenderDrawColor(renderer,
                                   p.accent_primary.r,
                                   p.accent_primary.g,
                                   p.accent_primary.b,
                                   p.accent_primary.a);
            (void)SDL_RenderDrawRect(renderer, &inner);
        }
    }

    hooks->draw_bitmap_text(renderer,
                            rect,
                            rect.x + m.pad_x,
                            hue_rect.y - m.line_h,
                            "HUE",
                            p.text_primary,
                            m.body_scale);
    draw_hue_slider(renderer, hue_rect);
    SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
    (void)SDL_RenderDrawRect(renderer, &hue_rect);
    marker_rect = (SDL_Rect){ hue_rect.x + ((int)ctx->ui.color_hue * (hue_rect.w - 1)) / 255 - 1,
                              hue_rect.y - 2,
                              3,
                              hue_rect.h + 4 };
    SDL_SetRenderDrawColor(renderer, p.text_primary.r, p.text_primary.g, p.text_primary.b, p.text_primary.a);
    (void)SDL_RenderDrawRect(renderer, &marker_rect);

    hooks->draw_bitmap_text(renderer,
                            rect,
                            rect.x + m.pad_x,
                            sv_rect.y - m.line_h,
                            "SATURATION / VALUE",
                            p.text_primary,
                            m.body_scale);
    draw_sv_grid(renderer, sv_rect, ctx->ui.color_hue);
    SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
    (void)SDL_RenderDrawRect(renderer, &sv_rect);
    marker_rect = (SDL_Rect){ sv_rect.x + ((int)ctx->ui.color_saturation * (sv_rect.w - 1)) / 255 - 2,
                              sv_rect.y + ((255 - (int)ctx->ui.color_value) * (sv_rect.h - 1)) / 255 - 2,
                              5,
                              5 };
    SDL_SetRenderDrawColor(renderer, p.text_primary.r, p.text_primary.g, p.text_primary.b, p.text_primary.a);
    (void)SDL_RenderDrawRect(renderer, &marker_rect);

    hooks->draw_bitmap_text(renderer,
                            rect,
                            rect.x + m.pad_x,
                            sv_rect.y + sv_rect.h + m.section_gap,
                            "PRESET SWATCHES",
                            p.text_primary,
                            m.body_scale);
    for (palette_i = 0u; palette_i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++palette_i) {
        SDL_Rect palette_rect = right_color_palette_swatch_rect(rect, m, palette_i);
        hooks->color_rgb_from_index(palette_i, &swatch_r, &swatch_g, &swatch_b);
        draw_color_swatch(renderer, palette_rect, swatch_r, swatch_g, swatch_b);
        SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
        (void)SDL_RenderDrawRect(renderer, &palette_rect);
        if (palette_i == active_color_index) {
            SDL_SetRenderDrawColor(renderer,
                                   p.accent_primary.r,
                                   p.accent_primary.g,
                                   p.accent_primary.b,
                                   p.accent_primary.a);
            (void)SDL_RenderDrawRect(renderer, &palette_rect);
            SDL_Rect inner = { palette_rect.x + 1, palette_rect.y + 1, palette_rect.w - 2, palette_rect.h - 2 };
            (void)SDL_RenderDrawRect(renderer, &inner);
        }
        (void)snprintf(line, sizeof(line), "C%u", (unsigned)palette_i + 1u);
        hooks->draw_bitmap_text(renderer,
                                rect,
                                palette_rect.x + 4,
                                palette_rect.y + ((palette_rect.h - m.body_glyph_h) / 2),
                                line,
                                p.text_primary,
                                m.body_scale);
    }
}
