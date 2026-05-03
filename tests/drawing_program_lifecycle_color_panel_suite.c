#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core_theme.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_project_state.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_ui_color_state.h"
#include "drawing_program/drawing_program_visual_input_handlers.h"
#include "drawing_program/drawing_program_visual_input_panel_clicks.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_layout_color.h"
#include "drawing_program/drawing_program_visual_tool_options.h"
#include "drawing_program_lifecycle_color_panel_suite.h"
#include "drawing_program_lifecycle_test_support.h"

static int lifecycle_test_point_in_rect(SDL_Rect rect, int x, int y) {
    return x >= rect.x && y >= rect.y && x < (rect.x + rect.w) && y < (rect.y + rect.h);
}

int drawing_program_lifecycle_run_color_panel_suite(DrawingProgramAppContext *workflow_ctx_ptr) {
#define workflow_ctx (*workflow_ctx_ptr)
    {
        DrawingProgramObjectRecord object_seed;
        const DrawingProgramObjectRecord *selected_object = 0;
        uint32_t object_id = 0u;
        SDL_Rect left_rect = { 0, 0, 220, 420 };
        SDL_Rect inspector_rect;
        SDL_Rect stroke_color_row_rect;
        SDL_Rect fill_color_row_rect;
        uint32_t history_count_before = 0u;
        DrawingProgramVisualInputHandlersHooks hooks;
        VisualPanelUiState panel_ui;
        SDL_Rect right_rect = { 0, 0, 220, 420 };
        VisualPaneLayoutMetrics metrics;
        SDL_Rect tab_color;
        SDL_Rect palette_rect;
        SDL_Rect recent_rect;
        SDL_Rect hue_rect;
        SDL_Rect sv_rect;
        uint8_t before_r = 0u;
        uint8_t before_g = 0u;
        uint8_t before_b = 0u;
        int8_t font_zoom_before = 0;
        uint8_t recent_slot = 0u;
        memset(&hooks, 0, sizeof(hooks));
        memset(&panel_ui, 0, sizeof(panel_ui));
        hooks.point_in_rect = lifecycle_test_point_in_rect;
        hooks.sync_panel_ui_from_app = drawing_program_visual_sync_panel_ui_from_app;
        hooks.clamp_left_slot = drawing_program_visual_clamp_left_slot;
        hooks.clamp_right_slot = drawing_program_visual_clamp_right_slot;
        metrics = make_pane_layout_metrics(&workflow_ctx);
        tab_color = right_panel_slot_tab_rect(right_rect, metrics, 2u, 4u);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      tab_color.x + 2,
                                                                      tab_color.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.ui.right_panel_slot != 2u) {
            fprintf(stderr, "lifecycle_test: expected color tab click to select right COLOR slot\n");
            return 1;
        }
        font_zoom_before = workflow_ctx.ui.font_zoom_step;
        for (recent_slot = 0u; recent_slot < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++recent_slot) {
            recent_rect = right_color_recent_swatch_rect(right_rect, metrics, recent_slot);
            drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                          right_rect,
                                                                          recent_rect.x + (recent_rect.w / 2),
                                                                          recent_rect.y + (recent_rect.h / 2),
                                                                          &workflow_ctx.selection,
                                                                          &panel_ui,
                                                                          &hooks);
            if (workflow_ctx.ui.selected_recent_color_index != recent_slot) {
                fprintf(stderr,
                        "lifecycle_test: expected paint slot click to select recent slot %u\n",
                        (unsigned)recent_slot);
                return 1;
            }
            if (workflow_ctx.ui.font_zoom_step != font_zoom_before) {
                fprintf(stderr,
                        "lifecycle_test: expected paint slot click to preserve font zoom step %d but saw %d on slot %u\n",
                        (int)font_zoom_before,
                        (int)workflow_ctx.ui.font_zoom_step,
                        (unsigned)recent_slot);
                return 1;
            }
        }
        palette_rect = right_color_palette_swatch_rect(right_rect, metrics, 5u);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      palette_rect.x + 2,
                                                                      palette_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.ui.active_color_index != 5u) {
            fprintf(stderr, "lifecycle_test: expected color palette grid click to select slot 5\n");
            return 1;
        }
        drawing_program_color_rgb_from_index(workflow_ctx.ui.active_color_index, &before_r, &before_g, &before_b);
        hue_rect = right_color_hue_slider_rect(right_rect, metrics);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      hue_rect.x + hue_rect.w - 2,
                                                                      hue_rect.y + (hue_rect.h / 2),
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        sv_rect = right_color_sv_grid_rect(right_rect, metrics);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      sv_rect.x + sv_rect.w - 2,
                                                                      sv_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.ui.active_paint_r == before_r &&
            workflow_ctx.ui.active_paint_g == before_g &&
            workflow_ctx.ui.active_paint_b == before_b) {
            fprintf(stderr, "lifecycle_test: expected hue/sv edit to change active paint rgb\n");
            return 1;
        }
        if (workflow_ctx.ui.color_palette_rgb[5u][0] != before_r ||
            workflow_ctx.ui.color_palette_rgb[5u][1] != before_g ||
            workflow_ctx.ui.color_palette_rgb[5u][2] != before_b) {
            fprintf(stderr, "lifecycle_test: expected hue/sv edit to preserve the preset swatch rgb\n");
            return 1;
        }
        memset(&object_seed, 0, sizeof(object_seed));
        object_seed.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT;
        object_seed.layer_id = workflow_ctx.editor.active_layer_id;
        object_seed.origin_x = 8;
        object_seed.origin_y = 8;
        object_seed.width = 16u;
        object_seed.height = 10u;
        object_seed.stroke_width = 1u;
        object_seed.style_mode = 2u;
        object_seed.stroke_color_value = drawing_program_color_value_from_index(1u);
        object_seed.fill_color_value = drawing_program_color_value_from_index(2u);
        if (!expect_ok(drawing_program_object_store_add(&workflow_ctx.object_store, &object_seed, &object_id),
                       "color_panel_object_target_add")) {
            return 1;
        }
        workflow_ctx.ui.left_panel_slot = 1u;
        drawing_program_selection_reset(&workflow_ctx.selection);
        drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, object_id);
        inspector_rect = left_panel_objects_inspector_rect(left_rect, metrics);
        stroke_color_row_rect = left_panel_objects_inspector_action_row_rect(inspector_rect, metrics, 0u, 6u);
        drawing_program_visual_input_handle_left_panel_click_payload(&workflow_ctx,
                                                                     left_rect,
                                                                     stroke_color_row_rect.x + 2,
                                                                     stroke_color_row_rect.y + 2,
                                                                     &workflow_ctx.selection,
                                                                     &panel_ui,
                                                                     &hooks);
        if (panel_ui.object_color_target_kind != VISUAL_OBJECT_COLOR_TARGET_STROKE ||
            panel_ui.object_color_target_object_id != object_id) {
            fprintf(stderr, "lifecycle_test: expected stroke color inspector row to arm stroke target\n");
            return 1;
        }
        drawing_program_ui_color_load_active_paint_from_swatch(&workflow_ctx, 6u);
        history_count_before = workflow_ctx.history.count;
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      hue_rect.x + hue_rect.w - 2,
                                                                      hue_rect.y + (hue_rect.h / 2),
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        selected_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!selected_object || workflow_ctx.ui.active_color_index != 6u ||
            selected_object->stroke_color_value != drawing_program_ui_color_active_paint_sample_value(&workflow_ctx) ||
            workflow_ctx.history.count <= history_count_before) {
            fprintf(stderr, "lifecycle_test: expected hue edit with armed stroke target to apply active paint sample\n");
            return 1;
        }
        fill_color_row_rect = left_panel_objects_inspector_action_row_rect(inspector_rect, metrics, 1u, 6u);
        drawing_program_visual_input_handle_left_panel_click_payload(&workflow_ctx,
                                                                     left_rect,
                                                                     fill_color_row_rect.x + 2,
                                                                     fill_color_row_rect.y + 2,
                                                                     &workflow_ctx.selection,
                                                                     &panel_ui,
                                                                     &hooks);
        if (panel_ui.object_color_target_kind != VISUAL_OBJECT_COLOR_TARGET_FILL ||
            panel_ui.object_color_target_object_id != object_id) {
            fprintf(stderr, "lifecycle_test: expected fill color inspector row to arm fill target\n");
            return 1;
        }
        history_count_before = workflow_ctx.history.count;
        palette_rect = right_color_palette_swatch_rect(right_rect, metrics, 4u);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      palette_rect.x + 2,
                                                                      palette_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        selected_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
        if (!selected_object || workflow_ctx.ui.active_color_index != 4u ||
            selected_object->fill_color_value != drawing_program_ui_color_active_paint_sample_value(&workflow_ctx) ||
            workflow_ctx.history.count <= history_count_before) {
            fprintf(stderr,
                    "lifecycle_test: expected palette slot click with armed fill target to apply selected sample\n");
            return 1;
        }
    }
    {
        static DrawingProgramAppContext save_ctx;
        static DrawingProgramAppContext load_ctx;
        uint8_t expected_hue = 0u;
        uint8_t expected_saturation = 0u;
        uint8_t expected_value = 0u;
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
        save_ctx.ui.recent_color_count = DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT;
        save_ctx.ui.selected_recent_color_index = 1u;
        save_ctx.ui.recent_color_rgb[0][0] = 12u;
        save_ctx.ui.recent_color_rgb[0][1] = 90u;
        save_ctx.ui.recent_color_rgb[0][2] = 200u;
        save_ctx.ui.recent_color_rgb[1][0] = 200u;
        save_ctx.ui.recent_color_rgb[1][1] = 40u;
        save_ctx.ui.recent_color_rgb[1][2] = 70u;
        save_ctx.ui.color_palette_rgb[6u][0] = 15u;
        save_ctx.ui.color_palette_rgb[6u][1] = 45u;
        save_ctx.ui.color_palette_rgb[6u][2] = 180u;
        drawing_program_ui_color_set_active_paint_rgb(&save_ctx, 15u, 45u, 180u, 0u);
        drawing_program_color_rgb_to_hsv(save_ctx.ui.active_paint_r,
                                         save_ctx.ui.active_paint_g,
                                         save_ctx.ui.active_paint_b,
                                         &expected_hue,
                                         &expected_saturation,
                                         &expected_value);
        drawing_program_color_load_palette_rgb(save_ctx.ui.color_palette_rgb);
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
            load_ctx.ui.left_panel_slot != 1u ||
            load_ctx.ui.right_panel_slot != 1u ||
            load_ctx.ui.active_color_index != 6u ||
            load_ctx.ui.active_paint_r != 15u ||
            load_ctx.ui.active_paint_g != 45u ||
            load_ctx.ui.active_paint_b != 180u ||
            load_ctx.ui.color_hue != expected_hue ||
            load_ctx.ui.color_saturation != expected_saturation ||
            load_ctx.ui.color_value != expected_value ||
            load_ctx.ui.tool_brush_size != 7u ||
            load_ctx.ui.tool_brush_opacity != 65u ||
            load_ctx.ui.tool_eraser_size != 5u ||
            load_ctx.ui.tool_shape_stroke_width != 4u ||
            load_ctx.ui.tool_shape_mode != 2u ||
            load_ctx.ui.tool_shape_target_mode != (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_OBJECT ||
            load_ctx.ui.tool_fill_tolerance != DRAWING_PROGRAM_UI_FILL_TOLERANCE_MAX ||
            load_ctx.ui.tool_select_mode != (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_SUBTRACT ||
            load_ctx.ui.recent_color_count != DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT ||
            load_ctx.ui.selected_recent_color_index != 1u ||
            load_ctx.ui.recent_color_rgb[0][0] != 12u ||
            load_ctx.ui.recent_color_rgb[0][1] != 90u ||
            load_ctx.ui.recent_color_rgb[0][2] != 200u ||
            load_ctx.ui.recent_color_rgb[1][0] != 200u ||
            load_ctx.ui.recent_color_rgb[1][1] != 40u ||
            load_ctx.ui.recent_color_rgb[1][2] != 70u ||
            load_ctx.ui.color_palette_rgb[6u][0] != 15u ||
            load_ctx.ui.color_palette_rgb[6u][1] != 45u ||
            load_ctx.ui.color_palette_rgb[6u][2] != 180u) {
            fprintf(stderr,
                    "lifecycle_test: persistence roundtrip mismatch tool=%u theme=%u zoom=%d left=%u right=%u color=%u paint=%u,%u,%u hsv=%u/%u/%u expected_hsv=%u/%u/%u brush=%u/%u eraser=%u shape=%u mode=%u target=%u fill_tol=%u select_mode=%u recent=%u selected_recent=%u palette6=%u,%u,%u expected_fill_tol_max=%u\n",
                    (unsigned)load_ctx.editor.active_tool,
                    (unsigned)load_ctx.ui.theme_preset_id,
                    (int)load_ctx.ui.font_zoom_step,
                    (unsigned)load_ctx.ui.left_panel_slot,
                    (unsigned)load_ctx.ui.right_panel_slot,
                    (unsigned)load_ctx.ui.active_color_index,
                    (unsigned)load_ctx.ui.active_paint_r,
                    (unsigned)load_ctx.ui.active_paint_g,
                    (unsigned)load_ctx.ui.active_paint_b,
                    (unsigned)load_ctx.ui.color_hue,
                    (unsigned)load_ctx.ui.color_saturation,
                    (unsigned)load_ctx.ui.color_value,
                    (unsigned)expected_hue,
                    (unsigned)expected_saturation,
                    (unsigned)expected_value,
                    (unsigned)load_ctx.ui.tool_brush_size,
                    (unsigned)load_ctx.ui.tool_brush_opacity,
                    (unsigned)load_ctx.ui.tool_eraser_size,
                    (unsigned)load_ctx.ui.tool_shape_stroke_width,
                    (unsigned)load_ctx.ui.tool_shape_mode,
                    (unsigned)load_ctx.ui.tool_shape_target_mode,
                    (unsigned)load_ctx.ui.tool_fill_tolerance,
                    (unsigned)load_ctx.ui.tool_select_mode,
                    (unsigned)load_ctx.ui.recent_color_count,
                    (unsigned)load_ctx.ui.selected_recent_color_index,
                    (unsigned)load_ctx.ui.color_palette_rgb[6u][0],
                    (unsigned)load_ctx.ui.color_palette_rgb[6u][1],
                    (unsigned)load_ctx.ui.color_palette_rgb[6u][2],
                    (unsigned)DRAWING_PROGRAM_UI_FILL_TOLERANCE_MAX);
            return 1;
        }
        load_ctx.session.persist_enabled = 0u;
        if (!expect_ok(drawing_program_app_shutdown(&load_ctx), "persist_roundtrip_shutdown_load")) {
            return 1;
        }
    }
    {
        static DrawingProgramAppContext project_ctx;
        static DrawingProgramAppContext relaunch_ctx;
        char runtime_root[] = "/tmp/drawing_program_session_ui_prefs";
        char project_arg0[] = "drawing_program_session_ui_prefs";
        char project_arg1[] = "--headless";
        char project_arg2[] = "--smoke-frames";
        char project_arg3[] = "1";
        char project_arg4[] = "--runtime-root";
        char *project_argv[] = { project_arg0, project_arg1, project_arg2, project_arg3, project_arg4, runtime_root, 0 };

        (void)unlink("/tmp/drawing_program_session_ui_prefs/last_session.pack");
        (void)unlink("/tmp/drawing_program_session_ui_prefs/session_prefs_v1.txt");
        (void)unlink("/tmp/drawing_program_session_ui_prefs/input/icon_project_001.pack");

        if (!expect_ok(drawing_program_app_bootstrap(&project_ctx, 6, project_argv), "session_ui_bootstrap_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&project_ctx), "session_ui_config_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&project_ctx), "session_ui_state_seed_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_subsystems_init(&project_ctx), "session_ui_subsystems_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_start(&project_ctx), "session_ui_runtime_start_save")) {
            return 1;
        }
        project_ctx.ui.theme_preset_id = (uint32_t)CORE_THEME_PRESET_DARK_DEFAULT;
        project_ctx.ui.font_preset_id = (uint32_t)CORE_FONT_PRESET_IDE;
        project_ctx.ui.font_zoom_step = -1;
        if (!expect_ok(drawing_program_project_state_save_current(&project_ctx), "session_ui_save_project")) {
            return 1;
        }
        project_ctx.ui.theme_preset_id = (uint32_t)CORE_THEME_PRESET_LIGHT_DEFAULT;
        project_ctx.ui.font_preset_id = (uint32_t)CORE_FONT_PRESET_IDE;
        project_ctx.ui.font_zoom_step = 4;
        if (!expect_ok(drawing_program_app_shutdown(&project_ctx), "session_ui_shutdown_save")) {
            return 1;
        }

        if (!expect_ok(drawing_program_app_bootstrap(&relaunch_ctx, 6, project_argv), "session_ui_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&relaunch_ctx), "session_ui_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&relaunch_ctx), "session_ui_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_subsystems_init(&relaunch_ctx), "session_ui_subsystems_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_start(&relaunch_ctx), "session_ui_runtime_start_load")) {
            return 1;
        }
        if (relaunch_ctx.ui.theme_preset_id != (uint32_t)CORE_THEME_PRESET_LIGHT_DEFAULT ||
            relaunch_ctx.ui.font_preset_id != (uint32_t)CORE_FONT_PRESET_IDE ||
            relaunch_ctx.ui.font_zoom_step != 4) {
            fprintf(stderr,
                    "lifecycle_test: expected session ui prefs to override older project snapshot theme=%u font=%u zoom=%d\n",
                    (unsigned)relaunch_ctx.ui.theme_preset_id,
                    (unsigned)relaunch_ctx.ui.font_preset_id,
                    (int)relaunch_ctx.ui.font_zoom_step);
            return 1;
        }
        relaunch_ctx.session.persist_enabled = 0u;
        if (!expect_ok(drawing_program_app_shutdown(&relaunch_ctx), "session_ui_shutdown_load")) {
            return 1;
        }
    }

#undef workflow_ctx
    return 0;
}
