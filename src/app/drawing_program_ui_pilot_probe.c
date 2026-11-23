#include "drawing_program/drawing_program_visual_canvas_coords.h"
#include "drawing_program/drawing_program_ui_pilot_probe.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_indexed_editor.h"
#include "drawing_program/drawing_program_indexed_cell_board.h"
#include "drawing_program/drawing_program_render_backend.h"
#include "drawing_program/drawing_program_ui_controls.h"
#include "drawing_program/drawing_program_ui_color_state.h"
#include "drawing_program/drawing_program_visual_layout_color.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "kit_workspace_authoring_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const uint32_t tabs[] = {DRAWING_UI_RIGHT_PANEL_RENDER_TAB_CANVAS,
                                DRAWING_UI_RIGHT_PANEL_RENDER_TAB_LAYER,
                                DRAWING_UI_RIGHT_PANEL_RENDER_TAB_COLOR,
                                DRAWING_UI_RIGHT_PANEL_RENDER_TAB_FILE,
                                DRAWING_UI_RIGHT_PANEL_RENDER_TAB_ASSET,
                                DRAWING_UI_RIGHT_PANEL_RENDER_TAB_EXPORT};
static void key(SDL_Keycode sym, SDL_Keymod mod, int up) {
  SDL_Event e = {0};
  e.type = up ? SDL_KEYUP : SDL_KEYDOWN;
  e.key.keysym.sym = sym;
  e.key.keysym.scancode = SDL_GetScancodeFromKey(sym);
  e.key.keysym.mod = mod;
  SDL_PushEvent(&e);
}
static int click(SDL_Window *window, SDL_Renderer *renderer, uint32_t tag,
                 uint64_t identity, int up) {
  const KitUiSurface *s = drawing_program_ui_controls_snapshot();
  for (uint32_t i = 0; i < s->count; i++)
    if (s->keys[i].domain == tag && s->keys[i].value == identity &&
        s->controls[i].enabled) {
      int w, h, lw, lh;
      if (drawing_program_render_backend_output_size(renderer, &w, &h) != 0)
        return 0;
      SDL_GetWindowSize(window, &lw, &lh);
      KitRenderRect b = s->controls[i].bounds;
      SDL_Event e = {0};
      e.type = up ? SDL_MOUSEBUTTONUP : SDL_MOUSEBUTTONDOWN;
      e.button.button = SDL_BUTTON_LEFT;
      e.button.x = (int)((b.x + b.width / 2) * lw / w);
      e.button.y = (int)((b.y + b.height / 2) * lh / h);
      return SDL_PushEvent(&e) == 1;
    }
  return 0;
}
static int click_palette_swatch(SDL_Window *window, SDL_Renderer *renderer,
                                const DrawingProgramAppContext *app, uint8_t swatch) {
    SDL_Rect pane;
    int drawable_w, drawable_h, window_w, window_h;
    if (!drawing_program_visual_pane_rect_for_module_type(app, 4u, &pane) ||
        drawing_program_render_backend_output_size(renderer, &drawable_w, &drawable_h) != 0 ||
        drawable_w <= 0 || drawable_h <= 0) return 0;
    SDL_GetWindowSize(window, &window_w, &window_h);
    SDL_Rect rect = right_color_palette_swatch_rect(pane, make_pane_layout_metrics(app), swatch);
    SDL_Event event = {0};
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.windowID = SDL_GetWindowID(window);
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = (int)((double)(rect.x + rect.w / 2) * window_w / drawable_w);
    event.button.y = (int)((double)(rect.y + rect.h / 2) * window_h / drawable_h);
    if (SDL_PushEvent(&event) != 1) return 0;
    event.type = SDL_MOUSEBUTTONUP;
    return SDL_PushEvent(&event) == 1;
}

static int verify_object_color_escape(int step, SDL_Window *window, SDL_Renderer *renderer,
                                      DrawingProgramAppContext *app, const VisualPanelUiState *panel) {
    static uint32_t object_id, cursor;
    static DrawingProgramRasterSample original_stroke;
    if (step == 0) {
        DrawingProgramObjectRecord object = {
            .type = DRAWING_PROGRAM_OBJECT_TYPE_RECT, .layer_id = app->editor.active_layer_id,
            .origin_x = 8, .origin_y = 8, .width = 16, .height = 10, .visible = 1,
            .stroke_width = 1, .style_mode = 2,
            .stroke_color_value = drawing_program_color_value_from_index(1u),
            .fill_color_value = drawing_program_color_value_from_index(2u),
        };
        if (drawing_program_object_store_add(&app->object_store, &object, &object_id).code != CORE_OK)
            return 0;
        drawing_program_object_selection_replace_single(&app->object_selection, object_id);
        original_stroke = object.stroke_color_value;
        cursor = app->history.cursor;
        return 1;
    }
    const DrawingProgramObjectRecord *object = drawing_program_object_store_get_by_id(&app->object_store, object_id);
    if (!object) return 0;
    if (step == 1 || step == 2 || step == 3 || step == 6) {
        uint32_t domain = step == 1 ? DRAWING_UI_PANEL_RENDER_TAB_OBJECTS :
                          step == 2 ? DRAWING_UI_RIGHT_PANEL_RENDER_TAB_COLOR :
                                      DRAWING_UI_PANEL_RENDER_STROKE_COLOR_ROW;
        uint64_t identity = (step == 3 || step == 6) ? object_id : 0u;
        return click(window, renderer, domain, identity, 0) && click(window, renderer, domain, identity, 1);
    }
    if (step == 4) {
        if (panel->object_color_target_kind != VISUAL_OBJECT_COLOR_TARGET_STROKE ||
            panel->object_color_target_object_id != object_id) return 0;
        puts("UI_PILOT object-color Escape armed");
        key(SDLK_ESCAPE, KMOD_NONE, 0);
        SDL_Event repeat = {0};
        repeat.type = SDL_KEYDOWN;
        repeat.key.keysym.sym = SDLK_ESCAPE;
        repeat.key.repeat = 1;
        if (SDL_PushEvent(&repeat) != 1) return 0;
        key(SDLK_ESCAPE, KMOD_NONE, 1);
        return 1;
    }
    if (step == 5) {
        if (panel->object_color_target_kind != VISUAL_OBJECT_COLOR_TARGET_NONE ||
            panel->object_color_target_object_id || object->stroke_color_value != original_stroke ||
            app->history.cursor != cursor) return 0;
        return click_palette_swatch(window, renderer, app, 5u);
    }
    if (step == 7) {
        if (panel->object_color_target_object_id != object_id ||
            drawing_program_ui_color_active_paint_sample_value(app) == original_stroke ||
            object->stroke_color_value != original_stroke || app->history.cursor != cursor) return 0;
        return click_palette_swatch(window, renderer, app, 5u);
    }
    if (object->stroke_color_value == original_stroke || app->history.cursor != cursor + 1u) return 0;
    puts("UI_PILOT object-color Escape, stale target and fresh commit status=pass");
    return 1;
}

static int probe_pointer(SDL_Window *window, SDL_Renderer *renderer, Uint32 type, int x, int y) {
    int drawable_w, drawable_h, window_w, window_h;
    if (drawing_program_render_backend_output_size(renderer, &drawable_w, &drawable_h) != 0 ||
        drawable_w <= 0 || drawable_h <= 0) return 0;
    SDL_GetWindowSize(window, &window_w, &window_h);
    SDL_Event event = {0};
    event.type = type;
    event.button.windowID = SDL_GetWindowID(window);
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = (int)((double)x * window_w / drawable_w);
    event.button.y = (int)((double)y * window_h / drawable_h);
    return SDL_PushEvent(&event) == 1;
}

static int verify_layer_opacity(int step, SDL_Window *window, SDL_Renderer *renderer,
                                const DrawingProgramAppContext *app) {
    static uint32_t layer_id, layer_index, cursor;
    static uint8_t original_visible, opacity;
    static uint64_t opacity_revision;
    SDL_Rect pane;
    if (step == 0) {
        layer_id = app->editor.active_layer_id;
        if (app->ui.right_panel_slot != 1 ||
            drawing_program_document_layer_index_for_id(&app->document, layer_id, &layer_index).code != CORE_OK)
            return 0;
        original_visible = app->document.layers[layer_index].visible;
        cursor = app->history.cursor;
    }
    if (step <= 1) {
        if (app->editor.active_layer_id != layer_id || app->history.cursor != cursor ||
            !drawing_program_visual_pane_rect_for_module_type(app, 4u, &pane)) return 0;
        if (step == 1) {
            opacity = drawing_program_visual_layer_opacity_get(app, layer_id);
            opacity_revision = app->runtime.render_layer_opacity_revision;
            if (opacity < 24 || opacity > 26) return 0;
        }
        VisualPaneLayoutMetrics metrics = make_pane_layout_metrics(app);
        SDL_Rect row = right_layer_opacity_row_rect(pane, metrics, app->document.layer_count);
        SDL_Rect track = right_layer_opacity_track_rect(row, metrics);
        const int x = track.x + track.w / 4, y = row.y + row.h / 2;
        return probe_pointer(window, renderer, SDL_MOUSEBUTTONDOWN, x, y) &&
               probe_pointer(window, renderer, SDL_MOUSEBUTTONUP, x, y);
    }
    if (drawing_program_visual_layer_opacity_get(app, layer_id) != opacity) return 0;
    uint8_t persisted[DRAWING_PROGRAM_MAX_LAYERS] = {0};
    drawing_program_texture_project_collect_surface_layer_opacity_by_index(
        &app->texture_project, app->texture_project.active_surface_index,
        persisted, DRAWING_PROGRAM_MAX_LAYERS);
    if (persisted[layer_index] != opacity) return 0;
    if (step == 2) {
        if (app->history.cursor != cursor || app->runtime.render_layer_opacity_revision != opacity_revision)
            return 0;
        key(SDLK_v, KMOD_NONE, 0);
        key(SDLK_v, KMOD_NONE, 1);
    } else if (step == 3) {
        if (app->history.cursor != cursor + 1 || app->document.layers[layer_index].visible == original_visible)
            return 0;
        key(SDLK_z, KMOD_NONE, 0);
        key(SDLK_z, KMOD_NONE, 1);
    } else if (step == 4) {
        if (app->history.cursor != cursor || app->document.layers[layer_index].visible != original_visible)
            return 0;
        key(SDLK_y, KMOD_NONE, 0);
        key(SDLK_y, KMOD_NONE, 1);
    } else {
        if (app->history.cursor != cursor + 1 || app->document.layers[layer_index].visible == original_visible)
            return 0;
        puts("UI_PILOT Layer opacity no-op, active surface and visibility Undo/Redo status=pass");
    }
    return 1;
}

static int verify_canvas_escape(int step, SDL_Window *window, SDL_Renderer *renderer,
                                const DrawingProgramAppContext *app, const VisualCanvasInteractionState *canvas) {
    static uint32_t cursor;
    SDL_Rect pane;
    if (!drawing_program_visual_pane_rect_for_module_type(app, 1u, &pane)) return 0;
    VisualCanvasSheetMetrics sheet;
    drawing_program_visual_compute_canvas_sheet_metrics(app, pane, &sheet);
    const int x = sheet.sheet_rect.x + sheet.sheet_rect.w / 3;
    const int y = sheet.sheet_rect.y + sheet.sheet_rect.h / 3;
    const int release_x = x + sheet.sheet_rect.w / 8;
    const int release_y = y + sheet.sheet_rect.h / 8;
    if (step == 0) {
        cursor = app->history.cursor;
        key(SDLK_r, KMOD_NONE, 0);
        key(SDLK_r, KMOD_NONE, 1);
        return 1;
    }
    if (app->editor.active_tool != DRAWING_PROGRAM_TOOL_RECT) return 0;
    if (step == 1 || step == 4)
        return probe_pointer(window, renderer, SDL_MOUSEBUTTONDOWN, x, y);
    if (step == 2) {
        if (!canvas->shape_active || app->history.cursor != cursor) {
            fprintf(stderr, "UI_PILOT shape press rejected x=%d y=%d active=%u mode=%u cursor=%u expected=%u\n",
                    x, y, canvas->shape_active, app->ui.canvas_control_mode, app->history.cursor, cursor);
            return 0;
        }
        puts("UI_PILOT canvas shape Escape armed");
        key(SDLK_ESCAPE, KMOD_NONE, 0);
        SDL_Event repeat = {0};
        repeat.type = SDL_KEYDOWN;
        repeat.key.keysym.sym = SDLK_ESCAPE;
        repeat.key.repeat = 1;
        if (SDL_PushEvent(&repeat) != 1) return 0;
        key(SDLK_ESCAPE, KMOD_NONE, 1);
        return 1;
    }
    if (step == 3 || step == 5) {
        if (canvas->shape_active != (step == 5) || app->history.cursor != cursor) return 0;
        return probe_pointer(window, renderer, SDL_MOUSEBUTTONUP, release_x, release_y);
    }
    if (step == 6) {
        if (canvas->shape_active || app->history.cursor <= cursor) {
            fprintf(stderr, "UI_PILOT shape release active=%u cursor=%u expected_above=%u\n",
                    canvas->shape_active, app->history.cursor, cursor);
            return 0;
        }
        key(SDLK_z, KMOD_NONE, 0);
        key(SDLK_z, KMOD_NONE, 1);
        return 1;
    }
    if (app->history.cursor != cursor) return 0;
    puts("UI_PILOT canvas shape Escape, late release and fresh release Undo status=pass");
    return 1;
}

static int verify_indexed_file(int step, SDL_Window *window, SDL_Renderer *renderer,
                               const DrawingProgramAppContext *app) {
    static size_t sample_offset;
    static uint8_t original_sample, slot;
    static int sample_x, sample_y;
    const DrawingProgramTextureProject *project = &app->texture_project;
    if (!drawing_program_indexed_editor_is_active(app) || !project->indexed_raster.indices ||
        !app->session.preset_path_cli_override) return 0;
    switch (step) {
    case 0:
    case 1:
        return click(window, renderer, tabs[2], 0, step == 1);
    case 2: {
        SDL_Rect inspector, canvas_pane;
        DrawingProgramIndexedCellBoardCard card;
        if (app->ui.right_panel_slot != 2 || app->editor.active_tool != DRAWING_PROGRAM_TOOL_BRUSH ||
            project->indexed_profile.slot_count < 5 ||
            !drawing_program_visual_pane_rect_for_module_type(app, 4u, &inspector) ||
            !drawing_program_visual_pane_rect_for_module_type(app, 1u, &canvas_pane) ||
            !drawing_program_indexed_cell_board_layout(app, canvas_pane, &card, 1)) return 0;
        sample_x = card.content_rect.x + card.content_rect.w / 2;
        sample_y = card.content_rect.y + card.content_rect.h / 2;
        uint32_t x, y;
        if (!drawing_program_indexed_cell_board_screen_to_sample(app, canvas_pane, sample_x, sample_y, &x, &y))
            return 0;
        sample_offset = (size_t)y * project->indexed_raster.width + x;
        original_sample = project->indexed_raster.indices[sample_offset];
        slot = (original_sample == 4 || app->ui.indexed_selected_slot == 4) ? 3 : 4;
        if (original_sample == slot || project->indexed_profile.transparent_slot_index == slot) return 0;
        SDL_Rect swatch = right_color_indexed_slot_rect(inspector, make_pane_layout_metrics(app), slot);
        const int swatch_x = swatch.x + swatch.w / 2, swatch_y = swatch.y + swatch.h / 2;
        return probe_pointer(window, renderer, SDL_MOUSEBUTTONDOWN, swatch_x, swatch_y) &&
               probe_pointer(window, renderer, SDL_MOUSEBUTTONUP, swatch_x, swatch_y);
    }
    case 3:
        if (app->ui.indexed_selected_slot != slot || project->indexed_raster.indices[sample_offset] != original_sample)
            return 0;
        return probe_pointer(window, renderer, SDL_MOUSEBUTTONDOWN, sample_x, sample_y);
    case 4:
        if (project->indexed_raster.indices[sample_offset] != slot) return 0;
        return probe_pointer(window, renderer, SDL_MOUSEBUTTONUP, sample_x, sample_y);
    case 5:
        key(SDLK_z, KMOD_NONE, 0);
        key(SDLK_z, KMOD_NONE, 1);
        return 1;
    case 6:
        if (project->indexed_raster.indices[sample_offset] != original_sample) return 0;
        key(SDLK_y, KMOD_NONE, 0);
        key(SDLK_y, KMOD_NONE, 1);
        return 1;
    case 7:
    case 8:
        return project->indexed_raster.indices[sample_offset] == slot &&
               click(window, renderer, tabs[3], 0, step == 8);
    case 9:
    case 10:
        return app->ui.right_panel_slot == 3 &&
               click(window, renderer, DRAWING_UI_RIGHT_PANEL_FILE_TABS_RENDER_SAVE_SESSION_BUTTON, 0, step == 10);
    case 11:
        if (strcmp(app->session.file_action_status_message, "SESSION SAVED") != 0) return 0;
        key(SDLK_z, KMOD_NONE, 0);
        key(SDLK_z, KMOD_NONE, 1);
        return 1;
    case 12:
    case 13:
        return project->indexed_raster.indices[sample_offset] == original_sample &&
               click(window, renderer, DRAWING_UI_RIGHT_PANEL_FILE_TABS_RENDER_RELOAD_SESSION_BUTTON, 0, step == 13);
    case 14:
        if (strcmp(app->session.file_action_status_message, "SESSION RELOADED") != 0 ||
            project->indexed_raster.indices[sample_offset] != slot) return 0;
        puts("UI_PILOT indexed swatch, brush, Undo/Redo and File session save/reload status=pass");
        return 1;
    default:
        return 0;
    }
}

int drawing_program_ui_pilot_probe(SDL_Window *window, SDL_Renderer *renderer,
                                   DrawingProgramAppContext *app, const VisualPanelUiState *panel,
                                   const VisualCanvasInteractionState *canvas) {
  static int phase = 0;
  static uint64_t start = 0;
  static uint32_t pressed_scope = 0;
  static uint32_t layer_cursor, layer_id, layer_index;
  static uint8_t visible;
  const char *dir = getenv("DRAWING_PROGRAM_UI_PROOF");
  if (!dir || !*dir)
    return 0;
  struct stat output;
  if (stat(dir,&output)!=0 || !S_ISDIR(output.st_mode)) {
    fprintf(stderr,"UI_PILOT status=fail reason=output-directory\n");
    return -1;
  }
  if (!start)
    start = SDL_GetTicks64();
  if (SDL_GetTicks64() - start > 30000) {
    fprintf(stderr, "UI_PILOT status=timeout phase=%d\n", phase);
    return -1;
  }
  /* Let native startup show and focus events settle before arming the first press. */
  if (phase == 0 && SDL_GetTicks64() - start < 250) return 0;
  const char *proof_case = getenv("DRAWING_PROGRAM_UI_PROOF_CASE");
  if (proof_case && strcmp(proof_case, "closeout") == 0)
    return drawing_program_l5_closeout_probe(window, renderer, app, panel, canvas);
  if (proof_case && strcmp(proof_case, "canvas-transform") == 0)
    return drawing_program_canvas_transform_probe(window, renderer, app, canvas);
  if (proof_case && strcmp(proof_case, "canvas-escape") == 0) {
    if (phase == 8) {
      puts("UI_PILOT status=complete");
      return 1;
    }
    if (!verify_canvas_escape(phase, window, renderer, app, canvas)) {
      fprintf(stderr, "UI_PILOT canvas-escape status=fail phase=%d\n", phase);
      return -1;
    }
    phase++;
    return 0;
  }
  if (proof_case && strcmp(proof_case, "indexed-file") == 0) {
    if (phase == 15) {
      puts("UI_PILOT status=complete");
      return 1;
    }
    if (!verify_indexed_file(phase, window, renderer, app)) {
      fprintf(stderr, "UI_PILOT indexed-file status=fail phase=%d\n", phase);
      return -1;
    }
    phase++;
    return 0;
  }
  if (proof_case && strcmp(proof_case, "object-color") == 0) {
    if (phase == 9) {
      puts("UI_PILOT status=complete");
      return 1;
    }
    if (!verify_object_color_escape(phase, window, renderer, app, panel)) {
      fprintf(stderr, "UI_PILOT object-color status=fail phase=%d\n", phase);
      return -1;
    }
    phase++;
    return 0;
  }
  if (phase < 18) {
    int tab = phase / 3, step = phase % 3;
    int disabled = tab == 1 && drawing_program_indexed_editor_is_active(app);
    if (disabled) {
      const KitUiSurface *s = drawing_program_ui_controls_snapshot();
      for (uint32_t i = 0; i < s->count; i++)
        if (s->keys[i].domain == tabs[tab] && s->controls[i].enabled)
          return -1;
      if (step == 2)
        puts("UI_PILOT indexed-layer-disabled status=pass");
    } else if (step < 2) {
      if (step == 1) {
        const KitUiSurface *surface = drawing_program_ui_controls_snapshot();
        KitUiSurfaceKey captured = {0};
        if (!kit_ui_surface_key(surface, surface->interaction.captured_id, &captured) ||
            captured.domain != tabs[tab]) {
          fprintf(stderr, "UI_PILOT native tab press not captured tab=%d domain=%u\n", tab, captured.domain);
          return -1;
        }
      }
      if (!click(window, renderer, tabs[tab], 0, step == 1))
        return -1;
    } else {
      if (app->ui.right_panel_slot != tab) {
        fprintf(stderr, "UI_PILOT status=fail tab=%d actual=%u\n", tab,
                app->ui.right_panel_slot);
        return -1;
      }
      char path[4096];
      snprintf(path, sizeof(path), "%s/tab-%d.bmp", dir, tab);
      if (!drawing_program_render_backend_request_capture(renderer, path))
        return -1;
      printf("UI_PILOT tab=%d status=pass controls=%u\n", tab,
             drawing_program_ui_controls_snapshot()->count);
    }
  } else if (phase == 18) {
    /* Space/Enter press ownership in the real loop, followed by modality. */
    key(SDLK_TAB, KMOD_SHIFT, 0);
    key(SDLK_TAB, KMOD_SHIFT, 1);
  } else if (phase == 19) {
    if (!drawing_program_ui_controls_snapshot()->interaction.focused_id)
      return -1;
    key(SDLK_SPACE, KMOD_NONE, 0);
  } else if (phase == 20) {
    pressed_scope = app->ui.right_panel_slot;
    key(SDLK_SPACE, KMOD_NONE, 1);
  } else if (phase == 21) {
    if (pressed_scope != 5 || app->ui.right_panel_slot != 4)
      return -1;
    printf("UI_PILOT keyboard status=pass previous_tab=%u current_tab=%u\n",
           pressed_scope, app->ui.right_panel_slot);
    key(SDLK_c, KMOD_ALT, 0);
    key(SDLK_v, KMOD_ALT, 0);
    key(SDLK_c, KMOD_ALT, 1);
    key(SDLK_v, KMOD_ALT, 1);
  } else if (phase == 22) {
    if (!drawing_program_authoring_host_active(app))
      return -1;
    const KitUiSurface *s = drawing_program_ui_controls_snapshot();
    for (uint32_t i = 0; i < s->count; i++)
      if (s->keys[i].domain != DRAWING_UI_AUTHORING_ACTION &&
          s->keys[i].domain != DRAWING_UI_AUTHORING_FONT_THEME)
        return -1;
    char path[4096];
    snprintf(path, sizeof(path), "%s/authoring.bmp", dir);
    if (!drawing_program_render_backend_request_capture(renderer, path))
      return -1;
    if (!click(window, renderer, DRAWING_UI_AUTHORING_ACTION,
               KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_MODE, 0))
      return -1;
  } else if (phase == 23) {
    if (!click(window, renderer, DRAWING_UI_AUTHORING_ACTION,
               KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_MODE, 1))
      return -1;
  } else if (phase == 24) {
    if (!drawing_program_authoring_host_font_theme_overlay_active(app))
      return -1;
    char path[4096];
    snprintf(path, sizeof(path), "%s/font-theme.bmp", dir);
    if (!drawing_program_render_backend_request_capture(renderer, path))
      return -1;
    key(SDLK_ESCAPE, KMOD_NONE, 0);
    key(SDLK_ESCAPE, KMOD_NONE, 1);
  } else if (phase == 25) {
    if (drawing_program_authoring_host_active(app))
      return -1;
    printf("UI_PILOT modal-cancel status=pass controls=%u\n",
           drawing_program_ui_controls_snapshot()->count);
  } else if (phase >= 26 && phase <= 34 && !drawing_program_indexed_editor_is_active(app)) {
    if (phase == 28 || phase == 29 || phase == 31 || phase == 32) {
      char path[4096];
      snprintf(path, sizeof(path), "%s/layer-%d.bmp", dir, phase);
      if (!drawing_program_render_backend_request_capture(renderer, path)) return -1;
    }
    if (phase == 26) {
      /* Give the visibility proof an observable canvas mark on its disposable project. */
      int width, height;
      SDL_GetWindowSize(window, &width, &height);
      SDL_Event stroke = {0};
      stroke.button.button = SDL_BUTTON_LEFT;
      stroke.button.x = width / 2;
      stroke.button.y = height / 2;
      stroke.type = SDL_MOUSEBUTTONDOWN;
      if (SDL_PushEvent(&stroke) != 1) return -1;
      stroke.type = SDL_MOUSEBUTTONUP;
      if (SDL_PushEvent(&stroke) != 1) return -1;
    }
    if (phase == 26 || phase == 27) {
      if (!click(window, renderer, tabs[1], 0, phase == 27)) return -1;
    } else if (phase == 28) {
      layer_id = app->editor.active_layer_id;
      if (drawing_program_document_layer_index_for_id(&app->document, layer_id, &layer_index).code != CORE_OK)
        return -1;
      visible = app->document.layers[layer_index].visible;
      layer_cursor = app->history.cursor;
      key(SDLK_v, KMOD_NONE, 0);
      key(SDLK_v, KMOD_NONE, 1);
    } else if (phase == 29 || phase == 30) {
      if (app->document.layers[layer_index].visible == visible || app->history.cursor != layer_cursor + 1)
        return -1;
      if (!click(window, renderer, DRAWING_UI_RIGHT_PANEL_RENDER_ACTIVE_VISIBLE_VISIBLE_ON_VISIBLE_OFF,
                 layer_id, phase == 30)) return -1;
    } else if (phase == 31) {
      if (app->document.layers[layer_index].visible != visible || app->history.cursor != layer_cursor + 2)
        return -1;
      key(SDLK_z, KMOD_NONE, 0);
      key(SDLK_z, KMOD_NONE, 1);
    } else if (phase == 32) {
      if (app->document.layers[layer_index].visible == visible || app->history.cursor != layer_cursor + 1)
        return -1;
      const KitUiSurface *surface = drawing_program_ui_controls_snapshot();
      KitUiSurfaceKey focused;
      if (!kit_ui_surface_key(surface, surface->interaction.focused_id, &focused) ||
          focused.domain != DRAWING_UI_RIGHT_PANEL_RENDER_ACTIVE_VISIBLE_VISIBLE_ON_VISIBLE_OFF ||
          focused.value != layer_id) return -1;
      key(SDLK_RETURN, KMOD_NONE, 0);
    } else if (phase == 33) {
      key(SDLK_RETURN, KMOD_NONE, 1);
    } else {
      if (app->document.layers[layer_index].visible != visible || app->history.cursor != layer_cursor + 2)
        return -1;
      puts("UI_PILOT visibility keyboard/pointer/undo/focused-enter status=pass");
    }
  } else if (phase >= 35 && phase <= 40 && !drawing_program_indexed_editor_is_active(app)) {
    if (!verify_layer_opacity(phase - 35, window, renderer, app)) {
      fprintf(stderr, "UI_PILOT layer-opacity status=fail phase=%d\n", phase);
      return -1;
    }
  } else {
    puts("UI_PILOT status=complete");
    return 1;
  }
  phase++;
  return 0;
}
