#include "drawing_program/drawing_program_visual_text_render.h"

#include <SDL2/SDL_ttf.h>

#include "drawing_program/drawing_program_visual_resources.h"

static uint32_t g_visual_text_font_preset_id = (uint32_t)CORE_FONT_PRESET_IDE;

static void bitmap_glyph_rows(unsigned char c, uint8_t rows[7]) {
    size_t i;
    for (i = 0; i < 7; ++i) {
        rows[i] = 0u;
    }
    if (c >= 'a' && c <= 'z') {
        c = (unsigned char)(c - 'a' + 'A');
    }
    switch (c) {
        case 'A': rows[0]=14; rows[1]=17; rows[2]=17; rows[3]=31; rows[4]=17; rows[5]=17; rows[6]=17; break;
        case 'B': rows[0]=30; rows[1]=17; rows[2]=17; rows[3]=30; rows[4]=17; rows[5]=17; rows[6]=30; break;
        case 'C': rows[0]=14; rows[1]=17; rows[2]=16; rows[3]=16; rows[4]=16; rows[5]=17; rows[6]=14; break;
        case 'D': rows[0]=30; rows[1]=17; rows[2]=17; rows[3]=17; rows[4]=17; rows[5]=17; rows[6]=30; break;
        case 'E': rows[0]=31; rows[1]=16; rows[2]=16; rows[3]=30; rows[4]=16; rows[5]=16; rows[6]=31; break;
        case 'F': rows[0]=31; rows[1]=16; rows[2]=16; rows[3]=30; rows[4]=16; rows[5]=16; rows[6]=16; break;
        case 'G': rows[0]=14; rows[1]=17; rows[2]=16; rows[3]=23; rows[4]=17; rows[5]=17; rows[6]=14; break;
        case 'H': rows[0]=17; rows[1]=17; rows[2]=17; rows[3]=31; rows[4]=17; rows[5]=17; rows[6]=17; break;
        case 'I': rows[0]=31; rows[1]=4; rows[2]=4; rows[3]=4; rows[4]=4; rows[5]=4; rows[6]=31; break;
        case 'J': rows[0]=7; rows[1]=2; rows[2]=2; rows[3]=2; rows[4]=2; rows[5]=18; rows[6]=12; break;
        case 'K': rows[0]=17; rows[1]=18; rows[2]=20; rows[3]=24; rows[4]=20; rows[5]=18; rows[6]=17; break;
        case 'L': rows[0]=16; rows[1]=16; rows[2]=16; rows[3]=16; rows[4]=16; rows[5]=16; rows[6]=31; break;
        case 'M': rows[0]=17; rows[1]=27; rows[2]=21; rows[3]=21; rows[4]=17; rows[5]=17; rows[6]=17; break;
        case 'N': rows[0]=17; rows[1]=25; rows[2]=21; rows[3]=19; rows[4]=17; rows[5]=17; rows[6]=17; break;
        case 'O': rows[0]=14; rows[1]=17; rows[2]=17; rows[3]=17; rows[4]=17; rows[5]=17; rows[6]=14; break;
        case 'P': rows[0]=30; rows[1]=17; rows[2]=17; rows[3]=30; rows[4]=16; rows[5]=16; rows[6]=16; break;
        case 'Q': rows[0]=14; rows[1]=17; rows[2]=17; rows[3]=17; rows[4]=21; rows[5]=18; rows[6]=13; break;
        case 'R': rows[0]=30; rows[1]=17; rows[2]=17; rows[3]=30; rows[4]=20; rows[5]=18; rows[6]=17; break;
        case 'S': rows[0]=15; rows[1]=16; rows[2]=16; rows[3]=14; rows[4]=1; rows[5]=1; rows[6]=30; break;
        case 'T': rows[0]=31; rows[1]=4; rows[2]=4; rows[3]=4; rows[4]=4; rows[5]=4; rows[6]=4; break;
        case 'U': rows[0]=17; rows[1]=17; rows[2]=17; rows[3]=17; rows[4]=17; rows[5]=17; rows[6]=14; break;
        case 'V': rows[0]=17; rows[1]=17; rows[2]=17; rows[3]=17; rows[4]=17; rows[5]=10; rows[6]=4; break;
        case 'W': rows[0]=17; rows[1]=17; rows[2]=17; rows[3]=21; rows[4]=21; rows[5]=21; rows[6]=10; break;
        case 'X': rows[0]=17; rows[1]=17; rows[2]=10; rows[3]=4; rows[4]=10; rows[5]=17; rows[6]=17; break;
        case 'Y': rows[0]=17; rows[1]=17; rows[2]=10; rows[3]=4; rows[4]=4; rows[5]=4; rows[6]=4; break;
        case 'Z': rows[0]=31; rows[1]=1; rows[2]=2; rows[3]=4; rows[4]=8; rows[5]=16; rows[6]=31; break;
        case '0': rows[0]=14; rows[1]=17; rows[2]=19; rows[3]=21; rows[4]=25; rows[5]=17; rows[6]=14; break;
        case '1': rows[0]=4; rows[1]=12; rows[2]=4; rows[3]=4; rows[4]=4; rows[5]=4; rows[6]=14; break;
        case '2': rows[0]=14; rows[1]=17; rows[2]=1; rows[3]=2; rows[4]=4; rows[5]=8; rows[6]=31; break;
        case '3': rows[0]=30; rows[1]=1; rows[2]=1; rows[3]=14; rows[4]=1; rows[5]=1; rows[6]=30; break;
        case '4': rows[0]=2; rows[1]=6; rows[2]=10; rows[3]=18; rows[4]=31; rows[5]=2; rows[6]=2; break;
        case '5': rows[0]=31; rows[1]=16; rows[2]=16; rows[3]=30; rows[4]=1; rows[5]=1; rows[6]=30; break;
        case '6': rows[0]=14; rows[1]=16; rows[2]=16; rows[3]=30; rows[4]=17; rows[5]=17; rows[6]=14; break;
        case '7': rows[0]=31; rows[1]=1; rows[2]=2; rows[3]=4; rows[4]=8; rows[5]=8; rows[6]=8; break;
        case '8': rows[0]=14; rows[1]=17; rows[2]=17; rows[3]=14; rows[4]=17; rows[5]=17; rows[6]=14; break;
        case '9': rows[0]=14; rows[1]=17; rows[2]=17; rows[3]=15; rows[4]=1; rows[5]=1; rows[6]=14; break;
        case ':': rows[0]=0; rows[1]=12; rows[2]=12; rows[3]=0; rows[4]=12; rows[5]=12; rows[6]=0; break;
        case '-': rows[0]=0; rows[1]=0; rows[2]=0; rows[3]=14; rows[4]=0; rows[5]=0; rows[6]=0; break;
        case ' ': break;
        default: rows[0]=14; rows[1]=17; rows[2]=1; rows[3]=2; rows[4]=4; rows[5]=0; rows[6]=4; break;
    }
}

void drawing_program_visual_text_set_font_preset_id(uint32_t ui_font_preset_id) {
    g_visual_text_font_preset_id = ui_font_preset_id;
}

int drawing_program_visual_draw_bitmap_text(SDL_Renderer *renderer,
                                            SDL_Rect clip_rect,
                                            int x,
                                            int y,
                                            const char *text,
                                            SDL_Color color,
                                            int scale) {
    TTF_Font *font = 0;
    SDL_Surface *surface = 0;
    SDL_Texture *texture = 0;
    int cursor_x = x;
    size_t i;
    if (!renderer || !text || scale < 1) {
        return 0;
    }
    font = drawing_program_visual_get_ttf_font_for_preset(g_visual_text_font_preset_id, scale);
    if (font) {
        surface = TTF_RenderUTF8_Blended(font, text, color);
        if (surface) {
            texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                SDL_Rect dst = { x, y, surface->w, surface->h };
                (void)SDL_RenderSetClipRect(renderer, &clip_rect);
                (void)SDL_RenderCopy(renderer, texture, 0, &dst);
                (void)SDL_RenderSetClipRect(renderer, 0);
                cursor_x = x + surface->w;
                SDL_DestroyTexture(texture);
                SDL_FreeSurface(surface);
                return cursor_x - x;
            }
            SDL_FreeSurface(surface);
        }
    }
    (void)SDL_RenderSetClipRect(renderer, &clip_rect);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (i = 0; text[i] != '\0'; ++i) {
        uint8_t rows[7];
        int gy;
        int gx;
        bitmap_glyph_rows((unsigned char)text[i], rows);
        for (gy = 0; gy < 7; ++gy) {
            for (gx = 0; gx < 5; ++gx) {
                if ((rows[gy] >> (4 - gx)) & 1u) {
                    SDL_Rect px = { cursor_x + gx * scale, y + gy * scale, scale, scale };
                    (void)SDL_RenderFillRect(renderer, &px);
                }
            }
        }
        cursor_x += (5 * scale) + scale;
        if (cursor_x > clip_rect.x + clip_rect.w - 8) {
            break;
        }
    }
    (void)SDL_RenderSetClipRect(renderer, 0);
    return cursor_x - x;
}

int drawing_program_visual_measure_bitmap_text_width(const char *text, int scale) {
    TTF_Font *font = 0;
    int width = 0;
    int height = 0;
    size_t i;
    if (!text || scale < 1) {
        return 0;
    }
    font = drawing_program_visual_get_ttf_font_for_preset(g_visual_text_font_preset_id, scale);
    if (font && TTF_SizeUTF8(font, text, &width, &height) == 0 && width > 0) {
        return width;
    }
    for (i = 0; text[i] != '\0'; ++i) {
        width += (5 * scale) + scale;
    }
    return width;
}
