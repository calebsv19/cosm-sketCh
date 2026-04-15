#include "drawing_program/drawing_program_visual_transform_ops.h"

#include <stdlib.h>

#include "drawing_program/drawing_program_selection.h"

void drawing_program_visual_cancel_canvas_draw_and_shape(VisualCanvasInteractionState *interaction) {
    if (!interaction) {
        return;
    }
    interaction->drawing_active = 0u;
    interaction->shape_active = 0u;
    interaction->transform_active = 0u;
    interaction->transform_kind = (uint8_t)VISUAL_TRANSFORM_SESSION_NONE;
    interaction->move_axis_lock = 0u;
    interaction->marquee_commit_mode = (uint8_t)VISUAL_MARQUEE_COMMIT_REPLACE;
    interaction->has_last_sample = 0u;
    interaction->object_move_active = 0u;
    interaction->object_move_anchor_sample_x = 0u;
    interaction->object_move_anchor_sample_y = 0u;
    interaction->object_move_offset_x = 0;
    interaction->object_move_offset_y = 0;
}

void drawing_program_visual_cancel_selection_transient(VisualSelectionState *selection) {
    drawing_program_selection_cancel_transient(selection);
}

void drawing_program_visual_apply_selection_move_axis_lock(VisualSelectionState *selection,
                                                           VisualCanvasInteractionState *interaction,
                                                           SDL_Keymod mods) {
    int32_t dx;
    int32_t dy;
    int lock_shift;
    if (!selection || !interaction || !selection->moving) {
        return;
    }
    lock_shift = (mods & KMOD_SHIFT) ? 1 : 0;
    if (!lock_shift) {
        interaction->move_axis_lock = 0u;
        return;
    }
    dx = selection->offset_x - selection->move_anchor_offset_x;
    dy = selection->offset_y - selection->move_anchor_offset_y;
    if (interaction->move_axis_lock == 0u) {
        interaction->move_axis_lock = (abs(dx) >= abs(dy)) ? 1u : 2u;
    }
    if (interaction->move_axis_lock == 1u) {
        selection->offset_y = selection->move_anchor_offset_y;
    } else if (interaction->move_axis_lock == 2u) {
        selection->offset_x = selection->move_anchor_offset_x;
    }
}

void drawing_program_visual_apply_selection_move_canvas_bounds(const DrawingProgramDocument *document,
                                                               VisualSelectionState *selection) {
    int32_t min_offset_x;
    int32_t max_offset_x;
    int32_t min_offset_y;
    int32_t max_offset_y;
    if (!document || !selection || !selection->has_payload) {
        return;
    }
    if (selection->width == 0u || selection->height == 0u ||
        document->raster_width == 0u || document->raster_height == 0u) {
        selection->offset_x = 0;
        selection->offset_y = 0;
        return;
    }
    min_offset_x = -(int32_t)selection->origin_x;
    min_offset_y = -(int32_t)selection->origin_y;
    max_offset_x = (int32_t)document->raster_width - (int32_t)(selection->origin_x + selection->width);
    max_offset_y = (int32_t)document->raster_height - (int32_t)(selection->origin_y + selection->height);
    if (selection->offset_x < min_offset_x) {
        selection->offset_x = min_offset_x;
    }
    if (selection->offset_x > max_offset_x) {
        selection->offset_x = max_offset_x;
    }
    if (selection->offset_y < min_offset_y) {
        selection->offset_y = min_offset_y;
    }
    if (selection->offset_y > max_offset_y) {
        selection->offset_y = max_offset_y;
    }
}

void drawing_program_visual_transform_session_reset(VisualCanvasInteractionState *interaction) {
    if (!interaction) {
        return;
    }
    interaction->transform_active = 0u;
    interaction->transform_kind = (uint8_t)VISUAL_TRANSFORM_SESSION_NONE;
    interaction->move_axis_lock = 0u;
    interaction->object_move_active = 0u;
    interaction->object_move_offset_x = 0;
    interaction->object_move_offset_y = 0;
}

int drawing_program_visual_transform_session_is_move_active(const VisualCanvasInteractionState *interaction) {
    if (!interaction) {
        return 0;
    }
    return interaction->transform_active &&
                   interaction->transform_kind == (uint8_t)VISUAL_TRANSFORM_SESSION_MOVE
               ? 1
               : 0;
}

int drawing_program_visual_transform_session_is_object_move_active(const VisualCanvasInteractionState *interaction) {
    if (!interaction) {
        return 0;
    }
    return drawing_program_visual_transform_session_is_move_active(interaction) && interaction->object_move_active ? 1 : 0;
}

void drawing_program_visual_transform_session_begin_move(VisualCanvasInteractionState *interaction,
                                                         VisualSelectionState *selection,
                                                         uint32_t sample_x,
                                                         uint32_t sample_y) {
    if (!interaction || !selection) {
        return;
    }
    drawing_program_selection_begin_move_tracking(selection, sample_x, sample_y);
    interaction->transform_active = 1u;
    interaction->transform_kind = (uint8_t)VISUAL_TRANSFORM_SESSION_MOVE;
    interaction->move_axis_lock = 0u;
    interaction->object_move_active = 0u;
    interaction->object_move_offset_x = 0;
    interaction->object_move_offset_y = 0;
}

void drawing_program_visual_transform_session_update_move(const DrawingProgramAppContext *ctx,
                                                          VisualCanvasInteractionState *interaction,
                                                          VisualSelectionState *selection,
                                                          uint32_t sample_x,
                                                          uint32_t sample_y,
                                                          SDL_Keymod mods) {
    if (!ctx || !selection || !drawing_program_visual_transform_session_is_move_active(interaction) || !selection->moving) {
        return;
    }
    drawing_program_selection_update_move_offset(selection, sample_x, sample_y);
    drawing_program_visual_apply_selection_move_axis_lock(selection, interaction, mods);
    drawing_program_visual_apply_selection_move_canvas_bounds(&ctx->document, selection);
}

CoreResult drawing_program_visual_transform_session_commit_move(DrawingProgramAppContext *ctx,
                                                                VisualCanvasInteractionState *interaction,
                                                                VisualSelectionState *selection,
                                                                const DrawingProgramVisualTransformOpsHooks *hooks) {
    CoreResult move_commit;
    if (!ctx || !selection || !hooks || !hooks->visual_selection_commit_move ||
        !drawing_program_visual_transform_session_is_move_active(interaction)) {
        return core_result_ok();
    }
    move_commit = hooks->visual_selection_commit_move(ctx, selection);
    if (move_commit.code != CORE_OK) {
        selection->moving = 0u;
        selection->offset_x = 0;
        selection->offset_y = 0;
    }
    drawing_program_visual_transform_session_reset(interaction);
    return move_commit;
}

CoreResult drawing_program_visual_transform_session_nudge_move(DrawingProgramAppContext *ctx,
                                                               VisualCanvasInteractionState *interaction,
                                                               VisualSelectionState *selection,
                                                               int32_t dx,
                                                               int32_t dy,
                                                               const DrawingProgramVisualTransformOpsHooks *hooks) {
    uint32_t anchor_x = 0u;
    uint32_t anchor_y = 0u;
    if (!ctx || !selection || !selection->has_payload) {
        return core_result_ok();
    }
    anchor_x = selection->origin_x;
    anchor_y = selection->origin_y;
    drawing_program_visual_transform_session_begin_move(interaction, selection, anchor_x, anchor_y);
    selection->offset_x = dx;
    selection->offset_y = dy;
    drawing_program_visual_apply_selection_move_canvas_bounds(&ctx->document, selection);
    return drawing_program_visual_transform_session_commit_move(ctx, interaction, selection, hooks);
}

static void drawing_program_visual_apply_object_move_axis_lock(VisualCanvasInteractionState *interaction,
                                                               SDL_Keymod mods) {
    int lock_shift;
    if (!interaction) {
        return;
    }
    lock_shift = (mods & KMOD_SHIFT) ? 1 : 0;
    if (!lock_shift) {
        interaction->move_axis_lock = 0u;
        return;
    }
    if (interaction->move_axis_lock == 0u) {
        interaction->move_axis_lock =
            (abs(interaction->object_move_offset_x) >= abs(interaction->object_move_offset_y)) ? 1u : 2u;
    }
    if (interaction->move_axis_lock == 1u) {
        interaction->object_move_offset_y = 0;
    } else if (interaction->move_axis_lock == 2u) {
        interaction->object_move_offset_x = 0;
    }
}

void drawing_program_visual_transform_session_begin_object_move(VisualCanvasInteractionState *interaction,
                                                                uint32_t sample_x,
                                                                uint32_t sample_y) {
    if (!interaction) {
        return;
    }
    interaction->transform_active = 1u;
    interaction->transform_kind = (uint8_t)VISUAL_TRANSFORM_SESSION_MOVE;
    interaction->object_move_active = 1u;
    interaction->move_axis_lock = 0u;
    interaction->object_move_anchor_sample_x = sample_x;
    interaction->object_move_anchor_sample_y = sample_y;
    interaction->object_move_offset_x = 0;
    interaction->object_move_offset_y = 0;
}

void drawing_program_visual_transform_session_update_object_move(const DrawingProgramAppContext *ctx,
                                                                 VisualCanvasInteractionState *interaction,
                                                                 uint32_t sample_x,
                                                                 uint32_t sample_y,
                                                                 SDL_Keymod mods) {
    CoreResult result;
    int32_t clamped_dx = 0;
    int32_t clamped_dy = 0;
    if (!ctx || !interaction || !drawing_program_visual_transform_session_is_object_move_active(interaction)) {
        return;
    }
    interaction->object_move_offset_x = (int32_t)sample_x - (int32_t)interaction->object_move_anchor_sample_x;
    interaction->object_move_offset_y = (int32_t)sample_y - (int32_t)interaction->object_move_anchor_sample_y;
    drawing_program_visual_apply_object_move_axis_lock(interaction, mods);
    result = drawing_program_object_store_clamp_selection_delta(&ctx->object_store,
                                                                &ctx->document,
                                                                &ctx->object_selection,
                                                                interaction->object_move_offset_x,
                                                                interaction->object_move_offset_y,
                                                                &clamped_dx,
                                                                &clamped_dy);
    if (result.code == CORE_OK || result.code == CORE_ERR_NOT_FOUND) {
        interaction->object_move_offset_x = clamped_dx;
        interaction->object_move_offset_y = clamped_dy;
    }
}

CoreResult drawing_program_visual_transform_session_commit_object_move(DrawingProgramAppContext *ctx,
                                                                       VisualCanvasInteractionState *interaction,
                                                                       const DrawingProgramVisualTransformOpsHooks *hooks) {
    CoreResult result;
    if (!ctx || !interaction || !hooks || !hooks->visual_object_commit_move ||
        !drawing_program_visual_transform_session_is_object_move_active(interaction)) {
        return core_result_ok();
    }
    result = hooks->visual_object_commit_move(
        ctx, interaction->object_move_offset_x, interaction->object_move_offset_y);
    drawing_program_visual_transform_session_reset(interaction);
    return result;
}

CoreResult drawing_program_visual_transform_session_nudge_object_move(DrawingProgramAppContext *ctx,
                                                                      VisualCanvasInteractionState *interaction,
                                                                      int32_t dx,
                                                                      int32_t dy,
                                                                      const DrawingProgramVisualTransformOpsHooks *hooks) {
    if (!ctx || !interaction || !hooks || !hooks->visual_object_commit_move) {
        return core_result_ok();
    }
    drawing_program_visual_transform_session_begin_object_move(interaction, 0u, 0u);
    interaction->object_move_offset_x = dx;
    interaction->object_move_offset_y = dy;
    return drawing_program_visual_transform_session_commit_object_move(ctx, interaction, hooks);
}

void drawing_program_visual_begin_canvas_history_group(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    (void)drawing_program_history_begin_group(&ctx->history);
}

void drawing_program_visual_end_canvas_history_group(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    (void)drawing_program_history_end_group(&ctx->history);
}

void drawing_program_visual_cancel_all_transient_interactions(DrawingProgramAppContext *ctx,
                                                              VisualCanvasInteractionState *interaction,
                                                              VisualSelectionState *selection,
                                                              int clear_pan_state) {
    drawing_program_visual_end_canvas_history_group(ctx);
    drawing_program_visual_cancel_canvas_draw_and_shape(interaction);
    drawing_program_visual_cancel_selection_transient(selection);
    if (clear_pan_state && interaction) {
        interaction->panning_active = 0u;
    }
    drawing_program_visual_transform_session_reset(interaction);
}
