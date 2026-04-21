#include "drawing_program/drawing_program_visual_input_panel_clicks.h"

#include "drawing_program/drawing_program_visual_input_core.h"
#include "drawing_program/drawing_program_visual_input_panel_color.h"
#include "drawing_program/drawing_program_ui_color_state.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"

enum {
    VISUAL_LEFT_PANEL_SLOT_TOOLS_VALUE = 0,
    VISUAL_LEFT_PANEL_SLOT_OBJECTS_VALUE = 1,
    VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE = 0,
    VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE = 1,
    VISUAL_RIGHT_PANEL_SLOT_COLOR_VALUE = 2
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

void drawing_program_visual_input_handle_left_panel_click_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    DrawingProgramSelectionState *selection,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
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
        if (hooks->point_in_rect(tab_tools, x, y)) {
            ctx->ui.left_panel_slot = (uint8_t)VISUAL_LEFT_PANEL_SLOT_TOOLS_VALUE;
            hooks->sync_panel_ui_from_app(ctx, ui);
            return;
        }
        if (hooks->point_in_rect(tab_objects, x, y)) {
            ctx->ui.left_panel_slot = (uint8_t)VISUAL_LEFT_PANEL_SLOT_OBJECTS_VALUE;
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
            if (row.y + row.h > list_rect.y + list_rect.h) {
                break;
            }
            if (hooks->point_in_rect(row, x, y)) {
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
            if (hooks->point_in_rect(stroke_color_row, x, y)) {
                ui->object_color_target_kind =
                    (ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_STROKE &&
                     ui->object_color_target_object_id == selected_object->object_id)
                        ? VISUAL_OBJECT_COLOR_TARGET_NONE
                        : VISUAL_OBJECT_COLOR_TARGET_STROKE;
                ui->object_color_target_object_id =
                    (ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_NONE) ? 0u : selected_object->object_id;
                return;
            }
            if (hooks->point_in_rect(fill_color_row, x, y)) {
                ui->object_color_target_kind =
                    (ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_FILL &&
                     ui->object_color_target_object_id == selected_object->object_id)
                        ? VISUAL_OBJECT_COLOR_TARGET_NONE
                        : VISUAL_OBJECT_COLOR_TARGET_FILL;
                ui->object_color_target_object_id =
                    (ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_NONE) ? 0u : selected_object->object_id;
                return;
            }
            if (hooks->point_in_rect(minus_rect, x, y)) {
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
            if (hooks->point_in_rect(plus_rect, x, y)) {
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
                if (hooks->point_in_rect(width_minus_rect, x, y) ||
                    hooks->point_in_rect(width_plus_rect, x, y)) {
                    uint32_t next_width = selected_object->width;
                    if (hooks->point_in_rect(width_minus_rect, x, y)) {
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
                if (hooks->point_in_rect(height_minus_rect, x, y) ||
                    hooks->point_in_rect(height_plus_rect, x, y)) {
                    uint32_t next_height = selected_object->height;
                    if (hooks->point_in_rect(height_minus_rect, x, y)) {
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
            if (hooks->point_in_rect(fill_row, x, y)) {
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
                if (hooks->point_in_rect(path_row, x, y)) {
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
        if (hooks->point_in_rect(row, x, y)) {
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
                if (hooks->point_in_rect(option_row, x, y) &&
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
                if (hooks->point_in_rect(minus_rect, x, y)) {
                    hooks->visual_tool_option_adjust_raw(ctx, option_kind_raw, -1);
                    return;
                }
                if (hooks->point_in_rect(plus_rect, x, y)) {
                    hooks->visual_tool_option_adjust_raw(ctx, option_kind_raw, 1);
                    return;
                }
            }
        }
    }
}

void drawing_program_visual_input_handle_right_panel_click_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    DrawingProgramSelectionState *selection,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
    VisualPaneLayoutMetrics m;
    SDL_Rect tab_canvas;
    SDL_Rect tab_layer;
    SDL_Rect tab_color;
    if (!ctx || !ui || !hooks) {
        return;
    }
    m = make_pane_layout_metrics(ctx);
    drawing_program_visual_layer_opacity_sync_document(ctx);
    tab_canvas = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE, 3u);
    tab_layer = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE, 3u);
    tab_color = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_COLOR_VALUE, 3u);
    if (hooks->point_in_rect(tab_canvas, x, y)) {
        ctx->ui.right_panel_slot = (uint8_t)VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE;
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (hooks->point_in_rect(tab_layer, x, y)) {
        ctx->ui.right_panel_slot = (uint8_t)VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE;
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (hooks->point_in_rect(tab_color, x, y)) {
        ctx->ui.right_panel_slot = (uint8_t)VISUAL_RIGHT_PANEL_SLOT_COLOR_VALUE;
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE) {
        uint32_t display_i;
        SDL_Rect action;
        SDL_Rect opacity_row;
        SDL_Rect opacity_track;
        uint32_t active_layer_id = 0u;
        uint32_t active_layer_index = 0u;
        for (display_i = 0u; display_i < ctx->document.layer_count; ++display_i) {
            uint32_t model_i = (ctx->document.layer_count - 1u) - display_i;
            SDL_Rect row = right_layer_row_rect(rect, m, display_i);
            if (hooks->point_in_rect(row, x, y)) {
                (void)drawing_program_runtime_orchestration_set_active_layer_id(
                    ctx, ctx->document.layers[model_i].layer_id);
                return;
            }
        }
        if (hooks->active_layer_query(ctx, &active_layer_id, &active_layer_index, 0, 0) &&
            active_layer_index < ctx->document.layer_count) {
            opacity_row = right_layer_opacity_row_rect(rect, m, ctx->document.layer_count);
            opacity_track = right_layer_opacity_track_rect(opacity_row, m);
            if (hooks->point_in_rect(opacity_row, x, y)) {
                int relative_x = x - opacity_track.x;
                int opacity = 100;
                if (relative_x < 0) {
                    relative_x = 0;
                }
                if (relative_x > opacity_track.w) {
                    relative_x = opacity_track.w;
                }
                if (opacity_track.w > 0) {
                    opacity = (relative_x * 100) / opacity_track.w;
                }
                if (opacity < 0) {
                    opacity = 0;
                }
                if (opacity > 100) {
                    opacity = 100;
                }
                drawing_program_visual_layer_opacity_set(ctx, active_layer_id, (uint8_t)opacity);
                return;
            }
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ADD);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_ADD_LAYER);
            drawing_program_visual_layer_opacity_sync_document(ctx);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_DUPLICATE);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_layer_duplicate_active(ctx);
            drawing_program_visual_layer_opacity_sync_document(ctx);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_RENAME);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_layer_rename_auto(ctx);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_DELETE);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_DELETE_ACTIVE_LAYER);
            drawing_program_visual_layer_opacity_sync_document(ctx);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ACTIVE_PREV);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_PREV);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ACTIVE_NEXT);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_NEXT);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_MOVE_UP);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_UP);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_MOVE_DOWN);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_DOWN);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_TOGGLE_VISIBLE);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_TOGGLE_LOCK);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_LOCK);
            return;
        }
    } else if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE) {
        SDL_Rect reset_view_button;
        SDL_Rect clear_canvas_button;
        SDL_Rect delete_selection_button;
        SDL_Rect clear_history_button;
        reset_view_button = right_canvas_reset_view_button_rect(rect, m);
        if (hooks->point_in_rect(reset_view_button, x, y)) {
            ctx->editor.viewport.pan_x = 0.0f;
            ctx->editor.viewport.pan_y = 0.0f;
            ctx->editor.viewport.zoom = 1.0f;
            return;
        }
        clear_canvas_button = right_canvas_clear_canvas_button_rect(rect, m);
        if (hooks->point_in_rect(clear_canvas_button, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS);
            return;
        }
        delete_selection_button = right_canvas_delete_selection_button_rect(rect, m);
        if (hooks->point_in_rect(delete_selection_button, x, y) &&
            selection &&
            hooks->delete_active_selection_payload_or_objects &&
            hooks->delete_active_selection_payload_or_objects(ctx, selection, hooks)) {
            return;
        }
        clear_history_button = right_canvas_clear_history_button_rect(rect, m);
        if (hooks->point_in_rect(clear_history_button, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_HISTORY);
            return;
        }
    } else if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_COLOR_VALUE) {
        if (drawing_program_visual_input_handle_right_color_panel_click_payload(ctx, rect, x, y, ui, hooks)) {
            return;
        }
    }
}
