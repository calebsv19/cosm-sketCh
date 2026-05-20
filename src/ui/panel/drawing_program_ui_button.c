#include "drawing_program_ui_button.h"

#include "drawing_program/drawing_program_visual_theme.h"

static CoreThemeColor drawing_program_ui_button_core_color(SDL_Color color) {
    CoreThemeColor out;
    out.r = color.r;
    out.g = color.g;
    out.b = color.b;
    out.a = color.a;
    return out;
}

static SDL_Color drawing_program_ui_button_sdl_color(CoreThemeColor color) {
    return (SDL_Color){ color.r, color.g, color.b, color.a };
}

static int drawing_program_ui_button_fill_edge(SDL_Renderer *renderer,
                                               int x,
                                               int y,
                                               int w,
                                               int h,
                                               SDL_Color color) {
    SDL_Rect rect = { x, y, w, h };

    if (!renderer || w <= 0 || h <= 0) {
        return 0;
    }
    if (SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a) != 0) {
        return 0;
    }
    return SDL_RenderFillRect(renderer, &rect) == 0;
}

static int drawing_program_ui_button_draw_outline(SDL_Renderer *renderer,
                                                  const SDL_Rect *rect,
                                                  SDL_Color color) {
    if (!renderer || !rect || rect->w <= 0 || rect->h <= 0) {
        return 0;
    }
    if (!drawing_program_ui_button_fill_edge(renderer, rect->x, rect->y, rect->w, 1, color)) {
        return 0;
    }
    if (rect->h > 1 &&
        !drawing_program_ui_button_fill_edge(renderer,
                                             rect->x,
                                             rect->y + rect->h - 1,
                                             rect->w,
                                             1,
                                             color)) {
        return 0;
    }
    if (rect->h > 2 &&
        !drawing_program_ui_button_fill_edge(renderer, rect->x, rect->y + 1, 1, rect->h - 2, color)) {
        return 0;
    }
    if (rect->w > 1 && rect->h > 2 &&
        !drawing_program_ui_button_fill_edge(renderer,
                                             rect->x + rect->w - 1,
                                             rect->y + 1,
                                             1,
                                             rect->h - 2,
                                             color)) {
        return 0;
    }
    return 1;
}

void drawing_program_ui_button_state_init(DrawingProgramUiButtonState *state) {
    kit_ui_button_state_init(state);
}

void drawing_program_ui_button_spec_init(DrawingProgramUiButtonSpec *spec, const char *label) {
    kit_ui_button_spec_init(spec, label);
}

int drawing_program_ui_button_style_resolve(SDL_Color fill,
                                            SDL_Color fill_hover,
                                            SDL_Color fill_active,
                                            SDL_Color border,
                                            SDL_Color text_primary,
                                            SDL_Color text_muted,
                                            const DrawingProgramUiButtonSpec *spec,
                                            DrawingProgramUiButtonStyle *out_style) {
    KitUiButtonTheme theme;

    if (!spec || !out_style) {
        return 1;
    }

    theme.idle_fill = drawing_program_ui_button_core_color(fill);
    theme.selected_fill = drawing_program_ui_button_core_color(fill_active);
    theme.hover_fill = drawing_program_ui_button_core_color(fill_hover);
    theme.positive_fill = drawing_program_ui_button_core_color(fill_active);
    theme.outline_idle = drawing_program_ui_button_core_color(border);
    theme.outline_highlight = drawing_program_ui_button_core_color(border);
    theme.text_primary = drawing_program_ui_button_core_color(text_primary);
    theme.text_muted = drawing_program_ui_button_core_color(text_muted);
    return kit_ui_button_style_resolve(&theme, spec, out_style) != 0 ? 0 : 1;
}

int drawing_program_ui_button_draw_frame(SDL_Renderer *renderer,
                                         SDL_Rect rect,
                                         const DrawingProgramUiButtonStyle *style) {
    SDL_Color bg;
    SDL_Color outline;

    if (!renderer || !style || rect.w <= 0 || rect.h <= 0) {
        return 1;
    }

    bg = drawing_program_ui_button_sdl_color(style->fill);
    outline = drawing_program_ui_button_sdl_color(style->outline);
    if (SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a) != 0 ||
        SDL_RenderFillRect(renderer, &rect) != 0 ||
        !drawing_program_ui_button_draw_outline(renderer, &rect, outline)) {
        return 1;
    }
    return 0;
}

int drawing_program_ui_button_draw(SDL_Renderer *renderer,
                                   SDL_Rect clip_rect,
                                   SDL_Rect rect,
                                   const char *label,
                                   SDL_Color fill,
                                   SDL_Color fill_hover,
                                   SDL_Color fill_active,
                                   SDL_Color border,
                                   SDL_Color text_primary,
                                   SDL_Color text_muted,
                                   int text_scale,
                                   int selected,
                                   int hovered,
                                   DrawingProgramUiButtonVariant variant,
                                   const DrawingProgramVisualPanelRenderHooks *hooks) {
    DrawingProgramUiButtonSpec spec;
    DrawingProgramUiButtonStyle style;
    SDL_Color bg;
    SDL_Color text_final;
    int glyph_h;
    int text_y;
    int text_w;
    int text_x;

    if (!renderer || !label || !hooks || !hooks->draw_bitmap_text || !hooks->measure_bitmap_text_width ||
        rect.w <= 0 || rect.h <= 0) {
        return 1;
    }

    drawing_program_ui_button_spec_init(&spec, label);
    spec.variant = variant;
    spec.state.selected = selected ? 1 : 0;
    spec.state.focused = selected ? 1 : 0;
    spec.state.hovered = hovered ? 1 : 0;
    if (drawing_program_ui_button_style_resolve(fill,
                                                fill_hover,
                                                fill_active,
                                                border,
                                                text_primary,
                                                text_muted,
                                                &spec,
                                                &style) != 0) {
        return 1;
    }

    bg = drawing_program_ui_button_sdl_color(style.fill);
    text_final = sdl_color_ensure_contrast(drawing_program_ui_button_sdl_color(style.text), bg);
    glyph_h = 7 * text_scale;
    text_w = hooks->measure_bitmap_text_width(label, text_scale);
    text_x = rect.x + ((rect.w - text_w) / 2);
    if (text_x < rect.x + 6) {
        text_x = rect.x + 6;
    }
    text_y = rect.y + ((rect.h - glyph_h) / 2);
    if (text_y < rect.y + 2) {
        text_y = rect.y + 2;
    }

    if (drawing_program_ui_button_draw_frame(renderer, rect, &style) != 0) {
        return 1;
    }

    hooks->draw_bitmap_text(renderer, clip_rect, text_x, text_y, label, text_final, text_scale);
    return 0;
}
