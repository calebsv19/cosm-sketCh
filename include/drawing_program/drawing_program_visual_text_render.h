#ifndef DRAWING_PROGRAM_VISUAL_TEXT_RENDER_H
#define DRAWING_PROGRAM_VISUAL_TEXT_RENDER_H

#include <SDL2/SDL.h>

void drawing_program_visual_text_set_font_preset_id(uint32_t ui_font_preset_id);

int drawing_program_visual_draw_bitmap_text(SDL_Renderer *renderer,
                                            SDL_Rect clip_rect,
                                            int x,
                                            int y,
                                            const char *text,
                                            SDL_Color color,
                                            int scale);

int drawing_program_visual_measure_bitmap_text_width(const char *text, int scale);

#endif
