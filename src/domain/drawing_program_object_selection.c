#include "drawing_program/drawing_program_object_store.h"

#include <string.h>

void drawing_program_object_selection_reset(DrawingProgramObjectSelectionState *selection) {
    if (!selection) {
        return;
    }
    memset(selection, 0, sizeof(*selection));
}

void drawing_program_object_selection_clear_path_point(DrawingProgramObjectSelectionState *selection) {
    if (!selection) {
        return;
    }
    selection->selected_path_point_active = 0u;
    selection->selected_path_point_index = 0u;
    selection->selected_path_point_object_id = 0u;
}

int drawing_program_object_selection_contains(const DrawingProgramObjectSelectionState *selection,
                                              uint32_t object_id) {
    uint32_t i;
    if (!selection || object_id == 0u) {
        return 0;
    }
    for (i = 0u; i < selection->count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        if (selection->object_ids[i] == object_id) {
            return 1;
        }
    }
    return 0;
}

void drawing_program_object_selection_replace_single(DrawingProgramObjectSelectionState *selection,
                                                     uint32_t object_id) {
    if (!selection) {
        return;
    }
    drawing_program_object_selection_reset(selection);
    if (object_id == 0u) {
        return;
    }
    selection->count = 1u;
    selection->active_object_id = object_id;
    selection->object_ids[0] = object_id;
}

int drawing_program_object_selection_add(DrawingProgramObjectSelectionState *selection,
                                         uint32_t object_id) {
    if (!selection || object_id == 0u) {
        return 0;
    }
    if (drawing_program_object_selection_contains(selection, object_id)) {
        selection->active_object_id = object_id;
        if (selection->selected_path_point_active &&
            selection->selected_path_point_object_id != object_id) {
            drawing_program_object_selection_clear_path_point(selection);
        }
        return 0;
    }
    if (selection->count >= DRAWING_PROGRAM_MAX_OBJECTS) {
        return 0;
    }
    selection->object_ids[selection->count] = object_id;
    selection->count += 1u;
    selection->active_object_id = object_id;
    drawing_program_object_selection_clear_path_point(selection);
    return 1;
}

int drawing_program_object_selection_set_path_point(DrawingProgramObjectSelectionState *selection,
                                                    uint32_t object_id,
                                                    uint16_t point_index) {
    if (!selection || object_id == 0u || !drawing_program_object_selection_contains(selection, object_id)) {
        return 0;
    }
    selection->selected_path_point_active = 1u;
    selection->selected_path_point_object_id = object_id;
    selection->selected_path_point_index = point_index;
    selection->active_object_id = object_id;
    return 1;
}

int drawing_program_object_selection_get_path_point(const DrawingProgramObjectSelectionState *selection,
                                                    uint32_t *out_object_id,
                                                    uint16_t *out_point_index) {
    if (!selection || !selection->selected_path_point_active) {
        return 0;
    }
    if (out_object_id) {
        *out_object_id = selection->selected_path_point_object_id;
    }
    if (out_point_index) {
        *out_point_index = selection->selected_path_point_index;
    }
    return 1;
}
