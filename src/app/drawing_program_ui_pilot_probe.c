#include "drawing_program/drawing_program_ui_pilot_probe.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_indexed_editor.h"
#include "drawing_program/drawing_program_render_backend.h"
#include "drawing_program/drawing_program_ui_controls.h"
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
int drawing_program_ui_pilot_probe(SDL_Window *window, SDL_Renderer *renderer,
                                   const DrawingProgramAppContext *app) {
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
  } else {
    puts("UI_PILOT status=complete");
    return 1;
  }
  phase++;
  return 0;
}
