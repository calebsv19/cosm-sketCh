#ifndef DRAWING_PROGRAM_VISUAL_THEME_H
#define DRAWING_PROGRAM_VISUAL_THEME_H

#include <stdint.h>
#include <SDL2/SDL.h>

#include "core_base.h"
#include "core_theme.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VisualThemePalette {
    SDL_Color app_background;
    SDL_Color pane_background;
    SDL_Color pane_background_alt;
    SDL_Color pane_border;
    SDL_Color button_fill;
    SDL_Color button_fill_hover;
    SDL_Color button_fill_active;
    SDL_Color button_border;
    SDL_Color text_primary;
    SDL_Color text_muted;
    SDL_Color accent_primary;
    SDL_Color status_ok;
    SDL_Color status_warn;
    SDL_Color status_error;
} VisualThemePalette;

SDL_Color sdl_color_ensure_contrast(SDL_Color preferred, SDL_Color background);
SDL_Color sdl_color_shift_by_luma(SDL_Color c, int amount);
CoreResult resolve_theme_color(const CoreThemePreset *theme,
                               CoreThemeColorToken token,
                               SDL_Color *out_color);
void resolve_visual_theme_palette(const CoreThemePreset *theme, VisualThemePalette *out);
void module_color(uint32_t module_type_id,
                  const CoreThemePreset *theme,
                  SDL_Color *out_fill,
                  SDL_Color *out_border);

#ifdef __cplusplus
}
#endif

#endif
