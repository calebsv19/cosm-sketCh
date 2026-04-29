#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core_theme.h"
#include "drawing_program/drawing_program_visual_input_handlers.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_visual_input_selection_ops.h"
#include "drawing_program/drawing_program_visual_input_support.h"
#include "drawing_program/drawing_program_visual_tool_options.h"
#include "drawing_program_lifecycle_color_panel_suite.h"
#include "drawing_program_lifecycle_runtime_path_suite.h"
#include "drawing_program_lifecycle_runtime_render_suite.h"
#include "drawing_program_lifecycle_runtime_ui_suite.h"
#include "drawing_program_lifecycle_test_support.h"

int drawing_program_lifecycle_run_runtime_render_suite(DrawingProgramAppContext *ctx_ptr,
                                                       DrawingProgramAppContext *workflow_ctx_ptr,
                                                       uint32_t center_x,
                                                       uint32_t center_y,
                                                       uint8_t expected_draw_value,
                                                       uint8_t expected_eraser_value)
{
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
        hooks.set_module_type_for_pane = drawing_program_visual_set_module_type_for_pane;
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

        if (drawing_program_lifecycle_run_runtime_path_suite(&workflow_ctx) != 0) {
            return 1;
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
            path_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
            path_seed.visible = 1u;
            path_seed.locked = 0u;
            path_seed.stroke_color_value = drawing_program_color_value_from_index(7u);
            path_seed.fill_color_value = drawing_program_color_value_from_index(7u);
            path_seed.stroke_width = 1u;
            path_seed.style_mode = 0u;
            path_payload.point_count = 4u;
            path_payload.closed = 1u;
            path_payload.points[0].x = 24;
            path_payload.points[0].y = 24;
            path_payload.points[1].x = 40;
            path_payload.points[1].y = 24;
            path_payload.points[2].x = 40;
            path_payload.points[2].y = 40;
            path_payload.points[3].x = 24;
            path_payload.points[3].y = 40;
            if (!expect_ok(drawing_program_object_store_add_path(
                               &workflow_ctx.object_store, &path_seed, &path_payload, &object_id),
                           "path_delete_selected_point_seed_add")) {
                return 1;
            }
            drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, object_id);
            if (!drawing_program_object_selection_set_path_point(&workflow_ctx.object_selection, object_id, 1u)) {
                fprintf(stderr, "lifecycle_test: expected selected path point set to succeed\n");
                return 1;
            }
            drawing_program_history_query_units(&workflow_ctx.history, &units_before, 0);
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
                fprintf(stderr,
                        "lifecycle_test: expected delete keydown to be consumed for selected path point\n");
                return 1;
            }
            drawing_program_history_query_units(&workflow_ctx.history, &units_after, 0);
            if (units_after != units_before + 1u) {
                fprintf(stderr,
                        "lifecycle_test: expected selected path point delete to append one history unit before=%u after=%u\n",
                        (unsigned)units_before,
                        (unsigned)units_after);
                return 1;
            }
            path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
            if (!path_object ||
                path_object->path_point_count != 3u ||
                !path_object->path_closed ||
                path_object->path_points[1].x != 40 ||
                path_object->path_points[1].y != 40 ||
                !workflow_ctx.object_selection.selected_path_point_active ||
                workflow_ctx.object_selection.selected_path_point_object_id != object_id ||
                workflow_ctx.object_selection.selected_path_point_index != 1u) {
                fprintf(stderr,
                        "lifecycle_test: selected path point delete did not preserve object/update point selection count=%u closed=%u next=(%d,%d) selected=%u obj=%u idx=%u\n",
                        path_object ? (unsigned)path_object->path_point_count : 0u,
                        path_object ? (unsigned)path_object->path_closed : 0u,
                        path_object ? path_object->path_points[1].x : 0,
                        path_object ? path_object->path_points[1].y : 0,
                        (unsigned)workflow_ctx.object_selection.selected_path_point_active,
                        (unsigned)workflow_ctx.object_selection.selected_path_point_object_id,
                        (unsigned)workflow_ctx.object_selection.selected_path_point_index);
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

        workflow_ctx.ui.font_zoom_step = 3;
        workflow_ctx.editor.viewport.pan_x = 120.0f;
        workflow_ctx.editor.viewport.pan_y = -80.0f;
        workflow_ctx.editor.viewport.zoom = 3.0f;
        memset(&event, 0, sizeof(event));
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_0;
        event.key.keysym.mod = KMOD_CTRL | KMOD_SHIFT;
        if (!drawing_program_visual_input_handle_keydown_payload(&event,
                                                                 1,
                                                                 (SDL_Rect){ 100, 100, 600, 400 },
                                                                 &selected_theme,
                                                                 &theme_preset,
                                                                 &workflow_ctx,
                                                                 &interaction,
                                                                 &workflow_ctx.selection,
                                                                 &panel_ui,
                                                                 &hooks)) {
            fprintf(stderr, "lifecycle_test: expected ctrl+shift+0 to be consumed for fit reset\n");
            return 1;
        }
        if (workflow_ctx.ui.font_zoom_step != 3) {
            fprintf(stderr,
                    "lifecycle_test: ctrl+shift+0 should not reset font zoom step got=%d\n",
                    (int)workflow_ctx.ui.font_zoom_step);
            return 1;
        }
        if (workflow_ctx.editor.viewport.pan_x == 120.0f &&
            workflow_ctx.editor.viewport.pan_y == -80.0f &&
            workflow_ctx.editor.viewport.zoom == 3.0f) {
            fprintf(stderr, "lifecycle_test: ctrl+shift+0 should change viewport state\n");
            return 1;
        }

        {
            uint32_t module_before = drawing_program_visual_module_type_for_pane(&workflow_ctx, 6u);
            memset(&event, 0, sizeof(event));
            event.type = SDL_KEYDOWN;
            event.key.keysym.sym = SDLK_1;
            event.key.keysym.mod = KMOD_CTRL | KMOD_SHIFT;
            if (drawing_program_visual_input_handle_keydown_payload(&event,
                                                                    1,
                                                                    (SDL_Rect){ 100, 100, 600, 400 },
                                                                    &selected_theme,
                                                                    &theme_preset,
                                                                    &workflow_ctx,
                                                                    &interaction,
                                                                    &workflow_ctx.selection,
                                                                    &panel_ui,
                                                                    &hooks)) {
                fprintf(stderr, "lifecycle_test: ctrl+shift+1 should no longer trigger debug pane swap\n");
                return 1;
            }
            if (drawing_program_visual_module_type_for_pane(&workflow_ctx, 6u) != module_before) {
                fprintf(stderr, "lifecycle_test: ctrl+shift+1 should preserve pane module binding\n");
                return 1;
            }
        }
    }
    if (drawing_program_lifecycle_run_runtime_ui_suite(&ctx, center_x, center_y, expected_draw_value) != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_color_panel_suite(&workflow_ctx) != 0) {
        return 1;
    }

#undef workflow_ctx
#undef ctx
    return 0;
}
