#include "drawing_program/drawing_program_visual_input_workspace_browser.h"

#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_texture_canvas_move.h"
#include "drawing_program/drawing_program_texture_canvas_resize.h"
#include "drawing_program/drawing_program_texture_workspace.h"
#include "drawing_program/drawing_program_visual_reflector_geometry.h"
#include "drawing_program/drawing_program_visual_input_workspace_surface.h"
#include "drawing_program/drawing_program_visual_panel_ui_state.h"

enum {
    DRAWING_PROGRAM_REFLECTOR_DRAG_NONE = 0,
    DRAWING_PROGRAM_REFLECTOR_DRAG_CENTER = 1,
    DRAWING_PROGRAM_REFLECTOR_DRAG_DIRECTION = 2
};

static int reflector_hit_radius(const VisualCanvasSheetMetrics *metrics) {
    int radius = 8;
    if (!metrics) {
        return radius;
    }
    radius = (metrics->pixel_size >= 6.0f) ? 10 : 8;
    return radius;
}

static int point_hits_handle(SDL_Point handle, int click_x, int click_y, int radius) {
    int dx = handle.x - click_x;
    int dy = handle.y - click_y;
    return ((dx * dx) + (dy * dy)) <= (radius * radius);
}

static int begin_reflector_drag_if_hit(int click_x,
                                       int click_y,
                                       SDL_Rect canvas_pane,
                                       DrawingProgramAppContext *app,
                                       VisualCanvasInteractionState *canvas_interaction,
                                       DrawingProgramSelectionState *selection_state,
                                       const DrawingProgramVisualInputHandlersHooks *hooks) {
    VisualCanvasSheetMetrics metrics;
    const DrawingProgramReflectionState *state = 0;
    SDL_Point anchor_handle;
    SDL_Point direction_handle;
    uint32_t i;
    int radius;
    if (!app || !canvas_interaction || !selection_state || !hooks ||
        !drawing_program_texture_workspace_surface_sheet_metrics(
            app, canvas_pane, app->texture_project.active_surface_index, &metrics)) {
        return 0;
    }
    state = drawing_program_canvas_reflection_active_state(app);
    if (!state || state->reflector_count == 0u) {
        return 0;
    }
    radius = reflector_hit_radius(&metrics);
    for (i = 0u; i < state->reflector_count && i < DRAWING_PROGRAM_REFLECTION_REFLECTOR_CAPACITY; ++i) {
        if (!drawing_program_visual_reflector_screen_handles(
                &metrics, &state->reflectors[i], &anchor_handle, &direction_handle)) {
            continue;
        }
        if (point_hits_handle(direction_handle, click_x, click_y, radius)) {
            hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 0);
            while (drawing_program_canvas_reflection_active_state(app) &&
                   drawing_program_canvas_reflection_active_state(app)->active_reflector_index != i) {
                if (!drawing_program_canvas_reflection_cycle_active_reflector(app, 1)) {
                    break;
                }
            }
            canvas_interaction->reflector_drag_active = 1u;
            canvas_interaction->reflector_drag_kind = DRAWING_PROGRAM_REFLECTOR_DRAG_DIRECTION;
            canvas_interaction->reflector_drag_index = i;
            return 1;
        }
    }
    if (!drawing_program_visual_reflector_screen_handles(
            &metrics,
            &state->reflectors[state->active_reflector_index < state->reflector_count
                                   ? state->active_reflector_index
                                   : 0u],
            &anchor_handle,
            &direction_handle)) {
        return 0;
    }
    if (!point_hits_handle(anchor_handle, click_x, click_y, radius)) {
        return 0;
    }
    hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 0);
    canvas_interaction->reflector_drag_active = 1u;
    canvas_interaction->reflector_drag_kind = DRAWING_PROGRAM_REFLECTOR_DRAG_CENTER;
    canvas_interaction->reflector_drag_index = state->active_reflector_index;
    return 1;
}

int drawing_program_visual_input_handle_workspace_canvas_click(
    int click_x,
    int click_y,
    SDL_Rect canvas_pane,
    DrawingProgramAppContext *app,
    VisualCanvasInteractionState *canvas_interaction,
    DrawingProgramSelectionState *selection_state,
    VisualPanelUiState *panel_ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
    uint32_t clicked_surface_index = 0u;
    uint32_t resize_surface_index = 0u;
    uint32_t sample_x = 0u;
    uint32_t sample_y = 0u;
    if (!app || !canvas_interaction || !selection_state || !panel_ui || !hooks) {
        return 0;
    }
    if (panel_ui->right_canvas_reflection_center_pick_pending) {
        if (drawing_program_texture_workspace_hit_test_surface(
                app, canvas_pane, click_x, click_y, &clicked_surface_index) &&
            drawing_program_visual_input_commit_workspace_surface(
                app, canvas_pane, clicked_surface_index, 0u, NULL).code != CORE_OK) {
            return 1;
        }
        if (hooks->screen_to_canvas_sample &&
            hooks->screen_to_canvas_sample(app, canvas_pane, click_x, click_y, &sample_x, &sample_y)) {
            hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 0);
            (void)drawing_program_canvas_reflection_set_active_center(app, sample_x, sample_y);
        }
        drawing_program_visual_panel_ui_disarm_reflection_center_pick(panel_ui);
        return 1;
    }
    if (begin_reflector_drag_if_hit(
            click_x, click_y, canvas_pane, app, canvas_interaction, selection_state, hooks)) {
        return 1;
    }
    if (app->ui.canvas_control_mode == (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT &&
        drawing_program_texture_canvas_resize_hit_test_handle(
            app, canvas_pane, click_x, click_y, &resize_surface_index)) {
        hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 0);
        if (drawing_program_visual_input_commit_workspace_surface(
                app, canvas_pane, resize_surface_index, 0u, NULL).code != CORE_OK) {
            return 1;
        }
        (void)drawing_program_texture_canvas_resize_begin(
            app, canvas_pane, canvas_interaction, resize_surface_index, click_x, click_y);
        return 1;
    }
    if (drawing_program_texture_workspace_hit_test_surface(
            app, canvas_pane, click_x, click_y, &clicked_surface_index) &&
        (app->ui.canvas_control_mode == (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT ||
         clicked_surface_index != app->texture_project.active_surface_index)) {
        uint8_t surface_ready = 0u;
        const uint8_t fit_after_commit =
            app->ui.canvas_control_mode == (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT ? 0u : 1u;
        hooks->cancel_all_transient_interactions(app, canvas_interaction, selection_state, 0);
        (void)drawing_program_visual_input_commit_workspace_surface(
            app, canvas_pane, clicked_surface_index, fit_after_commit, &surface_ready);
        if (surface_ready &&
            app->ui.canvas_control_mode == (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT) {
            (void)drawing_program_texture_canvas_move_begin(
                app, canvas_pane, canvas_interaction, clicked_surface_index, click_x, click_y);
        }
        return 1;
    }
    return 0;
}
