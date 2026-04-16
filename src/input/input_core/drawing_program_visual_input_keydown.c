#include "drawing_program/drawing_program_visual_input_handlers.h"

#include <stdio.h>

#include "drawing_program/drawing_program_visual_input_keymap.h"
#include "drawing_program/drawing_program_visual_input_selection_ops.h"

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
                                                          (int)app->ui.font_zoom_step,
                                                          &zoom_step)) {
        app->ui.font_zoom_step = (int8_t)hooks->clamp_font_zoom_step(zoom_step);
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
        drawing_program_object_selection_reset(&app->object_selection);
        hooks->cancel_canvas_draw_and_shape(canvas_interaction);
        return 1;
    }
    if (ctrl_or_cmd && event->key.keysym.sym == SDLK_a) {
        app->editor.active_tool = DRAWING_PROGRAM_TOOL_SELECT;
        app->runtime.tool_switch_total += 1u;
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
        *io_selected_theme = hooks->clamp_theme_preset_id(app->ui.theme_preset_id);
        if (hooks->cycle_theme_preset(*io_selected_theme, theme_direction, &next_theme)) {
            *io_selected_theme = next_theme;
            app->ui.theme_preset_id = (uint32_t)(*io_selected_theme);
            if (core_theme_get_preset(*io_selected_theme, io_theme_preset).code != CORE_OK) {
                *io_selected_theme = CORE_THEME_PRESET_DARK_DEFAULT;
                app->ui.theme_preset_id = (uint32_t)(*io_selected_theme);
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
    if (app->editor.active_tool == DRAWING_PROGRAM_TOOL_PATH) {
        if (event->key.keysym.sym == SDLK_RETURN || event->key.keysym.sym == SDLK_KP_ENTER) {
            CoreResult commit_result = hooks->path_draft_commit_closed
                                           ? hooks->path_draft_commit_closed(app, canvas_interaction)
                                           : core_result_ok();
            if (commit_result.code != CORE_OK) {
                fprintf(stderr, "drawing_program: path commit failed: %s\n", commit_result.message);
            }
            return 1;
        }
        if (event->key.keysym.sym == SDLK_ESCAPE) {
            if (hooks->path_draft_reset) {
                hooks->path_draft_reset(canvas_interaction);
            }
            return 1;
        }
        if ((event->key.keysym.sym == SDLK_BACKSPACE || event->key.keysym.sym == SDLK_DELETE) &&
            canvas_interaction->path_draft_point_count > 0u) {
            if (hooks->path_draft_pop_point) {
                (void)hooks->path_draft_pop_point(canvas_interaction);
            }
            return 1;
        }
    }
    if ((event->key.keysym.sym == SDLK_BACKSPACE || event->key.keysym.sym == SDLK_DELETE) &&
        hooks->delete_active_selection_payload_or_objects &&
        hooks->delete_active_selection_payload_or_objects(app, selection_state, hooks)) {
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
