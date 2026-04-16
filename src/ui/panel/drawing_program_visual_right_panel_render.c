#include "drawing_program/drawing_program_visual_right_panel_render.h"

#include <stdio.h>

#include "drawing_program/drawing_program_object_store.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_panel_render_common.h"
#include "drawing_program/drawing_program_visual_theme.h"

enum {
    VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE = 0,
    VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE = 1
};

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
    right_slot = hooks->clamp_right_slot(ctx->ui.right_panel_slot);
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
    drawing_program_visual_panel_draw_tab_button(renderer,
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
                                                 drawing_program_visual_panel_ui_hovered(ui, tab_canvas, hooks),
                                                 hooks);
    drawing_program_visual_panel_draw_tab_button(renderer,
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
                                                 drawing_program_visual_panel_ui_hovered(ui, tab_layer, hooks),
                                                 hooks);
    y += m.tab_h + m.section_gap;

    if (right_slot == VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE) {
        const char *pointer_state = "POINTER: PANEL";
        const char *interaction_hint = "LMB: DRAW  RMB: PAN VIEW  WHEEL: ZOOM";
        uint32_t history_cursor_units = 0u;
        uint32_t history_count_units = 0u;
        uint8_t active_color_index = hooks->color_index_clamp(ctx->ui.active_color_index);
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
                interaction_hint = "LMB: MOVE OBJECTS  ALT+LMB POINT: MOVE VERTEX  ARROWS:NUDGE  SHIFT+ARROW:x10";
            } else {
                interaction_hint = selection && selection->has_payload
                                       ? "LMB: MOVE SEL  ARROWS:NUDGE  SHIFT+ARROW:x10"
                                       : "MOVE TOOL: NO ACTIVE SELECTION";
            }
        } else if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_FILL) {
            interaction_hint = "LMB: FILL REGION  TOLERANCE CONTROLS MATCH RANGE";
        } else if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_PICKER) {
            interaction_hint = "LMB: PICK COLOR";
        } else if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_PATH) {
            interaction_hint = "LMB: ADD POINT  ENTER: COMMIT CLOSED PATH  ESC: CANCEL  BACKSPACE: UNDO POINT";
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
                           (unsigned)hooks->clamp_setting_u8(ctx->ui.tool_brush_opacity, 1u, 100u),
                           (unsigned)hooks->clamp_setting_u8(ctx->ui.tool_brush_hardness, 1u, 100u));
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
                               (unsigned)hooks->clamp_setting_u8(ctx->ui.tool_shape_stroke_width, 1u, 16u));
            } else {
                (void)snprintf(line,
                               sizeof(line),
                               "TOOL %s  W%u MODE %s TARGET %s",
                               hooks->tool_name(ctx->editor.active_tool),
                               (unsigned)hooks->clamp_setting_u8(ctx->ui.tool_shape_stroke_width, 1u, 16u),
                               hooks->shape_mode_name(hooks->tool_shape_mode(ctx)),
                               drawing_program_visual_shape_target_mode_name(
                                   hooks->clamp_setting_u8(ctx->ui.tool_shape_target_mode, 0u, 1u)));
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
                           drawing_program_visual_select_mode_name(hooks->clamp_setting_u8(ctx->ui.tool_select_mode, 0u, 2u)));
        } else if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_PATH) {
            (void)snprintf(line,
                           sizeof(line),
                           "TOOL %s  W%u MODE %s",
                           hooks->tool_name(ctx->editor.active_tool),
                           (unsigned)hooks->clamp_setting_u8(ctx->ui.tool_shape_stroke_width, 1u, 16u),
                           hooks->shape_mode_name(hooks->tool_shape_mode(ctx)));
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

        drawing_program_visual_panel_draw_tab_button(renderer, rect, reset_view_button, "RESET VIEW",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, reset_view_button, hooks), hooks);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, clear_canvas_button, "CLEAR CANVAS",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, clear_canvas_button, hooks), hooks);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, delete_selection_button, "DELETE SELECTION",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     delete_selection_enabled ? p.text_primary : p.text_muted, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, delete_selection_button, hooks), hooks);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, clear_history_button, "CLEAR HISTORY",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, clear_history_button, hooks), hooks);
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
            if (drawing_program_visual_panel_ui_hovered(ui, row, hooks)) {
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
        drawing_program_visual_panel_draw_tab_button(renderer,
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
                                                     drawing_program_visual_panel_ui_hovered(ui, opacity_row_rect, hooks),
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
        drawing_program_visual_panel_draw_tab_button(renderer, rect, button_rect, "ADD LAYER",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_DUPLICATE);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, button_rect, "DUPLICATE SELECTED",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_RENAME);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, button_rect, "RENAME SELECTED",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_DELETE);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, button_rect, "DELETE SELECTED",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ACTIVE_PREV);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, button_rect, "ACTIVE PREV",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ACTIVE_NEXT);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, button_rect, "ACTIVE NEXT",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_MOVE_UP);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, button_rect, "MOVE UP",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_MOVE_DOWN);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, button_rect, "MOVE DOWN",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_TOGGLE_VISIBLE);
        drawing_program_visual_panel_draw_tab_button(renderer,
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
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks),
                                                     hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_TOGGLE_LOCK);
        drawing_program_visual_panel_draw_tab_button(renderer,
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
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks),
                                                     hooks);
    }
}
