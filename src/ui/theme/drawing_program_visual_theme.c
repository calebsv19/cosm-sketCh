#include "drawing_program/drawing_program_visual_theme.h"

static SDL_Color sdl_color_from_theme(CoreThemeColor c) {
    SDL_Color out = { c.r, c.g, c.b, c.a };
    return out;
}

static SDL_Color sdl_color_adjust(SDL_Color c, int dr, int dg, int db) {
    int r = (int)c.r + dr;
    int g = (int)c.g + dg;
    int b = (int)c.b + db;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    c.r = (uint8_t)r;
    c.g = (uint8_t)g;
    c.b = (uint8_t)b;
    return c;
}

static int sdl_color_luma(SDL_Color c) {
    return ((int)c.r * 2126) + ((int)c.g * 7152) + ((int)c.b * 722);
}

static int sdl_color_contrast_distance(SDL_Color a, SDL_Color b) {
    int la = sdl_color_luma(a);
    int lb = sdl_color_luma(b);
    return (la > lb) ? (la - lb) : (lb - la);
}

SDL_Color sdl_color_ensure_contrast(SDL_Color preferred, SDL_Color background) {
    SDL_Color white = { 244u, 244u, 248u, 255u };
    SDL_Color black = { 18u, 20u, 24u, 255u };
    if (sdl_color_contrast_distance(preferred, background) >= 2400) {
        return preferred;
    }
    if (sdl_color_contrast_distance(white, background) >= sdl_color_contrast_distance(black, background)) {
        return white;
    }
    return black;
}

SDL_Color sdl_color_shift_by_luma(SDL_Color c, int amount) {
    if (sdl_color_luma(c) < 1280000) {
        return sdl_color_adjust(c, amount, amount, amount);
    }
    return sdl_color_adjust(c, -amount, -amount, -amount);
}

CoreResult resolve_theme_color(const CoreThemePreset *theme,
                               CoreThemeColorToken token,
                               SDL_Color *out_color) {
    CoreThemeColor color;
    CoreResult result;
    if (!theme || !out_color) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid theme resolve request" };
    }
    result = core_theme_get_color(theme, token, &color);
    if (result.code != CORE_OK) {
        return result;
    }
    *out_color = sdl_color_from_theme(color);
    return core_result_ok();
}

void resolve_visual_theme_palette(const CoreThemePreset *theme, VisualThemePalette *out) {
    SDL_Color surface0 = { 18u, 20u, 28u, 255u };
    SDL_Color surface1 = { 24u, 26u, 34u, 255u };
    SDL_Color surface2 = { 34u, 38u, 50u, 255u };
    SDL_Color text_primary = { 230u, 232u, 238u, 255u };
    SDL_Color text_muted = { 164u, 170u, 182u, 255u };
    SDL_Color accent = { 120u, 160u, 220u, 255u };
    SDL_Color ok = { 130u, 170u, 100u, 255u };
    SDL_Color warn = { 255u, 190u, 140u, 255u };
    SDL_Color err = { 180u, 60u, 60u, 255u };
    if (!out) {
        return;
    }
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_SURFACE_0, &surface0);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_SURFACE_1, &surface1);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_SURFACE_2, &surface2);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_TEXT_PRIMARY, &text_primary);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_TEXT_MUTED, &text_muted);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_ACCENT_PRIMARY, &accent);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_STATUS_OK, &ok);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_STATUS_WARN, &warn);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_STATUS_ERROR, &err);

    out->app_background = surface0;
    out->pane_background = surface1;
    out->pane_background_alt = surface2;
    out->pane_border = sdl_color_shift_by_luma(surface2, 10);
    out->button_fill = surface2;
    out->button_fill_hover = sdl_color_shift_by_luma(surface2, 8);
    out->button_fill_active = sdl_color_shift_by_luma(surface2, 16);
    out->button_border = sdl_color_shift_by_luma(surface2, 22);
    out->text_primary = text_primary;
    out->text_muted = text_muted;
    out->accent_primary = accent;
    out->status_ok = ok;
    out->status_warn = warn;
    out->status_error = err;
}

void module_color(uint32_t module_type_id,
                  const CoreThemePreset *theme,
                  SDL_Color *out_fill,
                  SDL_Color *out_border) {
    VisualThemePalette p;
    if (!theme || !out_fill || !out_border) {
        return;
    }
    resolve_visual_theme_palette(theme, &p);
    if (module_type_id == 3u) {
        *out_fill = p.pane_background;
        *out_border = p.pane_border;
        return;
    }
    if (module_type_id == 2u || module_type_id == 4u) {
        *out_fill = p.pane_background;
        *out_border = p.pane_border;
        return;
    }
    if (module_type_id == 1u) {
        *out_fill = p.pane_background_alt;
        *out_border = p.pane_border;
        return;
    }
    *out_fill = p.app_background;
    *out_border = p.pane_border;
}
