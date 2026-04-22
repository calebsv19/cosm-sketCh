#ifndef DRAWING_PROGRAM_VISUAL_RESOURCES_H
#define DRAWING_PROGRAM_VISUAL_RESOURCES_H

#include <stdint.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "core_font.h"

struct DrawingProgramDocument;
struct DrawingProgramLayerRasterStore;

TTF_Font *drawing_program_visual_get_ttf_font_for_preset(uint32_t ui_font_preset_id, int scale);
int drawing_program_visual_font_backend_is_ready(void);
void drawing_program_visual_font_cache_shutdown(void);

int drawing_program_visual_canvas_texture_sync(SDL_Renderer *renderer,
                                               const struct DrawingProgramDocument *document,
                                               const struct DrawingProgramLayerRasterStore *layer_rasters,
                                               const uint8_t *layer_opacity_percent,
                                               uint32_t layer_opacity_count);
int drawing_program_visual_canvas_texture_sync_with_signature(
    SDL_Renderer *renderer,
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    uint32_t raster_hash32,
    uint32_t raster_nonzero_count);
SDL_Texture *drawing_program_visual_canvas_texture_get(void);
void drawing_program_visual_canvas_texture_shutdown(void);

#endif
