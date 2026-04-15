#include "drawing_program/drawing_program_visual_panel_render.h"

#include <stdio.h>

#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_theme.h"

enum {
    VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE = 0,
    VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE = 1
};

static int ui_hovered(const VisualPanelUiState *ui,
                      SDL_Rect rect,
                      const DrawingProgramVisualPanelRenderHooks *hooks) {
    if (!ui || !ui->mouse_known || !hooks || !hooks->point_in_rect) {
        return 0;
    }
    return hooks->point_in_rect(rect, ui->mouse_x, ui->mouse_y);
}

static void draw_tab_button(SDL_Renderer *renderer,
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

static const char *shape_target_mode_name(uint8_t mode) {
    return (mode == (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_OBJECT) ? "OBJECT" : "PIXEL";
}

static const char *select_mode_name(uint8_t mode) {
    switch (mode) {
        case (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_ADD: return "ADD";
        case (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_SUBTRACT: return "SUBTRACT";
        case (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE:
        default:
            return "REPLACE";
    }
}

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
        int hovered = ui_hovered(ui, row, hooks);
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
            draw_tab_button(renderer,
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
                            ui_hovered(ui, option_row, hooks),
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
            draw_tab_button(renderer,
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
                            ui_hovered(ui, minus_rect, hooks),
                            hooks);
            draw_tab_button(renderer,
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
            draw_tab_button(renderer,
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
                            ui_hovered(ui, plus_rect, hooks),
                            hooks);
        }
    }
}

void drawing_program_visual_render_right_panel_chrome(SDL_Renderer *renderer,
                                                      SDL_Rect rect,
                                                      const DrawingProgramAppContext *ctx,
                                                      const CoreThemePreset *theme,
                                                      const VisualPanelUiState *ui,
                                                      const VisualSelectionState *selection,
                                                      const VisualCanvasInteractionState *interaction,
                                                      const DrawingProgramVisualPanelRenderHooks *hooks) {
    char line[160];
    uint8_t right_slot;
    VisualPaneLayoutMetrics m;
    VisualThemePalette p;
    SDL_Rect tab_canvas;
    SDL_Rect tab_layer;
    SDL_Rect row;
    int y;
    uint8_t active_visible = 0u;
    uint8_t active_locked = 0u;
    uint8_t active_opacity = 100u;
    uint32_t i;
    if (!renderer || !ctx || !hooks || !hooks->draw_bitmap_text || !hooks->clamp_right_slot || !hooks->tool_name ||
        !hooks->active_layer_allows_edits_visual || !hooks->tool_brush_radius_samples ||
        !hooks->tool_brush_spacing_samples || !hooks->tool_uses_shape_commit || !hooks->clamp_setting_u8 ||
        !hooks->shape_mode_name || !hooks->tool_shape_mode || !hooks->tool_fill_tolerance_setting ||
        !hooks->fill_tolerance_sample_delta || !hooks->selection_component_count || !hooks->pane_rect_for_module_type ||
        !hooks->color_index_clamp || !hooks->color_value_from_index || !hooks->color_rgb_from_index) {
        return;
    }
    right_slot = hooks->clamp_right_slot(ctx->ui_right_panel_slot);
    m = make_pane_layout_metrics(ctx);
    resolve_visual_theme_palette(theme, &p);

    for (i = 0u; i < ctx->document.layer_count; ++i) {
        if (ctx->document.layers[i].layer_id == ctx->editor.active_layer_id) {
            active_visible = ctx->document.layers[i].visible;
            active_locked = ctx->document.layers[i].locked;
            active_opacity = drawing_program_visual_layer_opacity_get(ctx, ctx->editor.active_layer_id);
            break;
        }
    }

    y = rect.y + m.pad_y;
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "RIGHT PANEL", p.text_primary, m.title_scale);
    y += m.title_glyph_h + m.section_gap;
    tab_canvas = (SDL_Rect){ rect.x + m.pad_x, y, (rect.w - (2 * m.pad_x) - m.tab_gap) / 2, m.tab_h };
    tab_layer = (SDL_Rect){ tab_canvas.x + tab_canvas.w + m.tab_gap, y, (rect.w - (2 * m.pad_x) - m.tab_gap) / 2, m.tab_h };
    draw_tab_button(renderer,
                    rect,
                    tab_canvas,
                    "CANVAS",
                    p.button_fill,
                    p.button_fill_hover,
                    p.button_fill_active,
                    p.button_border,
                    (right_slot == VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE) ? p.text_primary : p.text_muted,
                    m.body_scale,
                    right_slot == VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE,
                    ui_hovered(ui, tab_canvas, hooks),
                    hooks);
    draw_tab_button(renderer,
                    rect,
                    tab_layer,
                    "LAYER",
                    p.button_fill,
                    p.button_fill_hover,
                    p.button_fill_active,
                    p.button_border,
                    (right_slot == VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE) ? p.text_primary : p.text_muted,
                    m.body_scale,
                    right_slot == VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE,
                    ui_hovered(ui, tab_layer, hooks),
                    hooks);
    y += m.tab_h + m.section_gap;

    if (right_slot == VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE) {
        const char *pointer_state = "POINTER: PANEL";
        const char *interaction_hint = "LMB: DRAW  RMB: PAN VIEW  WHEEL: ZOOM";
        uint32_t history_cursor_units = 0u;
        uint32_t history_count_units = 0u;
        uint8_t active_color_index = hooks->color_index_clamp(ctx->ui_active_color_index);
        uint8_t active_color_value = hooks->color_value_from_index(active_color_index);
        uint8_t swatch_r = 0u;
        uint8_t swatch_g = 0u;
        uint8_t swatch_b = 0u;
        SDL_Rect reset_view_button;
        SDL_Rect clear_canvas_button;
        SDL_Rect delete_selection_button;
        SDL_Rect clear_history_button;
        uint8_t palette_i;
        uint32_t brush_radius = hooks->tool_brush_radius_samples(ctx, ctx->editor.active_tool);
        uint32_t brush_spacing = hooks->tool_brush_spacing_samples(ctx, ctx->editor.active_tool, brush_radius);
        uint32_t selection_w = (selection && selection->has_payload) ? selection->width : 0u;
        uint32_t selection_h = (selection && selection->has_payload) ? selection->height : 0u;
        uint32_t selection_payload = (selection && selection->has_payload) ? selection->payload_count : 0u;
        uint32_t selection_layer_id = (selection && selection->has_payload) ? selection->layer_id : 0u;
        uint32_t selection_regions = 0u;
        int delete_selection_enabled = (selection &&
                                        selection->has_payload &&
                                        hooks->active_layer_allows_edits_visual(ctx))
                                           ? 1
                                           : 0;
        uint32_t clipboard_w = ctx->clipboard.has_payload ? ctx->clipboard.width : 0u;
        uint32_t clipboard_h = ctx->clipboard.has_payload ? ctx->clipboard.height : 0u;
        uint32_t clipboard_payload = ctx->clipboard.has_payload ? ctx->clipboard.payload_count : 0u;
        uint32_t clipboard_source_layer_id = ctx->clipboard.has_payload ? ctx->clipboard.source_layer_id : 0u;
        const char *transform_axis = "FREE";
        if (ui && ui->mouse_known) {
            SDL_Rect canvas_rect = { 0, 0, 0, 0 };
            if (hooks->pane_rect_for_module_type(ctx, 1u, &canvas_rect) &&
                hooks->point_in_rect(canvas_rect, ui->mouse_x, ui->mouse_y)) {
                pointer_state = "POINTER: CANVAS";
            }
        }
        if (selection && selection->has_payload) {
            selection_regions = hooks->selection_component_count(selection);
            if (selection_regions == 0u) {
                selection_regions = 1u;
            }
        }
        if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_SELECT) {
            interaction_hint = (ctx->object_selection.count > 0u)
                                   ? "LMB OBJECT: PICK  EMPTY:MARQUEE  SHIFT+LMB:ADD  ALT+LMB:REMOVE"
                                   : "LMB: MARQUEE  SHIFT:ADD  ALT:SUBTRACT";
        } else if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE) {
            if (ctx->object_selection.count > 0u) {
                interaction_hint = "LMB: MOVE OBJECTS  ARROWS:NUDGE  SHIFT+ARROW:x10  CMD+R:RASTERIZE";
            } else {
                interaction_hint = selection && selection->has_payload
                                       ? "LMB: MOVE SEL  ARROWS:NUDGE  SHIFT+ARROW:x10"
                                       : "MOVE TOOL: NO ACTIVE SELECTION";
            }
        } else if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_FILL) {
            interaction_hint = "LMB: FILL REGION  TOLERANCE CONTROLS MATCH RANGE";
        } else if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_PICKER) {
            interaction_hint = "LMB: PICK COLOR";
        } else if (ctx->object_selection.count > 0u) {
            interaction_hint = "OBJECTS SELECTED  USE SELECT/MOVE OR CMD+R RASTERIZE";
        }
        if (selection && selection->moving) {
            if (interaction && interaction->move_axis_lock == 1u) {
                transform_axis = "X";
            } else if (interaction && interaction->move_axis_lock == 2u) {
                transform_axis = "Y";
            }
        } else if (interaction && interaction->object_move_active) {
            if (interaction->move_axis_lock == 1u) {
                transform_axis = "X";
            } else if (interaction->move_axis_lock == 2u) {
                transform_axis = "Y";
            }
        }
        drawing_program_history_query_units(&ctx->history, &history_cursor_units, &history_count_units);
        (void)snprintf(line, sizeof(line), "ACTIVE COLOR %u (%u)", (unsigned)active_color_index + 1u, (unsigned)active_color_value);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_primary, m.body_scale);
        y = right_canvas_palette_header_y(rect, m);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "PALETTE", p.text_primary, m.body_scale);
        for (palette_i = 0u; palette_i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++palette_i) {
            SDL_Rect swatch = right_canvas_palette_swatch_rect(rect, m, palette_i);
            int selected = (palette_i == active_color_index) ? 1 : 0;
            hooks->color_rgb_from_index(palette_i, &swatch_r, &swatch_g, &swatch_b);
            SDL_SetRenderDrawColor(renderer, swatch_r, swatch_g, swatch_b, 255u);
            (void)SDL_RenderFillRect(renderer, &swatch);
            if (selected) {
                SDL_SetRenderDrawColor(renderer, p.accent_primary.r, p.accent_primary.g, p.accent_primary.b, 255u);
            } else {
                SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
            }
            (void)SDL_RenderDrawRect(renderer, &swatch);
        }

        y = right_canvas_metrics_start_y(rect, m);
        (void)snprintf(line, sizeof(line), "HISTORY %u/%u", history_cursor_units, history_count_units);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_BRUSH) {
            (void)snprintf(line,
                           sizeof(line),
                           "TOOL %s  R%u S%u O%u%% H%u%%",
                           hooks->tool_name(ctx->editor.active_tool),
                           brush_radius,
                           brush_spacing,
                           (unsigned)hooks->clamp_setting_u8(ctx->ui_tool_brush_opacity, 1u, 100u),
                           (unsigned)hooks->clamp_setting_u8(ctx->ui_tool_brush_hardness, 1u, 100u));
        } else if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_ERASER) {
            (void)snprintf(line,
                           sizeof(line),
                           "TOOL %s  R%u S%u",
                           hooks->tool_name(ctx->editor.active_tool),
                           brush_radius,
                           brush_spacing);
        } else if (hooks->tool_uses_shape_commit(ctx->editor.active_tool)) {
            if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_LINE) {
                (void)snprintf(line,
                               sizeof(line),
                               "TOOL %s  W%u",
                               hooks->tool_name(ctx->editor.active_tool),
                               (unsigned)hooks->clamp_setting_u8(ctx->ui_tool_shape_stroke_width, 1u, 16u));
            } else {
                (void)snprintf(line,
                               sizeof(line),
                               "TOOL %s  W%u MODE %s TARGET %s",
                               hooks->tool_name(ctx->editor.active_tool),
                               (unsigned)hooks->clamp_setting_u8(ctx->ui_tool_shape_stroke_width, 1u, 16u),
                               hooks->shape_mode_name(hooks->tool_shape_mode(ctx)),
                               shape_target_mode_name(
                                   hooks->clamp_setting_u8(ctx->ui_tool_shape_target_mode, 0u, 1u)));
            }
        } else if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_FILL) {
            (void)snprintf(line,
                           sizeof(line),
                           "TOOL %s  TOL %u (d%u)",
                           hooks->tool_name(ctx->editor.active_tool),
                           (unsigned)hooks->tool_fill_tolerance_setting(ctx),
                           (unsigned)hooks->fill_tolerance_sample_delta(hooks->tool_fill_tolerance_setting(ctx)));
        } else if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_SELECT) {
            (void)snprintf(line,
                           sizeof(line),
                           "TOOL %s  MODE %s",
                           hooks->tool_name(ctx->editor.active_tool),
                           select_mode_name(hooks->clamp_setting_u8(ctx->ui_tool_select_mode, 0u, 2u)));
        } else {
            (void)snprintf(line, sizeof(line), "TOOL %s", hooks->tool_name(ctx->editor.active_tool));
        }
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line,
                       sizeof(line),
                       "LAYER %u O%u%% V%s K%s",
                       (unsigned)ctx->editor.active_layer_id,
                       (unsigned)active_opacity,
                       active_visible ? "ON" : "OFF",
                       active_locked ? "ON" : "OFF");
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line, sizeof(line), "VIEW Z%.2fx PAN %d,%d", (double)ctx->editor.viewport.zoom, (int)ctx->editor.viewport.pan_x, (int)ctx->editor.viewport.pan_y);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        if (selection && selection->has_payload) {
            (void)snprintf(line,
                           sizeof(line),
                           "SELECTION %ux%u P%u R%u L%u",
                           selection_w,
                           selection_h,
                           selection_payload,
                           selection_regions,
                           selection_layer_id);
        } else {
            (void)snprintf(line, sizeof(line), "SELECTION NONE");
        }
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        if (ctx->object_selection.count > 0u) {
            const DrawingProgramObjectRecord *active_object =
                drawing_program_object_store_get_by_id(&ctx->object_store, ctx->object_selection.active_object_id);
            (void)snprintf(line,
                           sizeof(line),
                           "OBJECTS %u ACTIVE %u",
                           (unsigned)ctx->object_selection.count,
                           (unsigned)ctx->object_selection.active_object_id);
            hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
            y += m.line_h;
            if (active_object) {
                (void)snprintf(line,
                               sizeof(line),
                               "OBJ ORIGIN %d,%d SIZE %ux%u",
                               (int)active_object->origin_x,
                               (int)active_object->origin_y,
                               (unsigned)active_object->width,
                               (unsigned)active_object->height);
            } else {
                (void)snprintf(line, sizeof(line), "OBJ ORIGIN n/a");
            }
        } else {
            (void)snprintf(line, sizeof(line), "OBJECTS NONE");
            hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
            y += m.line_h;
            (void)snprintf(line, sizeof(line), "OBJ ORIGIN n/a");
        }
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        if (selection && selection->moving) {
            (void)snprintf(line,
                           sizeof(line),
                           "TRANSFORM MOVE d(%d,%d) AXIS %s",
                           (int)selection->offset_x,
                           (int)selection->offset_y,
                           transform_axis);
        } else if (interaction && interaction->object_move_active) {
            (void)snprintf(line,
                           sizeof(line),
                           "TRANSFORM OBJ d(%d,%d) AXIS %s",
                           (int)interaction->object_move_offset_x,
                           (int)interaction->object_move_offset_y,
                           transform_axis);
        } else if (selection && selection->selecting) {
            (void)snprintf(line, sizeof(line), "TRANSFORM MARQUEE ACTIVE");
        } else {
            (void)snprintf(line, sizeof(line), "TRANSFORM IDLE");
        }
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        if (ctx->clipboard.has_payload) {
            (void)snprintf(line, sizeof(line), "CLIPBOARD %ux%u P%u L%u", clipboard_w, clipboard_h, clipboard_payload, clipboard_source_layer_id);
            hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
            y += m.line_h;
        }
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, pointer_state, p.text_muted, m.body_scale);
        y += m.line_h;
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, interaction_hint, p.text_muted, m.body_scale);

        reset_view_button = right_canvas_reset_view_button_rect(rect, m);
        clear_canvas_button = right_canvas_clear_canvas_button_rect(rect, m);
        delete_selection_button = right_canvas_delete_selection_button_rect(rect, m);
        clear_history_button = right_canvas_clear_history_button_rect(rect, m);

        draw_tab_button(renderer, rect, reset_view_button, "RESET VIEW",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, reset_view_button, hooks), hooks);
        draw_tab_button(renderer, rect, clear_canvas_button, "CLEAR CANVAS",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, clear_canvas_button, hooks), hooks);
        draw_tab_button(renderer, rect, delete_selection_button, "DELETE SELECTION",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        delete_selection_enabled ? p.text_primary : p.text_muted,
                        m.body_scale, 0, ui_hovered(ui, delete_selection_button, hooks), hooks);
        draw_tab_button(renderer, rect, clear_history_button, "CLEAR HISTORY",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, clear_history_button, hooks), hooks);
    } else {
        uint32_t display_i;
        SDL_Rect button_rect;
        SDL_Rect opacity_row_rect;
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "LAYER STACK", p.text_primary, m.body_scale);
        y += m.line_h;
        if (ctx->document.layer_count == 0u) {
            hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "NO LAYERS", p.text_muted, m.body_scale);
            y += m.line_h;
        }
        for (display_i = 0u; display_i < ctx->document.layer_count; ++display_i) {
            uint32_t model_i = (ctx->document.layer_count - 1u) - display_i;
            const DrawingProgramLayer *layer = &ctx->document.layers[model_i];
            int is_active = (layer->layer_id == ctx->editor.active_layer_id) ? 1 : 0;
            uint8_t layer_opacity = drawing_program_visual_layer_opacity_get(ctx, layer->layer_id);
            SDL_Color fill = is_active ? p.button_fill_active : p.button_fill;
            row = right_layer_row_rect(rect, m, display_i);
            if (ui_hovered(ui, row, hooks)) {
                fill = is_active ? p.button_fill_active : p.button_fill_hover;
            }
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            (void)SDL_RenderFillRect(renderer, &row);
            if (is_active) {
                SDL_SetRenderDrawColor(renderer, p.accent_primary.r, p.accent_primary.g, p.accent_primary.b, 255u);
            } else {
                SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
            }
            (void)SDL_RenderDrawRect(renderer, &row);
            (void)snprintf(line,
                           sizeof(line),
                           "%c L%u O%u%% V%s K%s %s",
                           is_active ? '*' : ' ',
                           (unsigned)layer->layer_id,
                           (unsigned)layer_opacity,
                           layer->visible ? "ON" : "OFF",
                           layer->locked ? "ON" : "OFF",
                           layer->name);
            hooks->draw_bitmap_text(renderer, rect, row.x + 6, row.y + m.row_text_y, line, p.text_primary, m.body_scale);
            y = row.y + row.h + m.section_gap;
        }

        opacity_row_rect = right_layer_opacity_row_rect(rect, m, ctx->document.layer_count);
        draw_tab_button(renderer,
                        rect,
                        opacity_row_rect,
                        "OPACITY",
                        p.button_fill,
                        p.button_fill_hover,
                        p.button_fill_active,
                        p.button_border,
                        p.text_muted,
                        m.body_scale,
                        0,
                        ui_hovered(ui, opacity_row_rect, hooks),
                        hooks);
        {
            SDL_Rect opacity_track = right_layer_opacity_track_rect(opacity_row_rect, m);
            SDL_SetRenderDrawColor(renderer, p.pane_background_alt.r, p.pane_background_alt.g, p.pane_background_alt.b, p.pane_background_alt.a);
            (void)SDL_RenderFillRect(renderer, &opacity_track);
            {
                SDL_Rect fill_track = opacity_track;
                int fill_w = (int)(((uint32_t)opacity_track.w * (uint32_t)active_opacity) / 100u);
                if (fill_w < 1) {
                    fill_w = 1;
                }
                fill_track.w = fill_w;
                SDL_SetRenderDrawColor(renderer, p.accent_primary.r, p.accent_primary.g, p.accent_primary.b, p.accent_primary.a);
                (void)SDL_RenderFillRect(renderer, &fill_track);
            }
            SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
            (void)SDL_RenderDrawRect(renderer, &opacity_track);
            (void)snprintf(line, sizeof(line), "OPACITY %u%%", (unsigned)active_opacity);
            hooks->draw_bitmap_text(renderer, rect, opacity_row_rect.x + 6, opacity_row_rect.y + m.row_text_y, line, p.text_primary, m.body_scale);
        }

        y = right_layer_actions_label_y(rect, m, ctx->document.layer_count);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "ACTIONS", p.text_primary, m.body_scale);

        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ADD);
        draw_tab_button(renderer, rect, button_rect, "ADD LAYER",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_DUPLICATE);
        draw_tab_button(renderer, rect, button_rect, "DUPLICATE SELECTED",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_RENAME);
        draw_tab_button(renderer, rect, button_rect, "RENAME SELECTED",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_DELETE);
        draw_tab_button(renderer, rect, button_rect, "DELETE SELECTED",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ACTIVE_PREV);
        draw_tab_button(renderer, rect, button_rect, "ACTIVE PREV",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ACTIVE_NEXT);
        draw_tab_button(renderer, rect, button_rect, "ACTIVE NEXT",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_MOVE_UP);
        draw_tab_button(renderer, rect, button_rect, "MOVE UP",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_MOVE_DOWN);
        draw_tab_button(renderer, rect, button_rect, "MOVE DOWN",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_TOGGLE_VISIBLE);
        draw_tab_button(renderer,
                        rect,
                        button_rect,
                        active_visible ? "VISIBLE: ON" : "VISIBLE: OFF",
                        p.button_fill,
                        p.button_fill_hover,
                        p.button_fill_active,
                        p.button_border,
                        p.text_primary,
                        m.body_scale,
                        0,
                        ui_hovered(ui, button_rect, hooks),
                        hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_TOGGLE_LOCK);
        draw_tab_button(renderer,
                        rect,
                        button_rect,
                        active_locked ? "LOCK: ON" : "LOCK: OFF",
                        p.button_fill,
                        p.button_fill_hover,
                        p.button_fill_active,
                        p.button_border,
                        p.text_primary,
                        m.body_scale,
                        0,
                        ui_hovered(ui, button_rect, hooks),
                        hooks);
    }
}
