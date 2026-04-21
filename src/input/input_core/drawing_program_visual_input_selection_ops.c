#include "drawing_program/drawing_program_visual_input_selection_ops.h"

#include <math.h>
#include <string.h>

#include "drawing_program/drawing_program_history.h"

static void prune_selected_path_point_state(DrawingProgramAppContext *ctx) {
    uint32_t object_id = 0u;
    uint16_t point_index = 0u;
    const DrawingProgramObjectRecord *object = 0;
    if (!ctx) {
        return;
    }
    if (!drawing_program_object_selection_get_path_point(&ctx->object_selection, &object_id, &point_index)) {
        return;
    }
    object = drawing_program_object_store_get_by_id(&ctx->object_store, object_id);
    if (!object ||
        object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
        point_index >= object->path_point_count) {
        drawing_program_object_selection_clear_path_point(&ctx->object_selection);
    }
}

void drawing_program_visual_input_object_selection_clear(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    drawing_program_object_selection_reset(&ctx->object_selection);
}

void drawing_program_visual_input_object_selection_replace(DrawingProgramAppContext *ctx, uint32_t object_id) {
    if (!ctx) {
        return;
    }
    drawing_program_object_selection_replace_single(&ctx->object_selection, object_id);
}

void drawing_program_visual_input_object_selection_add(DrawingProgramAppContext *ctx, uint32_t object_id) {
    if (!ctx) {
        return;
    }
    (void)drawing_program_object_selection_add(&ctx->object_selection, object_id);
}

void drawing_program_visual_input_object_selection_remove(DrawingProgramAppContext *ctx, uint32_t object_id) {
    uint32_t i;
    uint32_t write_index = 0u;
    uint32_t new_active = 0u;
    if (!ctx || object_id == 0u || ctx->object_selection.count == 0u) {
        return;
    }
    for (i = 0u; i < ctx->object_selection.count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        uint32_t id = ctx->object_selection.object_ids[i];
        if (id == 0u || id == object_id) {
            continue;
        }
        ctx->object_selection.object_ids[write_index] = id;
        new_active = id;
        write_index += 1u;
    }
    for (i = write_index; i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        ctx->object_selection.object_ids[i] = 0u;
    }
    ctx->object_selection.count = write_index;
    ctx->object_selection.active_object_id = new_active;
    prune_selected_path_point_state(ctx);
}

int drawing_program_visual_input_object_selection_hit_test(const DrawingProgramAppContext *ctx,
                                                           uint32_t sample_x,
                                                           uint32_t sample_y,
                                                           uint32_t *out_object_id) {
    CoreResult result;
    if (!ctx || !out_object_id) {
        return 0;
    }
    result = drawing_program_object_store_hit_test_topmost(
        &ctx->object_store, &ctx->document, sample_x, sample_y, out_object_id, 0);
    return (result.code == CORE_OK) ? 1 : 0;
}

VisualMarqueeCommitMode drawing_program_visual_input_resolve_select_commit_mode(
    const DrawingProgramAppContext *ctx,
    SDL_Keymod mods) {
    uint8_t ui_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
    if ((mods & KMOD_ALT) != 0) {
        return VISUAL_MARQUEE_COMMIT_SUBTRACT;
    }
    if ((mods & KMOD_SHIFT) != 0) {
        return VISUAL_MARQUEE_COMMIT_ADD;
    }
    if (ctx) {
        ui_mode = ctx->ui.tool_select_mode;
    }
    if (ui_mode > (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_SUBTRACT) {
        ui_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
    }
    return (VisualMarqueeCommitMode)ui_mode;
}

static uint32_t delete_selected_objects(DrawingProgramAppContext *ctx) {
    uint32_t removed = 0u;
    uint32_t i;
    uint32_t ids[DRAWING_PROGRAM_MAX_OBJECTS];
    if (!ctx || ctx->object_selection.count == 0u) {
        return 0u;
    }
    memset(ids, 0, sizeof(ids));
    for (i = 0u; i < ctx->object_selection.count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        ids[i] = ctx->object_selection.object_ids[i];
    }
    for (i = 0u; i < ctx->object_selection.count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        if (ids[i] == 0u) {
            continue;
        }
        if (drawing_program_object_store_remove_by_id(&ctx->object_store, ids[i], 0).code == CORE_OK) {
            removed += 1u;
        }
    }
    if (removed > 0u) {
        drawing_program_object_selection_reset(&ctx->object_selection);
    }
    return removed;
}

static uint32_t delete_selected_path_point(DrawingProgramAppContext *ctx) {
    uint32_t object_id = 0u;
    uint16_t point_index = 0u;
    DrawingProgramPathPayload payload;
    CoreResult result;
    uint16_t next_index = 0u;
    if (!ctx) {
        return 0u;
    }
    if (!drawing_program_object_selection_get_path_point(&ctx->object_selection, &object_id, &point_index)) {
        return 0u;
    }
    if (drawing_program_object_store_get_path_payload(&ctx->object_store, object_id, &payload).code != CORE_OK ||
        point_index >= payload.point_count) {
        drawing_program_object_selection_clear_path_point(&ctx->object_selection);
        return 0u;
    }
    if (payload.point_count <= 2u) {
        if (drawing_program_object_store_remove_by_id(&ctx->object_store, object_id, 0).code == CORE_OK) {
            drawing_program_visual_input_object_selection_remove(ctx, object_id);
            drawing_program_object_selection_clear_path_point(&ctx->object_selection);
            return 1u;
        }
        return 0u;
    }
    next_index = point_index;
    if ((uint32_t)next_index >= payload.point_count - 1u) {
        next_index = (uint16_t)(payload.point_count - 2u);
    }
    result = drawing_program_history_apply_remove_object_path_point(
        &ctx->history, &ctx->object_store, object_id, point_index);
    if (result.code != CORE_OK) {
        return 0u;
    }
    if (drawing_program_object_store_get_path_payload(&ctx->object_store, object_id, &payload).code != CORE_OK ||
        payload.point_count == 0u) {
        drawing_program_object_selection_clear_path_point(&ctx->object_selection);
        return 1u;
    }
    if ((uint32_t)next_index >= payload.point_count) {
        next_index = (uint16_t)(payload.point_count - 1u);
    }
    (void)drawing_program_object_selection_set_path_point(&ctx->object_selection, object_id, next_index);
    return 1u;
}

static int toggle_seed_handle_axis(int32_t dx,
                                   int32_t dy,
                                   double handle_length,
                                   int32_t *out_handle_dx,
                                   int32_t *out_handle_dy) {
    double length = 0.0;
    double scale = 0.0;
    if (!out_handle_dx || !out_handle_dy) {
        return 0;
    }
    length = sqrt((double)dx * (double)dx + (double)dy * (double)dy);
    if (length < 0.001 || handle_length <= 0.0) {
        *out_handle_dx = 0;
        *out_handle_dy = 0;
        return 0;
    }
    scale = handle_length / length;
    *out_handle_dx = (int32_t)lround((double)dx * scale);
    *out_handle_dy = (int32_t)lround((double)dy * scale);
    return (*out_handle_dx != 0 || *out_handle_dy != 0);
}

static void toggle_seed_path_point_bezier(DrawingProgramPathPayload *payload, uint16_t point_index) {
    DrawingProgramPathPoint *point = 0;
    const DrawingProgramPathPoint *prev = 0;
    const DrawingProgramPathPoint *next = 0;
    uint32_t prev_index = 0u;
    uint32_t next_index = 0u;
    int has_prev = 0;
    int has_next = 0;
    if (!payload || payload->point_count == 0u || (uint32_t)point_index >= payload->point_count) {
        return;
    }
    point = &payload->points[point_index];
    if (point->bezier_enabled) {
        point->bezier_enabled = 0u;
        point->handle_linked = 0u;
        point->handle_in_dx = 0;
        point->handle_in_dy = 0;
        point->handle_out_dx = 0;
        point->handle_out_dy = 0;
        return;
    }
    if (payload->closed || point_index > 0u) {
        prev_index = (point_index == 0u) ? (payload->point_count - 1u) : ((uint32_t)point_index - 1u);
        has_prev = (prev_index != (uint32_t)point_index);
    }
    if (payload->closed || ((uint32_t)point_index + 1u) < payload->point_count) {
        next_index = (((uint32_t)point_index + 1u) < payload->point_count) ? ((uint32_t)point_index + 1u) : 0u;
        has_next = (next_index != (uint32_t)point_index);
    }
    prev = has_prev ? &payload->points[prev_index] : 0;
    next = has_next ? &payload->points[next_index] : 0;

    point->bezier_enabled = 1u;
    point->handle_linked = 1u;
    point->handle_in_dx = 0;
    point->handle_in_dy = 0;
    point->handle_out_dx = 0;
    point->handle_out_dy = 0;

    if (has_prev && has_next) {
        int32_t tangent_dx = next->x - prev->x;
        int32_t tangent_dy = next->y - prev->y;
        double in_length = sqrt((double)(point->x - prev->x) * (double)(point->x - prev->x) +
                                (double)(point->y - prev->y) * (double)(point->y - prev->y)) /
                           3.0;
        double out_length = sqrt((double)(next->x - point->x) * (double)(next->x - point->x) +
                                 (double)(next->y - point->y) * (double)(next->y - point->y)) /
                            3.0;
        if (!toggle_seed_handle_axis(tangent_dx, tangent_dy, in_length, &point->handle_out_dx, &point->handle_out_dy)) {
            point->bezier_enabled = 0u;
            point->handle_linked = 0u;
            return;
        }
        point->handle_in_dx = -point->handle_out_dx;
        point->handle_in_dy = -point->handle_out_dy;
        if (out_length > 0.0) {
            (void)toggle_seed_handle_axis(tangent_dx, tangent_dy, out_length, &point->handle_out_dx, &point->handle_out_dy);
        }
        if (in_length > 0.0) {
            int32_t in_dx = 0;
            int32_t in_dy = 0;
            (void)toggle_seed_handle_axis(-tangent_dx, -tangent_dy, in_length, &in_dx, &in_dy);
            point->handle_in_dx = in_dx;
            point->handle_in_dy = in_dy;
        }
        return;
    }
    if (has_next) {
        int32_t tangent_dx = next->x - point->x;
        int32_t tangent_dy = next->y - point->y;
        double tangent_length = sqrt((double)tangent_dx * (double)tangent_dx +
                                     (double)tangent_dy * (double)tangent_dy) /
                                3.0;
        if (!toggle_seed_handle_axis(tangent_dx, tangent_dy, tangent_length, &point->handle_out_dx, &point->handle_out_dy)) {
            point->bezier_enabled = 0u;
            point->handle_linked = 0u;
            return;
        }
        point->handle_in_dx = -point->handle_out_dx;
        point->handle_in_dy = -point->handle_out_dy;
        return;
    }
    if (has_prev) {
        int32_t tangent_dx = point->x - prev->x;
        int32_t tangent_dy = point->y - prev->y;
        double tangent_length = sqrt((double)tangent_dx * (double)tangent_dx +
                                     (double)tangent_dy * (double)tangent_dy) /
                                3.0;
        if (!toggle_seed_handle_axis(tangent_dx, tangent_dy, tangent_length, &point->handle_out_dx, &point->handle_out_dy)) {
            point->bezier_enabled = 0u;
            point->handle_linked = 0u;
            return;
        }
        point->handle_in_dx = -point->handle_out_dx;
        point->handle_in_dy = -point->handle_out_dy;
        return;
    }
    point->bezier_enabled = 0u;
    point->handle_linked = 0u;
}

int delete_active_selection_payload_or_objects(DrawingProgramAppContext *ctx,
                                               DrawingProgramSelectionState *selection_state,
                                               const DrawingProgramVisualInputHandlersHooks *hooks) {
    if (!ctx || !selection_state || !hooks) {
        return 0;
    }
    if (ctx->object_selection.count > 0u) {
        if (delete_selected_path_point(ctx) > 0u) {
            drawing_program_selection_reset(selection_state);
            return 1;
        }
        if (delete_selected_objects(ctx) > 0u) {
            drawing_program_selection_reset(selection_state);
            return 1;
        }
    }
    if (selection_state->has_payload && hooks->active_layer_allows_edits_visual(ctx)) {
        (void)drawing_program_selection_delete_payload(&ctx->document,
                                                       &ctx->layer_rasters,
                                                       ctx->editor.active_layer_id,
                                                       &ctx->history,
                                                       selection_state);
        return 1;
    }
    return 0;
}

int drawing_program_visual_input_toggle_selected_path_point_bezier(DrawingProgramAppContext *ctx) {
    uint32_t object_id = 0u;
    uint16_t point_index = 0u;
    DrawingProgramPathPayload payload;
    if (!ctx || ctx->editor.active_tool != DRAWING_PROGRAM_TOOL_SELECT) {
        return 0;
    }
    if (!drawing_program_object_selection_get_path_point(&ctx->object_selection, &object_id, &point_index)) {
        return 0;
    }
    if (drawing_program_object_store_get_path_payload(&ctx->object_store, object_id, &payload).code != CORE_OK ||
        (uint32_t)point_index >= payload.point_count) {
        drawing_program_object_selection_clear_path_point(&ctx->object_selection);
        return 0;
    }
    toggle_seed_path_point_bezier(&payload, point_index);
    if (drawing_program_history_apply_set_object_path_point_data(
            &ctx->history, &ctx->object_store, object_id, point_index, &payload.points[point_index]).code != CORE_OK) {
        return 0;
    }
    (void)drawing_program_object_selection_set_path_point(&ctx->object_selection, object_id, point_index);
    return 1;
}

int drawing_program_visual_input_toggle_selected_path_point_handle_link(DrawingProgramAppContext *ctx) {
    uint32_t object_id = 0u;
    uint16_t point_index = 0u;
    DrawingProgramPathPayload payload;
    if (!ctx || ctx->editor.active_tool != DRAWING_PROGRAM_TOOL_SELECT) {
        return 0;
    }
    if (!drawing_program_object_selection_get_path_point(&ctx->object_selection, &object_id, &point_index)) {
        return 0;
    }
    if (drawing_program_object_store_get_path_payload(&ctx->object_store, object_id, &payload).code != CORE_OK ||
        (uint32_t)point_index >= payload.point_count) {
        drawing_program_object_selection_clear_path_point(&ctx->object_selection);
        return 0;
    }
    if (!payload.points[point_index].bezier_enabled) {
        return 0;
    }
    payload.points[point_index].handle_linked = payload.points[point_index].handle_linked ? 0u : 1u;
    if (drawing_program_history_apply_set_object_path_point_data(
            &ctx->history, &ctx->object_store, object_id, point_index, &payload.points[point_index]).code != CORE_OK) {
        return 0;
    }
    (void)drawing_program_object_selection_set_path_point(&ctx->object_selection, object_id, point_index);
    return 1;
}
