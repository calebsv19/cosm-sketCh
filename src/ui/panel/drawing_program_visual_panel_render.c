#include "drawing_program/drawing_program_visual_panel_render.h"

#include <stdio.h>

#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_panel_render_common.h"
#include "drawing_program/drawing_program_visual_right_panel_render.h"
#include "drawing_program/drawing_program_visual_theme.h"

void drawing_program_visual_render_menu_bar_chrome(SDL_Renderer *renderer,
                                                   SDL_Rect rect,
                                                   const DrawingProgramAppContext *ctx,
                                                   const CoreThemePreset *theme,
                                                   const DrawingProgramVisualPanelRenderHooks *hooks) {
    VisualPaneLayoutMetrics m;
    VisualThemePalette p;
    SDL_Rect chip;
    const char *menu_label = "MENU  FILE  EDIT  VIEW  HELP";
    const char *tool_label = "ACTIVE TOOL:";
    const char *tool_value = 0;
    int chip_w;
    int chip_x;
    int chip_right;
    int min_chip_x;
    int min_chip_w;
    int available_w;
    int desired_w;
    int menu_w;
    int label_w;
    int tool_w;
    int header_y;
    int tool_x = 0;
    if (!renderer || !ctx || !hooks || !hooks->draw_bitmap_text || !hooks->measure_bitmap_text_width ||
        !hooks->tool_name) {
        return;
    }
    resolve_visual_theme_palette(theme, &p);
    m = make_pane_layout_metrics(ctx);
    header_y = rect.y + m.pad_y;
    tool_value = hooks->tool_name(ctx->editor.active_tool);
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, header_y, menu_label, p.text_primary, m.title_scale);

    menu_w = hooks->measure_bitmap_text_width(menu_label, m.title_scale);
    label_w = hooks->measure_bitmap_text_width(tool_label, m.body_scale);
    tool_w = hooks->measure_bitmap_text_width(tool_value, m.body_scale);
    desired_w = 6 + label_w + 6 + tool_w + 6;
    min_chip_w = 120;
    if (desired_w < min_chip_w) {
        desired_w = min_chip_w;
    }

    min_chip_x = rect.x + m.pad_x + menu_w + 12;
    chip_right = rect.x + rect.w - m.pad_x;
    available_w = chip_right - min_chip_x;
    if (available_w < min_chip_w) {
        available_w = min_chip_w;
        min_chip_x = chip_right - available_w;
    }
    chip_w = desired_w;
    if (chip_w > available_w) {
        chip_w = available_w;
    }
    chip_x = chip_right - chip_w;
    if (chip_x < min_chip_x) {
        chip_x = min_chip_x;
    }

    chip.x = chip_x;
    chip.y = header_y - 1;
    chip.w = chip_w;
    chip.h = m.tab_h;
    SDL_SetRenderDrawColor(renderer, p.button_fill.r, p.button_fill.g, p.button_fill.b, p.button_fill.a);
    (void)SDL_RenderFillRect(renderer, &chip);
    SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
    (void)SDL_RenderDrawRect(renderer, &chip);
    label_w =
        hooks->draw_bitmap_text(renderer, chip, chip.x + 6, chip.y + m.tab_text_y, tool_label, p.text_muted, m.body_scale);
    tool_x = chip.x + 6 + label_w + 6;
    hooks->draw_bitmap_text(renderer, chip, tool_x, chip.y + m.tab_text_y, tool_value, p.text_primary, m.body_scale);
}

void drawing_program_visual_render_left_panel_chrome(SDL_Renderer *renderer,
                                                     SDL_Rect rect,
                                                     const DrawingProgramAppContext *ctx,
                                                     const CoreThemePreset *theme,
                                                     const VisualPanelUiState *ui,
                                                     const DrawingProgramVisualPanelRenderHooks *hooks) {
    uint32_t i;
    uint32_t tool_count;
    uint32_t option_count;
    VisualPaneLayoutMetrics m;
    VisualThemePalette p;
    SDL_Rect detail_rect;
    int y;
    char detail_header[64];
    if (!renderer || !ctx || !hooks || !hooks->draw_bitmap_text || !hooks->clamp_left_slot ||
        !hooks->visual_tool_count || !hooks->visual_tool_at || !hooks->visual_tool_option_count ||
        !hooks->visual_tool_option_kind_for_index_raw || !hooks->visual_tool_option_is_action_button_raw ||
        !hooks->visual_tool_option_label_raw || !hooks->visual_tool_option_value_text_raw || !hooks->tool_name) {
        return;
    }
    m = make_pane_layout_metrics(ctx);
    resolve_visual_theme_palette(theme, &p);
    tool_count = hooks->visual_tool_count();
    option_count = hooks->visual_tool_option_count(ctx, ctx->editor.active_tool);

    y = rect.y + m.pad_y;
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "LEFT PANEL", p.text_primary, m.title_scale);
    for (i = 0u; i < tool_count; ++i) {
        DrawingProgramToolKind tool = hooks->visual_tool_at(i);
        SDL_Rect row = left_panel_tool_row_rect(rect, m, i, tool_count);
        int active = (ctx->editor.active_tool == tool) ? 1 : 0;
        int hovered = drawing_program_visual_panel_ui_hovered(ui, row, hooks);
        SDL_Color row_color = active ? p.button_fill_active : (hovered ? p.button_fill_hover : p.button_fill);
        SDL_Color label_color = active ? p.text_primary : sdl_color_ensure_contrast(p.text_muted, row_color);
        SDL_SetRenderDrawColor(renderer, row_color.r, row_color.g, row_color.b, row_color.a);
        (void)SDL_RenderFillRect(renderer, &row);
        SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
        (void)SDL_RenderDrawRect(renderer, &row);
        hooks->draw_bitmap_text(
            renderer, rect, row.x + 6, row.y + m.row_text_y, hooks->tool_name(tool), label_color, m.body_scale);
    }

    detail_rect = left_panel_tool_detail_rect(rect, m, tool_count);
    SDL_SetRenderDrawColor(renderer,
                           p.pane_background_alt.r,
                           p.pane_background_alt.g,
                           p.pane_background_alt.b,
                           p.pane_background_alt.a);
    (void)SDL_RenderFillRect(renderer, &detail_rect);
    SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
    (void)SDL_RenderDrawRect(renderer, &detail_rect);

    (void)snprintf(detail_header,
                   sizeof(detail_header),
                   "SETTINGS: %s",
                   hooks->tool_name(ctx->editor.active_tool));
    hooks->draw_bitmap_text(
        renderer, rect, detail_rect.x + 6, detail_rect.y + m.row_text_y, detail_header, p.text_primary, m.body_scale);
    if (option_count == 0u) {
        hooks->draw_bitmap_text(renderer,
                                rect,
                                detail_rect.x + 6,
                                detail_rect.y + m.line_h + m.row_text_y,
                                "NO TOOL SETTINGS",
                                p.text_muted,
                                m.body_scale);
        return;
    }
    for (i = 0u; i < option_count; ++i) {
        uint32_t option_kind_raw =
            hooks->visual_tool_option_kind_for_index_raw(ctx, ctx->editor.active_tool, i);
        SDL_Rect option_row = left_panel_tool_detail_option_row_rect(detail_rect, m, i);
        char value_text[48];
        if (option_row.y + option_row.h > detail_rect.y + detail_rect.h) {
            break;
        }
        hooks->visual_tool_option_value_text_raw(ctx, option_kind_raw, value_text, sizeof(value_text));
        if (hooks->visual_tool_option_is_action_button_raw(option_kind_raw)) {
            drawing_program_visual_panel_draw_tab_button(renderer,
                            rect,
                            option_row,
                            value_text,
                            p.button_fill,
                            p.button_fill_hover,
                            p.button_fill_active,
                            p.button_border,
                            p.text_primary,
                            m.body_scale,
                            0,
                            drawing_program_visual_panel_ui_hovered(ui, option_row, hooks),
                            hooks);
        } else {
            SDL_Rect minus_rect = left_tool_option_minus_rect(option_row, m);
            SDL_Rect value_rect = left_tool_option_value_rect(option_row, m);
            SDL_Rect plus_rect = left_tool_option_plus_rect(option_row, m);
            SDL_SetRenderDrawColor(renderer, p.button_fill.r, p.button_fill.g, p.button_fill.b, p.button_fill.a);
            (void)SDL_RenderFillRect(renderer, &option_row);
            SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
            (void)SDL_RenderDrawRect(renderer, &option_row);
            hooks->draw_bitmap_text(renderer,
                                    rect,
                                    option_row.x + 6,
                                    option_row.y + m.row_text_y,
                                    hooks->visual_tool_option_label_raw(option_kind_raw),
                                    p.text_muted,
                                    m.body_scale);
            drawing_program_visual_panel_draw_tab_button(renderer,
                            rect,
                            minus_rect,
                            "-",
                            p.button_fill,
                            p.button_fill_hover,
                            p.button_fill_active,
                            p.button_border,
                            p.text_primary,
                            m.body_scale,
                            0,
                            drawing_program_visual_panel_ui_hovered(ui, minus_rect, hooks),
                            hooks);
            drawing_program_visual_panel_draw_tab_button(renderer,
                            rect,
                            value_rect,
                            value_text,
                            p.button_fill,
                            p.button_fill,
                            p.button_fill,
                            p.button_border,
                            p.text_primary,
                            m.body_scale,
                            0,
                            0,
                            hooks);
            drawing_program_visual_panel_draw_tab_button(renderer,
                            rect,
                            plus_rect,
                            "+",
                            p.button_fill,
                            p.button_fill_hover,
                            p.button_fill_active,
                            p.button_border,
                            p.text_primary,
                            m.body_scale,
                            0,
                            drawing_program_visual_panel_ui_hovered(ui, plus_rect, hooks),
                            hooks);
        }
    }
}
