#include "drawing_program/drawing_program_visual_panel_render_common.h"

#include "drawing_program/drawing_program_visual_theme.h"

int drawing_program_visual_panel_ui_hovered(const VisualPanelUiState *ui,
                                            SDL_Rect rect,
                                            const DrawingProgramVisualPanelRenderHooks *hooks) {
    if (!ui || !ui->mouse_known || !hooks || !hooks->point_in_rect) {
        return 0;
    }
    return hooks->point_in_rect(rect, ui->mouse_x, ui->mouse_y);
}

void drawing_program_visual_panel_draw_tab_button(SDL_Renderer *renderer,
                                                  SDL_Rect clip_rect,
                                                  SDL_Rect rect,
                                                  const char *label,
                                                  SDL_Color fill,
                                                  SDL_Color fill_hover,
                                                  SDL_Color fill_active,
                                                  SDL_Color border,
                                                  SDL_Color text,
                                                  int text_scale,
                                                  int selected,
                                                  int hovered,
                                                  const DrawingProgramVisualPanelRenderHooks *hooks) {
    SDL_Color bg = selected ? fill_active : (hovered ? fill_hover : fill);
    SDL_Color text_final = sdl_color_ensure_contrast(text, bg);
    int glyph_h = 7 * text_scale;
    int text_y = rect.y + ((rect.h - glyph_h) / 2);
    if (text_y < rect.y + 2) {
        text_y = rect.y + 2;
    }
    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
    (void)SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
    (void)SDL_RenderDrawRect(renderer, &rect);
    hooks->draw_bitmap_text(renderer, clip_rect, rect.x + 6, text_y, label, text_final, text_scale);
}

const char *drawing_program_visual_shape_target_mode_name(uint8_t mode) {
    return (mode == (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_OBJECT) ? "OBJECT" : "PIXEL";
}

const char *drawing_program_visual_select_mode_name(uint8_t mode) {
    switch (mode) {
        case (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_ADD: return "ADD";
        case (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_SUBTRACT: return "SUBTRACT";
        case (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE:
        default:
            return "REPLACE";
    }
}
