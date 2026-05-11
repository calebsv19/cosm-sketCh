#include "drawing_program/drawing_program_visual_input_right_file_tabs.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_icns_export.h"
#include "drawing_program/drawing_program_iconset_export.h"
#include "drawing_program/drawing_program_native_dialogs.h"
#include "drawing_program/drawing_program_png_export.h"
#include "drawing_program/drawing_program_project_state.h"
#include "drawing_program/drawing_program_snapshot.h"
#include "drawing_program/drawing_program_texture_export_intent.h"
#include "drawing_program/drawing_program_texture_overlay_material_intent.h"
#include "drawing_program/drawing_program_texture_export.h"
#include "drawing_program/drawing_program_texture_project_session.h"
#include "drawing_program/drawing_program_texture_scene_browser.h"
#include "drawing_program/drawing_program_texture_workspace.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_visual_right_panel_defs.h"

enum {
    VISUAL_RIGHT_FILE_ACTION_COUNT = 8,
    VISUAL_RIGHT_FILE_FOOTER_LINE_COUNT = 4,
    VISUAL_RIGHT_ASSET_ACTION_COUNT = 2,
    VISUAL_RIGHT_ASSET_FOOTER_LINE_COUNT = 4,
    VISUAL_RIGHT_EXPORT_ACTION_COUNT = 7,
    VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT = 6
};

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

static void visual_panel_clear_object_target_ui(VisualPanelUiState *ui) {
    if (!ui) {
        return;
    }
    ui->object_color_target_kind = VISUAL_OBJECT_COLOR_TARGET_NONE;
    ui->object_color_target_object_id = 0u;
}

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
    char suggested_name[128];
    char chosen_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx || !ui || !hooks || !hooks->sync_panel_ui_from_app) {
        return 0;
    }
    visual_right_panel_default_project_filename(ctx, suggested_name, sizeof(suggested_name));
    result = drawing_program_native_choose_save_file("Save project as",
                                                     ctx->session.input_root_path,
                                                     suggested_name,
                                                     chosen_path,
                                                     sizeof(chosen_path));
    if (result.code != CORE_OK) {
        visual_right_panel_set_file_status(ctx, "SAVE AS CANCELLED");
        return 0;
    }
    result = drawing_program_project_state_set_current_path(ctx, chosen_path);
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
    ctx->ui.right_panel_slot = (uint8_t)VISUAL_RIGHT_PANEL_SLOT_CANVAS;
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

static int visual_right_panel_toggle_texture_export_intent(DrawingProgramAppContext *ctx) {
    uint32_t next_kind = 0u;
    if (!ctx) {
        return 0;
    }
    next_kind = drawing_program_texture_export_intent_cycle(ctx->texture_project.export_intent_kind);
    ctx->texture_project.export_intent_kind = next_kind;
    visual_right_panel_set_file_status(ctx,
                                       (next_kind == DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_BASE_PLUS_OVERLAY)
                                           ? "EXPORT INTENT BASE+OVERLAY"
                                           : "EXPORT INTENT FLATTENED");
    return 1;
}

static int visual_right_panel_toggle_overlay_material_intent(DrawingProgramAppContext *ctx) {
    uint32_t next_kind = 0u;
    if (!ctx) {
        return 0;
    }
    next_kind = drawing_program_texture_overlay_material_intent_cycle(
        ctx->texture_project.overlay_material_intent_kind);
    ctx->texture_project.overlay_material_intent_kind = next_kind;
    visual_right_panel_set_file_status(ctx,
                                       "OVERLAY INTENT %s",
                                       drawing_program_texture_overlay_material_intent_kind_name(next_kind));
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
    queue_rect = right_file_project_queue_rect(rect, m, VISUAL_RIGHT_FILE_ACTION_COUNT, VISUAL_RIGHT_FILE_FOOTER_LINE_COUNT);
    if (!hooks->point_in_rect(queue_rect, x, y)) {
        return 0;
    }
    slot_count = right_file_target_queue_slot_count(ctx);
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

static int visual_right_panel_asset_queue_hit_slot(DrawingProgramAppContext *ctx,
                                                   SDL_Rect rect,
                                                   int x,
                                                   int y,
                                                   VisualPanelUiState *ui,
                                                   const DrawingProgramVisualInputHandlersHooks *hooks,
                                                   uint32_t *out_slot_index,
                                                   uint32_t *out_scene_entry_count,
                                                   uint32_t *out_object_entry_count,
                                                   DrawingProgramTextureSceneFileEntry *scene_entries,
                                                   DrawingProgramTextureSceneObjectEntry *object_entries) {
    VisualPaneLayoutMetrics m;
    SDL_Rect queue_rect;
    uint32_t slot_count = 0u;
    int scroll_y;
    uint32_t i;
    if (!ctx || !ui || !hooks || !hooks->point_in_rect || !out_slot_index || !out_scene_entry_count ||
        !out_object_entry_count || !scene_entries || !object_entries) {
        return 0;
    }
    *out_scene_entry_count = 0u;
    *out_object_entry_count = 0u;
    (void)drawing_program_texture_scene_browser_list_scene_files(ctx->session.scene_authored_root_path,
                                                                 scene_entries,
                                                                 DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY,
                                                                 out_scene_entry_count);
    if (ctx->session.selected_scene_path[0] != '\0') {
        (void)drawing_program_texture_scene_browser_list_supported_objects(ctx->session.selected_scene_path,
                                                                           object_entries,
                                                                           DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY,
                                                                           out_object_entry_count,
                                                                           0,
                                                                           0u);
    }
    if (ui->right_file_browser_mode == (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_OBJECTS &&
        ctx->session.selected_scene_path[0] != '\0') {
        slot_count = *out_object_entry_count > 0u ? *out_object_entry_count : 1u;
    } else {
        slot_count = *out_scene_entry_count > 0u ? *out_scene_entry_count : 1u;
    }
    m = make_pane_layout_metrics(ctx);
    queue_rect = right_asset_target_queue_rect(rect, m, VISUAL_RIGHT_ASSET_FOOTER_LINE_COUNT, VISUAL_RIGHT_ASSET_ACTION_COUNT);
    if (!hooks->point_in_rect(queue_rect, x, y)) {
        return 0;
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

int drawing_program_visual_input_handle_right_file_tab_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    DrawingProgramSelectionState *selection,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
    VisualPaneLayoutMetrics m;
    uint32_t slot_index = 0u;
    SDL_Rect new_project_button;
    SDL_Rect open_project_button;
    SDL_Rect save_project_button;
    SDL_Rect save_as_button;
    SDL_Rect load_project_button;
    SDL_Rect pick_input_root_button;
    SDL_Rect save_session_button;
    SDL_Rect reload_session_button;
    if (!ctx || !ui || !hooks) {
        return 0;
    }
    m = make_pane_layout_metrics(ctx);
    new_project_button = right_file_action_button_rect(rect, m, 0u, VISUAL_RIGHT_FILE_ACTION_COUNT);
    open_project_button = right_file_action_button_rect(rect, m, 1u, VISUAL_RIGHT_FILE_ACTION_COUNT);
    save_project_button = right_file_action_button_rect(rect, m, 2u, VISUAL_RIGHT_FILE_ACTION_COUNT);
    save_as_button = right_file_action_button_rect(rect, m, 3u, VISUAL_RIGHT_FILE_ACTION_COUNT);
    load_project_button = right_file_action_button_rect(rect, m, 4u, VISUAL_RIGHT_FILE_ACTION_COUNT);
    pick_input_root_button = right_file_action_button_rect(rect, m, 5u, VISUAL_RIGHT_FILE_ACTION_COUNT);
    save_session_button = right_file_action_button_rect(rect, m, 6u, VISUAL_RIGHT_FILE_ACTION_COUNT);
    reload_session_button = right_file_action_button_rect(rect, m, 7u, VISUAL_RIGHT_FILE_ACTION_COUNT);
    if (visual_right_panel_file_queue_hit_slot(ctx, rect, x, y, ui, hooks, &slot_index)) {
        (void)visual_right_panel_select_project_slot(ctx, slot_index, ui, hooks);
        return 1;
    }
    if (hooks->point_in_rect(new_project_button, x, y)) {
        (void)visual_right_panel_new_project(ctx, ui, hooks);
        return 1;
    }
    if (hooks->point_in_rect(open_project_button, x, y)) {
        (void)visual_right_panel_open_project(ctx, selection, ui, hooks);
        return 1;
    }
    if (hooks->point_in_rect(save_project_button, x, y)) {
        (void)visual_right_panel_save_project(ctx);
        return 1;
    }
    if (hooks->point_in_rect(save_as_button, x, y)) {
        (void)visual_right_panel_save_as_project(ctx, ui, hooks);
        return 1;
    }
    if (hooks->point_in_rect(load_project_button, x, y)) {
        (void)visual_right_panel_load_project(ctx, selection, ui, hooks);
        return 1;
    }
    if (hooks->point_in_rect(pick_input_root_button, x, y)) {
        (void)visual_right_panel_pick_input_root(ctx, ui, hooks);
        return 1;
    }
    if (hooks->point_in_rect(save_session_button, x, y)) {
        (void)visual_right_panel_save_session(ctx);
        return 1;
    }
    if (hooks->point_in_rect(reload_session_button, x, y)) {
        (void)visual_right_panel_reload_session(ctx, selection, ui, hooks);
        return 1;
    }
    return 0;
}

int drawing_program_visual_input_handle_right_asset_tab_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    DrawingProgramSelectionState *selection,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
    DrawingProgramTextureSceneFileEntry scene_entries[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY];
    DrawingProgramTextureSceneObjectEntry object_entries[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY];
    uint32_t scene_entry_count = 0u;
    uint32_t object_entry_count = 0u;
    uint32_t slot_index = 0u;
    VisualPaneLayoutMetrics m;
    SDL_Rect browser_scenes_tab;
    SDL_Rect browser_objects_tab;
    SDL_Rect pick_scene_root_button;
    SDL_Rect open_object_button;
    (void)selection;
    if (!ctx || !ui || !hooks) {
        return 0;
    }
    m = make_pane_layout_metrics(ctx);
    browser_scenes_tab = right_asset_browser_mode_tab_rect(rect, m, 0u, 2u);
    browser_objects_tab = right_asset_browser_mode_tab_rect(rect, m, 1u, 2u);
    pick_scene_root_button = right_file_route_action_button_rect(rect,
                                                                 m,
                                                                 VISUAL_RIGHT_ASSET_FOOTER_LINE_COUNT,
                                                                 0u,
                                                                 VISUAL_RIGHT_ASSET_ACTION_COUNT);
    open_object_button = right_file_route_action_button_rect(rect,
                                                             m,
                                                             VISUAL_RIGHT_ASSET_FOOTER_LINE_COUNT,
                                                             1u,
                                                             VISUAL_RIGHT_ASSET_ACTION_COUNT);
    if (hooks->point_in_rect(browser_scenes_tab, x, y)) {
        ui->right_file_browser_mode = (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_SCENES;
        ui->right_file_target_queue_scroll_y = 0;
        return 1;
    }
    if (hooks->point_in_rect(browser_objects_tab, x, y) && ctx->session.selected_scene_path[0] != '\0') {
        ui->right_file_browser_mode = (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_OBJECTS;
        ui->right_file_target_queue_scroll_y = 0;
        return 1;
    }
    if (visual_right_panel_asset_queue_hit_slot(ctx,
                                                rect,
                                                x,
                                                y,
                                                ui,
                                                hooks,
                                                &slot_index,
                                                &scene_entry_count,
                                                &object_entry_count,
                                                scene_entries,
                                                object_entries)) {
        if (ui->right_file_browser_mode == (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_OBJECTS &&
            slot_index < object_entry_count) {
            (void)snprintf(ctx->session.selected_scene_object_id,
                           sizeof(ctx->session.selected_scene_object_id),
                           "%s",
                           object_entries[slot_index].object_id);
            visual_right_panel_set_file_status(ctx, "OBJECT SELECTED");
        } else if (slot_index < scene_entry_count) {
            (void)snprintf(ctx->session.selected_scene_path,
                           sizeof(ctx->session.selected_scene_path),
                           "%s",
                           scene_entries[slot_index].scene_path);
            ctx->session.selected_scene_object_id[0] = '\0';
            ui->right_file_browser_mode = (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_OBJECTS;
            ui->right_file_target_queue_scroll_y = 0;
            visual_right_panel_set_file_status(ctx, "SCENE SELECTED");
        }
        return 1;
    }
    if (hooks->point_in_rect(pick_scene_root_button, x, y)) {
        (void)visual_right_panel_pick_scene_root(ctx, ui, hooks);
        return 1;
    }
    if (hooks->point_in_rect(open_object_button, x, y)) {
        (void)visual_right_panel_open_selected_scene_object(ctx, ui, hooks);
        return 1;
    }
    return 0;
}

int drawing_program_visual_input_handle_right_export_tab_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
    VisualPaneLayoutMetrics m;
    SDL_Rect pick_output_root_button;
    SDL_Rect export_intent_button;
    SDL_Rect overlay_material_intent_button;
    SDL_Rect export_png_button;
    SDL_Rect export_textures_button;
    SDL_Rect export_iconset_button;
    SDL_Rect export_icns_button;
    (void)ui;
    (void)hooks;
    if (!ctx) {
        return 0;
    }
    m = make_pane_layout_metrics(ctx);
    pick_output_root_button = right_file_route_action_button_rect(rect,
                                                                  m,
                                                                  VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT,
                                                                  0u,
                                                                  VISUAL_RIGHT_EXPORT_ACTION_COUNT);
    export_intent_button = right_file_route_action_button_rect(rect,
                                                               m,
                                                               VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT,
                                                               1u,
                                                               VISUAL_RIGHT_EXPORT_ACTION_COUNT);
    overlay_material_intent_button = right_file_route_action_button_rect(rect,
                                                                         m,
                                                                         VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT,
                                                                         2u,
                                                                         VISUAL_RIGHT_EXPORT_ACTION_COUNT);
    export_png_button = right_file_route_action_button_rect(rect,
                                                            m,
                                                            VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT,
                                                            3u,
                                                            VISUAL_RIGHT_EXPORT_ACTION_COUNT);
    export_textures_button = right_file_route_action_button_rect(rect,
                                                                 m,
                                                                 VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT,
                                                                 4u,
                                                                 VISUAL_RIGHT_EXPORT_ACTION_COUNT);
    export_iconset_button = right_file_route_action_button_rect(rect,
                                                                m,
                                                                VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT,
                                                                5u,
                                                                VISUAL_RIGHT_EXPORT_ACTION_COUNT);
    export_icns_button = right_file_route_action_button_rect(rect,
                                                             m,
                                                             VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT,
                                                             6u,
                                                             VISUAL_RIGHT_EXPORT_ACTION_COUNT);
    if (hooks->point_in_rect(pick_output_root_button, x, y)) {
        (void)visual_right_panel_pick_output_root(ctx);
        return 1;
    }
    if (hooks->point_in_rect(export_intent_button, x, y)) {
        (void)visual_right_panel_toggle_texture_export_intent(ctx);
        return 1;
    }
    if (hooks->point_in_rect(overlay_material_intent_button, x, y)) {
        (void)visual_right_panel_toggle_overlay_material_intent(ctx);
        return 1;
    }
    if (hooks->point_in_rect(export_png_button, x, y)) {
        (void)visual_right_panel_export_png(ctx);
        return 1;
    }
    if (hooks->point_in_rect(export_textures_button, x, y)) {
        (void)visual_right_panel_export_textures(ctx);
        return 1;
    }
    if (hooks->point_in_rect(export_iconset_button, x, y)) {
        (void)visual_right_panel_export_iconset(ctx);
        return 1;
    }
    if (hooks->point_in_rect(export_icns_button, x, y)) {
        (void)visual_right_panel_export_icns(ctx);
        return 1;
    }
    return 0;
}

int drawing_program_visual_input_handle_right_file_tabs_wheel_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    int wheel_y,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
    VisualPaneLayoutMetrics m;
    SDL_Rect queue_rect;
    uint32_t slot_count = 0u;
    int scroll_y;
    int row_stride;
    if (!ctx || !ui || !hooks || !hooks->point_in_rect || wheel_y == 0) {
        return 0;
    }
    m = make_pane_layout_metrics(ctx);
    if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_FILE) {
        queue_rect = right_file_project_queue_rect(rect, m, VISUAL_RIGHT_FILE_ACTION_COUNT, VISUAL_RIGHT_FILE_FOOTER_LINE_COUNT);
        slot_count = right_file_target_queue_slot_count(ctx);
    } else if (hooks->clamp_right_slot(ctx->ui.right_panel_slot) == (uint8_t)VISUAL_RIGHT_PANEL_SLOT_ASSET) {
        DrawingProgramTextureSceneFileEntry scene_entries[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY];
        DrawingProgramTextureSceneObjectEntry object_entries[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY];
        uint32_t scene_count = 0u;
        uint32_t object_count = 0u;
        queue_rect = right_asset_target_queue_rect(rect, m, VISUAL_RIGHT_ASSET_FOOTER_LINE_COUNT, VISUAL_RIGHT_ASSET_ACTION_COUNT);
        (void)drawing_program_texture_scene_browser_list_scene_files(ctx->session.scene_authored_root_path,
                                                                     scene_entries,
                                                                     DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY,
                                                                     &scene_count);
        if (ctx->session.selected_scene_path[0] != '\0') {
            (void)drawing_program_texture_scene_browser_list_supported_objects(ctx->session.selected_scene_path,
                                                                               object_entries,
                                                                               DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY,
                                                                               &object_count,
                                                                               0,
                                                                               0u);
        }
        if (ui->right_file_browser_mode == (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_OBJECTS &&
            ctx->session.selected_scene_path[0] != '\0') {
            slot_count = object_count > 0u ? object_count : 1u;
        } else {
            slot_count = scene_count > 0u ? scene_count : 1u;
        }
    } else {
        return 0;
    }
    if (!hooks->point_in_rect(queue_rect, x, y)) {
        return 0;
    }
    row_stride = m.row_h + m.section_gap;
    if (row_stride < 1) {
        row_stride = 1;
    }
    scroll_y = ui->right_file_target_queue_scroll_y - (wheel_y * row_stride);
    ui->right_file_target_queue_scroll_y =
        right_file_target_queue_clamp_scroll(queue_rect, m, slot_count, scroll_y);
    return 1;
}
