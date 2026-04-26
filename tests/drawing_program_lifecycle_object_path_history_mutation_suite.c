#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "drawing_program_lifecycle_object_path_history_mutation_suite.h"
#include "drawing_program_lifecycle_test_support.h"

int drawing_program_lifecycle_run_object_path_history_mutation_suite(DrawingProgramAppContext *workflow_ctx_ptr)
{
#define workflow_ctx (*workflow_ctx_ptr)
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
        path_seed.fill_color_value = drawing_program_color_value_from_index(5u);
        path_seed.stroke_color_value = drawing_program_color_value_from_index(7u);
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
        path_seed.fill_color_value = drawing_program_color_value_from_index(5u);
        path_seed.stroke_color_value = drawing_program_color_value_from_index(7u);
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
        path_seed.fill_color_value = drawing_program_color_value_from_index(5u);
        path_seed.stroke_color_value = drawing_program_color_value_from_index(7u);
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
        path_seed.fill_color_value = drawing_program_color_value_from_index(5u);
        path_seed.stroke_color_value = drawing_program_color_value_from_index(7u);
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
        path_seed.fill_color_value = drawing_program_color_value_from_index(5u);
        path_seed.stroke_color_value = drawing_program_color_value_from_index(7u);
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
        rect_seed.stroke_color_value = drawing_program_color_value_from_index(2u);
        rect_seed.fill_color_value = drawing_program_color_value_from_index(4u);
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
                           &workflow_ctx.history, &workflow_ctx.object_store, object_id,
                           drawing_program_color_value_from_index(6u)),
                       "object_color_history_set_stroke")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_object_fill_color(
                           &workflow_ctx.history, &workflow_ctx.object_store, object_id,
                           drawing_program_color_value_from_index(1u)),
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
        if (!object ||
            object->stroke_color_value != drawing_program_color_value_from_index(6u) ||
            object->fill_color_value != drawing_program_color_value_from_index(1u)) {
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
        if (!object ||
            object->stroke_color_value != drawing_program_color_value_from_index(2u) ||
            object->fill_color_value != drawing_program_color_value_from_index(4u)) {
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
        if (!object ||
            object->stroke_color_value != drawing_program_color_value_from_index(6u) ||
            object->fill_color_value != drawing_program_color_value_from_index(1u)) {
            fprintf(stderr, "lifecycle_test: expected color redo to restore edited retained object colors\n");
            return 1;
        }
    }

#undef workflow_ctx
    return 0;
}
