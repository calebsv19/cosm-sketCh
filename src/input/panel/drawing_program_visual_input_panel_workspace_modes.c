#include "drawing_program/drawing_program_visual_input_panel_workspace_modes.h"

#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_texture_canvas_ops.h"
#include "drawing_program/drawing_program_visual_input_workspace_surface.h"
#include "drawing_program/drawing_program_visual_input_workspace_view.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_visual_panel_ui_state.h"
#include "drawing_program/drawing_program_visual_right_panel_defs.h"

int drawing_program_visual_input_handle_right_canvas_workspace_mode_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    VisualPaneLayoutMetrics metrics,
    int x,
    int y,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
    SDL_Rect center_pick_button;
    SDL_Rect center_reset_button;
    SDL_Rect reflector_add_button;
    SDL_Rect reflector_cycle_button;
    SDL_Rect reflector_toggle_button;
    SDL_Rect reflector_delete_button;
    SDL_Rect delete_canvas_button;
    uint32_t surface_count = 0u;
    uint32_t surface_index = 0u;
    if (!ctx || !ui || !hooks || !hooks->point_in_rect) {
        return 0;
    }

    center_pick_button = right_canvas_center_pick_button_rect(rect, metrics);
    if (hooks->point_in_rect(center_pick_button, x, y)) {
        drawing_program_visual_panel_ui_disarm_canvas_delete(ui);
        drawing_program_visual_panel_ui_toggle_reflection_center_pick(ui);
        return 1;
    }

    center_reset_button = right_canvas_center_reset_button_rect(rect, metrics);
    if (hooks->point_in_rect(center_reset_button, x, y)) {
        drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
        drawing_program_canvas_reflection_reset_active_center(ctx);
        return 1;
    }

    reflector_add_button = right_canvas_reflector_add_button_rect(rect, metrics);
    if (hooks->point_in_rect(reflector_add_button, x, y)) {
        drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
        (void)drawing_program_canvas_reflection_add_active_reflector(ctx, 1, 1);
        return 1;
    }

    reflector_cycle_button = right_canvas_reflector_cycle_button_rect(rect, metrics);
    if (hooks->point_in_rect(reflector_cycle_button, x, y)) {
        drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
        (void)drawing_program_canvas_reflection_cycle_active_reflector(ctx, 1);
        return 1;
    }

    reflector_toggle_button = right_canvas_reflector_toggle_button_rect(rect, metrics);
    if (hooks->point_in_rect(reflector_toggle_button, x, y)) {
        drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
        (void)drawing_program_canvas_reflection_toggle_active_reflector_enabled(ctx);
        return 1;
    }

    reflector_delete_button = right_canvas_reflector_delete_button_rect(rect, metrics);
    if (hooks->point_in_rect(reflector_delete_button, x, y)) {
        drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
        (void)drawing_program_canvas_reflection_delete_active_reflector(ctx);
        return 1;
    }

    delete_canvas_button = right_canvas_delete_canvas_button_rect(rect, metrics);
    if (hooks->point_in_rect(delete_canvas_button, x, y)) {
        if (ctx->texture_project.surface_count <= 1u) {
            drawing_program_visual_panel_ui_disarm_canvas_delete(ui);
            return 1;
        }
        if (drawing_program_visual_panel_ui_canvas_delete_is_confirmable(ctx, ui)) {
            if (drawing_program_texture_canvas_delete_active(ctx, &surface_index).code == CORE_OK) {
                drawing_program_visual_panel_ui_disarm_canvas_delete(ui);
                (void)drawing_program_visual_input_workspace_view_fit_surface(ctx, surface_index);
            }
            return 1;
        }
        if (drawing_program_visual_panel_ui_canvas_delete_is_waiting_for_min_confirm_delay(ctx, ui)) {
            return 1;
        }
        drawing_program_visual_panel_ui_arm_canvas_delete(ctx, ui);
        drawing_program_visual_panel_ui_disarm_reflection_center_pick(ui);
        return 1;
    }

    if (ui->right_canvas_delete_confirm_pending) {
        drawing_program_visual_panel_ui_disarm_canvas_delete(ui);
    }

    surface_count = ctx->texture_project.surface_count;
    for (surface_index = 0u; surface_index < surface_count; ++surface_index) {
        SDL_Rect surface_row = right_canvas_surface_row_rect(rect, metrics, surface_index);
        if (hooks->point_in_rect(surface_row, x, y)) {
            SDL_Rect canvas_rect = {0, 0, 0, 0};
            drawing_program_visual_panel_ui_disarm_reflection_center_pick(ui);
            if (drawing_program_visual_pane_rect_for_module_type(ctx, 1u, &canvas_rect)) {
                (void)drawing_program_visual_input_commit_workspace_surface(
                    ctx, canvas_rect, surface_index, 1u, NULL);
            }
            return 1;
        }
    }

    return 0;
}
