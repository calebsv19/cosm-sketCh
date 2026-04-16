#include "drawing_program/drawing_program_visual_input_panel_clicks.h"

#include "drawing_program/drawing_program_visual_input_core.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"

enum {
    VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE = 1
};

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
    int content_y;
    SDL_Rect tab_canvas;
    SDL_Rect tab_layer;
    if (!ctx || !ui || !hooks) {
        return;
    }
    m = make_pane_layout_metrics(ctx);
    drawing_program_visual_layer_opacity_sync_document(ctx);
    content_y = rect.y + m.pad_y + m.title_glyph_h + m.section_gap;
    tab_canvas = (SDL_Rect){ rect.x + m.pad_x, content_y, (rect.w - (2 * m.pad_x) - m.tab_gap) / 2, m.tab_h };
    tab_layer =
        (SDL_Rect){ tab_canvas.x + tab_canvas.w + m.tab_gap, content_y, (rect.w - (2 * m.pad_x) - m.tab_gap) / 2, m.tab_h };
    content_y += m.tab_h + m.section_gap;
    if (hooks->point_in_rect(tab_canvas, x, y)) {
        ctx->ui.right_panel_slot = (uint8_t)(VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE - 1);
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (hooks->point_in_rect(tab_layer, x, y)) {
        ctx->ui.right_panel_slot = (uint8_t)VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE;
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
    } else {
        uint8_t palette_i;
        SDL_Rect reset_view_button;
        SDL_Rect clear_canvas_button;
        SDL_Rect delete_selection_button;
        SDL_Rect clear_history_button;
        for (palette_i = 0u; palette_i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++palette_i) {
            SDL_Rect swatch = right_canvas_palette_swatch_rect(rect, m, palette_i);
            if (hooks->point_in_rect(swatch, x, y)) {
                ctx->ui.active_color_index = palette_i;
                return;
            }
        }
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
    }
}
