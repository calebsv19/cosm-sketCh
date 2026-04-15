#include "drawing_program/drawing_program_visual_input_handlers.h"

#include <stdio.h>

#include "drawing_program/drawing_program_visual_input_core.h"
#include "drawing_program/drawing_program_visual_input_keymap.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"

enum {
    VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE = 1
};

static void object_selection_clear(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    drawing_program_object_selection_reset(&ctx->object_selection);
}

static void object_selection_replace(DrawingProgramAppContext *ctx, uint32_t object_id) {
    if (!ctx) {
        return;
    }
    drawing_program_object_selection_replace_single(&ctx->object_selection, object_id);
}

static void object_selection_add(DrawingProgramAppContext *ctx, uint32_t object_id) {
    if (!ctx) {
        return;
    }
    (void)drawing_program_object_selection_add(&ctx->object_selection, object_id);
}

static void object_selection_remove(DrawingProgramAppContext *ctx, uint32_t object_id) {
    uint32_t i;
    uint32_t write_index = 0u;
    uint32_t new_active = 0u;
    if (!ctx || object_id == 0u || ctx->object_selection.count == 0u) {
        return;
    }
    for (i = 0u; i < ctx->object_selection.count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        uint32_t id = ctx->object_selection.object_ids[i];
        if (id == 0u || id == object_id) {
            continue;
        }
        ctx->object_selection.object_ids[write_index] = id;
        new_active = id;
        write_index += 1u;
    }
    for (i = write_index; i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        ctx->object_selection.object_ids[i] = 0u;
    }
    ctx->object_selection.count = write_index;
    ctx->object_selection.active_object_id = new_active;
}

static int object_selection_hit_test(const DrawingProgramAppContext *ctx,
                                     uint32_t sample_x,
                                     uint32_t sample_y,
                                     uint32_t *out_object_id) {
    CoreResult result;
    if (!ctx || !out_object_id) {
        return 0;
    }
    result = drawing_program_object_store_hit_test_topmost(
        &ctx->object_store, &ctx->document, sample_x, sample_y, out_object_id, 0);
    return (result.code == CORE_OK) ? 1 : 0;
}

static VisualMarqueeCommitMode resolve_select_commit_mode(const DrawingProgramAppContext *ctx,
                                                          SDL_Keymod mods) {
    uint8_t ui_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
    if ((mods & KMOD_ALT) != 0) {
        return VISUAL_MARQUEE_COMMIT_SUBTRACT;
    }
    if ((mods & KMOD_SHIFT) != 0) {
        return VISUAL_MARQUEE_COMMIT_ADD;
    }
    if (ctx) {
        ui_mode = ctx->ui_tool_select_mode;
    }
    if (ui_mode > (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_SUBTRACT) {
        ui_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
    }
    return (VisualMarqueeCommitMode)ui_mode;
}

static void handle_left_panel_click_payload(DrawingProgramAppContext *ctx,
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

static void handle_right_panel_click_payload(DrawingProgramAppContext *ctx,
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
    tab_layer = (SDL_Rect){ tab_canvas.x + tab_canvas.w + m.tab_gap, content_y, (rect.w - (2 * m.pad_x) - m.tab_gap) / 2, m.tab_h };
    content_y += m.tab_h + m.section_gap;
    if (hooks->point_in_rect(tab_canvas, x, y)) {
        ctx->ui_right_panel_slot = (uint8_t)(VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE - 1);
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (hooks->point_in_rect(tab_layer, x, y)) {
        ctx->ui_right_panel_slot = (uint8_t)VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE;
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (hooks->clamp_right_slot(ctx->ui_right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE) {
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
                (void)drawing_program_runtime_orchestration_set_active_layer_id(ctx, ctx->document.layers[model_i].layer_id);
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
                ctx->ui_active_color_index = palette_i;
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
            selection->has_payload &&
            hooks->active_layer_allows_edits_visual(ctx)) {
            (void)drawing_program_selection_delete_payload(&ctx->document,
                                                           &ctx->layer_rasters,
                                                           ctx->editor.active_layer_id,
                                                           &ctx->history,
                                                           selection);
            return;
        }
        clear_history_button = right_canvas_clear_history_button_rect(rect, m);
        if (hooks->point_in_rect(clear_history_button, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_HISTORY);
            return;
        }
    }
}

int drawing_program_visual_input_handle_mouse_button_up_payload(const SDL_Event *event,
                                                                int event_has_position,
                                                                int event_x,
                                                                int event_y,
                                                                int has_canvas_pane,
                                                                SDL_Rect canvas_pane,
                                                                DrawingProgramAppContext *app,
                                                                VisualCanvasInteractionState *canvas_interaction,
                                                                DrawingProgramSelectionState *selection_state,
                                                                const DrawingProgramVisualInputHandlersHooks *hooks) {
    DrawingProgramVisualMouseUpAction up_action;
    int release_x = 0;
    int release_y = 0;
    uint32_t sample_x = 0u;
    uint32_t sample_y = 0u;
    CoreResult move_commit;
    if (!event || !app || !canvas_interaction || !selection_state || !hooks) {
        return 0;
    }
    up_action = drawing_program_visual_input_classify_mouse_up(event->button.button);
    if (up_action == DRAWING_PROGRAM_VISUAL_MOUSE_UP_ACTION_RIGHT_RELEASE) {
        hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 1);
        return 0;
    }
    if (up_action != DRAWING_PROGRAM_VISUAL_MOUSE_UP_ACTION_LEFT_RELEASE) {
        return 0;
    }
    drawing_program_visual_input_resolve_button_coords(event,
                                                       event_has_position,
                                                       event_x,
                                                       event_y,
                                                       &release_x,
                                                       &release_y);
    if (canvas_interaction->panning_active) {
        hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 1);
        return 1;
    }
    if (has_canvas_pane &&
        hooks->screen_to_canvas_sample(app, canvas_pane, release_x, release_y, &sample_x, &sample_y)) {
        if (selection_state->selecting) {
            selection_state->marquee_end_x = sample_x;
            selection_state->marquee_end_y = sample_y;
        }
        if (hooks->visual_transform_session_is_object_move_active(canvas_interaction)) {
            (void)hooks->screen_to_canvas_sample_clamped(
                app, canvas_pane, release_x, release_y, &sample_x, &sample_y);
            hooks->visual_transform_session_update_object_move(
                app, canvas_interaction, sample_x, sample_y, SDL_GetModState());
        } else if (hooks->visual_transform_session_is_move_active(canvas_interaction) && selection_state->moving) {
            (void)hooks->screen_to_canvas_sample_clamped(
                app, canvas_pane, release_x, release_y, &sample_x, &sample_y);
            hooks->visual_transform_session_update_move(app,
                                                        canvas_interaction,
                                                        selection_state,
                                                        sample_x,
                                                        sample_y,
                                                        SDL_GetModState());
        }
    }
    if (selection_state->selecting) {
        if (app->editor.active_tool == DRAWING_PROGRAM_TOOL_SELECT &&
            selection_state->marquee_start_x == selection_state->marquee_end_x &&
            selection_state->marquee_start_y == selection_state->marquee_end_y) {
            /* Empty-canvas click in select mode should clear all selection lanes. */
            object_selection_clear(app);
            drawing_program_selection_reset(selection_state);
            hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 0);
            return 1;
        }
        VisualMarqueeCommitMode mode =
            hooks->visual_marquee_commit_mode_clamp(canvas_interaction->marquee_commit_mode);
        (void)hooks->visual_selection_capture_from_marquee(app, selection_state, mode);
    }
    if (hooks->visual_transform_session_is_move_active(canvas_interaction) && selection_state->moving) {
        move_commit = hooks->visual_transform_session_commit_move(app, canvas_interaction, selection_state);
        if (move_commit.code != CORE_OK) {
            fprintf(stderr, "drawing_program: selection move commit failed: %s\n", move_commit.message);
        }
    } else if (hooks->visual_transform_session_is_object_move_active(canvas_interaction)) {
        move_commit = hooks->visual_transform_session_commit_object_move(app, canvas_interaction);
        if (move_commit.code != CORE_OK) {
            fprintf(stderr, "drawing_program: object move commit failed: %s\n", move_commit.message);
        }
    }
    if (canvas_interaction->shape_active &&
        hooks->tool_uses_shape_commit((DrawingProgramToolKind)canvas_interaction->shape_tool) &&
        has_canvas_pane &&
        hooks->screen_to_canvas_sample(app, canvas_pane, release_x, release_y, &sample_x, &sample_y)) {
        (void)hooks->apply_canvas_shape_commit(app,
                                               (DrawingProgramToolKind)canvas_interaction->shape_tool,
                                               canvas_interaction->shape_start_sample_x,
                                               canvas_interaction->shape_start_sample_y,
                                               sample_x,
                                               sample_y);
    }
    hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 0);
    return 0;
}

int drawing_program_visual_input_handle_mouse_motion_payload(const SDL_Event *event,
                                                             int event_has_position,
                                                             int event_x,
                                                             int event_y,
                                                             int has_canvas_pane,
                                                             SDL_Rect canvas_pane,
                                                             DrawingProgramAppContext *app,
                                                             VisualCanvasInteractionState *canvas_interaction,
                                                             DrawingProgramSelectionState *selection_state,
                                                             VisualPanelUiState *panel_ui,
                                                             const DrawingProgramVisualInputHandlersHooks *hooks) {
    if (!event || !app || !canvas_interaction || !selection_state || !panel_ui || !hooks) {
        return 0;
    }
    if (event->type != SDL_MOUSEMOTION || !has_canvas_pane) {
        return 0;
    }
    panel_ui->mouse_known = 1u;
    panel_ui->mouse_x = event_has_position ? event_x : event->motion.x;
    panel_ui->mouse_y = event_has_position ? event_y : event->motion.y;
    if (drawing_program_visual_input_apply_pan_motion(app,
                                                      canvas_interaction->panning_active,
                                                      panel_ui->mouse_x,
                                                      panel_ui->mouse_y,
                                                      &canvas_interaction->last_mouse_x,
                                                      &canvas_interaction->last_mouse_y)) {
        return 1;
    }
    if (selection_state->selecting && app->editor.active_tool == DRAWING_PROGRAM_TOOL_SELECT) {
        uint32_t sample_x = 0u;
        uint32_t sample_y = 0u;
        if (hooks->screen_to_canvas_sample(
                app, canvas_pane, panel_ui->mouse_x, panel_ui->mouse_y, &sample_x, &sample_y)) {
            selection_state->marquee_end_x = sample_x;
            selection_state->marquee_end_y = sample_y;
        }
    }
    if (hooks->visual_transform_session_is_object_move_active(canvas_interaction) &&
        app->editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE) {
        uint32_t sample_x = 0u;
        uint32_t sample_y = 0u;
        if (hooks->screen_to_canvas_sample_clamped(
                app, canvas_pane, panel_ui->mouse_x, panel_ui->mouse_y, &sample_x, &sample_y)) {
            hooks->visual_transform_session_update_object_move(app,
                                                               canvas_interaction,
                                                               sample_x,
                                                               sample_y,
                                                               SDL_GetModState());
        }
    } else if (hooks->visual_transform_session_is_move_active(canvas_interaction) &&
        selection_state->moving &&
        app->editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE) {
        uint32_t sample_x = 0u;
        uint32_t sample_y = 0u;
        if (hooks->screen_to_canvas_sample_clamped(
                app, canvas_pane, panel_ui->mouse_x, panel_ui->mouse_y, &sample_x, &sample_y)) {
            hooks->visual_transform_session_update_move(app,
                                                        canvas_interaction,
                                                        selection_state,
                                                        sample_x,
                                                        sample_y,
                                                        SDL_GetModState());
        }
    }
    if (canvas_interaction->drawing_active) {
        (void)hooks->apply_canvas_draw_at_screen(
            app, canvas_pane, panel_ui->mouse_x, panel_ui->mouse_y, canvas_interaction);
    }
    return 0;
}

int drawing_program_visual_input_handle_mouse_button_down_payload(const SDL_Event *event,
                                                                  int event_has_position,
                                                                  int event_x,
                                                                  int event_y,
                                                                  int has_left_pane,
                                                                  SDL_Rect left_pane,
                                                                  int has_right_pane,
                                                                  SDL_Rect right_pane,
                                                                  int has_canvas_pane,
                                                                  SDL_Rect canvas_pane,
                                                                  DrawingProgramAppContext *app,
                                                                  VisualCanvasInteractionState *canvas_interaction,
                                                                  DrawingProgramSelectionState *selection_state,
                                                                  VisualPanelUiState *panel_ui,
                                                                  const DrawingProgramVisualInputHandlersHooks *hooks) {
    int click_x = 0;
    int click_y = 0;
    uint32_t sample_x = 0u;
    uint32_t sample_y = 0u;
    DrawingProgramVisualPaneHitState hit;
    DrawingProgramVisualMouseDownAction down_action;
    if (!event || !app || !canvas_interaction || !selection_state || !panel_ui || !hooks) {
        return 0;
    }
    if (event->type != SDL_MOUSEBUTTONDOWN) {
        return 0;
    }
    drawing_program_visual_input_resolve_button_coords(event,
                                                       event_has_position,
                                                       event_x,
                                                       event_y,
                                                       &click_x,
                                                       &click_y);
    hit = drawing_program_visual_input_classify_hit(click_x,
                                                    click_y,
                                                    has_left_pane,
                                                    left_pane,
                                                    has_right_pane,
                                                    right_pane,
                                                    has_canvas_pane,
                                                    canvas_pane);
    down_action = drawing_program_visual_input_classify_mouse_down(event->button.button, hit);
    if (down_action == DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_START_PAN) {
        hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 0);
        drawing_program_visual_input_begin_pan(click_x,
                                               click_y,
                                               &canvas_interaction->panning_active,
                                               &canvas_interaction->last_mouse_x,
                                               &canvas_interaction->last_mouse_y);
        return 1;
    }
    if (down_action == DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_HANDLE_LEFT_PANEL) {
        hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 0);
        handle_left_panel_click_payload(app, left_pane, click_x, click_y, selection_state, panel_ui, hooks);
        return 1;
    }
    if (down_action == DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_HANDLE_RIGHT_PANEL) {
        hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 0);
        handle_right_panel_click_payload(app, right_pane, click_x, click_y, selection_state, panel_ui, hooks);
        return 1;
    }
    if (down_action == DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_CLEAR_OUTSIDE_PANES) {
        hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 0);
        return 1;
    }
    if (down_action == DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_CANVAS_LEFT &&
        !canvas_interaction->panning_active) {
        if (app->editor.active_tool == DRAWING_PROGRAM_TOOL_SELECT &&
            hooks->screen_to_canvas_sample(app, canvas_pane, click_x, click_y, &sample_x, &sample_y)) {
            uint32_t hit_object_id = 0u;
            SDL_Keymod mods = SDL_GetModState();
            VisualMarqueeCommitMode select_mode = resolve_select_commit_mode(app, mods);
            if (object_selection_hit_test(app, sample_x, sample_y, &hit_object_id)) {
                if (select_mode == VISUAL_MARQUEE_COMMIT_SUBTRACT) {
                    object_selection_remove(app, hit_object_id);
                } else if (select_mode == VISUAL_MARQUEE_COMMIT_ADD) {
                    object_selection_add(app, hit_object_id);
                } else {
                    object_selection_replace(app, hit_object_id);
                }
                /* Object-select mode is exclusive with pixel payload selection preview. */
                drawing_program_selection_reset(selection_state);
                hooks->cancel_canvas_draw_and_shape(canvas_interaction);
                hooks->cancel_selection_transient(selection_state);
                return 1;
            }
            if (select_mode == VISUAL_MARQUEE_COMMIT_REPLACE) {
                object_selection_clear(app);
            }
            drawing_program_selection_begin_marquee(selection_state, sample_x, sample_y);
            hooks->cancel_canvas_draw_and_shape(canvas_interaction);
            canvas_interaction->marquee_commit_mode = (uint8_t)select_mode;
        } else if (app->editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE &&
                   app->object_selection.count > 0u &&
                   hooks->screen_to_canvas_sample_clamped(
                       app, canvas_pane, click_x, click_y, &sample_x, &sample_y)) {
            hooks->cancel_canvas_draw_and_shape(canvas_interaction);
            hooks->cancel_selection_transient(selection_state);
            hooks->visual_transform_session_begin_object_move(canvas_interaction, sample_x, sample_y);
        } else if (app->editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE &&
                   selection_state->has_payload &&
                   ((hooks->screen_to_canvas_sample(app, canvas_pane, click_x, click_y, &sample_x, &sample_y) &&
                     hooks->visual_selection_begin_move(selection_state, sample_x, sample_y)) ||
                    hooks->selection_move_handle_hit(app, canvas_pane, selection_state, click_x, click_y)) &&
                   hooks->screen_to_canvas_sample_clamped(
                       app, canvas_pane, click_x, click_y, &sample_x, &sample_y)) {
            hooks->cancel_canvas_draw_and_shape(canvas_interaction);
            hooks->visual_transform_session_begin_move(canvas_interaction, selection_state, sample_x, sample_y);
        } else if (app->editor.active_tool == DRAWING_PROGRAM_TOOL_PICKER) {
            hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 0);
            (void)hooks->apply_canvas_picker_at_screen(app, canvas_pane, click_x, click_y);
        } else if (app->editor.active_tool == DRAWING_PROGRAM_TOOL_FILL) {
            hooks->begin_canvas_history_group(app);
            (void)hooks->apply_canvas_fill_at_screen(app, canvas_pane, click_x, click_y);
            hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 0);
        } else if (hooks->tool_uses_shape_commit(app->editor.active_tool) &&
                   hooks->screen_to_canvas_sample(app, canvas_pane, click_x, click_y, &sample_x, &sample_y)) {
            hooks->begin_canvas_history_group(app);
            hooks->cancel_selection_transient(selection_state);
            canvas_interaction->shape_active = 1u;
            canvas_interaction->shape_tool = (uint8_t)app->editor.active_tool;
            canvas_interaction->shape_start_sample_x = sample_x;
            canvas_interaction->shape_start_sample_y = sample_y;
            canvas_interaction->drawing_active = 0u;
            canvas_interaction->has_last_sample = 0u;
        } else if (hooks->tool_uses_direct_sample_stroke(app->editor.active_tool)) {
            canvas_interaction->shape_active = 0u;
            canvas_interaction->drawing_active = 1u;
            canvas_interaction->has_last_sample = 0u;
            hooks->begin_canvas_history_group(app);
            (void)hooks->apply_canvas_draw_at_screen(app, canvas_pane, click_x, click_y, canvas_interaction);
        }
    }
    return 0;
}

int drawing_program_visual_input_handle_keydown_payload(const SDL_Event *event,
                                                        int has_canvas_pane,
                                                        SDL_Rect canvas_pane,
                                                        CoreThemePresetId *io_selected_theme,
                                                        CoreThemePreset *io_theme_preset,
                                                        DrawingProgramAppContext *app,
                                                        VisualCanvasInteractionState *canvas_interaction,
                                                        DrawingProgramSelectionState *selection_state,
                                                        VisualPanelUiState *panel_ui,
                                                        const DrawingProgramVisualInputHandlersHooks *hooks) {
    DrawingProgramWorkflowControl control = DRAWING_PROGRAM_WORKFLOW_CONTROL_NONE;
    int ctrl_or_cmd;
    int shift;
    uint32_t module_type_id = 0u;
    int zoom_step = 0;
    int theme_direction = 0;
    if (!event || !io_selected_theme || !io_theme_preset || !app || !canvas_interaction || !selection_state ||
        !panel_ui || !hooks) {
        return 0;
    }
    if (event->type != SDL_KEYDOWN) {
        return 0;
    }
    ctrl_or_cmd = (event->key.keysym.mod & (KMOD_CTRL | KMOD_GUI)) != 0;
    shift = (event->key.keysym.mod & KMOD_SHIFT) != 0;
    if (drawing_program_visual_input_try_module_slot_hotkey(ctrl_or_cmd,
                                                            shift,
                                                            event->key.keysym.sym,
                                                            &module_type_id)) {
        (void)hooks->set_module_type_for_pane(app, 6u, module_type_id);
        return 1;
    }
    if (drawing_program_visual_input_try_font_zoom_hotkey(ctrl_or_cmd,
                                                          event->key.keysym.sym,
                                                          (int)app->ui_font_zoom_step,
                                                          &zoom_step)) {
        app->ui_font_zoom_step = (int8_t)hooks->clamp_font_zoom_step(zoom_step);
        return 1;
    }
    if (ctrl_or_cmd && event->key.keysym.sym == SDLK_k) {
        hooks->apply_workflow_control_if_valid(app, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_HISTORY);
        return 1;
    }
    if (ctrl_or_cmd && event->key.keysym.sym == SDLK_r) {
        hooks->cancel_canvas_draw_and_shape(canvas_interaction);
        hooks->cancel_selection_transient(selection_state);
        hooks->apply_workflow_control_if_valid(app, DRAWING_PROGRAM_WORKFLOW_CONTROL_RASTERIZE_SELECTED_OBJECTS);
        return 1;
    }
    if (ctrl_or_cmd && event->key.keysym.sym == SDLK_c) {
        (void)drawing_program_selection_copy_payload(selection_state, &app->clipboard);
        return 1;
    }
    if (ctrl_or_cmd && event->key.keysym.sym == SDLK_x) {
        if (hooks->active_layer_allows_edits_visual(app)) {
            hooks->cancel_canvas_draw_and_shape(canvas_interaction);
            hooks->cancel_selection_transient(selection_state);
            (void)drawing_program_selection_cut_to_clipboard(&app->document,
                                                             &app->layer_rasters,
                                                             app->editor.active_layer_id,
                                                             &app->history,
                                                             selection_state,
                                                             &app->clipboard);
        }
        return 1;
    }
    if (ctrl_or_cmd && event->key.keysym.sym == SDLK_v) {
        if (hooks->active_layer_allows_edits_visual(app)) {
            int32_t paste_x = 0;
            int32_t paste_y = 0;
            uint32_t sample_x = 0u;
            uint32_t sample_y = 0u;
            hooks->cancel_canvas_draw_and_shape(canvas_interaction);
            hooks->cancel_selection_transient(selection_state);
            if (selection_state->has_payload) {
                paste_x = (int32_t)selection_state->origin_x;
                paste_y = (int32_t)selection_state->origin_y;
            } else if (app->clipboard.has_payload) {
                if (app->document.raster_width > app->clipboard.width) {
                    paste_x = (int32_t)((app->document.raster_width - app->clipboard.width) / 2u);
                }
                if (app->document.raster_height > app->clipboard.height) {
                    paste_y = (int32_t)((app->document.raster_height - app->clipboard.height) / 2u);
                }
            }
            if (has_canvas_pane &&
                panel_ui->mouse_known &&
                hooks->point_in_rect(canvas_pane, panel_ui->mouse_x, panel_ui->mouse_y) &&
                hooks->screen_to_canvas_sample(
                    app, canvas_pane, panel_ui->mouse_x, panel_ui->mouse_y, &sample_x, &sample_y)) {
                paste_x = (int32_t)sample_x;
                paste_y = (int32_t)sample_y;
            }
            (void)drawing_program_selection_paste_from_clipboard(&app->document,
                                                                 &app->layer_rasters,
                                                                 app->editor.active_layer_id,
                                                                 &app->history,
                                                                 selection_state,
                                                                 &app->clipboard,
                                                                 paste_x,
                                                                 paste_y);
        }
        return 1;
    }
    if (ctrl_or_cmd && shift && event->key.keysym.sym == SDLK_n) {
        hooks->apply_workflow_control_if_valid(app, DRAWING_PROGRAM_WORKFLOW_CONTROL_ADD_LAYER);
        return 1;
    }
    if (ctrl_or_cmd && event->key.keysym.sym == SDLK_d) {
        drawing_program_selection_reset(selection_state);
        object_selection_clear(app);
        hooks->cancel_canvas_draw_and_shape(canvas_interaction);
        return 1;
    }
    if (ctrl_or_cmd && event->key.keysym.sym == SDLK_a) {
        app->editor.active_tool = DRAWING_PROGRAM_TOOL_SELECT;
        app->tool_switch_total += 1u;
        (void)drawing_program_selection_select_all(&app->document,
                                                   &app->layer_rasters,
                                                   app->editor.active_layer_id,
                                                   selection_state);
        hooks->cancel_canvas_draw_and_shape(canvas_interaction);
        return 1;
    }
    if (drawing_program_visual_input_try_theme_cycle_hotkey(ctrl_or_cmd,
                                                            shift,
                                                            event->key.keysym.sym,
                                                            &theme_direction)) {
        CoreThemePresetId next_theme = *io_selected_theme;
        *io_selected_theme = hooks->clamp_theme_preset_id(app->ui_theme_preset_id);
        if (hooks->cycle_theme_preset(*io_selected_theme, theme_direction, &next_theme)) {
            *io_selected_theme = next_theme;
            app->ui_theme_preset_id = (uint32_t)(*io_selected_theme);
            if (core_theme_get_preset(*io_selected_theme, io_theme_preset).code != CORE_OK) {
                *io_selected_theme = CORE_THEME_PRESET_DARK_DEFAULT;
                app->ui_theme_preset_id = (uint32_t)(*io_selected_theme);
                (void)core_theme_get_preset(*io_selected_theme, io_theme_preset);
            }
        }
        return 1;
    }
    if (ctrl_or_cmd &&
        (event->key.keysym.sym == SDLK_LEFTBRACKET || event->key.keysym.sym == SDLK_RIGHTBRACKET)) {
        hooks->apply_workflow_control_if_valid(
            app,
            (event->key.keysym.sym == SDLK_LEFTBRACKET) ? DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_PREV
                                                         : DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_NEXT);
        return 1;
    }
    if (event->key.keysym.sym == SDLK_LEFTBRACKET || event->key.keysym.sym == SDLK_RIGHTBRACKET) {
        if (drawing_program_visual_input_try_apply_palette_key(app, event->key.keysym.sym)) {
            return 1;
        }
    }
    if (event->key.keysym.sym >= SDLK_1 && event->key.keysym.sym <= SDLK_8) {
        if (drawing_program_visual_input_try_apply_palette_key(app, event->key.keysym.sym)) {
            return 1;
        }
    }
    if (app->editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE &&
        app->object_selection.count > 0u) {
        int32_t dx = 0;
        int32_t dy = 0;
        CoreResult move_commit;
        if (drawing_program_visual_input_try_move_nudge_key(event->key.keysym.sym, shift, &dx, &dy)) {
            hooks->cancel_canvas_draw_and_shape(canvas_interaction);
            hooks->cancel_selection_transient(selection_state);
            move_commit = hooks->visual_transform_session_nudge_object_move(app, canvas_interaction, dx, dy);
            if (move_commit.code != CORE_OK) {
                fprintf(stderr, "drawing_program: object nudge commit failed: %s\n", move_commit.message);
            }
            return 1;
        }
    } else if (app->editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE && selection_state->has_payload) {
        int32_t dx = 0;
        int32_t dy = 0;
        CoreResult move_commit;
        if (drawing_program_visual_input_try_move_nudge_key(event->key.keysym.sym, shift, &dx, &dy)) {
            hooks->cancel_canvas_draw_and_shape(canvas_interaction);
            hooks->cancel_selection_transient(selection_state);
            move_commit =
                hooks->visual_transform_session_nudge_move(app, canvas_interaction, selection_state, dx, dy);
            if (move_commit.code != CORE_OK) {
                fprintf(stderr, "drawing_program: selection nudge commit failed: %s\n", move_commit.message);
                selection_state->offset_x = 0;
                selection_state->offset_y = 0;
            }
            return 1;
        }
    }
    if ((event->key.keysym.sym == SDLK_BACKSPACE || event->key.keysym.sym == SDLK_DELETE) &&
        selection_state->has_payload &&
        hooks->active_layer_allows_edits_visual(app)) {
        (void)drawing_program_selection_delete_payload(&app->document,
                                                       &app->layer_rasters,
                                                       app->editor.active_layer_id,
                                                       &app->history,
                                                       selection_state);
        return 1;
    }
    control = drawing_program_visual_input_control_for_key(event->key.keysym.sym, event->key.keysym.mod);
    hooks->apply_workflow_control_if_valid(app, control);
    if (control != DRAWING_PROGRAM_WORKFLOW_CONTROL_NONE) {
        hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 0);
        return 1;
    }
    return 0;
}
