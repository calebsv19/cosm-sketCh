#include "drawing_program/drawing_program_object_store.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static CoreResult object_store_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static int object_type_valid(uint8_t type) {
    return (type >= (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT &&
            type <= (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH);
}

static int object_bounds_contains(const DrawingProgramObjectRecord *object, uint32_t sample_x, uint32_t sample_y) {
    int64_t left;
    int64_t top;
    int64_t right;
    int64_t bottom;
    int64_t sx;
    int64_t sy;
    if (!object) {
        return 0;
    }
    left = (int64_t)object->origin_x;
    top = (int64_t)object->origin_y;
    right = left + (int64_t)object->width;
    bottom = top + (int64_t)object->height;
    sx = (int64_t)sample_x;
    sy = (int64_t)sample_y;
    if (sx < left || sy < top) {
        return 0;
    }
    if (sx >= right || sy >= bottom) {
        return 0;
    }
    return 1;
}

static int object_ellipse_contains(const DrawingProgramObjectRecord *object, uint32_t sample_x, uint32_t sample_y) {
    double rx;
    double ry;
    double cx;
    double cy;
    double dx;
    double dy;
    if (!object || object->width == 0u || object->height == 0u) {
        return 0;
    }
    if (!object_bounds_contains(object, sample_x, sample_y)) {
        return 0;
    }
    rx = ((double)object->width) * 0.5;
    ry = ((double)object->height) * 0.5;
    if (rx <= 0.0 || ry <= 0.0) {
        return 0;
    }
    cx = (double)object->origin_x + rx;
    cy = (double)object->origin_y + ry;
    dx = (((double)sample_x) + 0.5) - cx;
    dy = (((double)sample_y) + 0.5) - cy;
    return (((dx * dx) / (rx * rx)) + ((dy * dy) / (ry * ry)) <= 1.0) ? 1 : 0;
}

static CoreResult object_store_prepare_record(const DrawingProgramObjectRecord *seed,
                                              uint32_t object_id,
                                              DrawingProgramObjectRecord *out_record) {
    if (!seed || !out_record || object_id == 0u) {
        return object_store_invalid("invalid object record seed");
    }
    if (seed->layer_id == 0u) {
        return object_store_invalid("object layer_id must be non-zero");
    }
    if (!object_type_valid(seed->type)) {
        return object_store_invalid("unsupported object type");
    }
    if (seed->width == 0u || seed->height == 0u) {
        return object_store_invalid("object bounds must be non-zero");
    }

    memset(out_record, 0, sizeof(*out_record));
    *out_record = *seed;
    out_record->object_id = object_id;
    out_record->visible = seed->visible ? 1u : 0u;
    out_record->locked = seed->locked ? 1u : 0u;
    out_record->name[DRAWING_PROGRAM_OBJECT_NAME_CAPACITY - 1u] = '\0';
    return core_result_ok();
}

void drawing_program_object_store_reset(DrawingProgramObjectStore *store) {
    if (!store) {
        return;
    }
    memset(store, 0, sizeof(*store));
    store->schema_version = DRAWING_PROGRAM_OBJECT_STORE_SCHEMA_VERSION;
    store->next_object_id = 1u;
}

CoreResult drawing_program_object_store_find_index_for_id(const DrawingProgramObjectStore *store,
                                                          uint32_t object_id,
                                                          uint32_t *out_index) {
    uint32_t i;
    if (!store || !out_index || object_id == 0u) {
        return object_store_invalid("invalid object id lookup");
    }
    for (i = 0u; i < store->object_count; ++i) {
        if (store->objects[i].object_id == object_id) {
            *out_index = i;
            return core_result_ok();
        }
    }
    return (CoreResult){ CORE_ERR_NOT_FOUND, "object id not found" };
}

const DrawingProgramObjectRecord *drawing_program_object_store_get_by_id(const DrawingProgramObjectStore *store,
                                                                         uint32_t object_id) {
    uint32_t index = 0u;
    if (!store || object_id == 0u) {
        return 0;
    }
    if (drawing_program_object_store_find_index_for_id(store, object_id, &index).code != CORE_OK) {
        return 0;
    }
    return &store->objects[index];
}

CoreResult drawing_program_object_store_insert_with_id(DrawingProgramObjectStore *store,
                                                       const DrawingProgramObjectRecord *seed,
                                                       uint32_t object_id) {
    DrawingProgramObjectRecord record;
    CoreResult result;
    uint32_t ignored_index = 0u;
    if (!store || !seed || object_id == 0u) {
        return object_store_invalid("invalid object insert request");
    }
    if (store->object_count >= DRAWING_PROGRAM_MAX_OBJECTS) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "object store capacity reached" };
    }
    result = drawing_program_object_store_find_index_for_id(store, object_id, &ignored_index);
    if (result.code == CORE_OK) {
        return (CoreResult){ CORE_ERR_FORMAT, "object id already exists" };
    }
    if (result.code != CORE_ERR_NOT_FOUND) {
        return result;
    }
    result = object_store_prepare_record(seed, object_id, &record);
    if (result.code != CORE_OK) {
        return result;
    }
    store->objects[store->object_count] = record;
    store->object_count += 1u;
    if (object_id >= store->next_object_id) {
        store->next_object_id = object_id + 1u;
        if (store->next_object_id == 0u) {
            store->next_object_id = 1u;
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_object_store_add(DrawingProgramObjectStore *store,
                                            const DrawingProgramObjectRecord *seed,
                                            uint32_t *out_object_id) {
    uint32_t object_id = 0u;
    CoreResult result;
    if (!store || !seed) {
        return object_store_invalid("invalid object add request");
    }
    if (store->next_object_id == 0u) {
        store->next_object_id = 1u;
    }
    object_id = store->next_object_id;
    result = drawing_program_object_store_insert_with_id(store, seed, object_id);
    if (result.code != CORE_OK) {
        return result;
    }
    if (out_object_id) {
        *out_object_id = object_id;
    }
    return core_result_ok();
}

CoreResult drawing_program_object_store_remove_by_id(DrawingProgramObjectStore *store,
                                                     uint32_t object_id,
                                                     uint32_t *out_removed_index) {
    uint32_t index = 0u;
    uint32_t i;
    CoreResult result;
    if (!store || object_id == 0u) {
        return object_store_invalid("invalid object remove request");
    }
    result = drawing_program_object_store_find_index_for_id(store, object_id, &index);
    if (result.code != CORE_OK) {
        return result;
    }
    if (out_removed_index) {
        *out_removed_index = index;
    }
    for (i = index + 1u; i < store->object_count; ++i) {
        store->objects[i - 1u] = store->objects[i];
    }
    memset(&store->objects[store->object_count - 1u], 0, sizeof(store->objects[store->object_count - 1u]));
    store->object_count -= 1u;
    return core_result_ok();
}

CoreResult drawing_program_object_store_replace_all(DrawingProgramObjectStore *store,
                                                    const DrawingProgramObjectRecord *records,
                                                    uint32_t record_count,
                                                    uint32_t next_object_id) {
    DrawingProgramObjectStore staged;
    uint32_t i;
    CoreResult result;
    if (!store) {
        return object_store_invalid("invalid object replace request");
    }
    if (record_count > 0u && !records) {
        return object_store_invalid("missing replacement object records");
    }
    if (record_count > DRAWING_PROGRAM_MAX_OBJECTS) {
        return object_store_invalid("replacement object count exceeds capacity");
    }

    drawing_program_object_store_reset(&staged);
    for (i = 0u; i < record_count; ++i) {
        result = drawing_program_object_store_insert_with_id(&staged, &records[i], records[i].object_id);
        if (result.code != CORE_OK) {
            return result;
        }
    }
    if (next_object_id == 0u) {
        next_object_id = 1u;
    }
    if (next_object_id <= staged.next_object_id) {
        staged.next_object_id = staged.next_object_id;
    } else {
        staged.next_object_id = next_object_id;
    }
    *store = staged;
    return core_result_ok();
}

CoreResult drawing_program_object_store_set_origin(DrawingProgramObjectStore *store,
                                                   uint32_t object_id,
                                                   int32_t origin_x,
                                                   int32_t origin_y,
                                                   int32_t *out_previous_x,
                                                   int32_t *out_previous_y) {
    uint32_t index = 0u;
    CoreResult result;
    if (!store || object_id == 0u) {
        return object_store_invalid("invalid object set-origin request");
    }
    result = drawing_program_object_store_find_index_for_id(store, object_id, &index);
    if (result.code != CORE_OK) {
        return result;
    }
    if (out_previous_x) {
        *out_previous_x = store->objects[index].origin_x;
    }
    if (out_previous_y) {
        *out_previous_y = store->objects[index].origin_y;
    }
    store->objects[index].origin_x = origin_x;
    store->objects[index].origin_y = origin_y;
    return core_result_ok();
}

CoreResult drawing_program_object_store_clamp_selection_delta(const DrawingProgramObjectStore *store,
                                                              const DrawingProgramDocument *document,
                                                              const DrawingProgramObjectSelectionState *selection,
                                                              int32_t requested_dx,
                                                              int32_t requested_dy,
                                                              int32_t *out_dx,
                                                              int32_t *out_dy) {
    int64_t min_dx = INT32_MIN;
    int64_t max_dx = INT32_MAX;
    int64_t min_dy = INT32_MIN;
    int64_t max_dy = INT32_MAX;
    uint32_t i;
    uint32_t resolved = 0u;
    int64_t clamped_dx;
    int64_t clamped_dy;
    if (!store || !document || !selection || !out_dx || !out_dy) {
        return object_store_invalid("invalid object selection clamp request");
    }
    if (selection->count == 0u) {
        *out_dx = 0;
        *out_dy = 0;
        return core_result_ok();
    }
    for (i = 0u; i < selection->count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        const DrawingProgramObjectRecord *object = drawing_program_object_store_get_by_id(store, selection->object_ids[i]);
        int64_t local_min_dx;
        int64_t local_max_dx;
        int64_t local_min_dy;
        int64_t local_max_dy;
        if (!object) {
            continue;
        }
        local_min_dx = -(int64_t)object->origin_x;
        local_min_dy = -(int64_t)object->origin_y;
        local_max_dx = (int64_t)document->raster_width - ((int64_t)object->origin_x + (int64_t)object->width);
        local_max_dy = (int64_t)document->raster_height - ((int64_t)object->origin_y + (int64_t)object->height);
        if (local_min_dx > min_dx) {
            min_dx = local_min_dx;
        }
        if (local_max_dx < max_dx) {
            max_dx = local_max_dx;
        }
        if (local_min_dy > min_dy) {
            min_dy = local_min_dy;
        }
        if (local_max_dy < max_dy) {
            max_dy = local_max_dy;
        }
        resolved += 1u;
    }
    if (resolved == 0u) {
        *out_dx = 0;
        *out_dy = 0;
        return (CoreResult){ CORE_ERR_NOT_FOUND, "no selected objects available for clamp" };
    }
    if (min_dx > max_dx || min_dy > max_dy) {
        *out_dx = 0;
        *out_dy = 0;
        return core_result_ok();
    }
    clamped_dx = (int64_t)requested_dx;
    clamped_dy = (int64_t)requested_dy;
    if (clamped_dx < min_dx) {
        clamped_dx = min_dx;
    }
    if (clamped_dx > max_dx) {
        clamped_dx = max_dx;
    }
    if (clamped_dy < min_dy) {
        clamped_dy = min_dy;
    }
    if (clamped_dy > max_dy) {
        clamped_dy = max_dy;
    }
    *out_dx = (int32_t)clamped_dx;
    *out_dy = (int32_t)clamped_dy;
    return core_result_ok();
}

CoreResult drawing_program_object_store_apply_selection_delta(DrawingProgramObjectStore *store,
                                                              const DrawingProgramObjectSelectionState *selection,
                                                              int32_t dx,
                                                              int32_t dy) {
    uint32_t i;
    if (!store || !selection) {
        return object_store_invalid("invalid object selection delta apply request");
    }
    for (i = 0u; i < selection->count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        const DrawingProgramObjectRecord *object = 0;
        uint32_t object_id = selection->object_ids[i];
        if (object_id == 0u) {
            continue;
        }
        object = drawing_program_object_store_get_by_id(store, object_id);
        if (!object) {
            continue;
        }
        (void)drawing_program_object_store_set_origin(
            store, object_id, object->origin_x + dx, object->origin_y + dy, 0, 0);
    }
    return core_result_ok();
}

CoreResult drawing_program_object_store_hit_test_topmost(const DrawingProgramObjectStore *store,
                                                         const DrawingProgramDocument *document,
                                                         uint32_t sample_x,
                                                         uint32_t sample_y,
                                                         uint32_t *out_object_id,
                                                         uint32_t *out_index) {
    uint32_t i;
    if (!store || !out_object_id) {
        return object_store_invalid("invalid object hit-test request");
    }
    for (i = store->object_count; i > 0u; --i) {
        uint32_t object_index = i - 1u;
        const DrawingProgramObjectRecord *object = &store->objects[object_index];
        if (!object->visible || object->locked) {
            continue;
        }
        if (document) {
            uint32_t layer_index = 0u;
            if (drawing_program_document_layer_index_for_id(document, object->layer_id, &layer_index).code != CORE_OK) {
                continue;
            }
            if (!document->layers[layer_index].visible || document->layers[layer_index].locked) {
                continue;
            }
        }
        if (!object_bounds_contains(object, sample_x, sample_y)) {
            continue;
        }
        if (object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_ELLIPSE &&
            !object_ellipse_contains(object, sample_x, sample_y)) {
            continue;
        }
        *out_object_id = object->object_id;
        if (out_index) {
            *out_index = object_index;
        }
        return core_result_ok();
    }
    return (CoreResult){ CORE_ERR_NOT_FOUND, "no object hit for sample" };
}

uint32_t drawing_program_object_store_collect_visible_indices(const DrawingProgramObjectStore *store,
                                                              const DrawingProgramDocument *document,
                                                              uint32_t *out_indices,
                                                              uint32_t out_capacity) {
    uint32_t i;
    uint32_t count = 0u;
    if (!store) {
        return 0u;
    }
    for (i = 0u; i < store->object_count; ++i) {
        const DrawingProgramObjectRecord *object = &store->objects[i];
        uint8_t visible = object->visible ? 1u : 0u;
        if (!visible) {
            continue;
        }
        if (document) {
            uint32_t layer_index = 0u;
            if (drawing_program_document_layer_index_for_id(document, object->layer_id, &layer_index).code != CORE_OK) {
                continue;
            }
            if (!document->layers[layer_index].visible) {
                continue;
            }
        }
        if (out_indices && count < out_capacity) {
            out_indices[count] = i;
        }
        count += 1u;
    }
    return count;
}

void drawing_program_object_selection_reset(DrawingProgramObjectSelectionState *selection) {
    if (!selection) {
        return;
    }
    memset(selection, 0, sizeof(*selection));
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
        return 0;
    }
    if (selection->count >= DRAWING_PROGRAM_MAX_OBJECTS) {
        return 0;
    }
    selection->object_ids[selection->count] = object_id;
    selection->count += 1u;
    selection->active_object_id = object_id;
    return 1;
}
