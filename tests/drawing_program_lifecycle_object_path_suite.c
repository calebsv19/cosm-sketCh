#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_object_geometry.h"
#include "drawing_program/drawing_program_object_transform.h"
#include "drawing_program/drawing_program_visual_transform_ops.h"
#include "drawing_program_lifecycle_object_path_history_suite.h"
#include "drawing_program_lifecycle_object_path_suite.h"
#include "drawing_program_lifecycle_test_support.h"

static CoreResult lifecycle_test_object_commit_move(DrawingProgramAppContext *ctx,
                                                    int32_t requested_dx,
                                                    int32_t requested_dy) {
    return drawing_program_object_transform_commit_move(&ctx->history,
                                                        &ctx->object_store,
                                                        &ctx->document,
                                                        &ctx->object_selection,
                                                        requested_dx,
                                                        requested_dy,
                                                        0,
                                                        0);
}

int drawing_program_lifecycle_run_object_path_suite(DrawingProgramAppContext *workflow_ctx_ptr,
                                                    uint8_t expected_draw_value,
                                                    uint8_t expected_eraser_value)
{
    (void)expected_draw_value;
    (void)expected_eraser_value;
#define workflow_ctx (*workflow_ctx_ptr)

    {
        DrawingProgramObjectStore hit_store;
        DrawingProgramObjectRecord seed_object;
        DrawingProgramObjectSelectionState object_selection;
        DrawingProgramSelectionState raster_selection_before;
        DrawingProgramDocument hit_document;
        CoreResult hit_result;
        uint32_t hit_object_id = 0u;
        uint32_t back_id = 0u;
        uint32_t mid_id = 0u;
        uint32_t top_id = 0u;
        uint32_t object_index = 0u;
        drawing_program_object_store_reset(&hit_store);
        memset(&seed_object, 0, sizeof(seed_object));
        seed_object.layer_id = workflow_ctx.document.layers[0].layer_id;
        seed_object.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT;
        seed_object.visible = 1u;
        seed_object.locked = 0u;
        seed_object.origin_x = 40;
        seed_object.origin_y = 40;
        seed_object.width = 48u;
        seed_object.height = 48u;
        if (!expect_ok(drawing_program_object_store_add(&hit_store, &seed_object, &back_id), "object_hit_add_back")) {
            return 1;
        }
        if (!expect_ok(drawing_program_object_store_add(&hit_store, &seed_object, &mid_id), "object_hit_add_mid")) {
            return 1;
        }
        if (!expect_ok(drawing_program_object_store_add(&hit_store, &seed_object, &top_id), "object_hit_add_top")) {
            return 1;
        }

        hit_document = workflow_ctx.document;
        hit_result = drawing_program_object_store_hit_test_topmost(
            &hit_store, &hit_document, 44u, 44u, &hit_object_id, 0);
        if (hit_result.code != CORE_OK || hit_object_id != top_id) {
            fprintf(stderr,
                    "lifecycle_test: expected topmost object hit id=%u got code=%d id=%u\n",
                    (unsigned)top_id,
                    (int)hit_result.code,
                    (unsigned)hit_object_id);
            return 1;
        }
        if (!expect_ok(drawing_program_object_store_find_index_for_id(&hit_store, top_id, &object_index),
                       "object_hit_find_top")) {
            return 1;
        }
        hit_store.objects[object_index].visible = 0u;
        if (!expect_ok(drawing_program_object_store_find_index_for_id(&hit_store, mid_id, &object_index),
                       "object_hit_find_mid")) {
            return 1;
        }
        hit_store.objects[object_index].locked = 1u;
        hit_result = drawing_program_object_store_hit_test_topmost(
            &hit_store, &hit_document, 44u, 44u, &hit_object_id, 0);
        if (hit_result.code != CORE_OK || hit_object_id != back_id) {
            fprintf(stderr,
                    "lifecycle_test: expected fallback visible unlocked hit id=%u got code=%d id=%u\n",
                    (unsigned)back_id,
                    (int)hit_result.code,
                    (unsigned)hit_object_id);
            return 1;
        }
        hit_document.layers[0].visible = 0u;
        hit_result = drawing_program_object_store_hit_test_topmost(
            &hit_store, &hit_document, 44u, 44u, &hit_object_id, 0);
        if (hit_result.code != CORE_ERR_NOT_FOUND) {
            fprintf(stderr,
                    "lifecycle_test: expected no hit when owner layer hidden got code=%d id=%u\n",
                    (int)hit_result.code,
                    (unsigned)hit_object_id);
            return 1;
        }

        drawing_program_object_selection_reset(&object_selection);
        drawing_program_object_selection_replace_single(&object_selection, back_id);
        if (object_selection.count != 1u ||
            object_selection.active_object_id != back_id ||
            object_selection.object_ids[0] != back_id) {
            fprintf(stderr, "lifecycle_test: object selection replace contract failed\n");
            return 1;
        }
        if (!drawing_program_object_selection_add(&object_selection, top_id) ||
            object_selection.count != 2u ||
            object_selection.active_object_id != top_id ||
            !drawing_program_object_selection_contains(&object_selection, back_id) ||
            !drawing_program_object_selection_contains(&object_selection, top_id)) {
            fprintf(stderr, "lifecycle_test: object selection additive contract failed\n");
            return 1;
        }
        if (drawing_program_object_selection_add(&object_selection, back_id) ||
            object_selection.count != 2u ||
            object_selection.active_object_id != back_id) {
            fprintf(stderr, "lifecycle_test: object selection duplicate-add contract failed\n");
            return 1;
        }
        drawing_program_object_selection_reset(&object_selection);
        if (object_selection.count != 0u || object_selection.active_object_id != 0u) {
            fprintf(stderr, "lifecycle_test: object selection clear contract failed\n");
            return 1;
        }

        raster_selection_before = workflow_ctx.selection;
        drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, back_id);
        (void)drawing_program_object_selection_add(&workflow_ctx.object_selection, mid_id);
        if (memcmp(&raster_selection_before, &workflow_ctx.selection, sizeof(raster_selection_before)) != 0) {
            fprintf(stderr, "lifecycle_test: object selection mutation should not mutate raster selection state\n");
            return 1;
        }
    }
    {
        DrawingProgramObjectStore shape_store;
        DrawingProgramObjectRecord shape_seed;
        DrawingProgramHistory history;
        const DrawingProgramObjectRecord *shape_object = 0;
        CoreResult result;
        uint32_t object_id = 0u;
        drawing_program_object_store_reset(&shape_store);
        drawing_program_history_init(&history);
        memset(&shape_seed, 0, sizeof(shape_seed));
        shape_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
        shape_seed.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT;
        shape_seed.visible = 1u;
        shape_seed.locked = 0u;
        shape_seed.origin_x = 20;
        shape_seed.origin_y = 24;
        shape_seed.width = 32u;
        shape_seed.height = 20u;
        shape_seed.stroke_width = 2u;
        shape_seed.style_mode = 2u;
        if (!expect_ok(drawing_program_object_store_add(&shape_store, &shape_seed, &object_id),
                       "shape_size_seed_add")) {
            return 1;
        }
        result = drawing_program_history_apply_set_object_size(&history, &shape_store, object_id, 48u, 28u);
        if (!expect_ok(result, "shape_size_apply")) {
            return 1;
        }
        shape_object = drawing_program_object_store_get_by_id(&shape_store, object_id);
        if (!shape_object || shape_object->width != 48u || shape_object->height != 28u) {
            fprintf(stderr,
                    "lifecycle_test: shape size apply failed width=%u height=%u\n",
                    shape_object ? (unsigned)shape_object->width : 0u,
                    shape_object ? (unsigned)shape_object->height : 0u);
            return 1;
        }
        if (!expect_ok(drawing_program_history_undo(&history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &shape_store),
                       "shape_size_undo")) {
            return 1;
        }
        shape_object = drawing_program_object_store_get_by_id(&shape_store, object_id);
        if (!shape_object || shape_object->width != 32u || shape_object->height != 20u) {
            fprintf(stderr,
                    "lifecycle_test: shape size undo failed width=%u height=%u\n",
                    shape_object ? (unsigned)shape_object->width : 0u,
                    shape_object ? (unsigned)shape_object->height : 0u);
            return 1;
        }
        if (!expect_ok(drawing_program_history_redo(&history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &shape_store),
                       "shape_size_redo")) {
            return 1;
        }
        shape_object = drawing_program_object_store_get_by_id(&shape_store, object_id);
        if (!shape_object || shape_object->width != 48u || shape_object->height != 28u) {
            fprintf(stderr,
                    "lifecycle_test: shape size redo failed width=%u height=%u\n",
                    shape_object ? (unsigned)shape_object->width : 0u,
                    shape_object ? (unsigned)shape_object->height : 0u);
            return 1;
        }
    }
    {
        DrawingProgramObjectStore path_hit_store;
        DrawingProgramObjectRecord style_seed;
        DrawingProgramPathPayload path_payload;
        DrawingProgramDocument path_hit_document;
        CoreResult hit_result;
        uint32_t fill_id = 0u;
        uint32_t outline_id = 0u;
        uint32_t hit_object_id = 0u;
        uint16_t hit_point_index = 0u;
        DrawingProgramObjectSelectionState path_selection;

        drawing_program_object_store_reset(&path_hit_store);
        memset(&style_seed, 0, sizeof(style_seed));
        memset(&path_payload, 0, sizeof(path_payload));
        path_hit_document = workflow_ctx.document;

        style_seed.layer_id = path_hit_document.layers[0].layer_id;
        style_seed.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH;
        style_seed.visible = 1u;
        style_seed.locked = 0u;
        style_seed.stroke_color_value = drawing_program_color_value_from_index(0u);
        style_seed.fill_color_value = drawing_program_color_value_from_index(7u);
        style_seed.stroke_width = 2u;
        style_seed.style_mode = 1u; /* fill-only */

        path_payload.point_count = 3u;
        path_payload.closed = 1u;
        path_payload.points[0].x = 140;
        path_payload.points[0].y = 40;
        path_payload.points[1].x = 188;
        path_payload.points[1].y = 40;
        path_payload.points[2].x = 164;
        path_payload.points[2].y = 88;
        if (!expect_ok(drawing_program_object_store_add_path(&path_hit_store, &style_seed, &path_payload, &fill_id),
                       "path_hit_add_fill")) {
            return 1;
        }
        hit_result = drawing_program_object_store_hit_test_topmost(
            &path_hit_store, &path_hit_document, 164u, 56u, &hit_object_id, 0);
        if (hit_result.code != CORE_OK || hit_object_id != fill_id) {
            fprintf(stderr,
                    "lifecycle_test: expected fill path hit id=%u got code=%d id=%u\n",
                    (unsigned)fill_id,
                    (int)hit_result.code,
                    (unsigned)hit_object_id);
            return 1;
        }
        hit_result = drawing_program_object_store_hit_test_topmost(
            &path_hit_store, &path_hit_document, 142u, 84u, &hit_object_id, 0);
        if (hit_result.code != CORE_ERR_NOT_FOUND) {
            fprintf(stderr,
                    "lifecycle_test: expected fill path miss outside polygon got code=%d id=%u\n",
                    (int)hit_result.code,
                    (unsigned)hit_object_id);
            return 1;
        }

        style_seed.style_mode = 0u; /* outline-only */
        path_payload.point_count = 2u;
        path_payload.closed = 0u;
        path_payload.points[0].x = 132;
        path_payload.points[0].y = 120;
        path_payload.points[1].x = 188;
        path_payload.points[1].y = 120;
        if (!expect_ok(
                drawing_program_object_store_add_path(&path_hit_store, &style_seed, &path_payload, &outline_id),
                "path_hit_add_outline")) {
            return 1;
        }
        hit_result = drawing_program_object_store_hit_test_topmost(
            &path_hit_store, &path_hit_document, 160u, 120u, &hit_object_id, 0);
        if (hit_result.code != CORE_OK || hit_object_id != outline_id) {
            fprintf(stderr,
                    "lifecycle_test: expected outline path hit id=%u got code=%d id=%u\n",
                    (unsigned)outline_id,
                    (int)hit_result.code,
                    (unsigned)hit_object_id);
            return 1;
        }
        if (!expect_ok(drawing_program_object_store_set_origin(&path_hit_store, outline_id, 152, 140, 0, 0),
                       "path_hit_outline_move_origin")) {
            return 1;
        }
        hit_result = drawing_program_object_store_hit_test_topmost(
            &path_hit_store, &path_hit_document, 160u, 120u, &hit_object_id, 0);
        if (hit_result.code != CORE_ERR_NOT_FOUND) {
            fprintf(stderr,
                    "lifecycle_test: expected moved outline path to miss old location got code=%d id=%u\n",
                    (int)hit_result.code,
                    (unsigned)hit_object_id);
            return 1;
        }
        hit_result = drawing_program_object_store_hit_test_topmost(
            &path_hit_store, &path_hit_document, 160u, 140u, &hit_object_id, 0);
        if (hit_result.code != CORE_OK || hit_object_id != outline_id) {
            fprintf(stderr,
                    "lifecycle_test: expected moved outline path hit id=%u got code=%d id=%u\n",
                    (unsigned)outline_id,
                    (int)hit_result.code,
                    (unsigned)hit_object_id);
            return 1;
        }
        drawing_program_object_selection_reset(&path_selection);
        drawing_program_object_selection_replace_single(&path_selection, outline_id);
        hit_result = drawing_program_object_store_hit_test_selected_path_point(
            &path_hit_store, &path_hit_document, &path_selection, 152u, 140u, 4u, &hit_object_id, &hit_point_index);
        if (hit_result.code != CORE_OK || hit_object_id != outline_id || hit_point_index != 0u) {
            fprintf(stderr,
                    "lifecycle_test: expected selected path-point hit id=%u idx=0 got code=%d id=%u idx=%u\n",
                    (unsigned)outline_id,
                    (int)hit_result.code,
                    (unsigned)hit_object_id,
                    (unsigned)hit_point_index);
            return 1;
        }
        hit_result = drawing_program_object_store_hit_test_selected_path_edge(&path_hit_store,
                                                                              &path_hit_document,
                                                                              &path_selection,
                                                                              170u,
                                                                              140u,
                                                                              6u,
                                                                              &hit_object_id,
                                                                              &hit_point_index,
                                                                              &path_payload.points[0].x,
                                                                              &path_payload.points[0].y);
        if (hit_result.code != CORE_OK || hit_object_id != outline_id || hit_point_index != 1u ||
            path_payload.points[0].x != 170 || path_payload.points[0].y != 140) {
            fprintf(stderr,
                    "lifecycle_test: expected selected path-edge hit id=%u idx=1 point=(170,140) got code=%d id=%u idx=%u point=(%d,%d)\n",
                    (unsigned)outline_id,
                    (int)hit_result.code,
                    (unsigned)hit_object_id,
                    (unsigned)hit_point_index,
                    (int)path_payload.points[0].x,
                    (int)path_payload.points[0].y);
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord obj_a;
        DrawingProgramObjectRecord obj_b;
        DrawingProgramObjectRecord obj_a_after_nudge;
        DrawingProgramObjectRecord obj_b_after_nudge;
        const DrawingProgramObjectRecord *obj = 0;
        DrawingProgramVisualTransformOpsHooks hooks;
        VisualCanvasInteractionState interaction;
        uint32_t id_a = 0u;
        uint32_t id_b = 0u;
        uint32_t unit_cursor_before = 0u;
        uint32_t unit_count_before = 0u;
        uint32_t unit_cursor_after = 0u;
        uint32_t unit_count_after = 0u;
        int32_t clamped_dx = 0;
        int32_t clamped_dy = 0;
        CoreResult result;

        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&obj_a, 0, sizeof(obj_a));
        memset(&obj_b, 0, sizeof(obj_b));
        obj_a.layer_id = workflow_ctx.document.layers[0].layer_id;
        obj_a.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT;
        obj_a.visible = 1u;
        obj_a.origin_x = 20;
        obj_a.origin_y = 20;
        obj_a.width = 24u;
        obj_a.height = 24u;
        obj_b = obj_a;
        obj_b.origin_x = 80;
        obj_b.origin_y = 36;
        if (!expect_ok(drawing_program_object_store_add(&workflow_ctx.object_store, &obj_a, &id_a),
                       "object_move_seed_add_a")) {
            return 1;
        }
        if (!expect_ok(drawing_program_object_store_add(&workflow_ctx.object_store, &obj_b, &id_b),
                       "object_move_seed_add_b")) {
            return 1;
        }
        drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, id_a);
        (void)drawing_program_object_selection_add(&workflow_ctx.object_selection, id_b);
        drawing_program_history_query_units(&workflow_ctx.history, &unit_cursor_before, &unit_count_before);
        memset(&hooks, 0, sizeof(hooks));
        hooks.visual_object_commit_move = lifecycle_test_object_commit_move;
        memset(&interaction, 0, sizeof(interaction));
        if (!expect_ok(drawing_program_visual_transform_session_nudge_object_move(
                           &workflow_ctx, &interaction, 4, -2, &hooks),
                       "object_move_nudge_commit")) {
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &unit_cursor_after, &unit_count_after);
        if (unit_count_after != unit_count_before + 1u || unit_cursor_after != unit_cursor_before + 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected object nudge to create one grouped history unit before=%u/%u after=%u/%u\n",
                    (unsigned)unit_cursor_before,
                    (unsigned)unit_count_before,
                    (unsigned)unit_cursor_after,
                    (unsigned)unit_count_after);
            return 1;
        }
        obj = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, id_a);
        if (!obj) {
            fprintf(stderr, "lifecycle_test: expected object A after nudge\n");
            return 1;
        }
        obj_a_after_nudge = *obj;
        obj = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, id_b);
        if (!obj) {
            fprintf(stderr, "lifecycle_test: expected object B after nudge\n");
            return 1;
        }
        obj_b_after_nudge = *obj;
        if (obj_a_after_nudge.origin_x != 24 || obj_a_after_nudge.origin_y != 18 ||
            obj_b_after_nudge.origin_x != 84 || obj_b_after_nudge.origin_y != 34) {
            fprintf(stderr,
                    "lifecycle_test: unexpected object nudge result A=%d,%d B=%d,%d\n",
                    (int)obj_a_after_nudge.origin_x,
                    (int)obj_a_after_nudge.origin_y,
                    (int)obj_b_after_nudge.origin_x,
                    (int)obj_b_after_nudge.origin_y);
            return 1;
        }
        if (!expect_ok(drawing_program_history_undo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_move_undo")) {
            return 1;
        }
        obj = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, id_a);
        if (!obj || obj->origin_x != 20 || obj->origin_y != 20) {
            fprintf(stderr, "lifecycle_test: object undo failed for A\n");
            return 1;
        }
        obj = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, id_b);
        if (!obj || obj->origin_x != 80 || obj->origin_y != 36) {
            fprintf(stderr, "lifecycle_test: object undo failed for B\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_redo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_move_redo")) {
            return 1;
        }
        obj = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, id_a);
        if (!obj || obj->origin_x != obj_a_after_nudge.origin_x || obj->origin_y != obj_a_after_nudge.origin_y) {
            fprintf(stderr, "lifecycle_test: object redo failed for A\n");
            return 1;
        }
        obj = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, id_b);
        if (!obj || obj->origin_x != obj_b_after_nudge.origin_x || obj->origin_y != obj_b_after_nudge.origin_y) {
            fprintf(stderr, "lifecycle_test: object redo failed for B\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_undo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_move_undo_before_drag_parity")) {
            return 1;
        }
        drawing_program_visual_transform_session_begin_object_move(&interaction, 10u, 10u);
        drawing_program_visual_transform_session_update_object_move(
            &workflow_ctx, &interaction, 14u, 8u, (SDL_Keymod)0);
        if (!expect_ok(drawing_program_visual_transform_session_commit_object_move(&workflow_ctx, &interaction, &hooks),
                       "object_move_drag_commit")) {
            return 1;
        }
        obj = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, id_a);
        if (!obj || obj->origin_x != obj_a_after_nudge.origin_x || obj->origin_y != obj_a_after_nudge.origin_y) {
            fprintf(stderr, "lifecycle_test: object drag parity mismatch for A\n");
            return 1;
        }
        obj = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, id_b);
        if (!obj || obj->origin_x != obj_b_after_nudge.origin_x || obj->origin_y != obj_b_after_nudge.origin_y) {
            fprintf(stderr, "lifecycle_test: object drag parity mismatch for B\n");
            return 1;
        }
        result = drawing_program_object_store_clamp_selection_delta(&workflow_ctx.object_store,
                                                                    &workflow_ctx.document,
                                                                    &workflow_ctx.object_selection,
                                                                    -1000,
                                                                    -1000,
                                                                    &clamped_dx,
                                                                    &clamped_dy);
        if (result.code != CORE_OK || clamped_dx >= 0 || clamped_dy >= 0) {
            fprintf(stderr,
                    "lifecycle_test: expected object delta clamp bounds to resolve negative cap got code=%d dx=%d dy=%d\n",
                    (int)result.code,
                    (int)clamped_dx,
                    (int)clamped_dy);
            return 1;
        }
    }
    if (drawing_program_lifecycle_run_object_path_history_suite(&workflow_ctx) != 0) {
        return 1;
    }

#undef workflow_ctx
    return 0;
}
