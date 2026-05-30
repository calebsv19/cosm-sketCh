#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_texture_canvas_move.h"
#include "drawing_program/drawing_program_texture_project_session.h"
#include "drawing_program/drawing_program_texture_workspace.h"
#include "drawing_program/drawing_program_visual_canvas_action_ops.h"
#include "drawing_program/drawing_program_visual_canvas_coords.h"
#include "drawing_program/drawing_program_visual_input_handlers.h"
#include "drawing_program/drawing_program_visual_input_selection_ops.h"
#include "drawing_program/drawing_program_visual_input_support.h"
#include "drawing_program/drawing_program_visual_reflector_geometry.h"
#include "drawing_program/drawing_program_visual_shape_ops.h"
#include "drawing_program/drawing_program_visual_transform_ops.h"
#include "drawing_program_lifecycle_runtime_path_pointer_suite.h"
#include "drawing_program_lifecycle_test_support.h"

static int lifecycle_test_screen_to_canvas_sample(const DrawingProgramAppContext *ctx,
                                                  SDL_Rect pane_rect,
                                                  int sx,
                                                  int sy,
                                                  uint32_t *out_sample_x,
                                                  uint32_t *out_sample_y) {
    (void)ctx;
    (void)pane_rect;
    if (!out_sample_x || !out_sample_y || sx < 0 || sy < 0) {
        return 0;
    }
    *out_sample_x = (uint32_t)sx;
    *out_sample_y = (uint32_t)sy;
    return 1;
}

static int lifecycle_test_screen_to_canvas_sample_clamped(const DrawingProgramAppContext *ctx,
                                                          SDL_Rect pane_rect,
                                                          int sx,
                                                          int sy,
                                                          uint32_t *out_sample_x,
                                                          uint32_t *out_sample_y) {
    return lifecycle_test_screen_to_canvas_sample(ctx, pane_rect, sx, sy, out_sample_x, out_sample_y);
}

static int lifecycle_test_active_layer_query(const DrawingProgramAppContext *ctx,
                                             uint32_t *out_layer_id,
                                             uint32_t *out_index,
                                             uint8_t *out_visible,
                                             uint8_t *out_locked) {
    CoreResult result = drawing_program_runtime_orchestration_resolve_active_layer(ctx,
                                                                                   out_layer_id,
                                                                                   out_index,
                                                                                   out_visible,
                                                                                   out_locked);
    return (result.code == CORE_OK) ? 1 : 0;
}

static int lifecycle_test_active_layer_allows_edits_visual(const DrawingProgramAppContext *ctx) {
    uint8_t visible = 0u;
    uint8_t locked = 0u;
    return lifecycle_test_active_layer_query(ctx, 0, 0, &visible, &locked) ? ((visible && !locked) ? 1 : 0) : 0;
}

static CoreResult lifecycle_test_active_layer_sample_read_visual(const DrawingProgramAppContext *ctx,
                                                                 uint32_t sample_x,
                                                                 uint32_t sample_y,
                                                                 DrawingProgramRasterSample *out_value) {
    CoreResult result;
    if (!ctx || !out_value) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid test active-layer sample read" };
    }
    result = drawing_program_layer_raster_store_raster_sample_read(&ctx->layer_rasters,
                                                                   ctx->editor.active_layer_id,
                                                                   sample_x,
                                                                   sample_y,
                                                                   out_value);
    if (result.code == CORE_OK) {
        return core_result_ok();
    }
    return drawing_program_document_raster_sample_read(&ctx->document, sample_x, sample_y, out_value);
}

static int lifecycle_test_object_path_point_hit_test_selected(const DrawingProgramAppContext *ctx,
                                                              uint32_t sample_x,
                                                              uint32_t sample_y,
                                                              uint32_t *out_object_id,
                                                              uint16_t *out_point_index) {
    CoreResult result = drawing_program_object_store_hit_test_selected_path_point(&ctx->object_store,
                                                                                  &ctx->document,
                                                                                  &ctx->object_selection,
                                                                                  sample_x,
                                                                                  sample_y,
                                                                                  8u,
                                                                                  out_object_id,
                                                                                  out_point_index);
    return (result.code == CORE_OK) ? 1 : 0;
}

static int lifecycle_test_object_path_handle_hit_test_selected(const DrawingProgramAppContext *ctx,
                                                               uint32_t sample_x,
                                                               uint32_t sample_y,
                                                               uint32_t *out_object_id,
                                                               uint16_t *out_point_index,
                                                               uint8_t *out_handle_kind) {
    CoreResult result = drawing_program_object_store_hit_test_selected_path_handle(&ctx->object_store,
                                                                                   &ctx->document,
                                                                                   &ctx->object_selection,
                                                                                   sample_x,
                                                                                   sample_y,
                                                                                   14u,
                                                                                   out_object_id,
                                                                                   out_point_index,
                                                                                   out_handle_kind);
    return (result.code == CORE_OK) ? 1 : 0;
}

static int lifecycle_test_object_path_edge_hit_test_selected(const DrawingProgramAppContext *ctx,
                                                             uint32_t sample_x,
                                                             uint32_t sample_y,
                                                             uint32_t *out_object_id,
                                                             uint16_t *out_insert_index,
                                                             int32_t *out_point_x,
                                                             int32_t *out_point_y) {
    CoreResult result = drawing_program_object_store_hit_test_selected_path_edge(&ctx->object_store,
                                                                                 &ctx->document,
                                                                                 &ctx->object_selection,
                                                                                 sample_x,
                                                                                 sample_y,
                                                                                 6u,
                                                                                 out_object_id,
                                                                                 out_insert_index,
                                                                                 out_point_x,
                                                                                 out_point_y);
    return (result.code == CORE_OK) ? 1 : 0;
}

static CoreResult lifecycle_test_apply_insert_object_path_point(DrawingProgramAppContext *ctx,
                                                                uint32_t object_id,
                                                                uint16_t insert_index,
                                                                int32_t point_x,
                                                                int32_t point_y) {
    return drawing_program_history_apply_insert_object_path_point(
        &ctx->history, &ctx->object_store, object_id, insert_index, point_x, point_y);
}

static CoreResult lifecycle_test_object_commit_path_point_data(DrawingProgramAppContext *ctx,
                                                               uint32_t object_id,
                                                               uint16_t point_index,
                                                               const DrawingProgramPathPoint *point) {
    return drawing_program_history_apply_set_object_path_point_data(
        &ctx->history, &ctx->object_store, object_id, point_index, point);
}

static CoreResult lifecycle_test_commit_object_path_handle_move(DrawingProgramAppContext *ctx,
                                                                VisualCanvasInteractionState *interaction) {
    DrawingProgramVisualTransformOpsHooks hooks;
    memset(&hooks, 0, sizeof(hooks));
    hooks.visual_object_commit_path_point_data = lifecycle_test_object_commit_path_point_data;
    return drawing_program_visual_transform_session_commit_object_path_handle_move(ctx, interaction, &hooks);
}

int drawing_program_lifecycle_run_runtime_path_pointer_suite(DrawingProgramAppContext *workflow_ctx_ptr) {
#define workflow_ctx (*workflow_ctx_ptr)
    DrawingProgramVisualInputHandlersHooks hooks;
    DrawingProgramVisualCanvasActionOpsHooks fill_hooks;
    VisualCanvasInteractionState interaction;
    VisualPanelUiState panel_ui;
    SDL_Event event;

    memset(&hooks, 0, sizeof(hooks));
    memset(&fill_hooks, 0, sizeof(fill_hooks));
    hooks.cancel_canvas_draw_and_shape = lifecycle_test_cancel_canvas_draw_and_shape;
    hooks.cancel_selection_transient = lifecycle_test_cancel_selection_transient;
    hooks.cancel_all_transient_interactions = lifecycle_test_cancel_all_transient_interactions;
    hooks.delete_active_selection_payload_or_objects = delete_active_selection_payload_or_objects;
    hooks.path_draft_commit = path_draft_commit;
    hooks.screen_to_canvas_sample = lifecycle_test_screen_to_canvas_sample;
    hooks.screen_to_canvas_sample_clamped = lifecycle_test_screen_to_canvas_sample_clamped;
    hooks.object_path_point_hit_test_selected = lifecycle_test_object_path_point_hit_test_selected;
    hooks.object_path_handle_hit_test_selected = lifecycle_test_object_path_handle_hit_test_selected;
    hooks.object_path_edge_hit_test_selected = lifecycle_test_object_path_edge_hit_test_selected;
    hooks.apply_insert_object_path_point = lifecycle_test_apply_insert_object_path_point;
    hooks.visual_transform_session_is_object_path_handle_move_active =
        drawing_program_visual_transform_session_is_object_path_handle_move_active;
    hooks.visual_transform_session_update_object_path_handle_move =
        drawing_program_visual_transform_session_update_object_path_handle_move;
    hooks.visual_transform_session_commit_object_path_handle_move =
        lifecycle_test_commit_object_path_handle_move;
    hooks.visual_transform_session_begin_object_path_handle_move =
        drawing_program_visual_transform_session_begin_object_path_handle_move;
    hooks.path_draft_reset = path_draft_reset;
    hooks.path_draft_pop_point = path_draft_pop_point;
    fill_hooks.screen_to_canvas_sample = lifecycle_test_screen_to_canvas_sample;
    fill_hooks.active_layer_allows_edits_visual = lifecycle_test_active_layer_allows_edits_visual;
    fill_hooks.active_layer_query = lifecycle_test_active_layer_query;
    fill_hooks.sample_value_for_tool = drawing_program_visual_sample_value_for_tool;
    fill_hooks.tool_fill_tolerance_setting = drawing_program_visual_tool_fill_tolerance_setting;
    fill_hooks.fill_sample_matches_tolerance = drawing_program_visual_fill_sample_matches_tolerance;
    fill_hooks.visible_sample_read_visual = lifecycle_test_active_layer_sample_read_visual;
    fill_hooks.active_layer_sample_read_visual = lifecycle_test_active_layer_sample_read_visual;
    memset(&interaction, 0, sizeof(interaction));
    memset(&panel_ui, 0, sizeof(panel_ui));

    {
        SDL_Rect canvas_rect = { 0, 0, 240, 180 };
        VisualCanvasSheetMetrics side_metrics;
        uint32_t side_surface_index = 0u;

        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_selection_reset(&workflow_ctx.selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&interaction, 0, sizeof(interaction));
        memset(&panel_ui, 0, sizeof(panel_ui));
        workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_PATH;
        workflow_ctx.ui.canvas_control_mode = (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT;

        if (!expect_ok(drawing_program_texture_project_session_seed_blank(
                           &workflow_ctx, 128u, 96u, DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD),
                       "runtime_path_workspace_layout_seed_blank") ||
            !expect_ok(drawing_program_texture_project_session_add_surface(&workflow_ctx,
                                                                           "Right",
                                                                           64u,
                                                                           96u,
                                                                           1u,
                                                                           DRAWING_PROGRAM_TEXTURE_FACE_ROLE_RIGHT,
                                                                           DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD,
                                                                           &side_surface_index),
                       "runtime_path_workspace_layout_add_surface") ||
            !drawing_program_texture_workspace_fit_all(&workflow_ctx, canvas_rect) ||
            !drawing_program_texture_workspace_surface_sheet_metrics(
                &workflow_ctx, canvas_rect, side_surface_index, &side_metrics)) {
            fprintf(stderr, "lifecycle_test: expected workspace layout setup for atlas-shell click split\n");
            return 1;
        }

        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEBUTTONDOWN;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = side_metrics.sheet_rect.x + (side_metrics.sheet_rect.w / 2);
        event.button.y = side_metrics.sheet_rect.y + (side_metrics.sheet_rect.h / 2);
        if (!drawing_program_visual_input_handle_mouse_button_down_payload(&event,
                                                                           1,
                                                                           event.button.x,
                                                                           event.button.y,
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           1,
                                                                           canvas_rect,
                                                                           &workflow_ctx,
                                                                           &interaction,
                                                                           &workflow_ctx.selection,
                                                                           &panel_ui,
                                                                           &hooks)) {
            fprintf(stderr, "lifecycle_test: expected layout-mode atlas sheet click to be consumed\n");
            return 1;
        }
        if (workflow_ctx.texture_project.active_surface_index != side_surface_index ||
            !interaction.canvas_move_active ||
            interaction.canvas_move_surface_index != side_surface_index ||
            workflow_ctx.object_selection.count != 0u ||
            interaction.path_draft_active) {
            fprintf(stderr,
                    "lifecycle_test: expected atlas-shell layout click to switch active surface and start move without entering true edit path active=%u move=%u move_surface=%u selection=%u path_draft=%u\n",
                    (unsigned)workflow_ctx.texture_project.active_surface_index,
                    (unsigned)interaction.canvas_move_active,
                    (unsigned)interaction.canvas_move_surface_index,
                    (unsigned)workflow_ctx.object_selection.count,
                    (unsigned)interaction.path_draft_active);
            return 1;
        }
        drawing_program_texture_canvas_move_end(&interaction);
        workflow_ctx.ui.canvas_control_mode = (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_PAINT;
        if (!expect_ok(drawing_program_texture_project_session_seed_blank(
                           &workflow_ctx, 128u, 128u, DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD),
                       "runtime_path_workspace_layout_restore_blank")) {
            return 1;
        }
    }

    {
        const DrawingProgramObjectRecord *path_object = 0;
        SDL_Rect canvas_rect = { 0, 0, 160, 160 };

        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_selection_reset(&workflow_ctx.selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&interaction, 0, sizeof(interaction));
        workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_PATH;
        workflow_ctx.editor.active_layer_id = workflow_ctx.document.layers[0].layer_id;
        workflow_ctx.document.layers[0].visible = 1u;
        workflow_ctx.document.layers[0].locked = 0u;

        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEBUTTONDOWN;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = 40;
        event.button.y = 40;
        if (!drawing_program_visual_input_handle_mouse_button_down_payload(&event,
                                                                           1,
                                                                           40,
                                                                           40,
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           1,
                                                                           canvas_rect,
                                                                           &workflow_ctx,
                                                                           &interaction,
                                                                           &workflow_ctx.selection,
                                                                           &panel_ui,
                                                                           &hooks)) {
            fprintf(stderr, "lifecycle_test: expected first path draft click to be consumed\n");
            return 1;
        }
        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEBUTTONUP;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = 40;
        event.button.y = 40;
        (void)drawing_program_visual_input_handle_mouse_button_up_payload(&event,
                                                                          1,
                                                                          40,
                                                                          40,
                                                                          1,
                                                                          canvas_rect,
                                                                          &workflow_ctx,
                                                                          &interaction,
                                                                          &workflow_ctx.selection,
                                                                          &hooks);

        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEBUTTONDOWN;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = 80;
        event.button.y = 40;
        if (!drawing_program_visual_input_handle_mouse_button_down_payload(&event,
                                                                           1,
                                                                           80,
                                                                           40,
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           1,
                                                                           canvas_rect,
                                                                           &workflow_ctx,
                                                                           &interaction,
                                                                           &workflow_ctx.selection,
                                                                           &panel_ui,
                                                                           &hooks)) {
            fprintf(stderr, "lifecycle_test: expected dragged path draft click to be consumed\n");
            return 1;
        }
        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEMOTION;
        event.motion.x = 96;
        event.motion.y = 56;
        (void)drawing_program_visual_input_handle_mouse_motion_payload(&event,
                                                                       1,
                                                                       96,
                                                                       56,
                                                                       1,
                                                                       canvas_rect,
                                                                       &workflow_ctx,
                                                                       &interaction,
                                                                       &workflow_ctx.selection,
                                                                       &panel_ui,
                                                                       &hooks);
        if (interaction.path_draft_point_count != 2u ||
            !interaction.path_draft_points[1].bezier_enabled ||
            interaction.path_draft_points[1].handle_linked != 1u ||
            interaction.path_draft_points[1].handle_in_dx != -16 ||
            interaction.path_draft_points[1].handle_in_dy != -16 ||
            interaction.path_draft_points[1].handle_out_dx != 16 ||
            interaction.path_draft_points[1].handle_out_dy != 16) {
            fprintf(stderr,
                    "lifecycle_test: expected dragged draft point to promote to linked bezier in=(%d,%d) out=(%d,%d) enabled=%u linked=%u count=%u\n",
                    interaction.path_draft_points[1].handle_in_dx,
                    interaction.path_draft_points[1].handle_in_dy,
                    interaction.path_draft_points[1].handle_out_dx,
                    interaction.path_draft_points[1].handle_out_dy,
                    (unsigned)interaction.path_draft_points[1].bezier_enabled,
                    (unsigned)interaction.path_draft_points[1].handle_linked,
                    (unsigned)interaction.path_draft_point_count);
            return 1;
        }

        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEBUTTONUP;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = 96;
        event.button.y = 56;
        (void)drawing_program_visual_input_handle_mouse_button_up_payload(&event,
                                                                          1,
                                                                          96,
                                                                          56,
                                                                          1,
                                                                          canvas_rect,
                                                                          &workflow_ctx,
                                                                          &interaction,
                                                                          &workflow_ctx.selection,
                                                                          &hooks);
        if (interaction.path_draft_drag_active) {
            fprintf(stderr, "lifecycle_test: expected path draft drag state to clear on mouse release\n");
            return 1;
        }

        if (!expect_ok(path_draft_commit(&workflow_ctx, &interaction, 0u),
                       "path_drag_bezier_commit")) {
            return 1;
        }
        if (workflow_ctx.object_selection.count != 1u || workflow_ctx.object_selection.active_object_id == 0u) {
            fprintf(stderr, "lifecycle_test: expected committed dragged draft path to become active selection\n");
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store,
                                                             workflow_ctx.object_selection.active_object_id);
        if (!path_object ||
            path_object->path_point_count != 2u ||
            !path_object->path_points[1].bezier_enabled ||
            path_object->path_points[1].handle_linked != 1u ||
            path_object->path_points[1].handle_in_dx != -16 ||
            path_object->path_points[1].handle_in_dy != -16 ||
            path_object->path_points[1].handle_out_dx != 16 ||
            path_object->path_points[1].handle_out_dy != 16) {
            fprintf(stderr,
                    "lifecycle_test: expected committed dragged draft point to preserve bezier handles in=(%d,%d) out=(%d,%d) enabled=%u linked=%u points=%u\n",
                    path_object ? path_object->path_points[1].handle_in_dx : 0,
                    path_object ? path_object->path_points[1].handle_in_dy : 0,
                    path_object ? path_object->path_points[1].handle_out_dx : 0,
                    path_object ? path_object->path_points[1].handle_out_dy : 0,
                    path_object ? (unsigned)path_object->path_points[1].bezier_enabled : 0u,
                    path_object ? (unsigned)path_object->path_points[1].handle_linked : 0u,
                    path_object ? (unsigned)path_object->path_point_count : 0u);
            return 1;
        }
    }
    {
        uint8_t fill_before_a = 0u;
        uint8_t fill_before_b = 0u;
        uint8_t fill_before_c = 0u;
        uint8_t fill_after_a = 0u;
        uint8_t fill_after_b = 0u;
        uint8_t fill_after_c = 0u;
        uint8_t fill_after_outside = 0u;
        uint8_t fill_after_undo_a = 0u;
        uint8_t fill_after_undo_b = 0u;
        uint8_t fill_after_undo_c = 0u;
        uint8_t replacement = 0u;
        uint8_t base_a = 16u;
        uint8_t base_b = 20u;
        uint32_t history_units = 0u;
        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_selection_reset(&workflow_ctx.selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&interaction, 0, sizeof(interaction));
        workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_FILL;
        workflow_ctx.editor.active_layer_id = workflow_ctx.document.layers[0].layer_id;
        workflow_ctx.document.layers[0].visible = 1u;
        workflow_ctx.document.layers[0].locked = 0u;
        workflow_ctx.ui.tool_fill_tolerance = 1u;
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "fill_tolerance_clear_canvas")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_write(&workflow_ctx.layer_rasters,
                                                                       workflow_ctx.editor.active_layer_id,
                                                                       10u,
                                                                       10u,
                                                                       base_a,
                                                                       0),
                       "fill_tolerance_seed_layer_a")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_write(&workflow_ctx.layer_rasters,
                                                                       workflow_ctx.editor.active_layer_id,
                                                                       11u,
                                                                       10u,
                                                                       base_b,
                                                                       0),
                       "fill_tolerance_seed_layer_b")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_write(&workflow_ctx.layer_rasters,
                                                                       workflow_ctx.editor.active_layer_id,
                                                                       12u,
                                                                       10u,
                                                                       base_a,
                                                                       0),
                       "fill_tolerance_seed_layer_c")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_write(&workflow_ctx.document, 10u, 10u, base_a, 0),
                       "fill_tolerance_seed_doc_a")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_write(&workflow_ctx.document, 11u, 10u, base_b, 0),
                       "fill_tolerance_seed_doc_b")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_write(&workflow_ctx.document, 12u, 10u, base_a, 0),
                       "fill_tolerance_seed_doc_c")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, 10u, 10u, &fill_before_a),
                       "fill_tolerance_before_a")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, 11u, 10u, &fill_before_b),
                       "fill_tolerance_before_b")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, 12u, 10u, &fill_before_c),
                       "fill_tolerance_before_c")) {
            return 1;
        }
        if (fill_before_a != base_a || fill_before_b != base_b || fill_before_c != base_a) {
            fprintf(stderr,
                    "lifecycle_test: expected mixed-value fill seed row %u,%u,%u got %u,%u,%u\n",
                    (unsigned)base_a,
                    (unsigned)base_b,
                    (unsigned)base_a,
                    (unsigned)fill_before_a,
                    (unsigned)fill_before_b,
                    (unsigned)fill_before_c);
            return 1;
        }
        replacement = drawing_program_visual_sample_value_for_tool(&workflow_ctx, workflow_ctx.editor.active_tool);
        if (!expect_ok(drawing_program_visual_apply_canvas_fill_at_screen(&workflow_ctx,
                                                                          (SDL_Rect){ 0, 0, 128, 128 },
                                                                          10,
                                                                          10,
                                                                          &fill_hooks),
                       "fill_tolerance_mixed_span_apply")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, 10u, 10u, &fill_after_a),
                       "fill_tolerance_after_a")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, 11u, 10u, &fill_after_b),
                       "fill_tolerance_after_b")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, 12u, 10u, &fill_after_c),
                       "fill_tolerance_after_c")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, 13u, 10u, &fill_after_outside),
                       "fill_tolerance_after_outside")) {
            return 1;
        }
        if (fill_after_a != replacement || fill_after_b != replacement || fill_after_c != replacement) {
            fprintf(stderr,
                    "lifecycle_test: expected mixed-value tolerance fill to replace row with %u got %u,%u,%u\n",
                    (unsigned)replacement,
                    (unsigned)fill_after_a,
                    (unsigned)fill_after_b,
                    (unsigned)fill_after_c);
            return 1;
        }
        if (fill_after_outside !=
            drawing_program_color_legacy_sample_from_sample(drawing_program_color_eraser_value())) {
            fprintf(stderr,
                    "lifecycle_test: expected tolerance fill to stop before outside sample got=%u\n",
                    (unsigned)fill_after_outside);
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &history_units, 0);
        if (history_units != 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected mixed-value tolerance fill to record as 1 history unit got=%u\n",
                    (unsigned)history_units);
            return 1;
        }
        if (!expect_ok(drawing_program_history_undo(&workflow_ctx.history,
                                                    &workflow_ctx.document,
                                                    &workflow_ctx.layer_rasters,
                                                    &workflow_ctx.object_store),
                       "fill_tolerance_undo")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, 10u, 10u, &fill_after_undo_a),
                       "fill_tolerance_after_undo_a")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, 11u, 10u, &fill_after_undo_b),
                       "fill_tolerance_after_undo_b")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document, 12u, 10u, &fill_after_undo_c),
                       "fill_tolerance_after_undo_c")) {
            return 1;
        }
        if (fill_after_undo_a != base_a || fill_after_undo_b != base_b || fill_after_undo_c != base_a) {
            fprintf(stderr,
                    "lifecycle_test: expected fill undo to restore %u,%u,%u got %u,%u,%u\n",
                    (unsigned)base_a,
                    (unsigned)base_b,
                    (unsigned)base_a,
                    (unsigned)fill_after_undo_a,
                    (unsigned)fill_after_undo_b,
                    (unsigned)fill_after_undo_c);
            return 1;
        }
    }
    {
        uint8_t fill_center_after = 0u;
        uint8_t fill_center_undo = 0u;
        uint8_t replacement = 0u;
        uint32_t history_units_before_undo = 0u;
        uint32_t history_units = 0u;
        uint32_t undo_count = 0u;
        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_selection_reset(&workflow_ctx.selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&interaction, 0, sizeof(interaction));
        workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_FILL;
        workflow_ctx.editor.active_layer_id = workflow_ctx.document.layers[0].layer_id;
        workflow_ctx.document.layers[0].visible = 1u;
        workflow_ctx.document.layers[0].locked = 0u;
        workflow_ctx.ui.tool_fill_tolerance = 0u;
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                       "fill_large_clear_canvas")) {
            return 1;
        }
        replacement = drawing_program_visual_sample_value_for_tool(&workflow_ctx, workflow_ctx.editor.active_tool);
        if (!expect_ok(drawing_program_visual_apply_canvas_fill_at_screen(&workflow_ctx,
                                                                          (SDL_Rect){ 0, 0, 128, 128 },
                                                                          0,
                                                                          0,
                                                                          &fill_hooks),
                       "fill_large_apply")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document,
                                                            workflow_ctx.document.raster_width / 2u,
                                                            workflow_ctx.document.raster_height / 2u,
                                                            &fill_center_after),
                       "fill_large_center_after")) {
            return 1;
        }
        if (fill_center_after != replacement) {
            fprintf(stderr,
                    "lifecycle_test: expected large fill to replace center with %u got=%u\n",
                    (unsigned)replacement,
                    (unsigned)fill_center_after);
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &history_units, 0);
        if (history_units != 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected fill to remain one undo unit regardless of size got=%u\n",
                    (unsigned)history_units);
            return 1;
        }
        history_units_before_undo = history_units;
        if (workflow_ctx.document.raster_sample_count > DRAWING_PROGRAM_HISTORY_DELTA_BLOCK_FLUSH_CAPACITY &&
            workflow_ctx.history.count <= 3u) {
            fprintf(stderr,
                    "lifecycle_test: expected large fill to span multiple delta-block commands count=%u samples=%u\n",
                    (unsigned)workflow_ctx.history.count,
                    (unsigned)workflow_ctx.document.raster_sample_count);
            return 1;
        }
        while (drawing_program_history_undo(&workflow_ctx.history,
                                            &workflow_ctx.document,
                                            &workflow_ctx.layer_rasters,
                                            &workflow_ctx.object_store)
                   .code == CORE_OK) {
            undo_count += 1u;
        }
        if (undo_count != history_units_before_undo) {
            fprintf(stderr,
                    "lifecycle_test: expected large fill undo count=%u to match history units=%u\n",
                    (unsigned)undo_count,
                    (unsigned)history_units_before_undo);
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document,
                                                            workflow_ctx.document.raster_width / 2u,
                                                            workflow_ctx.document.raster_height / 2u,
                                                            &fill_center_undo),
                       "fill_large_center_undo")) {
            return 1;
        }
        if (fill_center_undo !=
            drawing_program_color_legacy_sample_from_sample(drawing_program_color_eraser_value())) {
            fprintf(stderr,
                    "lifecycle_test: expected large fill undo to restore clear canvas center got=%u\n",
                    (unsigned)fill_center_undo);
            return 1;
        }
    }
    {
        DrawingProgramVisualInputHandlersHooks reflector_hooks;
        SDL_Rect canvas_rect = { 0, 0, 320, 240 };
        VisualCanvasSheetMetrics metrics;
        SDL_Point anchor_handle;
        SDL_Point direction_handle;
        const DrawingProgramReflectionState *state = 0;
        uint32_t expected_sample_x = 0u;
        uint32_t expected_sample_y = 0u;
        memset(&interaction, 0, sizeof(interaction));
        memset(&panel_ui, 0, sizeof(panel_ui));
        workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_MOVE;
        workflow_ctx.ui.canvas_control_mode = (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_PAINT;
        reflector_hooks = hooks;
        reflector_hooks.screen_to_canvas_sample = drawing_program_visual_screen_to_canvas_sample;
        reflector_hooks.screen_to_canvas_sample_clamped = drawing_program_visual_screen_to_canvas_sample_clamped;
        if (!expect_ok(drawing_program_texture_project_session_seed_blank(
                           &workflow_ctx, 128u, 96u, DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD),
                       "runtime_reflector_seed_blank")) {
            return 1;
        }
        if (!drawing_program_canvas_reflection_add_active_reflector(&workflow_ctx, 1, 1)) {
            fprintf(stderr, "lifecycle_test: expected reflector add helper to succeed for drag test\n");
            return 1;
        }
        state = drawing_program_canvas_reflection_active_state(&workflow_ctx);
        if (!state ||
            !drawing_program_texture_workspace_surface_sheet_metrics(
                &workflow_ctx, canvas_rect, workflow_ctx.texture_project.active_surface_index, &metrics) ||
            !drawing_program_visual_reflector_screen_handles(
                &metrics, &state->reflectors[state->active_reflector_index], &anchor_handle, &direction_handle)) {
            fprintf(stderr, "lifecycle_test: expected reflector drag test metrics/handles to resolve\n");
            return 1;
        }
        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEBUTTONDOWN;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = direction_handle.x;
        event.button.y = direction_handle.y;
        if (!drawing_program_visual_input_handle_mouse_button_down_payload(&event,
                                                                           1,
                                                                           direction_handle.x,
                                                                           direction_handle.y,
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           1,
                                                                           canvas_rect,
                                                                           &workflow_ctx,
                                                                           &interaction,
                                                                           &workflow_ctx.selection,
                                                                           &panel_ui,
                                                                           &reflector_hooks) ||
            !interaction.reflector_drag_active ||
            interaction.reflector_drag_kind != 2u) {
            fprintf(stderr, "lifecycle_test: expected direction-handle click to begin reflector direction drag\n");
            return 1;
        }
        if (!drawing_program_visual_screen_to_canvas_sample_clamped(&workflow_ctx,
                                                                    canvas_rect,
                                                                    direction_handle.x + 24,
                                                                    direction_handle.y - 10,
                                                                    &expected_sample_x,
                                                                    &expected_sample_y)) {
            fprintf(stderr, "lifecycle_test: expected reflector direction drag target sample to resolve\n");
            return 1;
        }
        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEMOTION;
        event.motion.x = direction_handle.x + 24;
        event.motion.y = direction_handle.y - 10;
        (void)drawing_program_visual_input_handle_mouse_motion_payload(&event,
                                                                       1,
                                                                       direction_handle.x + 24,
                                                                       direction_handle.y - 10,
                                                                       1,
                                                                       canvas_rect,
                                                                       &workflow_ctx,
                                                                       &interaction,
                                                                       &workflow_ctx.selection,
                                                                       &panel_ui,
                                                                       &reflector_hooks);
        state = drawing_program_canvas_reflection_active_state(&workflow_ctx);
        if (!state ||
            state->reflectors[state->active_reflector_index].direction_dx !=
                (int32_t)expected_sample_x - (int32_t)state->center_x ||
            state->reflectors[state->active_reflector_index].direction_dy !=
                (int32_t)expected_sample_y - (int32_t)state->center_y) {
            fprintf(stderr,
                    "lifecycle_test: expected reflector direction drag to update direction to %d,%d got=%d,%d\n",
                    (int32_t)expected_sample_x - (int32_t)(state ? state->center_x : 0u),
                    (int32_t)expected_sample_y - (int32_t)(state ? state->center_y : 0u),
                    state ? state->reflectors[state->active_reflector_index].direction_dx : 0,
                    state ? state->reflectors[state->active_reflector_index].direction_dy : 0);
            return 1;
        }
        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEBUTTONUP;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = direction_handle.x + 24;
        event.button.y = direction_handle.y - 10;
        (void)drawing_program_visual_input_handle_mouse_button_up_payload(&event,
                                                                          1,
                                                                          direction_handle.x + 24,
                                                                          direction_handle.y - 10,
                                                                          1,
                                                                          canvas_rect,
                                                                          &workflow_ctx,
                                                                          &interaction,
                                                                          &workflow_ctx.selection,
                                                                          &reflector_hooks);
        if (interaction.reflector_drag_active) {
            fprintf(stderr, "lifecycle_test: expected reflector direction drag to clear on mouse release\n");
            return 1;
        }
        state = drawing_program_canvas_reflection_active_state(&workflow_ctx);
        if (!state ||
            !drawing_program_visual_reflector_screen_handles(
                &metrics, &state->reflectors[state->active_reflector_index], &anchor_handle, &direction_handle)) {
            fprintf(stderr, "lifecycle_test: expected reflector anchor handle to resolve after direction drag\n");
            return 1;
        }
        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEBUTTONDOWN;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = anchor_handle.x;
        event.button.y = anchor_handle.y;
        if (!drawing_program_visual_input_handle_mouse_button_down_payload(&event,
                                                                           1,
                                                                           anchor_handle.x,
                                                                           anchor_handle.y,
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           1,
                                                                           canvas_rect,
                                                                           &workflow_ctx,
                                                                           &interaction,
                                                                           &workflow_ctx.selection,
                                                                           &panel_ui,
                                                                           &reflector_hooks) ||
            !interaction.reflector_drag_active ||
            interaction.reflector_drag_kind != 1u) {
            fprintf(stderr, "lifecycle_test: expected anchor-handle click to begin reflector center drag\n");
            return 1;
        }
        if (!drawing_program_visual_screen_to_canvas_sample_clamped(&workflow_ctx,
                                                                    canvas_rect,
                                                                    anchor_handle.x + 18,
                                                                    anchor_handle.y + 12,
                                                                    &expected_sample_x,
                                                                    &expected_sample_y)) {
            fprintf(stderr, "lifecycle_test: expected reflector center drag target sample to resolve\n");
            return 1;
        }
        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEMOTION;
        event.motion.x = anchor_handle.x + 18;
        event.motion.y = anchor_handle.y + 12;
        (void)drawing_program_visual_input_handle_mouse_motion_payload(&event,
                                                                       1,
                                                                       anchor_handle.x + 18,
                                                                       anchor_handle.y + 12,
                                                                       1,
                                                                       canvas_rect,
                                                                       &workflow_ctx,
                                                                       &interaction,
                                                                       &workflow_ctx.selection,
                                                                       &panel_ui,
                                                                       &reflector_hooks);
        state = drawing_program_canvas_reflection_active_state(&workflow_ctx);
        if (!state || state->center_x != expected_sample_x || state->center_y != expected_sample_y) {
            fprintf(stderr,
                    "lifecycle_test: expected reflector center drag to move center to %u,%u got=%u,%u\n",
                    (unsigned)expected_sample_x,
                    (unsigned)expected_sample_y,
                    (unsigned)(state ? state->center_x : 0u),
                    (unsigned)(state ? state->center_y : 0u));
            return 1;
        }
        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEBUTTONUP;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = anchor_handle.x + 18;
        event.button.y = anchor_handle.y + 12;
        (void)drawing_program_visual_input_handle_mouse_button_up_payload(&event,
                                                                          1,
                                                                          anchor_handle.x + 18,
                                                                          anchor_handle.y + 12,
                                                                          1,
                                                                          canvas_rect,
                                                                          &workflow_ctx,
                                                                          &interaction,
                                                                          &workflow_ctx.selection,
                                                                          &reflector_hooks);
        if (interaction.reflector_drag_active) {
            fprintf(stderr, "lifecycle_test: expected reflector center drag to clear on mouse release\n");
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        const DrawingProgramObjectRecord *path_object = 0;
        SDL_Rect canvas_rect = { 0, 0, 160, 160 };
        uint32_t object_id = 0u;

        memset(&path_seed, 0, sizeof(path_seed));
        memset(&path_payload, 0, sizeof(path_payload));
        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_selection_reset(&workflow_ctx.selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&interaction, 0, sizeof(interaction));
        workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_SELECT;
        path_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
        path_seed.visible = 1u;
        path_seed.locked = 0u;
        path_seed.stroke_width = 2u;
        path_seed.style_mode = 0u;
        path_payload.point_count = 2u;
        path_payload.closed = 0u;
        path_payload.points[0].x = 40;
        path_payload.points[0].y = 40;
        path_payload.points[0].bezier_enabled = 1u;
        path_payload.points[0].handle_linked = 1u;
        path_payload.points[0].handle_out_dx = 16;
        path_payload.points[0].handle_out_dy = 8;
        path_payload.points[1].x = 88;
        path_payload.points[1].y = 40;
        if (!expect_ok(drawing_program_object_store_add_path(&workflow_ctx.object_store,
                                                             &path_seed,
                                                             &path_payload,
                                                             &object_id),
                       "select_mode_handle_drag_seed_add")) {
            return 1;
        }
        drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, object_id);
        if (!drawing_program_object_selection_set_path_point(&workflow_ctx.object_selection, object_id, 0u)) {
            fprintf(stderr, "lifecycle_test: expected selected path point for select-mode handle drag\n");
            return 1;
        }

        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEBUTTONDOWN;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = 58;
        event.button.y = 50;
        if (!drawing_program_visual_input_handle_mouse_button_down_payload(&event,
                                                                           1,
                                                                           58,
                                                                           50,
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           1,
                                                                           canvas_rect,
                                                                           &workflow_ctx,
                                                                           &interaction,
                                                                           &workflow_ctx.selection,
                                                                           &panel_ui,
                                                                           &hooks)) {
            fprintf(stderr, "lifecycle_test: expected select-mode bezier handle click to be consumed\n");
            return 1;
        }
        if (!interaction.object_path_handle_move_active ||
            interaction.object_path_handle_object_id != object_id ||
            interaction.object_path_handle_point_index != 0u ||
            interaction.object_path_handle_kind != (uint8_t)DRAWING_PROGRAM_PATH_HANDLE_OUT ||
            workflow_ctx.object_selection.count != 1u ||
            workflow_ctx.object_selection.active_object_id != object_id) {
            fprintf(stderr,
                    "lifecycle_test: expected select-mode handle click to preserve selection and start drag active=%u obj=%u idx=%u kind=%u count=%u active=%u\n",
                    (unsigned)interaction.object_path_handle_move_active,
                    (unsigned)interaction.object_path_handle_object_id,
                    (unsigned)interaction.object_path_handle_point_index,
                    (unsigned)interaction.object_path_handle_kind,
                    (unsigned)workflow_ctx.object_selection.count,
                    (unsigned)workflow_ctx.object_selection.active_object_id);
            return 1;
        }

        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEMOTION;
        event.motion.x = 66;
        event.motion.y = 58;
        (void)drawing_program_visual_input_handle_mouse_motion_payload(&event,
                                                                       1,
                                                                       66,
                                                                       58,
                                                                       1,
                                                                       canvas_rect,
                                                                       &workflow_ctx,
                                                                       &interaction,
                                                                       &workflow_ctx.selection,
                                                                       &panel_ui,
                                                                       &hooks);

        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEBUTTONUP;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = 66;
        event.button.y = 58;
        (void)drawing_program_visual_input_handle_mouse_button_up_payload(&event,
                                                                          1,
                                                                          66,
                                                                          58,
                                                                          1,
                                                                          canvas_rect,
                                                                          &workflow_ctx,
                                                                          &interaction,
                                                                          &workflow_ctx.selection,
                                                                          &hooks);

        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!path_object ||
            path_object->path_points[0].handle_out_dx != 26 ||
            path_object->path_points[0].handle_out_dy != 18 ||
            workflow_ctx.object_selection.count != 1u ||
            workflow_ctx.object_selection.active_object_id != object_id) {
            fprintf(stderr,
                    "lifecycle_test: expected select-mode handle drag commit without deselect handle=(%d,%d) count=%u active=%u\n",
                    path_object ? path_object->path_points[0].handle_out_dx : 0,
                    path_object ? path_object->path_points[0].handle_out_dy : 0,
                    (unsigned)workflow_ctx.object_selection.count,
                    (unsigned)workflow_ctx.object_selection.active_object_id);
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        const DrawingProgramObjectRecord *path_object = 0;
        uint32_t object_id = 0u;
        uint32_t history_before = 0u;
        uint32_t history_after = 0u;
        memset(&path_seed, 0, sizeof(path_seed));
        memset(&path_payload, 0, sizeof(path_payload));
        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_selection_reset(&workflow_ctx.selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&interaction, 0, sizeof(interaction));
        workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_PATH;
        path_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
        path_seed.visible = 1u;
        path_seed.locked = 0u;
        path_seed.stroke_width = 2u;
        path_seed.style_mode = 0u;
        path_payload.point_count = 2u;
        path_payload.closed = 0u;
        path_payload.points[0].x = 40;
        path_payload.points[0].y = 40;
        path_payload.points[1].x = 80;
        path_payload.points[1].y = 40;
        if (!expect_ok(drawing_program_object_store_add_path(&workflow_ctx.object_store,
                                                             &path_seed,
                                                             &path_payload,
                                                             &object_id),
                       "path_insert_handler_seed_add")) {
            return 1;
        }
        drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, object_id);
        drawing_program_history_query_units(&workflow_ctx.history, &history_before, 0);
        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEBUTTONDOWN;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = 60;
        event.button.y = 41;
        if (!drawing_program_visual_input_handle_mouse_button_down_payload(&event,
                                                                           1,
                                                                           60,
                                                                           41,
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           1,
                                                                           (SDL_Rect){ 0, 0, 128, 128 },
                                                                           &workflow_ctx,
                                                                           &interaction,
                                                                           &workflow_ctx.selection,
                                                                           &panel_ui,
                                                                           &hooks)) {
            fprintf(stderr, "lifecycle_test: expected path edge click insertion to be consumed\n");
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &history_after, 0);
        if (history_after != history_before + 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected path edge insertion to append one history unit before=%u after=%u\n",
                    (unsigned)history_before,
                    (unsigned)history_after);
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!path_object ||
            path_object->path_point_count != 3u ||
            path_object->path_points[1].x != 60 ||
            path_object->path_points[1].y != 40) {
            fprintf(stderr,
                    "lifecycle_test: expected path edge insertion to place midpoint at index 1 count=%u point=(%d,%d)\n",
                    path_object ? (unsigned)path_object->path_point_count : 0u,
                    path_object ? (int)path_object->path_points[1].x : 0,
                    path_object ? (int)path_object->path_points[1].y : 0);
            return 1;
        }
        if (!workflow_ctx.object_selection.selected_path_point_active ||
            workflow_ctx.object_selection.selected_path_point_object_id != object_id ||
            workflow_ctx.object_selection.selected_path_point_index != 1u) {
            fprintf(stderr, "lifecycle_test: expected inserted path point to become active selection target\n");
            return 1;
        }
        if (interaction.path_draft_active || interaction.path_draft_point_count != 0u) {
            fprintf(stderr, "lifecycle_test: edge insertion should not start a path draft session\n");
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        const DrawingProgramObjectRecord *path_object = 0;
        uint32_t object_id = 0u;
        uint32_t history_before = 0u;
        uint32_t history_after = 0u;
        memset(&path_seed, 0, sizeof(path_seed));
        memset(&path_payload, 0, sizeof(path_payload));
        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_selection_reset(&workflow_ctx.selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&interaction, 0, sizeof(interaction));
        workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_PATH;
        path_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
        path_seed.visible = 1u;
        path_seed.locked = 0u;
        path_seed.stroke_width = 2u;
        path_seed.style_mode = 0u;
        path_payload.point_count = 2u;
        path_payload.closed = 0u;
        path_payload.points[0].x = 40;
        path_payload.points[0].y = 40;
        path_payload.points[1].x = 80;
        path_payload.points[1].y = 40;
        if (!expect_ok(drawing_program_object_store_add_path(&workflow_ctx.object_store,
                                                             &path_seed,
                                                             &path_payload,
                                                             &object_id),
                       "path_append_selected_endpoint_seed_add")) {
            return 1;
        }
        drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, object_id);
        if (!drawing_program_object_selection_set_path_point(&workflow_ctx.object_selection, object_id, 1u)) {
            fprintf(stderr, "lifecycle_test: expected selected endpoint set for path append\n");
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &history_before, 0);
        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEBUTTONDOWN;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = 96;
        event.button.y = 60;
        if (!drawing_program_visual_input_handle_mouse_button_down_payload(&event,
                                                                           1,
                                                                           96,
                                                                           60,
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           1,
                                                                           (SDL_Rect){ 0, 0, 128, 128 },
                                                                           &workflow_ctx,
                                                                           &interaction,
                                                                           &workflow_ctx.selection,
                                                                           &panel_ui,
                                                                           &hooks)) {
            fprintf(stderr, "lifecycle_test: expected selected-endpoint append click to be consumed\n");
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &history_after, 0);
        if (history_after != history_before + 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected selected-endpoint append to append one history unit before=%u after=%u\n",
                    (unsigned)history_before,
                    (unsigned)history_after);
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!path_object ||
            path_object->path_point_count != 3u ||
            path_object->path_points[2].x != 96 ||
            path_object->path_points[2].y != 60 ||
            !workflow_ctx.object_selection.selected_path_point_active ||
            workflow_ctx.object_selection.selected_path_point_object_id != object_id ||
            workflow_ctx.object_selection.selected_path_point_index != 2u) {
            fprintf(stderr,
                    "lifecycle_test: selected-endpoint append produced wrong topology count=%u last=(%d,%d) selected=%u obj=%u idx=%u\n",
                    path_object ? (unsigned)path_object->path_point_count : 0u,
                    path_object ? path_object->path_points[2].x : 0,
                    path_object ? path_object->path_points[2].y : 0,
                    (unsigned)workflow_ctx.object_selection.selected_path_point_active,
                    (unsigned)workflow_ctx.object_selection.selected_path_point_object_id,
                    (unsigned)workflow_ctx.object_selection.selected_path_point_index);
            return 1;
        }
        if (interaction.path_draft_active || interaction.path_draft_point_count != 0u) {
            fprintf(stderr, "lifecycle_test: selected-endpoint append should not start a path draft session\n");
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        const DrawingProgramObjectRecord *path_object = 0;
        uint32_t object_id = 0u;
        uint32_t history_before = 0u;
        uint32_t history_after = 0u;
        memset(&path_seed, 0, sizeof(path_seed));
        memset(&path_payload, 0, sizeof(path_payload));
        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_selection_reset(&workflow_ctx.selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&interaction, 0, sizeof(interaction));
        workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_PATH;
        path_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
        path_seed.visible = 1u;
        path_seed.locked = 0u;
        path_seed.stroke_width = 2u;
        path_seed.style_mode = 0u;
        path_payload.point_count = 2u;
        path_payload.closed = 0u;
        path_payload.points[0].x = 40;
        path_payload.points[0].y = 40;
        path_payload.points[1].x = 80;
        path_payload.points[1].y = 40;
        if (!expect_ok(drawing_program_object_store_add_path(&workflow_ctx.object_store,
                                                             &path_seed,
                                                             &path_payload,
                                                             &object_id),
                       "path_append_nearest_endpoint_seed_add")) {
            return 1;
        }
        drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, object_id);
        drawing_program_history_query_units(&workflow_ctx.history, &history_before, 0);
        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEBUTTONDOWN;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = 24;
        event.button.y = 60;
        if (!drawing_program_visual_input_handle_mouse_button_down_payload(&event,
                                                                           1,
                                                                           24,
                                                                           60,
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           1,
                                                                           (SDL_Rect){ 0, 0, 128, 128 },
                                                                           &workflow_ctx,
                                                                           &interaction,
                                                                           &workflow_ctx.selection,
                                                                           &panel_ui,
                                                                           &hooks)) {
            fprintf(stderr, "lifecycle_test: expected nearest-endpoint append click to be consumed\n");
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &history_after, 0);
        if (history_after != history_before + 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected nearest-endpoint append to append one history unit before=%u after=%u\n",
                    (unsigned)history_before,
                    (unsigned)history_after);
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!path_object ||
            path_object->path_point_count != 3u ||
            path_object->path_points[0].x != 24 ||
            path_object->path_points[0].y != 60 ||
            path_object->path_points[1].x != 40 ||
            path_object->path_points[1].y != 40 ||
            !workflow_ctx.object_selection.selected_path_point_active ||
            workflow_ctx.object_selection.selected_path_point_object_id != object_id ||
            workflow_ctx.object_selection.selected_path_point_index != 0u) {
            fprintf(stderr,
                    "lifecycle_test: nearest-endpoint append produced wrong topology count=%u first=(%d,%d) next=(%d,%d) selected=%u obj=%u idx=%u\n",
                    path_object ? (unsigned)path_object->path_point_count : 0u,
                    path_object ? path_object->path_points[0].x : 0,
                    path_object ? path_object->path_points[0].y : 0,
                    path_object ? path_object->path_points[1].x : 0,
                    path_object ? path_object->path_points[1].y : 0,
                    (unsigned)workflow_ctx.object_selection.selected_path_point_active,
                    (unsigned)workflow_ctx.object_selection.selected_path_point_object_id,
                    (unsigned)workflow_ctx.object_selection.selected_path_point_index);
            return 1;
        }
        if (interaction.path_draft_active || interaction.path_draft_point_count != 0u) {
            fprintf(stderr, "lifecycle_test: nearest-endpoint append should not start a path draft session\n");
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        const DrawingProgramObjectRecord *path_object = 0;
        uint32_t object_id = 0u;
        uint32_t history_before = 0u;
        uint32_t history_after = 0u;
        memset(&path_seed, 0, sizeof(path_seed));
        memset(&path_payload, 0, sizeof(path_payload));
        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_selection_reset(&workflow_ctx.selection);
        drawing_program_history_clear(&workflow_ctx.history);
        memset(&interaction, 0, sizeof(interaction));
        workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_PATH;
        path_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
        path_seed.visible = 1u;
        path_seed.locked = 0u;
        path_seed.stroke_width = 2u;
        path_seed.style_mode = 0u;
        path_payload.point_count = 3u;
        path_payload.closed = 1u;
        path_payload.points[0].x = 32;
        path_payload.points[0].y = 32;
        path_payload.points[1].x = 56;
        path_payload.points[1].y = 32;
        path_payload.points[2].x = 44;
        path_payload.points[2].y = 56;
        if (!expect_ok(drawing_program_object_store_add_path(&workflow_ctx.object_store,
                                                             &path_seed,
                                                             &path_payload,
                                                             &object_id),
                       "path_closed_miss_no_append_seed_add")) {
            return 1;
        }
        drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, object_id);
        drawing_program_history_query_units(&workflow_ctx.history, &history_before, 0);
        memset(&event, 0, sizeof(event));
        event.type = SDL_MOUSEBUTTONDOWN;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = 96;
        event.button.y = 96;
        if (!drawing_program_visual_input_handle_mouse_button_down_payload(&event,
                                                                           1,
                                                                           96,
                                                                           96,
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           0,
                                                                           (SDL_Rect){ 0, 0, 0, 0 },
                                                                           1,
                                                                           (SDL_Rect){ 0, 0, 128, 128 },
                                                                           &workflow_ctx,
                                                                           &interaction,
                                                                           &workflow_ctx.selection,
                                                                           &panel_ui,
                                                                           &hooks)) {
            fprintf(stderr, "lifecycle_test: expected closed-path miss click to be consumed by draft fallback\n");
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &history_after, 0);
        if (history_after != history_before) {
            fprintf(stderr,
                    "lifecycle_test: closed-path miss should not append history before=%u after=%u\n",
                    (unsigned)history_before,
                    (unsigned)history_after);
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!path_object ||
            path_object->path_point_count != 3u ||
            !path_object->path_closed ||
            !interaction.path_draft_active ||
            interaction.path_draft_point_count != 1u ||
            interaction.path_draft_points[0].x != 96 ||
            interaction.path_draft_points[0].y != 96) {
            fprintf(stderr,
                    "lifecycle_test: closed-path miss should preserve object and start new draft count=%u closed=%u draft_active=%u draft_points=%u first=(%d,%d)\n",
                    path_object ? (unsigned)path_object->path_point_count : 0u,
                    path_object ? (unsigned)path_object->path_closed : 0u,
                    (unsigned)interaction.path_draft_active,
                    (unsigned)interaction.path_draft_point_count,
                    interaction.path_draft_point_count > 0u ? interaction.path_draft_points[0].x : 0,
                    interaction.path_draft_point_count > 0u ? interaction.path_draft_points[0].y : 0);
            return 1;
        }
    }

#undef workflow_ctx
    return 0;
}
