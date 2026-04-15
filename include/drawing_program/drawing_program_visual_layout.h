#ifndef DRAWING_PROGRAM_VISUAL_LAYOUT_H
#define DRAWING_PROGRAM_VISUAL_LAYOUT_H

#include <stdint.h>
#include <SDL2/SDL.h>

#include "core_theme.h"
#include "drawing_program/drawing_program_app_main.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VisualPaneLayoutMetrics {
    int pad_x;
    int pad_y;
    int section_gap;
    int line_h;
    int row_h;
    int tab_h;
    int tab_gap;
    int title_scale;
    int body_scale;
    int title_glyph_h;
    int body_glyph_h;
    int row_text_y;
    int tab_text_y;
} VisualPaneLayoutMetrics;

typedef enum VisualLayerActionButton {
    VISUAL_LAYER_ACTION_ADD = 0,
    VISUAL_LAYER_ACTION_DUPLICATE,
    VISUAL_LAYER_ACTION_RENAME,
    VISUAL_LAYER_ACTION_DELETE,
    VISUAL_LAYER_ACTION_ACTIVE_PREV,
    VISUAL_LAYER_ACTION_ACTIVE_NEXT,
    VISUAL_LAYER_ACTION_MOVE_UP,
    VISUAL_LAYER_ACTION_MOVE_DOWN,
    VISUAL_LAYER_ACTION_TOGGLE_VISIBLE,
    VISUAL_LAYER_ACTION_TOGGLE_LOCK
} VisualLayerActionButton;

CoreThemePresetId clamp_theme_preset_id(uint32_t raw);
int clamp_font_zoom_step(int step);
int ui_scale_for_text(const DrawingProgramAppContext *ctx, int base_scale);
VisualPaneLayoutMetrics make_pane_layout_metrics(const DrawingProgramAppContext *ctx);

int right_canvas_content_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m);
int right_canvas_palette_header_y(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_palette_swatch_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint8_t color_index);
SDL_Rect right_canvas_reset_view_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_clear_canvas_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_delete_selection_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_clear_history_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
int right_canvas_metrics_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m);

int right_layer_content_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_layer_row_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t display_row_index);
int right_layer_after_rows_y(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t layer_count);
SDL_Rect right_layer_opacity_row_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t layer_count);
SDL_Rect right_layer_opacity_track_rect(SDL_Rect opacity_row, VisualPaneLayoutMetrics m);
int right_layer_actions_label_y(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t layer_count);
SDL_Rect right_layer_action_button_rect(SDL_Rect rect,
                                        VisualPaneLayoutMetrics m,
                                        uint32_t layer_count,
                                        VisualLayerActionButton button);

int left_panel_content_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect left_panel_tool_list_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t tool_count);
SDL_Rect left_panel_tool_row_rect(SDL_Rect rect,
                                  VisualPaneLayoutMetrics m,
                                  uint32_t tool_index,
                                  uint32_t tool_count);
SDL_Rect left_panel_tool_detail_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t tool_count);
int left_panel_tool_detail_rows_start_y(SDL_Rect detail_rect, VisualPaneLayoutMetrics m);
SDL_Rect left_panel_tool_detail_option_row_rect(SDL_Rect detail_rect,
                                                VisualPaneLayoutMetrics m,
                                                uint32_t option_index);

SDL_Rect left_tool_option_row_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, int y);
SDL_Rect left_tool_option_minus_rect(SDL_Rect row, VisualPaneLayoutMetrics m);
SDL_Rect left_tool_option_plus_rect(SDL_Rect row, VisualPaneLayoutMetrics m);
SDL_Rect left_tool_option_value_rect(SDL_Rect row, VisualPaneLayoutMetrics m);

int cycle_theme_preset(CoreThemePresetId current, int direction, CoreThemePresetId *out_next);

#ifdef __cplusplus
}
#endif

#endif
