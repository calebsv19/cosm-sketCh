#include "drawing_program/drawing_program_object_store.h"

#include <float.h>
#include <limits.h>
#include <math.h>

#include "drawing_program/drawing_program_object_geometry.h"
#include "drawing_program_object_store_internal.h"

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
        return drawing_program_object_store_invalid("invalid selected path-point hit-test request");
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
            !drawing_program_object_store_selection_contains_object_id(selection, object->object_id) ||
            !object->visible ||
            object->locked ||
            !drawing_program_object_layer_allows_interaction(document, object->layer_id)) {
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

CoreResult drawing_program_object_store_hit_test_selected_path_handle(
    const DrawingProgramObjectStore *store,
    const DrawingProgramDocument *document,
    const DrawingProgramObjectSelectionState *selection,
    uint32_t sample_x,
    uint32_t sample_y,
    uint32_t pick_radius_samples,
    uint32_t *out_object_id,
    uint16_t *out_point_index,
    uint8_t *out_handle_kind) {
    const DrawingProgramObjectRecord *object = 0;
    const DrawingProgramPathPoint *point = 0;
    int64_t threshold_sq;
    int64_t best_sq = LLONG_MAX;
    uint8_t best_kind = (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_NONE;
    int64_t dx = 0;
    int64_t dy = 0;
    int32_t handle_x = 0;
    int32_t handle_y = 0;
    if (!store || !selection || !out_object_id || !out_point_index || !out_handle_kind) {
        return drawing_program_object_store_invalid("invalid selected path-handle hit-test request");
    }
    if (!selection->selected_path_point_active || selection->selected_path_point_object_id == 0u) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "no selected path point for handle hit-test" };
    }
    if (pick_radius_samples == 0u) {
        pick_radius_samples = 4u;
    }
    object = drawing_program_object_store_get_by_id(store, selection->selected_path_point_object_id);
    if (!object || object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
        selection->selected_path_point_index >= object->path_point_count ||
        !drawing_program_object_store_selection_contains_object_id(selection, object->object_id) ||
        !object->visible ||
        object->locked ||
        !drawing_program_object_layer_allows_interaction(document, object->layer_id)) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "selected path handle target missing" };
    }
    point = &object->path_points[selection->selected_path_point_index];
    if (!point->bezier_enabled) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "selected path point is linear" };
    }
    threshold_sq = (int64_t)pick_radius_samples * (int64_t)pick_radius_samples;
    if (point->handle_in_dx != 0 || point->handle_in_dy != 0) {
        handle_x = point->x + point->handle_in_dx;
        handle_y = point->y + point->handle_in_dy;
        dx = (int64_t)handle_x - (int64_t)sample_x;
        dy = (int64_t)handle_y - (int64_t)sample_y;
        best_sq = (dx * dx) + (dy * dy);
        if (best_sq <= threshold_sq) {
            best_kind = (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_IN;
        }
    }
    if (point->handle_out_dx != 0 || point->handle_out_dy != 0) {
        int64_t out_sq;
        handle_x = point->x + point->handle_out_dx;
        handle_y = point->y + point->handle_out_dy;
        dx = (int64_t)handle_x - (int64_t)sample_x;
        dy = (int64_t)handle_y - (int64_t)sample_y;
        out_sq = (dx * dx) + (dy * dy);
        if (out_sq <= threshold_sq && out_sq < best_sq) {
            best_sq = out_sq;
            best_kind = (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_OUT;
        }
    }
    if (best_kind == (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_NONE) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "no selected path handle hit for sample" };
    }
    *out_object_id = object->object_id;
    *out_point_index = selection->selected_path_point_index;
    *out_handle_kind = best_kind;
    return core_result_ok();
}

CoreResult drawing_program_object_store_hit_test_selected_path_edge(
    const DrawingProgramObjectStore *store,
    const DrawingProgramDocument *document,
    const DrawingProgramObjectSelectionState *selection,
    uint32_t sample_x,
    uint32_t sample_y,
    uint32_t pick_radius_samples,
    uint32_t *out_object_id,
    uint16_t *out_insert_index,
    int32_t *out_point_x,
    int32_t *out_point_y) {
    double threshold_sq;
    uint32_t i;
    if (!store || !selection || !out_object_id || !out_insert_index || !out_point_x || !out_point_y) {
        return drawing_program_object_store_invalid("invalid selected path-edge hit-test request");
    }
    if (selection->count == 0u || selection->active_object_id == 0u) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "no selected objects for path-edge hit-test" };
    }
    if (pick_radius_samples == 0u) {
        pick_radius_samples = 4u;
    }
    threshold_sq = (double)pick_radius_samples * (double)pick_radius_samples;
    for (i = store->object_count; i > 0u; --i) {
        uint32_t object_index = i - 1u;
        const DrawingProgramObjectRecord *object = &store->objects[object_index];
        uint32_t point_count;
        uint32_t segment_count;
        uint32_t seg_i;
        double best_sq = DBL_MAX;
        uint16_t best_insert_index = 0u;
        double best_proj_x = 0.0;
        double best_proj_y = 0.0;
        if (object->object_id != selection->active_object_id ||
            object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
            object->path_point_count < 2u ||
            !object->visible ||
            object->locked ||
            !drawing_program_object_layer_allows_interaction(document, object->layer_id)) {
            continue;
        }
        point_count = (uint32_t)object->path_point_count;
        segment_count = point_count - 1u;
        if (object->path_closed) {
            segment_count += 1u;
        }
        for (seg_i = 0u; seg_i < segment_count; ++seg_i) {
            uint32_t a = seg_i;
            uint32_t b = (seg_i + 1u < point_count) ? (seg_i + 1u) : 0u;
            double proj_x = 0.0;
            double proj_y = 0.0;
            double dist_sq = drawing_program_object_path_point_segment_distance_sq(
                &object->path_points[a], &object->path_points[b], (double)sample_x + 0.5, (double)sample_y + 0.5);
            if (dist_sq > threshold_sq || dist_sq >= best_sq) {
                continue;
            }
            drawing_program_object_path_project_on_point_segment(
                &object->path_points[a], &object->path_points[b], (double)sample_x + 0.5, (double)sample_y + 0.5, &proj_x, &proj_y);
            best_sq = dist_sq;
            best_insert_index = (b == 0u) ? (uint16_t)point_count : (uint16_t)b;
            best_proj_x = proj_x;
            best_proj_y = proj_y;
        }
        if (best_sq <= threshold_sq) {
            *out_object_id = object->object_id;
            *out_insert_index = best_insert_index;
            *out_point_x = (int32_t)lround(best_proj_x - 0.5);
            *out_point_y = (int32_t)lround(best_proj_y - 0.5);
            return core_result_ok();
        }
    }
    return (CoreResult){ CORE_ERR_NOT_FOUND, "no selected path edge hit for sample" };
}

CoreResult drawing_program_object_store_resolve_selected_open_path_append_target(
    const DrawingProgramObjectStore *store,
    const DrawingProgramDocument *document,
    const DrawingProgramObjectSelectionState *selection,
    uint32_t sample_x,
    uint32_t sample_y,
    uint32_t *out_object_id,
    uint16_t *out_insert_index) {
    const DrawingProgramObjectRecord *object = 0;
    uint16_t selected_point_index = 0u;
    int has_selected_point = 0;
    int64_t first_dx;
    int64_t first_dy;
    int64_t last_dx;
    int64_t last_dy;
    int64_t first_dist_sq;
    int64_t last_dist_sq;
    if (!store || !selection || !out_object_id || !out_insert_index) {
        return drawing_program_object_store_invalid("invalid selected open path append target request");
    }
    if (selection->count == 0u || selection->active_object_id == 0u) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "no selected active object for path append target" };
    }
    object = drawing_program_object_store_get_by_id(store, selection->active_object_id);
    if (!object ||
        object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
        object->path_point_count < 2u ||
        object->path_closed ||
        !object->visible ||
        object->locked ||
        !drawing_program_object_layer_allows_interaction(document, object->layer_id)) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "selected active object is not an editable open path" };
    }
    has_selected_point = drawing_program_object_selection_get_path_point(
        selection, 0, &selected_point_index);
    if (has_selected_point &&
        selection->selected_path_point_object_id == object->object_id) {
        if (selected_point_index == 0u) {
            *out_object_id = object->object_id;
            *out_insert_index = 0u;
            return core_result_ok();
        }
        if ((uint32_t)selected_point_index + 1u == (uint32_t)object->path_point_count) {
            *out_object_id = object->object_id;
            *out_insert_index = object->path_point_count;
            return core_result_ok();
        }
    }
    first_dx = (int64_t)object->path_points[0].x - (int64_t)sample_x;
    first_dy = (int64_t)object->path_points[0].y - (int64_t)sample_y;
    last_dx = (int64_t)object->path_points[object->path_point_count - 1u].x - (int64_t)sample_x;
    last_dy = (int64_t)object->path_points[object->path_point_count - 1u].y - (int64_t)sample_y;
    first_dist_sq = (first_dx * first_dx) + (first_dy * first_dy);
    last_dist_sq = (last_dx * last_dx) + (last_dy * last_dy);
    *out_object_id = object->object_id;
    *out_insert_index = (first_dist_sq <= last_dist_sq) ? 0u : object->path_point_count;
    return core_result_ok();
}
