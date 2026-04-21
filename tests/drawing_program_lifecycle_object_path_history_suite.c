#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_object_geometry.h"
#include "drawing_program/drawing_program_object_transform.h"
#include "drawing_program/drawing_program_visual_transform_ops.h"
#include "drawing_program_lifecycle_object_path_history_suite.h"
#include "drawing_program_lifecycle_test_support.h"

static CoreResult lifecycle_test_object_commit_path_point_data(DrawingProgramAppContext *ctx,
                                                               uint32_t object_id,
                                                               uint16_t point_index,
                                                               const DrawingProgramPathPoint *point) {
    return drawing_program_history_apply_set_object_path_point_data(
        &ctx->history, &ctx->object_store, object_id, point_index, point);
}

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

int drawing_program_lifecycle_run_object_path_history_suite(DrawingProgramAppContext *workflow_ctx_ptr)
{
#define workflow_ctx (*workflow_ctx_ptr)
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
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        DrawingProgramPathPoint toggled_point;
        const DrawingProgramObjectRecord *path_object = 0;
        uint32_t path_object_id = 0u;

        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&path_seed, 0, sizeof(path_seed));
        memset(&path_payload, 0, sizeof(path_payload));
        memset(&toggled_point, 0, sizeof(toggled_point));
        path_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
        path_seed.visible = 1u;
        path_seed.locked = 0u;
        path_seed.stroke_color_index = 6u;
        path_seed.fill_color_index = 6u;
        path_seed.stroke_width = 2u;
        path_seed.style_mode = 0u;
        path_payload.point_count = 3u;
        path_payload.closed = 0u;
        path_payload.points[0].x = 40;
        path_payload.points[0].y = 40;
        path_payload.points[1].x = 72;
        path_payload.points[1].y = 40;
        path_payload.points[2].x = 104;
        path_payload.points[2].y = 56;
        if (!expect_ok(drawing_program_object_store_add_path(
                           &workflow_ctx.object_store, &path_seed, &path_payload, &path_object_id),
                       "path_point_bezier_seed_add")) {
            return 1;
        }
        toggled_point = path_payload.points[1];
        toggled_point.handle_in_dx = -12;
        toggled_point.handle_in_dy = -2;
        toggled_point.handle_out_dx = 12;
        toggled_point.handle_out_dy = 2;
        toggled_point.bezier_enabled = 1u;
        toggled_point.handle_linked = 1u;
        if (!expect_ok(drawing_program_history_apply_set_object_path_point_data(
                           &workflow_ctx.history, &workflow_ctx.object_store, path_object_id, 1u, &toggled_point),
                       "path_point_bezier_history_apply")) {
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, path_object_id);
        if (!path_object ||
            !path_object->path_points[1].bezier_enabled ||
            path_object->path_points[1].handle_in_dx != -12 ||
            path_object->path_points[1].handle_out_dx != 12) {
            fprintf(stderr, "lifecycle_test: expected bezier point data apply to persist full point state\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_undo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "path_point_bezier_undo")) {
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, path_object_id);
        if (!path_object ||
            path_object->path_points[1].bezier_enabled ||
            path_object->path_points[1].handle_in_dx != 0 ||
            path_object->path_points[1].handle_out_dx != 0) {
            fprintf(stderr, "lifecycle_test: expected bezier point undo to restore linear point state\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_redo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "path_point_bezier_redo")) {
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, path_object_id);
        if (!path_object ||
            !path_object->path_points[1].bezier_enabled ||
            path_object->path_points[1].handle_in_dx != -12 ||
            path_object->path_points[1].handle_out_dx != 12) {
            fprintf(stderr, "lifecycle_test: expected bezier point redo to restore bezier state\n");
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        DrawingProgramPathPoint point_after;
        DrawingProgramVisualTransformOpsHooks hooks;
        VisualCanvasInteractionState interaction;
        const DrawingProgramObjectRecord *path_object = 0;
        CoreResult hit_result;
        uint32_t path_object_id = 0u;
        uint32_t hit_object_id = 0u;
        uint32_t units_before = 0u;
        uint32_t units_after = 0u;
        uint16_t hit_point_index = 0u;
        uint8_t hit_handle_kind = 0u;

        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&path_seed, 0, sizeof(path_seed));
        memset(&path_payload, 0, sizeof(path_payload));
        memset(&point_after, 0, sizeof(point_after));
        memset(&hooks, 0, sizeof(hooks));
        memset(&interaction, 0, sizeof(interaction));
        path_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
        path_seed.visible = 1u;
        path_seed.locked = 0u;
        path_seed.stroke_color_index = 6u;
        path_seed.fill_color_index = 6u;
        path_seed.stroke_width = 2u;
        path_seed.style_mode = 0u;
        path_payload.point_count = 3u;
        path_payload.closed = 0u;
        path_payload.points[0].x = 40;
        path_payload.points[0].y = 40;
        path_payload.points[1].x = 72;
        path_payload.points[1].y = 40;
        path_payload.points[1].handle_in_dx = -12;
        path_payload.points[1].handle_in_dy = -2;
        path_payload.points[1].handle_out_dx = 12;
        path_payload.points[1].handle_out_dy = 2;
        path_payload.points[1].bezier_enabled = 1u;
        path_payload.points[1].handle_linked = 1u;
        path_payload.points[2].x = 104;
        path_payload.points[2].y = 56;
        if (!expect_ok(drawing_program_object_store_add_path(
                           &workflow_ctx.object_store, &path_seed, &path_payload, &path_object_id),
                       "path_handle_move_seed_add")) {
            return 1;
        }
        drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, path_object_id);
        if (!drawing_program_object_selection_set_path_point(&workflow_ctx.object_selection, path_object_id, 1u)) {
            fprintf(stderr, "lifecycle_test: expected selected path point set before handle drag\n");
            return 1;
        }
        hit_result = drawing_program_object_store_hit_test_selected_path_handle(&workflow_ctx.object_store,
                                                                                &workflow_ctx.document,
                                                                                &workflow_ctx.object_selection,
                                                                                84u,
                                                                                42u,
                                                                                6u,
                                                                                &hit_object_id,
                                                                                &hit_point_index,
                                                                                &hit_handle_kind);
        if (hit_result.code != CORE_OK ||
            hit_object_id != path_object_id ||
            hit_point_index != 1u ||
            hit_handle_kind != (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_OUT) {
            fprintf(stderr,
                    "lifecycle_test: expected selected bezier handle hit object=%u point=%u kind=%u got code=%d object=%u point=%u kind=%u\n",
                    (unsigned)path_object_id,
                    1u,
                    (unsigned)DRAWING_PROGRAM_PATH_HANDLE_OUT,
                    (int)hit_result.code,
                    (unsigned)hit_object_id,
                    (unsigned)hit_point_index,
                    (unsigned)hit_handle_kind);
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, path_object_id);
        if (!path_object) {
            fprintf(stderr, "lifecycle_test: expected path object for handle drag target\n");
            return 1;
        }
        hooks.visual_object_commit_path_point_data = lifecycle_test_object_commit_path_point_data;
        drawing_program_visual_transform_session_begin_object_path_handle_move(&interaction,
                                                                               path_object_id,
                                                                               1u,
                                                                               (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_OUT,
                                                                               &path_object->path_points[1]);
        drawing_program_visual_transform_session_update_object_path_handle_move(
            &workflow_ctx, &interaction, 96u, 48u);
        drawing_program_history_query_units(&workflow_ctx.history, &units_before, 0);
        if (!expect_ok(drawing_program_visual_transform_session_commit_object_path_handle_move(
                           &workflow_ctx, &interaction, &hooks),
                       "path_handle_move_commit")) {
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &units_after, 0);
        if (units_after != units_before + 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected path-handle drag to append one history unit before=%u after=%u\n",
                    (unsigned)units_before,
                    (unsigned)units_after);
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, path_object_id);
        if (!path_object) {
            fprintf(stderr, "lifecycle_test: expected path object after handle drag commit\n");
            return 1;
        }
        point_after = path_object->path_points[1];
        if (point_after.handle_out_dx != 24 ||
            point_after.handle_out_dy != 8 ||
            point_after.handle_in_dx != -24 ||
            point_after.handle_in_dy != -8 ||
            !point_after.bezier_enabled) {
            fprintf(stderr,
                    "lifecycle_test: expected bezier handle drag commit to update linked handles out=(%d,%d) in=(%d,%d) enabled=%u\n",
                    point_after.handle_out_dx,
                    point_after.handle_out_dy,
                    point_after.handle_in_dx,
                    point_after.handle_in_dy,
                    (unsigned)point_after.bezier_enabled);
            return 1;
        }
        if (!expect_ok(drawing_program_history_undo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "path_handle_move_undo")) {
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, path_object_id);
        if (!path_object ||
            path_object->path_points[1].handle_out_dx != 12 ||
            path_object->path_points[1].handle_out_dy != 2 ||
            path_object->path_points[1].handle_in_dx != -12 ||
            path_object->path_points[1].handle_in_dy != -2) {
            fprintf(stderr, "lifecycle_test: expected handle drag undo to restore original bezier handles\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_redo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "path_handle_move_redo")) {
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, path_object_id);
        if (!path_object ||
            path_object->path_points[1].handle_out_dx != 24 ||
            path_object->path_points[1].handle_out_dy != 8 ||
            path_object->path_points[1].handle_in_dx != -24 ||
            path_object->path_points[1].handle_in_dy != -8) {
            fprintf(stderr, "lifecycle_test: expected handle drag redo to restore moved bezier handles\n");
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        const DrawingProgramObjectRecord *path_object = 0;
        DrawingProgramPathPoint point_after;
        DrawingProgramVisualTransformOpsHooks hooks;
        VisualCanvasInteractionState interaction;
        uint32_t path_object_id = 0u;
        uint32_t units_before = 0u;
        uint32_t units_after = 0u;

        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&path_seed, 0, sizeof(path_seed));
        memset(&path_payload, 0, sizeof(path_payload));
        memset(&point_after, 0, sizeof(point_after));
        memset(&hooks, 0, sizeof(hooks));
        memset(&interaction, 0, sizeof(interaction));
        path_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
        path_seed.visible = 1u;
        path_seed.locked = 0u;
        path_seed.stroke_color_index = 6u;
        path_seed.fill_color_index = 6u;
        path_seed.stroke_width = 2u;
        path_seed.style_mode = 0u;
        path_payload.point_count = 3u;
        path_payload.closed = 0u;
        path_payload.points[0].x = 40;
        path_payload.points[0].y = 40;
        path_payload.points[1].x = 72;
        path_payload.points[1].y = 40;
        path_payload.points[1].handle_in_dx = -12;
        path_payload.points[1].handle_in_dy = -2;
        path_payload.points[1].handle_out_dx = 12;
        path_payload.points[1].handle_out_dy = 2;
        path_payload.points[1].bezier_enabled = 1u;
        path_payload.points[1].handle_linked = 0u;
        path_payload.points[2].x = 104;
        path_payload.points[2].y = 56;
        if (!expect_ok(drawing_program_object_store_add_path(
                           &workflow_ctx.object_store, &path_seed, &path_payload, &path_object_id),
                       "path_handle_move_unlinked_seed_add")) {
            return 1;
        }
        drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, path_object_id);
        if (!drawing_program_object_selection_set_path_point(&workflow_ctx.object_selection, path_object_id, 1u)) {
            fprintf(stderr, "lifecycle_test: expected selected path point set before unlinked handle drag\n");
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, path_object_id);
        if (!path_object) {
            fprintf(stderr, "lifecycle_test: expected path object for unlinked handle drag target\n");
            return 1;
        }
        hooks.visual_object_commit_path_point_data = lifecycle_test_object_commit_path_point_data;
        drawing_program_visual_transform_session_begin_object_path_handle_move(&interaction,
                                                                               path_object_id,
                                                                               1u,
                                                                               (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_OUT,
                                                                               &path_object->path_points[1]);
        drawing_program_visual_transform_session_update_object_path_handle_move(
            &workflow_ctx, &interaction, 96u, 48u);
        drawing_program_history_query_units(&workflow_ctx.history, &units_before, 0);
        if (!expect_ok(drawing_program_visual_transform_session_commit_object_path_handle_move(
                           &workflow_ctx, &interaction, &hooks),
                       "path_handle_move_unlinked_commit")) {
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &units_after, 0);
        if (units_after != units_before + 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected unlinked path-handle drag to append one history unit before=%u after=%u\n",
                    (unsigned)units_before,
                    (unsigned)units_after);
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, path_object_id);
        if (!path_object) {
            fprintf(stderr, "lifecycle_test: expected path object after unlinked handle drag commit\n");
            return 1;
        }
        point_after = path_object->path_points[1];
        if (point_after.handle_out_dx != 24 ||
            point_after.handle_out_dy != 8 ||
            point_after.handle_in_dx != -12 ||
            point_after.handle_in_dy != -2 ||
            point_after.handle_linked != 0u ||
            !point_after.bezier_enabled) {
            fprintf(stderr,
                    "lifecycle_test: expected unlinked bezier handle drag to preserve opposite handle out=(%d,%d) in=(%d,%d) linked=%u enabled=%u\n",
                    point_after.handle_out_dx,
                    point_after.handle_out_dy,
                    point_after.handle_in_dx,
                    point_after.handle_in_dy,
                    (unsigned)point_after.handle_linked,
                    (unsigned)point_after.bezier_enabled);
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
    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        DrawingProgramObjectSelectionState selection;
        const DrawingProgramObjectRecord *object = 0;
        uint32_t object_id = 0u;
        uint32_t hit_object_id = 0u;
        uint16_t insert_index = 0u;
        int32_t insert_x = 0;
        int32_t insert_y = 0;
        uint8_t sample_value = drawing_program_color_eraser_value();
        uint8_t expected_stroke = drawing_program_color_value_from_index(5u);

        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&path_seed, 0, sizeof(path_seed));
        memset(&path_payload, 0, sizeof(path_payload));
        memset(&selection, 0, sizeof(selection));
        path_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
        path_seed.visible = 1u;
        path_seed.locked = 0u;
        path_seed.fill_color_index = 3u;
        path_seed.stroke_color_index = 5u;
        path_seed.stroke_width = 3u;
        path_seed.style_mode = 0u;
        path_payload.point_count = 2u;
        path_payload.closed = 0u;
        path_payload.points[0].x = 20;
        path_payload.points[0].y = 20;
        path_payload.points[0].bezier_enabled = 1u;
        path_payload.points[0].handle_linked = 1u;
        path_payload.points[0].handle_out_dx = 0;
        path_payload.points[0].handle_out_dy = 30;
        path_payload.points[1].x = 60;
        path_payload.points[1].y = 20;
        path_payload.points[1].bezier_enabled = 1u;
        path_payload.points[1].handle_linked = 1u;
        path_payload.points[1].handle_in_dx = 0;
        path_payload.points[1].handle_in_dy = 30;
        if (!expect_ok(drawing_program_object_store_add_path(&workflow_ctx.object_store,
                                                             &path_seed,
                                                             &path_payload,
                                                             &object_id),
                       "bezier_path_curve_seed_add")) {
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object) {
            fprintf(stderr, "lifecycle_test: expected bezier path seed object to exist\n");
            return 1;
        }
        if (!drawing_program_object_path_contains(object, 40u, 42u)) {
            fprintf(stderr, "lifecycle_test: expected curved bezier path to contain bowed sample\n");
            return 1;
        }
        drawing_program_object_selection_replace_single(&selection, object_id);
        if (!expect_ok(drawing_program_object_store_hit_test_selected_path_edge(&workflow_ctx.object_store,
                                                                                &workflow_ctx.document,
                                                                                &selection,
                                                                                40u,
                                                                                42u,
                                                                                4u,
                                                                                &hit_object_id,
                                                                                &insert_index,
                                                                                &insert_x,
                                                                                &insert_y),
                       "bezier_path_curve_edge_hit")) {
            return 1;
        }
        if (hit_object_id != object_id || insert_index != 1u || insert_y < 36 || insert_y > 46) {
            fprintf(stderr,
                    "lifecycle_test: expected curved edge hit id=%u index=1 y~42 got id=%u index=%u point=%d,%d\n",
                    (unsigned)object_id,
                    (unsigned)hit_object_id,
                    (unsigned)insert_index,
                    (int)insert_x,
                    (int)insert_y);
            return 1;
        }
        drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, object_id);
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_RASTERIZE_SELECTED_OBJECTS),
                       "bezier_path_curve_rasterize_workflow_control")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      workflow_ctx.editor.active_layer_id,
                                                                      40u,
                                                                      42u,
                                                                      &sample_value),
                       "bezier_path_curve_rasterize_sample_read")) {
            return 1;
        }
        if (sample_value != expected_stroke) {
            fprintf(stderr,
                    "lifecycle_test: expected curved bezier raster sample=%u got=%u\n",
                    (unsigned)expected_stroke,
                    (unsigned)sample_value);
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        const DrawingProgramObjectRecord *object = 0;
        uint32_t object_id = 0u;
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
        path_seed.style_mode = 0u;
        path_payload.point_count = 2u;
        path_payload.closed = 0u;
        path_payload.points[0].x = 24;
        path_payload.points[0].y = 24;
        path_payload.points[1].x = 56;
        path_payload.points[1].y = 24;
        if (!expect_ok(drawing_program_object_store_add_path(&workflow_ctx.object_store,
                                                             &path_seed,
                                                             &path_payload,
                                                             &object_id),
                       "object_path_insert_seed_add")) {
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &units_before, 0);
        if (!expect_ok(drawing_program_history_apply_insert_object_path_point(
                           &workflow_ctx.history, &workflow_ctx.object_store, object_id, 1u, 40, 24),
                       "object_path_insert_history_apply")) {
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &units_after, 0);
        if (units_after != units_before + 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected path-point insert to append one history unit before=%u after=%u\n",
                    (unsigned)units_before,
                    (unsigned)units_after);
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object ||
            object->path_point_count != 3u ||
            object->path_points[1].x != 40 ||
            object->path_points[1].y != 24) {
            fprintf(stderr, "lifecycle_test: expected path-point insert history to update retained path topology\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_undo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_path_insert_undo")) {
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object ||
            object->path_point_count != 2u ||
            object->path_points[0].x != 24 ||
            object->path_points[1].x != 56) {
            fprintf(stderr, "lifecycle_test: expected path-point insert undo to restore original path topology\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_redo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_path_insert_redo")) {
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object ||
            object->path_point_count != 3u ||
            object->path_points[1].x != 40 ||
            object->path_points[1].y != 24) {
            fprintf(stderr, "lifecycle_test: expected path-point insert redo to restore inserted point\n");
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        const DrawingProgramObjectRecord *object = 0;
        uint32_t object_id = 0u;
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
        path_seed.style_mode = 0u;
        path_payload.point_count = 4u;
        path_payload.closed = 0u;
        path_payload.points[0].x = 24;
        path_payload.points[0].y = 24;
        path_payload.points[1].x = 40;
        path_payload.points[1].y = 24;
        path_payload.points[2].x = 56;
        path_payload.points[2].y = 24;
        path_payload.points[3].x = 72;
        path_payload.points[3].y = 24;
        if (!expect_ok(drawing_program_object_store_add_path(&workflow_ctx.object_store,
                                                             &path_seed,
                                                             &path_payload,
                                                             &object_id),
                       "object_path_remove_mid_seed_add")) {
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &units_before, 0);
        if (!expect_ok(drawing_program_history_apply_remove_object_path_point(
                           &workflow_ctx.history, &workflow_ctx.object_store, object_id, 1u),
                       "object_path_remove_mid_history_apply")) {
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &units_after, 0);
        if (units_after != units_before + 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected middle path-point remove to append one history unit before=%u after=%u\n",
                    (unsigned)units_before,
                    (unsigned)units_after);
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object ||
            object->path_point_count != 3u ||
            object->path_points[0].x != 24 ||
            object->path_points[1].x != 56 ||
            object->path_points[2].x != 72) {
            fprintf(stderr, "lifecycle_test: expected middle path-point remove to reconnect neighbors\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_undo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_path_remove_mid_undo")) {
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object ||
            object->path_point_count != 4u ||
            object->path_points[1].x != 40 ||
            object->path_points[2].x != 56) {
            fprintf(stderr, "lifecycle_test: expected middle path-point remove undo to restore topology\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_redo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_path_remove_mid_redo")) {
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object ||
            object->path_point_count != 3u ||
            object->path_points[1].x != 56) {
            fprintf(stderr, "lifecycle_test: expected middle path-point remove redo to restore collapsed chain\n");
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        const DrawingProgramObjectRecord *object = 0;
        uint32_t object_id = 0u;

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
        path_seed.style_mode = 0u;
        path_payload.point_count = 3u;
        path_payload.closed = 0u;
        path_payload.points[0].x = 24;
        path_payload.points[0].y = 24;
        path_payload.points[0].bezier_enabled = 1u;
        path_payload.points[0].handle_linked = 0u;
        path_payload.points[0].handle_out_dx = 10;
        path_payload.points[0].handle_out_dy = 12;
        path_payload.points[1].x = 48;
        path_payload.points[1].y = 24;
        path_payload.points[1].bezier_enabled = 1u;
        path_payload.points[1].handle_linked = 0u;
        path_payload.points[1].handle_in_dx = -6;
        path_payload.points[1].handle_in_dy = -4;
        path_payload.points[1].handle_out_dx = 8;
        path_payload.points[1].handle_out_dy = 6;
        path_payload.points[2].x = 72;
        path_payload.points[2].y = 40;
        path_payload.points[2].bezier_enabled = 1u;
        path_payload.points[2].handle_linked = 0u;
        path_payload.points[2].handle_in_dx = -12;
        path_payload.points[2].handle_in_dy = -10;
        if (!expect_ok(drawing_program_object_store_add_path(&workflow_ctx.object_store,
                                                             &path_seed,
                                                             &path_payload,
                                                             &object_id),
                       "object_path_remove_bezier_mid_seed_add")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_remove_object_path_point(
                           &workflow_ctx.history, &workflow_ctx.object_store, object_id, 1u),
                       "object_path_remove_bezier_mid_history_apply")) {
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object ||
            object->path_point_count != 2u ||
            !object->path_points[0].bezier_enabled ||
            object->path_points[0].handle_out_dx != 10 ||
            object->path_points[0].handle_out_dy != 12 ||
            !object->path_points[1].bezier_enabled ||
            object->path_points[1].handle_in_dx != -12 ||
            object->path_points[1].handle_in_dy != -10) {
            fprintf(stderr,
                    "lifecycle_test: expected bezier middle path-point remove to preserve neighbor curve handles\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_undo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_path_remove_bezier_mid_undo")) {
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object ||
            object->path_point_count != 3u ||
            !object->path_points[1].bezier_enabled ||
            object->path_points[1].handle_in_dx != -6 ||
            object->path_points[1].handle_in_dy != -4 ||
            object->path_points[1].handle_out_dx != 8 ||
            object->path_points[1].handle_out_dy != 6 ||
            object->path_points[1].handle_linked != 0u) {
            fprintf(stderr,
                    "lifecycle_test: expected bezier middle path-point remove undo to restore removed point handle state\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_redo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_path_remove_bezier_mid_redo")) {
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object ||
            object->path_point_count != 2u ||
            object->path_points[0].handle_out_dx != 10 ||
            object->path_points[1].handle_in_dx != -12) {
            fprintf(stderr,
                    "lifecycle_test: expected bezier middle path-point remove redo to restore collapsed curve chain\n");
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        const DrawingProgramObjectRecord *object = 0;
        uint32_t object_id = 0u;

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
        path_seed.style_mode = 0u;
        path_payload.point_count = 4u;
        path_payload.closed = 0u;
        path_payload.points[0].x = 24;
        path_payload.points[0].y = 24;
        path_payload.points[1].x = 40;
        path_payload.points[1].y = 24;
        path_payload.points[2].x = 56;
        path_payload.points[2].y = 24;
        path_payload.points[3].x = 72;
        path_payload.points[3].y = 24;
        if (!expect_ok(drawing_program_object_store_add_path(&workflow_ctx.object_store,
                                                             &path_seed,
                                                             &path_payload,
                                                             &object_id),
                       "object_path_remove_end_seed_add")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_remove_object_path_point(
                           &workflow_ctx.history, &workflow_ctx.object_store, object_id, 0u),
                       "object_path_remove_end_history_apply")) {
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object ||
            object->path_point_count != 3u ||
            object->path_points[0].x != 40 ||
            object->path_points[1].x != 56 ||
            object->path_points[2].x != 72) {
            fprintf(stderr, "lifecycle_test: expected endpoint remove to drop only the endpoint\n");
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        const DrawingProgramObjectRecord *object = 0;
        uint32_t object_id = 0u;
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
        path_seed.style_mode = 0u;
        path_payload.point_count = 3u;
        path_payload.closed = 1u;
        path_payload.points[0].x = 24;
        path_payload.points[0].y = 24;
        path_payload.points[1].x = 56;
        path_payload.points[1].y = 24;
        path_payload.points[2].x = 40;
        path_payload.points[2].y = 56;
        if (!expect_ok(drawing_program_object_store_add_path(&workflow_ctx.object_store,
                                                             &path_seed,
                                                             &path_payload,
                                                             &object_id),
                       "object_property_history_seed_add")) {
            return 1;
        }

        drawing_program_history_query_units(&workflow_ctx.history, &units_before, &units_after);
        if (!expect_ok(drawing_program_history_apply_set_object_stroke_width(
                           &workflow_ctx.history, &workflow_ctx.object_store, object_id, 5u),
                       "object_property_history_set_stroke_width")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_object_style_mode(
                           &workflow_ctx.history, &workflow_ctx.object_store, object_id, 2u),
                       "object_property_history_set_style_mode")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_object_path_closed(
                           &workflow_ctx.history, &workflow_ctx.object_store, object_id, 0u),
                       "object_property_history_set_path_closed")) {
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &units_after, 0);
        if (units_after != units_before + 3u) {
            fprintf(stderr,
                    "lifecycle_test: expected object property edits to append three history units before=%u after=%u\n",
                    (unsigned)units_before,
                    (unsigned)units_after);
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object ||
            object->stroke_width != 5u ||
            object->style_mode != 2u ||
            object->path_closed != 0u) {
            fprintf(stderr, "lifecycle_test: expected property edits to update retained object fields\n");
            return 1;
        }

        if (!expect_ok(drawing_program_history_undo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_property_path_closed_undo")) {
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object || object->path_closed != 1u) {
            fprintf(stderr, "lifecycle_test: expected path-closed undo to restore closed state\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_undo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_property_style_mode_undo")) {
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object || object->style_mode != 0u) {
            fprintf(stderr, "lifecycle_test: expected style-mode undo to restore outline mode\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_undo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_property_stroke_width_undo")) {
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object || object->stroke_width != 2u) {
            fprintf(stderr, "lifecycle_test: expected stroke-width undo to restore original width\n");
            return 1;
        }

        if (!expect_ok(drawing_program_history_redo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_property_stroke_width_redo")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_redo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_property_style_mode_redo")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_redo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_property_path_closed_redo")) {
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object ||
            object->stroke_width != 5u ||
            object->style_mode != 2u ||
            object->path_closed != 0u) {
            fprintf(stderr, "lifecycle_test: expected property redos to restore edited retained object fields\n");
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord rect_seed;
        const DrawingProgramObjectRecord *object = 0;
        uint32_t object_id = 0u;
        uint32_t units_before = 0u;
        uint32_t units_after = 0u;

        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&rect_seed, 0, sizeof(rect_seed));
        rect_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
        rect_seed.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT;
        rect_seed.visible = 1u;
        rect_seed.locked = 0u;
        rect_seed.stroke_color_index = 2u;
        rect_seed.fill_color_index = 4u;
        rect_seed.stroke_width = 2u;
        rect_seed.style_mode = 2u;
        rect_seed.origin_x = 20;
        rect_seed.origin_y = 20;
        rect_seed.width = 12u;
        rect_seed.height = 10u;
        if (!expect_ok(drawing_program_object_store_add(&workflow_ctx.object_store, &rect_seed, &object_id),
                       "object_color_history_seed_add")) {
            return 1;
        }

        drawing_program_history_query_units(&workflow_ctx.history, &units_before, 0);
        if (!expect_ok(drawing_program_history_apply_set_object_stroke_color(
                           &workflow_ctx.history, &workflow_ctx.object_store, object_id, 6u),
                       "object_color_history_set_stroke")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_object_fill_color(
                           &workflow_ctx.history, &workflow_ctx.object_store, object_id, 1u),
                       "object_color_history_set_fill")) {
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &units_after, 0);
        if (units_after != units_before + 2u) {
            fprintf(stderr,
                    "lifecycle_test: expected object color edits to append two history units before=%u after=%u\n",
                    (unsigned)units_before,
                    (unsigned)units_after);
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object || object->stroke_color_index != 6u || object->fill_color_index != 1u) {
            fprintf(stderr, "lifecycle_test: expected object color edits to update retained object colors\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_undo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_color_history_fill_undo")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_undo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_color_history_stroke_undo")) {
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object || object->stroke_color_index != 2u || object->fill_color_index != 4u) {
            fprintf(stderr, "lifecycle_test: expected color undo to restore original retained object colors\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_redo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_color_history_stroke_redo")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_redo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "object_color_history_fill_redo")) {
            return 1;
        }
        object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!object || object->stroke_color_index != 6u || object->fill_color_index != 1u) {
            fprintf(stderr, "lifecycle_test: expected color redo to restore edited retained object colors\n");
            return 1;
        }
    }

#undef workflow_ctx
    return 0;
}
