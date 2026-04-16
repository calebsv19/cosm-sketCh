#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "drawing_program_lifecycle_selection_layer_suite.h"
#include "drawing_program_lifecycle_selection_payload_suite.h"
#include "drawing_program_lifecycle_test_support.h"

int drawing_program_lifecycle_run_selection_layer_suite(DrawingProgramAppContext *workflow_ctx_ptr,
                                                        DrawingProgramClipboardState *workflow_clipboard_ptr,
                                                        char **argv,
                                                        uint32_t workflow_center_x,
                                                        uint32_t workflow_center_y,
                                                        uint8_t expected_draw_value,
                                                        uint8_t expected_eraser_value)
{
#define workflow_ctx (*workflow_ctx_ptr)
#define workflow_clipboard (*workflow_clipboard_ptr)

    if (drawing_program_lifecycle_run_selection_payload_suite(workflow_ctx_ptr,
                                                              workflow_clipboard_ptr,
                                                              argv,
                                                              workflow_center_x,
                                                              workflow_center_y,
                                                              expected_draw_value,
                                                              expected_eraser_value) != 0) {
        return 1;
    }

    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY),
                   "workflow_toggle_layer_visibility")) {
        return 1;
    }
    if (workflow_ctx.document.layers[0].visible != 0u) {
        fprintf(stderr, "lifecycle_test: expected workflow toggle to hide active layer\n");
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_UNDO),
                   "workflow_undo")) {
        return 1;
    }
    if (workflow_ctx.document.layers[0].visible != 1u) {
        fprintf(stderr, "lifecycle_test: expected workflow undo to restore active layer visibility\n");
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_REDO),
                   "workflow_redo")) {
        return 1;
    }
    if (workflow_ctx.document.layers[0].visible != 0u) {
        fprintf(stderr, "lifecycle_test: expected workflow redo to hide active layer again\n");
        return 1;
    }
    {
        uint32_t base_layer_id = workflow_ctx.document.layers[0].layer_id;
        uint32_t new_layer_id = 0u;
        uint8_t sample_before_lock_stamp = 0u;
        uint8_t sample_after_lock_stamp = 0u;
        uint8_t base_before_active_stamp = 0u;
        uint8_t base_after_active_stamp = 0u;
        uint8_t active_before_visible_stamp = 0u;
        uint8_t active_after_visible_stamp = 0u;
        uint8_t active_before_hidden_stamp = 0u;
        uint8_t active_after_hidden_stamp = 0u;
        uint32_t resolved_active_layer_id = 0u;
        uint32_t resolved_active_layer_index = 0u;
        uint8_t resolved_active_visible = 0u;
        uint8_t resolved_active_locked = 0u;
        uint32_t policy_seed_x = workflow_center_x;
        uint32_t policy_seed_y = workflow_center_y + 2u;
        uint8_t policy_base_seed_value = 64u;
        uint8_t policy_active_seed_value = 192u;
        uint8_t policy_read_value = 0u;
        uint32_t clear_seed_x = workflow_center_x + 4u;
        uint32_t clear_seed_y = workflow_center_y + 4u;
        uint8_t clear_after_value = 0u;
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_ADD_LAYER),
                       "workflow_add_layer")) {
            return 1;
        }
        if (workflow_ctx.document.layer_count != 2u) {
            fprintf(stderr,
                    "lifecycle_test: expected layer_count=2 after add, got %u\n",
                    workflow_ctx.document.layer_count);
            return 1;
        }
        new_layer_id = workflow_ctx.editor.active_layer_id;
        if (new_layer_id == 0u || workflow_ctx.document.layers[1].layer_id != new_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected active layer to be newly added appended layer id=%u index1=%u\n",
                    (unsigned)new_layer_id,
                    (unsigned)workflow_ctx.document.layers[1].layer_id);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_resolve_active_layer(&workflow_ctx,
                                                                                   &resolved_active_layer_id,
                                                                                   &resolved_active_layer_index,
                                                                                   &resolved_active_visible,
                                                                                   &resolved_active_locked),
                       "workflow_resolve_active_layer")) {
            return 1;
        }
        if (resolved_active_layer_id != new_layer_id ||
            resolved_active_layer_index >= workflow_ctx.document.layer_count ||
            workflow_ctx.document.layers[resolved_active_layer_index].layer_id != resolved_active_layer_id ||
            resolved_active_visible != workflow_ctx.document.layers[resolved_active_layer_index].visible ||
            resolved_active_locked != workflow_ctx.document.layers[resolved_active_layer_index].locked) {
            fprintf(stderr,
                    "lifecycle_test: expected active-layer resolve to return coherent id/index mapping "
                    "(id=%u index=%u mapped=%u visible=%u locked=%u)\n",
                    (unsigned)resolved_active_layer_id,
                    (unsigned)resolved_active_layer_index,
                    (unsigned)((resolved_active_layer_index < workflow_ctx.document.layer_count)
                                   ? workflow_ctx.document.layers[resolved_active_layer_index].layer_id
                                   : 0u),
                    (unsigned)resolved_active_visible,
                    (unsigned)resolved_active_locked);
            return 1;
        }
        if (clear_seed_x >= workflow_ctx.document.raster_width || clear_seed_y >= workflow_ctx.document.raster_height) {
            fprintf(stderr, "lifecycle_test: clear-canvas probe coordinate out of bounds\n");
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY),
                       "workflow_clear_canvas_seed_hide_active")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_LOCK),
                       "workflow_clear_canvas_seed_lock_active")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "workflow_clear_canvas")) {
            return 1;
        }
        if (workflow_ctx.document.layers[1].visible != 1u || workflow_ctx.document.layers[1].locked != 0u) {
            fprintf(stderr,
                    "lifecycle_test: expected clear canvas to restore active layer editable state visible=%u locked=%u\n",
                    (unsigned)workflow_ctx.document.layers[1].visible,
                    (unsigned)workflow_ctx.document.layers[1].locked);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE),
                       "workflow_clear_canvas_stamp_after_clear")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                      &workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      clear_seed_x,
                                                                      clear_seed_y,
                                                                      expected_draw_value),
                       "workflow_clear_canvas_seed_active_probe")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      clear_seed_x,
                                                                      clear_seed_y,
                                                                      &clear_after_value),
                       "workflow_clear_canvas_probe_after_write")) {
            return 1;
        }
        if (clear_after_value != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected draw write after clear canvas value=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)clear_after_value);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_LOCK),
                       "workflow_toggle_layer_lock_on")) {
            return 1;
        }
        if (workflow_ctx.document.layers[1].locked != 1u) {
            fprintf(stderr, "lifecycle_test: expected active layer lock on after toggle\n");
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document,
                                                            workflow_center_x,
                                                            workflow_center_y,
                                                            &sample_before_lock_stamp),
                       "workflow_sample_before_lock_stamp")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE),
                       "workflow_stamp_locked_layer")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document,
                                                            workflow_center_x,
                                                            workflow_center_y,
                                                            &sample_after_lock_stamp),
                       "workflow_sample_after_lock_stamp")) {
            return 1;
        }
        if (sample_before_lock_stamp != sample_after_lock_stamp) {
            fprintf(stderr,
                    "lifecycle_test: expected locked layer stamp to no-op before=%u after=%u\n",
                    (unsigned)sample_before_lock_stamp,
                    (unsigned)sample_after_lock_stamp);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_LOCK),
                       "workflow_toggle_layer_lock_off")) {
            return 1;
        }
        if (workflow_ctx.document.layers[1].locked != 0u) {
            fprintf(stderr, "lifecycle_test: expected active layer lock off after second toggle\n");
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      base_layer_id,
                                                                      workflow_center_x,
                                                                      workflow_center_y,
                                                                      &base_before_active_stamp),
                       "workflow_base_sample_before_active_stamp")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      workflow_center_x,
                                                                      workflow_center_y,
                                                                      &active_before_visible_stamp),
                       "workflow_active_sample_before_visible_stamp")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE),
                       "workflow_stamp_visible_active_layer")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      base_layer_id,
                                                                      workflow_center_x,
                                                                      workflow_center_y,
                                                                      &base_after_active_stamp),
                       "workflow_base_sample_after_active_stamp")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      workflow_center_x,
                                                                      workflow_center_y,
                                                                      &active_after_visible_stamp),
                       "workflow_active_sample_after_visible_stamp")) {
            return 1;
        }
        if (base_before_active_stamp != base_after_active_stamp ||
            active_after_visible_stamp != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected active-layer stamp to update only active layer "
                    "(base %u->%u active %u->%u expected_active=%u)\n",
                    (unsigned)base_before_active_stamp,
                    (unsigned)base_after_active_stamp,
                    (unsigned)active_before_visible_stamp,
                    (unsigned)active_after_visible_stamp,
                    (unsigned)expected_draw_value);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY),
                       "workflow_toggle_active_layer_visibility_off_for_stamp_noop")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      workflow_center_x,
                                                                      workflow_center_y,
                                                                      &active_before_hidden_stamp),
                       "workflow_active_sample_before_hidden_stamp")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE),
                       "workflow_stamp_hidden_active_layer_noop")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      workflow_center_x,
                                                                      workflow_center_y,
                                                                      &active_after_hidden_stamp),
                       "workflow_active_sample_after_hidden_stamp")) {
            return 1;
        }
        if (active_before_hidden_stamp != active_after_hidden_stamp) {
            fprintf(stderr,
                    "lifecycle_test: expected hidden-layer stamp no-op before=%u after=%u\n",
                    (unsigned)active_before_hidden_stamp,
                    (unsigned)active_after_hidden_stamp);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY),
                       "workflow_toggle_active_layer_visibility_on_restore")) {
            return 1;
        }
        {
            DrawingProgramRenderInvalidation render_invalidation = {0};
            DrawingProgramRenderFrameProjection projection_visible = {0};
            DrawingProgramRenderFrameProjection projection_hidden = {0};
            uint8_t previous_visible = 0u;
            if (!expect_ok(drawing_program_document_sample_write(&workflow_ctx.document,
                                                                 workflow_center_x,
                                                                 workflow_center_y,
                                                                 104u,
                                                                 0),
                           "workflow_compose_seed_legacy_surface")) {
                return 1;
            }
            if (!expect_ok(drawing_program_layer_raster_store_sample_write(&workflow_ctx.layer_rasters,
                                                                           new_layer_id,
                                                                           workflow_center_x,
                                                                           workflow_center_y,
                                                                           200u,
                                                                           0),
                           "workflow_compose_seed_layer_surface")) {
                return 1;
            }
            if (!expect_ok(drawing_program_render_project_frame(&workflow_ctx.document,
                                                                &workflow_ctx.layer_rasters,
                                                                &workflow_ctx.editor,
                                                                &render_invalidation,
                                                                &projection_visible),
                           "workflow_compose_projection_visible")) {
                return 1;
            }
            if (!expect_ok(drawing_program_document_set_layer_visibility(&workflow_ctx.document,
                                                                         new_layer_id,
                                                                         0u,
                                                                         &previous_visible),
                           "workflow_compose_hide_new_layer")) {
                return 1;
            }
            if (!expect_ok(drawing_program_render_project_frame(&workflow_ctx.document,
                                                                &workflow_ctx.layer_rasters,
                                                                &workflow_ctx.editor,
                                                                &render_invalidation,
                                                                &projection_hidden),
                           "workflow_compose_projection_hidden")) {
                return 1;
            }
            if (projection_visible.raster_hash32 == projection_hidden.raster_hash32 ||
                projection_visible.visible_layer_count == projection_hidden.visible_layer_count) {
                fprintf(stderr,
                        "lifecycle_test: expected layer visibility to change composited projection hash/visible-count "
                        "visible(hash=%u vis=%u) hidden(hash=%u vis=%u)\n",
                        (unsigned)projection_visible.raster_hash32,
                        (unsigned)projection_visible.visible_layer_count,
                        (unsigned)projection_hidden.raster_hash32,
                        (unsigned)projection_hidden.visible_layer_count);
                return 1;
            }
            if (!expect_ok(drawing_program_document_set_layer_visibility(&workflow_ctx.document,
                                                                         new_layer_id,
                                                                         previous_visible,
                                                                         0),
                           "workflow_compose_restore_new_layer_visibility")) {
                return 1;
            }
        }
        if (policy_seed_y >= workflow_ctx.document.raster_height ||
            (policy_seed_x + 3u) >= workflow_ctx.document.raster_width) {
            fprintf(stderr, "lifecycle_test: layered selection policy seed position out of bounds\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                      &workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      base_layer_id,
                                                                      policy_seed_x,
                                                                      policy_seed_y,
                                                                      policy_base_seed_value),
                       "workflow_policy_seed_base_layer")) {
            return 1;
        }
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&workflow_ctx.history,
                                                                      &workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      policy_seed_x + 1u,
                                                                      policy_seed_y,
                                                                      policy_active_seed_value),
                       "workflow_policy_seed_active_layer")) {
            return 1;
        }
        if (!drawing_program_selection_capture_from_rect(&workflow_ctx.document,
                                                        &workflow_ctx.layer_rasters,
                                                        new_layer_id,
                                                        &workflow_ctx.selection,
                                                        (int32_t)policy_seed_x,
                                                        (int32_t)policy_seed_y,
                                                        2u,
                                                        1u)) {
            fprintf(stderr, "lifecycle_test: expected layered selection capture on active layer\n");
            return 1;
        }
        if (!workflow_ctx.selection.has_payload ||
            workflow_ctx.selection.layer_id != new_layer_id ||
            workflow_ctx.selection.payload_count != 1u ||
            drawing_program_selection_mask_at(&workflow_ctx.selection, 0u, 0u) != 0u ||
            drawing_program_selection_mask_at(&workflow_ctx.selection, 1u, 0u) != 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected active-layer-only selection payload "
                    "(layer=%u payload=%u mask0=%u mask1=%u)\n",
                    (unsigned)workflow_ctx.selection.layer_id,
                    (unsigned)workflow_ctx.selection.payload_count,
                    (unsigned)drawing_program_selection_mask_at(&workflow_ctx.selection, 0u, 0u),
                    (unsigned)drawing_program_selection_mask_at(&workflow_ctx.selection, 1u, 0u));
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_set_active_layer_id(&workflow_ctx, base_layer_id),
                       "workflow_policy_set_active_base_before_cut")) {
            return 1;
        }
        if (!expect_ok(drawing_program_selection_cut_to_clipboard(&workflow_ctx.document,
                                                                  &workflow_ctx.layer_rasters,
                                                                  workflow_ctx.editor.active_layer_id,
                                                                  &workflow_ctx.history,
                                                                  &workflow_ctx.selection,
                                                                  &workflow_clipboard),
                       "workflow_policy_cut_selection_layer_binding")) {
            return 1;
        }
        if (!workflow_clipboard.has_payload || workflow_clipboard.source_layer_id != new_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected clipboard source layer=%u has_payload=%u got_layer=%u\n",
                    (unsigned)new_layer_id,
                    (unsigned)workflow_clipboard.has_payload,
                    (unsigned)workflow_clipboard.source_layer_id);
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      base_layer_id,
                                                                      policy_seed_x,
                                                                      policy_seed_y,
                                                                      &policy_read_value),
                       "workflow_policy_base_after_cut")) {
            return 1;
        }
        if (policy_read_value != policy_base_seed_value) {
            fprintf(stderr,
                    "lifecycle_test: expected base layer seed unchanged after cut expected=%u got=%u\n",
                    (unsigned)policy_base_seed_value,
                    (unsigned)policy_read_value);
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      policy_seed_x + 1u,
                                                                      policy_seed_y,
                                                                      &policy_read_value),
                       "workflow_policy_active_after_cut")) {
            return 1;
        }
        if (policy_read_value != expected_eraser_value) {
            fprintf(stderr,
                    "lifecycle_test: expected active-layer cut clear expected=%u got=%u\n",
                    (unsigned)expected_eraser_value,
                    (unsigned)policy_read_value);
            return 1;
        }
        if (!expect_ok(drawing_program_selection_paste_from_clipboard(&workflow_ctx.document,
                                                                      &workflow_ctx.layer_rasters,
                                                                      workflow_ctx.editor.active_layer_id,
                                                                      &workflow_ctx.history,
                                                                      &workflow_ctx.selection,
                                                                      &workflow_clipboard,
                                                                      (int32_t)policy_seed_x + 2,
                                                                      (int32_t)policy_seed_y),
                       "workflow_policy_paste_to_active_layer")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&workflow_ctx.layer_rasters,
                                                                      base_layer_id,
                                                                      policy_seed_x + 3u,
                                                                      policy_seed_y,
                                                                      &policy_read_value),
                       "workflow_policy_base_after_paste")) {
            return 1;
        }
        if (policy_read_value != policy_active_seed_value) {
            fprintf(stderr,
                    "lifecycle_test: expected paste target on active(base) layer expected=%u got=%u\n",
                    (unsigned)policy_active_seed_value,
                    (unsigned)policy_read_value);
            return 1;
        }
        if (!workflow_ctx.selection.has_payload || workflow_ctx.selection.layer_id != base_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected pasted selection to bind to active target layer=%u got=%u\n",
                    (unsigned)base_layer_id,
                    (unsigned)workflow_ctx.selection.layer_id);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_set_active_layer_id(&workflow_ctx, new_layer_id),
                       "workflow_policy_restore_active_new_before_cycle")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_PREV),
                       "workflow_select_layer_prev")) {
            return 1;
        }
        if (workflow_ctx.editor.active_layer_id != base_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected prev layer to select base id=%u got=%u\n",
                    (unsigned)base_layer_id,
                    (unsigned)workflow_ctx.editor.active_layer_id);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_NEXT),
                       "workflow_select_layer_next")) {
            return 1;
        }
        if (workflow_ctx.editor.active_layer_id != new_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected next layer to select added id=%u got=%u\n",
                    (unsigned)new_layer_id,
                    (unsigned)workflow_ctx.editor.active_layer_id);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_DOWN),
                       "workflow_move_active_layer_down")) {
            return 1;
        }
        if (workflow_ctx.document.layers[0].layer_id != new_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected move down to place active layer at index0 id=%u got=%u\n",
                    (unsigned)new_layer_id,
                    (unsigned)workflow_ctx.document.layers[0].layer_id);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_UP),
                       "workflow_move_active_layer_up")) {
            return 1;
        }
        if (workflow_ctx.document.layers[1].layer_id != new_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected move up to restore active layer at index1 id=%u got=%u\n",
                    (unsigned)new_layer_id,
                    (unsigned)workflow_ctx.document.layers[1].layer_id);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_set_active_layer_id(&workflow_ctx, base_layer_id),
                       "workflow_set_active_layer_id")) {
            return 1;
        }
        if (workflow_ctx.editor.active_layer_id != base_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected set_active_layer_id to target base id=%u got=%u\n",
                    (unsigned)base_layer_id,
                    (unsigned)workflow_ctx.editor.active_layer_id);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_set_active_layer_id(&workflow_ctx, new_layer_id),
                       "workflow_set_active_layer_new_before_delete")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_DELETE_ACTIVE_LAYER),
                       "workflow_delete_active_layer")) {
            return 1;
        }
        if (workflow_ctx.document.layer_count != 1u ||
            workflow_ctx.document.layers[0].layer_id != base_layer_id ||
            workflow_ctx.editor.active_layer_id != base_layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected delete active layer to retain base only "
                    "(count=%u base=%u active=%u expected_base=%u)\n",
                    (unsigned)workflow_ctx.document.layer_count,
                    (unsigned)workflow_ctx.document.layers[0].layer_id,
                    (unsigned)workflow_ctx.editor.active_layer_id,
                    (unsigned)base_layer_id);
            return 1;
        }
    }

#undef workflow_clipboard
#undef workflow_ctx
    return 0;
}
