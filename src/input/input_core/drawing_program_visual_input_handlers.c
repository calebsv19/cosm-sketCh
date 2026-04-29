#include "drawing_program/drawing_program_visual_input_handlers.h"

#include <limits.h>
#include <string.h>

#include "drawing_program/drawing_program_object_geometry.h"
#include "drawing_program/drawing_program_ui_color_state.h"
#include "drawing_program/drawing_program_visual_input_core.h"
#include "drawing_program/drawing_program_visual_input_keymap.h"
#include "drawing_program/drawing_program_visual_input_panel_clicks.h"
#include "drawing_program/drawing_program_visual_input_selection_ops.h"
#include "drawing_program/drawing_program_visual_input_support.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"

static void path_draft_preview_cache_clear(VisualCanvasInteractionState *interaction) {
    if (!interaction) {
        return;
    }
    interaction->path_preview_flatten_dirty = 0u;
    interaction->path_preview_flatten_valid = 0u;
    interaction->path_preview_flatten_point_count = 0u;
}

static void path_draft_preview_cache_invalidate(VisualCanvasInteractionState *interaction) {
    if (!interaction) {
        return;
    }
    interaction->path_preview_flatten_dirty = 1u;
    interaction->path_preview_flatten_valid = 0u;
    interaction->path_preview_flatten_point_count = 0u;
}

static void path_draft_preview_cache_refresh(VisualCanvasInteractionState *interaction) {
    DrawingProgramObjectRecord preview_object;
    CoreResult result;
    uint32_t flat_count = 0u;
    if (!interaction || !interaction->path_preview_flatten_dirty) {
        return;
    }
    interaction->path_preview_flatten_dirty = 0u;
    interaction->path_preview_flatten_valid = 0u;
    interaction->path_preview_flatten_point_count = 0u;
    if (interaction->path_draft_point_count < 2u ||
        interaction->path_draft_point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
        return;
    }
    memset(&preview_object, 0, sizeof(preview_object));
    preview_object.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH;
    preview_object.path_point_count = interaction->path_draft_point_count;
    preview_object.path_closed = interaction->path_draft_closed ? 1u : 0u;
    memcpy(preview_object.path_points,
           interaction->path_draft_points,
           (size_t)preview_object.path_point_count * sizeof(preview_object.path_points[0]));
    result = drawing_program_object_path_flatten_points(&preview_object,
                                                        0,
                                                        0,
                                                        interaction->path_preview_flatten_xy,
                                                        DRAWING_PROGRAM_PATH_DRAFT_PREVIEW_FLAT_MAX_POINTS,
                                                        &flat_count);
    if (result.code != CORE_OK || flat_count < 2u || flat_count > UINT16_MAX) {
        return;
    }
    interaction->path_preview_flatten_point_count = (uint16_t)flat_count;
    interaction->path_preview_flatten_valid = 1u;
}

void path_draft_reset(VisualCanvasInteractionState *interaction) {
    if (!interaction) {
        return;
    }
    interaction->path_draft_active = 0u;
    interaction->path_draft_closed = 0u;
    interaction->path_draft_drag_active = 0u;
    interaction->path_preview_sample_x = 0u;
    interaction->path_preview_sample_y = 0u;
    interaction->path_draft_point_count = 0u;
    interaction->path_draft_drag_point_index = 0u;
    memset(interaction->path_draft_points, 0, sizeof(interaction->path_draft_points));
    path_draft_preview_cache_clear(interaction);
}

int path_draft_pop_point(VisualCanvasInteractionState *interaction) {
    if (!interaction || interaction->path_draft_point_count == 0u) {
        return 0;
    }
    interaction->path_draft_point_count -= 1u;
    memset(&interaction->path_draft_points[interaction->path_draft_point_count],
           0,
           sizeof(interaction->path_draft_points[interaction->path_draft_point_count]));
    interaction->path_draft_drag_active = 0u;
    interaction->path_draft_drag_point_index = 0u;
    if (interaction->path_draft_point_count == 0u) {
        path_draft_reset(interaction);
        return 1;
    }
    interaction->path_preview_sample_x = (uint32_t)interaction->path_draft_points[interaction->path_draft_point_count - 1u].x;
    interaction->path_preview_sample_y = (uint32_t)interaction->path_draft_points[interaction->path_draft_point_count - 1u].y;
    path_draft_preview_cache_invalidate(interaction);
    path_draft_preview_cache_refresh(interaction);
    return 1;
}

static int path_draft_add_point(VisualCanvasInteractionState *interaction, uint32_t sample_x, uint32_t sample_y) {
    uint16_t count;
    if (!interaction || sample_x > (uint32_t)INT32_MAX || sample_y > (uint32_t)INT32_MAX) {
        return 0;
    }
    count = interaction->path_draft_point_count;
    if (count > 0u) {
        const DrawingProgramPathPoint *last = &interaction->path_draft_points[count - 1u];
        if ((uint32_t)last->x == sample_x && (uint32_t)last->y == sample_y) {
            interaction->path_preview_sample_x = sample_x;
            interaction->path_preview_sample_y = sample_y;
            return 1;
        }
    }
    if (count >= (uint16_t)DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
        return 0;
    }
    memset(&interaction->path_draft_points[count], 0, sizeof(interaction->path_draft_points[count]));
    interaction->path_draft_points[count].x = (int32_t)sample_x;
    interaction->path_draft_points[count].y = (int32_t)sample_y;
    interaction->path_draft_point_count = (uint16_t)(count + 1u);
    interaction->path_draft_active = 1u;
    interaction->path_draft_closed = 0u;
    interaction->path_preview_sample_x = sample_x;
    interaction->path_preview_sample_y = sample_y;
    path_draft_preview_cache_invalidate(interaction);
    path_draft_preview_cache_refresh(interaction);
    return 1;
}

static void path_draft_update_drag_handle(VisualCanvasInteractionState *interaction,
                                          uint32_t sample_x,
                                          uint32_t sample_y) {
    DrawingProgramPathPoint *point;
    int32_t dx;
    int32_t dy;
    if (!interaction || !interaction->path_draft_drag_active ||
        interaction->path_draft_drag_point_index >= interaction->path_draft_point_count ||
        sample_x > (uint32_t)INT32_MAX || sample_y > (uint32_t)INT32_MAX) {
        return;
    }
    point = &interaction->path_draft_points[interaction->path_draft_drag_point_index];
    dx = (int32_t)sample_x - point->x;
    dy = (int32_t)sample_y - point->y;
    if (interaction->path_preview_sample_x == sample_x &&
        interaction->path_preview_sample_y == sample_y &&
        point->bezier_enabled &&
        point->handle_out_dx == dx &&
        point->handle_out_dy == dy &&
        point->handle_in_dx == -dx &&
        point->handle_in_dy == -dy) {
        return;
    }
    interaction->path_preview_sample_x = sample_x;
    interaction->path_preview_sample_y = sample_y;
    if (dx == 0 && dy == 0) {
        point->bezier_enabled = 0u;
        point->handle_linked = 0u;
        point->handle_out_dx = 0;
        point->handle_out_dy = 0;
        point->handle_in_dx = 0;
        point->handle_in_dy = 0;
        path_draft_preview_cache_invalidate(interaction);
        path_draft_preview_cache_refresh(interaction);
        return;
    }
    point->bezier_enabled = 1u;
    point->handle_linked = 1u;
    point->handle_out_dx = dx;
    point->handle_out_dy = dy;
    point->handle_in_dx = -dx;
    point->handle_in_dy = -dy;
    path_draft_preview_cache_invalidate(interaction);
    path_draft_preview_cache_refresh(interaction);
}

CoreResult path_draft_commit(DrawingProgramAppContext *ctx,
                             VisualCanvasInteractionState *interaction,
                             uint8_t closed) {
    DrawingProgramPathPayload payload;
    DrawingProgramObjectRecord style_seed;
    uint32_t active_layer_id = 0u;
    uint32_t object_id = 0u;
    uint8_t visible = 0u;
    uint8_t locked = 0u;
    DrawingProgramRasterSample color_value;
    CoreResult result;
    if (!ctx || !interaction) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid path draft commit request" };
    }
    if (!interaction->path_draft_active) {
        return core_result_ok();
    }
    if ((closed && interaction->path_draft_point_count < 3u) ||
        (!closed && interaction->path_draft_point_count < 2u)) {
        return core_result_ok();
    }
    result = drawing_program_runtime_orchestration_resolve_active_layer(
        ctx, &active_layer_id, 0, &visible, &locked);
    if (result.code != CORE_OK || active_layer_id == 0u || !visible || locked) {
        return core_result_ok();
    }
    memset(&payload, 0, sizeof(payload));
    payload.point_count = interaction->path_draft_point_count;
    payload.closed = closed ? 1u : 0u;
    memcpy(payload.points,
           interaction->path_draft_points,
           (size_t)payload.point_count * sizeof(payload.points[0]));
    memset(&style_seed, 0, sizeof(style_seed));
    color_value = drawing_program_ui_color_active_paint_sample_value(ctx);
    style_seed.layer_id = active_layer_id;
    style_seed.visible = 1u;
    style_seed.locked = 0u;
    style_seed.stroke_color_value = color_value;
    style_seed.fill_color_value = color_value;
    style_seed.stroke_width = ctx->ui.tool_shape_stroke_width;
    if (style_seed.stroke_width < 1u) {
        style_seed.stroke_width = 1u;
    } else if (style_seed.stroke_width > 16u) {
        style_seed.stroke_width = 16u;
    }
    /* Phase-14 path baseline is outline-only while interactive fill policies are deferred. */
    style_seed.style_mode = 0u;
    result = drawing_program_object_store_add_path(&ctx->object_store, &style_seed, &payload, &object_id);
    if (result.code != CORE_OK) {
        return result;
    }
    interaction->path_draft_closed = payload.closed;
    drawing_program_selection_reset(&ctx->selection);
    drawing_program_object_selection_replace_single(&ctx->object_selection, object_id);
    path_draft_reset(interaction);
    return core_result_ok();
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
    if (app->editor.active_tool == DRAWING_PROGRAM_TOOL_PATH) {
        canvas_interaction->path_draft_drag_active = 0u;
        canvas_interaction->path_draft_drag_point_index = 0u;
        canvas_interaction->drawing_active = 0u;
        canvas_interaction->shape_active = 0u;
        canvas_interaction->has_last_sample = 0u;
        return 1;
    }
    if (has_canvas_pane &&
        hooks->screen_to_canvas_sample(app, canvas_pane, release_x, release_y, &sample_x, &sample_y)) {
        if (selection_state->selecting) {
            selection_state->marquee_end_x = sample_x;
            selection_state->marquee_end_y = sample_y;
        }
        if (hooks->visual_transform_session_is_object_path_handle_move_active &&
            hooks->visual_transform_session_update_object_path_handle_move &&
            hooks->visual_transform_session_is_object_path_handle_move_active(canvas_interaction)) {
            (void)hooks->screen_to_canvas_sample_clamped(
                app, canvas_pane, release_x, release_y, &sample_x, &sample_y);
            hooks->visual_transform_session_update_object_path_handle_move(
                app, canvas_interaction, sample_x, sample_y);
        } else if (hooks->visual_transform_session_is_object_path_point_move_active &&
            hooks->visual_transform_session_update_object_path_point_move &&
            hooks->visual_transform_session_is_object_path_point_move_active(canvas_interaction)) {
            (void)hooks->screen_to_canvas_sample_clamped(
                app, canvas_pane, release_x, release_y, &sample_x, &sample_y);
            hooks->visual_transform_session_update_object_path_point_move(
                app, canvas_interaction, sample_x, sample_y, SDL_GetModState());
        } else if (hooks->visual_transform_session_is_object_move_active &&
                   hooks->visual_transform_session_update_object_move &&
                   hooks->visual_transform_session_is_object_move_active(canvas_interaction)) {
            (void)hooks->screen_to_canvas_sample_clamped(
                app, canvas_pane, release_x, release_y, &sample_x, &sample_y);
            hooks->visual_transform_session_update_object_move(
                app, canvas_interaction, sample_x, sample_y, SDL_GetModState());
        } else if (hooks->visual_transform_session_is_move_active &&
                   hooks->visual_transform_session_update_move &&
                   hooks->visual_transform_session_is_move_active(canvas_interaction) &&
                   selection_state->moving) {
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
            drawing_program_visual_input_object_selection_clear(app);
            drawing_program_selection_reset(selection_state);
            hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 0);
            return 1;
        }
        VisualMarqueeCommitMode mode =
            hooks->visual_marquee_commit_mode_clamp(canvas_interaction->marquee_commit_mode);
        (void)hooks->visual_selection_capture_from_marquee(app, selection_state, mode);
    }
    if (hooks->visual_transform_session_is_object_path_handle_move_active &&
        hooks->visual_transform_session_commit_object_path_handle_move &&
        hooks->visual_transform_session_is_object_path_handle_move_active(canvas_interaction)) {
        move_commit = hooks->visual_transform_session_commit_object_path_handle_move(app, canvas_interaction);
        if (move_commit.code != CORE_OK) {
            fprintf(stderr, "drawing_program: object path-handle commit failed: %s\n", move_commit.message);
        }
    } else if (hooks->visual_transform_session_is_object_path_point_move_active &&
        hooks->visual_transform_session_commit_object_path_point_move &&
        hooks->visual_transform_session_is_object_path_point_move_active(canvas_interaction)) {
        move_commit = hooks->visual_transform_session_commit_object_path_point_move(app, canvas_interaction);
        if (move_commit.code != CORE_OK) {
            fprintf(stderr, "drawing_program: object path-point commit failed: %s\n", move_commit.message);
        }
    } else if (hooks->visual_transform_session_is_move_active &&
               hooks->visual_transform_session_commit_move &&
               hooks->visual_transform_session_is_move_active(canvas_interaction) &&
               selection_state->moving) {
        move_commit = hooks->visual_transform_session_commit_move(app, canvas_interaction, selection_state);
        if (move_commit.code != CORE_OK) {
            fprintf(stderr, "drawing_program: selection move commit failed: %s\n", move_commit.message);
        }
    } else if (hooks->visual_transform_session_is_object_move_active &&
               hooks->visual_transform_session_commit_object_move &&
               hooks->visual_transform_session_is_object_move_active(canvas_interaction)) {
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
                                                      canvas_pane,
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
    if (app->editor.active_tool == DRAWING_PROGRAM_TOOL_PATH &&
        canvas_interaction->path_draft_active) {
        uint32_t sample_x = 0u;
        uint32_t sample_y = 0u;
        if (hooks->screen_to_canvas_sample(
                app, canvas_pane, panel_ui->mouse_x, panel_ui->mouse_y, &sample_x, &sample_y)) {
            path_draft_update_drag_handle(canvas_interaction, sample_x, sample_y);
            if (!canvas_interaction->path_draft_drag_active) {
                canvas_interaction->path_preview_sample_x = sample_x;
                canvas_interaction->path_preview_sample_y = sample_y;
            }
        }
    }
    if (hooks->visual_transform_session_is_object_path_handle_move_active &&
        hooks->visual_transform_session_update_object_path_handle_move &&
        hooks->visual_transform_session_is_object_path_handle_move_active(canvas_interaction) &&
        (app->editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE ||
         app->editor.active_tool == DRAWING_PROGRAM_TOOL_SELECT)) {
        uint32_t sample_x = 0u;
        uint32_t sample_y = 0u;
        if (hooks->screen_to_canvas_sample_clamped(
                app, canvas_pane, panel_ui->mouse_x, panel_ui->mouse_y, &sample_x, &sample_y)) {
            hooks->visual_transform_session_update_object_path_handle_move(
                app, canvas_interaction, sample_x, sample_y);
        }
    } else if (hooks->visual_transform_session_is_object_path_point_move_active &&
        hooks->visual_transform_session_update_object_path_point_move &&
        hooks->visual_transform_session_is_object_path_point_move_active(canvas_interaction) &&
        app->editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE) {
        uint32_t sample_x = 0u;
        uint32_t sample_y = 0u;
        if (hooks->screen_to_canvas_sample_clamped(
                app, canvas_pane, panel_ui->mouse_x, panel_ui->mouse_y, &sample_x, &sample_y)) {
            hooks->visual_transform_session_update_object_path_point_move(app,
                                                                          canvas_interaction,
                                                                          sample_x,
                                                                          sample_y,
                                                                          SDL_GetModState());
        }
    } else if (hooks->visual_transform_session_is_object_move_active &&
        hooks->visual_transform_session_update_object_move &&
        hooks->visual_transform_session_is_object_move_active(canvas_interaction) &&
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
    } else if (hooks->visual_transform_session_is_move_active &&
        hooks->visual_transform_session_update_move &&
        hooks->visual_transform_session_is_move_active(canvas_interaction) &&
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
        drawing_program_visual_input_handle_left_panel_click_payload(
            app, left_pane, click_x, click_y, selection_state, panel_ui, hooks);
        return 1;
    }
    if (down_action == DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_HANDLE_RIGHT_PANEL) {
        hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 0);
        drawing_program_visual_input_handle_right_panel_click_payload(
            app, right_pane, click_x, click_y, selection_state, panel_ui, hooks);
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
            uint16_t hit_point_index = 0u;
            uint8_t hit_handle_kind = (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_NONE;
            SDL_Keymod mods = SDL_GetModState();
            VisualMarqueeCommitMode select_mode =
                drawing_program_visual_input_resolve_select_commit_mode(app, mods);
            if (app->object_selection.count > 0u &&
                hooks->object_path_handle_hit_test_selected &&
                hooks->visual_transform_session_begin_object_path_handle_move &&
                hooks->object_path_handle_hit_test_selected(
                    app, sample_x, sample_y, &hit_object_id, &hit_point_index, &hit_handle_kind)) {
                const DrawingProgramObjectRecord *object =
                    drawing_program_object_store_get_by_id(&app->object_store, hit_object_id);
                if (object && hit_point_index < object->path_point_count) {
                    (void)drawing_program_object_selection_set_path_point(
                        &app->object_selection, hit_object_id, hit_point_index);
                    drawing_program_selection_reset(selection_state);
                    hooks->cancel_canvas_draw_and_shape(canvas_interaction);
                    hooks->cancel_selection_transient(selection_state);
                    hooks->visual_transform_session_begin_object_path_handle_move(
                        canvas_interaction,
                        hit_object_id,
                        hit_point_index,
                        hit_handle_kind,
                        &object->path_points[hit_point_index]);
                    return 1;
                }
            }
            if (app->object_selection.count > 0u &&
                hooks->object_path_point_hit_test_selected &&
                hooks->object_path_point_hit_test_selected(
                    app, sample_x, sample_y, &hit_object_id, &hit_point_index)) {
                (void)drawing_program_object_selection_set_path_point(
                    &app->object_selection, hit_object_id, hit_point_index);
                drawing_program_selection_reset(selection_state);
                hooks->cancel_canvas_draw_and_shape(canvas_interaction);
                hooks->cancel_selection_transient(selection_state);
                return 1;
            }
            if (drawing_program_visual_input_object_selection_hit_test(
                    app, sample_x, sample_y, &hit_object_id)) {
                if (select_mode == VISUAL_MARQUEE_COMMIT_SUBTRACT) {
                    drawing_program_visual_input_object_selection_remove(app, hit_object_id);
                } else if (select_mode == VISUAL_MARQUEE_COMMIT_ADD) {
                    drawing_program_visual_input_object_selection_add(app, hit_object_id);
                } else {
                    drawing_program_visual_input_object_selection_replace(app, hit_object_id);
                }
                /* Object-select mode is exclusive with pixel payload selection preview. */
                drawing_program_selection_reset(selection_state);
                hooks->cancel_canvas_draw_and_shape(canvas_interaction);
                hooks->cancel_selection_transient(selection_state);
                return 1;
            }
            if (select_mode == VISUAL_MARQUEE_COMMIT_REPLACE) {
                drawing_program_visual_input_object_selection_clear(app);
            }
            drawing_program_selection_begin_marquee(selection_state, sample_x, sample_y);
            hooks->cancel_canvas_draw_and_shape(canvas_interaction);
            canvas_interaction->marquee_commit_mode = (uint8_t)select_mode;
        } else if (app->editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE &&
                   app->object_selection.count > 0u &&
                   hooks->screen_to_canvas_sample_clamped(
                       app, canvas_pane, click_x, click_y, &sample_x, &sample_y)) {
            uint32_t hit_object_id = 0u;
            uint16_t hit_point_index = 0u;
            uint8_t hit_handle_kind = (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_NONE;
            SDL_Keymod mods = SDL_GetModState();
            int force_point_drag = ((mods & KMOD_ALT) != 0) ? 1 : 0;
            hooks->cancel_canvas_draw_and_shape(canvas_interaction);
            hooks->cancel_selection_transient(selection_state);
            if (hooks->object_path_handle_hit_test_selected &&
                hooks->visual_transform_session_begin_object_path_handle_move &&
                hooks->object_path_handle_hit_test_selected(
                    app, sample_x, sample_y, &hit_object_id, &hit_point_index, &hit_handle_kind)) {
                const DrawingProgramObjectRecord *object =
                    drawing_program_object_store_get_by_id(&app->object_store, hit_object_id);
                if (object && hit_point_index < object->path_point_count) {
                    hooks->visual_transform_session_begin_object_path_handle_move(
                        canvas_interaction,
                        hit_object_id,
                        hit_point_index,
                        hit_handle_kind,
                        &object->path_points[hit_point_index]);
                }
            } else if (hooks->object_path_point_hit_test_selected &&
                hooks->visual_transform_session_begin_object_path_point_move &&
                hooks->object_path_point_hit_test_selected(
                    app, sample_x, sample_y, &hit_object_id, &hit_point_index)) {
                hooks->visual_transform_session_begin_object_path_point_move(
                    canvas_interaction, hit_object_id, hit_point_index, sample_x, sample_y);
            } else if (!force_point_drag) {
                hooks->visual_transform_session_begin_object_move(canvas_interaction, sample_x, sample_y);
            }
        } else if (app->editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE &&
                   selection_state->has_payload &&
                   ((hooks->screen_to_canvas_sample(app, canvas_pane, click_x, click_y, &sample_x, &sample_y) &&
                     hooks->visual_selection_begin_move(selection_state, sample_x, sample_y)) ||
                    hooks->selection_move_handle_hit(app, canvas_pane, selection_state, click_x, click_y)) &&
                   hooks->screen_to_canvas_sample_clamped(
                       app, canvas_pane, click_x, click_y, &sample_x, &sample_y)) {
            hooks->cancel_canvas_draw_and_shape(canvas_interaction);
            hooks->visual_transform_session_begin_move(canvas_interaction, selection_state, sample_x, sample_y);
        } else if (app->editor.active_tool == DRAWING_PROGRAM_TOOL_PATH &&
                   hooks->screen_to_canvas_sample(app, canvas_pane, click_x, click_y, &sample_x, &sample_y)) {
            uint32_t hit_object_id = 0u;
            uint16_t insert_index = 0u;
            int32_t insert_x = 0;
            int32_t insert_y = 0;
            CoreResult insert_result;
            if (!canvas_interaction->path_draft_active &&
                app->object_selection.count > 0u &&
                hooks->object_path_edge_hit_test_selected &&
                hooks->apply_insert_object_path_point &&
                hooks->object_path_edge_hit_test_selected(
                    app, sample_x, sample_y, &hit_object_id, &insert_index, &insert_x, &insert_y)) {
                hooks->cancel_selection_transient(selection_state);
                hooks->cancel_canvas_draw_and_shape(canvas_interaction);
                insert_result = hooks->apply_insert_object_path_point(
                    app, hit_object_id, insert_index, insert_x, insert_y);
                if (insert_result.code == CORE_OK) {
                    (void)drawing_program_object_selection_set_path_point(
                        &app->object_selection, hit_object_id, insert_index);
                    drawing_program_selection_reset(selection_state);
                    return 1;
                }
                fprintf(stderr, "drawing_program: object path insert failed: %s\n", insert_result.message);
                return 1;
            }
            if (!canvas_interaction->path_draft_active &&
                app->object_selection.count > 0u &&
                hooks->apply_insert_object_path_point &&
                drawing_program_object_store_resolve_selected_open_path_append_target(
                    &app->object_store,
                    &app->document,
                    &app->object_selection,
                    sample_x,
                    sample_y,
                    &hit_object_id,
                    &insert_index).code == CORE_OK) {
                hooks->cancel_selection_transient(selection_state);
                hooks->cancel_canvas_draw_and_shape(canvas_interaction);
                insert_result = hooks->apply_insert_object_path_point(
                    app, hit_object_id, insert_index, (int32_t)sample_x, (int32_t)sample_y);
                if (insert_result.code == CORE_OK) {
                    (void)drawing_program_object_selection_set_path_point(
                        &app->object_selection, hit_object_id, insert_index);
                    drawing_program_selection_reset(selection_state);
                    return 1;
                }
                fprintf(stderr, "drawing_program: object path append failed: %s\n", insert_result.message);
                return 1;
            }
            if (canvas_interaction->path_draft_point_count == 0u) {
                drawing_program_visual_input_object_selection_clear(app);
                drawing_program_selection_reset(selection_state);
            }
            hooks->cancel_selection_transient(selection_state);
            canvas_interaction->drawing_active = 0u;
            canvas_interaction->shape_active = 0u;
            canvas_interaction->transform_active = 0u;
            canvas_interaction->transform_kind = (uint8_t)VISUAL_TRANSFORM_SESSION_NONE;
            canvas_interaction->object_move_active = 0u;
            canvas_interaction->move_axis_lock = 0u;
            canvas_interaction->has_last_sample = 0u;
            (void)path_draft_add_point(canvas_interaction, sample_x, sample_y);
            if (canvas_interaction->path_draft_point_count > 0u) {
                canvas_interaction->path_draft_drag_active = 1u;
                canvas_interaction->path_draft_drag_point_index =
                    (uint16_t)(canvas_interaction->path_draft_point_count - 1u);
            }
            return 1;
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
