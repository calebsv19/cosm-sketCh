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
