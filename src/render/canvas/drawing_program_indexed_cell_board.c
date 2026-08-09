#include "drawing_program/drawing_program_indexed_cell_board.h"

#include <stdio.h>

#include "drawing_program/drawing_program_indexed_editor.h"

enum {
    INDEXED_BOARD_PADDING = 18,
    INDEXED_BOARD_GUTTER = 14,
    INDEXED_BOARD_TITLE_HEIGHT = 18,
    INDEXED_BOARD_FRAME = 2
};

typedef struct DrawingProgramIndexedCellBoardClipState {
    SDL_bool enabled;
    SDL_Rect rect;
} DrawingProgramIndexedCellBoardClipState;

static DrawingProgramIndexedCellBoardClipState drawing_program_indexed_cell_board_begin_clip(
    SDL_Renderer *renderer,
    SDL_Rect pane_rect) {
    DrawingProgramIndexedCellBoardClipState state = { SDL_FALSE, { 0, 0, 0, 0 } };
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

static void drawing_program_indexed_cell_board_restore_clip(
    SDL_Renderer *renderer,
    const DrawingProgramIndexedCellBoardClipState *state) {
    (void)SDL_RenderSetClipRect(renderer, state->enabled ? &state->rect : 0);
}

uint32_t drawing_program_indexed_cell_board_layout(
    const DrawingProgramAppContext *ctx,
    SDL_Rect pane_rect,
    DrawingProgramIndexedCellBoardCard *out_cards,
    uint32_t capacity) {
    const DrawingProgramIndexedCellTable *table;
    uint32_t count;
    uint32_t columns = 1u;
    uint32_t rows;
    int available_w;
    int available_h;
    int pixel_by_width;
    int pixel_by_height;
    int pixel_size;
    int card_w;
    int card_h;
    uint32_t i;
    if (!ctx || !out_cards || capacity == 0u ||
        ctx->texture_project.profile_kind != DRAWING_PROGRAM_TEXTURE_PROJECT_PROFILE_INDEXED_ATLAS_V1) {
        return 0u;
    }
    table = &ctx->texture_project.indexed_cells;
    count = table->count < capacity ? table->count : capacity;
    if (count == 0u || pane_rect.w <= (INDEXED_BOARD_PADDING * 2) ||
        pane_rect.h <= (INDEXED_BOARD_PADDING * 2 + INDEXED_BOARD_TITLE_HEIGHT)) {
        return 0u;
    }
    while (columns * columns < count) {
        columns += 1u;
    }
    rows = (count + columns - 1u) / columns;
    available_w = pane_rect.w - (INDEXED_BOARD_PADDING * 2) -
                  ((int)(columns - 1u) * INDEXED_BOARD_GUTTER) - (int)(columns * INDEXED_BOARD_FRAME * 2u);
    available_h = pane_rect.h - (INDEXED_BOARD_PADDING * 2) - INDEXED_BOARD_TITLE_HEIGHT -
                  ((int)(rows - 1u) * INDEXED_BOARD_GUTTER) -
                  ((int)rows * (INDEXED_BOARD_TITLE_HEIGHT + INDEXED_BOARD_FRAME * 2));
    pixel_by_width = available_w / ((int)columns * (int)ctx->texture_project.indexed_profile.logical_cell_width);
    pixel_by_height = available_h / ((int)rows * (int)ctx->texture_project.indexed_profile.logical_cell_height);
    pixel_size = pixel_by_width < pixel_by_height ? pixel_by_width : pixel_by_height;
    if (pixel_size < 1) {
        pixel_size = 1;
    }
    card_w = ((int)ctx->texture_project.indexed_profile.logical_cell_width * pixel_size) + INDEXED_BOARD_FRAME * 2;
    card_h = INDEXED_BOARD_TITLE_HEIGHT +
             ((int)ctx->texture_project.indexed_profile.logical_cell_height * pixel_size) + INDEXED_BOARD_FRAME * 2;
    for (i = 0u; i < count; ++i) {
        uint32_t row = i / columns;
        uint32_t column = i % columns;
        int x = pane_rect.x + INDEXED_BOARD_PADDING + (int)column * (card_w + INDEXED_BOARD_GUTTER);
        int y = pane_rect.y + INDEXED_BOARD_PADDING + INDEXED_BOARD_TITLE_HEIGHT +
                (int)row * (card_h + INDEXED_BOARD_GUTTER);
        out_cards[i].cell_index = i;
        out_cards[i].frame_rect = (SDL_Rect){ x, y, card_w, card_h };
        out_cards[i].title_rect = (SDL_Rect){ x + INDEXED_BOARD_FRAME, y + INDEXED_BOARD_FRAME,
                                              card_w - INDEXED_BOARD_FRAME * 2, INDEXED_BOARD_TITLE_HEIGHT };
        out_cards[i].content_rect = (SDL_Rect){ x + INDEXED_BOARD_FRAME,
                                                y + INDEXED_BOARD_FRAME + INDEXED_BOARD_TITLE_HEIGHT,
                                                (int)ctx->texture_project.indexed_profile.logical_cell_width * pixel_size,
                                                (int)ctx->texture_project.indexed_profile.logical_cell_height * pixel_size };
        out_cards[i].pixel_size = pixel_size;
    }
    return count;
}

int drawing_program_indexed_cell_board_screen_to_sample(
    const DrawingProgramAppContext *ctx,
    SDL_Rect pane_rect,
    int sx,
    int sy,
    uint32_t *out_sample_x,
    uint32_t *out_sample_y) {
    DrawingProgramIndexedCellBoardCard cards[DRAWING_PROGRAM_INDEXED_CELL_CAPACITY];
    uint32_t count;
    uint32_t i;
    SDL_Point point = { sx, sy };
    if (!ctx || !out_sample_x || !out_sample_y) {
        return 0;
    }
    count = drawing_program_indexed_cell_board_layout(ctx, pane_rect, cards, DRAWING_PROGRAM_INDEXED_CELL_CAPACITY);
    for (i = 0u; i < count; ++i) {
        const DrawingProgramIndexedCell *cell;
        uint32_t local_x;
        uint32_t local_y;
        if (!SDL_PointInRect(&point, &cards[i].content_rect) || cards[i].pixel_size <= 0) {
            continue;
        }
        cell = &ctx->texture_project.indexed_cells.cells[cards[i].cell_index];
        local_x = (uint32_t)((sx - cards[i].content_rect.x) / cards[i].pixel_size);
        local_y = (uint32_t)((sy - cards[i].content_rect.y) / cards[i].pixel_size);
        if (local_x >= cell->width || local_y >= cell->height) {
            return 0;
        }
        *out_sample_x = cell->x + local_x;
        *out_sample_y = cell->y + local_y;
        return 1;
    }
    return 0;
}

void drawing_program_visual_draw_indexed_cell_board(
    SDL_Renderer *renderer,
    SDL_Rect pane_rect,
    const DrawingProgramAppContext *ctx,
    int (*draw_bitmap_text)(SDL_Renderer *, SDL_Rect, int, int, const char *, SDL_Color, int)) {
    DrawingProgramIndexedCellBoardCard cards[DRAWING_PROGRAM_INDEXED_CELL_CAPACITY];
    uint32_t count;
    uint32_t i;
    char line[160];
    DrawingProgramIndexedCellBoardClipState clip_state;
    if (!renderer || !ctx || !drawing_program_indexed_editor_is_active(ctx)) {
        return;
    }
    clip_state = drawing_program_indexed_cell_board_begin_clip(renderer, pane_rect);
    count = drawing_program_indexed_cell_board_layout(ctx, pane_rect, cards, DRAWING_PROGRAM_INDEXED_CELL_CAPACITY);
    if (draw_bitmap_text) {
        (void)snprintf(line, sizeof(line), "INDEXED CELL BOARD  %u CELLS  /  16x16 SOURCE PIXELS",
                       (unsigned)count);
        (void)draw_bitmap_text(renderer, pane_rect, pane_rect.x + INDEXED_BOARD_PADDING,
                               pane_rect.y + 4, line, (SDL_Color){245u,190u,92u,255u}, 1);
    }
    for (i = 0u; i < count; ++i) {
        const DrawingProgramIndexedCell *cell = &ctx->texture_project.indexed_cells.cells[cards[i].cell_index];
        SDL_Color frame = cards[i].cell_index == ctx->ui.indexed_selected_cell
            ? (SDL_Color){245u,190u,92u,255u} : (SDL_Color){92u,104u,120u,255u};
        SDL_SetRenderDrawColor(renderer, 20u, 24u, 30u, 255u);
        (void)SDL_RenderFillRect(renderer, &cards[i].frame_rect);
        SDL_SetRenderDrawColor(renderer, frame.r, frame.g, frame.b, frame.a);
        (void)SDL_RenderDrawRect(renderer, &cards[i].frame_rect);
        SDL_SetRenderDrawColor(renderer, 31u, 37u, 45u, 255u);
        (void)SDL_RenderFillRect(renderer, &cards[i].title_rect);
        if (draw_bitmap_text) {
            (void)draw_bitmap_text(renderer, cards[i].title_rect,
                                   cards[i].title_rect.x + 3, cards[i].title_rect.y + 3,
                                   cell->id, frame, 1);
        }
        (void)SDL_RenderSetClipRect(renderer, &cards[i].content_rect);
        for (uint32_t y = 0u; y < cell->height; ++y) {
            for (uint32_t x = 0u; x < cell->width; ++x) {
                uint8_t slot = ctx->texture_project.indexed_tile_canvases.count ==
                        ctx->texture_project.indexed_cells.count
                    ? ctx->texture_project.indexed_tile_canvases.canvases[cards[i].cell_index]
                        .indices[y * cell->width + x]
                    : ctx->texture_project.indexed_raster.indices[(cell->y + y) *
                        ctx->texture_project.indexed_raster.width + cell->x + x];
                CoreAuthoredTextureRgba8 rgba;
                SDL_Rect pixel = { cards[i].content_rect.x + (int)x * cards[i].pixel_size,
                                   cards[i].content_rect.y + (int)y * cards[i].pixel_size,
                                   cards[i].pixel_size, cards[i].pixel_size };
                if ((uint32_t)slot >= ctx->texture_project.indexed_profile.slot_count) continue;
                rgba = ctx->texture_project.indexed_profile.slots[slot].preview_rgba;
                if (rgba.a == 0u) continue;
                SDL_SetRenderDrawColor(renderer, rgba.r, rgba.g, rgba.b, rgba.a);
                (void)SDL_RenderFillRect(renderer, &pixel);
            }
        }
        if (cards[i].pixel_size >= 6) {
            SDL_SetRenderDrawColor(renderer, 28u, 31u, 38u, 160u);
            for (uint32_t x = 1u; x < cell->width; ++x) {
                int px = cards[i].content_rect.x + (int)x * cards[i].pixel_size;
                (void)SDL_RenderDrawLine(renderer, px, cards[i].content_rect.y, px,
                                         cards[i].content_rect.y + cards[i].content_rect.h);
            }
            for (uint32_t y = 1u; y < cell->height; ++y) {
                int py = cards[i].content_rect.y + (int)y * cards[i].pixel_size;
                (void)SDL_RenderDrawLine(renderer, cards[i].content_rect.x, py,
                                         cards[i].content_rect.x + cards[i].content_rect.w, py);
            }
        }
        (void)SDL_RenderSetClipRect(renderer, &pane_rect);
    }
    drawing_program_indexed_cell_board_restore_clip(renderer, &clip_state);
}
