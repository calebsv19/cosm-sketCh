#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_object_transform.h"
#include "drawing_program/drawing_program_visual_transform_ops.h"
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
        style_seed.stroke_color_index = 0u;
        style_seed.fill_color_index = 7u;
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
    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        const DrawingProgramObjectRecord *path_object = 0;
        uint32_t path_object_id = 0u;
        uint32_t unit_cursor_before = 0u;
        uint32_t unit_count_before = 0u;
        uint32_t unit_cursor_after = 0u;
        uint32_t unit_count_after = 0u;

        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&path_seed, 0, sizeof(path_seed));
        memset(&path_payload, 0, sizeof(path_payload));
        path_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
        path_seed.visible = 1u;
        path_seed.locked = 0u;
        path_seed.stroke_color_index = 6u;
        path_seed.fill_color_index = 6u;
        path_seed.stroke_width = 2u;
        path_seed.style_mode = 0u;
        path_payload.point_count = 3u;
        path_payload.closed = 1u;
        path_payload.points[0].x = 64;
        path_payload.points[0].y = 64;
        path_payload.points[1].x = 96;
        path_payload.points[1].y = 64;
        path_payload.points[2].x = 80;
        path_payload.points[2].y = 96;
        if (!expect_ok(drawing_program_object_store_add_path(
                           &workflow_ctx.object_store, &path_seed, &path_payload, &path_object_id),
                       "path_point_move_seed_add")) {
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &unit_cursor_before, &unit_count_before);
        if (!expect_ok(drawing_program_history_apply_set_object_path_point(
                           &workflow_ctx.history, &workflow_ctx.object_store, path_object_id, 1u, 110, 70),
                       "path_point_move_history_apply")) {
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &unit_cursor_after, &unit_count_after);
        if (unit_count_after != unit_count_before + 1u ||
            unit_cursor_after != unit_cursor_before + 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected path-point edit one history unit before=%u/%u after=%u/%u\n",
                    (unsigned)unit_cursor_before,
                    (unsigned)unit_count_before,
                    (unsigned)unit_cursor_after,
                    (unsigned)unit_count_after);
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, path_object_id);
        if (!path_object ||
            path_object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
            path_object->path_points[1].x != 110 ||
            path_object->path_points[1].y != 70) {
            fprintf(stderr, "lifecycle_test: expected path point update to commit\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_undo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "path_point_move_undo")) {
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, path_object_id);
        if (!path_object || path_object->path_points[1].x != 96 || path_object->path_points[1].y != 64) {
            fprintf(stderr, "lifecycle_test: expected path-point undo to restore prior coordinate\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_redo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "path_point_move_redo")) {
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, path_object_id);
        if (!path_object || path_object->path_points[1].x != 110 || path_object->path_points[1].y != 70) {
            fprintf(stderr, "lifecycle_test: expected path-point redo to restore new coordinate\n");
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord back_object;
        DrawingProgramObjectRecord front_object;
        DrawingProgramVisualTransformOpsHooks hooks;
        VisualCanvasInteractionState interaction;
        CoreResult hit_result;
        uint32_t back_id = 0u;
        uint32_t front_id = 0u;
        uint32_t hit_object_id = 0u;

        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&back_object, 0, sizeof(back_object));
        memset(&front_object, 0, sizeof(front_object));
        back_object.layer_id = workflow_ctx.document.layers[0].layer_id;
        back_object.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT;
        back_object.visible = 1u;
        back_object.origin_x = 40;
        back_object.origin_y = 40;
        back_object.width = 40u;
        back_object.height = 40u;
        front_object = back_object;
        front_object.origin_x = 44;
        front_object.origin_y = 44;
        if (!expect_ok(drawing_program_object_store_add(&workflow_ctx.object_store, &back_object, &back_id),
                       "object_promote_seed_back_add")) {
            return 1;
        }
        if (!expect_ok(drawing_program_object_store_add(&workflow_ctx.object_store, &front_object, &front_id),
                       "object_promote_seed_front_add")) {
            return 1;
        }
        hit_result = drawing_program_object_store_hit_test_topmost(
            &workflow_ctx.object_store, &workflow_ctx.document, 50u, 50u, &hit_object_id, 0);
        if (hit_result.code != CORE_OK || hit_object_id != front_id) {
            fprintf(stderr,
                    "lifecycle_test: expected initial topmost hit=front id=%u got code=%d id=%u\n",
                    (unsigned)front_id,
                    (int)hit_result.code,
                    (unsigned)hit_object_id);
            return 1;
        }
        drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, back_id);
        memset(&hooks, 0, sizeof(hooks));
        hooks.visual_object_commit_move = lifecycle_test_object_commit_move;
        memset(&interaction, 0, sizeof(interaction));
        if (!expect_ok(drawing_program_visual_transform_session_nudge_object_move(
                           &workflow_ctx, &interaction, 4, 0, &hooks),
                       "object_promote_move_nudge")) {
            return 1;
        }
        hit_result = drawing_program_object_store_hit_test_topmost(
            &workflow_ctx.object_store, &workflow_ctx.document, 50u, 50u, &hit_object_id, 0);
        if (hit_result.code != CORE_OK || hit_object_id != back_id) {
            fprintf(stderr,
                    "lifecycle_test: expected moved selected object promoted topmost id=%u got code=%d id=%u\n",
                    (unsigned)back_id,
                    (int)hit_result.code,
                    (unsigned)hit_object_id);
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord raster_object;
        uint32_t object_id = 0u;
        uint8_t sample_value = drawing_program_color_eraser_value();
        uint8_t expected_fill = drawing_program_color_value_from_index(3u);
        uint32_t units_before = 0u;
        uint32_t units_after = 0u;
        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&raster_object, 0, sizeof(raster_object));
        raster_object.layer_id = workflow_ctx.document.layers[0].layer_id;
        raster_object.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT;
        raster_object.visible = 1u;
        raster_object.locked = 0u;
        raster_object.fill_color_index = 3u;
        raster_object.stroke_color_index = 7u;
        raster_object.stroke_width = 2u;
        raster_object.style_mode = 1u;
        raster_object.origin_x = 24;
        raster_object.origin_y = 24;
        raster_object.width = 10u;
        raster_object.height = 8u;
        if (!expect_ok(drawing_program_object_store_add(&workflow_ctx.object_store, &raster_object, &object_id),
                       "object_rasterize_seed_add")) {
            return 1;
        }
        drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, object_id);
        drawing_program_history_query_units(&workflow_ctx.history, &units_before, 0);
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_RASTERIZE_SELECTED_OBJECTS),
                       "object_rasterize_workflow_control")) {
            return 1;
        }
        if (workflow_ctx.object_store.object_count != 0u ||
            workflow_ctx.object_selection.count != 0u ||
            workflow_ctx.object_selection.active_object_id != 0u) {
            fprintf(stderr,
                    "lifecycle_test: expected rasterize workflow to remove selected object entities\n");
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      workflow_ctx.editor.active_layer_id,
                                                                      27u,
                                                                      27u,
                                                                      &sample_value),
                       "object_rasterize_sample_read")) {
            return 1;
        }
        if (sample_value != expected_fill) {
            fprintf(stderr,
                    "lifecycle_test: expected rasterized object sample=%u got=%u\n",
                    (unsigned)expected_fill,
                    (unsigned)sample_value);
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &units_after, 0);
        if (units_after != units_before + 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected rasterize workflow to append one history unit before=%u after=%u\n",
                    (unsigned)units_before,
                    (unsigned)units_after);
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        uint32_t object_id = 0u;
        uint8_t sample_value = drawing_program_color_eraser_value();
        uint8_t expected_fill = drawing_program_color_value_from_index(5u);
        uint32_t units_before = 0u;
        uint32_t units_after = 0u;
        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&path_seed, 0, sizeof(path_seed));
        memset(&path_payload, 0, sizeof(path_payload));
        path_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
        path_seed.visible = 1u;
        path_seed.locked = 0u;
        path_seed.fill_color_index = 5u;
        path_seed.stroke_color_index = 7u;
        path_seed.stroke_width = 2u;
        path_seed.style_mode = 2u;
        path_payload.point_count = 3u;
        path_payload.closed = 1u;
        path_payload.points[0].x = 40;
        path_payload.points[0].y = 40;
        path_payload.points[1].x = 80;
        path_payload.points[1].y = 40;
        path_payload.points[2].x = 60;
        path_payload.points[2].y = 80;
        if (!expect_ok(drawing_program_object_store_add_path(&workflow_ctx.object_store,
                                                             &path_seed,
                                                             &path_payload,
                                                             &object_id),
                       "path_rasterize_seed_add")) {
            return 1;
        }
        drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, object_id);
        drawing_program_history_query_units(&workflow_ctx.history, &units_before, 0);
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_RASTERIZE_SELECTED_OBJECTS),
                       "path_rasterize_workflow_control")) {
            return 1;
        }
        if (workflow_ctx.object_store.object_count != 0u ||
            workflow_ctx.object_selection.count != 0u ||
            workflow_ctx.object_selection.active_object_id != 0u) {
            fprintf(stderr,
                    "lifecycle_test: expected path rasterize workflow to remove selected object entities\n");
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      workflow_ctx.editor.active_layer_id,
                                                                      60u,
                                                                      55u,
                                                                      &sample_value),
                       "path_rasterize_sample_read")) {
            return 1;
        }
        if (sample_value != expected_fill) {
            fprintf(stderr,
                    "lifecycle_test: expected rasterized path sample=%u got=%u\n",
                    (unsigned)expected_fill,
                    (unsigned)sample_value);
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &units_after, 0);
        if (units_after != units_before + 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected path rasterize workflow to append one history unit before=%u after=%u\n",
                    (unsigned)units_before,
                    (unsigned)units_after);
            return 1;
        }
    }

#undef workflow_ctx
    return 0;
}
