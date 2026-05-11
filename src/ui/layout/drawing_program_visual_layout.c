#include "drawing_program/drawing_program_visual_layout.h"

static const CoreThemePresetId k_visual_theme_cycle_order[] = {
    CORE_THEME_PRESET_DAW_DEFAULT,
    CORE_THEME_PRESET_MAP_FORGE_DEFAULT,
    CORE_THEME_PRESET_DARK_DEFAULT,
    CORE_THEME_PRESET_LIGHT_DEFAULT,
    CORE_THEME_PRESET_IDE_GRAY,
    CORE_THEME_PRESET_GREYSCALE
};

CoreThemePresetId clamp_theme_preset_id(uint32_t raw) {
    if (raw < (uint32_t)CORE_THEME_PRESET_COUNT) {
        return (CoreThemePresetId)raw;
    }
    return CORE_THEME_PRESET_DARK_DEFAULT;
}

int clamp_font_zoom_step(int step) {
    if (step < -2) {
        return -2;
    }
    if (step > 4) {
        return 4;
    }
    return step;
}

int ui_scale_for_text(const DrawingProgramAppContext *ctx, int base_scale) {
    int scale = base_scale;
    int step = 0;
    if (ctx) {
        step = clamp_font_zoom_step((int)ctx->ui.font_zoom_step);
    }
    scale += step;
    if (scale < 1) {
        scale = 1;
    }
    if (scale > 8) {
        scale = 8;
    }
    return scale;
}

VisualPaneLayoutMetrics make_pane_layout_metrics(const DrawingProgramAppContext *ctx) {
    VisualPaneLayoutMetrics m;
    int body_scale = ui_scale_for_text(ctx, 1);
    int title_scale = ui_scale_for_text(ctx, 2);
    int body_glyph_h = 7 * body_scale;
    int title_glyph_h = 7 * title_scale;
    m.pad_x = 8 + (body_scale * 2);
    m.pad_y = 8 + body_scale;
    m.section_gap = 4 + body_scale;
    m.line_h = body_glyph_h + 3 + body_scale;
    m.row_h = body_glyph_h + 6 + body_scale;
    if (m.row_h < 18) {
        m.row_h = 18;
    }
    m.tab_h = body_glyph_h + 5 + body_scale;
    if (m.tab_h < 18) {
        m.tab_h = 18;
    }
    m.tab_gap = 4 + (body_scale / 2);
    m.title_scale = title_scale;
    m.body_scale = body_scale;
    m.title_glyph_h = title_glyph_h;
    m.body_glyph_h = body_glyph_h;
    m.row_text_y = (m.row_h - m.body_glyph_h) / 2;
    if (m.row_text_y < 2) {
        m.row_text_y = 2;
    }
    m.tab_text_y = (m.tab_h - m.body_glyph_h) / 2;
    if (m.tab_text_y < 2) {
        m.tab_text_y = 2;
    }
    return m;
}

int right_canvas_content_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    int y = rect.y + m.pad_y + m.title_glyph_h + m.section_gap;
    y += m.tab_h + m.section_gap;
    return y;
}

static int right_canvas_controls_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    int y = right_canvas_metrics_start_y(rect, m);
    /* Keep the canvas HUD compact so controls stay visible without crowding. */
    y += 6 * m.line_h;
    y += m.section_gap;
    return y;
}

static SDL_Rect right_canvas_split_button_rect(SDL_Rect row,
                                               VisualPaneLayoutMetrics m,
                                               uint32_t column_index,
                                               uint32_t column_count) {
    int total_gap = 0;
    int button_w = 0;
    if (column_count == 0u) {
        column_count = 1u;
    }
    total_gap = (int)(column_count - 1u) * m.section_gap;
    button_w = (row.w - total_gap) / (int)column_count;
    return (SDL_Rect){ row.x + (int)column_index * (button_w + m.section_gap), row.y, button_w, row.h };
}

SDL_Rect right_panel_slot_tab_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint8_t slot_index, uint8_t slot_count) {
    int total_gap = 0;
    int tab_w = 0;
    int x = 0;
    int y = rect.y + m.pad_y + m.title_glyph_h + m.section_gap;
    if (slot_count == 0u) {
        slot_count = 1u;
    }
    total_gap = (int)(slot_count - 1u) * m.tab_gap;
    tab_w = (rect.w - (2 * m.pad_x) - total_gap) / (int)slot_count;
    if (tab_w < 36) {
        tab_w = 36;
    }
    x = rect.x + m.pad_x + (int)slot_index * (tab_w + m.tab_gap);
    return (SDL_Rect){ x, y, tab_w, m.tab_h };
}

SDL_Rect right_canvas_add_surface_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    int y = right_canvas_controls_start_y(rect, m);
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
}

SDL_Rect right_canvas_duplicate_surface_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect add_surface = right_canvas_add_surface_button_rect(rect, m);
    return (SDL_Rect){ add_surface.x, add_surface.y + add_surface.h + m.section_gap, add_surface.w, add_surface.h };
}

SDL_Rect right_canvas_mode_toggle_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect duplicate_surface = right_canvas_duplicate_surface_button_rect(rect, m);
    return (SDL_Rect){ duplicate_surface.x,
                       duplicate_surface.y + duplicate_surface.h + m.section_gap,
                       duplicate_surface.w,
                       duplicate_surface.h };
}

SDL_Rect right_canvas_guide_mode_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect mode_toggle = right_canvas_mode_toggle_button_rect(rect, m);
    return (SDL_Rect){ mode_toggle.x, mode_toggle.y + mode_toggle.h + m.section_gap, mode_toggle.w, mode_toggle.h };
}

SDL_Rect right_canvas_reflect_horizontal_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect guide_toggle = right_canvas_guide_mode_button_rect(rect, m);
    SDL_Rect row = { guide_toggle.x, guide_toggle.y + guide_toggle.h + m.section_gap, guide_toggle.w, guide_toggle.h };
    return right_canvas_split_button_rect(row, m, 0u, 2u);
}

SDL_Rect right_canvas_reflect_vertical_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect guide_toggle = right_canvas_guide_mode_button_rect(rect, m);
    SDL_Rect row = { guide_toggle.x, guide_toggle.y + guide_toggle.h + m.section_gap, guide_toggle.w, guide_toggle.h };
    return right_canvas_split_button_rect(row, m, 1u, 2u);
}

SDL_Rect right_canvas_center_pick_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect reflect_horizontal = right_canvas_reflect_horizontal_button_rect(rect, m);
    SDL_Rect row = { reflect_horizontal.x,
                     reflect_horizontal.y + reflect_horizontal.h + m.section_gap,
                     right_canvas_guide_mode_button_rect(rect, m).w,
                     reflect_horizontal.h };
    return right_canvas_split_button_rect(row, m, 0u, 2u);
}

SDL_Rect right_canvas_center_reset_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect reflect_horizontal = right_canvas_reflect_horizontal_button_rect(rect, m);
    SDL_Rect row = { reflect_horizontal.x,
                     reflect_horizontal.y + reflect_horizontal.h + m.section_gap,
                     right_canvas_guide_mode_button_rect(rect, m).w,
                     reflect_horizontal.h };
    return right_canvas_split_button_rect(row, m, 1u, 2u);
}

SDL_Rect right_canvas_delete_canvas_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect center_pick = right_canvas_center_pick_button_rect(rect, m);
    return (SDL_Rect){ center_pick.x,
                       center_pick.y + center_pick.h + m.section_gap,
                       right_canvas_guide_mode_button_rect(rect, m).w,
                       center_pick.h };
}

SDL_Rect right_canvas_reset_object_layout_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    int y = rect.y + rect.h - m.pad_y - (6 * m.row_h) - (5 * m.section_gap);
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
}

SDL_Rect right_canvas_reset_view_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect reset_layout = right_canvas_reset_object_layout_button_rect(rect, m);
    int y = reset_layout.y + reset_layout.h + m.section_gap;
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
}

SDL_Rect right_canvas_clear_canvas_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect reset = right_canvas_reset_view_button_rect(rect, m);
    return (SDL_Rect){ reset.x, reset.y + reset.h + m.section_gap, reset.w, reset.h };
}

SDL_Rect right_canvas_clear_objects_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect clear_canvas = right_canvas_clear_canvas_button_rect(rect, m);
    return (SDL_Rect){ clear_canvas.x, clear_canvas.y + clear_canvas.h + m.section_gap, clear_canvas.w, clear_canvas.h };
}

SDL_Rect right_canvas_delete_selection_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect clear_objects = right_canvas_clear_objects_button_rect(rect, m);
    return (SDL_Rect){ clear_objects.x, clear_objects.y + clear_objects.h + m.section_gap, clear_objects.w, clear_objects.h };
}

SDL_Rect right_canvas_clear_history_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect delete_selection = right_canvas_delete_selection_button_rect(rect, m);
    return (SDL_Rect){ delete_selection.x, delete_selection.y + delete_selection.h + m.section_gap, delete_selection.w, delete_selection.h };
}

int right_canvas_metrics_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    return right_canvas_content_start_y(rect, m);
}

int right_canvas_surface_rows_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect delete_canvas = right_canvas_delete_canvas_button_rect(rect, m);
    int y = delete_canvas.y + delete_canvas.h + m.line_h;
    return y;
}

SDL_Rect right_canvas_surface_row_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t row_index) {
    int y = right_canvas_surface_rows_start_y(rect, m);
    y += (int)row_index * (m.row_h + m.section_gap);
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
}

int right_file_content_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    return right_canvas_content_start_y(rect, m);
}

int right_file_actions_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    return right_file_content_start_y(rect, m) + m.line_h;
}

int right_file_recent_projects_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t action_count) {
    int y = right_file_actions_start_y(rect, m);
    if (action_count == 0u) {
        action_count = 1u;
    }
    y += (int)action_count * m.row_h;
    y += (int)(action_count - 1u) * m.section_gap;
    y += m.line_h;
    y += m.section_gap;
    return y;
}

int right_file_state_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t line_count) {
    int y = rect.y + rect.h - m.pad_y;
    if (line_count == 0u) {
        line_count = 1u;
    }
    y -= (int)line_count * m.line_h;
    return y;
}

SDL_Rect right_file_project_queue_rect(SDL_Rect rect,
                                       VisualPaneLayoutMetrics m,
                                       uint32_t top_action_count,
                                       uint32_t state_line_count) {
    int y = right_file_target_queue_label_y(rect, m, top_action_count) + m.line_h + m.section_gap;
    int bottom = right_file_state_start_y(rect, m, state_line_count) - m.section_gap;
    int h = bottom - y;
    if (h < m.row_h) {
        h = m.row_h;
    }
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), h };
}

SDL_Rect right_file_browser_mode_tab_rect(SDL_Rect rect,
                                          VisualPaneLayoutMetrics m,
                                          uint32_t mode_index,
                                          uint32_t mode_count) {
    int total_gap = 0;
    int tab_w = 0;
    int x = 0;
    int y = right_file_target_queue_label_y(rect, m, 7u) + m.line_h;
    if (mode_count == 0u) {
        mode_count = 1u;
    }
    total_gap = (int)(mode_count - 1u) * m.tab_gap;
    tab_w = (rect.w - (2 * m.pad_x) - total_gap) / (int)mode_count;
    if (tab_w < 28) {
        tab_w = 28;
    }
    x = rect.x + m.pad_x + (int)mode_index * (tab_w + m.tab_gap);
    return (SDL_Rect){ x, y, tab_w, m.row_h };
}

SDL_Rect right_asset_browser_mode_tab_rect(SDL_Rect rect,
                                           VisualPaneLayoutMetrics m,
                                           uint32_t mode_index,
                                           uint32_t mode_count) {
    int total_gap = 0;
    int tab_w = 0;
    int x = 0;
    int y = right_file_content_start_y(rect, m) + m.line_h;
    if (mode_count == 0u) {
        mode_count = 1u;
    }
    total_gap = (int)(mode_count - 1u) * m.tab_gap;
    tab_w = (rect.w - (2 * m.pad_x) - total_gap) / (int)mode_count;
    if (tab_w < 28) {
        tab_w = 28;
    }
    x = rect.x + m.pad_x + (int)mode_index * (tab_w + m.tab_gap);
    return (SDL_Rect){ x, y, tab_w, m.row_h };
}

int right_file_route_actions_start_y(SDL_Rect rect,
                                     VisualPaneLayoutMetrics m,
                                     uint32_t state_line_count,
                                     uint32_t action_count) {
    int y = right_file_state_start_y(rect, m, state_line_count);
    int action_block_h = 0;
    if (action_count == 0u) {
        action_count = 1u;
    }
    action_block_h = (int)action_count * m.row_h;
    action_block_h += (int)(action_count - 1u) * m.section_gap;
    y -= m.section_gap;
    y -= action_block_h;
    return y;
}

SDL_Rect right_asset_target_queue_rect(SDL_Rect rect,
                                       VisualPaneLayoutMetrics m,
                                       uint32_t state_line_count,
                                       uint32_t action_count) {
    SDL_Rect browser_tab = right_asset_browser_mode_tab_rect(rect, m, 0u, 3u);
    int y = browser_tab.y + browser_tab.h + m.section_gap;
    int bottom = right_file_route_actions_start_y(rect, m, state_line_count, action_count) - m.section_gap;
    int h = bottom - y;
    if (h < m.row_h) {
        h = m.row_h;
    }
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), h };
}

uint32_t right_file_target_queue_slot_count(const DrawingProgramAppContext *ctx) {
    uint32_t count = 1u;
    if (ctx && ctx->session.recent_project_count > 0u) {
        count = (uint32_t)ctx->session.recent_project_count;
    }
    if (count > DRAWING_PROGRAM_RECENT_PROJECT_CAPACITY) {
        count = DRAWING_PROGRAM_RECENT_PROJECT_CAPACITY;
    }
    return count;
}

int right_file_target_queue_label_y(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t action_count) {
    int y = right_file_actions_start_y(rect, m);
    if (action_count == 0u) {
        action_count = 1u;
    }
    y += (int)action_count * m.row_h;
    y += (int)(action_count - 1u) * m.section_gap;
    y += m.section_gap;
    return y;
}

SDL_Rect right_file_target_queue_rect(SDL_Rect rect,
                                      VisualPaneLayoutMetrics m,
                                      uint32_t state_line_count,
                                      uint32_t route_action_count) {
    SDL_Rect browser_tab = right_file_browser_mode_tab_rect(rect, m, 0u, 3u);
    int y = browser_tab.y + browser_tab.h + m.section_gap;
    int bottom = right_file_route_actions_start_y(rect, m, state_line_count, route_action_count) - m.section_gap;
    int h = bottom - y;
    if (h < m.row_h) {
        h = m.row_h;
    }
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), h };
}

int right_file_target_queue_scroll_max(SDL_Rect queue_rect,
                                       VisualPaneLayoutMetrics m,
                                       uint32_t slot_count) {
    int row_stride = m.row_h + m.section_gap;
    (void)queue_rect;
    if (slot_count <= 1u || row_stride <= 0) {
        return 0;
    }
    return (int)(slot_count - 1u) * row_stride;
}

int right_file_target_queue_clamp_scroll(SDL_Rect queue_rect,
                                         VisualPaneLayoutMetrics m,
                                         uint32_t slot_count,
                                         int scroll_y) {
    int max_scroll = right_file_target_queue_scroll_max(queue_rect, m, slot_count);
    if (scroll_y < 0) {
        return 0;
    }
    if (scroll_y > max_scroll) {
        return max_scroll;
    }
    return scroll_y;
}

SDL_Rect right_file_recent_project_row_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t row_index) {
    SDL_Rect queue_rect = right_file_target_queue_rect(rect, m, 10u, 5u);
    return right_file_target_queue_row_rect(queue_rect, m, row_index, 0);
}

SDL_Rect right_file_target_queue_row_rect(SDL_Rect queue_rect,
                                          VisualPaneLayoutMetrics m,
                                          uint32_t row_index,
                                          int scroll_y) {
    int y = queue_rect.y + (int)row_index * (m.row_h + m.section_gap) - scroll_y;
    int row_w = queue_rect.w;
    if (row_w > 10) {
        row_w -= 10;
    }
    return (SDL_Rect){ queue_rect.x, y, row_w, m.row_h };
}

SDL_Rect right_file_action_button_rect(SDL_Rect rect,
                                       VisualPaneLayoutMetrics m,
                                       uint32_t action_index,
                                       uint32_t action_count) {
    int y;
    if (action_count == 0u) {
        action_count = 1u;
    }
    y = right_file_actions_start_y(rect, m);
    y += (int)action_index * (m.row_h + m.section_gap);
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
}

SDL_Rect right_file_save_session_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    return right_file_action_button_rect(rect, m, 5u, 7u);
}

SDL_Rect right_file_reload_session_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    return right_file_action_button_rect(rect, m, 6u, 7u);
}

SDL_Rect right_file_route_action_button_rect(SDL_Rect rect,
                                             VisualPaneLayoutMetrics m,
                                             uint32_t state_line_count,
                                             uint32_t action_index,
                                             uint32_t action_count) {
    int y;
    if (action_count == 0u) {
        action_count = 1u;
    }
    y = right_file_route_actions_start_y(rect, m, state_line_count, action_count);
    y += (int)action_index * (m.row_h + m.section_gap);
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
}

int right_layer_content_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    int y = rect.y + m.pad_y + m.title_glyph_h + m.section_gap;
    y += m.tab_h + m.section_gap;
    return y;
}

SDL_Rect right_layer_row_rect(SDL_Rect rect,
                              VisualPaneLayoutMetrics m,
                              uint32_t display_row_index) {
    int y = right_layer_content_start_y(rect, m);
    y += m.line_h;
    y += (int)display_row_index * (m.row_h + m.section_gap);
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
}

int right_layer_after_rows_y(SDL_Rect rect,
                             VisualPaneLayoutMetrics m,
                             uint32_t layer_count) {
    int y = right_layer_content_start_y(rect, m);
    y += m.line_h;
    y += (int)layer_count * m.row_h;
    if (layer_count > 1u) {
        y += (int)(layer_count - 1u) * m.section_gap;
    }
    return y;
}

SDL_Rect right_layer_opacity_row_rect(SDL_Rect rect,
                                      VisualPaneLayoutMetrics m,
                                      uint32_t layer_count) {
    int y = right_layer_after_rows_y(rect, m, layer_count);
    y += m.section_gap;
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.line_h + m.section_gap + m.row_h };
}

SDL_Rect right_layer_opacity_track_rect(SDL_Rect opacity_row, VisualPaneLayoutMetrics m) {
    SDL_Rect track = opacity_row;
    int inner_pad = m.tab_gap + 4;
    if (inner_pad < 6) {
        inner_pad = 6;
    }
    track.x += inner_pad;
    track.w -= (inner_pad * 2);
    if (track.w < 24) {
        track.w = 24;
    }
    track.y += m.line_h + m.section_gap + ((m.row_h - ((m.row_h > 10) ? (m.row_h / 3) : 3)) / 2);
    track.h = (m.row_h > 10) ? (m.row_h / 3) : 3;
    return track;
}

int right_layer_actions_label_y(SDL_Rect rect,
                                VisualPaneLayoutMetrics m,
                                uint32_t layer_count) {
    SDL_Rect opacity_row = right_layer_opacity_row_rect(rect, m, layer_count);
    return opacity_row.y + opacity_row.h + m.section_gap;
}

SDL_Rect right_layer_action_button_rect(SDL_Rect rect,
                                        VisualPaneLayoutMetrics m,
                                        uint32_t layer_count,
                                        VisualLayerActionButton button) {
    int y = right_layer_actions_label_y(rect, m, layer_count);
    y += m.line_h + m.section_gap;
    y += (int)button * (m.row_h + m.section_gap);
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
}

int right_layer_role_section_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t layer_count) {
    SDL_Rect last_action = right_layer_action_button_rect(rect, m, layer_count, VISUAL_LAYER_ACTION_TOGGLE_LOCK);
    return last_action.y + last_action.h + m.section_gap;
}

SDL_Rect right_layer_role_button_rect(SDL_Rect rect,
                                      VisualPaneLayoutMetrics m,
                                      uint32_t layer_count,
                                      uint32_t role_index) {
    int y = right_layer_role_section_start_y(rect, m, layer_count);
    int button_y = y + (3 * m.line_h) + m.section_gap;
    int full_w = rect.w - (2 * m.pad_x);
    int half_w = (full_w - m.tab_gap) / 2;
    if (half_w < 36) {
        half_w = 36;
    }
    if (role_index >= 4u) {
        return (SDL_Rect){ rect.x + m.pad_x, button_y + (2 * (m.row_h + m.section_gap)), full_w, m.row_h };
    }
    return (SDL_Rect){ rect.x + m.pad_x + ((int)(role_index % 2u) * (half_w + m.tab_gap)),
                       button_y + ((int)(role_index / 2u) * (m.row_h + m.section_gap)),
                       half_w,
                       m.row_h };
}

int left_panel_content_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    int y = rect.y + m.pad_y + m.title_glyph_h + m.section_gap;
    y += m.tab_h + m.section_gap;
    return y;
}

SDL_Rect left_panel_slot_tab_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint8_t slot_index, uint8_t slot_count) {
    int total_gap = 0;
    int tab_w = 0;
    int x = 0;
    int y = rect.y + m.pad_y + m.title_glyph_h + m.section_gap;
    if (slot_count == 0u) {
        slot_count = 1u;
    }
    total_gap = (int)(slot_count - 1u) * m.tab_gap;
    tab_w = (rect.w - (2 * m.pad_x) - total_gap) / (int)slot_count;
    if (tab_w < 36) {
        tab_w = 36;
    }
    x = rect.x + m.pad_x + (int)slot_index * (tab_w + m.tab_gap);
    return (SDL_Rect){ x, y, tab_w, m.tab_h };
}

SDL_Rect left_panel_tool_list_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t tool_count) {
    int y = left_panel_content_start_y(rect, m);
    int h = (int)tool_count * m.row_h;
    if (tool_count > 1u) {
        h += (int)(tool_count - 1u) * m.section_gap;
    }
    if (h < m.row_h) {
        h = m.row_h;
    }
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), h };
}

SDL_Rect left_panel_tool_row_rect(SDL_Rect rect,
                                  VisualPaneLayoutMetrics m,
                                  uint32_t tool_index,
                                  uint32_t tool_count) {
    SDL_Rect list = left_panel_tool_list_rect(rect, m, tool_count);
    int y = list.y + (int)tool_index * (m.row_h + m.section_gap);
    return (SDL_Rect){ list.x, y, list.w, m.row_h };
}

SDL_Rect left_panel_tool_detail_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint32_t tool_count) {
    SDL_Rect list = left_panel_tool_list_rect(rect, m, tool_count);
    int y = list.y + list.h + m.section_gap;
    int h = (rect.y + rect.h - m.pad_y) - y;
    if (h < (m.line_h + m.section_gap + m.row_h)) {
        h = m.line_h + m.section_gap + m.row_h;
    }
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), h };
}

SDL_Rect left_panel_objects_list_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    int y = left_panel_content_start_y(rect, m);
    int content_bottom = rect.y + rect.h - m.pad_y;
    int content_h = content_bottom - y;
    int list_h = (content_h * 2) / 5;
    int min_h = m.line_h + m.section_gap + (4 * m.row_h) + (3 * m.section_gap);
    if (list_h < min_h) {
        list_h = min_h;
    }
    if (list_h > content_h - (m.line_h + m.section_gap + m.row_h)) {
        list_h = content_h - (m.line_h + m.section_gap + m.row_h);
    }
    if (list_h < m.row_h) {
        list_h = m.row_h;
    }
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), list_h };
}

SDL_Rect left_panel_objects_inspector_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect list = left_panel_objects_list_rect(rect, m);
    int y = list.y + list.h + m.section_gap;
    int h = (rect.y + rect.h - m.pad_y) - y;
    if (h < (m.line_h + m.section_gap + m.row_h)) {
        h = m.line_h + m.section_gap + m.row_h;
    }
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), h };
}

int left_panel_objects_rows_start_y(SDL_Rect list_rect, VisualPaneLayoutMetrics m) {
    return list_rect.y + m.line_h + m.section_gap;
}

SDL_Rect left_panel_objects_row_rect(SDL_Rect list_rect, VisualPaneLayoutMetrics m, uint32_t row_index) {
    int y = left_panel_objects_rows_start_y(list_rect, m) + (int)row_index * (m.row_h + m.section_gap);
    return (SDL_Rect){ list_rect.x + m.tab_gap, y, list_rect.w - m.tab_gap, m.row_h };
}

SDL_Rect left_panel_objects_inspector_action_row_rect(SDL_Rect inspector_rect,
                                                      VisualPaneLayoutMetrics m,
                                                      uint32_t action_index,
                                                      uint32_t action_count) {
    int total_h = 0;
    int y = 0;
    if (action_count == 0u) {
        action_count = 1u;
    }
    total_h = (int)action_count * m.row_h;
    if (action_count > 1u) {
        total_h += (int)(action_count - 1u) * m.section_gap;
    }
    y = inspector_rect.y + inspector_rect.h - m.tab_gap - total_h;
    if (y < inspector_rect.y + m.tab_gap) {
        y = inspector_rect.y + m.tab_gap;
    }
    y += (int)action_index * (m.row_h + m.section_gap);
    return (SDL_Rect){ inspector_rect.x + m.tab_gap, y, inspector_rect.w - m.tab_gap, m.row_h };
}

int left_panel_tool_detail_rows_start_y(SDL_Rect detail_rect, VisualPaneLayoutMetrics m) {
    return detail_rect.y + m.line_h + m.section_gap;
}

SDL_Rect left_panel_tool_detail_option_row_rect(SDL_Rect detail_rect,
                                                VisualPaneLayoutMetrics m,
                                                uint32_t option_index) {
    int y = left_panel_tool_detail_rows_start_y(detail_rect, m) + (int)option_index * (m.row_h + m.section_gap);
    return (SDL_Rect){ detail_rect.x + m.tab_gap, y, detail_rect.w - m.tab_gap, m.row_h };
}

SDL_Rect left_tool_option_row_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, int y) {
    return (SDL_Rect){
        rect.x + m.pad_x + m.tab_gap,
        y,
        rect.w - (2 * m.pad_x) - m.tab_gap,
        m.row_h
    };
}

SDL_Rect left_tool_option_minus_rect(SDL_Rect row, VisualPaneLayoutMetrics m) {
    int button_w = m.tab_h;
    int value_w = (int)((float)row.w * 0.34f);
    int value_x;
    if (value_w < 74) {
        value_w = 74;
    }
    if (value_w > 126) {
        value_w = 126;
    }
    value_x = row.x + row.w - button_w - m.tab_gap - value_w;
    return (SDL_Rect){ value_x - m.tab_gap - button_w, row.y, button_w, row.h };
}

SDL_Rect left_tool_option_plus_rect(SDL_Rect row, VisualPaneLayoutMetrics m) {
    int button_w = m.tab_h;
    return (SDL_Rect){ row.x + row.w - button_w, row.y, button_w, row.h };
}

SDL_Rect left_tool_option_value_rect(SDL_Rect row, VisualPaneLayoutMetrics m) {
    SDL_Rect plus = left_tool_option_plus_rect(row, m);
    int value_w = (int)((float)row.w * 0.34f);
    if (value_w < 74) {
        value_w = 74;
    }
    if (value_w > 126) {
        value_w = 126;
    }
    return (SDL_Rect){ plus.x - m.tab_gap - value_w, row.y, value_w, row.h };
}

int cycle_theme_preset(CoreThemePresetId current, int direction, CoreThemePresetId *out_next) {
    uint32_t i;
    uint32_t count = (uint32_t)(sizeof(k_visual_theme_cycle_order) / sizeof(k_visual_theme_cycle_order[0]));
    if (!out_next || count == 0u) {
        return 0;
    }
    for (i = 0u; i < count; ++i) {
        if (k_visual_theme_cycle_order[i] == current) {
            if (direction >= 0) {
                *out_next = k_visual_theme_cycle_order[(i + 1u) % count];
            } else {
                *out_next = k_visual_theme_cycle_order[(i + count - 1u) % count];
            }
            return 1;
        }
    }
    *out_next = k_visual_theme_cycle_order[0];
    return 1;
}
