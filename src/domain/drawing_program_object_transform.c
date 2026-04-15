#include "drawing_program/drawing_program_object_transform.h"

static CoreResult object_transform_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

CoreResult drawing_program_object_transform_commit_move(DrawingProgramHistory *history,
                                                        DrawingProgramObjectStore *object_store,
                                                        const DrawingProgramDocument *document,
                                                        const DrawingProgramObjectSelectionState *selection,
                                                        int32_t requested_dx,
                                                        int32_t requested_dy,
                                                        int32_t *out_applied_dx,
                                                        int32_t *out_applied_dy) {
    CoreResult result;
    int32_t applied_dx = 0;
    int32_t applied_dy = 0;
    uint32_t i;
    if (!history || !object_store || !document || !selection) {
        return object_transform_invalid("invalid object move commit request");
    }
    result = drawing_program_object_store_clamp_selection_delta(object_store,
                                                                document,
                                                                selection,
                                                                requested_dx,
                                                                requested_dy,
                                                                &applied_dx,
                                                                &applied_dy);
    if (result.code != CORE_OK && result.code != CORE_ERR_NOT_FOUND) {
        return result;
    }
    if (out_applied_dx) {
        *out_applied_dx = applied_dx;
    }
    if (out_applied_dy) {
        *out_applied_dy = applied_dy;
    }
    if (selection->count == 0u || (applied_dx == 0 && applied_dy == 0)) {
        return core_result_ok();
    }
    result = drawing_program_history_begin_group(history);
    if (result.code != CORE_OK) {
        return result;
    }
    for (i = 0u; i < selection->count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        uint32_t object_id = selection->object_ids[i];
        const DrawingProgramObjectRecord *object = 0;
        if (object_id == 0u) {
            continue;
        }
        object = drawing_program_object_store_get_by_id(object_store, object_id);
        if (!object || object->locked || !object->visible) {
            continue;
        }
        result = drawing_program_history_apply_set_object_origin(history,
                                                                 object_store,
                                                                 object_id,
                                                                 object->origin_x + applied_dx,
                                                                 object->origin_y + applied_dy);
        if (result.code != CORE_OK) {
            (void)drawing_program_history_end_group(history);
            return result;
        }
    }
    result = drawing_program_history_end_group(history);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_object_store_promote_selection(object_store, selection);
    if (result.code != CORE_OK) {
        return result;
    }
    return core_result_ok();
}
