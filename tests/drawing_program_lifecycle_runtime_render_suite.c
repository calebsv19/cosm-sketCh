#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core_theme.h"
#include "drawing_program/drawing_program_visual_input_handlers.h"
#include "drawing_program/drawing_program_visual_input_selection_ops.h"
#include "drawing_program/drawing_program_visual_input_support.h"
#include "drawing_program/drawing_program_visual_transform_ops.h"
#include "drawing_program_lifecycle_runtime_render_suite.h"
#include "drawing_program_lifecycle_test_support.h"

int drawing_program_lifecycle_run_runtime_render_suite(DrawingProgramAppContext *ctx_ptr,
                                                       DrawingProgramAppContext *workflow_ctx_ptr,
                                                       uint32_t center_x,
                                                       uint32_t center_y,
                                                       uint8_t center_before,
                                                       uint8_t expected_draw_value,
                                                       uint8_t expected_eraser_value)
{
    uint8_t center_after = 0u;
    uint8_t center_undo = 0u;
    uint8_t center_redo = 0u;
    (void)expected_eraser_value;
#define ctx (*ctx_ptr)
#define workflow_ctx (*workflow_ctx_ptr)

    {
        DrawingProgramVisualInputHandlersHooks hooks;
        VisualCanvasInteractionState interaction;
        VisualPanelUiState panel_ui;
        CoreThemePresetId selected_theme = CORE_THEME_PRESET_DARK_DEFAULT;
        CoreThemePreset theme_preset;
        SDL_Event event;
        memset(&hooks, 0, sizeof(hooks));
        hooks.apply_workflow_control_if_valid = lifecycle_test_apply_workflow_control;
        hooks.visual_transform_session_nudge_object_move = lifecycle_test_nudge_object_move;
        hooks.visual_transform_session_nudge_move = lifecycle_test_nudge_selection_move;
        hooks.cancel_canvas_draw_and_shape = lifecycle_test_cancel_canvas_draw_and_shape;
        hooks.cancel_selection_transient = lifecycle_test_cancel_selection_transient;
        hooks.cancel_all_transient_interactions = lifecycle_test_cancel_all_transient_interactions;
        hooks.delete_active_selection_payload_or_objects = delete_active_selection_payload_or_objects;
        hooks.path_draft_commit_closed = path_draft_commit_closed;
        hooks.path_draft_reset = path_draft_reset;
        hooks.path_draft_pop_point = path_draft_pop_point;
        memset(&interaction, 0, sizeof(interaction));
        memset(&panel_ui, 0, sizeof(panel_ui));

        lifecycle_test_reset_input_handler_counters();
        workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_BRUSH;
        workflow_ctx.object_selection.count = 1u;
        workflow_ctx.object_selection.active_object_id = 99u;
        workflow_ctx.object_selection.object_ids[0] = 99u;
        memset(&event, 0, sizeof(event));
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_r;
        event.key.keysym.mod = KMOD_CTRL;
        if (!drawing_program_visual_input_handle_keydown_payload(&event,
                                                                 0,
                                                                 (SDL_Rect){ 0, 0, 0, 0 },
                                                                 &selected_theme,
                                                                 &theme_preset,
                                                                 &workflow_ctx,
                                                                 &interaction,
                                                                 &workflow_ctx.selection,
                                                                 &panel_ui,
                                                                 &hooks)) {
            fprintf(stderr, "lifecycle_test: expected ctrl+r keydown to be consumed by rasterize action\n");
            return 1;
        }
        if (g_test_apply_workflow_calls != 1u ||
            g_test_last_workflow_control != DRAWING_PROGRAM_WORKFLOW_CONTROL_RASTERIZE_SELECTED_OBJECTS) {
            fprintf(stderr, "lifecycle_test: ctrl+r did not route to rasterize workflow control\n");
            return 1;
        }
        if (workflow_ctx.editor.active_tool != DRAWING_PROGRAM_TOOL_BRUSH) {
            fprintf(stderr, "lifecycle_test: ctrl+r should not switch active tool to rect\n");
            return 1;
        }

        lifecycle_test_reset_input_handler_counters();
        memset(&event, 0, sizeof(event));
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_p;
        event.key.keysym.mod = KMOD_NONE;
        if (!drawing_program_visual_input_handle_keydown_payload(&event,
                                                                 0,
                                                                 (SDL_Rect){ 0, 0, 0, 0 },
                                                                 &selected_theme,
                                                                 &theme_preset,
                                                                 &workflow_ctx,
                                                                 &interaction,
                                                                 &workflow_ctx.selection,
                                                                 &panel_ui,
                                                                 &hooks)) {
            fprintf(stderr, "lifecycle_test: expected path tool keydown to be consumed\n");
            return 1;
        }
        if (g_test_apply_workflow_calls != 1u ||
            g_test_last_workflow_control != DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_PATH) {
            fprintf(stderr, "lifecycle_test: path keydown did not route to set-tool-path workflow control\n");
            return 1;
        }

        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_selection_reset(&workflow_ctx.selection);
        memset(&interaction, 0, sizeof(interaction));
        workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_PATH;
        workflow_ctx.editor.active_layer_id = workflow_ctx.document.layers[0].layer_id;
        workflow_ctx.document.layers[0].visible = 1u;
        workflow_ctx.document.layers[0].locked = 0u;
        interaction.path_draft_active = 1u;
        interaction.path_draft_point_count = 3u;
        interaction.path_draft_points[0].x = 20;
        interaction.path_draft_points[0].y = 24;
        interaction.path_draft_points[1].x = 28;
        interaction.path_draft_points[1].y = 24;
        interaction.path_draft_points[2].x = 24;
        interaction.path_draft_points[2].y = 32;
        memset(&event, 0, sizeof(event));
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_RETURN;
        event.key.keysym.mod = KMOD_NONE;
        if (!drawing_program_visual_input_handle_keydown_payload(&event,
                                                                 0,
                                                                 (SDL_Rect){ 0, 0, 0, 0 },
                                                                 &selected_theme,
                                                                 &theme_preset,
                                                                 &workflow_ctx,
                                                                 &interaction,
                                                                 &workflow_ctx.selection,
                                                                 &panel_ui,
                                                                 &hooks)) {
            fprintf(stderr, "lifecycle_test: expected path commit keydown to be consumed\n");
            return 1;
        }
        if (workflow_ctx.object_store.object_count != 1u ||
            workflow_ctx.object_selection.count != 1u ||
            workflow_ctx.object_selection.active_object_id == 0u ||
            interaction.path_draft_active ||
            interaction.path_draft_point_count != 0u) {
            fprintf(stderr,
                    "lifecycle_test: path commit keydown failed store_count=%u sel_count=%u active=%u draft_active=%u draft_points=%u\n",
                    (unsigned)workflow_ctx.object_store.object_count,
                    (unsigned)workflow_ctx.object_selection.count,
                    (unsigned)workflow_ctx.object_selection.active_object_id,
                    (unsigned)interaction.path_draft_active,
                    (unsigned)interaction.path_draft_point_count);
            return 1;
        }
        {
            const DrawingProgramObjectRecord *path_object =
                drawing_program_object_store_get_by_id(&workflow_ctx.object_store,
                                                       workflow_ctx.object_selection.active_object_id);
            if (!path_object ||
                path_object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
                path_object->path_point_count != 3u ||
                !path_object->path_closed ||
                path_object->style_mode != 0u) {
                fprintf(stderr,
                        "lifecycle_test: path commit produced invalid object type=%u points=%u closed=%u style=%u\n",
                        path_object ? (unsigned)path_object->type : 0u,
                        path_object ? (unsigned)path_object->path_point_count : 0u,
                        path_object ? (unsigned)path_object->path_closed : 0u,
                        path_object ? (unsigned)path_object->style_mode : 0u);
                return 1;
            }
        }

        {
            DrawingProgramObjectRecord object_seed;
            uint32_t object_id = 0u;
            memset(&object_seed, 0, sizeof(object_seed));
            drawing_program_object_store_reset(&workflow_ctx.object_store);
            drawing_program_object_selection_reset(&workflow_ctx.object_selection);
            object_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
            object_seed.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT;
            object_seed.visible = 1u;
            object_seed.locked = 0u;
            object_seed.origin_x = 32;
            object_seed.origin_y = 32;
            object_seed.width = 16u;
            object_seed.height = 16u;
            if (!expect_ok(drawing_program_object_store_add(&workflow_ctx.object_store, &object_seed, &object_id),
                           "object_delete_seed_add")) {
                return 1;
            }
            drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, object_id);
            memset(&event, 0, sizeof(event));
            event.type = SDL_KEYDOWN;
            event.key.keysym.sym = SDLK_DELETE;
            event.key.keysym.mod = KMOD_NONE;
            if (!drawing_program_visual_input_handle_keydown_payload(&event,
                                                                     0,
                                                                     (SDL_Rect){ 0, 0, 0, 0 },
                                                                     &selected_theme,
                                                                     &theme_preset,
                                                                     &workflow_ctx,
                                                                     &interaction,
                                                                     &workflow_ctx.selection,
                                                                     &panel_ui,
                                                                     &hooks)) {
                fprintf(stderr, "lifecycle_test: expected delete keydown to be consumed for object selection\n");
                return 1;
            }
            if (workflow_ctx.object_store.object_count != 0u ||
                workflow_ctx.object_selection.count != 0u ||
                workflow_ctx.object_selection.active_object_id != 0u) {
                fprintf(stderr,
                        "lifecycle_test: delete keydown failed to remove selected object count=%u sel_count=%u active=%u\n",
                        (unsigned)workflow_ctx.object_store.object_count,
                        (unsigned)workflow_ctx.object_selection.count,
                        (unsigned)workflow_ctx.object_selection.active_object_id);
                return 1;
            }
        }

        lifecycle_test_reset_input_handler_counters();
        workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_MOVE;
        workflow_ctx.object_selection.count = 1u;
        workflow_ctx.object_selection.active_object_id = 77u;
        workflow_ctx.object_selection.object_ids[0] = 77u;
        workflow_ctx.selection.has_payload = 1u;
        memset(&event, 0, sizeof(event));
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_RIGHT;
        event.key.keysym.mod = KMOD_NONE;
        if (!drawing_program_visual_input_handle_keydown_payload(&event,
                                                                 0,
                                                                 (SDL_Rect){ 0, 0, 0, 0 },
                                                                 &selected_theme,
                                                                 &theme_preset,
                                                                 &workflow_ctx,
                                                                 &interaction,
                                                                 &workflow_ctx.selection,
                                                                 &panel_ui,
                                                                 &hooks)) {
            fprintf(stderr, "lifecycle_test: expected move nudge keydown to be consumed\n");
            return 1;
        }
        if (g_test_object_nudge_calls != 1u || g_test_selection_nudge_calls != 0u) {
            fprintf(stderr,
                    "lifecycle_test: move nudge precedence should favor object selection when present object_calls=%u selection_calls=%u\n",
                    (unsigned)g_test_object_nudge_calls,
                    (unsigned)g_test_selection_nudge_calls);
            return 1;
        }

        lifecycle_test_reset_input_handler_counters();
        workflow_ctx.object_selection.count = 0u;
        workflow_ctx.object_selection.active_object_id = 0u;
        workflow_ctx.object_selection.object_ids[0] = 0u;
        workflow_ctx.selection.has_payload = 1u;
        memset(&event, 0, sizeof(event));
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_RIGHT;
        event.key.keysym.mod = KMOD_NONE;
        if (!drawing_program_visual_input_handle_keydown_payload(&event,
                                                                 0,
                                                                 (SDL_Rect){ 0, 0, 0, 0 },
                                                                 &selected_theme,
                                                                 &theme_preset,
                                                                 &workflow_ctx,
                                                                 &interaction,
                                                                 &workflow_ctx.selection,
                                                                 &panel_ui,
                                                                 &hooks)) {
            fprintf(stderr, "lifecycle_test: expected selection nudge keydown to be consumed\n");
            return 1;
        }
        if (g_test_object_nudge_calls != 0u || g_test_selection_nudge_calls != 1u) {
            fprintf(stderr,
                    "lifecycle_test: move nudge precedence should fallback to raster selection when object selection is empty object_calls=%u selection_calls=%u\n",
                    (unsigned)g_test_object_nudge_calls,
                    (unsigned)g_test_selection_nudge_calls);
            return 1;
        }
    }
    if (!expect_ok(drawing_program_app_run_loop(&ctx), "run_loop")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&ctx.document, center_x, center_y, &center_after),
                   "sample_read_center_after")) {
        return 1;
    }
    if (center_after != expected_draw_value) {
        fprintf(stderr, "lifecycle_test: expected brush seed to set center sample to %u got=%u\n",
                (unsigned)expected_draw_value,
                (unsigned)center_after);
        return 1;
    }
    if (!expect_ok(drawing_program_history_apply_set_sample_value(&ctx.history,
                                                                  &ctx.document,
                                                                  &ctx.layer_rasters,
                                                                  ctx.editor.active_layer_id,
                                                                  center_x,
                                                                  center_y,
                                                                  center_after),
                   "history_apply_set_sample_value_noop")) {
        return 1;
    }
    if (ctx.history.count != 1u || ctx.history.cursor != 1u) {
        fprintf(stderr,
                "lifecycle_test: expected no-op sample command to avoid history push count=%u cursor=%u\n",
                ctx.history.count,
                ctx.history.cursor);
        return 1;
    }
    if (ctx.editor.active_tool != DRAWING_PROGRAM_TOOL_SELECT || ctx.runtime.tool_switch_total != 1u) {
        fprintf(stderr,
                "lifecycle_test: expected tool switch to SELECT with count=1 got tool=%u switches=%llu\n",
                (unsigned)ctx.editor.active_tool,
                (unsigned long long)ctx.runtime.tool_switch_total);
        return 1;
    }
    if (ctx.runtime.viewport_sample_probe_success_total != 2u) {
        fprintf(stderr, "lifecycle_test: expected viewport probe success count 2 got=%llu\n",
                (unsigned long long)ctx.runtime.viewport_sample_probe_success_total);
        return 1;
    }
    if (!expect_ok(drawing_program_app_shutdown(&ctx), "shutdown")) {
        return 1;
    }
    if (ctx.runtime.frame_counter != 2u) {
        fprintf(stderr, "lifecycle_test: expected frame_counter=2 got=%llu\n",
                (unsigned long long)ctx.runtime.frame_counter);
        return 1;
    }
    if (ctx.runtime.input_events_processed != 4u) {
        fprintf(stderr, "lifecycle_test: expected input_events_processed=4 got=%llu\n",
                (unsigned long long)ctx.runtime.input_events_processed);
        return 1;
    }
    if (ctx.runtime.input_actions_emitted != 4u) {
        fprintf(stderr, "lifecycle_test: expected input_actions_emitted=4 got=%llu\n",
                (unsigned long long)ctx.runtime.input_actions_emitted);
        return 1;
    }
    if (ctx.runtime.routed_global_total != 2u ||
        ctx.runtime.routed_pane_total != 2u ||
        ctx.runtime.routed_fallback_total != 0u) {
        fprintf(stderr,
                "lifecycle_test: unexpected route totals g=%llu p=%llu f=%llu\n",
                (unsigned long long)ctx.runtime.routed_global_total,
                (unsigned long long)ctx.runtime.routed_pane_total,
                (unsigned long long)ctx.runtime.routed_fallback_total);
        return 1;
    }
    if (ctx.runtime.invalidation_target_total != 1u || ctx.runtime.invalidation_full_total != 1u) {
        fprintf(stderr,
                "lifecycle_test: unexpected invalidation totals target=%llu full=%llu\n",
                (unsigned long long)ctx.runtime.invalidation_target_total,
                (unsigned long long)ctx.runtime.invalidation_full_total);
        return 1;
    }
    if (ctx.runtime.render_frames_projected_total != 2u ||
        ctx.runtime.render_layers_visible_total != 2u ||
        ctx.runtime.render_full_redraw_total != 1u ||
        ctx.runtime.render_target_redraw_total != 1u) {
        fprintf(stderr,
                "lifecycle_test: unexpected render projection totals frames=%llu visible=%llu full=%llu target=%llu\n",
                (unsigned long long)ctx.runtime.render_frames_projected_total,
                (unsigned long long)ctx.runtime.render_layers_visible_total,
                (unsigned long long)ctx.runtime.render_full_redraw_total,
                (unsigned long long)ctx.runtime.render_target_redraw_total);
        return 1;
    }
    if (ctx.runtime.render_module_calls_total != 12u ||
        ctx.runtime.render_module_canvas_calls_total != 3u ||
        ctx.runtime.render_module_palette_calls_total != 3u) {
        fprintf(stderr,
                "lifecycle_test: unexpected module render call totals total=%llu canvas=%llu palette=%llu\n",
                (unsigned long long)ctx.runtime.render_module_calls_total,
                (unsigned long long)ctx.runtime.render_module_canvas_calls_total,
                (unsigned long long)ctx.runtime.render_module_palette_calls_total);
        return 1;
    }
    if (!expect_ok(drawing_program_history_undo(&ctx.history, &ctx.document, &ctx.layer_rasters, &ctx.object_store), "history_undo_brush_seed")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&ctx.document, center_x, center_y, &center_undo),
                   "sample_read_center_after_undo")) {
        return 1;
    }
    if (center_undo != center_before) {
        fprintf(stderr, "lifecycle_test: expected center sample restore on undo before=%u after_undo=%u\n",
                (unsigned)center_before,
                (unsigned)center_undo);
        return 1;
    }
    if (!expect_ok(drawing_program_history_redo(&ctx.history, &ctx.document, &ctx.layer_rasters, &ctx.object_store), "history_redo_brush_seed")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&ctx.document, center_x, center_y, &center_redo),
                   "sample_read_center_after_redo")) {
        return 1;
    }
    if (center_redo != expected_draw_value) {
        fprintf(stderr,
                "lifecycle_test: expected center sample %u after redo got=%u\n",
                (unsigned)expected_draw_value,
                (unsigned)center_redo);
        return 1;
    }
    if (ctx.runtime.render_projection.raster_sample_count == 0u ||
        ctx.runtime.render_projection.raster_nonzero_count == 0u ||
        ctx.runtime.render_projection.raster_hash32 == 0u) {
        fprintf(stderr, "lifecycle_test: expected raster projection baseline populated\n");
        return 1;
    }
    if (ctx.runtime.render_canvas_last_raster_hash != ctx.runtime.render_projection.raster_hash32 ||
        ctx.runtime.render_canvas_last_nonzero_samples != ctx.runtime.render_projection.raster_nonzero_count) {
        fprintf(stderr, "lifecycle_test: expected canvas module to consume raster projection fields\n");
        return 1;
    }
    if (!ctx.runtime.render_last_has_active_layer || ctx.runtime.render_last_active_layer_id != 1u) {
        fprintf(stderr,
                "lifecycle_test: expected active layer projection id=1 has_active=1 got id=%u has_active=%u\n",
                ctx.runtime.render_last_active_layer_id,
                (unsigned)ctx.runtime.render_last_has_active_layer);
        return 1;
    }
    {
        uint8_t sample = 0u;
        uint8_t bg = drawing_program_color_eraser_value();
        ctx.ui.active_color_index = drawing_program_color_default_index();
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "s3_shape_mode_clear_canvas_fill_rect")) {
            return 1;
        }
        ctx.ui.tool_shape_mode = 1u; /* FILL */
        ctx.ui.tool_shape_stroke_width = 1u;
        if (!expect_ok(drawing_program_app_shape_commit_samples(&ctx,
                                                                DRAWING_PROGRAM_TOOL_RECT,
                                                                8u,
                                                                8u,
                                                                12u,
                                                                12u),
                       "s3_shape_mode_rect_fill_commit")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 10u, 10u, &sample),
                       "s3_shape_mode_rect_fill_center_sample")) {
            return 1;
        }
        if (sample != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected rect fill center=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)sample);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "s3_shape_mode_clear_canvas_outline_rect")) {
            return 1;
        }
        ctx.ui.tool_shape_mode = 0u; /* OUTLINE */
        ctx.ui.tool_shape_stroke_width = 1u;
        if (!expect_ok(drawing_program_app_shape_commit_samples(&ctx,
                                                                DRAWING_PROGRAM_TOOL_RECT,
                                                                8u,
                                                                8u,
                                                                12u,
                                                                12u),
                       "s3_shape_mode_rect_outline_commit")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 10u, 10u, &sample),
                       "s3_shape_mode_rect_outline_center_sample")) {
            return 1;
        }
        if (sample != bg) {
            fprintf(stderr,
                    "lifecycle_test: expected rect outline center background=%u got=%u\n",
                    (unsigned)bg,
                    (unsigned)sample);
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 8u, 10u, &sample),
                       "s3_shape_mode_rect_outline_edge_sample")) {
            return 1;
        }
        if (sample != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected rect outline edge=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)sample);
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "s3_shape_mode_clear_canvas_line_width")) {
            return 1;
        }
        ctx.ui.tool_shape_mode = 0u; /* OUTLINE */
        ctx.ui.tool_shape_stroke_width = 3u;
        if (!expect_ok(drawing_program_app_shape_commit_samples(&ctx,
                                                                DRAWING_PROGRAM_TOOL_LINE,
                                                                40u,
                                                                20u,
                                                                40u,
                                                                28u),
                       "s3_shape_mode_line_stroke_commit")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 41u, 24u, &sample),
                       "s3_shape_mode_line_stroke_adjacent_sample")) {
            return 1;
        }
        if (sample != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected thick line adjacent=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)sample);
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 43u, 24u, &sample),
                       "s3_shape_mode_line_stroke_outside_sample")) {
            return 1;
        }
        if (sample != bg) {
            fprintf(stderr,
                    "lifecycle_test: expected thick line outside background=%u got=%u\n",
                    (unsigned)bg,
                    (unsigned)sample);
            return 1;
        }
        drawing_program_object_store_reset(&ctx.object_store);
        drawing_program_object_selection_reset(&ctx.object_selection);
        ctx.ui.tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_OBJECT;
        ctx.ui.tool_shape_mode = 2u; /* FILL+OUTLINE */
        ctx.ui.tool_shape_stroke_width = 2u;
        if (!expect_ok(drawing_program_app_shape_commit_samples(&ctx,
                                                                DRAWING_PROGRAM_TOOL_RECT,
                                                                14u,
                                                                14u,
                                                                22u,
                                                                18u),
                       "s3_shape_target_rect_object_commit")) {
            return 1;
        }
        if (ctx.object_store.object_count != 1u ||
            ctx.object_selection.count != 1u ||
            ctx.object_selection.active_object_id == 0u) {
            fprintf(stderr,
                    "lifecycle_test: expected rect object target to create one selected object count=%u sel=%u active=%u\n",
                    (unsigned)ctx.object_store.object_count,
                    (unsigned)ctx.object_selection.count,
                    (unsigned)ctx.object_selection.active_object_id);
            return 1;
        }
        {
            const DrawingProgramObjectRecord *object =
                drawing_program_object_store_get_by_id(&ctx.object_store, ctx.object_selection.active_object_id);
            if (!object ||
                object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT ||
                object->origin_x != 14 ||
                object->origin_y != 14 ||
                object->width != 9u ||
                object->height != 5u) {
                fprintf(stderr,
                        "lifecycle_test: expected rect object target geometry origin=%d,%d size=%ux%u type=%u\n",
                        object ? object->origin_x : -1,
                        object ? object->origin_y : -1,
                        object ? (unsigned)object->width : 0u,
                        object ? (unsigned)object->height : 0u,
                        object ? (unsigned)object->type : 0u);
                return 1;
            }
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 18u, 16u, &sample),
                       "s3_shape_target_rect_object_canvas_sample")) {
            return 1;
        }
        if (sample != bg) {
            fprintf(stderr,
                    "lifecycle_test: expected rect object target to avoid direct raster write background=%u got=%u\n",
                    (unsigned)bg,
                    (unsigned)sample);
            return 1;
        }
        drawing_program_object_store_reset(&ctx.object_store);
        drawing_program_object_selection_reset(&ctx.object_selection);
        if (!expect_ok(drawing_program_app_shape_commit_samples(&ctx,
                                                                DRAWING_PROGRAM_TOOL_CIRCLE,
                                                                32u,
                                                                32u,
                                                                36u,
                                                                38u),
                       "s3_shape_target_circle_object_commit")) {
            return 1;
        }
        if (ctx.object_store.object_count != 1u ||
            ctx.object_selection.count != 1u ||
            ctx.object_selection.active_object_id == 0u) {
            fprintf(stderr,
                    "lifecycle_test: expected circle object target to create one selected object count=%u sel=%u active=%u\n",
                    (unsigned)ctx.object_store.object_count,
                    (unsigned)ctx.object_selection.count,
                    (unsigned)ctx.object_selection.active_object_id);
            return 1;
        }
        {
            const DrawingProgramObjectRecord *object =
                drawing_program_object_store_get_by_id(&ctx.object_store, ctx.object_selection.active_object_id);
            if (!object ||
                object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_ELLIPSE ||
                object->width == 0u ||
                object->height == 0u) {
                fprintf(stderr,
                        "lifecycle_test: expected circle object target ellipse geometry type=%u size=%ux%u\n",
                        object ? (unsigned)object->type : 0u,
                        object ? (unsigned)object->width : 0u,
                        object ? (unsigned)object->height : 0u);
                return 1;
            }
        }
        if (!expect_ok(drawing_program_document_sample_read(&ctx.document, 32u, 32u, &sample),
                       "s3_shape_target_circle_object_canvas_sample")) {
            return 1;
        }
        if (sample != bg) {
            fprintf(stderr,
                    "lifecycle_test: expected circle object target to avoid direct raster write background=%u got=%u\n",
                    (unsigned)bg,
                    (unsigned)sample);
            return 1;
        }
        ctx.ui.tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
    }
    {
        DrawingProgramAppContext save_ctx;
        DrawingProgramAppContext load_ctx;
        char persist_arg0[] = "drawing_program_persist_roundtrip";
        char persist_arg1[] = "--headless";
        char persist_arg2[] = "--smoke-frames";
        char persist_arg3[] = "1";
        char persist_arg4[] = "--preset";
        char persist_arg5[] = "/tmp/drawing_program_persist_roundtrip.pack";
        char *persist_argv[] = { persist_arg0, persist_arg1, persist_arg2, persist_arg3, persist_arg4, persist_arg5, 0 };
        (void)unlink(persist_arg5);
        if (!expect_ok(drawing_program_app_bootstrap(&save_ctx, 6, persist_argv), "persist_roundtrip_bootstrap_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&save_ctx), "persist_roundtrip_config_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&save_ctx), "persist_roundtrip_state_seed_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_subsystems_init(&save_ctx), "persist_roundtrip_subsystems_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_start(&save_ctx), "persist_roundtrip_runtime_start_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &save_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_PICKER),
                       "persist_roundtrip_set_tool_picker")) {
            return 1;
        }
        save_ctx.ui.theme_preset_id = (uint32_t)CORE_THEME_PRESET_LIGHT_DEFAULT;
        save_ctx.ui.font_zoom_step = 3;
        save_ctx.ui.left_panel_slot = 1u;
        save_ctx.ui.right_panel_slot = 1u;
        save_ctx.ui.active_color_index = 6u;
        save_ctx.ui.tool_brush_size = 7u;
        save_ctx.ui.tool_brush_opacity = 65u;
        save_ctx.ui.tool_eraser_size = 5u;
        save_ctx.ui.tool_shape_stroke_width = 4u;
        save_ctx.ui.tool_shape_mode = 2u;
        save_ctx.ui.tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_OBJECT;
        save_ctx.ui.tool_fill_tolerance = 255u;
        save_ctx.ui.tool_select_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_SUBTRACT;
        if (!expect_ok(drawing_program_app_shutdown(&save_ctx), "persist_roundtrip_shutdown_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&load_ctx, 6, persist_argv), "persist_roundtrip_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&load_ctx), "persist_roundtrip_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&load_ctx), "persist_roundtrip_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_subsystems_init(&load_ctx), "persist_roundtrip_subsystems_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_start(&load_ctx), "persist_roundtrip_runtime_start_load")) {
            return 1;
        }
        if (load_ctx.editor.active_tool != DRAWING_PROGRAM_TOOL_PICKER ||
            load_ctx.ui.theme_preset_id != (uint32_t)CORE_THEME_PRESET_LIGHT_DEFAULT ||
            load_ctx.ui.font_zoom_step != 3 ||
            load_ctx.ui.left_panel_slot != 1u ||
            load_ctx.ui.right_panel_slot != 1u ||
            load_ctx.ui.active_color_index != 6u ||
            load_ctx.ui.tool_brush_size != 7u ||
            load_ctx.ui.tool_brush_opacity != 65u ||
            load_ctx.ui.tool_eraser_size != 5u ||
            load_ctx.ui.tool_shape_stroke_width != 4u ||
            load_ctx.ui.tool_shape_mode != 2u ||
            load_ctx.ui.tool_shape_target_mode != (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_OBJECT ||
            load_ctx.ui.tool_fill_tolerance != DRAWING_PROGRAM_UI_FILL_TOLERANCE_MAX ||
            load_ctx.ui.tool_select_mode != (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_SUBTRACT) {
            fprintf(stderr,
                    "lifecycle_test: persistence roundtrip mismatch tool=%u theme=%u zoom=%d left=%u right=%u color=%u brush=%u/%u eraser=%u shape=%u mode=%u target=%u fill_tol=%u select_mode=%u expected_fill_tol_max=%u\n",
                    (unsigned)load_ctx.editor.active_tool,
                    (unsigned)load_ctx.ui.theme_preset_id,
                    (int)load_ctx.ui.font_zoom_step,
                    (unsigned)load_ctx.ui.left_panel_slot,
                    (unsigned)load_ctx.ui.right_panel_slot,
                    (unsigned)load_ctx.ui.active_color_index,
                    (unsigned)load_ctx.ui.tool_brush_size,
                    (unsigned)load_ctx.ui.tool_brush_opacity,
                    (unsigned)load_ctx.ui.tool_eraser_size,
                    (unsigned)load_ctx.ui.tool_shape_stroke_width,
                    (unsigned)load_ctx.ui.tool_shape_mode,
                    (unsigned)load_ctx.ui.tool_shape_target_mode,
                    (unsigned)load_ctx.ui.tool_fill_tolerance,
                    (unsigned)load_ctx.ui.tool_select_mode,
                    (unsigned)DRAWING_PROGRAM_UI_FILL_TOLERANCE_MAX);
            return 1;
        }
        load_ctx.session.persist_enabled = 0u;
        if (!expect_ok(drawing_program_app_shutdown(&load_ctx), "persist_roundtrip_shutdown_load")) {
            return 1;
        }
    }

#undef workflow_ctx
#undef ctx
    return 0;
}
