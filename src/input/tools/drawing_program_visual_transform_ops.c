#include "drawing_program/drawing_program_visual_transform_ops.h"

#include <stdlib.h>
#include <string.h>

#include "drawing_program/drawing_program_selection.h"
#include "drawing_program/drawing_program_visual_canvas_stroke_ops.h"

void drawing_program_visual_cancel_canvas_draw_and_shape(VisualCanvasInteractionState *interaction) {
    if (!interaction) {
        return;
    }
    interaction->drawing_active = 0u;
    interaction->shape_active = 0u;
    interaction->path_draft_active = 0u;
    interaction->path_draft_closed = 0u;
    interaction->path_draft_drag_active = 0u;
    interaction->transform_active = 0u;
    interaction->transform_kind = (uint8_t)VISUAL_TRANSFORM_SESSION_NONE;
    interaction->move_axis_lock = 0u;
    interaction->marquee_commit_mode = (uint8_t)VISUAL_MARQUEE_COMMIT_REPLACE;
    interaction->has_last_sample = 0u;
    interaction->direct_stroke_group_step_count = 0u;
    interaction->direct_stroke_history_layer_id = 0u;
    interaction->direct_stroke_pending_delta_count = 0u;
    interaction->object_move_active = 0u;
    interaction->object_move_anchor_sample_x = 0u;
    interaction->object_move_anchor_sample_y = 0u;
    interaction->object_move_offset_x = 0;
    interaction->object_move_offset_y = 0;
    interaction->object_path_point_move_active = 0u;
    interaction->object_path_point_index = 0u;
    interaction->object_path_point_object_id = 0u;
    interaction->object_path_point_anchor_sample_x = 0u;
    interaction->object_path_point_anchor_sample_y = 0u;
    interaction->object_path_point_offset_x = 0;
    interaction->object_path_point_offset_y = 0;
    interaction->object_path_handle_move_active = 0u;
    interaction->object_path_handle_kind = (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_NONE;
    interaction->object_path_handle_point_index = 0u;
    interaction->object_path_handle_object_id = 0u;
    interaction->object_path_handle_dx = 0;
    interaction->object_path_handle_dy = 0;
    interaction->canvas_resize_active = 0u;
    interaction->canvas_resize_surface_index = 0u;
    interaction->canvas_resize_start_logical_width = 0u;
    interaction->canvas_resize_start_logical_height = 0u;
    interaction->canvas_resize_sample_density = 0u;
    interaction->canvas_resize_pixels_per_logical = 0.0f;
    interaction->canvas_resize_anchor_mouse_x = 0;
    interaction->canvas_resize_anchor_mouse_y = 0;
    interaction->canvas_move_active = 0u;
    interaction->canvas_move_surface_index = 0u;
    interaction->canvas_move_start_offset_x = 0.0f;
    interaction->canvas_move_start_offset_y = 0.0f;
    interaction->canvas_move_zoom = 0.0f;
    interaction->canvas_move_anchor_mouse_x = 0;
    interaction->canvas_move_anchor_mouse_y = 0;
    interaction->path_preview_sample_x = 0u;
    interaction->path_preview_sample_y = 0u;
    interaction->path_draft_point_count = 0u;
    interaction->path_draft_drag_point_index = 0u;
    interaction->path_preview_flatten_dirty = 0u;
    interaction->path_preview_flatten_valid = 0u;
    interaction->path_preview_flatten_point_count = 0u;
    memset(interaction->path_draft_points, 0, sizeof(interaction->path_draft_points));
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
    interaction->object_path_point_move_active = 0u;
    interaction->object_path_point_index = 0u;
    interaction->object_path_point_object_id = 0u;
    interaction->object_path_point_offset_x = 0;
    interaction->object_path_point_offset_y = 0;
    interaction->object_path_handle_move_active = 0u;
    interaction->object_path_handle_kind = (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_NONE;
    interaction->object_path_handle_point_index = 0u;
    interaction->object_path_handle_object_id = 0u;
    interaction->object_path_handle_dx = 0;
    interaction->object_path_handle_dy = 0;
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

int drawing_program_visual_transform_session_is_object_path_point_move_active(
    const VisualCanvasInteractionState *interaction) {
    if (!interaction) {
        return 0;
    }
    return drawing_program_visual_transform_session_is_move_active(interaction) &&
                   interaction->object_path_point_move_active
               ? 1
               : 0;
}

int drawing_program_visual_transform_session_is_object_path_handle_move_active(
    const VisualCanvasInteractionState *interaction) {
    if (!interaction) {
        return 0;
    }
    return drawing_program_visual_transform_session_is_move_active(interaction) &&
                   interaction->object_path_handle_move_active
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
    interaction->object_move_active = 0u;
    interaction->object_move_offset_x = 0;
    interaction->object_move_offset_y = 0;
    interaction->object_path_point_move_active = 0u;
    interaction->object_path_point_index = 0u;
    interaction->object_path_point_object_id = 0u;
    interaction->object_path_point_offset_x = 0;
    interaction->object_path_point_offset_y = 0;
    interaction->object_path_handle_move_active = 0u;
    interaction->object_path_handle_kind = (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_NONE;
    interaction->object_path_handle_point_index = 0u;
    interaction->object_path_handle_object_id = 0u;
    interaction->object_path_handle_dx = 0;
    interaction->object_path_handle_dy = 0;
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
    interaction->object_path_point_move_active = 0u;
    interaction->object_path_point_index = 0u;
    interaction->object_path_point_object_id = 0u;
    interaction->object_path_point_offset_x = 0;
    interaction->object_path_point_offset_y = 0;
    interaction->object_path_handle_move_active = 0u;
    interaction->object_path_handle_kind = (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_NONE;
    interaction->object_path_handle_point_index = 0u;
    interaction->object_path_handle_object_id = 0u;
    interaction->object_path_handle_dx = 0;
    interaction->object_path_handle_dy = 0;
}

void drawing_program_visual_transform_session_begin_object_path_handle_move(
    VisualCanvasInteractionState *interaction,
    uint32_t object_id,
    uint16_t point_index,
    uint8_t handle_kind,
    const DrawingProgramPathPoint *point) {
    if (!interaction || !point || object_id == 0u ||
        (handle_kind != (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_IN &&
         handle_kind != (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_OUT)) {
        return;
    }
    interaction->transform_active = 1u;
    interaction->transform_kind = (uint8_t)VISUAL_TRANSFORM_SESSION_MOVE;
    interaction->object_move_active = 0u;
    interaction->object_move_offset_x = 0;
    interaction->object_move_offset_y = 0;
    interaction->object_path_point_move_active = 0u;
    interaction->object_path_point_index = 0u;
    interaction->object_path_point_object_id = 0u;
    interaction->object_path_point_offset_x = 0;
    interaction->object_path_point_offset_y = 0;
    interaction->object_path_handle_move_active = 1u;
    interaction->object_path_handle_kind = handle_kind;
    interaction->object_path_handle_point_index = point_index;
    interaction->object_path_handle_object_id = object_id;
    interaction->object_path_handle_dx =
        (handle_kind == (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_IN) ? point->handle_in_dx : point->handle_out_dx;
    interaction->object_path_handle_dy =
        (handle_kind == (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_IN) ? point->handle_in_dy : point->handle_out_dy;
    interaction->move_axis_lock = 0u;
}

void drawing_program_visual_transform_session_begin_object_path_point_move(VisualCanvasInteractionState *interaction,
                                                                           uint32_t object_id,
                                                                           uint16_t point_index,
                                                                           uint32_t sample_x,
                                                                           uint32_t sample_y) {
    if (!interaction || object_id == 0u) {
        return;
    }
    interaction->transform_active = 1u;
    interaction->transform_kind = (uint8_t)VISUAL_TRANSFORM_SESSION_MOVE;
    interaction->object_move_active = 0u;
    interaction->object_move_offset_x = 0;
    interaction->object_move_offset_y = 0;
    interaction->object_path_point_move_active = 1u;
    interaction->object_path_point_object_id = object_id;
    interaction->object_path_point_index = point_index;
    interaction->object_path_point_anchor_sample_x = sample_x;
    interaction->object_path_point_anchor_sample_y = sample_y;
    interaction->object_path_point_offset_x = 0;
    interaction->object_path_point_offset_y = 0;
    interaction->move_axis_lock = 0u;
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

void drawing_program_visual_transform_session_update_object_path_point_move(
    const DrawingProgramAppContext *ctx,
    VisualCanvasInteractionState *interaction,
    uint32_t sample_x,
    uint32_t sample_y,
    SDL_Keymod mods) {
    const DrawingProgramObjectRecord *object = 0;
    int32_t clamped_x = 0;
    int32_t clamped_y = 0;
    int32_t offset_x = 0;
    int32_t offset_y = 0;
    if (!ctx || !interaction ||
        !drawing_program_visual_transform_session_is_object_path_point_move_active(interaction)) {
        return;
    }
    object = drawing_program_object_store_get_by_id(&ctx->object_store, interaction->object_path_point_object_id);
    if (!object || object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
        interaction->object_path_point_index >= object->path_point_count) {
        drawing_program_visual_transform_session_reset(interaction);
        return;
    }
    offset_x = (int32_t)sample_x - (int32_t)interaction->object_path_point_anchor_sample_x;
    offset_y = (int32_t)sample_y - (int32_t)interaction->object_path_point_anchor_sample_y;
    interaction->object_move_offset_x = offset_x;
    interaction->object_move_offset_y = offset_y;
    drawing_program_visual_apply_object_move_axis_lock(interaction, mods);
    offset_x = interaction->object_move_offset_x;
    offset_y = interaction->object_move_offset_y;
    clamped_x = object->path_points[interaction->object_path_point_index].x + offset_x;
    clamped_y = object->path_points[interaction->object_path_point_index].y + offset_y;
    if (clamped_x < 0) {
        clamped_x = 0;
    }
    if (clamped_y < 0) {
        clamped_y = 0;
    }
    if (ctx->document.raster_width > 0u &&
        clamped_x >= (int32_t)ctx->document.raster_width) {
        clamped_x = (int32_t)ctx->document.raster_width - 1;
    }
    if (ctx->document.raster_height > 0u &&
        clamped_y >= (int32_t)ctx->document.raster_height) {
        clamped_y = (int32_t)ctx->document.raster_height - 1;
    }
    interaction->object_path_point_offset_x =
        clamped_x - object->path_points[interaction->object_path_point_index].x;
    interaction->object_path_point_offset_y =
        clamped_y - object->path_points[interaction->object_path_point_index].y;
}

void drawing_program_visual_transform_session_update_object_path_handle_move(
    const DrawingProgramAppContext *ctx,
    VisualCanvasInteractionState *interaction,
    uint32_t sample_x,
    uint32_t sample_y) {
    const DrawingProgramObjectRecord *object = 0;
    const DrawingProgramPathPoint *point = 0;
    int32_t dx = 0;
    int32_t dy = 0;
    if (!ctx || !interaction ||
        !drawing_program_visual_transform_session_is_object_path_handle_move_active(interaction)) {
        return;
    }
    object = drawing_program_object_store_get_by_id(&ctx->object_store, interaction->object_path_handle_object_id);
    if (!object || object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
        interaction->object_path_handle_point_index >= object->path_point_count) {
        drawing_program_visual_transform_session_reset(interaction);
        return;
    }
    point = &object->path_points[interaction->object_path_handle_point_index];
    dx = (int32_t)sample_x - point->x;
    dy = (int32_t)sample_y - point->y;
    interaction->object_path_handle_dx = dx;
    interaction->object_path_handle_dy = dy;
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

CoreResult drawing_program_visual_transform_session_commit_object_path_point_move(
    DrawingProgramAppContext *ctx,
    VisualCanvasInteractionState *interaction,
    const DrawingProgramVisualTransformOpsHooks *hooks) {
    const DrawingProgramObjectRecord *object = 0;
    int32_t target_x = 0;
    int32_t target_y = 0;
    CoreResult result;
    if (!ctx || !interaction || !hooks || !hooks->visual_object_commit_path_point ||
        !drawing_program_visual_transform_session_is_object_path_point_move_active(interaction)) {
        return core_result_ok();
    }
    object = drawing_program_object_store_get_by_id(&ctx->object_store, interaction->object_path_point_object_id);
    if (!object || object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
        interaction->object_path_point_index >= object->path_point_count) {
        drawing_program_visual_transform_session_reset(interaction);
        return core_result_ok();
    }
    target_x = object->path_points[interaction->object_path_point_index].x +
               interaction->object_path_point_offset_x;
    target_y = object->path_points[interaction->object_path_point_index].y +
               interaction->object_path_point_offset_y;
    result = hooks->visual_object_commit_path_point(ctx,
                                                    interaction->object_path_point_object_id,
                                                    interaction->object_path_point_index,
                                                    target_x,
                                                    target_y);
    drawing_program_visual_transform_session_reset(interaction);
    return result;
}

CoreResult drawing_program_visual_transform_session_commit_object_path_handle_move(
    DrawingProgramAppContext *ctx,
    VisualCanvasInteractionState *interaction,
    const DrawingProgramVisualTransformOpsHooks *hooks) {
    const DrawingProgramObjectRecord *object = 0;
    DrawingProgramPathPoint point;
    CoreResult result;
    if (!ctx || !interaction || !hooks || !hooks->visual_object_commit_path_point_data ||
        !drawing_program_visual_transform_session_is_object_path_handle_move_active(interaction)) {
        return core_result_ok();
    }
    object = drawing_program_object_store_get_by_id(&ctx->object_store, interaction->object_path_handle_object_id);
    if (!object || object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
        interaction->object_path_handle_point_index >= object->path_point_count) {
        drawing_program_visual_transform_session_reset(interaction);
        return core_result_ok();
    }
    point = object->path_points[interaction->object_path_handle_point_index];
    if (interaction->object_path_handle_kind == (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_IN) {
        point.handle_in_dx = interaction->object_path_handle_dx;
        point.handle_in_dy = interaction->object_path_handle_dy;
        if (point.handle_linked) {
            point.handle_out_dx = -interaction->object_path_handle_dx;
            point.handle_out_dy = -interaction->object_path_handle_dy;
        }
    } else if (interaction->object_path_handle_kind == (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_OUT) {
        point.handle_out_dx = interaction->object_path_handle_dx;
        point.handle_out_dy = interaction->object_path_handle_dy;
        if (point.handle_linked) {
            point.handle_in_dx = -interaction->object_path_handle_dx;
            point.handle_in_dy = -interaction->object_path_handle_dy;
        }
    }
    point.bezier_enabled = 1u;
    result = hooks->visual_object_commit_path_point_data(
        ctx,
        interaction->object_path_handle_object_id,
        interaction->object_path_handle_point_index,
        &point);
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
    if (ctx && interaction) {
        (void)drawing_program_visual_flush_direct_stroke_history(
            ctx, interaction, interaction->direct_stroke_history_layer_id);
    }
    drawing_program_visual_end_canvas_history_group(ctx);
    drawing_program_visual_cancel_canvas_draw_and_shape(interaction);
    drawing_program_visual_cancel_selection_transient(selection);
    if (clear_pan_state && interaction) {
        interaction->panning_active = 0u;
    }
    drawing_program_visual_transform_session_reset(interaction);
}
