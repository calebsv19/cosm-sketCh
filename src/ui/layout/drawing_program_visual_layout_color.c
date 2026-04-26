#include "drawing_program/drawing_program_visual_layout_color.h"

#include "drawing_program/drawing_program_visual_layout.h"

static int right_color_active_preview_height(VisualPaneLayoutMetrics m) {
    int h = m.body_glyph_h + 10;
    if (h < 18) {
        h = 18;
    }
    return h;
}

static int right_color_compact_swatch_height(VisualPaneLayoutMetrics m) {
    int h = m.body_glyph_h + 8;
    if (h < 16) {
        h = 16;
    }
    return h;
}

static int right_color_recent_swatches_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect save_button = right_color_save_preset_button_rect(rect, m);
    int hsv_line_y = save_button.y + save_button.h + m.section_gap;
    int recent_label_y = hsv_line_y + m.line_h + m.section_gap;
    return recent_label_y + m.line_h + m.section_gap;
}

int right_color_content_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    int y = rect.y + m.pad_y + m.title_glyph_h + m.section_gap;
    y += m.tab_h + m.section_gap;
    return y;
}

SDL_Rect right_color_active_swatch_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    int y = right_color_content_start_y(rect, m) + (3 * m.line_h) + m.section_gap;
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), right_color_active_preview_height(m) };
}

SDL_Rect right_color_save_preset_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect active = right_color_active_swatch_rect(rect, m);
    int y = active.y + active.h + m.line_h + m.section_gap;
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
}

SDL_Rect right_color_recent_swatch_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint8_t recent_index) {
    const int cols = 4;
    int swatch_y = right_color_recent_swatches_start_y(rect, m);
    int swatch_w = (rect.w - (2 * m.pad_x) - ((cols - 1) * m.tab_gap)) / cols;
    int swatch_h = right_color_compact_swatch_height(m);
    int row = (int)(recent_index / (uint8_t)cols);
    int col = (int)(recent_index % (uint8_t)cols);
    if (swatch_w < 18) {
        swatch_w = 18;
    }
    return (SDL_Rect){
        rect.x + m.pad_x + col * (swatch_w + m.tab_gap),
        swatch_y + row * (swatch_h + m.section_gap),
        swatch_w,
        swatch_h
    };
}

SDL_Rect right_color_hue_slider_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect recent_last = right_color_recent_swatch_rect(rect, m, 7u);
    int y = recent_last.y + recent_last.h + m.line_h + (2 * m.section_gap);
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
}

SDL_Rect right_color_sv_grid_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect hue = right_color_hue_slider_rect(rect, m);
    int y = hue.y + hue.h + m.line_h + (2 * m.section_gap);
    int size = rect.w - (2 * m.pad_x);
    int max_h = (rect.y + rect.h - m.pad_y) - y - (m.line_h + m.section_gap + (2 * m.row_h) + m.section_gap);
    if (size > max_h) {
        size = max_h;
    }
    if (size < (m.row_h * 3)) {
        size = m.row_h * 3;
    }
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), size };
}

SDL_Rect right_color_palette_swatch_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint8_t color_index) {
    const int cols = 4;
    SDL_Rect grid = right_color_sv_grid_rect(rect, m);
    int y = grid.y + grid.h + m.line_h + (2 * m.section_gap);
    int swatch_w = (rect.w - (2 * m.pad_x) - ((cols - 1) * m.tab_gap)) / cols;
    int swatch_h = right_color_compact_swatch_height(m);
    int row = (int)(color_index / (uint8_t)cols);
    int col = (int)(color_index % (uint8_t)cols);
    if (swatch_w < 18) {
        swatch_w = 18;
    }
    return (SDL_Rect){
        rect.x + m.pad_x + col * (swatch_w + m.tab_gap),
        y + row * (swatch_h + m.section_gap),
        swatch_w,
        swatch_h
    };
}
