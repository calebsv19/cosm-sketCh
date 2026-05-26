#include "drawing_program/drawing_program_visual_input_workspace_browser.h"

#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_texture_canvas_move.h"
#include "drawing_program/drawing_program_texture_canvas_resize.h"
#include "drawing_program/drawing_program_texture_workspace.h"
#include "drawing_program/drawing_program_visual_input_workspace_surface.h"

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
        panel_ui->right_canvas_reflection_center_pick_pending = 0u;
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
