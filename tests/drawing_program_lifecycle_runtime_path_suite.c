#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core_theme.h"
#include "drawing_program/drawing_program_visual_input_handlers.h"
#include "drawing_program/drawing_program_visual_input_selection_ops.h"
#include "drawing_program/drawing_program_visual_input_support.h"
#include "drawing_program_lifecycle_runtime_path_pointer_suite.h"
#include "drawing_program_lifecycle_runtime_path_suite.h"
#include "drawing_program_lifecycle_test_support.h"

int drawing_program_lifecycle_run_runtime_path_suite(DrawingProgramAppContext *workflow_ctx_ptr) {
#define workflow_ctx (*workflow_ctx_ptr)
    DrawingProgramVisualInputHandlersHooks hooks;
    VisualCanvasInteractionState interaction;
    VisualPanelUiState panel_ui;
    CoreThemePresetId selected_theme = CORE_THEME_PRESET_DARK_DEFAULT;
    CoreThemePreset theme_preset;
    SDL_Event event;

    memset(&hooks, 0, sizeof(hooks));
    hooks.apply_workflow_control_if_valid = lifecycle_test_apply_workflow_control;
    hooks.cancel_canvas_draw_and_shape = lifecycle_test_cancel_canvas_draw_and_shape;
    hooks.cancel_selection_transient = lifecycle_test_cancel_selection_transient;
    hooks.cancel_all_transient_interactions = lifecycle_test_cancel_all_transient_interactions;
    hooks.delete_active_selection_payload_or_objects = delete_active_selection_payload_or_objects;
    hooks.path_draft_commit = path_draft_commit;
    hooks.path_draft_reset = path_draft_reset;
    hooks.path_draft_pop_point = path_draft_pop_point;
    memset(&interaction, 0, sizeof(interaction));
    memset(&panel_ui, 0, sizeof(panel_ui));

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

    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        const DrawingProgramObjectRecord *path_object = 0;
        uint32_t object_id = 0u;
        uint32_t units_before = 0u;
        uint32_t units_after = 0u;
        memset(&path_seed, 0, sizeof(path_seed));
        memset(&path_payload, 0, sizeof(path_payload));
        drawing_program_object_store_reset(&workflow_ctx.object_store);
        drawing_program_object_selection_reset(&workflow_ctx.object_selection);
        drawing_program_selection_reset(&workflow_ctx.selection);
        drawing_program_history_clear(&workflow_ctx.history);
        workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_SELECT;
        path_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
        path_seed.visible = 1u;
        path_seed.locked = 0u;
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
                           &workflow_ctx.object_store, &path_seed, &path_payload, &object_id),
                       "path_bezier_toggle_key_seed_add")) {
            return 1;
        }
        drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, object_id);
        if (!drawing_program_object_selection_set_path_point(&workflow_ctx.object_selection, object_id, 1u)) {
            fprintf(stderr, "lifecycle_test: expected selected path point set for bezier toggle\n");
            return 1;
        }
        lifecycle_test_reset_input_handler_counters();
        drawing_program_history_query_units(&workflow_ctx.history, &units_before, 0);
        memset(&event, 0, sizeof(event));
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_b;
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
            fprintf(stderr, "lifecycle_test: expected bezier toggle keydown to be consumed\n");
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &units_after, 0);
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (g_test_apply_workflow_calls != 0u ||
            units_after != units_before + 1u ||
            !path_object ||
            !path_object->path_points[1].bezier_enabled ||
            (path_object->path_points[1].handle_in_dx == 0 && path_object->path_points[1].handle_in_dy == 0) ||
            (path_object->path_points[1].handle_out_dx == 0 && path_object->path_points[1].handle_out_dy == 0)) {
            fprintf(stderr,
                    "lifecycle_test: expected context bezier toggle without tool switch calls=%u units=%u/%u enabled=%u in=(%d,%d) out=(%d,%d)\n",
                    (unsigned)g_test_apply_workflow_calls,
                    (unsigned)units_before,
                    (unsigned)units_after,
                    path_object ? (unsigned)path_object->path_points[1].bezier_enabled : 0u,
                    path_object ? path_object->path_points[1].handle_in_dx : 0,
                    path_object ? path_object->path_points[1].handle_in_dy : 0,
                    path_object ? path_object->path_points[1].handle_out_dx : 0,
                    path_object ? path_object->path_points[1].handle_out_dy : 0);
            return 1;
        }
        lifecycle_test_reset_input_handler_counters();
        memset(&event, 0, sizeof(event));
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_b;
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
            fprintf(stderr, "lifecycle_test: expected second bezier toggle keydown to be consumed\n");
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (g_test_apply_workflow_calls != 0u ||
            !path_object ||
            path_object->path_points[1].bezier_enabled ||
            path_object->path_points[1].handle_in_dx != 0 ||
            path_object->path_points[1].handle_in_dy != 0 ||
            path_object->path_points[1].handle_out_dx != 0 ||
            path_object->path_points[1].handle_out_dy != 0) {
            fprintf(stderr, "lifecycle_test: expected second bezier toggle to restore linear point state\n");
            return 1;
        }
    }
    {
        DrawingProgramObjectRecord path_seed;
        DrawingProgramPathPayload path_payload;
        const DrawingProgramObjectRecord *path_object = 0;
        uint32_t object_id = 0u;
        uint32_t units_before = 0u;
        uint32_t units_after = 0u;

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
        path_payload.point_count = 3u;
        path_payload.closed = 0u;
        path_payload.points[0].x = 40;
        path_payload.points[0].y = 40;
        path_payload.points[1].x = 72;
        path_payload.points[1].y = 40;
        path_payload.points[1].bezier_enabled = 1u;
        path_payload.points[1].handle_linked = 1u;
        path_payload.points[1].handle_in_dx = -10;
        path_payload.points[1].handle_in_dy = 0;
        path_payload.points[1].handle_out_dx = 10;
        path_payload.points[1].handle_out_dy = 0;
        path_payload.points[2].x = 104;
        path_payload.points[2].y = 56;
        if (!expect_ok(drawing_program_object_store_add_path(&workflow_ctx.object_store,
                                                             &path_seed,
                                                             &path_payload,
                                                             &object_id),
                       "path_handle_link_toggle_seed_add")) {
            return 1;
        }
        drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, object_id);
        if (!drawing_program_object_selection_set_path_point(&workflow_ctx.object_selection, object_id, 1u)) {
            fprintf(stderr, "lifecycle_test: expected selected path point for handle-link toggle\n");
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &units_before, 0);
        memset(&event, 0, sizeof(event));
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_l;
        event.key.keysym.mod = KMOD_SHIFT;
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
            fprintf(stderr, "lifecycle_test: expected shift+l handle-link toggle keydown to be consumed\n");
            return 1;
        }
        drawing_program_history_query_units(&workflow_ctx.history, &units_after, 0);
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (units_after != units_before + 1u ||
            !path_object ||
            path_object->path_points[1].handle_linked != 0u) {
            fprintf(stderr,
                    "lifecycle_test: expected shift+l to unlink selected bezier point units=%u/%u linked=%u\n",
                    (unsigned)units_before,
                    (unsigned)units_after,
                    path_object ? (unsigned)path_object->path_points[1].handle_linked : 99u);
            return 1;
        }
        if (!expect_ok(drawing_program_history_undo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "path_handle_link_toggle_undo")) {
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!path_object || path_object->path_points[1].handle_linked != 1u) {
            fprintf(stderr, "lifecycle_test: expected handle-link undo to restore linked state\n");
            return 1;
        }
        if (!expect_ok(drawing_program_history_redo(
                           &workflow_ctx.history, &workflow_ctx.document, &workflow_ctx.layer_rasters, &workflow_ctx.object_store),
                       "path_handle_link_toggle_redo")) {
            return 1;
        }
        path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!path_object || path_object->path_points[1].handle_linked != 0u) {
            fprintf(stderr, "lifecycle_test: expected handle-link redo to restore unlinked state\n");
            return 1;
        }
    }

    if (drawing_program_lifecycle_run_runtime_path_pointer_suite(&workflow_ctx) != 0) {
        return 1;
    }

    lifecycle_test_reset_input_handler_counters();
    drawing_program_object_selection_reset(&workflow_ctx.object_selection);
    workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_MOVE;
    memset(&event, 0, sizeof(event));
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_b;
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
        fprintf(stderr, "lifecycle_test: expected brush fallback keydown to be consumed\n");
        return 1;
    }
    if (g_test_apply_workflow_calls != 1u ||
        g_test_last_workflow_control != DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_BRUSH) {
        fprintf(stderr, "lifecycle_test: expected B fallback to route to brush workflow control\n");
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

    drawing_program_object_store_reset(&workflow_ctx.object_store);
    drawing_program_object_selection_reset(&workflow_ctx.object_selection);
    drawing_program_selection_reset(&workflow_ctx.selection);
    memset(&interaction, 0, sizeof(interaction));
    workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_PATH;
    workflow_ctx.editor.active_layer_id = workflow_ctx.document.layers[0].layer_id;
    interaction.path_draft_active = 1u;
    interaction.path_draft_point_count = 2u;
    interaction.path_draft_points[0].x = 48;
    interaction.path_draft_points[0].y = 56;
    interaction.path_draft_points[1].x = 60;
    interaction.path_draft_points[1].y = 72;
    memset(&event, 0, sizeof(event));
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_RETURN;
    event.key.keysym.mod = KMOD_SHIFT;
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
        fprintf(stderr, "lifecycle_test: expected open-path commit keydown to be consumed\n");
        return 1;
    }
    {
        const DrawingProgramObjectRecord *path_object =
            drawing_program_object_store_get_by_id(&workflow_ctx.object_store,
                                                   workflow_ctx.object_selection.active_object_id);
        if (!path_object ||
            path_object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
            path_object->path_point_count != 2u ||
            path_object->path_closed) {
            fprintf(stderr,
                    "lifecycle_test: shift+enter path commit should create open path type=%u points=%u closed=%u\n",
                    path_object ? (unsigned)path_object->type : 0u,
                    path_object ? (unsigned)path_object->path_point_count : 0u,
                    path_object ? (unsigned)path_object->path_closed : 0u);
            return 1;
        }
    }

#undef workflow_ctx
    return 0;
}
