#include "drawing_program/drawing_program_visual_panel_ui_state.h"

#define VISUAL_RIGHT_CANVAS_DELETE_CONFIRM_MIN_FRAMES 8u
#define VISUAL_RIGHT_CANVAS_DELETE_CONFIRM_MAX_FRAMES 180u

void drawing_program_visual_panel_ui_disarm_canvas_delete(VisualPanelUiState *ui) {
    if (!ui) {
        return;
    }
    ui->right_canvas_delete_confirm_pending = 0u;
    ui->right_canvas_delete_confirm_surface_id = 0u;
    ui->right_canvas_delete_confirm_armed_frame = 0u;
}

void drawing_program_visual_panel_ui_arm_canvas_delete(DrawingProgramAppContext *ctx, VisualPanelUiState *ui) {
    if (!ctx || !ui) {
        return;
    }
    const DrawingProgramTextureSurface *surface = drawing_program_texture_project_surface_at(
        &ctx->texture_project, ctx->texture_project.active_surface_index);
    if (!surface) {
        drawing_program_visual_panel_ui_disarm_canvas_delete(ui);
        return;
    }
    ui->right_canvas_delete_confirm_pending = 1u;
    ui->right_canvas_delete_confirm_surface_id = surface->surface_id;
    ui->right_canvas_delete_confirm_armed_frame = ctx->runtime.frame_counter;
}

int drawing_program_visual_panel_ui_canvas_delete_is_armed(const DrawingProgramAppContext *ctx,
                                                          const VisualPanelUiState *ui) {
    if (!ctx || !ui || !ui->right_canvas_delete_confirm_pending) {
        return 0;
    }
    const DrawingProgramTextureSurface *surface = drawing_program_texture_project_surface_at(
        &ctx->texture_project, ctx->texture_project.active_surface_index);
    return surface && ui->right_canvas_delete_confirm_surface_id == surface->surface_id;
}

int drawing_program_visual_panel_ui_canvas_delete_is_confirmable(const DrawingProgramAppContext *ctx,
                                                                 const VisualPanelUiState *ui) {
    uint64_t delta = 0u;
    if (!drawing_program_visual_panel_ui_canvas_delete_is_armed(ctx, ui) ||
        ctx->runtime.frame_counter < ui->right_canvas_delete_confirm_armed_frame) {
        return 0;
    }
    delta = ctx->runtime.frame_counter - ui->right_canvas_delete_confirm_armed_frame;
    return (delta >= VISUAL_RIGHT_CANVAS_DELETE_CONFIRM_MIN_FRAMES &&
            delta <= VISUAL_RIGHT_CANVAS_DELETE_CONFIRM_MAX_FRAMES)
               ? 1
               : 0;
}

int drawing_program_visual_panel_ui_canvas_delete_is_waiting_for_min_confirm_delay(
    const DrawingProgramAppContext *ctx,
    const VisualPanelUiState *ui) {
    uint64_t delta = 0u;
    if (!drawing_program_visual_panel_ui_canvas_delete_is_armed(ctx, ui) ||
        ctx->runtime.frame_counter <= ui->right_canvas_delete_confirm_armed_frame) {
        return 0;
    }
    delta = ctx->runtime.frame_counter - ui->right_canvas_delete_confirm_armed_frame;
    return (delta < VISUAL_RIGHT_CANVAS_DELETE_CONFIRM_MIN_FRAMES) ? 1 : 0;
}

void drawing_program_visual_panel_ui_disarm_reflection_center_pick(VisualPanelUiState *ui) {
    if (!ui) {
        return;
    }
    ui->right_canvas_reflection_center_pick_pending = 0u;
}

void drawing_program_visual_panel_ui_arm_reflection_center_pick(VisualPanelUiState *ui) {
    if (!ui) {
        return;
    }
    ui->right_canvas_reflection_center_pick_pending = 1u;
}

void drawing_program_visual_panel_ui_toggle_reflection_center_pick(VisualPanelUiState *ui) {
    if (!ui) {
        return;
    }
    if (ui->right_canvas_reflection_center_pick_pending) {
        drawing_program_visual_panel_ui_disarm_reflection_center_pick(ui);
    } else {
        drawing_program_visual_panel_ui_arm_reflection_center_pick(ui);
    }
}

void drawing_program_visual_panel_ui_disarm_right_canvas_transients(VisualPanelUiState *ui) {
    drawing_program_visual_panel_ui_disarm_canvas_delete(ui);
    drawing_program_visual_panel_ui_disarm_reflection_center_pick(ui);
}
