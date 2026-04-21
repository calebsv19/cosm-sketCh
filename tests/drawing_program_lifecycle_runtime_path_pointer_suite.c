#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_visual_input_handlers.h"
#include "drawing_program/drawing_program_visual_input_selection_ops.h"
#include "drawing_program/drawing_program_visual_input_support.h"
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
    VisualCanvasInteractionState interaction;
    VisualPanelUiState panel_ui;
    SDL_Event event;

    memset(&hooks, 0, sizeof(hooks));
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
    memset(&interaction, 0, sizeof(interaction));
    memset(&panel_ui, 0, sizeof(panel_ui));

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
