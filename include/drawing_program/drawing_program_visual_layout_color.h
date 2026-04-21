#ifndef DRAWING_PROGRAM_VISUAL_LAYOUT_COLOR_H
#define DRAWING_PROGRAM_VISUAL_LAYOUT_COLOR_H

#include <stdint.h>
#include <SDL2/SDL.h>

typedef struct VisualPaneLayoutMetrics VisualPaneLayoutMetrics;

#ifdef __cplusplus
extern "C" {
#endif

int right_color_content_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_color_active_swatch_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_color_recent_swatch_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint8_t recent_index);
SDL_Rect right_color_hue_slider_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_color_sv_grid_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_color_palette_swatch_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint8_t color_index);

#ifdef __cplusplus
}
#endif

#endif
