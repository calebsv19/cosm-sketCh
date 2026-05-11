#include "drawing_program/drawing_program_visual_input_panel_clicks.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_icns_export.h"
#include "drawing_program/drawing_program_canvas_reflection.h"
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
#include "drawing_program/drawing_program_visual_input_right_file_tabs.h"
#include "drawing_program/drawing_program_ui_color_state.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_layer_roles.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_visual_right_panel_defs.h"
#include "drawing_program/drawing_program_viewport.h"

enum {
    VISUAL_LEFT_PANEL_SLOT_TOOLS_VALUE = 0,
    VISUAL_LEFT_PANEL_SLOT_OBJECTS_VALUE = 1,
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

static int visual_object_style_fill_enabled(uint8_t style_mode) {
    return (style_mode == 1u || style_mode == 2u) ? 1 : 0;
}

static uint8_t visual_object_style_set_fill_enabled(uint8_t style_mode, int enabled) {
    if (enabled) {
        if (style_mode == 0u) {
            return 2u;
        }
        if (style_mode > 2u) {
            return 2u;
        }
        return style_mode;
    }
    return 0u;
}

#if 0
static void visual_right_panel_set_file_status(DrawingProgramAppContext *ctx, const char *format, ...) {
    va_list args;
    if (!ctx || !format) {
        return;
    }
    va_start(args, format);
    (void)vsnprintf(ctx->session.file_action_status_message,
                    sizeof(ctx->session.file_action_status_message),
                    format,
                    args);
    va_end(args);
}
#endif

static void visual_panel_clear_object_target_ui(VisualPanelUiState *ui) {
    if (!ui) {
        return;
    }
    ui->object_color_target_kind = VISUAL_OBJECT_COLOR_TARGET_NONE;
    ui->object_color_target_object_id = 0u;
}

#if 0
static void visual_right_panel_default_project_filename(const DrawingProgramAppContext *ctx,
                                                        char *out_name,
                                                        size_t out_cap) {
    const char *project_path = 0;
    const char *base_name = 0;
    if (!out_name || out_cap == 0u) {
        return;
    }
    project_path = (ctx && ctx->session.project_path) ? ctx->session.project_path : 0;
    base_name = project_path ? strrchr(project_path, '/') : 0;
    if (base_name && base_name[0] == '/') {
        ++base_name;
    } else if (!base_name) {
        base_name = project_path;
    }
    if (!base_name || base_name[0] == '\0') {
        base_name = "icon_project_001.pack";
    }
    (void)snprintf(out_name, out_cap, "%s", base_name);
}
#endif

#define VISUAL_RIGHT_CANVAS_DELETE_CONFIRM_MIN_FRAMES 8u
#define VISUAL_RIGHT_CANVAS_DELETE_CONFIRM_MAX_FRAMES 180u

static void visual_right_panel_disarm_canvas_delete(VisualPanelUiState *ui) {
    if (!ui) {
        return;
    }
    ui->right_canvas_delete_confirm_pending = 0u;
    ui->right_canvas_delete_confirm_surface_index = 0u;
    ui->right_canvas_delete_confirm_armed_frame = 0u;
}

static void visual_right_panel_disarm_canvas_reflection_pick(VisualPanelUiState *ui) {
    if (!ui) {
        return;
    }
    ui->right_canvas_reflection_center_pick_pending = 0u;
}

static void visual_right_panel_arm_canvas_reflection_pick(VisualPanelUiState *ui) {
    if (!ui) {
        return;
    }
    ui->right_canvas_reflection_center_pick_pending = 1u;
}

static void visual_right_panel_arm_canvas_delete(DrawingProgramAppContext *ctx, VisualPanelUiState *ui) {
    if (!ctx || !ui) {
        return;
    }
    ui->right_canvas_delete_confirm_pending = 1u;
    ui->right_canvas_delete_confirm_surface_index = ctx->texture_project.active_surface_index;
    ui->right_canvas_delete_confirm_armed_frame = ctx->runtime.frame_counter;
}

static int visual_right_panel_canvas_delete_is_confirmable(const DrawingProgramAppContext *ctx,
                                                           const VisualPanelUiState *ui) {
    uint64_t delta = 0u;
    if (!ctx || !ui || !ui->right_canvas_delete_confirm_pending ||
        ui->right_canvas_delete_confirm_surface_index != ctx->texture_project.active_surface_index ||
        ctx->runtime.frame_counter < ui->right_canvas_delete_confirm_armed_frame) {
        return 0;
    }
    delta = ctx->runtime.frame_counter - ui->right_canvas_delete_confirm_armed_frame;
    return (delta >= VISUAL_RIGHT_CANVAS_DELETE_CONFIRM_MIN_FRAMES &&
            delta <= VISUAL_RIGHT_CANVAS_DELETE_CONFIRM_MAX_FRAMES)
               ? 1
               : 0;
}

#if 0
static int visual_right_panel_save_session(DrawingProgramAppContext *ctx) {
    CoreResult result;
    if (!ctx || !ctx->session.preset_path) {
        return 0;
    }
    result = drawing_program_snapshot_save(ctx, ctx->session.preset_path);
    if (result.code == CORE_OK) {
        visual_right_panel_set_file_status(ctx, "SESSION SAVED");
        return 1;
    }
    visual_right_panel_set_file_status(ctx, "SESSION SAVE FAILED");
    return 0;
}

static int visual_right_panel_after_snapshot_load(DrawingProgramAppContext *ctx,
                                                  DrawingProgramSelectionState *selection,
                                                  VisualPanelUiState *ui,
                                                  const DrawingProgramVisualInputHandlersHooks *hooks,
                                                  int preserve_project_clean_state) {
    if (!ctx || !ui || !hooks || !hooks->sync_panel_ui_from_app) {
        return 0;
    }
    drawing_program_selection_reset(selection ? selection : &ctx->selection);
    drawing_program_object_selection_reset(&ctx->object_selection);
    visual_panel_clear_object_target_ui(ui);
    if (!preserve_project_clean_state) {
        ctx->session.project_has_saved_state = 0u;
    }
    drawing_program_visual_layer_opacity_sync_document(ctx);
    hooks->sync_panel_ui_from_app(ctx, ui);
    return 1;
}

static int visual_right_panel_reload_session(DrawingProgramAppContext *ctx,
                                             DrawingProgramSelectionState *selection,
                                             VisualPanelUiState *ui,
                                             const DrawingProgramVisualInputHandlersHooks *hooks) {
    CoreResult result;
    if (!ctx || !ctx->session.preset_path || !ui || !hooks || !hooks->sync_panel_ui_from_app) {
        return 0;
    }
    result = drawing_program_snapshot_load(ctx, ctx->session.preset_path);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "SESSION RELOAD FAILED");
        return 0;
    }
    visual_right_panel_set_file_status(ctx, "SESSION RELOADED");
    return visual_right_panel_after_snapshot_load(ctx, selection, ui, hooks, 0);
}

static int visual_right_panel_new_project(DrawingProgramAppContext *ctx,
                                          VisualPanelUiState *ui,
                                          const DrawingProgramVisualInputHandlersHooks *hooks) {
    CoreResult result;
    if (!ctx || !ui || !hooks || !hooks->sync_panel_ui_from_app) {
        return 0;
    }
    result = drawing_program_project_state_begin_new_blank(ctx);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "NEW PROJECT FAILED");
        return 0;
    }
    visual_right_panel_set_file_status(ctx, "NEW PROJECT READY");
    return visual_right_panel_after_snapshot_load(ctx, 0, ui, hooks, 0);
}

static int visual_right_panel_open_project(DrawingProgramAppContext *ctx,
                                           DrawingProgramSelectionState *selection,
                                           VisualPanelUiState *ui,
                                           const DrawingProgramVisualInputHandlersHooks *hooks) {
    char chosen_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx || !ui || !hooks || !hooks->sync_panel_ui_from_app) {
        return 0;
    }
    result = drawing_program_native_choose_file("Open project pack",
                                                ctx->session.input_root_path,
                                                chosen_path,
                                                sizeof(chosen_path));
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "OPEN PROJECT CANCELLED");
        return 0;
    }
    result = drawing_program_project_state_set_current_path(ctx, chosen_path);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "OPEN PROJECT TARGET FAILED");
        return 0;
    }
    result = drawing_program_project_state_load_current(ctx);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "OPEN PROJECT FAILED");
        return 0;
    }
    visual_right_panel_set_file_status(ctx, "PROJECT OPENED");
    return visual_right_panel_after_snapshot_load(ctx, selection, ui, hooks, 1);
}

static int visual_right_panel_save_project(DrawingProgramAppContext *ctx) {
    CoreResult result;
    if (!ctx) {
        return 0;
    }
    result = drawing_program_project_state_save_current(ctx);
    if (result.code == CORE_OK) {
        visual_right_panel_set_file_status(ctx, "PROJECT SAVED");
        return 1;
    }
    visual_right_panel_set_file_status(ctx, "PROJECT SAVE FAILED");
    return 0;
}

static int visual_right_panel_save_as_project(DrawingProgramAppContext *ctx,
                                              VisualPanelUiState *ui,
                                              const DrawingProgramVisualInputHandlersHooks *hooks) {
    char default_name[128];
    char chosen_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx || !ui || !hooks || !hooks->sync_panel_ui_from_app) {
        return 0;
    }
    visual_right_panel_default_project_filename(ctx, default_name, sizeof(default_name));
    result = drawing_program_native_choose_save_file("Save project as",
                                                     ctx->session.input_root_path,
                                                     default_name,
                                                     chosen_path,
                                                     sizeof(chosen_path));
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "SAVE AS CANCELLED");
        return 0;
    }
    result = drawing_program_project_state_set_save_as_path(ctx, chosen_path);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "SAVE AS TARGET FAILED");
        return 0;
    }
    result = drawing_program_project_state_save_current(ctx);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "SAVE AS FAILED");
        return 0;
    }
    visual_right_panel_set_file_status(ctx, "PROJECT SAVED AS");
    hooks->sync_panel_ui_from_app(ctx, ui);
    return 1;
}

static int visual_right_panel_load_project(DrawingProgramAppContext *ctx,
                                           DrawingProgramSelectionState *selection,
                                           VisualPanelUiState *ui,
                                           const DrawingProgramVisualInputHandlersHooks *hooks) {
    CoreResult result;
    uint8_t exists = 0u;
    if (!ctx || !ui || !hooks || !hooks->sync_panel_ui_from_app) {
        return 0;
    }
    result = drawing_program_project_state_current_exists(ctx, &exists);
    if (result.code != CORE_OK || !exists) {
        visual_right_panel_set_file_status(ctx, "PROJECT TARGET EMPTY");
        return 0;
    }
    result = drawing_program_project_state_load_current(ctx);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "PROJECT LOAD FAILED");
        return 0;
    }
    visual_right_panel_set_file_status(ctx, "PROJECT LOADED");
    return visual_right_panel_after_snapshot_load(ctx, selection, ui, hooks, 1);
}

static int visual_right_panel_select_project_slot(DrawingProgramAppContext *ctx,
                                                  uint32_t slot_index,
                                                  VisualPanelUiState *ui,
                                                  const DrawingProgramVisualInputHandlersHooks *hooks) {
    CoreResult result;
    char slot_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    uint8_t existing = 0u;
    if (!ctx || !ui || !hooks || !hooks->sync_panel_ui_from_app) {
        return 0;
    }
    result = drawing_program_project_state_slot_path(ctx, slot_index, slot_path, sizeof(slot_path), &existing);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "PROJECT TARGET SELECT FAILED");
        return 0;
    }
    result = drawing_program_project_state_select_slot(ctx, slot_index);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "PROJECT TARGET SELECT FAILED");
        return 0;
    }
    visual_right_panel_set_file_status(ctx, existing ? "TARGET READY TO LOAD" : "TARGET READY TO SAVE");
    hooks->sync_panel_ui_from_app(ctx, ui);
    return 1;
}

static int visual_right_panel_file_queue_hit_slot(DrawingProgramAppContext *ctx,
                                                  SDL_Rect rect,
                                                  int x,
                                                  int y,
                                                  VisualPanelUiState *ui,
                                                  const DrawingProgramVisualInputHandlersHooks *hooks,
                                                  uint32_t *out_slot_index) {
    VisualPaneLayoutMetrics m;
    SDL_Rect queue_rect;
    uint32_t slot_count;
    int scroll_y;
    uint32_t i;
    if (!ctx || !ui || !hooks || !hooks->point_in_rect || !out_slot_index) {
        return 0;
    }
    m = make_pane_layout_metrics(ctx);
    queue_rect = right_file_target_queue_rect(rect, m, 12u, VISUAL_RIGHT_FILE_ROUTE_ACTION_COUNT);
    if (!hooks->point_in_rect(queue_rect, x, y)) {
        return 0;
    }
    if (ui->right_file_browser_mode == (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_PROJECTS) {
        slot_count = right_file_target_queue_slot_count(ctx);
    } else if (ui->right_file_browser_mode == (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_OBJECTS &&
               ctx->session.selected_scene_path[0] != '\0') {
        DrawingProgramTextureSceneObjectEntry object_entries[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY];
        uint32_t object_count = 0u;
        (void)drawing_program_texture_scene_browser_list_supported_objects(
            ctx->session.selected_scene_path,
            object_entries,
            DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY,
            &object_count,
            0,
            0u);
        slot_count = object_count > 0u ? object_count : 1u;
    } else {
        DrawingProgramTextureSceneFileEntry scene_entries[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY];
        uint32_t scene_count = 0u;
        (void)drawing_program_texture_scene_browser_list_scene_files(ctx->session.scene_authored_root_path,
                                                                     scene_entries,
                                                                     DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY,
                                                                     &scene_count);
        slot_count = scene_count > 0u ? scene_count : 1u;
    }
    scroll_y = right_file_target_queue_clamp_scroll(queue_rect,
                                                    m,
                                                    slot_count,
                                                    ui->right_file_target_queue_scroll_y);
    ui->right_file_target_queue_scroll_y = scroll_y;
    for (i = 0u; i < slot_count; ++i) {
        SDL_Rect row = right_file_target_queue_row_rect(queue_rect, m, i, scroll_y);
        if (hooks->point_in_rect(row, x, y)) {
            *out_slot_index = i;
            return 1;
        }
    }
    return 0;
}

static int visual_right_panel_pick_input_root(DrawingProgramAppContext *ctx,
                                              VisualPanelUiState *ui,
                                              const DrawingProgramVisualInputHandlersHooks *hooks) {
    char chosen_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx || !ui || !hooks || !hooks->sync_panel_ui_from_app) {
        return 0;
    }
    result = drawing_program_native_choose_directory("Select project input root",
                                                     ctx->session.input_root_path,
                                                     chosen_path,
                                                     sizeof(chosen_path));
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "INPUT ROOT PICKER CANCELLED");
        return 0;
    }
    result = drawing_program_project_state_set_input_root(ctx, chosen_path);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "INPUT ROOT UPDATE FAILED");
        return 0;
    }
    visual_right_panel_set_file_status(ctx, "INPUT ROOT UPDATED");
    hooks->sync_panel_ui_from_app(ctx, ui);
    return 1;
}

static int visual_right_panel_pick_output_root(DrawingProgramAppContext *ctx) {
    char chosen_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx) {
        return 0;
    }
    result = drawing_program_native_choose_directory("Select export output root",
                                                     ctx->session.output_root_path,
                                                     chosen_path,
                                                     sizeof(chosen_path));
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "OUTPUT ROOT PICKER CANCELLED");
        return 0;
    }
    result = drawing_program_project_state_set_output_root(ctx, chosen_path);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "OUTPUT ROOT UPDATE FAILED");
        return 0;
    }
    visual_right_panel_set_file_status(ctx, "OUTPUT ROOT UPDATED");
    return 1;
}

static int visual_right_panel_pick_scene_root(DrawingProgramAppContext *ctx,
                                              VisualPanelUiState *ui,
                                              const DrawingProgramVisualInputHandlersHooks *hooks) {
    char chosen_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx || !ui || !hooks || !hooks->sync_panel_ui_from_app) {
        return 0;
    }
    result = drawing_program_native_choose_directory("Select authored scene root",
                                                     ctx->session.scene_authored_root_path,
                                                     chosen_path,
                                                     sizeof(chosen_path));
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "SCENE ROOT PICKER CANCELLED");
        return 0;
    }
    result = drawing_program_project_state_set_scene_authored_root(ctx, chosen_path);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "SCENE ROOT UPDATE FAILED");
        return 0;
    }
    ctx->session.selected_scene_path[0] = '\0';
    ctx->session.selected_scene_object_id[0] = '\0';
    ui->right_file_browser_mode = (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_SCENES;
    ui->right_file_target_queue_scroll_y = 0;
    visual_right_panel_set_file_status(ctx, "SCENE ROOT UPDATED");
    hooks->sync_panel_ui_from_app(ctx, ui);
    return 1;
}

static int visual_right_panel_open_selected_scene_object(DrawingProgramAppContext *ctx,
                                                         VisualPanelUiState *ui,
                                                         const DrawingProgramVisualInputHandlersHooks *hooks) {
    CoreResult result;
    if (!ctx || !ui || !hooks || !hooks->sync_panel_ui_from_app ||
        ctx->session.selected_scene_path[0] == '\0' ||
        ctx->session.selected_scene_object_id[0] == '\0') {
        return 0;
    }
    result = drawing_program_texture_project_session_import_scene_object(
        ctx,
        ctx->session.selected_scene_path,
        ctx->session.selected_scene_object_id,
        ctx->session.texture_scene_import_quality_preset);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "OBJECT PACK OPEN FAILED");
        return 0;
    }
    ctx->ui.right_panel_slot = (uint8_t)VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE;
    visual_right_panel_set_file_status(ctx, "OBJECT PACK OPENED");
    {
        SDL_Rect canvas_rect = { 0, 0, 0, 0 };
        if (drawing_program_visual_pane_rect_for_module_type(ctx, 1u, &canvas_rect)) {
            (void)drawing_program_texture_workspace_fit_all(ctx, canvas_rect);
        }
    }
    hooks->sync_panel_ui_from_app(ctx, ui);
    return 1;
}

static int visual_right_panel_export_png(DrawingProgramAppContext *ctx) {
    char export_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx) {
        return 0;
    }
    result = drawing_program_png_export_default_path(ctx, export_path, sizeof(export_path));
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "EXPORT PATH FAILED");
        return 0;
    }
    result = drawing_program_png_export_current_document(ctx, export_path);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "PNG EXPORT FAILED");
        return 0;
    }
    visual_right_panel_set_file_status(ctx, "PNG EXPORTED");
    return 1;
}

static int visual_right_panel_export_iconset(DrawingProgramAppContext *ctx) {
    char iconset_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx) {
        return 0;
    }
    result = drawing_program_iconset_export_default_path(ctx, iconset_path, sizeof(iconset_path));
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "ICONSET PATH FAILED");
        return 0;
    }
    result = drawing_program_iconset_export_current_document(ctx, iconset_path);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "ICONSET EXPORT FAILED");
        return 0;
    }
    visual_right_panel_set_file_status(ctx, "ICONSET EXPORTED");
    return 1;
}

static int visual_right_panel_export_textures(DrawingProgramAppContext *ctx) {
    char export_dir[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx) {
        return 0;
    }
    result = drawing_program_texture_export_default_directory(ctx, export_dir, sizeof(export_dir));
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "TEXTURE PATH FAILED");
        return 0;
    }
    result = drawing_program_texture_export_current_project(ctx, export_dir);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "TEXTURE EXPORT FAILED");
        return 0;
    }
    visual_right_panel_set_file_status(ctx, "TEXTURE SET EXPORTED");
    return 1;
}

static int visual_right_panel_export_icns(DrawingProgramAppContext *ctx) {
    char icns_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx) {
        return 0;
    }
    result = drawing_program_icns_export_default_path(ctx, icns_path, sizeof(icns_path));
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "ICNS PATH FAILED");
        return 0;
    }
    result = drawing_program_icns_export_current_document(ctx, icns_path);
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "ICNS EXPORT FAILED");
        return 0;
    }
    visual_right_panel_set_file_status(ctx, "ICNS EXPORTED");
    return 1;
}
#endif

void drawing_program_visual_input_handle_left_panel_click_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    DrawingProgramSelectionState *selection,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
    VisualPaneLayoutMetrics m;
    uint32_t tool_count;
    uint32_t option_count;
    uint32_t i;
    if (!ctx || !ui || !hooks) {
        return;
    }
    m = make_pane_layout_metrics(ctx);
    {
        SDL_Rect tab_tools = left_panel_slot_tab_rect(rect, m, VISUAL_LEFT_PANEL_SLOT_TOOLS_VALUE, 2u);
        SDL_Rect tab_objects = left_panel_slot_tab_rect(rect, m, VISUAL_LEFT_PANEL_SLOT_OBJECTS_VALUE, 2u);
        if (hooks->point_in_rect(tab_tools, x, y)) {
            ctx->ui.left_panel_slot = (uint8_t)VISUAL_LEFT_PANEL_SLOT_TOOLS_VALUE;
            hooks->sync_panel_ui_from_app(ctx, ui);
            return;
        }
        if (hooks->point_in_rect(tab_objects, x, y)) {
            ctx->ui.left_panel_slot = (uint8_t)VISUAL_LEFT_PANEL_SLOT_OBJECTS_VALUE;
            hooks->sync_panel_ui_from_app(ctx, ui);
            return;
        }
    }
    if (hooks->clamp_left_slot(ctx->ui.left_panel_slot) == (uint8_t)VISUAL_LEFT_PANEL_SLOT_OBJECTS_VALUE) {
        SDL_Rect list_rect = left_panel_objects_list_rect(rect, m);
        SDL_Rect inspector_rect = left_panel_objects_inspector_rect(rect, m);
        const DrawingProgramObjectRecord *selected_object = 0;
        uint32_t display_i;
        for (display_i = 0u; display_i < ctx->object_store.object_count; ++display_i) {
            uint32_t model_i = (ctx->object_store.object_count - 1u) - display_i;
            SDL_Rect row = left_panel_objects_row_rect(list_rect, m, display_i);
            if (row.y + row.h > list_rect.y + list_rect.h) {
                break;
            }
            if (hooks->point_in_rect(inspector_rect, x, y)) {
                break;
            }
            if (hooks->point_in_rect(row, x, y)) {
                drawing_program_selection_reset(selection);
                drawing_program_object_selection_replace_single(
                    &ctx->object_selection, ctx->object_store.objects[model_i].object_id);
                ui->object_color_target_kind = VISUAL_OBJECT_COLOR_TARGET_NONE;
                ui->object_color_target_object_id = 0u;
                return;
            }
        }
        selected_object =
            drawing_program_object_store_get_by_id(&ctx->object_store, ctx->object_selection.active_object_id);
        if (selected_object) {
            int is_shape_object =
                (selected_object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT ||
                 selected_object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_ELLIPSE);
            uint32_t action_count =
                (selected_object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) ? 5u :
                (is_shape_object ? 6u : 4u);
            SDL_Rect stroke_color_row =
                left_panel_objects_inspector_action_row_rect(inspector_rect, m, 0u, action_count);
            SDL_Rect fill_color_row =
                left_panel_objects_inspector_action_row_rect(inspector_rect, m, 1u, action_count);
            SDL_Rect stroke_row =
                left_panel_objects_inspector_action_row_rect(inspector_rect, m, 2u, action_count);
            SDL_Rect minus_rect = left_tool_option_minus_rect(stroke_row, m);
            SDL_Rect plus_rect = left_tool_option_plus_rect(stroke_row, m);
            SDL_Rect fill_row = left_panel_objects_inspector_action_row_rect(
                inspector_rect, m, is_shape_object ? 5u : 3u, action_count);
            if (hooks->point_in_rect(stroke_color_row, x, y)) {
                ui->object_color_target_kind =
                    (ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_STROKE &&
                     ui->object_color_target_object_id == selected_object->object_id)
                        ? VISUAL_OBJECT_COLOR_TARGET_NONE
                        : VISUAL_OBJECT_COLOR_TARGET_STROKE;
                ui->object_color_target_object_id =
                    (ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_NONE) ? 0u : selected_object->object_id;
                return;
            }
            if (hooks->point_in_rect(fill_color_row, x, y)) {
                ui->object_color_target_kind =
                    (ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_FILL &&
                     ui->object_color_target_object_id == selected_object->object_id)
                        ? VISUAL_OBJECT_COLOR_TARGET_NONE
                        : VISUAL_OBJECT_COLOR_TARGET_FILL;
                ui->object_color_target_object_id =
                    (ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_NONE) ? 0u : selected_object->object_id;
                return;
            }
            if (hooks->point_in_rect(minus_rect, x, y)) {
                uint8_t stroke_width = selected_object->stroke_width;
                if (stroke_width > 1u) {
                    stroke_width -= 1u;
                } else {
                    stroke_width = 1u;
                }
                (void)drawing_program_history_apply_set_object_stroke_width(
                    &ctx->history, &ctx->object_store, selected_object->object_id, stroke_width);
                return;
            }
            if (hooks->point_in_rect(plus_rect, x, y)) {
                uint8_t stroke_width = selected_object->stroke_width;
                if (stroke_width < 16u) {
                    stroke_width += 1u;
                } else {
                    stroke_width = 16u;
                }
                (void)drawing_program_history_apply_set_object_stroke_width(
                    &ctx->history, &ctx->object_store, selected_object->object_id, stroke_width);
                return;
            }
            if (is_shape_object) {
                SDL_Rect width_row =
                    left_panel_objects_inspector_action_row_rect(inspector_rect, m, 3u, action_count);
                SDL_Rect width_minus_rect = left_tool_option_minus_rect(width_row, m);
                SDL_Rect width_plus_rect = left_tool_option_plus_rect(width_row, m);
                SDL_Rect height_row =
                    left_panel_objects_inspector_action_row_rect(inspector_rect, m, 4u, action_count);
                SDL_Rect height_minus_rect = left_tool_option_minus_rect(height_row, m);
                SDL_Rect height_plus_rect = left_tool_option_plus_rect(height_row, m);
                if (hooks->point_in_rect(width_minus_rect, x, y) ||
                    hooks->point_in_rect(width_plus_rect, x, y)) {
                    uint32_t next_width = selected_object->width;
                    if (hooks->point_in_rect(width_minus_rect, x, y)) {
                        next_width = (next_width > 1u) ? (next_width - 1u) : 1u;
                    } else {
                        next_width += 1u;
                    }
                    (void)drawing_program_history_apply_set_object_size(&ctx->history,
                                                                        &ctx->object_store,
                                                                        selected_object->object_id,
                                                                        next_width,
                                                                        selected_object->height);
                    return;
                }
                if (hooks->point_in_rect(height_minus_rect, x, y) ||
                    hooks->point_in_rect(height_plus_rect, x, y)) {
                    uint32_t next_height = selected_object->height;
                    if (hooks->point_in_rect(height_minus_rect, x, y)) {
                        next_height = (next_height > 1u) ? (next_height - 1u) : 1u;
                    } else {
                        next_height += 1u;
                    }
                    (void)drawing_program_history_apply_set_object_size(&ctx->history,
                                                                        &ctx->object_store,
                                                                        selected_object->object_id,
                                                                        selected_object->width,
                                                                        next_height);
                    return;
                }
            }
            if (hooks->point_in_rect(fill_row, x, y)) {
                uint8_t next_style_mode =
                    visual_object_style_set_fill_enabled(selected_object->style_mode,
                                                         !visual_object_style_fill_enabled(selected_object->style_mode));
                (void)drawing_program_history_apply_set_object_style_mode(
                    &ctx->history, &ctx->object_store, selected_object->object_id, next_style_mode);
                return;
            }
            if (selected_object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
                SDL_Rect path_row =
                    left_panel_objects_inspector_action_row_rect(inspector_rect, m, 4u, action_count);
                if (hooks->point_in_rect(path_row, x, y)) {
                    if (selected_object->path_closed || selected_object->path_point_count >= 3u) {
                        (void)drawing_program_history_apply_set_object_path_closed(
                            &ctx->history,
                            &ctx->object_store,
                            selected_object->object_id,
                            selected_object->path_closed ? 0u : 1u);
                    }
                    return;
                }
            }
        }
        return;
    }
    tool_count = hooks->visual_tool_count();
    option_count = hooks->visual_tool_option_count(ctx, ctx->editor.active_tool);

    for (i = 0u; i < tool_count; ++i) {
        SDL_Rect row = left_panel_tool_row_rect(rect, m, i, tool_count);
        DrawingProgramToolKind tool = hooks->visual_tool_at(i);
        if (hooks->point_in_rect(row, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, hooks->workflow_control_for_tool(tool));
            return;
        }
    }

    {
        SDL_Rect detail_rect = left_panel_tool_detail_rect(rect, m, tool_count);
        uint32_t option_i;
        for (option_i = 0u; option_i < option_count; ++option_i) {
            uint32_t option_kind_raw =
                hooks->visual_tool_option_kind_for_index_raw(ctx, ctx->editor.active_tool, option_i);
            SDL_Rect option_row = left_panel_tool_detail_option_row_rect(detail_rect, m, option_i);
            if (hooks->visual_tool_option_is_action_button_raw(option_kind_raw)) {
                if (hooks->point_in_rect(option_row, x, y) &&
                    hooks->visual_tool_option_is_select_delete_raw(option_kind_raw) &&
                    selection &&
                    selection->has_payload &&
                    hooks->active_layer_allows_edits_visual(ctx)) {
                    (void)drawing_program_selection_delete_payload(&ctx->document,
                                                                   &ctx->layer_rasters,
                                                                   ctx->editor.active_layer_id,
                                                                   &ctx->history,
                                                                   selection);
                    return;
                }
            } else {
                SDL_Rect minus_rect = left_tool_option_minus_rect(option_row, m);
                SDL_Rect plus_rect = left_tool_option_plus_rect(option_row, m);
                if (hooks->point_in_rect(minus_rect, x, y)) {
                    hooks->visual_tool_option_adjust_raw(ctx, option_kind_raw, -1);
                    return;
                }
                if (hooks->point_in_rect(plus_rect, x, y)) {
                    hooks->visual_tool_option_adjust_raw(ctx, option_kind_raw, 1);
                    return;
                }
            }
        }
    }
}

void drawing_program_visual_input_handle_right_panel_click_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    DrawingProgramSelectionState *selection,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
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
    if (hooks->point_in_rect(tab_canvas, x, y)) {
        ctx->ui.right_panel_slot = (uint8_t)VISUAL_RIGHT_PANEL_SLOT_CANVAS;
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (hooks->point_in_rect(tab_layer, x, y)) {
        ctx->ui.right_panel_slot = (uint8_t)VISUAL_RIGHT_PANEL_SLOT_LAYER;
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (hooks->point_in_rect(tab_color, x, y)) {
        ctx->ui.right_panel_slot = (uint8_t)VISUAL_RIGHT_PANEL_SLOT_COLOR;
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (hooks->point_in_rect(tab_file, x, y)) {
        ctx->ui.right_panel_slot = (uint8_t)VISUAL_RIGHT_PANEL_SLOT_FILE;
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (hooks->point_in_rect(tab_asset, x, y)) {
        ctx->ui.right_panel_slot = (uint8_t)VISUAL_RIGHT_PANEL_SLOT_ASSET;
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (hooks->point_in_rect(tab_export, x, y)) {
        ctx->ui.right_panel_slot = (uint8_t)VISUAL_RIGHT_PANEL_SLOT_EXPORT;
        hooks->sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_LAYER_VALUE) {
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
            if (hooks->point_in_rect(row, x, y)) {
                (void)drawing_program_runtime_orchestration_set_active_layer_id(
                    ctx, ctx->document.layers[model_i].layer_id);
                return;
            }
        }
        if (hooks->active_layer_query(ctx, &active_layer_id, &active_layer_index, 0, 0) &&
            active_layer_index < ctx->document.layer_count) {
            opacity_row = right_layer_opacity_row_rect(rect, m, ctx->document.layer_count);
            opacity_track = right_layer_opacity_track_rect(opacity_row, m);
            if (hooks->point_in_rect(opacity_track, x, y)) {
                int relative_x = x - opacity_track.x;
                int opacity = 100;
                if (relative_x < 0) {
                    relative_x = 0;
                }
                if (relative_x > opacity_track.w) {
                    relative_x = opacity_track.w;
                }
                if (opacity_track.w > 0) {
                    opacity = (relative_x * 100) / opacity_track.w;
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
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_ADD_LAYER);
            drawing_program_visual_layer_opacity_sync_document(ctx);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_DUPLICATE);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_layer_duplicate_active(ctx);
            drawing_program_visual_layer_opacity_sync_document(ctx);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_RENAME);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_layer_rename_auto(ctx);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_DELETE);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_DELETE_ACTIVE_LAYER);
            drawing_program_visual_layer_opacity_sync_document(ctx);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ACTIVE_PREV);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_PREV);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ACTIVE_NEXT);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_NEXT);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_MOVE_UP);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_UP);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_MOVE_DOWN);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_DOWN);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_TOGGLE_VISIBLE);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_TOGGLE_LOCK);
        if (hooks->point_in_rect(action, x, y)) {
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_LOCK);
            return;
        }
        for (role_i = 0u; role_i < (uint32_t)DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_COUNT; ++role_i) {
            role_button = right_layer_role_button_rect(rect, m, ctx->document.layer_count, role_i);
            if (hooks->point_in_rect(role_button, x, y)) {
                drawing_program_visual_apply_layer_role_preset_active(
                    ctx, (DrawingProgramVisualLayerRolePreset)role_i);
                return;
            }
        }
    } else if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_CANVAS_VALUE) {
        SDL_Rect add_surface_button;
        SDL_Rect duplicate_surface_button;
        SDL_Rect canvas_mode_button;
        SDL_Rect canvas_guide_button;
        SDL_Rect reflect_horizontal_button;
        SDL_Rect reflect_vertical_button;
        SDL_Rect center_pick_button;
        SDL_Rect center_reset_button;
        SDL_Rect delete_canvas_button;
        SDL_Rect reset_layout_button;
        SDL_Rect reset_view_button;
        SDL_Rect clear_canvas_button;
        SDL_Rect clear_objects_button;
        SDL_Rect delete_selection_button;
        SDL_Rect clear_history_button;
        uint32_t surface_index = 0u;
        uint32_t surface_count = ctx->texture_project.surface_count;
        add_surface_button = right_canvas_add_surface_button_rect(rect, m);
        if (hooks->point_in_rect(add_surface_button, x, y)) {
            visual_right_panel_disarm_canvas_delete(ui);
            visual_right_panel_disarm_canvas_reflection_pick(ui);
            if (drawing_program_texture_canvas_add_blank_from_active(ctx, &surface_index).code == CORE_OK) {
                SDL_Rect canvas_rect = {0, 0, 0, 0};
                if (drawing_program_visual_pane_rect_for_module_type(ctx, 1u, &canvas_rect)) {
                    (void)drawing_program_texture_workspace_fit_surface(ctx, canvas_rect, surface_index);
                }
            }
            return;
        }
        duplicate_surface_button = right_canvas_duplicate_surface_button_rect(rect, m);
        if (hooks->point_in_rect(duplicate_surface_button, x, y)) {
            visual_right_panel_disarm_canvas_delete(ui);
            visual_right_panel_disarm_canvas_reflection_pick(ui);
            if (drawing_program_texture_canvas_duplicate_active(ctx, &surface_index).code == CORE_OK) {
                SDL_Rect canvas_rect = {0, 0, 0, 0};
                if (drawing_program_visual_pane_rect_for_module_type(ctx, 1u, &canvas_rect)) {
                    (void)drawing_program_texture_workspace_fit_surface(ctx, canvas_rect, surface_index);
                }
            }
            return;
        }
        canvas_mode_button = right_canvas_mode_toggle_button_rect(rect, m);
        if (hooks->point_in_rect(canvas_mode_button, x, y)) {
            visual_right_panel_disarm_canvas_delete(ui);
            visual_right_panel_disarm_canvas_reflection_pick(ui);
            drawing_program_texture_canvas_toggle_control_mode(ctx);
            return;
        }
        canvas_guide_button = right_canvas_guide_mode_button_rect(rect, m);
        if (hooks->point_in_rect(canvas_guide_button, x, y)) {
            visual_right_panel_disarm_canvas_delete(ui);
            visual_right_panel_disarm_canvas_reflection_pick(ui);
            drawing_program_texture_canvas_cycle_guide_mode(ctx);
            return;
        }
        reflect_horizontal_button = right_canvas_reflect_horizontal_button_rect(rect, m);
        if (hooks->point_in_rect(reflect_horizontal_button, x, y)) {
            visual_right_panel_disarm_canvas_delete(ui);
            visual_right_panel_disarm_canvas_reflection_pick(ui);
            ctx->editor.symmetry_horizontal = ctx->editor.symmetry_horizontal ? 0u : 1u;
            drawing_program_canvas_reflection_sync_active_surface_from_editor(ctx);
            return;
        }
        reflect_vertical_button = right_canvas_reflect_vertical_button_rect(rect, m);
        if (hooks->point_in_rect(reflect_vertical_button, x, y)) {
            visual_right_panel_disarm_canvas_delete(ui);
            visual_right_panel_disarm_canvas_reflection_pick(ui);
            ctx->editor.symmetry_vertical = ctx->editor.symmetry_vertical ? 0u : 1u;
            drawing_program_canvas_reflection_sync_active_surface_from_editor(ctx);
            return;
        }
        center_pick_button = right_canvas_center_pick_button_rect(rect, m);
        if (hooks->point_in_rect(center_pick_button, x, y)) {
            visual_right_panel_disarm_canvas_delete(ui);
            if (ui && ui->right_canvas_reflection_center_pick_pending) {
                visual_right_panel_disarm_canvas_reflection_pick(ui);
            } else {
                visual_right_panel_arm_canvas_reflection_pick(ui);
            }
            return;
        }
        center_reset_button = right_canvas_center_reset_button_rect(rect, m);
        if (hooks->point_in_rect(center_reset_button, x, y)) {
            visual_right_panel_disarm_canvas_delete(ui);
            visual_right_panel_disarm_canvas_reflection_pick(ui);
            drawing_program_canvas_reflection_reset_active_center(ctx);
            return;
        }
        delete_canvas_button = right_canvas_delete_canvas_button_rect(rect, m);
        if (hooks->point_in_rect(delete_canvas_button, x, y)) {
            if (ctx->texture_project.surface_count <= 1u) {
                visual_right_panel_disarm_canvas_delete(ui);
                return;
            }
            if (visual_right_panel_canvas_delete_is_confirmable(ctx, ui)) {
                if (drawing_program_texture_canvas_delete_active(ctx, &surface_index).code == CORE_OK) {
                    SDL_Rect canvas_rect = {0, 0, 0, 0};
                    visual_right_panel_disarm_canvas_delete(ui);
                    if (drawing_program_visual_pane_rect_for_module_type(ctx, 1u, &canvas_rect)) {
                        (void)drawing_program_texture_workspace_fit_surface(ctx, canvas_rect, surface_index);
                    }
                }
                return;
            }
            if (ui &&
                ui->right_canvas_delete_confirm_pending &&
                ui->right_canvas_delete_confirm_surface_index == ctx->texture_project.active_surface_index &&
                ctx->runtime.frame_counter > ui->right_canvas_delete_confirm_armed_frame &&
                (ctx->runtime.frame_counter - ui->right_canvas_delete_confirm_armed_frame) <
                    VISUAL_RIGHT_CANVAS_DELETE_CONFIRM_MIN_FRAMES) {
                return;
            }
            visual_right_panel_arm_canvas_delete(ctx, ui);
            visual_right_panel_disarm_canvas_reflection_pick(ui);
            return;
        }
        if (ui && ui->right_canvas_delete_confirm_pending) {
            visual_right_panel_disarm_canvas_delete(ui);
        }
        for (surface_index = 0u; surface_index < surface_count; ++surface_index) {
            SDL_Rect surface_row = right_canvas_surface_row_rect(rect, m, surface_index);
            if (hooks->point_in_rect(surface_row, x, y)) {
                visual_right_panel_disarm_canvas_reflection_pick(ui);
                if (surface_index == ctx->texture_project.active_surface_index) {
                    SDL_Rect canvas_rect = {0, 0, 0, 0};
                    if (drawing_program_visual_pane_rect_for_module_type(ctx, 1u, &canvas_rect)) {
                        (void)drawing_program_texture_workspace_fit_surface(ctx, canvas_rect, surface_index);
                    }
                } else if (drawing_program_texture_project_session_select_surface(ctx, surface_index).code == CORE_OK) {
                    SDL_Rect canvas_rect = {0, 0, 0, 0};
                    if (drawing_program_visual_pane_rect_for_module_type(ctx, 1u, &canvas_rect)) {
                        (void)drawing_program_texture_workspace_fit_surface(ctx, canvas_rect, surface_index);
                    }
                }
                return;
            }
        }
        reset_layout_button = right_canvas_reset_object_layout_button_rect(rect, m);
        if (hooks->point_in_rect(reset_layout_button, x, y)) {
            visual_right_panel_disarm_canvas_reflection_pick(ui);
            if (drawing_program_texture_canvas_reset_object_layout(ctx).code == CORE_OK) {
                SDL_Rect canvas_rect = {0, 0, 0, 0};
                if (drawing_program_visual_pane_rect_for_module_type(ctx, 1u, &canvas_rect)) {
                    (void)drawing_program_texture_workspace_fit_all(ctx, canvas_rect);
                }
            }
            return;
        }
        reset_view_button = right_canvas_reset_view_button_rect(rect, m);
        if (hooks->point_in_rect(reset_view_button, x, y)) {
            visual_right_panel_disarm_canvas_reflection_pick(ui);
            SDL_Rect canvas_rect = {0, 0, 0, 0};
            if (!drawing_program_visual_pane_rect_for_module_type(ctx, 1u, &canvas_rect) ||
                !drawing_program_texture_workspace_fit_all(ctx, canvas_rect)) {
                drawing_program_viewport_reset(&ctx->editor.viewport);
            }
            return;
        }
        clear_canvas_button = right_canvas_clear_canvas_button_rect(rect, m);
        if (hooks->point_in_rect(clear_canvas_button, x, y)) {
            visual_right_panel_disarm_canvas_reflection_pick(ui);
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS);
            return;
        }
        clear_objects_button = right_canvas_clear_objects_button_rect(rect, m);
        if (hooks->point_in_rect(clear_objects_button, x, y)) {
            visual_right_panel_disarm_canvas_reflection_pick(ui);
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_OBJECTS);
            visual_panel_clear_object_target_ui(ui);
            return;
        }
        delete_selection_button = right_canvas_delete_selection_button_rect(rect, m);
        if (hooks->point_in_rect(delete_selection_button, x, y) &&
            selection &&
            hooks->delete_active_selection_payload_or_objects &&
            hooks->delete_active_selection_payload_or_objects(ctx, selection, hooks)) {
            visual_right_panel_disarm_canvas_reflection_pick(ui);
            return;
        }
        clear_history_button = right_canvas_clear_history_button_rect(rect, m);
        if (hooks->point_in_rect(clear_history_button, x, y)) {
            visual_right_panel_disarm_canvas_reflection_pick(ui);
            hooks->apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_HISTORY);
            return;
        }
    } else if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_COLOR) {
        if (drawing_program_visual_input_handle_right_color_panel_click_payload(ctx, rect, x, y, ui, hooks)) {
            return;
        }
    } else if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_FILE) {
        if (drawing_program_visual_input_handle_right_file_tab_payload(ctx, rect, x, y, selection, ui, hooks)) {
            return;
        }
    } else if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_ASSET) {
        if (drawing_program_visual_input_handle_right_asset_tab_payload(ctx, rect, x, y, selection, ui, hooks)) {
            return;
        }
    } else if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_EXPORT) {
        if (drawing_program_visual_input_handle_right_export_tab_payload(ctx, rect, x, y, ui, hooks)) {
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
