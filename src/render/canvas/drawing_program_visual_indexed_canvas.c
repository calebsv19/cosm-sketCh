#include "drawing_program/drawing_program_visual_indexed_canvas.h"

#include <stdio.h>

#include "drawing_program/drawing_program_indexed_editor.h"

typedef struct DrawingProgramIndexedCanvasClipState {
    SDL_bool enabled;
    SDL_Rect rect;
} DrawingProgramIndexedCanvasClipState;

static int drawing_program_visual_indexed_canvas_edge(const VisualCanvasSheetMetrics *metrics,
                                                      uint32_t coordinate) {
    return metrics->sheet_rect.x + (int)((float)coordinate * metrics->pixel_size);
}

static int drawing_program_visual_indexed_canvas_vertical_edge(const VisualCanvasSheetMetrics *metrics,
                                                               uint32_t coordinate) {
    return metrics->sheet_rect.y + (int)((float)coordinate * metrics->pixel_size);
}

static DrawingProgramIndexedCanvasClipState drawing_program_visual_indexed_canvas_begin_clip(
    SDL_Renderer *renderer,
    SDL_Rect pane_rect) {
    DrawingProgramIndexedCanvasClipState state = { SDL_FALSE, { 0, 0, 0, 0 } };
    SDL_Rect effective = pane_rect;
    state.enabled = SDL_RenderIsClipEnabled(renderer);
    if (state.enabled) {
        SDL_RenderGetClipRect(renderer, &state.rect);
        if (!SDL_IntersectRect(&state.rect, &pane_rect, &effective)) {
            effective = (SDL_Rect){ 0, 0, 0, 0 };
        }
    }
    (void)SDL_RenderSetClipRect(renderer, &effective);
    return state;
}

static void drawing_program_visual_indexed_canvas_restore_clip(
    SDL_Renderer *renderer,
    const DrawingProgramIndexedCanvasClipState *state) {
    (void)SDL_RenderSetClipRect(renderer, state->enabled ? &state->rect : 0);
}

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
    DrawingProgramIndexedCanvasClipState clip_state;
    if (!renderer || !metrics || !drawing_program_indexed_editor_is_active(ctx)) {
        return;
    }
    profile = &ctx->texture_project.indexed_profile;
    raster = &ctx->texture_project.indexed_raster;
    selected_slot = ctx->ui.indexed_selected_slot < profile->slot_count
        ? ctx->ui.indexed_selected_slot
        : profile->transparent_slot_index;
    /* The atlas may be much larger than its pane at deep zoom; own the pane clip. */
    clip_state = drawing_program_visual_indexed_canvas_begin_clip(renderer, pane_rect);
    (void)SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (y = 0u; y < raster->height; ++y) {
        for (x = 0u; x < raster->width; ++x) {
            uint8_t slot = raster->indices[y * raster->width + x];
            if ((uint32_t)slot >= profile->slot_count) {
                continue;
            }
            CoreAuthoredTextureRgba8 rgba = profile->slots[slot].preview_rgba;
            int left = drawing_program_visual_indexed_canvas_edge(metrics, x);
            int top = drawing_program_visual_indexed_canvas_vertical_edge(metrics, y);
            int right = drawing_program_visual_indexed_canvas_edge(metrics, x + 1u);
            int bottom = drawing_program_visual_indexed_canvas_vertical_edge(metrics, y + 1u);
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
            int sx = drawing_program_visual_indexed_canvas_edge(metrics, x);
            (void)SDL_RenderDrawLine(renderer, sx, metrics->sheet_rect.y, sx,
                                     drawing_program_visual_indexed_canvas_vertical_edge(metrics, raster->height));
        }
        for (y = 1u; y < raster->height; ++y) {
            int sy = drawing_program_visual_indexed_canvas_vertical_edge(metrics, y);
            (void)SDL_RenderDrawLine(renderer, metrics->sheet_rect.x, sy,
                                     drawing_program_visual_indexed_canvas_edge(metrics, raster->width), sy);
        }
    }
    for (x = 0u; x < ctx->texture_project.indexed_cells.count; ++x) {
        const DrawingProgramIndexedCell *cell = &ctx->texture_project.indexed_cells.cells[x];
        SDL_Rect cell_rect = {
            drawing_program_visual_indexed_canvas_edge(metrics, cell->x),
            drawing_program_visual_indexed_canvas_vertical_edge(metrics, cell->y),
            drawing_program_visual_indexed_canvas_edge(metrics, cell->x + cell->width) -
                drawing_program_visual_indexed_canvas_edge(metrics, cell->x),
            drawing_program_visual_indexed_canvas_vertical_edge(metrics, cell->y + cell->height) -
                drawing_program_visual_indexed_canvas_vertical_edge(metrics, cell->y)
        };
        SDL_SetRenderDrawColor(renderer, 245u, 190u, 92u, 230u);
        (void)SDL_RenderDrawRect(renderer, &cell_rect);
        if (draw_bitmap_text && metrics->pixel_size >= 2.0f) {
            (void)draw_bitmap_text(renderer, cell_rect, cell_rect.x + 2, cell_rect.y + 2,
                                   cell->id, (SDL_Color){245u, 190u, 92u, 255u}, 1);
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
    drawing_program_visual_indexed_canvas_restore_clip(renderer, &clip_state);
}
