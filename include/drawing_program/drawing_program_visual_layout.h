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
SDL_Rect right_panel_slot_tab_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint8_t slot_index, uint8_t slot_count);
SDL_Rect right_canvas_add_surface_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_duplicate_surface_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_mode_toggle_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_guide_mode_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_reflect_horizontal_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_reflect_vertical_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_center_pick_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_center_reset_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_delete_canvas_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_reset_object_layout_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_reset_view_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_clear_canvas_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_clear_objects_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_delete_selection_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_clear_history_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
int right_canvas_metrics_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m);
int right_canvas_surface_rows_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_canvas_surface_row_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t row_index);
int right_file_content_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m);
int right_file_actions_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m);
int right_file_recent_projects_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t action_count);
int right_file_state_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t line_count);
SDL_Rect right_file_project_queue_rect(SDL_Rect rect,
                                       VisualPaneLayoutMetrics m,
                                       uint32_t top_action_count,
                                       uint32_t state_line_count);
SDL_Rect right_file_browser_mode_tab_rect(SDL_Rect rect,
                                          VisualPaneLayoutMetrics m,
                                          uint32_t mode_index,
                                          uint32_t mode_count);
SDL_Rect right_asset_browser_mode_tab_rect(SDL_Rect rect,
                                           VisualPaneLayoutMetrics m,
                                           uint32_t mode_index,
                                           uint32_t mode_count);
SDL_Rect right_asset_target_queue_rect(SDL_Rect rect,
                                       VisualPaneLayoutMetrics m,
                                       uint32_t state_line_count,
                                       uint32_t action_count);
int right_file_route_actions_start_y(SDL_Rect rect,
                                     VisualPaneLayoutMetrics m,
                                     uint32_t state_line_count,
                                     uint32_t action_count);
uint32_t right_file_target_queue_slot_count(const DrawingProgramAppContext *ctx);
int right_file_target_queue_label_y(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t action_count);
SDL_Rect right_file_target_queue_rect(SDL_Rect rect,
                                      VisualPaneLayoutMetrics m,
                                      uint32_t state_line_count,
                                      uint32_t route_action_count);
int right_file_target_queue_scroll_max(SDL_Rect queue_rect,
                                       VisualPaneLayoutMetrics m,
                                       uint32_t slot_count);
int right_file_target_queue_clamp_scroll(SDL_Rect queue_rect,
                                         VisualPaneLayoutMetrics m,
                                         uint32_t slot_count,
                                         int scroll_y);
SDL_Rect right_file_recent_project_row_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t row_index);
SDL_Rect right_file_target_queue_row_rect(SDL_Rect queue_rect,
                                          VisualPaneLayoutMetrics m,
                                          uint32_t row_index,
                                          int scroll_y);
SDL_Rect right_file_action_button_rect(SDL_Rect rect,
                                       VisualPaneLayoutMetrics m,
                                       uint32_t action_index,
                                       uint32_t action_count);
SDL_Rect right_file_save_session_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_file_reload_session_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect right_file_route_action_button_rect(SDL_Rect rect,
                                             VisualPaneLayoutMetrics m,
                                             uint32_t state_line_count,
                                             uint32_t action_index,
                                             uint32_t action_count);

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
int right_layer_role_section_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t layer_count);
SDL_Rect right_layer_role_button_rect(SDL_Rect rect,
                                      VisualPaneLayoutMetrics m,
                                      uint32_t layer_count,
                                      uint32_t role_index);

int left_panel_content_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect left_panel_slot_tab_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint8_t slot_index, uint8_t slot_count);
SDL_Rect left_panel_tool_list_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t tool_count);
SDL_Rect left_panel_tool_row_rect(SDL_Rect rect,
                                  VisualPaneLayoutMetrics m,
                                  uint32_t tool_index,
                                  uint32_t tool_count);
SDL_Rect left_panel_tool_detail_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t tool_count);
SDL_Rect left_panel_objects_list_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
SDL_Rect left_panel_objects_inspector_rect(SDL_Rect rect, VisualPaneLayoutMetrics m);
int left_panel_objects_rows_start_y(SDL_Rect list_rect, VisualPaneLayoutMetrics m);
SDL_Rect left_panel_objects_row_rect(SDL_Rect list_rect, VisualPaneLayoutMetrics m, uint32_t row_index);
SDL_Rect left_panel_objects_inspector_action_row_rect(SDL_Rect inspector_rect,
                                                      VisualPaneLayoutMetrics m,
                                                      uint32_t action_index,
                                                      uint32_t action_count);
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
