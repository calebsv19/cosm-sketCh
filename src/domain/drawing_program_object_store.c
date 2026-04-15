#include "drawing_program/drawing_program_object_store.h"

#include <limits.h>
#include <math.h>
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

static int selection_contains_object_id(const DrawingProgramObjectSelectionState *selection, uint32_t object_id) {
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

static void object_path_payload_clear(DrawingProgramObjectRecord *object) {
    if (!object) {
        return;
    }
    object->path_point_count = 0u;
    object->path_closed = 0u;
    object->path_reserved0 = 0u;
    memset(object->path_points, 0, sizeof(object->path_points));
}

static int object_path_payload_valid(const DrawingProgramObjectRecord *object) {
    if (!object) {
        return 0;
    }
    if (object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
        return 1;
    }
    if (object->path_point_count < 2u ||
        object->path_point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
        return 0;
    }
    return 1;
}

static CoreResult object_path_payload_bounds(const DrawingProgramPathPayload *payload,
                                             int32_t *out_origin_x,
                                             int32_t *out_origin_y,
                                             uint32_t *out_width,
                                             uint32_t *out_height) {
    uint32_t i;
    int32_t min_x;
    int32_t min_y;
    int32_t max_x;
    int32_t max_y;
    int64_t span_x;
    int64_t span_y;
    if (!payload || !out_origin_x || !out_origin_y || !out_width || !out_height) {
        return object_store_invalid("invalid path bounds request");
    }
    if (payload->point_count < 2u ||
        payload->point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
        return object_store_invalid("invalid path point count");
    }
    min_x = payload->points[0].x;
    min_y = payload->points[0].y;
    max_x = payload->points[0].x;
    max_y = payload->points[0].y;
    for (i = 1u; i < payload->point_count; ++i) {
        const int32_t px = payload->points[i].x;
        const int32_t py = payload->points[i].y;
        if (px < min_x) {
            min_x = px;
        }
        if (px > max_x) {
            max_x = px;
        }
        if (py < min_y) {
            min_y = py;
        }
        if (py > max_y) {
            max_y = py;
        }
    }
    span_x = ((int64_t)max_x - (int64_t)min_x) + 1ll;
    span_y = ((int64_t)max_y - (int64_t)min_y) + 1ll;
    if (span_x <= 0ll || span_y <= 0ll ||
        span_x > (int64_t)UINT32_MAX || span_y > (int64_t)UINT32_MAX) {
        return object_store_invalid("invalid path bounds span");
    }
    *out_origin_x = min_x;
    *out_origin_y = min_y;
    *out_width = (uint32_t)span_x;
    *out_height = (uint32_t)span_y;
    return core_result_ok();
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

static int object_layer_allows_interaction(const DrawingProgramDocument *document, uint32_t layer_id) {
    uint32_t layer_index = 0u;
    if (!document) {
        return 1;
    }
    if (drawing_program_document_layer_index_for_id(document, layer_id, &layer_index).code != CORE_OK) {
        return 0;
    }
    if (!document->layers[layer_index].visible || document->layers[layer_index].locked) {
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

static int object_style_includes_fill(uint8_t style_mode) {
    return (style_mode == 1u || style_mode == 2u) ? 1 : 0;
}

static int object_style_includes_outline(uint8_t style_mode) {
    return (style_mode == 1u) ? 0 : 1;
}

static double path_segment_distance_sq(double px,
                                       double py,
                                       double x0,
                                       double y0,
                                       double x1,
                                       double y1) {
    double dx = x1 - x0;
    double dy = y1 - y0;
    double len_sq = (dx * dx) + (dy * dy);
    double t;
    double rx;
    double ry;
    if (len_sq <= 0.0) {
        rx = px - x0;
        ry = py - y0;
        return (rx * rx) + (ry * ry);
    }
    t = (((px - x0) * dx) + ((py - y0) * dy)) / len_sq;
    if (t < 0.0) {
        t = 0.0;
    } else if (t > 1.0) {
        t = 1.0;
    }
    rx = px - (x0 + (t * dx));
    ry = py - (y0 + (t * dy));
    return (rx * rx) + (ry * ry);
}

static int object_path_contains_fill(const DrawingProgramObjectRecord *object, double px, double py) {
    uint32_t i;
    uint32_t j;
    int inside = 0;
    uint32_t point_count;
    if (!object || object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH || !object->path_closed) {
        return 0;
    }
    point_count = (uint32_t)object->path_point_count;
    if (point_count < 3u || point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
        return 0;
    }
    for (i = 0u, j = point_count - 1u; i < point_count; j = i++) {
        const double xi = (double)object->path_points[i].x + 0.5;
        const double yi = (double)object->path_points[i].y + 0.5;
        const double xj = (double)object->path_points[j].x + 0.5;
        const double yj = (double)object->path_points[j].y + 0.5;
        const int y_cross = ((yi > py) != (yj > py));
        if (!y_cross) {
            continue;
        }
        if (fabs(yj - yi) < 1e-6) {
            continue;
        }
        if (px < (((xj - xi) * (py - yi)) / (yj - yi)) + xi) {
            inside = !inside;
        }
    }
    return inside;
}

static int object_path_contains_outline(const DrawingProgramObjectRecord *object, double px, double py) {
    uint32_t segment_count = 0u;
    uint32_t point_count;
    uint32_t i;
    double half_width;
    const double pick_radius = 1.35;
    double threshold_sq;
    if (!object || object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
        return 0;
    }
    point_count = (uint32_t)object->path_point_count;
    if (point_count < 2u || point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
        return 0;
    }
    segment_count = point_count - 1u;
    if (object->path_closed) {
        segment_count += 1u;
    }
    if (segment_count == 0u) {
        return 0;
    }
    half_width = ((double)(object->stroke_width == 0u ? 1u : object->stroke_width)) * 0.5;
    if (half_width < 0.6) {
        half_width = 0.6;
    }
    half_width += pick_radius;
    threshold_sq = half_width * half_width;
    for (i = 0u; i < segment_count; ++i) {
        uint32_t a = i;
        uint32_t b = (i + 1u < point_count) ? (i + 1u) : 0u;
        double d_sq = path_segment_distance_sq(px,
                                               py,
                                               (double)object->path_points[a].x + 0.5,
                                               (double)object->path_points[a].y + 0.5,
                                               (double)object->path_points[b].x + 0.5,
                                               (double)object->path_points[b].y + 0.5);
        if (d_sq <= threshold_sq) {
            return 1;
        }
    }
    return 0;
}

static int object_path_contains(const DrawingProgramObjectRecord *object, uint32_t sample_x, uint32_t sample_y) {
    double px;
    double py;
    int fill_hit;
    int outline_hit;
    if (!object) {
        return 0;
    }
    if (!object_bounds_contains(object, sample_x, sample_y)) {
        return 0;
    }
    px = (double)sample_x + 0.5;
    py = (double)sample_y + 0.5;
    fill_hit = object_style_includes_fill(object->style_mode) ? object_path_contains_fill(object, px, py) : 0;
    outline_hit = object_style_includes_outline(object->style_mode) ? object_path_contains_outline(object, px, py) : 0;
    return (fill_hit || outline_hit) ? 1 : 0;
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
    if (out_record->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
        object_path_payload_clear(out_record);
    } else {
        out_record->path_closed = out_record->path_closed ? 1u : 0u;
        if (!object_path_payload_valid(out_record)) {
            return object_store_invalid("invalid path payload");
        }
    }
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

CoreResult drawing_program_object_store_add_path(DrawingProgramObjectStore *store,
                                                 const DrawingProgramObjectRecord *seed_style,
                                                 const DrawingProgramPathPayload *payload,
                                                 uint32_t *out_object_id) {
    DrawingProgramObjectRecord seed;
    CoreResult bounds_result;
    if (!store || !payload || !seed_style) {
        return object_store_invalid("invalid path add request");
    }
    memset(&seed, 0, sizeof(seed));
    seed = *seed_style;
    seed.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH;
    seed.path_closed = payload->closed ? 1u : 0u;
    seed.path_point_count = (uint16_t)payload->point_count;
    if (payload->point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
        return object_store_invalid("path point count exceeds capacity");
    }
    memcpy(seed.path_points,
           payload->points,
           (size_t)payload->point_count * sizeof(seed.path_points[0]));
    bounds_result = object_path_payload_bounds(payload, &seed.origin_x, &seed.origin_y, &seed.width, &seed.height);
    if (bounds_result.code != CORE_OK) {
        return bounds_result;
    }
    return drawing_program_object_store_add(store, &seed, out_object_id);
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
    int32_t previous_x = 0;
    int32_t previous_y = 0;
    if (!store || object_id == 0u) {
        return object_store_invalid("invalid object set-origin request");
    }
    result = drawing_program_object_store_find_index_for_id(store, object_id, &index);
    if (result.code != CORE_OK) {
        return result;
    }
    previous_x = store->objects[index].origin_x;
    previous_y = store->objects[index].origin_y;
    if (out_previous_x) {
        *out_previous_x = previous_x;
    }
    if (out_previous_y) {
        *out_previous_y = previous_y;
    }
    if (store->objects[index].type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH &&
        store->objects[index].path_point_count > 0u) {
        uint32_t i;
        int64_t delta_x = (int64_t)origin_x - (int64_t)previous_x;
        int64_t delta_y = (int64_t)origin_y - (int64_t)previous_y;
        for (i = 0u; i < (uint32_t)store->objects[index].path_point_count; ++i) {
            int64_t next_x = (int64_t)store->objects[index].path_points[i].x + delta_x;
            int64_t next_y = (int64_t)store->objects[index].path_points[i].y + delta_y;
            if (next_x < (int64_t)INT32_MIN || next_x > (int64_t)INT32_MAX ||
                next_y < (int64_t)INT32_MIN || next_y > (int64_t)INT32_MAX) {
                return object_store_invalid("path move origin delta out of range");
            }
            store->objects[index].path_points[i].x = (int32_t)next_x;
            store->objects[index].path_points[i].y = (int32_t)next_y;
        }
    }
    store->objects[index].origin_x = origin_x;
    store->objects[index].origin_y = origin_y;
    return core_result_ok();
}

CoreResult drawing_program_object_store_set_path_payload(DrawingProgramObjectStore *store,
                                                         uint32_t object_id,
                                                         const DrawingProgramPathPayload *payload) {
    uint32_t index = 0u;
    CoreResult result;
    int32_t origin_x = 0;
    int32_t origin_y = 0;
    uint32_t width = 0u;
    uint32_t height = 0u;
    if (!store || !payload || object_id == 0u) {
        return object_store_invalid("invalid object path-payload set request");
    }
    result = drawing_program_object_store_find_index_for_id(store, object_id, &index);
    if (result.code != CORE_OK) {
        return result;
    }
    if (store->objects[index].type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
        return object_store_invalid("path payload set requires PATH object type");
    }
    result = object_path_payload_bounds(payload, &origin_x, &origin_y, &width, &height);
    if (result.code != CORE_OK) {
        return result;
    }
    memset(store->objects[index].path_points, 0, sizeof(store->objects[index].path_points));
    memcpy(store->objects[index].path_points,
           payload->points,
           (size_t)payload->point_count * sizeof(store->objects[index].path_points[0]));
    store->objects[index].path_point_count = (uint16_t)payload->point_count;
    store->objects[index].path_closed = payload->closed ? 1u : 0u;
    store->objects[index].origin_x = origin_x;
    store->objects[index].origin_y = origin_y;
    store->objects[index].width = width;
    store->objects[index].height = height;
    return core_result_ok();
}

CoreResult drawing_program_object_store_set_path_point(DrawingProgramObjectStore *store,
                                                       uint32_t object_id,
                                                       uint16_t point_index,
                                                       int32_t point_x,
                                                       int32_t point_y,
                                                       int32_t *out_previous_x,
                                                       int32_t *out_previous_y) {
    DrawingProgramPathPayload payload;
    CoreResult result;
    if (!store || object_id == 0u) {
        return object_store_invalid("invalid object path-point set request");
    }
    result = drawing_program_object_store_get_path_payload(store, object_id, &payload);
    if (result.code != CORE_OK) {
        return result;
    }
    if ((uint32_t)point_index >= payload.point_count) {
        return object_store_invalid("path-point index out of range");
    }
    if (out_previous_x) {
        *out_previous_x = payload.points[point_index].x;
    }
    if (out_previous_y) {
        *out_previous_y = payload.points[point_index].y;
    }
    payload.points[point_index].x = point_x;
    payload.points[point_index].y = point_y;
    return drawing_program_object_store_set_path_payload(store, object_id, &payload);
}

CoreResult drawing_program_object_store_get_path_payload(const DrawingProgramObjectStore *store,
                                                         uint32_t object_id,
                                                         DrawingProgramPathPayload *out_payload) {
    uint32_t index = 0u;
    if (!store || !out_payload || object_id == 0u) {
        return object_store_invalid("invalid object path-payload get request");
    }
    if (drawing_program_object_store_find_index_for_id(store, object_id, &index).code != CORE_OK) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "object id not found for path payload" };
    }
    if (store->objects[index].type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
        return object_store_invalid("path payload get requires PATH object type");
    }
    memset(out_payload, 0, sizeof(*out_payload));
    out_payload->point_count = (uint32_t)store->objects[index].path_point_count;
    out_payload->closed = store->objects[index].path_closed ? 1u : 0u;
    if (out_payload->point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
        return (CoreResult){ CORE_ERR_FORMAT, "stored path payload exceeds max points" };
    }
    memcpy(out_payload->points,
           store->objects[index].path_points,
           (size_t)out_payload->point_count * sizeof(out_payload->points[0]));
    return core_result_ok();
}

CoreResult drawing_program_object_store_hit_test_selected_path_point(
    const DrawingProgramObjectStore *store,
    const DrawingProgramDocument *document,
    const DrawingProgramObjectSelectionState *selection,
    uint32_t sample_x,
    uint32_t sample_y,
    uint32_t pick_radius_samples,
    uint32_t *out_object_id,
    uint16_t *out_point_index) {
    int64_t threshold_sq;
    uint32_t i;
    if (!store || !selection || !out_object_id || !out_point_index) {
        return object_store_invalid("invalid selected path-point hit-test request");
    }
    if (selection->count == 0u) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "no selected objects for path-point hit-test" };
    }
    if (pick_radius_samples == 0u) {
        pick_radius_samples = 4u;
    }
    threshold_sq = (int64_t)pick_radius_samples * (int64_t)pick_radius_samples;
    for (i = store->object_count; i > 0u; --i) {
        uint32_t object_index = i - 1u;
        const DrawingProgramObjectRecord *object = &store->objects[object_index];
        uint32_t point_i = 0u;
        int64_t best_sq = LLONG_MAX;
        uint16_t best_index = 0u;
        if (object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
            object->path_point_count == 0u ||
            !selection_contains_object_id(selection, object->object_id) ||
            !object->visible ||
            object->locked ||
            !object_layer_allows_interaction(document, object->layer_id)) {
            continue;
        }
        for (point_i = 0u; point_i < (uint32_t)object->path_point_count; ++point_i) {
            int64_t dx = (int64_t)object->path_points[point_i].x - (int64_t)sample_x;
            int64_t dy = (int64_t)object->path_points[point_i].y - (int64_t)sample_y;
            int64_t dist_sq = (dx * dx) + (dy * dy);
            if (dist_sq > threshold_sq || dist_sq >= best_sq) {
                continue;
            }
            best_sq = dist_sq;
            best_index = (uint16_t)point_i;
            if (best_sq == 0) {
                break;
            }
        }
        if (best_sq <= threshold_sq) {
            *out_object_id = object->object_id;
            *out_point_index = best_index;
            return core_result_ok();
        }
    }
    return (CoreResult){ CORE_ERR_NOT_FOUND, "no selected path point hit for sample" };
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

CoreResult drawing_program_object_store_promote_selection(DrawingProgramObjectStore *store,
                                                          const DrawingProgramObjectSelectionState *selection) {
    DrawingProgramObjectRecord reordered[DRAWING_PROGRAM_MAX_OBJECTS];
    uint32_t write_index = 0u;
    uint32_t i;
    if (!store || !selection) {
        return object_store_invalid("invalid object selection promote request");
    }
    if (selection->count == 0u || store->object_count <= 1u) {
        return core_result_ok();
    }
    memset(reordered, 0, sizeof(reordered));
    for (i = 0u; i < store->object_count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        const DrawingProgramObjectRecord *object = &store->objects[i];
        if (selection_contains_object_id(selection, object->object_id)) {
            continue;
        }
        reordered[write_index++] = *object;
    }
    for (i = 0u; i < store->object_count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        const DrawingProgramObjectRecord *object = &store->objects[i];
        if (!selection_contains_object_id(selection, object->object_id)) {
            continue;
        }
        reordered[write_index++] = *object;
    }
    if (write_index != store->object_count) {
        return object_store_invalid("object selection promote reorder mismatch");
    }
    memcpy(store->objects, reordered, sizeof(reordered));
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
            if (!object_layer_allows_interaction(document, object->layer_id)) {
                continue;
            }
        }
        if (object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_ELLIPSE) {
            if (!object_ellipse_contains(object, sample_x, sample_y)) {
                continue;
            }
        } else if (object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
            if (!object_path_contains(object, sample_x, sample_y)) {
                continue;
            }
        } else if (!object_bounds_contains(object, sample_x, sample_y)) {
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
