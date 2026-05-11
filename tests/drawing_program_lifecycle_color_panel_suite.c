#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "core_theme.h"
#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_project_state.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_texture_scene_browser.h"
#include "drawing_program/drawing_program_texture_project.h"
#include "drawing_program/drawing_program_texture_export_intent.h"
#include "drawing_program/drawing_program_ui_color_state.h"
#include "drawing_program/drawing_program_visual_layer_actions.h"
#include "drawing_program/drawing_program_visual_input_handlers.h"
#include "drawing_program/drawing_program_visual_input_panel_clicks.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_layer_roles.h"
#include "drawing_program/drawing_program_visual_layout_color.h"
#include "drawing_program/drawing_program_visual_tool_options.h"
#include "drawing_program_lifecycle_color_panel_suite.h"
#include "drawing_program_lifecycle_test_support.h"

static int lifecycle_test_point_in_rect(SDL_Rect rect, int x, int y) {
    return x >= rect.x && y >= rect.y && x < (rect.x + rect.w) && y < (rect.y + rect.h);
}

static int lifecycle_test_active_layer_query(const DrawingProgramAppContext *ctx,
                                             uint32_t *out_layer_id,
                                             uint32_t *out_index,
                                             uint8_t *out_visible,
                                             uint8_t *out_locked) {
    CoreResult result = drawing_program_runtime_orchestration_resolve_active_layer(
        ctx, out_layer_id, out_index, out_visible, out_locked);
    return (result.code == CORE_OK) ? 1 : 0;
}

static int write_color_panel_scene_fixture(const char *path) {
    const char *json =
        "{"
        "\"schema_family\":\"codework_scene\","
        "\"schema_variant\":\"scene_authoring_v1\","
        "\"schema_version\":1,"
        "\"scene_id\":\"scene_panel_browser\","
        "\"space_mode_default\":\"3d\","
        "\"objects\":["
          "{"
            "\"object_id\":\"obj_plane_browser\","
            "\"object_type\":\"plane_primitive\","
            "\"dimensional_mode\":\"plane_locked\","
            "\"locked_plane\":\"xy\","
            "\"primitive\":{"
              "\"kind\":\"plane_primitive\","
              "\"width\":4.0,"
              "\"height\":3.0,"
              "\"lock_to_construction_plane\":true,"
              "\"lock_to_bounds\":false,"
              "\"frame\":{"
                "\"origin\":{\"x\":0.0,\"y\":0.0,\"z\":0.0},"
                "\"axis_u\":{\"x\":1.0,\"y\":0.0,\"z\":0.0},"
                "\"axis_v\":{\"x\":0.0,\"y\":1.0,\"z\":0.0},"
                "\"normal\":{\"x\":0.0,\"y\":0.0,\"z\":1.0}"
              "}"
            "}"
          "},"
          "{"
            "\"object_id\":\"obj_prism_browser\","
            "\"object_type\":\"rect_prism_primitive\","
            "\"dimensional_mode\":\"full_3d\","
            "\"primitive\":{"
              "\"kind\":\"rect_prism_primitive\","
              "\"width\":2.0,"
              "\"height\":2.0,"
              "\"depth\":5.0,"
              "\"lock_to_construction_plane\":false,"
              "\"lock_to_bounds\":true,"
              "\"frame\":{"
                "\"origin\":{\"x\":0.0,\"y\":0.0,\"z\":0.0},"
                "\"axis_u\":{\"x\":1.0,\"y\":0.0,\"z\":0.0},"
                "\"axis_v\":{\"x\":0.0,\"y\":1.0,\"z\":0.0},"
                "\"normal\":{\"x\":0.0,\"y\":0.0,\"z\":1.0}"
              "}"
            "}"
          "}"
        "],"
        "\"materials\":[],"
        "\"lights\":[],"
        "\"cameras\":[]"
        "}";
    FILE *file = fopen(path, "wb");
    if (!file) {
        return 0;
    }
    if (fwrite(json, 1u, strlen(json), file) != strlen(json)) {
        fclose(file);
        return 0;
    }
    fclose(file);
    return 1;
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
        SDL_Rect tab_layer;
        SDL_Rect tab_canvas;
        SDL_Rect palette_rect;
        SDL_Rect recent_rect;
        SDL_Rect hue_rect;
        SDL_Rect sv_rect;
        SDL_Rect canvas_add_rect;
        SDL_Rect canvas_duplicate_rect;
        SDL_Rect canvas_mode_rect;
        SDL_Rect canvas_guide_rect;
        SDL_Rect reflect_horizontal_rect;
        SDL_Rect reflect_vertical_rect;
        SDL_Rect center_pick_rect;
        SDL_Rect center_reset_rect;
        SDL_Rect canvas_delete_rect;
        SDL_Rect layer_add_rect;
        SDL_Rect layer_role_grime_rect;
        SDL_Rect layer_role_damage_rect;
        SDL_Rect file_rect = { 0, 0, 220, 720 };
        SDL_Rect tab_file;
        SDL_Rect tab_export;
        SDL_Rect browser_scenes_tab;
        SDL_Rect queue_rect;
        SDL_Rect scene_row_rect;
        SDL_Rect object_row_rect;
        SDL_Rect open_object_rect;
        SDL_Rect export_intent_rect;
        uint8_t before_r = 0u;
        uint8_t before_g = 0u;
        uint8_t before_b = 0u;
        uint32_t surface_count_before_add = 0u;
        uint32_t surface_count_before_duplicate = 0u;
        const DrawingProgramTextureSurface *active_surface = 0;
        int8_t font_zoom_before = 0;
        uint8_t recent_slot = 0u;
        const char *scene_root = "/tmp/drawing_program_color_panel_scene_root";
        const char *scene_file = "/tmp/drawing_program_color_panel_scene_root/panel_scene.json";
        memset(&hooks, 0, sizeof(hooks));
        memset(&panel_ui, 0, sizeof(panel_ui));
        hooks.point_in_rect = lifecycle_test_point_in_rect;
        hooks.sync_panel_ui_from_app = drawing_program_visual_sync_panel_ui_from_app;
        hooks.clamp_left_slot = drawing_program_visual_clamp_left_slot;
        hooks.clamp_right_slot = drawing_program_visual_clamp_right_slot;
        hooks.apply_layer_rename_auto = drawing_program_visual_apply_layer_rename_auto;
        hooks.apply_layer_duplicate_active = drawing_program_visual_apply_layer_duplicate_active;
        hooks.active_layer_query = lifecycle_test_active_layer_query;
        hooks.apply_workflow_control_if_valid = drawing_program_visual_apply_workflow_control_if_valid;
        metrics = make_pane_layout_metrics(&workflow_ctx);
        tab_canvas = right_panel_slot_tab_rect(right_rect, metrics, 0u, 6u);
        tab_layer = right_panel_slot_tab_rect(right_rect, metrics, 1u, 6u);
        tab_color = right_panel_slot_tab_rect(right_rect, metrics, 2u, 6u);
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
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      tab_canvas.x + 2,
                                                                      tab_canvas.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.ui.right_panel_slot != 0u) {
            fprintf(stderr, "lifecycle_test: expected canvas tab click to select right CANVAS slot\n");
            return 1;
        }
        surface_count_before_add = workflow_ctx.texture_project.surface_count;
        canvas_add_rect = right_canvas_add_surface_button_rect(right_rect, metrics);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      canvas_add_rect.x + 2,
                                                                      canvas_add_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        active_surface =
            drawing_program_texture_project_surface_at(&workflow_ctx.texture_project,
                                                       workflow_ctx.texture_project.active_surface_index);
        if (workflow_ctx.texture_project.surface_count != surface_count_before_add + 1u ||
            workflow_ctx.texture_project.active_surface_index != surface_count_before_add ||
            !active_surface ||
            !active_surface->user_created ||
            !active_surface->is_blank ||
            active_surface->resize_locked) {
            fprintf(stderr,
                    "lifecycle_test: expected add canvas click to select blank resizable user surface count=%u active=%u expected_count=%u expected_active=%u user=%u blank=%u lock=%u\n",
                    (unsigned)workflow_ctx.texture_project.surface_count,
                    (unsigned)workflow_ctx.texture_project.active_surface_index,
                    (unsigned)(surface_count_before_add + 1u),
                    (unsigned)surface_count_before_add,
                    (unsigned)(active_surface ? active_surface->user_created : 0u),
                    (unsigned)(active_surface ? active_surface->is_blank : 0u),
                    (unsigned)(active_surface ? active_surface->resize_locked : 0u));
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_write(&workflow_ctx.document, 2u, 2u, 123u, 0),
                       "color_panel_canvas_seed_duplicate_source")) {
            return 1;
        }
        surface_count_before_duplicate = workflow_ctx.texture_project.surface_count;
        canvas_duplicate_rect = right_canvas_duplicate_surface_button_rect(right_rect, metrics);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      canvas_duplicate_rect.x + 2,
                                                                      canvas_duplicate_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        active_surface =
            drawing_program_texture_project_surface_at(&workflow_ctx.texture_project,
                                                       workflow_ctx.texture_project.active_surface_index);
        if (workflow_ctx.texture_project.surface_count != surface_count_before_duplicate + 1u ||
            workflow_ctx.texture_project.active_surface_index != surface_count_before_duplicate ||
            !active_surface ||
            !active_surface->user_created ||
            active_surface->is_blank ||
            !active_surface->resize_locked) {
            fprintf(stderr,
                    "lifecycle_test: expected duplicate canvas click to select painted locked user surface count=%u active=%u expected_count=%u expected_active=%u user=%u blank=%u lock=%u\n",
                    (unsigned)workflow_ctx.texture_project.surface_count,
                    (unsigned)workflow_ctx.texture_project.active_surface_index,
                    (unsigned)(surface_count_before_duplicate + 1u),
                    (unsigned)surface_count_before_duplicate,
                    (unsigned)(active_surface ? active_surface->user_created : 0u),
                    (unsigned)(active_surface ? active_surface->is_blank : 0u),
                    (unsigned)(active_surface ? active_surface->resize_locked : 0u));
            return 1;
        }
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      tab_layer.x + 2,
                                                                      tab_layer.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.ui.right_panel_slot != 1u) {
            fprintf(stderr, "lifecycle_test: expected layer tab click to select right LAYER slot\n");
            return 1;
        }
        if (strcmp(workflow_ctx.document.layers[0].name, "Base") != 0) {
            fprintf(stderr,
                    "lifecycle_test: expected default first layer name Base got %s\n",
                    workflow_ctx.document.layers[0].name);
            return 1;
        }
        layer_add_rect = right_layer_action_button_rect(right_rect, metrics, workflow_ctx.document.layer_count,
                                                        VISUAL_LAYER_ACTION_ADD);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      layer_add_rect.x + 2,
                                                                      layer_add_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.document.layer_count != 2u ||
            strcmp(workflow_ctx.document.layers[1].name, "Material Detail") != 0 ||
            workflow_ctx.editor.active_layer_id != workflow_ctx.document.layers[1].layer_id) {
            fprintf(stderr,
                    "lifecycle_test: expected add-layer convention to create active Material Detail layer count=%u name=%s active=%u expected_active=%u\n",
                    (unsigned)workflow_ctx.document.layer_count,
                    workflow_ctx.document.layers[1].name,
                    (unsigned)workflow_ctx.editor.active_layer_id,
                    (unsigned)workflow_ctx.document.layers[1].layer_id);
            return 1;
        }
        layer_role_grime_rect = right_layer_role_button_rect(
            right_rect, metrics, workflow_ctx.document.layer_count, DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_GRIME);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      layer_role_grime_rect.x + 2,
                                                                      layer_role_grime_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (strcmp(workflow_ctx.document.layers[1].name, "Grime") != 0 ||
            drawing_program_visual_layer_role_detect_active(&workflow_ctx) !=
                DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_GRIME) {
            fprintf(stderr,
                    "lifecycle_test: expected grime role preset to rename active layer to Grime got %s role=%u\n",
                    workflow_ctx.document.layers[1].name,
                    (unsigned)drawing_program_visual_layer_role_detect_active(&workflow_ctx));
            return 1;
        }
        layer_add_rect = right_layer_action_button_rect(right_rect, metrics, workflow_ctx.document.layer_count,
                                                        VISUAL_LAYER_ACTION_ADD);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      layer_add_rect.x + 2,
                                                                      layer_add_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.document.layer_count != 3u ||
            strcmp(workflow_ctx.document.layers[2].name, "Material Detail") != 0) {
            fprintf(stderr,
                    "lifecycle_test: expected next suggested layer role Material Detail got count=%u name=%s\n",
                    (unsigned)workflow_ctx.document.layer_count,
                    workflow_ctx.document.layers[2].name);
            return 1;
        }
        layer_role_damage_rect = right_layer_role_button_rect(
            right_rect, metrics, workflow_ctx.document.layer_count, DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DAMAGE);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      layer_role_damage_rect.x + 2,
                                                                      layer_role_damage_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (strcmp(workflow_ctx.document.layers[2].name, "Damage") != 0 ||
            drawing_program_visual_layer_role_detect_active(&workflow_ctx) !=
                DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DAMAGE) {
            fprintf(stderr,
                    "lifecycle_test: expected damage role preset to rename active layer to Damage got %s role=%u\n",
                    workflow_ctx.document.layers[2].name,
                    (unsigned)drawing_program_visual_layer_role_detect_active(&workflow_ctx));
            return 1;
        }
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      tab_canvas.x + 2,
                                                                      tab_canvas.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        canvas_delete_rect = right_canvas_delete_canvas_button_rect(right_rect, metrics);
        workflow_ctx.runtime.frame_counter = 100u;
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      canvas_delete_rect.x + 2,
                                                                      canvas_delete_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (!panel_ui.right_canvas_delete_confirm_pending ||
            panel_ui.right_canvas_delete_confirm_surface_index != workflow_ctx.texture_project.active_surface_index) {
            fprintf(stderr, "lifecycle_test: expected first delete canvas click to arm confirmation\n");
            return 1;
        }
        {
            SDL_Rect canvas_empty_rect =
                right_canvas_surface_row_rect(right_rect, metrics, workflow_ctx.texture_project.surface_count);
            drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                          right_rect,
                                                                          canvas_empty_rect.x + 2,
                                                                          canvas_empty_rect.y + 2,
                                                                          &workflow_ctx.selection,
                                                                          &panel_ui,
                                                                          &hooks);
            if (panel_ui.right_canvas_delete_confirm_pending) {
                fprintf(stderr, "lifecycle_test: expected non-delete canvas click to disarm delete confirmation\n");
                return 1;
            }
        }
        workflow_ctx.runtime.frame_counter = 100u;
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      canvas_delete_rect.x + 2,
                                                                      canvas_delete_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (!panel_ui.right_canvas_delete_confirm_pending ||
            panel_ui.right_canvas_delete_confirm_surface_index != workflow_ctx.texture_project.active_surface_index) {
            fprintf(stderr, "lifecycle_test: expected second delete canvas click to rearm confirmation\n");
            return 1;
        }
        workflow_ctx.runtime.frame_counter = 103u;
        surface_count_before_duplicate = workflow_ctx.texture_project.surface_count;
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      canvas_delete_rect.x + 2,
                                                                      canvas_delete_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.texture_project.surface_count != surface_count_before_duplicate ||
            !panel_ui.right_canvas_delete_confirm_pending) {
            fprintf(stderr, "lifecycle_test: expected too-fast delete confirm to keep surface count unchanged\n");
            return 1;
        }
        workflow_ctx.runtime.frame_counter = 110u;
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      canvas_delete_rect.x + 2,
                                                                      canvas_delete_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.texture_project.surface_count != surface_count_before_duplicate - 1u ||
            panel_ui.right_canvas_delete_confirm_pending) {
            fprintf(stderr,
                    "lifecycle_test: expected confirmed delete canvas click to remove one surface count=%u expected=%u pending=%u\n",
                    (unsigned)workflow_ctx.texture_project.surface_count,
                    (unsigned)(surface_count_before_duplicate - 1u),
                    (unsigned)panel_ui.right_canvas_delete_confirm_pending);
            return 1;
        }
        canvas_mode_rect = right_canvas_mode_toggle_button_rect(right_rect, metrics);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      canvas_mode_rect.x + 2,
                                                                      canvas_mode_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.ui.canvas_control_mode != (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT) {
            fprintf(stderr, "lifecycle_test: expected canvas mode click to enter layout mode\n");
            return 1;
        }
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      canvas_mode_rect.x + 2,
                                                                      canvas_mode_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.ui.canvas_control_mode != (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_PAINT) {
            fprintf(stderr, "lifecycle_test: expected canvas mode click to return to paint mode\n");
            return 1;
        }
        canvas_guide_rect = right_canvas_guide_mode_button_rect(right_rect, metrics);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      canvas_guide_rect.x + 2,
                                                                      canvas_guide_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.ui.canvas_guide_mode != (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_CORNERS) {
            fprintf(stderr, "lifecycle_test: expected guide mode click to enter corners mode\n");
            return 1;
        }
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      canvas_guide_rect.x + 2,
                                                                      canvas_guide_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.ui.canvas_guide_mode !=
            (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_CORNERS_AND_EDGES) {
            fprintf(stderr, "lifecycle_test: expected second guide mode click to enter corners+edges mode\n");
            return 1;
        }
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      canvas_guide_rect.x + 2,
                                                                      canvas_guide_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.ui.canvas_guide_mode != (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_OFF) {
            fprintf(stderr, "lifecycle_test: expected third guide mode click to return to off mode\n");
            return 1;
        }
        reflect_horizontal_rect = right_canvas_reflect_horizontal_button_rect(right_rect, metrics);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      reflect_horizontal_rect.x + 2,
                                                                      reflect_horizontal_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (!workflow_ctx.editor.symmetry_horizontal) {
            fprintf(stderr, "lifecycle_test: expected reflect-h button to enable horizontal symmetry\n");
            return 1;
        }
        reflect_vertical_rect = right_canvas_reflect_vertical_button_rect(right_rect, metrics);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      reflect_vertical_rect.x + 2,
                                                                      reflect_vertical_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (!workflow_ctx.editor.symmetry_vertical) {
            fprintf(stderr, "lifecycle_test: expected reflect-v button to enable vertical symmetry\n");
            return 1;
        }
        center_pick_rect = right_canvas_center_pick_button_rect(right_rect, metrics);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      center_pick_rect.x + 2,
                                                                      center_pick_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (!panel_ui.right_canvas_reflection_center_pick_pending) {
            fprintf(stderr, "lifecycle_test: expected set-center button to arm canvas center pick\n");
            return 1;
        }
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      center_pick_rect.x + 2,
                                                                      center_pick_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (panel_ui.right_canvas_reflection_center_pick_pending) {
            fprintf(stderr, "lifecycle_test: expected second set-center click to disarm canvas center pick\n");
            return 1;
        }
        if (!drawing_program_canvas_reflection_set_active_center(&workflow_ctx, 3u, 5u)) {
            fprintf(stderr, "lifecycle_test: expected reflection center set helper to succeed\n");
            return 1;
        }
        center_reset_rect = right_canvas_center_reset_button_rect(right_rect, metrics);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      right_rect,
                                                                      center_reset_rect.x + 2,
                                                                      center_reset_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        {
            uint32_t center_x = 0u;
            uint32_t center_y = 0u;
            uint32_t expected_center_x = 0u;
            uint32_t expected_center_y = 0u;
            drawing_program_canvas_reflection_default_center_for_document(
                &workflow_ctx.document, &expected_center_x, &expected_center_y);
            if (!drawing_program_canvas_reflection_active_center(&workflow_ctx, &center_x, &center_y) ||
                center_x != expected_center_x ||
                center_y != expected_center_y) {
                fprintf(stderr,
                        "lifecycle_test: expected reset-center button to restore midpoint center=%u,%u expected=%u,%u\n",
                        (unsigned)center_x,
                        (unsigned)center_y,
                        (unsigned)expected_center_x,
                        (unsigned)expected_center_y);
                return 1;
            }
        }
        (void)mkdir(scene_root, 0775);
        (void)unlink(scene_file);
        if (!write_color_panel_scene_fixture(scene_file)) {
            fprintf(stderr, "lifecycle_test: failed to write color panel scene browser fixture\n");
            return 1;
        }
        if (!expect_ok(drawing_program_project_state_set_scene_authored_root(&workflow_ctx, scene_root),
                       "color_panel_set_scene_root")) {
            return 1;
        }
        tab_file = right_panel_slot_tab_rect(file_rect, metrics, 4u, 6u);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      file_rect,
                                                                      tab_file.x + 2,
                                                                      tab_file.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.ui.right_panel_slot != 4u) {
            fprintf(stderr, "lifecycle_test: expected asset tab click to select right ASSET slot\n");
            return 1;
        }
        browser_scenes_tab = right_asset_browser_mode_tab_rect(file_rect, metrics, 0u, 2u);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      file_rect,
                                                                      browser_scenes_tab.x + 2,
                                                                      browser_scenes_tab.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (panel_ui.right_file_browser_mode != (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_SCENES) {
            fprintf(stderr, "lifecycle_test: expected scenes browser tab click to enter scenes mode\n");
            return 1;
        }
        queue_rect = right_asset_target_queue_rect(file_rect, metrics, 4u, 2u);
        scene_row_rect = right_file_target_queue_row_rect(queue_rect, metrics, 0u, 0);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      file_rect,
                                                                      scene_row_rect.x + 2,
                                                                      scene_row_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (strcmp(workflow_ctx.session.selected_scene_path, scene_file) != 0 ||
            panel_ui.right_file_browser_mode != (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_OBJECTS) {
            fprintf(stderr,
                    "lifecycle_test: expected scene browser click to select scene and enter objects mode path=%s mode=%u\n",
                    workflow_ctx.session.selected_scene_path,
                    (unsigned)panel_ui.right_file_browser_mode);
            return 1;
        }
        object_row_rect = right_file_target_queue_row_rect(queue_rect, metrics, 0u, 0);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      file_rect,
                                                                      object_row_rect.x + 2,
                                                                      object_row_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (strcmp(workflow_ctx.session.selected_scene_object_id, "obj_plane_browser") != 0) {
            fprintf(stderr,
                    "lifecycle_test: expected object browser click to select obj_plane_browser got=%s\n",
                    workflow_ctx.session.selected_scene_object_id);
            return 1;
        }
        open_object_rect = right_file_route_action_button_rect(file_rect, metrics, 4u, 1u, 2u);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      file_rect,
                                                                      open_object_rect.x + 2,
                                                                      open_object_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.ui.right_panel_slot != 0u ||
            strcmp(workflow_ctx.texture_project.source_object_id, "obj_plane_browser") != 0 ||
            strcmp(workflow_ctx.texture_project.source_scene_id, "scene_panel_browser") != 0) {
            fprintf(stderr,
                    "lifecycle_test: expected open object click to load plane browser pack slot=%u scene=%s object=%s\n",
                    (unsigned)workflow_ctx.ui.right_panel_slot,
                    workflow_ctx.texture_project.source_scene_id,
                    workflow_ctx.texture_project.source_object_id);
            return 1;
        }
        if (!workflow_ctx.session.project_path ||
            strstr(workflow_ctx.session.project_path, "scene_panel_browser__obj_plane_browser") == 0 ||
            strncmp(workflow_ctx.session.project_path,
                    workflow_ctx.session.input_root_path,
                    strlen(workflow_ctx.session.input_root_path)) != 0) {
            fprintf(stderr,
                    "lifecycle_test: expected object browser open to target object-derived input-root project path path=%s input_root=%s\n",
                    workflow_ctx.session.project_path ? workflow_ctx.session.project_path : "(null)",
                    workflow_ctx.session.input_root_path);
            return 1;
        }
        tab_export = right_panel_slot_tab_rect(file_rect, metrics, 5u, 6u);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      file_rect,
                                                                      tab_export.x + 2,
                                                                      tab_export.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.ui.right_panel_slot != 5u) {
            fprintf(stderr, "lifecycle_test: expected export tab click to select right EXPORT slot\n");
            return 1;
        }
        export_intent_rect = right_file_route_action_button_rect(file_rect, metrics, 6u, 1u, 7u);
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      file_rect,
                                                                      export_intent_rect.x + 2,
                                                                      export_intent_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.texture_project.export_intent_kind !=
            DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_BASE_PLUS_OVERLAY) {
            fprintf(stderr,
                    "lifecycle_test: expected export intent toggle to enter base-plus-overlay got=%u\n",
                    (unsigned)workflow_ctx.texture_project.export_intent_kind);
            return 1;
        }
        drawing_program_visual_input_handle_right_panel_click_payload(&workflow_ctx,
                                                                      file_rect,
                                                                      export_intent_rect.x + 2,
                                                                      export_intent_rect.y + 2,
                                                                      &workflow_ctx.selection,
                                                                      &panel_ui,
                                                                      &hooks);
        if (workflow_ctx.texture_project.export_intent_kind !=
            DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_FLATTENED_ONLY) {
            fprintf(stderr,
                    "lifecycle_test: expected second export intent toggle to return to flattened got=%u\n",
                    (unsigned)workflow_ctx.texture_project.export_intent_kind);
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
        char persist_arg6[] = "--runtime-root";
        char persist_arg7[128];
        char persist_arg8[] = "--input-root";
        char persist_arg9[128];
        char persist_arg10[] = "--output-root";
        char persist_arg11[128];
        char *persist_argv[] = { persist_arg0,
                                 persist_arg1,
                                 persist_arg2,
                                 persist_arg3,
                                 persist_arg4,
                                 persist_arg5,
                                 persist_arg6,
                                 persist_arg7,
                                 persist_arg8,
                                 persist_arg9,
                                 persist_arg10,
                                 persist_arg11,
                                 0 };
        (void)snprintf(persist_arg7,
                       sizeof(persist_arg7),
                       "/tmp/drawing_program_persist_roundtrip_runtime_%ld",
                       (long)getpid());
        (void)snprintf(persist_arg9,
                       sizeof(persist_arg9),
                       "/tmp/drawing_program_persist_roundtrip_input_%ld",
                       (long)getpid());
        (void)snprintf(persist_arg11,
                       sizeof(persist_arg11),
                       "/tmp/drawing_program_persist_roundtrip_output_%ld",
                       (long)getpid());
        (void)unlink(persist_arg5);
        if (!expect_ok(drawing_program_app_bootstrap(&save_ctx, 12, persist_argv), "persist_roundtrip_bootstrap_save")) {
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
        save_ctx.ui.canvas_control_mode = (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT;
        save_ctx.ui.canvas_guide_mode = (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_CORNERS_AND_EDGES;
        save_ctx.editor.symmetry_horizontal = 1u;
        save_ctx.editor.symmetry_vertical = 1u;
        if (!drawing_program_canvas_reflection_set_active_center(&save_ctx, 9u, 11u)) {
            fprintf(stderr, "lifecycle_test: expected save_ctx reflection center set to succeed\n");
            return 1;
        }
        drawing_program_canvas_reflection_sync_active_surface_from_editor(&save_ctx);
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
        if (!expect_ok(drawing_program_app_bootstrap(&load_ctx, 12, persist_argv), "persist_roundtrip_bootstrap_load")) {
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
            load_ctx.ui.canvas_control_mode != (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT ||
            load_ctx.ui.canvas_guide_mode !=
                (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_CORNERS_AND_EDGES ||
            !load_ctx.editor.symmetry_horizontal ||
            !load_ctx.editor.symmetry_vertical ||
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
        {
            uint32_t reflection_center_x = 0u;
            uint32_t reflection_center_y = 0u;
            if (!drawing_program_canvas_reflection_active_center(&load_ctx, &reflection_center_x, &reflection_center_y) ||
                reflection_center_x != 9u ||
                reflection_center_y != 11u) {
                fprintf(stderr,
                        "lifecycle_test: persistence roundtrip reflection center mismatch center=%u,%u expected=9,11\n",
                        (unsigned)reflection_center_x,
                        (unsigned)reflection_center_y);
                return 1;
            }
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
        if (!expect_ok(drawing_program_project_state_set_scene_authored_root(&project_ctx,
                                                                             "/tmp/drawing_program_session_ui_prefs/scenes"),
                       "session_ui_set_scene_root")) {
            return 1;
        }
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
            relaunch_ctx.ui.font_zoom_step != 4 ||
            strcmp(relaunch_ctx.session.scene_authored_root_path, "/tmp/drawing_program_session_ui_prefs/scenes") != 0) {
            fprintf(stderr,
                    "lifecycle_test: expected session ui prefs to override older project snapshot theme=%u font=%u zoom=%d scene_root=%s\n",
                    (unsigned)relaunch_ctx.ui.theme_preset_id,
                    (unsigned)relaunch_ctx.ui.font_preset_id,
                    (int)relaunch_ctx.ui.font_zoom_step,
                    relaunch_ctx.session.scene_authored_root_path);
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
