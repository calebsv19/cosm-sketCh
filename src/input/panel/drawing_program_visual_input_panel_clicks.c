#include "drawing_program_panel_intent.h"
#include "drawing_program/drawing_program_visual_input_panel_clicks.h"

#include <string.h>

#include "drawing_program/drawing_program_icns_export.h"
#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_indexed_editor.h"
#include "drawing_program/drawing_program_native_dialogs.h"
#include "drawing_program/drawing_program_iconset_export.h"
#include "drawing_program/drawing_program_png_export.h"
#include "drawing_program/drawing_program_project_state.h"
#include "drawing_program/drawing_program_snapshot.h"
#include "drawing_program/drawing_program_texture_canvas_ops.h"
#include "drawing_program/drawing_program_texture_scene_browser.h"
#include "drawing_program/drawing_program_texture_export.h"
#include "drawing_program/drawing_program_texture_project_session.h"
#include "drawing_program/drawing_program_texture_workspace.h"
#include "drawing_program/drawing_program_visual_input_core.h"
#include "drawing_program/drawing_program_visual_input_panel_color.h"
#include "drawing_program/drawing_program_visual_input_panel_workspace_modes.h"
#include "drawing_program/drawing_program_visual_input_workspace_view.h"
#include "drawing_program/drawing_program_visual_input_right_file_tabs.h"
#include "drawing_program/drawing_program_visual_input_workspace_surface.h"
#include "drawing_program/drawing_program_ui_color_state.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_layer_roles.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_panel_ui_state.h"
#include "drawing_program/drawing_program_visual_right_panel_defs.h"
#include "drawing_program/drawing_program_visual_tool_options.h"

enum {
    VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE = VISUAL_RIGHT_PANEL_SLOT_CANVAS,
    VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE = VISUAL_RIGHT_PANEL_SLOT_LAYER,
    VISUAL_RIGHT_PANEL_SLOT_COLOR_VALUE = VISUAL_RIGHT_PANEL_SLOT_COLOR,
    VISUAL_RIGHT_PANEL_SLOT_FILE_VALUE = VISUAL_RIGHT_PANEL_SLOT_FILE,
    VISUAL_RIGHT_PANEL_SLOT_ASSET_VALUE = VISUAL_RIGHT_PANEL_SLOT_ASSET,
    VISUAL_RIGHT_PANEL_SLOT_EXPORT_VALUE = VISUAL_RIGHT_PANEL_SLOT_EXPORT,
    VISUAL_RIGHT_FILE_ACTION_NEW_PROJECT = 0,
    VISUAL_RIGHT_FILE_ACTION_OPEN_PROJECT = 1,
    VISUAL_RIGHT_FILE_ACTION_SAVE_PROJECT = 2,
    VISUAL_RIGHT_FILE_ACTION_SAVE_AS = 3,
    VISUAL_RIGHT_FILE_ACTION_LOAD_PROJECT = 4,
    VISUAL_RIGHT_FILE_ACTION_SAVE_SESSION = 5,
    VISUAL_RIGHT_FILE_ACTION_RELOAD_SESSION = 6,
    VISUAL_RIGHT_FILE_ACTION_COUNT = 7,
    VISUAL_RIGHT_FILE_ROUTE_ACTION_PICK_INPUT = 0,
    VISUAL_RIGHT_FILE_ROUTE_ACTION_PICK_OUTPUT = 1,
    VISUAL_RIGHT_FILE_ROUTE_ACTION_PICK_SCENE = 2,
    VISUAL_RIGHT_FILE_ROUTE_ACTION_OPEN_OBJECT = 3,
    VISUAL_RIGHT_FILE_ROUTE_ACTION_EXPORT_PNG = 4,
    VISUAL_RIGHT_FILE_ROUTE_ACTION_EXPORT_TEXTURES = 5,
    VISUAL_RIGHT_FILE_ROUTE_ACTION_EXPORT_ICONSET = 6,
    VISUAL_RIGHT_FILE_ROUTE_ACTION_EXPORT_ICNS = 7,
    VISUAL_RIGHT_FILE_ROUTE_ACTION_COUNT = 8
};

static void visual_panel_clear_object_target_ui(VisualPanelUiState *ui) {
    if (!ui) {
        return;
    }
    ui->object_color_target_kind = VISUAL_OBJECT_COLOR_TARGET_NONE;
    ui->object_color_target_object_id = 0u;
}



void drawing_program_visual_input_handle_right_panel_click_payload_intent(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    DrawingProgramPanelIntent intent,
    DrawingProgramSelectionState *selection,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
    int x = intent.x, y = intent.y;
    VisualPaneLayoutMetrics m;
    SDL_Rect tab_canvas;
    SDL_Rect tab_layer;
    SDL_Rect tab_color;
    SDL_Rect tab_file;
    SDL_Rect tab_asset;
    SDL_Rect tab_export;
    if (!ctx || !ui || !hooks) {
        return;
    }
    m = make_pane_layout_metrics(ctx);
    drawing_program_visual_layer_opacity_sync_document(ctx);
    tab_canvas = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_CANVAS, VISUAL_RIGHT_PANEL_SLOT_COUNT);
    tab_layer = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_LAYER, VISUAL_RIGHT_PANEL_SLOT_COUNT);
    tab_color = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_COLOR, VISUAL_RIGHT_PANEL_SLOT_COUNT);
    tab_file = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_FILE, VISUAL_RIGHT_PANEL_SLOT_COUNT);
    tab_asset = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_ASSET, VISUAL_RIGHT_PANEL_SLOT_COUNT);
    tab_export = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_EXPORT, VISUAL_RIGHT_PANEL_SLOT_COUNT);
    if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_TAB_CANVAS, 0, tab_canvas, hooks)) {
        drawing_program_visual_set_right_panel_slot(ctx, (uint8_t)VISUAL_RIGHT_PANEL_SLOT_CANVAS);
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_TAB_LAYER, 0, tab_layer, hooks)) {
        if (drawing_program_indexed_editor_is_active(ctx)) {
            return;
        }
        drawing_program_visual_set_right_panel_slot(ctx, (uint8_t)VISUAL_RIGHT_PANEL_SLOT_LAYER);
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_TAB_COLOR, 0, tab_color, hooks)) {
        drawing_program_visual_set_right_panel_slot(ctx, (uint8_t)VISUAL_RIGHT_PANEL_SLOT_COLOR);
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_TAB_FILE, 0, tab_file, hooks)) {
        drawing_program_visual_set_right_panel_slot(ctx, (uint8_t)VISUAL_RIGHT_PANEL_SLOT_FILE);
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_TAB_ASSET, 0, tab_asset, hooks)) {
        drawing_program_visual_set_right_panel_slot(ctx, (uint8_t)VISUAL_RIGHT_PANEL_SLOT_ASSET);
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_TAB_EXPORT, 0, tab_export, hooks)) {
        drawing_program_visual_set_right_panel_slot(ctx, (uint8_t)VISUAL_RIGHT_PANEL_SLOT_EXPORT);
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE) {
        if (drawing_program_indexed_editor_is_active(ctx)) {
            return;
        }
        uint32_t display_i;
        uint32_t role_i;
        SDL_Rect action;
        SDL_Rect opacity_row;
        SDL_Rect opacity_track;
        SDL_Rect role_button;
        uint32_t active_layer_id = 0u;
        uint32_t active_layer_index = 0u;
        for (display_i = 0u; display_i < ctx->document.layer_count; ++display_i) {
            uint32_t model_i = (ctx->document.layer_count - 1u) - display_i;
            SDL_Rect row = right_layer_row_rect(rect, m, display_i);
            if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_SELECT_LAYER, ctx->document.layers[model_i].layer_id, row, hooks)) {
                (void)drawing_program_runtime_orchestration_set_active_layer_id(
                    ctx, ctx->document.layers[model_i].layer_id);
                return;
            }
        }
        if (hooks->active_layer_query(ctx, &active_layer_id, &active_layer_index, 0, 0) &&
            active_layer_index < ctx->document.layer_count) {
            opacity_row = right_layer_opacity_row_rect(rect, m, ctx->document.layer_count);
            opacity_track = right_layer_opacity_track_rect(opacity_row, m);
            if ((!intent.semantic && hooks->point_in_rect(opacity_row, x, y))) {
                int relative_x = x - opacity_track.x;
                int opacity = 100;
                if (relative_x < 0) {
                    relative_x = 0;
                }
                if (relative_x > opacity_track.w) {
                    relative_x = opacity_track.w;
                }
                if (opacity_track.w > 0) {
                    opacity = ((relative_x * 100) + (opacity_track.w / 2)) / opacity_track.w;
                }
                if (opacity < 0) {
                    opacity = 0;
                }
                if (opacity > 100) {
                    opacity = 100;
                }
                drawing_program_visual_layer_opacity_set(ctx, active_layer_id, (uint8_t)opacity);
                return;
            }
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ADD);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_ADD_LAYER, 0, action, hooks)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_ADD_LAYER);
            drawing_program_visual_layer_opacity_sync_document(ctx);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_DUPLICATE);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_DUPLICATE_SELECTED, 0, action, hooks)) {
            hooks->apply_layer_duplicate_active(ctx);
            drawing_program_visual_layer_opacity_sync_document(ctx);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_RENAME);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_AUTO_ROLE_NAME, 0, action, hooks)) {
            hooks->apply_layer_rename_auto(ctx);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_DELETE);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_DELETE_SELECTED, 0, action, hooks)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_DELETE_ACTIVE_LAYER);
            drawing_program_visual_layer_opacity_sync_document(ctx);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ACTIVE_PREV);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_ACTIVE_PREV, 0, action, hooks)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_PREV);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ACTIVE_NEXT);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_ACTIVE_NEXT, 0, action, hooks)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_NEXT);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_MOVE_UP);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_MOVE_UP, 0, action, hooks)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_UP);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_MOVE_DOWN);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_MOVE_DOWN, 0, action, hooks)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_DOWN);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_TOGGLE_VISIBLE);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_ACTIVE_VISIBLE_VISIBLE_ON_VISIBLE_OFF, 0, action, hooks)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_TOGGLE_LOCK);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_ACTIVE_LOCKED_LOCK_ON_LOCK_OFF, 0, action, hooks)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_LOCK);
            return;
        }
        for (role_i = 0u; role_i < (uint32_t)DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_COUNT; ++role_i) {
            role_button = right_layer_role_button_rect(rect, m, ctx->document.layer_count, role_i);
            if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_ROLE_BUTTON_RECT, (uint64_t)role_i, role_button, hooks)) {
                drawing_program_visual_apply_layer_role_preset_active(
                    ctx, (DrawingProgramVisualLayerRolePreset)role_i);
                return;
            }
        }
    } else if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE) {
        if (drawing_program_indexed_editor_is_active(ctx)) {
            return;
        }
        SDL_Rect add_surface_button;
        SDL_Rect duplicate_surface_button;
        SDL_Rect canvas_mode_button;
        SDL_Rect canvas_guide_button;
        SDL_Rect reflect_horizontal_button;
        SDL_Rect reflect_vertical_button;
        SDL_Rect reset_layout_button;
        SDL_Rect reset_view_button;
        SDL_Rect clear_canvas_button;
        SDL_Rect clear_objects_button;
        SDL_Rect delete_selection_button;
        SDL_Rect clear_history_button;
        uint32_t surface_index = 0u;
        add_surface_button = right_canvas_add_surface_button_rect(rect, m);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_ADD_SURFACE_BUTTON, 0, add_surface_button, hooks)) {
            drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
            if (drawing_program_texture_canvas_add_blank_from_active(ctx, &surface_index).code == CORE_OK) {
                (void)drawing_program_visual_input_workspace_view_fit_surface(ctx, surface_index);
            }
            return;
        }
        duplicate_surface_button = right_canvas_duplicate_surface_button_rect(rect, m);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_DUPLICATE_SURFACE_BUTTON, 0, duplicate_surface_button, hooks)) {
            drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
            if (drawing_program_texture_canvas_duplicate_active(ctx, &surface_index).code == CORE_OK) {
                (void)drawing_program_visual_input_workspace_view_fit_surface(ctx, surface_index);
            }
            return;
        }
        canvas_mode_button = right_canvas_mode_toggle_button_rect(rect, m);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_CANVAS_MODE_BUTTON, 0, canvas_mode_button, hooks)) {
            drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
            drawing_program_texture_canvas_toggle_control_mode(ctx);
            return;
        }
        canvas_guide_button = right_canvas_guide_mode_button_rect(rect, m);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_CANVAS_GUIDE_BUTTON, 0, canvas_guide_button, hooks)) {
            drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
            drawing_program_texture_canvas_cycle_guide_mode(ctx);
            return;
        }
        reflect_horizontal_button = right_canvas_reflect_horizontal_button_rect(rect, m);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_REFLECT_HORIZONTAL_BUTTON, 0, reflect_horizontal_button, hooks)) {
            drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
            (void)drawing_program_canvas_reflection_set_crosshair_enabled(
                ctx, ctx->editor.symmetry_horizontal ? 0u : 1u, ctx->editor.symmetry_vertical);
            return;
        }
        reflect_vertical_button = right_canvas_reflect_vertical_button_rect(rect, m);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_REFLECT_VERTICAL_BUTTON, 0, reflect_vertical_button, hooks)) {
            drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
            (void)drawing_program_canvas_reflection_set_crosshair_enabled(
                ctx, ctx->editor.symmetry_horizontal, ctx->editor.symmetry_vertical ? 0u : 1u);
            return;
        }
        if (drawing_program_visual_input_handle_right_canvas_workspace_mode_payload_intent(ctx, rect, m, intent, ui, hooks)) {
            return;
        }
        reset_layout_button = right_canvas_reset_object_layout_button_rect(rect, m);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_RESET_LAYOUT_BUTTON, 0, reset_layout_button, hooks)) {
            drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
            if (drawing_program_texture_canvas_reset_object_layout(ctx).code == CORE_OK) {
                (void)drawing_program_visual_input_workspace_view_fit_all(ctx);
            }
            return;
        }
        reset_view_button = right_canvas_reset_view_button_rect(rect, m);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_RESET_VIEW_BUTTON, 0, reset_view_button, hooks)) {
            drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
            (void)drawing_program_visual_input_workspace_view_fit_all_or_reset(ctx);
            return;
        }
        clear_canvas_button = right_canvas_clear_canvas_button_rect(rect, m);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_CLEAR_CANVAS_BUTTON, 0, clear_canvas_button, hooks)) {
            drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS);
            return;
        }
        clear_objects_button = right_canvas_clear_objects_button_rect(rect, m);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_CLEAR_OBJECTS_BUTTON, 0, clear_objects_button, hooks)) {
            drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_OBJECTS);
            visual_panel_clear_object_target_ui(ui);
            return;
        }
        delete_selection_button = right_canvas_delete_selection_button_rect(rect, m);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_DELETE_SELECTION_BUTTON, 0, delete_selection_button, hooks) &&
            selection &&
            hooks->delete_active_selection_payload_or_objects &&
            hooks->delete_active_selection_payload_or_objects(ctx, selection, hooks)) {
            drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
            return;
        }
        clear_history_button = right_canvas_clear_history_button_rect(rect, m);
        if (drawing_program_panel_intent_matches(intent, DRAWING_UI_RIGHT_PANEL_RENDER_CLEAR_HISTORY_BUTTON, 0, clear_history_button, hooks)) {
            drawing_program_visual_panel_ui_disarm_right_canvas_transients(ui);
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_HISTORY);
            return;
        }
    } else if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_COLOR) {
        if (drawing_program_visual_input_handle_right_color_panel_click_payload_intent(ctx, rect, intent, ui, hooks)) {
            return;
        }
    } else if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_FILE) {
        if (drawing_program_visual_input_handle_right_file_tab_payload_intent(ctx, rect, intent, selection, ui, hooks)) {
            return;
        }
    } else if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_ASSET) {
        if (drawing_program_visual_input_handle_right_asset_tab_payload_intent(ctx, rect, intent, selection, ui, hooks)) {
            return;
        }
    } else if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_EXPORT) {
        if (drawing_program_visual_input_handle_right_export_tab_payload_intent(ctx, rect, intent, ui, hooks)) {
            return;
        }
    }
}

int drawing_program_visual_input_handle_right_panel_wheel_payload(DrawingProgramAppContext *ctx,
                                                                  SDL_Rect rect,
                                                                  int x,
                                                                  int y,
                                                                  int wheel_y,
                                                                  VisualPanelUiState *ui,
                                                                  const DrawingProgramVisualInputHandlersHooks *hooks) {
    if (!ctx || !ui || !hooks || !hooks->point_in_rect || wheel_y == 0) {
        return 0;
    }
    return drawing_program_visual_input_handle_right_file_tabs_wheel_payload(ctx, rect, x, y, wheel_y, ui, hooks);
}

void drawing_program_visual_input_handle_right_panel_click_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    DrawingProgramSelectionState *selection,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
    drawing_program_visual_input_handle_right_panel_click_payload_intent(ctx, rect, (DrawingProgramPanelIntent){.x=x, .y=y}, selection, ui, hooks);
}
