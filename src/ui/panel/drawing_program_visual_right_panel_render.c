#include "drawing_program/drawing_program_visual_right_panel_render.h"

#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_icns_export.h"
#include "drawing_program/drawing_program_object_store.h"
#include "drawing_program/drawing_program_iconset_export.h"
#include "drawing_program/drawing_program_png_export.h"
#include "drawing_program/drawing_program_project_state.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_panel_render_common.h"
#include "drawing_program/drawing_program_visual_right_panel_color_render.h"
#include "drawing_program/drawing_program_visual_theme.h"

enum {
    VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE = 0,
    VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE = 1,
    VISUAL_RIGHT_PANEL_SLOT_COLOR_VALUE = 2,
    VISUAL_RIGHT_PANEL_SLOT_FILE_VALUE = 3,
    VISUAL_RIGHT_FILE_ACTION_NEW_PROJECT = 0,
    VISUAL_RIGHT_FILE_ACTION_OPEN_PROJECT = 1,
    VISUAL_RIGHT_FILE_ACTION_SAVE_PROJECT = 2,
    VISUAL_RIGHT_FILE_ACTION_SAVE_AS = 3,
    VISUAL_RIGHT_FILE_ACTION_LOAD_PROJECT = 4,
    VISUAL_RIGHT_FILE_ACTION_SAVE_SESSION = 5,
    VISUAL_RIGHT_FILE_ACTION_RELOAD_SESSION = 6,
    VISUAL_RIGHT_FILE_ACTION_COUNT = 7,
    VISUAL_RIGHT_FILE_ROUTE_ACTION_PICK_INPUT = 0,
    VISUAL_RIGHT_FILE_ROUTE_ACTION_PICK_OUTPUT = 1,
    VISUAL_RIGHT_FILE_ROUTE_ACTION_EXPORT_PNG = 2,
    VISUAL_RIGHT_FILE_ROUTE_ACTION_EXPORT_ICONSET = 3,
    VISUAL_RIGHT_FILE_ROUTE_ACTION_EXPORT_ICNS = 4,
    VISUAL_RIGHT_FILE_ROUTE_ACTION_COUNT = 5
};

static void visual_right_panel_format_path_line(char *out_line,
                                                size_t out_cap,
                                                const char *label,
                                                const char *path) {
    size_t path_len = 0u;
    const char *display_path = path;
    if (!out_line || out_cap == 0u) {
        return;
    }
    if (!label) {
        label = "PATH";
    }
    if (!path || path[0] == '\0') {
        (void)snprintf(out_line, out_cap, "%s (unset)", label);
        return;
    }
    path_len = strlen(path);
    if (path_len > 28u) {
        display_path = path + (path_len - 28u);
        while (*display_path != '\0' && *display_path != '/') {
            ++display_path;
        }
        if (*display_path == '/') {
            ++display_path;
        } else {
            display_path = path + (path_len - 28u);
        }
        (void)snprintf(out_line, out_cap, "%s .../%s", label, display_path);
        return;
    }
    (void)snprintf(out_line, out_cap, "%s %s", label, path);
}

static void visual_right_panel_format_recent_project_line(char *out_line,
                                                          size_t out_cap,
                                                          uint32_t row_index,
                                                          const char *path,
                                                          uint8_t existing) {
    const char *name = path;
    size_t name_len = 0u;
    if (!out_line || out_cap == 0u) {
        return;
    }
    if (!path || path[0] == '\0') {
        (void)snprintf(out_line, out_cap, "N%u NEW TARGET", (unsigned)(row_index + 1u));
        return;
    }
    name = strrchr(path, '/');
    name = name ? (name + 1) : path;
    name_len = strlen(name);
    if (name_len > 22u) {
        name += (name_len - 22u);
    }
    (void)snprintf(out_line,
                   out_cap,
                   "%c%u %s %s",
                   existing ? 'S' : 'N',
                   (unsigned)(row_index + 1u),
                   existing ? "LOAD" : "SAVE",
                   name);
}

static void visual_right_panel_draw_scrollbar(SDL_Renderer *renderer,
                                              SDL_Rect viewport,
                                              int offset_y,
                                              int max_offset_y,
                                              SDL_Color track_color,
                                              SDL_Color thumb_color) {
    SDL_Rect track;
    SDL_Rect thumb;
    float ratio;
    if (!renderer || viewport.h <= 0 || max_offset_y <= 0) {
        return;
    }
    track.x = viewport.x + viewport.w - 8;
    track.y = viewport.y;
    track.w = 6;
    track.h = viewport.h;
    if (track.w < 1 || track.h < 1) {
        return;
    }
    if (offset_y < 0) {
        offset_y = 0;
    }
    if (offset_y > max_offset_y) {
        offset_y = max_offset_y;
    }
    ratio = (float)viewport.h / (float)(viewport.h + max_offset_y);
    if (ratio < 0.1f) {
        ratio = 0.1f;
    }
    thumb = track;
    thumb.h = (int)((float)track.h * ratio);
    if (thumb.h < 6) {
        thumb.h = 6;
    }
    if (thumb.h > track.h) {
        thumb.h = track.h;
    }
    thumb.y = track.y + (int)((float)(track.h - thumb.h) * ((float)offset_y / (float)max_offset_y));
    SDL_SetRenderDrawColor(renderer, track_color.r, track_color.g, track_color.b, track_color.a);
    (void)SDL_RenderFillRect(renderer, &track);
    SDL_SetRenderDrawColor(renderer, thumb_color.r, thumb_color.g, thumb_color.b, thumb_color.a);
    (void)SDL_RenderFillRect(renderer, &thumb);
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
    SDL_Rect tab_color;
    SDL_Rect tab_file;
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
        !hooks->color_index_clamp || !hooks->color_rgb_from_index) {
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
    tab_canvas = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE, 4u);
    tab_layer = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE, 4u);
    tab_color = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_COLOR_VALUE, 4u);
    tab_file = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_FILE_VALUE, 4u);
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
    drawing_program_visual_panel_draw_tab_button(renderer,
                                                 rect,
                                                 tab_color,
                                                 "COLOR",
                                                 p.button_fill,
                                                 p.button_fill_hover,
                                                 p.button_fill_active,
                                                 p.button_border,
                                                 (right_slot == VISUAL_RIGHT_PANEL_SLOT_COLOR_VALUE) ? p.text_primary : p.text_muted,
                                                 m.body_scale,
                                                 right_slot == VISUAL_RIGHT_PANEL_SLOT_COLOR_VALUE,
                                                 drawing_program_visual_panel_ui_hovered(ui, tab_color, hooks),
                                                 hooks);
    drawing_program_visual_panel_draw_tab_button(renderer,
                                                 rect,
                                                 tab_file,
                                                 "FILE",
                                                 p.button_fill,
                                                 p.button_fill_hover,
                                                 p.button_fill_active,
                                                 p.button_border,
                                                 (right_slot == VISUAL_RIGHT_PANEL_SLOT_FILE_VALUE) ? p.text_primary
                                                                                                   : p.text_muted,
                                                 m.body_scale,
                                                 right_slot == VISUAL_RIGHT_PANEL_SLOT_FILE_VALUE,
                                                 drawing_program_visual_panel_ui_hovered(ui, tab_file, hooks),
                                                 hooks);
    y += m.tab_h + m.section_gap;

    if (right_slot == VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE) {
        const char *pointer_state = "POINTER: PANEL";
        const char *interaction_hint = "LMB: DRAW  RMB: PAN VIEW  WHEEL: ZOOM";
        uint32_t history_cursor_units = 0u;
        uint32_t history_count_units = 0u;
        SDL_Rect reset_view_button;
        SDL_Rect clear_canvas_button;
        SDL_Rect clear_objects_button;
        SDL_Rect delete_selection_button;
        SDL_Rect clear_history_button;
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
            interaction_hint =
                "LMB: ADD POINT  ENTER: CLOSED PATH  SHIFT+ENTER: OPEN PATH  ESC: CANCEL  BACKSPACE: UNDO POINT";
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
        clear_objects_button = right_canvas_clear_objects_button_rect(rect, m);
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
        drawing_program_visual_panel_draw_tab_button(renderer, rect, clear_objects_button, "CLEAR OBJECTS",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, clear_objects_button, hooks), hooks);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, delete_selection_button, "DELETE SELECTION",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     delete_selection_enabled ? p.text_primary : p.text_muted, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, delete_selection_button, hooks), hooks);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, clear_history_button, "CLEAR HISTORY",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, clear_history_button, hooks), hooks);
    } else if (right_slot == VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE) {
        uint32_t display_i;
        SDL_Rect button_rect;
        SDL_Rect opacity_row_rect;
        int opacity_label_y;
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
        SDL_SetRenderDrawColor(renderer, p.button_fill.r, p.button_fill.g, p.button_fill.b, p.button_fill.a);
        (void)SDL_RenderFillRect(renderer, &opacity_row_rect);
        SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
        (void)SDL_RenderDrawRect(renderer, &opacity_row_rect);
        {
            SDL_Rect opacity_track = right_layer_opacity_track_rect(opacity_row_rect, m);
            opacity_label_y = opacity_row_rect.y + ((m.line_h - m.body_glyph_h) / 2);
            if (opacity_label_y < opacity_row_rect.y + 1) {
                opacity_label_y = opacity_row_rect.y + 1;
            }
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
            hooks->draw_bitmap_text(renderer, rect, opacity_row_rect.x + 6, opacity_label_y, line, p.text_primary, m.body_scale);
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
    } else if (right_slot == VISUAL_RIGHT_PANEL_SLOT_FILE_VALUE) {
        uint32_t state_line_count = 10u;
        uint32_t target_slot_count = right_file_target_queue_slot_count(ctx);
        SDL_Rect new_project_button = right_file_action_button_rect(rect, m, VISUAL_RIGHT_FILE_ACTION_NEW_PROJECT,
                                                                    VISUAL_RIGHT_FILE_ACTION_COUNT);
        SDL_Rect open_project_button = right_file_action_button_rect(rect, m, VISUAL_RIGHT_FILE_ACTION_OPEN_PROJECT,
                                                                     VISUAL_RIGHT_FILE_ACTION_COUNT);
        SDL_Rect save_project_button = right_file_action_button_rect(rect, m, VISUAL_RIGHT_FILE_ACTION_SAVE_PROJECT,
                                                                     VISUAL_RIGHT_FILE_ACTION_COUNT);
        SDL_Rect save_as_button = right_file_action_button_rect(rect, m, VISUAL_RIGHT_FILE_ACTION_SAVE_AS,
                                                                VISUAL_RIGHT_FILE_ACTION_COUNT);
        SDL_Rect load_project_button = right_file_action_button_rect(rect, m, VISUAL_RIGHT_FILE_ACTION_LOAD_PROJECT,
                                                                     VISUAL_RIGHT_FILE_ACTION_COUNT);
        SDL_Rect save_session_button = right_file_save_session_button_rect(rect, m);
        SDL_Rect reload_session_button = right_file_reload_session_button_rect(rect, m);
        SDL_Rect pick_input_root_button = right_file_route_action_button_rect(rect,
                                                                              m,
                                                                              state_line_count,
                                                                              VISUAL_RIGHT_FILE_ROUTE_ACTION_PICK_INPUT,
                                                                              VISUAL_RIGHT_FILE_ROUTE_ACTION_COUNT);
        SDL_Rect pick_output_root_button = right_file_route_action_button_rect(rect,
                                                                               m,
                                                                               state_line_count,
                                                                               VISUAL_RIGHT_FILE_ROUTE_ACTION_PICK_OUTPUT,
                                                                               VISUAL_RIGHT_FILE_ROUTE_ACTION_COUNT);
        SDL_Rect export_png_button = right_file_route_action_button_rect(rect,
                                                                         m,
                                                                         state_line_count,
                                                                         VISUAL_RIGHT_FILE_ROUTE_ACTION_EXPORT_PNG,
                                                                         VISUAL_RIGHT_FILE_ROUTE_ACTION_COUNT);
        SDL_Rect export_iconset_button = right_file_route_action_button_rect(rect,
                                                                             m,
                                                                             state_line_count,
                                                                             VISUAL_RIGHT_FILE_ROUTE_ACTION_EXPORT_ICONSET,
                                                                             VISUAL_RIGHT_FILE_ROUTE_ACTION_COUNT);
        SDL_Rect export_icns_button = right_file_route_action_button_rect(rect,
                                                                          m,
                                                                          state_line_count,
                                                                          VISUAL_RIGHT_FILE_ROUTE_ACTION_EXPORT_ICNS,
                                                                          VISUAL_RIGHT_FILE_ROUTE_ACTION_COUNT);
        SDL_Rect target_queue_rect = right_file_target_queue_rect(rect,
                                                                  m,
                                                                  state_line_count,
                                                                  VISUAL_RIGHT_FILE_ROUTE_ACTION_COUNT);
        int target_scroll_y =
            ui ? right_file_target_queue_clamp_scroll(target_queue_rect,
                                                      m,
                                                      target_slot_count,
                                                      ui->right_file_target_queue_scroll_y)
               : 0;
        int target_scroll_max = right_file_target_queue_scroll_max(target_queue_rect, m, target_slot_count);
        uint8_t project_dirty = drawing_program_project_state_current_is_dirty(ctx);
        char export_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        char iconset_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        char icns_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        if (drawing_program_png_export_default_path(ctx, export_path, sizeof(export_path)).code != CORE_OK) {
            export_path[0] = '\0';
        }
        if (drawing_program_iconset_export_default_path(ctx, iconset_path, sizeof(iconset_path)).code != CORE_OK) {
            iconset_path[0] = '\0';
        }
        if (drawing_program_icns_export_default_path(ctx, icns_path, sizeof(icns_path)).code != CORE_OK) {
            icns_path[0] = '\0';
        }
        y = right_file_content_start_y(rect, m);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "PROJECT ACTIONS", p.text_primary, m.body_scale);
        hooks->draw_bitmap_text(renderer,
                                rect,
                                rect.x + m.pad_x,
                                right_file_target_queue_label_y(rect, m, VISUAL_RIGHT_FILE_ACTION_COUNT),
                                "TARGET QUEUE  S=LOAD  N=SAVE",
                                p.text_primary,
                                m.body_scale);
        SDL_SetRenderDrawColor(renderer,
                               p.pane_background_alt.r,
                               p.pane_background_alt.g,
                               p.pane_background_alt.b,
                               p.pane_background_alt.a);
        (void)SDL_RenderFillRect(renderer, &target_queue_rect);
        SDL_SetRenderDrawColor(renderer,
                               p.button_border.r,
                               p.button_border.g,
                               p.button_border.b,
                               p.button_border.a);
        (void)SDL_RenderDrawRect(renderer, &target_queue_rect);
        for (i = 0u; i < target_slot_count; ++i) {
            SDL_Rect project_row = right_file_target_queue_row_rect(target_queue_rect, m, i, target_scroll_y);
            SDL_Rect clipped_row;
            char slot_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
            uint8_t existing = 0u;
            int selected = 0;
            int hovered = 0;
            SDL_Color fill = p.button_fill;
            if (!SDL_IntersectRect(&project_row, &target_queue_rect, &clipped_row)) {
                continue;
            }
            if (drawing_program_project_state_slot_path(ctx, i, slot_path, sizeof(slot_path), &existing).code != CORE_OK) {
                slot_path[0] = '\0';
                existing = 0u;
            }
            selected = (slot_path[0] != '\0' &&
                        ctx->session.project_path &&
                        strcmp(slot_path, ctx->session.project_path) == 0)
                           ? 1
                           : 0;
            hovered = drawing_program_visual_panel_ui_hovered(ui, project_row, hooks);
            fill = selected ? p.button_fill_active : (hovered ? p.button_fill_hover : p.button_fill);
            visual_right_panel_format_recent_project_line(line, sizeof(line), i, slot_path, existing);
            (void)SDL_RenderSetClipRect(renderer, &target_queue_rect);
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            (void)SDL_RenderFillRect(renderer, &project_row);
            SDL_SetRenderDrawColor(renderer,
                                   selected ? p.accent_primary.r : p.button_border.r,
                                   selected ? p.accent_primary.g : p.button_border.g,
                                   selected ? p.accent_primary.b : p.button_border.b,
                                   selected ? p.accent_primary.a : p.button_border.a);
            (void)SDL_RenderDrawRect(renderer, &project_row);
            hooks->draw_bitmap_text(renderer,
                                    target_queue_rect,
                                    project_row.x + 6,
                                    project_row.y + m.row_text_y,
                                    line,
                                    selected ? p.text_primary : p.text_muted,
                                    m.body_scale);
        }
        (void)SDL_RenderSetClipRect(renderer, 0);
        visual_right_panel_draw_scrollbar(renderer,
                                          target_queue_rect,
                                          target_scroll_y,
                                          target_scroll_max,
                                          p.button_fill,
                                          p.button_border);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     new_project_button,
                                                     "NEW BLANK",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     0,
                                                     drawing_program_visual_panel_ui_hovered(ui, new_project_button, hooks),
                                                     hooks);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     open_project_button,
                                                     "OPEN PROJECT",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     0,
                                                     drawing_program_visual_panel_ui_hovered(ui, open_project_button, hooks),
                                                     hooks);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     save_project_button,
                                                     "SAVE TARGET",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     0,
                                                     drawing_program_visual_panel_ui_hovered(ui, save_project_button, hooks),
                                                     hooks);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     save_as_button,
                                                     "SAVE AS",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     0,
                                                     drawing_program_visual_panel_ui_hovered(ui, save_as_button, hooks),
                                                     hooks);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     load_project_button,
                                                     "LOAD TARGET",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     0,
                                                     drawing_program_visual_panel_ui_hovered(ui, load_project_button, hooks),
                                                     hooks);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     save_session_button,
                                                     "SAVE SESSION",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     0,
                                                     drawing_program_visual_panel_ui_hovered(ui, save_session_button, hooks),
                                                     hooks);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     reload_session_button,
                                                     "RELOAD SESSION",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     0,
                                                     drawing_program_visual_panel_ui_hovered(ui, reload_session_button, hooks),
                                                     hooks);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     pick_input_root_button,
                                                     "PICK INPUT ROOT",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     0,
                                                     drawing_program_visual_panel_ui_hovered(ui, pick_input_root_button, hooks),
                                                     hooks);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     pick_output_root_button,
                                                     "PICK OUTPUT ROOT",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     0,
                                                     drawing_program_visual_panel_ui_hovered(ui, pick_output_root_button, hooks),
                                                     hooks);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     export_png_button,
                                                     "EXPORT PNG",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     0,
                                                     drawing_program_visual_panel_ui_hovered(ui, export_png_button, hooks),
                                                     hooks);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     export_iconset_button,
                                                     "EXPORT ICONSET",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     0,
                                                     drawing_program_visual_panel_ui_hovered(ui, export_iconset_button, hooks),
                                                     hooks);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     export_icns_button,
                                                     "EXPORT ICNS",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     0,
                                                     drawing_program_visual_panel_ui_hovered(ui, export_icns_button, hooks),
                                                     hooks);
        y = right_file_state_start_y(rect, m, state_line_count);
        visual_right_panel_format_path_line(line, sizeof(line), "TARGET", ctx->session.project_path);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line, sizeof(line), "STATUS %s", project_dirty ? "DIRTY" : "CLEAN");
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        visual_right_panel_format_path_line(line, sizeof(line), "AUTOSAVE", ctx->session.preset_path);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        visual_right_panel_format_path_line(line, sizeof(line), "INPUT", ctx->session.input_root_path);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        visual_right_panel_format_path_line(line, sizeof(line), "OUTPUT", ctx->session.output_root_path);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        visual_right_panel_format_path_line(line, sizeof(line), "EXPORT", export_path);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        visual_right_panel_format_path_line(line, sizeof(line), "ICONSET", iconset_path);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        visual_right_panel_format_path_line(line, sizeof(line), "ICNS", icns_path);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        if (ctx->session.export_json_path && ctx->session.export_json_path[0] != '\0') {
            visual_right_panel_format_path_line(line, sizeof(line), "DEBUG", ctx->session.export_json_path);
        } else {
            (void)snprintf(line, sizeof(line), "DEBUG export path not set");
        }
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line, sizeof(line), "ACTION %s", ctx->session.file_action_status_message);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
    } else {
        drawing_program_visual_render_right_panel_color_tab(renderer, rect, y, ctx, ui, m, p, hooks);
    }
}
