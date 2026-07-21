#include "drawing_program/drawing_program_visual_indexed_canvas.h"

#include <stdio.h>

#include "drawing_program/drawing_program_indexed_editor.h"

void drawing_program_visual_draw_indexed_canvas(
    SDL_Renderer *renderer,
    SDL_Rect pane_rect,
    const DrawingProgramAppContext *ctx,
    const VisualCanvasSheetMetrics *metrics,
    int (*draw_bitmap_text)(SDL_Renderer *, SDL_Rect, int, int, const char *, SDL_Color, int)) {
    const DrawingProgramIndexedTilesetProfile *profile;
    const DrawingProgramIndexedLayerRaster *raster;
    uint32_t x;
    uint32_t y;
    uint8_t selected_slot;
    char status[160];
    if (!renderer || !metrics || !drawing_program_indexed_editor_is_active(ctx)) {
        return;
    }
    profile = &ctx->texture_project.indexed_profile;
    raster = &ctx->texture_project.indexed_raster;
    selected_slot = ctx->ui.indexed_selected_slot < profile->slot_count
        ? ctx->ui.indexed_selected_slot
        : profile->transparent_slot_index;
    (void)SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (y = 0u; y < raster->height; ++y) {
        for (x = 0u; x < raster->width; ++x) {
            uint8_t slot = raster->indices[y * raster->width + x];
            if ((uint32_t)slot >= profile->slot_count) {
                continue;
            }
            CoreAuthoredTextureRgba8 rgba = profile->slots[slot].preview_rgba;
            int left = metrics->sheet_rect.x + (int)((float)x * metrics->pixel_size);
            int top = metrics->sheet_rect.y + (int)((float)y * metrics->pixel_size);
            int right = metrics->sheet_rect.x + (int)((float)(x + 1u) * metrics->pixel_size);
            int bottom = metrics->sheet_rect.y + (int)((float)(y + 1u) * metrics->pixel_size);
            SDL_Rect pixel_rect = { left, top, right - left, bottom - top };
            if (rgba.a == 0u || pixel_rect.w <= 0 || pixel_rect.h <= 0) {
                continue;
            }
            SDL_SetRenderDrawColor(renderer, rgba.r, rgba.g, rgba.b, rgba.a);
            (void)SDL_RenderFillRect(renderer, &pixel_rect);
        }
    }
    if (metrics->pixel_size >= 6.0f) {
        SDL_SetRenderDrawColor(renderer, 28u, 31u, 38u, 150u);
        for (x = 1u; x < raster->width; ++x) {
            int sx = metrics->sheet_rect.x + (int)((float)x * metrics->pixel_size);
            (void)SDL_RenderDrawLine(renderer, sx, metrics->sheet_rect.y, sx,
                                     metrics->sheet_rect.y + metrics->sheet_rect.h);
        }
        for (y = 1u; y < raster->height; ++y) {
            int sy = metrics->sheet_rect.y + (int)((float)y * metrics->pixel_size);
            (void)SDL_RenderDrawLine(renderer, metrics->sheet_rect.x, sy,
                                     metrics->sheet_rect.x + metrics->sheet_rect.w, sy);
        }
    }
    if (draw_bitmap_text) {
        (void)snprintf(status,
                       sizeof(status),
                       "INDEXED ATLAS V1  %ux%u  CELL %ux%u  SLOT %u:%s",
                       (unsigned)raster->width,
                       (unsigned)raster->height,
                       (unsigned)profile->logical_cell_width,
                       (unsigned)profile->logical_cell_height,
                       (unsigned)selected_slot,
                       profile->slots[selected_slot].id);
        (void)draw_bitmap_text(renderer,
                               pane_rect,
                               pane_rect.x + 10,
                               pane_rect.y + 10,
                               status,
                               (SDL_Color){245u, 190u, 92u, 255u},
                               1);
    }
}
