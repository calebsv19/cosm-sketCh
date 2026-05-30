#include "drawing_program/drawing_program_visual_input_panel_workspace_modes.h"

#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_texture_canvas_ops.h"
#include "drawing_program/drawing_program_visual_input_workspace_surface.h"
#include "drawing_program/drawing_program_visual_input_workspace_view.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_visual_right_panel_defs.h"

#define VISUAL_RIGHT_CANVAS_DELETE_CONFIRM_MIN_FRAMES 8u
#define VISUAL_RIGHT_CANVAS_DELETE_CONFIRM_MAX_FRAMES 180u

static void visual_right_panel_disarm_canvas_delete(VisualPanelUiState *ui) {
    if (!ui) {
        return;
    }
    ui->right_canvas_delete_confirm_pending = 0u;
    ui->right_canvas_delete_confirm_surface_index = 0u;
    ui->right_canvas_delete_confirm_armed_frame = 0u;
}

static void visual_right_panel_disarm_canvas_reflection_pick(VisualPanelUiState *ui) {
    if (!ui) {
        return;
    }
    ui->right_canvas_reflection_center_pick_pending = 0u;
}

static void visual_right_panel_arm_canvas_reflection_pick(VisualPanelUiState *ui) {
    if (!ui) {
        return;
    }
    ui->right_canvas_reflection_center_pick_pending = 1u;
}

static void visual_right_panel_arm_canvas_delete(DrawingProgramAppContext *ctx, VisualPanelUiState *ui) {
    if (!ctx || !ui) {
        return;
    }
    ui->right_canvas_delete_confirm_pending = 1u;
    ui->right_canvas_delete_confirm_surface_index = ctx->texture_project.active_surface_index;
    ui->right_canvas_delete_confirm_armed_frame = ctx->runtime.frame_counter;
}

static int visual_right_panel_canvas_delete_is_confirmable(const DrawingProgramAppContext *ctx,
                                                           const VisualPanelUiState *ui) {
    uint64_t delta = 0u;
    if (!ctx || !ui || !ui->right_canvas_delete_confirm_pending ||
        ui->right_canvas_delete_confirm_surface_index != ctx->texture_project.active_surface_index ||
        ctx->runtime.frame_counter < ui->right_canvas_delete_confirm_armed_frame) {
        return 0;
    }
    delta = ctx->runtime.frame_counter - ui->right_canvas_delete_confirm_armed_frame;
    return (delta >= VISUAL_RIGHT_CANVAS_DELETE_CONFIRM_MIN_FRAMES &&
            delta <= VISUAL_RIGHT_CANVAS_DELETE_CONFIRM_MAX_FRAMES)
               ? 1
               : 0;
}

void drawing_program_visual_input_disarm_right_canvas_workspace_modes(VisualPanelUiState *ui) {
    visual_right_panel_disarm_canvas_delete(ui);
    visual_right_panel_disarm_canvas_reflection_pick(ui);
}

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
        visual_right_panel_disarm_canvas_delete(ui);
        if (ui->right_canvas_reflection_center_pick_pending) {
            visual_right_panel_disarm_canvas_reflection_pick(ui);
        } else {
            visual_right_panel_arm_canvas_reflection_pick(ui);
        }
        return 1;
    }

    center_reset_button = right_canvas_center_reset_button_rect(rect, metrics);
    if (hooks->point_in_rect(center_reset_button, x, y)) {
        drawing_program_visual_input_disarm_right_canvas_workspace_modes(ui);
        drawing_program_canvas_reflection_reset_active_center(ctx);
        return 1;
    }

    reflector_add_button = right_canvas_reflector_add_button_rect(rect, metrics);
    if (hooks->point_in_rect(reflector_add_button, x, y)) {
        drawing_program_visual_input_disarm_right_canvas_workspace_modes(ui);
        (void)drawing_program_canvas_reflection_add_active_reflector(ctx, 1, 1);
        return 1;
    }

    reflector_cycle_button = right_canvas_reflector_cycle_button_rect(rect, metrics);
    if (hooks->point_in_rect(reflector_cycle_button, x, y)) {
        drawing_program_visual_input_disarm_right_canvas_workspace_modes(ui);
        (void)drawing_program_canvas_reflection_cycle_active_reflector(ctx, 1);
        return 1;
    }

    reflector_toggle_button = right_canvas_reflector_toggle_button_rect(rect, metrics);
    if (hooks->point_in_rect(reflector_toggle_button, x, y)) {
        drawing_program_visual_input_disarm_right_canvas_workspace_modes(ui);
        (void)drawing_program_canvas_reflection_toggle_active_reflector_enabled(ctx);
        return 1;
    }

    reflector_delete_button = right_canvas_reflector_delete_button_rect(rect, metrics);
    if (hooks->point_in_rect(reflector_delete_button, x, y)) {
        drawing_program_visual_input_disarm_right_canvas_workspace_modes(ui);
        (void)drawing_program_canvas_reflection_delete_active_reflector(ctx);
        return 1;
    }

    delete_canvas_button = right_canvas_delete_canvas_button_rect(rect, metrics);
    if (hooks->point_in_rect(delete_canvas_button, x, y)) {
        if (ctx->texture_project.surface_count <= 1u) {
            visual_right_panel_disarm_canvas_delete(ui);
            return 1;
        }
        if (visual_right_panel_canvas_delete_is_confirmable(ctx, ui)) {
            if (drawing_program_texture_canvas_delete_active(ctx, &surface_index).code == CORE_OK) {
                visual_right_panel_disarm_canvas_delete(ui);
                (void)drawing_program_visual_input_workspace_view_fit_surface(ctx, surface_index);
            }
            return 1;
        }
        if (ui->right_canvas_delete_confirm_pending &&
            ui->right_canvas_delete_confirm_surface_index == ctx->texture_project.active_surface_index &&
            ctx->runtime.frame_counter > ui->right_canvas_delete_confirm_armed_frame &&
            (ctx->runtime.frame_counter - ui->right_canvas_delete_confirm_armed_frame) <
                VISUAL_RIGHT_CANVAS_DELETE_CONFIRM_MIN_FRAMES) {
            return 1;
        }
        visual_right_panel_arm_canvas_delete(ctx, ui);
        visual_right_panel_disarm_canvas_reflection_pick(ui);
        return 1;
    }

    if (ui->right_canvas_delete_confirm_pending) {
        visual_right_panel_disarm_canvas_delete(ui);
    }

    surface_count = ctx->texture_project.surface_count;
    for (surface_index = 0u; surface_index < surface_count; ++surface_index) {
        SDL_Rect surface_row = right_canvas_surface_row_rect(rect, metrics, surface_index);
        if (hooks->point_in_rect(surface_row, x, y)) {
            SDL_Rect canvas_rect = {0, 0, 0, 0};
            visual_right_panel_disarm_canvas_reflection_pick(ui);
            if (drawing_program_visual_pane_rect_for_module_type(ctx, 1u, &canvas_rect)) {
                (void)drawing_program_visual_input_commit_workspace_surface(
                    ctx, canvas_rect, surface_index, 1u, NULL);
            }
            return 1;
        }
    }

    return 0;
}
