#include "drawing_program_panel_intent.h"
#include "drawing_program/drawing_program_visual_input_panel_clicks.h"

#include "drawing_program/drawing_program_history.h"
#include "drawing_program/drawing_program_object_store.h"
#include "drawing_program/drawing_program_selection.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_tool_options.h"

enum {
    VISUAL_LEFT_PANEL_SLOT_TOOLS_VALUE = 0,
    VISUAL_LEFT_PANEL_SLOT_OBJECTS_VALUE = 1
};

static int visual_object_style_fill_enabled(uint8_t style_mode) {
    return (style_mode == 1u || style_mode == 2u) ? 1 : 0;
}

static uint8_t visual_object_style_set_fill_enabled(uint8_t style_mode, int enabled) {
    if (enabled) {
        if (style_mode == 0u) {
            return 2u;
        }
        if (style_mode > 2u) {
            return 2u;
        }
        return style_mode;
    }
    return 0u;
}

void drawing_program_visual_input_handle_left_panel_click_payload_intent(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    DrawingProgramPanelIntent intent,
    DrawingProgramSelectionState *selection,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
    int x = intent.x, y = intent.y;
    VisualPaneLayoutMetrics m;
    uint32_t tool_count;
    uint32_t option_count;
    uint32_t i;
    if (!ctx || !ui || !hooks) {
        return;
    }
    m = make_pane_layout_metrics(ctx);
    {
        SDL_Rect tab_tools = left_panel_slot_tab_rect(rect, m, VISUAL_LEFT_PANEL_SLOT_TOOLS_VALUE, 2u);
        SDL_Rect tab_objects = left_panel_slot_tab_rect(rect, m, VISUAL_LEFT_PANEL_SLOT_OBJECTS_VALUE, 2u);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_TAB_TOOLS, 0, tab_tools, hooks)) {
            drawing_program_visual_set_left_panel_slot(ctx, (uint8_t)VISUAL_LEFT_PANEL_SLOT_TOOLS_VALUE);
            hooks->sync_panel_ui_from_app(ctx, ui);
            return;
        }
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_TAB_OBJECTS, 0, tab_objects, hooks)) {
            drawing_program_visual_set_left_panel_slot(ctx, (uint8_t)VISUAL_LEFT_PANEL_SLOT_OBJECTS_VALUE);
            hooks->sync_panel_ui_from_app(ctx, ui);
            return;
        }
    }
    if (hooks->clamp_left_slot(ctx->ui.left_panel_slot) == (uint8_t)VISUAL_LEFT_PANEL_SLOT_OBJECTS_VALUE) {
        SDL_Rect list_rect = left_panel_objects_list_rect(rect, m);
        SDL_Rect inspector_rect = left_panel_objects_inspector_rect(rect, m);
        const DrawingProgramObjectRecord *selected_object = 0;
        uint32_t display_i;
        for (display_i = 0u; display_i < ctx->object_store.object_count; ++display_i) {
            uint32_t model_i = (ctx->object_store.object_count - 1u) - display_i;
            SDL_Rect row = left_panel_objects_row_rect(list_rect, m, display_i);
            if (!intent.semantic && row.y + row.h > list_rect.y + list_rect.h) {
                break;
            }
            if (!intent.semantic && hooks->point_in_rect(inspector_rect, x, y)) {
                break;
            }
            if (drawing_program_panel_intent_matches(intent, DRAWING_UI_LEFT_OBJECT, ctx->object_store.objects[model_i].object_id, row, hooks)) {
                drawing_program_selection_reset(selection);
                drawing_program_object_selection_replace_single(
                    &ctx->object_selection, ctx->object_store.objects[model_i].object_id);
                ui->object_color_target_kind = VISUAL_OBJECT_COLOR_TARGET_NONE;
                ui->object_color_target_object_id = 0u;
                return;
            }
        }
        selected_object =
            drawing_program_object_store_get_by_id(&ctx->object_store, ctx->object_selection.active_object_id);
        if (selected_object) {
            int is_shape_object =
                (selected_object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT ||
                 selected_object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_ELLIPSE);
            uint32_t action_count =
                (selected_object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) ? 5u :
                (is_shape_object ? 6u : 4u);
            SDL_Rect stroke_color_row =
                left_panel_objects_inspector_action_row_rect(inspector_rect, m, 0u, action_count);
            SDL_Rect fill_color_row =
                left_panel_objects_inspector_action_row_rect(inspector_rect, m, 1u, action_count);
            SDL_Rect stroke_row =
                left_panel_objects_inspector_action_row_rect(inspector_rect, m, 2u, action_count);
            SDL_Rect minus_rect = left_tool_option_minus_rect(stroke_row, m);
            SDL_Rect plus_rect = left_tool_option_plus_rect(stroke_row, m);
            SDL_Rect fill_row = left_panel_objects_inspector_action_row_rect(
                inspector_rect, m, is_shape_object ? 5u : 3u, action_count);
            if (drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_STROKE_COLOR_ROW, selected_object->object_id, stroke_color_row, hooks)) {
                ui->object_color_target_kind =
                    (ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_STROKE &&
                     ui->object_color_target_object_id == selected_object->object_id)
                        ? VISUAL_OBJECT_COLOR_TARGET_NONE
                        : VISUAL_OBJECT_COLOR_TARGET_STROKE;
                ui->object_color_target_object_id =
                    (ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_NONE) ? 0u : selected_object->object_id;
                return;
            }
            if (drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_FILL_COLOR_ROW, selected_object->object_id, fill_color_row, hooks)) {
                ui->object_color_target_kind =
                    (ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_FILL &&
                     ui->object_color_target_object_id == selected_object->object_id)
                        ? VISUAL_OBJECT_COLOR_TARGET_NONE
                        : VISUAL_OBJECT_COLOR_TARGET_FILL;
                ui->object_color_target_object_id =
                    (ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_NONE) ? 0u : selected_object->object_id;
                return;
            }
            if (drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_MINUS_RECT, selected_object->object_id, minus_rect, hooks)) {
                uint8_t stroke_width = selected_object->stroke_width;
                if (stroke_width > 1u) {
                    stroke_width -= 1u;
                } else {
                    stroke_width = 1u;
                }
                (void)drawing_program_history_apply_set_object_stroke_width(
                    &ctx->history, &ctx->object_store, selected_object->object_id, stroke_width);
                return;
            }
            if (drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_PLUS_RECT, selected_object->object_id, plus_rect, hooks)) {
                uint8_t stroke_width = selected_object->stroke_width;
                if (stroke_width < 16u) {
                    stroke_width += 1u;
                } else {
                    stroke_width = 16u;
                }
                (void)drawing_program_history_apply_set_object_stroke_width(
                    &ctx->history, &ctx->object_store, selected_object->object_id, stroke_width);
                return;
            }
            if (is_shape_object) {
                SDL_Rect width_row =
                    left_panel_objects_inspector_action_row_rect(inspector_rect, m, 3u, action_count);
                SDL_Rect width_minus_rect = left_tool_option_minus_rect(width_row, m);
                SDL_Rect width_plus_rect = left_tool_option_plus_rect(width_row, m);
                SDL_Rect height_row =
                    left_panel_objects_inspector_action_row_rect(inspector_rect, m, 4u, action_count);
                SDL_Rect height_minus_rect = left_tool_option_minus_rect(height_row, m);
                SDL_Rect height_plus_rect = left_tool_option_plus_rect(height_row, m);
                if (drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_WIDTH_MINUS_RECT, selected_object->object_id, width_minus_rect, hooks) ||
                    drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_WIDTH_PLUS_RECT, selected_object->object_id, width_plus_rect, hooks)) {
                    uint32_t next_width = selected_object->width;
                    if (drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_WIDTH_MINUS_RECT, selected_object->object_id, width_minus_rect, hooks)) {
                        next_width = (next_width > 1u) ? (next_width - 1u) : 1u;
                    } else {
                        next_width += 1u;
                    }
                    (void)drawing_program_history_apply_set_object_size(&ctx->history,
                                                                        &ctx->object_store,
                                                                        selected_object->object_id,
                                                                        next_width,
                                                                        selected_object->height);
                    return;
                }
                if (drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_HEIGHT_MINUS_RECT, selected_object->object_id, height_minus_rect, hooks) ||
                    drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_HEIGHT_PLUS_RECT, selected_object->object_id, height_plus_rect, hooks)) {
                    uint32_t next_height = selected_object->height;
                    if (drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_HEIGHT_MINUS_RECT, selected_object->object_id, height_minus_rect, hooks)) {
                        next_height = (next_height > 1u) ? (next_height - 1u) : 1u;
                    } else {
                        next_height += 1u;
                    }
                    (void)drawing_program_history_apply_set_object_size(&ctx->history,
                                                                        &ctx->object_store,
                                                                        selected_object->object_id,
                                                                        selected_object->width,
                                                                        next_height);
                    return;
                }
            }
            if (drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_FILL_ROW, selected_object->object_id, fill_row, hooks)) {
                uint8_t next_style_mode =
                    visual_object_style_set_fill_enabled(selected_object->style_mode,
                                                         !visual_object_style_fill_enabled(selected_object->style_mode));
                (void)drawing_program_history_apply_set_object_style_mode(
                    &ctx->history, &ctx->object_store, selected_object->object_id, next_style_mode);
                return;
            }
            if (selected_object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
                SDL_Rect path_row =
                    left_panel_objects_inspector_action_row_rect(inspector_rect, m, 4u, action_count);
                if (drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_PATH_ROW, selected_object->object_id, path_row, hooks)) {
                    if (selected_object->path_closed || selected_object->path_point_count >= 3u) {
                        (void)drawing_program_history_apply_set_object_path_closed(
                            &ctx->history,
                            &ctx->object_store,
                            selected_object->object_id,
                            selected_object->path_closed ? 0u : 1u);
                    }
                    return;
                }
            }
        }
        return;
    }
    tool_count = hooks->visual_tool_count();
    option_count = hooks->visual_tool_option_count(ctx, ctx->editor.active_tool);

    for (i = 0u; i < tool_count; ++i) {
        SDL_Rect row = left_panel_tool_row_rect(rect, m, i, tool_count);
        DrawingProgramToolKind tool = hooks->visual_tool_at(i);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_LEFT_TOOL, (uint64_t)tool, row, hooks)) {
            hooks->apply_workflow_control_if_valid(ctx, hooks->workflow_control_for_tool(tool));
            return;
        }
    }

    {
        SDL_Rect detail_rect = left_panel_tool_detail_rect(rect, m, tool_count);
        uint32_t option_i;
        for (option_i = 0u; option_i < option_count; ++option_i) {
            uint32_t option_kind_raw =
                hooks->visual_tool_option_kind_for_index_raw(ctx, ctx->editor.active_tool, option_i);
            SDL_Rect option_row = left_panel_tool_detail_option_row_rect(detail_rect, m, option_i);
            if (hooks->visual_tool_option_is_action_button_raw(option_kind_raw)) {
                if (drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_OPTION_ROW, option_kind_raw, option_row, hooks) &&
                    hooks->visual_tool_option_is_select_delete_raw(option_kind_raw) &&
                    selection &&
                    selection->has_payload &&
                    hooks->active_layer_allows_edits_visual(ctx)) {
                    (void)drawing_program_selection_delete_payload(&ctx->document,
                                                                   &ctx->layer_rasters,
                                                                   ctx->editor.active_layer_id,
                                                                   &ctx->history,
                                                                   selection);
                    return;
                }
            } else {
                SDL_Rect minus_rect = left_tool_option_minus_rect(option_row, m);
                SDL_Rect plus_rect = left_tool_option_plus_rect(option_row, m);
                if (drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_MINUS_RECT, option_kind_raw, minus_rect, hooks)) {
                    hooks->visual_tool_option_adjust_raw(ctx, option_kind_raw, -1);
                    return;
                }
                if (drawing_program_panel_intent_matches(intent, DRAWING_UI_PANEL_RENDER_PLUS_RECT, option_kind_raw, plus_rect, hooks)) {
                    hooks->visual_tool_option_adjust_raw(ctx, option_kind_raw, 1);
                    return;
                }
            }
        }
    }
}

void drawing_program_visual_input_handle_left_panel_click_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    DrawingProgramSelectionState *selection,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
    drawing_program_visual_input_handle_left_panel_click_payload_intent(ctx, rect, (DrawingProgramPanelIntent){.x=x, .y=y}, selection, ui, hooks);
}
