#include "drawing_program/drawing_program_visual_right_panel_file_tabs_render.h"

#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_icns_export.h"
#include "drawing_program/drawing_program_iconset_export.h"
#include "drawing_program/drawing_program_png_export.h"
#include "drawing_program/drawing_program_project_state.h"
#include "drawing_program/drawing_program_texture_export_intent.h"
#include "drawing_program/drawing_program_texture_overlay_material_intent.h"
#include "drawing_program/drawing_program_texture_export.h"
#include "drawing_program/drawing_program_texture_scene_browser.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_panel_render_common.h"

enum {
    VISUAL_RIGHT_FILE_ACTION_COUNT = 8,
    VISUAL_RIGHT_FILE_FOOTER_LINE_COUNT = 4,
    VISUAL_RIGHT_ASSET_ACTION_COUNT = 2,
    VISUAL_RIGHT_ASSET_FOOTER_LINE_COUNT = 4,
    VISUAL_RIGHT_EXPORT_ACTION_COUNT = 7,
    VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT = 6
};

static void visual_right_panel_format_path_line(char *out_line,
                                                size_t out_cap,
                                                const char *label,
                                                const char *path) {
    size_t path_len = 0u;
    const char *display_path = path;
    if (!out_line || out_cap == 0u) {
        return;
    }
    if (!label) {
        label = "PATH";
    }
    if (!path || path[0] == '\0') {
        (void)snprintf(out_line, out_cap, "%s (unset)", label);
        return;
    }
    path_len = strlen(path);
    if (path_len > 28u) {
        display_path = path + (path_len - 28u);
        while (*display_path != '\0' && *display_path != '/') {
            ++display_path;
        }
        if (*display_path == '/') {
            ++display_path;
        } else {
            display_path = path + (path_len - 28u);
        }
        (void)snprintf(out_line, out_cap, "%s .../%s", label, display_path);
        return;
    }
    (void)snprintf(out_line, out_cap, "%s %s", label, path);
}

static void visual_right_panel_format_recent_project_line(char *out_line,
                                                          size_t out_cap,
                                                          uint32_t row_index,
                                                          const char *path,
                                                          uint8_t existing) {
    const char *name = path;
    size_t name_len = 0u;
    if (!out_line || out_cap == 0u) {
        return;
    }
    if (!path || path[0] == '\0') {
        (void)snprintf(out_line, out_cap, "N%u NEW TARGET", (unsigned)(row_index + 1u));
        return;
    }
    name = strrchr(path, '/');
    name = name ? (name + 1) : path;
    name_len = strlen(name);
    if (name_len > 22u) {
        name += (name_len - 22u);
    }
    (void)snprintf(out_line,
                   out_cap,
                   "%c%u %s %s",
                   existing ? 'S' : 'N',
                   (unsigned)(row_index + 1u),
                   existing ? "LOAD" : "SAVE",
                   name);
}

static void visual_right_panel_format_scene_line(char *out_line,
                                                 size_t out_cap,
                                                 uint32_t row_index,
                                                 const DrawingProgramTextureSceneFileEntry *entry) {
    if (!out_line || out_cap == 0u) {
        return;
    }
    if (!entry) {
        (void)snprintf(out_line, out_cap, "SCENE %u", (unsigned)(row_index + 1u));
        return;
    }
    (void)snprintf(out_line, out_cap, "%u %s", (unsigned)(row_index + 1u), entry->scene_id);
}

static void visual_right_panel_format_scene_object_line(char *out_line,
                                                        size_t out_cap,
                                                        uint32_t row_index,
                                                        const DrawingProgramTextureSceneObjectEntry *entry) {
    if (!out_line || out_cap == 0u) {
        return;
    }
    if (!entry) {
        (void)snprintf(out_line, out_cap, "OBJECT %u", (unsigned)(row_index + 1u));
        return;
    }
    (void)snprintf(out_line, out_cap, "%u %s", (unsigned)(row_index + 1u), entry->summary);
}

static void visual_right_panel_draw_scrollbar(SDL_Renderer *renderer,
                                              SDL_Rect viewport,
                                              int offset_y,
                                              int max_offset_y,
                                              SDL_Color track_color,
                                              SDL_Color thumb_color) {
    SDL_Rect track;
    SDL_Rect thumb;
    float ratio;
    if (!renderer || viewport.h <= 0 || max_offset_y <= 0) {
        return;
    }
    track.x = viewport.x + viewport.w - 8;
    track.y = viewport.y;
    track.w = 6;
    track.h = viewport.h;
    if (track.w < 1 || track.h < 1) {
        return;
    }
    if (offset_y < 0) {
        offset_y = 0;
    }
    if (offset_y > max_offset_y) {
        offset_y = max_offset_y;
    }
    ratio = (float)viewport.h / (float)(viewport.h + max_offset_y);
    if (ratio < 0.1f) {
        ratio = 0.1f;
    }
    thumb = track;
    thumb.h = (int)((float)track.h * ratio);
    if (thumb.h < 6) {
        thumb.h = 6;
    }
    if (thumb.h > track.h) {
        thumb.h = track.h;
    }
    thumb.y = track.y + (int)((float)(track.h - thumb.h) * ((float)offset_y / (float)max_offset_y));
    SDL_SetRenderDrawColor(renderer, track_color.r, track_color.g, track_color.b, track_color.a);
    (void)SDL_RenderFillRect(renderer, &track);
    SDL_SetRenderDrawColor(renderer, thumb_color.r, thumb_color.g, thumb_color.b, thumb_color.a);
    (void)SDL_RenderFillRect(renderer, &thumb);
}

static void visual_right_panel_draw_queue_row(SDL_Renderer *renderer,
                                              SDL_Rect clip_rect,
                                              SDL_Rect row_rect,
                                              const char *label,
                                              int selected,
                                              int hovered,
                                              VisualPaneLayoutMetrics m,
                                              VisualThemePalette p,
                                              const DrawingProgramVisualPanelRenderHooks *hooks) {
    SDL_Color fill = selected ? p.button_fill_active : (hovered ? p.button_fill_hover : p.button_fill);
    SDL_Color border = selected ? p.accent_primary : p.button_border;
    (void)SDL_RenderSetClipRect(renderer, &clip_rect);
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
    (void)SDL_RenderFillRect(renderer, &row_rect);
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
    (void)SDL_RenderDrawRect(renderer, &row_rect);
    hooks->draw_bitmap_text(renderer,
                            clip_rect,
                            row_rect.x + 6,
                            row_rect.y + m.row_text_y,
                            label,
                            selected ? p.text_primary : p.text_muted,
                            m.body_scale);
}

static void visual_right_panel_draw_button(SDL_Renderer *renderer,
                                           SDL_Rect rect,
                                           SDL_Rect button_rect,
                                           const char *label,
                                           SDL_Color text_color,
                                           const VisualPanelUiState *ui,
                                           VisualPaneLayoutMetrics m,
                                           VisualThemePalette p,
                                           const DrawingProgramVisualPanelRenderHooks *hooks) {
    drawing_program_visual_panel_draw_tab_button(renderer,
                                                 rect,
                                                 button_rect,
                                                 label,
                                                 p.button_fill,
                                                 p.button_fill_hover,
                                                 p.button_fill_active,
                                                 p.button_border,
                                                 text_color,
                                                 m.body_scale,
                                                 0,
                                                 drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks),
                                                 hooks);
}

void drawing_program_visual_render_right_file_tab(SDL_Renderer *renderer,
                                                  SDL_Rect rect,
                                                  const DrawingProgramAppContext *ctx,
                                                  const VisualPanelUiState *ui,
                                                  VisualPaneLayoutMetrics m,
                                                  VisualThemePalette p,
                                                  const DrawingProgramVisualPanelRenderHooks *hooks) {
    char line[160];
    uint32_t target_slot_count = right_file_target_queue_slot_count(ctx);
    SDL_Rect new_project_button = right_file_action_button_rect(rect, m, 0u, VISUAL_RIGHT_FILE_ACTION_COUNT);
    SDL_Rect open_project_button = right_file_action_button_rect(rect, m, 1u, VISUAL_RIGHT_FILE_ACTION_COUNT);
    SDL_Rect save_project_button = right_file_action_button_rect(rect, m, 2u, VISUAL_RIGHT_FILE_ACTION_COUNT);
    SDL_Rect save_as_button = right_file_action_button_rect(rect, m, 3u, VISUAL_RIGHT_FILE_ACTION_COUNT);
    SDL_Rect load_project_button = right_file_action_button_rect(rect, m, 4u, VISUAL_RIGHT_FILE_ACTION_COUNT);
    SDL_Rect pick_input_root_button = right_file_action_button_rect(rect, m, 5u, VISUAL_RIGHT_FILE_ACTION_COUNT);
    SDL_Rect save_session_button = right_file_action_button_rect(rect, m, 6u, VISUAL_RIGHT_FILE_ACTION_COUNT);
    SDL_Rect reload_session_button = right_file_action_button_rect(rect, m, 7u, VISUAL_RIGHT_FILE_ACTION_COUNT);
    SDL_Rect target_queue_rect =
        right_file_project_queue_rect(rect, m, VISUAL_RIGHT_FILE_ACTION_COUNT, VISUAL_RIGHT_FILE_FOOTER_LINE_COUNT);
    int target_scroll_y =
        ui ? right_file_target_queue_clamp_scroll(target_queue_rect,
                                                  m,
                                                  target_slot_count,
                                                  ui->right_file_target_queue_scroll_y)
           : 0;
    int target_scroll_max = right_file_target_queue_scroll_max(target_queue_rect, m, target_slot_count);
    int y = right_file_content_start_y(rect, m);
    uint32_t i;

    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "PROJECT ACTIONS", p.text_primary, m.body_scale);
    hooks->draw_bitmap_text(renderer,
                            rect,
                            rect.x + m.pad_x,
                            right_file_target_queue_label_y(rect, m, VISUAL_RIGHT_FILE_ACTION_COUNT),
                            "SAVED DRAWING.PACK TARGETS",
                            p.text_primary,
                            m.body_scale);

    visual_right_panel_draw_button(renderer, rect, new_project_button, "NEW BLANK", p.text_primary, ui, m, p, hooks);
    visual_right_panel_draw_button(renderer, rect, open_project_button, "OPEN PROJECT", p.text_primary, ui, m, p, hooks);
    visual_right_panel_draw_button(renderer, rect, save_project_button, "SAVE", p.text_primary, ui, m, p, hooks);
    visual_right_panel_draw_button(renderer, rect, save_as_button, "SAVE AS", p.text_primary, ui, m, p, hooks);
    visual_right_panel_draw_button(renderer, rect, load_project_button, "LOAD SELECTED", p.text_primary, ui, m, p, hooks);
    visual_right_panel_draw_button(renderer,
                                   rect,
                                   pick_input_root_button,
                                   "PICK INPUT ROOT",
                                   p.text_primary,
                                   ui,
                                   m,
                                   p,
                                   hooks);
    visual_right_panel_draw_button(renderer, rect, save_session_button, "SAVE SESSION", p.text_primary, ui, m, p, hooks);
    visual_right_panel_draw_button(renderer,
                                   rect,
                                   reload_session_button,
                                   "RELOAD SESSION",
                                   p.text_primary,
                                   ui,
                                   m,
                                   p,
                                   hooks);

    SDL_SetRenderDrawColor(renderer,
                           p.pane_background_alt.r,
                           p.pane_background_alt.g,
                           p.pane_background_alt.b,
                           p.pane_background_alt.a);
    (void)SDL_RenderFillRect(renderer, &target_queue_rect);
    SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
    (void)SDL_RenderDrawRect(renderer, &target_queue_rect);
    for (i = 0u; i < target_slot_count; ++i) {
        SDL_Rect row_rect = right_file_target_queue_row_rect(target_queue_rect, m, i, target_scroll_y);
        SDL_Rect clipped_row;
        char slot_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        uint8_t existing = 0u;
        int selected = 0;
        int hovered = 0;
        if (!SDL_IntersectRect(&row_rect, &target_queue_rect, &clipped_row)) {
            continue;
        }
        slot_path[0] = '\0';
        if (drawing_program_project_state_slot_path(ctx, i, slot_path, sizeof(slot_path), &existing).code != CORE_OK) {
            slot_path[0] = '\0';
            existing = 0u;
        }
        selected = (slot_path[0] != '\0' && ctx->session.project_path &&
                    strcmp(slot_path, ctx->session.project_path) == 0)
                       ? 1
                       : 0;
        visual_right_panel_format_recent_project_line(line, sizeof(line), i, slot_path, existing);
        hovered = drawing_program_visual_panel_ui_hovered(ui, row_rect, hooks);
        visual_right_panel_draw_queue_row(renderer, target_queue_rect, row_rect, line, selected, hovered, m, p, hooks);
    }
    (void)SDL_RenderSetClipRect(renderer, 0);
    visual_right_panel_draw_scrollbar(renderer,
                                      target_queue_rect,
                                      target_scroll_y,
                                      target_scroll_max,
                                      p.button_fill,
                                      p.button_border);

    y = right_file_state_start_y(rect, m, VISUAL_RIGHT_FILE_FOOTER_LINE_COUNT);
    visual_right_panel_format_path_line(line, sizeof(line), "PROJECT", ctx->session.project_path);
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
    y += m.line_h;
    (void)snprintf(line,
                   sizeof(line),
                   "STATUS %s",
                   drawing_program_project_state_current_is_dirty(ctx) ? "DIRTY" : "CLEAN");
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
    y += m.line_h;
    visual_right_panel_format_path_line(line, sizeof(line), "INPUT", ctx->session.input_root_path);
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
    y += m.line_h;
    (void)snprintf(line, sizeof(line), "ACTION %s", ctx->session.file_action_status_message);
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
}

void drawing_program_visual_render_right_asset_tab(SDL_Renderer *renderer,
                                                   SDL_Rect rect,
                                                   const DrawingProgramAppContext *ctx,
                                                   const VisualPanelUiState *ui,
                                                   VisualPaneLayoutMetrics m,
                                                   VisualThemePalette p,
                                                   const DrawingProgramVisualPanelRenderHooks *hooks) {
    char line[160];
    DrawingProgramTextureSceneFileEntry scene_entries[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY];
    DrawingProgramTextureSceneObjectEntry object_entries[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY];
    uint32_t scene_entry_count = 0u;
    uint32_t object_entry_count = 0u;
    uint32_t target_slot_count = 0u;
    uint8_t browser_mode = ui ? ui->right_file_browser_mode : (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_SCENES;
    SDL_Rect browser_scenes_tab = right_asset_browser_mode_tab_rect(rect, m, 0u, 2u);
    SDL_Rect browser_objects_tab = right_asset_browser_mode_tab_rect(rect, m, 1u, 2u);
    SDL_Rect target_queue_rect =
        right_asset_target_queue_rect(rect, m, VISUAL_RIGHT_ASSET_FOOTER_LINE_COUNT, VISUAL_RIGHT_ASSET_ACTION_COUNT);
    SDL_Rect pick_scene_root_button = right_file_route_action_button_rect(rect,
                                                                          m,
                                                                          VISUAL_RIGHT_ASSET_FOOTER_LINE_COUNT,
                                                                          0u,
                                                                          VISUAL_RIGHT_ASSET_ACTION_COUNT);
    SDL_Rect open_object_button = right_file_route_action_button_rect(rect,
                                                                      m,
                                                                      VISUAL_RIGHT_ASSET_FOOTER_LINE_COUNT,
                                                                      1u,
                                                                      VISUAL_RIGHT_ASSET_ACTION_COUNT);
    int target_scroll_y = 0;
    int target_scroll_max = 0;
    int y = right_file_content_start_y(rect, m);
    uint32_t i;

    (void)drawing_program_texture_scene_browser_list_scene_files(ctx->session.scene_authored_root_path,
                                                                 scene_entries,
                                                                 DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY,
                                                                 &scene_entry_count);
    if (ctx->session.selected_scene_path[0] != '\0') {
        (void)drawing_program_texture_scene_browser_list_supported_objects(ctx->session.selected_scene_path,
                                                                           object_entries,
                                                                           DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY,
                                                                           &object_entry_count,
                                                                           0,
                                                                           0u);
    }
    if (browser_mode == (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_OBJECTS &&
        ctx->session.selected_scene_path[0] != '\0') {
        target_slot_count = object_entry_count > 0u ? object_entry_count : 1u;
    } else {
        target_slot_count = scene_entry_count > 0u ? scene_entry_count : 1u;
    }
    target_scroll_y =
        ui ? right_file_target_queue_clamp_scroll(target_queue_rect,
                                                  m,
                                                  target_slot_count,
                                                  ui->right_file_target_queue_scroll_y)
           : 0;
    target_scroll_max = right_file_target_queue_scroll_max(target_queue_rect, m, target_slot_count);

    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "OBJECT LOADER", p.text_primary, m.body_scale);
    drawing_program_visual_panel_draw_tab_button(renderer,
                                                 rect,
                                                 browser_scenes_tab,
                                                 "SCENES",
                                                 p.button_fill,
                                                 p.button_fill_hover,
                                                 p.button_fill_active,
                                                 p.button_border,
                                                 p.text_primary,
                                                 m.body_scale,
                                                 browser_mode != (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_OBJECTS,
                                                 drawing_program_visual_panel_ui_hovered(ui, browser_scenes_tab, hooks),
                                                 hooks);
    drawing_program_visual_panel_draw_tab_button(renderer,
                                                 rect,
                                                 browser_objects_tab,
                                                 "OBJECTS",
                                                 p.button_fill,
                                                 p.button_fill_hover,
                                                 p.button_fill_active,
                                                 p.button_border,
                                                 (ctx->session.selected_scene_path[0] != '\0') ? p.text_primary : p.text_muted,
                                                 m.body_scale,
                                                 browser_mode == (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_OBJECTS,
                                                 drawing_program_visual_panel_ui_hovered(ui, browser_objects_tab, hooks),
                                                 hooks);

    SDL_SetRenderDrawColor(renderer,
                           p.pane_background_alt.r,
                           p.pane_background_alt.g,
                           p.pane_background_alt.b,
                           p.pane_background_alt.a);
    (void)SDL_RenderFillRect(renderer, &target_queue_rect);
    SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
    (void)SDL_RenderDrawRect(renderer, &target_queue_rect);
    for (i = 0u; i < target_slot_count; ++i) {
        SDL_Rect row_rect = right_file_target_queue_row_rect(target_queue_rect, m, i, target_scroll_y);
        SDL_Rect clipped_row;
        int selected = 0;
        int hovered = 0;
        if (!SDL_IntersectRect(&row_rect, &target_queue_rect, &clipped_row)) {
            continue;
        }
        if (browser_mode == (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_OBJECTS &&
            ctx->session.selected_scene_path[0] != '\0' && i < object_entry_count) {
            selected = strcmp(object_entries[i].object_id, ctx->session.selected_scene_object_id) == 0;
            visual_right_panel_format_scene_object_line(line, sizeof(line), i, &object_entries[i]);
        } else if (i < scene_entry_count) {
            selected = strcmp(scene_entries[i].scene_path, ctx->session.selected_scene_path) == 0;
            visual_right_panel_format_scene_line(line, sizeof(line), i, &scene_entries[i]);
        } else if (browser_mode == (uint8_t)VISUAL_RIGHT_FILE_BROWSER_MODE_OBJECTS) {
            (void)snprintf(line, sizeof(line), "NO SUPPORTED OBJECTS");
        } else {
            (void)snprintf(line, sizeof(line), "NO AUTHORED SCENES");
        }
        hovered = drawing_program_visual_panel_ui_hovered(ui, row_rect, hooks);
        visual_right_panel_draw_queue_row(renderer, target_queue_rect, row_rect, line, selected, hovered, m, p, hooks);
    }
    (void)SDL_RenderSetClipRect(renderer, 0);
    visual_right_panel_draw_scrollbar(renderer,
                                      target_queue_rect,
                                      target_scroll_y,
                                      target_scroll_max,
                                      p.button_fill,
                                      p.button_border);

    visual_right_panel_draw_button(renderer,
                                   rect,
                                   pick_scene_root_button,
                                   "PICK SCENE ROOT",
                                   p.text_primary,
                                   ui,
                                   m,
                                   p,
                                   hooks);
    visual_right_panel_draw_button(renderer,
                                   rect,
                                   open_object_button,
                                   "OPEN OBJECT AS PACK",
                                   (ctx->session.selected_scene_path[0] != '\0' &&
                                    ctx->session.selected_scene_object_id[0] != '\0')
                                       ? p.text_primary
                                       : p.text_muted,
                                   ui,
                                   m,
                                   p,
                                   hooks);

    y = right_file_state_start_y(rect, m, VISUAL_RIGHT_ASSET_FOOTER_LINE_COUNT);
    visual_right_panel_format_path_line(line, sizeof(line), "SCENE ROOT", ctx->session.scene_authored_root_path);
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
    y += m.line_h;
    visual_right_panel_format_path_line(line, sizeof(line), "SCENE", ctx->session.selected_scene_path);
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
    y += m.line_h;
    (void)snprintf(line,
                   sizeof(line),
                   "OBJECT %s",
                   ctx->session.selected_scene_object_id[0] ? ctx->session.selected_scene_object_id : "(unset)");
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
    y += m.line_h;
    (void)snprintf(line, sizeof(line), "ACTION %s", ctx->session.file_action_status_message);
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
}

void drawing_program_visual_render_right_export_tab(SDL_Renderer *renderer,
                                                    SDL_Rect rect,
                                                    const DrawingProgramAppContext *ctx,
                                                    const VisualPanelUiState *ui,
                                                    VisualPaneLayoutMetrics m,
                                                    VisualThemePalette p,
                                                    const DrawingProgramVisualPanelRenderHooks *hooks) {
    char line[160];
    char export_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    char texture_export_dir[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    int y = right_file_content_start_y(rect, m);
    SDL_Rect pick_output_root_button = right_file_route_action_button_rect(rect,
                                                                           m,
                                                                           VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT,
                                                                           0u,
                                                                           VISUAL_RIGHT_EXPORT_ACTION_COUNT);
    SDL_Rect export_intent_button = right_file_route_action_button_rect(rect,
                                                                        m,
                                                                        VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT,
                                                                        1u,
                                                                        VISUAL_RIGHT_EXPORT_ACTION_COUNT);
    SDL_Rect overlay_material_intent_button = right_file_route_action_button_rect(rect,
                                                                                  m,
                                                                                  VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT,
                                                                                  2u,
                                                                                  VISUAL_RIGHT_EXPORT_ACTION_COUNT);
    SDL_Rect export_png_button = right_file_route_action_button_rect(rect,
                                                                     m,
                                                                     VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT,
                                                                     3u,
                                                                     VISUAL_RIGHT_EXPORT_ACTION_COUNT);
    SDL_Rect export_textures_button = right_file_route_action_button_rect(rect,
                                                                          m,
                                                                          VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT,
                                                                          4u,
                                                                          VISUAL_RIGHT_EXPORT_ACTION_COUNT);
    SDL_Rect export_iconset_button = right_file_route_action_button_rect(rect,
                                                                         m,
                                                                         VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT,
                                                                         5u,
                                                                         VISUAL_RIGHT_EXPORT_ACTION_COUNT);
    SDL_Rect export_icns_button = right_file_route_action_button_rect(rect,
                                                                      m,
                                                                      VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT,
                                                                      6u,
                                                                      VISUAL_RIGHT_EXPORT_ACTION_COUNT);
    if (drawing_program_png_export_default_path(ctx, export_path, sizeof(export_path)).code != CORE_OK) {
        export_path[0] = '\0';
    }
    if (drawing_program_texture_export_default_directory(ctx, texture_export_dir, sizeof(texture_export_dir)).code !=
        CORE_OK) {
        texture_export_dir[0] = '\0';
    }

    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "EXPORT ACTIONS", p.text_primary, m.body_scale);
    visual_right_panel_draw_button(renderer,
                                   rect,
                                   pick_output_root_button,
                                   "PICK OUTPUT ROOT",
                                   p.text_primary,
                                   ui,
                                   m,
                                   p,
                                   hooks);
    visual_right_panel_draw_button(renderer,
                                   rect,
                                   export_intent_button,
                                   drawing_program_texture_export_intent_button_label(
                                       ctx->texture_project.export_intent_kind),
                                   p.text_primary,
                                   ui,
                                   m,
                                   p,
                                   hooks);
    visual_right_panel_draw_button(renderer,
                                   rect,
                                   overlay_material_intent_button,
                                   drawing_program_texture_overlay_material_intent_button_label(
                                       ctx->texture_project.overlay_material_intent_kind),
                                   p.text_primary,
                                   ui,
                                   m,
                                   p,
                                   hooks);
    visual_right_panel_draw_button(renderer, rect, export_png_button, "EXPORT PNG", p.text_primary, ui, m, p, hooks);
    visual_right_panel_draw_button(renderer,
                                   rect,
                                   export_textures_button,
                                   "EXPORT TEXTURES",
                                   p.text_primary,
                                   ui,
                                   m,
                                   p,
                                   hooks);
    visual_right_panel_draw_button(renderer,
                                   rect,
                                   export_iconset_button,
                                   "EXPORT ICONSET",
                                   p.text_primary,
                                   ui,
                                   m,
                                   p,
                                   hooks);
    visual_right_panel_draw_button(renderer, rect, export_icns_button, "EXPORT ICNS", p.text_primary, ui, m, p, hooks);

    y = right_file_state_start_y(rect, m, VISUAL_RIGHT_EXPORT_FOOTER_LINE_COUNT);
    visual_right_panel_format_path_line(line, sizeof(line), "OUTPUT", ctx->session.output_root_path);
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
    y += m.line_h;
    (void)snprintf(line,
                   sizeof(line),
                   "INTENT %s",
                   drawing_program_texture_export_intent_kind_name(ctx->texture_project.export_intent_kind));
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
    y += m.line_h;
    (void)snprintf(line,
                   sizeof(line),
                   "EMIT %s",
                   drawing_program_texture_export_intent_kind_name(
                       drawing_program_texture_export_intent_emitted_output_kind(
                           ctx->texture_project.export_intent_kind)));
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
    y += m.line_h;
    (void)snprintf(line,
                   sizeof(line),
                   "OVERLAY %s",
                   drawing_program_texture_overlay_material_intent_kind_name(
                       ctx->texture_project.overlay_material_intent_kind));
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
    y += m.line_h;
    visual_right_panel_format_path_line(line, sizeof(line), "PNG", export_path);
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
    y += m.line_h;
    visual_right_panel_format_path_line(line, sizeof(line), "TEXTURES", texture_export_dir);
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
    y += m.line_h;
    (void)snprintf(line, sizeof(line), "ACTION %s", ctx->session.file_action_status_message);
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
}
