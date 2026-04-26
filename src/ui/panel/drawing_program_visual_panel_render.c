#include "drawing_program/drawing_program_visual_panel_render.h"

#include <stdio.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_panel_render_common.h"
#include "drawing_program/drawing_program_visual_right_panel_render.h"
#include "drawing_program/drawing_program_visual_theme.h"

enum {
    VISUAL_LEFT_PANEL_SLOT_TOOLS_VALUE = 0,
    VISUAL_LEFT_PANEL_SLOT_OBJECTS_VALUE = 1
};

static const char *visual_object_style_name(uint8_t style_mode) {
    switch (style_mode) {
        case 1u: return "FILL";
        case 2u: return "BOTH";
        case 0u:
        default:
            return "OUTLINE";
    }
}

static int visual_object_style_fill_enabled(uint8_t style_mode) {
    return (style_mode == 1u || style_mode == 2u) ? 1 : 0;
}

static const char *visual_object_type_name(uint8_t type) {
    switch (type) {
        case (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT: return "RECT";
        case (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_ELLIPSE: return "ELLIPSE";
        case (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH: return "PATH";
        default: return "OBJECT";
    }
}

static const char *visual_layer_name_for_id(const DrawingProgramDocument *document, uint32_t layer_id) {
    uint32_t layer_index = 0u;
    if (!document ||
        drawing_program_document_layer_index_for_id(document, layer_id, &layer_index).code != CORE_OK ||
        layer_index >= document->layer_count) {
        return "UNKNOWN";
    }
    return document->layers[layer_index].name[0] ? document->layers[layer_index].name : "LAYER";
}

static void visual_object_sample_label(DrawingProgramRasterSample sample,
                                       char *buffer,
                                       size_t buffer_size) {
    uint8_t r = 0u;
    uint8_t g = 0u;
    uint8_t b = 0u;
    uint8_t a = 0u;
    if (!buffer || buffer_size == 0u) {
        return;
    }
    drawing_program_color_rgba_from_sample(sample, &r, &g, &b, &a);
    if (a < 255u) {
        (void)snprintf(buffer, buffer_size, "#%02X%02X%02X A%u", (unsigned)r, (unsigned)g, (unsigned)b, (unsigned)a);
    } else {
        (void)snprintf(buffer, buffer_size, "#%02X%02X%02X", (unsigned)r, (unsigned)g, (unsigned)b);
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
    uint8_t left_slot;
    VisualPaneLayoutMetrics m;
    VisualThemePalette p;
    SDL_Rect tab_tools;
    SDL_Rect tab_objects;
    SDL_Rect detail_rect;
    SDL_Rect object_list_rect;
    SDL_Rect object_inspector_rect;
    const DrawingProgramObjectRecord *selected_object = 0;
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
    left_slot = hooks->clamp_left_slot(ctx->ui.left_panel_slot);

    y = rect.y + m.pad_y;
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "LEFT PANEL", p.text_primary, m.title_scale);
    tab_tools = left_panel_slot_tab_rect(rect, m, VISUAL_LEFT_PANEL_SLOT_TOOLS_VALUE, 2u);
    tab_objects = left_panel_slot_tab_rect(rect, m, VISUAL_LEFT_PANEL_SLOT_OBJECTS_VALUE, 2u);
    drawing_program_visual_panel_draw_tab_button(renderer,
                                                 rect,
                                                 tab_tools,
                                                 "TOOLS",
                                                 p.button_fill,
                                                 p.button_fill_hover,
                                                 p.button_fill_active,
                                                 p.button_border,
                                                 (left_slot == VISUAL_LEFT_PANEL_SLOT_TOOLS_VALUE) ? p.text_primary : p.text_muted,
                                                 m.body_scale,
                                                 left_slot == VISUAL_LEFT_PANEL_SLOT_TOOLS_VALUE,
                                                 drawing_program_visual_panel_ui_hovered(ui, tab_tools, hooks),
                                                 hooks);
    drawing_program_visual_panel_draw_tab_button(renderer,
                                                 rect,
                                                 tab_objects,
                                                 "OBJECTS",
                                                 p.button_fill,
                                                 p.button_fill_hover,
                                                 p.button_fill_active,
                                                 p.button_border,
                                                 (left_slot == VISUAL_LEFT_PANEL_SLOT_OBJECTS_VALUE) ? p.text_primary : p.text_muted,
                                                 m.body_scale,
                                                 left_slot == VISUAL_LEFT_PANEL_SLOT_OBJECTS_VALUE,
                                                 drawing_program_visual_panel_ui_hovered(ui, tab_objects, hooks),
                                                 hooks);

    if (left_slot == VISUAL_LEFT_PANEL_SLOT_OBJECTS_VALUE) {
        object_list_rect = left_panel_objects_list_rect(rect, m);
        object_inspector_rect = left_panel_objects_inspector_rect(rect, m);

        SDL_SetRenderDrawColor(renderer,
                               p.pane_background_alt.r,
                               p.pane_background_alt.g,
                               p.pane_background_alt.b,
                               p.pane_background_alt.a);
        (void)SDL_RenderFillRect(renderer, &object_list_rect);
        SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
        (void)SDL_RenderDrawRect(renderer, &object_list_rect);
        hooks->draw_bitmap_text(renderer,
                                rect,
                                object_list_rect.x + 6,
                                object_list_rect.y + m.row_text_y,
                                "OBJECT LIST",
                                p.text_primary,
                                m.body_scale);
        hooks->draw_bitmap_text(renderer,
                                rect,
                                object_list_rect.x + 6,
                                object_list_rect.y + m.line_h + m.row_text_y,
                                "LIVE RETAINED OBJECTS",
                                p.text_muted,
                                m.body_scale);
        if (ctx->object_store.object_count == 0u) {
            hooks->draw_bitmap_text(renderer,
                                    rect,
                                    object_list_rect.x + 6,
                                    object_list_rect.y + m.line_h + m.row_h + m.section_gap + m.row_text_y,
                                    "NO RETAINED OBJECTS",
                                    p.text_muted,
                                    m.body_scale);
        } else {
            uint32_t display_i;
            for (display_i = 0u; display_i < ctx->object_store.object_count; ++display_i) {
                uint32_t model_i = (ctx->object_store.object_count - 1u) - display_i;
                const DrawingProgramObjectRecord *object = &ctx->object_store.objects[model_i];
                SDL_Rect row = left_panel_objects_row_rect(object_list_rect, m, display_i);
                char line[64];
                int selected;
                SDL_Color row_color;
                SDL_Color label_color;
                if (row.y + row.h > object_list_rect.y + object_list_rect.h) {
                    break;
                }
                selected = drawing_program_object_selection_contains(&ctx->object_selection, object->object_id);
                row_color = selected ? p.button_fill_active : p.button_fill;
                label_color = selected ? p.text_primary : sdl_color_ensure_contrast(p.text_muted, row_color);
                SDL_SetRenderDrawColor(renderer, row_color.r, row_color.g, row_color.b, row_color.a);
                (void)SDL_RenderFillRect(renderer, &row);
                SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
                (void)SDL_RenderDrawRect(renderer, &row);
                (void)snprintf(line,
                               sizeof(line),
                               "ID %u %s",
                               (unsigned)object->object_id,
                               visual_object_type_name(object->type));
                hooks->draw_bitmap_text(renderer,
                                        rect,
                                        row.x + 6,
                                        row.y + m.row_text_y,
                                        line,
                                        label_color,
                                        m.body_scale);
            }
        }

        SDL_SetRenderDrawColor(renderer,
                               p.pane_background_alt.r,
                               p.pane_background_alt.g,
                               p.pane_background_alt.b,
                               p.pane_background_alt.a);
        (void)SDL_RenderFillRect(renderer, &object_inspector_rect);
        SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
        (void)SDL_RenderDrawRect(renderer, &object_inspector_rect);
        hooks->draw_bitmap_text(renderer,
                                rect,
                                object_inspector_rect.x + 6,
                                object_inspector_rect.y + m.row_text_y,
                                "SELECTED OBJECT",
                                p.text_primary,
                                m.body_scale);
        selected_object =
            drawing_program_object_store_get_by_id(&ctx->object_store, ctx->object_selection.active_object_id);
        if (!selected_object) {
            hooks->draw_bitmap_text(renderer,
                                    rect,
                                    object_inspector_rect.x + 6,
                                    object_inspector_rect.y + m.line_h + m.row_text_y,
                                    "NO OBJECT SELECTED",
                                    p.text_muted,
                                    m.body_scale);
            hooks->draw_bitmap_text(renderer,
                                    rect,
                                    object_inspector_rect.x + 6,
                                    object_inspector_rect.y + m.line_h + m.row_h + m.section_gap + m.row_text_y,
                                    "SELECT FROM CANVAS OR LIST",
                                    p.text_muted,
                                    m.body_scale);
        } else {
            char line[96];
            char stroke_label[24];
            char fill_label[24];
            int info_y = object_inspector_rect.y + m.line_h + m.row_text_y;
            uint32_t selected_point_object_id = 0u;
            uint16_t selected_point_index = 0u;
            int is_shape_object =
                (selected_object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT ||
                 selected_object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_ELLIPSE);
            int has_selected_point =
                drawing_program_object_selection_get_path_point(&ctx->object_selection,
                                                                &selected_point_object_id,
                                                                &selected_point_index) &&
                selected_point_object_id == selected_object->object_id;
            uint32_t action_count =
                (selected_object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) ? 5u :
                (is_shape_object ? 6u : 4u);
            int stroke_color_target_active =
                ui && ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_STROKE &&
                ui->object_color_target_object_id == selected_object->object_id;
            int fill_color_target_active =
                ui && ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_FILL &&
                ui->object_color_target_object_id == selected_object->object_id;
            visual_object_sample_label(selected_object->stroke_color_value, stroke_label, sizeof(stroke_label));
            visual_object_sample_label(selected_object->fill_color_value, fill_label, sizeof(fill_label));

            (void)snprintf(line,
                           sizeof(line),
                           "ID %u  TYPE %s",
                           (unsigned)selected_object->object_id,
                           visual_object_type_name(selected_object->type));
            hooks->draw_bitmap_text(renderer, rect, object_inspector_rect.x + 6, info_y, line, p.text_primary, m.body_scale);
            info_y += m.line_h;

            (void)snprintf(line,
                           sizeof(line),
                           "LAYER %u %s",
                           (unsigned)selected_object->layer_id,
                           visual_layer_name_for_id(&ctx->document, selected_object->layer_id));
            hooks->draw_bitmap_text(renderer, rect, object_inspector_rect.x + 6, info_y, line, p.text_muted, m.body_scale);
            info_y += m.line_h;

            (void)snprintf(line,
                           sizeof(line),
                           "ORIGIN %d,%d  SIZE %ux%u",
                           (int)selected_object->origin_x,
                           (int)selected_object->origin_y,
                           (unsigned)selected_object->width,
                           (unsigned)selected_object->height);
            hooks->draw_bitmap_text(renderer, rect, object_inspector_rect.x + 6, info_y, line, p.text_muted, m.body_scale);
            info_y += m.line_h;

            (void)snprintf(line,
                           sizeof(line),
                           "STYLE %s  STROKE W%u %s",
                           visual_object_style_name(selected_object->style_mode),
                           (unsigned)selected_object->stroke_width,
                           stroke_label);
            hooks->draw_bitmap_text(renderer, rect, object_inspector_rect.x + 6, info_y, line, p.text_muted, m.body_scale);
            info_y += m.line_h;

            (void)snprintf(line,
                           sizeof(line),
                           "FILL %s  VISIBLE %s  LOCKED %s",
                           fill_label,
                           selected_object->visible ? "ON" : "OFF",
                           selected_object->locked ? "ON" : "OFF");
            hooks->draw_bitmap_text(renderer, rect, object_inspector_rect.x + 6, info_y, line, p.text_muted, m.body_scale);
            info_y += m.line_h;

            if (selected_object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
                (void)snprintf(line,
                               sizeof(line),
                               "PATH POINTS %u  %s",
                               (unsigned)selected_object->path_point_count,
                               selected_object->path_closed ? "CLOSED" : "OPEN");
                hooks->draw_bitmap_text(renderer,
                                        rect,
                                        object_inspector_rect.x + 6,
                                        info_y,
                                        line,
                                        p.text_muted,
                                        m.body_scale);
                info_y += m.line_h;
                if (has_selected_point) {
                    (void)snprintf(line,
                                   sizeof(line),
                                   "POINT %u/%u SELECTED",
                                   (unsigned)selected_point_index + 1u,
                                   (unsigned)selected_object->path_point_count);
                    hooks->draw_bitmap_text(renderer,
                                            rect,
                                            object_inspector_rect.x + 6,
                                            info_y,
                                            line,
                                            p.text_muted,
                                            m.body_scale);
                    info_y += m.line_h;
                    (void)snprintf(line,
                                   sizeof(line),
                                   "POINT MODE %s %s",
                                   selected_object->path_points[selected_point_index].bezier_enabled ? "BEZIER" : "LINEAR",
                                   (selected_object->path_points[selected_point_index].bezier_enabled &&
                                    selected_object->path_points[selected_point_index].handle_linked)
                                       ? "LINKED"
                                       : (selected_object->path_points[selected_point_index].bezier_enabled ? "UNLINKED"
                                                                                                           : ""));
                } else {
                    (void)snprintf(line, sizeof(line), "POINT NONE SELECTED");
                }
                hooks->draw_bitmap_text(renderer,
                                        rect,
                                        object_inspector_rect.x + 6,
                                        info_y,
                                        line,
                                        p.text_muted,
                                        m.body_scale);
                info_y += m.line_h;
            }
            {
                SDL_Rect stroke_color_row =
                    left_panel_objects_inspector_action_row_rect(object_inspector_rect, m, 0u, action_count);
                char stroke_color_label[32];
                (void)snprintf(stroke_color_label,
                               sizeof(stroke_color_label),
                               "SET STROKE COLOR");
                drawing_program_visual_panel_draw_tab_button(renderer,
                                                             rect,
                                                             stroke_color_row,
                                                             stroke_color_label,
                                                             p.button_fill,
                                                             p.button_fill_hover,
                                                             p.button_fill_active,
                                                             p.button_border,
                                                             p.text_primary,
                                                             m.body_scale,
                                                             stroke_color_target_active,
                                                             drawing_program_visual_panel_ui_hovered(ui, stroke_color_row, hooks),
                                                             hooks);
            }
            {
                SDL_Rect fill_color_row =
                    left_panel_objects_inspector_action_row_rect(object_inspector_rect, m, 1u, action_count);
                char fill_color_label[32];
                (void)snprintf(fill_color_label,
                               sizeof(fill_color_label),
                               "SET FILL COLOR");
                drawing_program_visual_panel_draw_tab_button(renderer,
                                                             rect,
                                                             fill_color_row,
                                                             fill_color_label,
                                                             p.button_fill,
                                                             p.button_fill_hover,
                                                             p.button_fill_active,
                                                             p.button_border,
                                                             p.text_primary,
                                                             m.body_scale,
                                                             fill_color_target_active,
                                                             drawing_program_visual_panel_ui_hovered(ui, fill_color_row, hooks),
                                                             hooks);
            }
            {
                SDL_Rect stroke_row =
                    left_panel_objects_inspector_action_row_rect(object_inspector_rect, m, 2u, action_count);
                SDL_Rect minus_rect = left_tool_option_minus_rect(stroke_row, m);
                SDL_Rect value_rect = left_tool_option_value_rect(stroke_row, m);
                SDL_Rect plus_rect = left_tool_option_plus_rect(stroke_row, m);
                char value_text[24];
                hooks->draw_bitmap_text(renderer,
                                        rect,
                                        stroke_row.x + 6,
                                        stroke_row.y + m.row_text_y,
                                        "STROKE WIDTH",
                                        p.text_primary,
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
                SDL_SetRenderDrawColor(renderer, p.button_fill.r, p.button_fill.g, p.button_fill.b, p.button_fill.a);
                (void)SDL_RenderFillRect(renderer, &value_rect);
                SDL_SetRenderDrawColor(renderer,
                                       p.button_border.r,
                                       p.button_border.g,
                                       p.button_border.b,
                                       p.button_border.a);
                (void)SDL_RenderDrawRect(renderer, &value_rect);
                (void)snprintf(value_text, sizeof(value_text), "W%u", (unsigned)selected_object->stroke_width);
                hooks->draw_bitmap_text(renderer,
                                        rect,
                                        value_rect.x + 6,
                                        value_rect.y + m.row_text_y,
                                        value_text,
                                        p.text_primary,
                                        m.body_scale);
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
            {
                SDL_Rect fill_row =
                    left_panel_objects_inspector_action_row_rect(object_inspector_rect,
                                                                 m,
                                                                 is_shape_object ? 5u : 3u,
                                                                 action_count);
                const char *fill_label =
                    visual_object_style_fill_enabled(selected_object->style_mode) ? "FILL ON" : "FILL OFF";
                drawing_program_visual_panel_draw_tab_button(renderer,
                                                             rect,
                                                             fill_row,
                                                             fill_label,
                                                             p.button_fill,
                                                             p.button_fill_hover,
                                                             p.button_fill_active,
                                                             p.button_border,
                                                             p.text_primary,
                                                             m.body_scale,
                                                             visual_object_style_fill_enabled(selected_object->style_mode),
                                                             drawing_program_visual_panel_ui_hovered(ui, fill_row, hooks),
                                                             hooks);
            }
            if (is_shape_object) {
                SDL_Rect width_row =
                    left_panel_objects_inspector_action_row_rect(object_inspector_rect, m, 3u, action_count);
                SDL_Rect width_minus_rect = left_tool_option_minus_rect(width_row, m);
                SDL_Rect width_value_rect = left_tool_option_value_rect(width_row, m);
                SDL_Rect width_plus_rect = left_tool_option_plus_rect(width_row, m);
                SDL_Rect height_row =
                    left_panel_objects_inspector_action_row_rect(object_inspector_rect, m, 4u, action_count);
                SDL_Rect height_minus_rect = left_tool_option_minus_rect(height_row, m);
                SDL_Rect height_value_rect = left_tool_option_value_rect(height_row, m);
                SDL_Rect height_plus_rect = left_tool_option_plus_rect(height_row, m);
                char value_text[24];
                hooks->draw_bitmap_text(renderer,
                                        rect,
                                        width_row.x + 6,
                                        width_row.y + m.row_text_y,
                                        "WIDTH",
                                        p.text_primary,
                                        m.body_scale);
                drawing_program_visual_panel_draw_tab_button(renderer,
                                                             rect,
                                                             width_minus_rect,
                                                             "-",
                                                             p.button_fill,
                                                             p.button_fill_hover,
                                                             p.button_fill_active,
                                                             p.button_border,
                                                             p.text_primary,
                                                             m.body_scale,
                                                             0,
                                                             drawing_program_visual_panel_ui_hovered(ui, width_minus_rect, hooks),
                                                             hooks);
                SDL_SetRenderDrawColor(renderer, p.button_fill.r, p.button_fill.g, p.button_fill.b, p.button_fill.a);
                (void)SDL_RenderFillRect(renderer, &width_value_rect);
                SDL_SetRenderDrawColor(renderer,
                                       p.button_border.r,
                                       p.button_border.g,
                                       p.button_border.b,
                                       p.button_border.a);
                (void)SDL_RenderDrawRect(renderer, &width_value_rect);
                (void)snprintf(value_text, sizeof(value_text), "%u", (unsigned)selected_object->width);
                hooks->draw_bitmap_text(renderer,
                                        rect,
                                        width_value_rect.x + 6,
                                        width_value_rect.y + m.row_text_y,
                                        value_text,
                                        p.text_primary,
                                        m.body_scale);
                drawing_program_visual_panel_draw_tab_button(renderer,
                                                             rect,
                                                             width_plus_rect,
                                                             "+",
                                                             p.button_fill,
                                                             p.button_fill_hover,
                                                             p.button_fill_active,
                                                             p.button_border,
                                                             p.text_primary,
                                                             m.body_scale,
                                                             0,
                                                             drawing_program_visual_panel_ui_hovered(ui, width_plus_rect, hooks),
                                                             hooks);

                hooks->draw_bitmap_text(renderer,
                                        rect,
                                        height_row.x + 6,
                                        height_row.y + m.row_text_y,
                                        "HEIGHT",
                                        p.text_primary,
                                        m.body_scale);
                drawing_program_visual_panel_draw_tab_button(renderer,
                                                             rect,
                                                             height_minus_rect,
                                                             "-",
                                                             p.button_fill,
                                                             p.button_fill_hover,
                                                             p.button_fill_active,
                                                             p.button_border,
                                                             p.text_primary,
                                                             m.body_scale,
                                                             0,
                                                             drawing_program_visual_panel_ui_hovered(ui, height_minus_rect, hooks),
                                                             hooks);
                SDL_SetRenderDrawColor(renderer, p.button_fill.r, p.button_fill.g, p.button_fill.b, p.button_fill.a);
                (void)SDL_RenderFillRect(renderer, &height_value_rect);
                SDL_SetRenderDrawColor(renderer,
                                       p.button_border.r,
                                       p.button_border.g,
                                       p.button_border.b,
                                       p.button_border.a);
                (void)SDL_RenderDrawRect(renderer, &height_value_rect);
                (void)snprintf(value_text, sizeof(value_text), "%u", (unsigned)selected_object->height);
                hooks->draw_bitmap_text(renderer,
                                        rect,
                                        height_value_rect.x + 6,
                                        height_value_rect.y + m.row_text_y,
                                        value_text,
                                        p.text_primary,
                                        m.body_scale);
                drawing_program_visual_panel_draw_tab_button(renderer,
                                                             rect,
                                                             height_plus_rect,
                                                             "+",
                                                             p.button_fill,
                                                             p.button_fill_hover,
                                                             p.button_fill_active,
                                                             p.button_border,
                                                             p.text_primary,
                                                             m.body_scale,
                                                             0,
                                                             drawing_program_visual_panel_ui_hovered(ui, height_plus_rect, hooks),
                                                             hooks);
            }
            if (selected_object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
                SDL_Rect path_row =
                    left_panel_objects_inspector_action_row_rect(object_inspector_rect, m, 4u, action_count);
                int can_close = (selected_object->path_closed || selected_object->path_point_count >= 3u) ? 1 : 0;
                const char *path_label = selected_object->path_closed ? "PATH CLOSED" :
                                         (can_close ? "PATH OPEN" : "PATH OPEN (3+)");
                drawing_program_visual_panel_draw_tab_button(renderer,
                                                             rect,
                                                             path_row,
                                                             path_label,
                                                             p.button_fill,
                                                             p.button_fill_hover,
                                                             p.button_fill_active,
                                                             p.button_border,
                                                             can_close ? p.text_primary : p.text_muted,
                                                             m.body_scale,
                                                             selected_object->path_closed ? 1 : 0,
                                                             drawing_program_visual_panel_ui_hovered(ui, path_row, hooks),
                                                             hooks);
            }
        }
        return;
    }

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
