#ifndef DRAWING_PROGRAM_INDEXED_CELL_BOARD_H
#define DRAWING_PROGRAM_INDEXED_CELL_BOARD_H

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_app_main.h"

typedef struct DrawingProgramIndexedCellBoardCard {
    uint32_t cell_index;
    SDL_Rect frame_rect;
    SDL_Rect title_rect;
    SDL_Rect content_rect;
    int pixel_size;
} DrawingProgramIndexedCellBoardCard;

uint32_t drawing_program_indexed_cell_board_layout(
    const DrawingProgramAppContext *ctx,
    SDL_Rect pane_rect,
    DrawingProgramIndexedCellBoardCard *out_cards,
    uint32_t capacity);

int drawing_program_indexed_cell_board_screen_to_sample(
    const DrawingProgramAppContext *ctx,
    SDL_Rect pane_rect,
    int sx,
    int sy,
    uint32_t *out_sample_x,
    uint32_t *out_sample_y);

void drawing_program_visual_draw_indexed_cell_board(
    SDL_Renderer *renderer,
    SDL_Rect pane_rect,
    const DrawingProgramAppContext *ctx,
    int (*draw_bitmap_text)(SDL_Renderer *, SDL_Rect, int, int, const char *, SDL_Color, int));

#endif
