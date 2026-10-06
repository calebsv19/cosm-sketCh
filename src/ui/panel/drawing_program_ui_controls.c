#include "drawing_program/drawing_program_ui_controls.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "kit_ui_focus_scope.h"
#include "kit_ui_interaction_sdl.h"
#include <string.h>

static KitUiSurface surface;
static KitUiFocusScope focus;
static KitUiSurfaceKey pending;
static int pending_enabled;
static int geometry_valid;
static int modal_collecting;
static uint32_t controls_scope(const DrawingProgramAppContext *app) {
  return drawing_program_authoring_host_active(app) ? 2u : 1u;
}
void drawing_program_ui_controls_reset(void) {
  kit_ui_surface_reset(&surface);
  memset(&focus, 0, sizeof(focus));
  pending = (KitUiSurfaceKey){0};
  geometry_valid = 0;
}
CoreResult
drawing_program_ui_controls_begin(const DrawingProgramAppContext *app) {
  CoreResult r =
      kit_ui_focus_scope_sync(&focus, &surface, controls_scope(app),
                              drawing_program_authoring_host_active(app));
  if (r.code != CORE_OK)
    return r;
  modal_collecting = drawing_program_authoring_host_active(app);
  kit_ui_surface_begin(&surface, controls_scope(app));
  pending = (KitUiSurfaceKey){0};
  return r;
}
void drawing_program_ui_controls_key(uint32_t op, uint64_t identity,
                                     int enabled) {
  pending = (KitUiSurfaceKey){op, identity};
  pending_enabled = !!enabled;
}
uint64_t drawing_program_ui_controls_string_id(const char *s) {
  /* Stable domain strings; 64-bit hash collisions are caught as duplicate
   * semantic keys by kit_ui_surface, never resolved with a row offset. */
  uint64_t hash = 14695981039346656037ULL;
  if (s)
    for (; *s; s++) {
      hash ^= (unsigned char)*s;
      hash *= 1099511628211ULL;
    }
  return hash;
}
void drawing_program_ui_controls_button(SDL_Renderer *renderer, SDL_Rect clip,
                                        SDL_Rect rect, KitUiButtonSpec *spec) {
  if (!pending.domain)
    return;
  KitUiSurfaceKey key = pending;
  pending = (KitUiSurfaceKey){0};
  if (!surface.collecting ||
      (modal_collecting && key.domain != DRAWING_UI_AUTHORING_ACTION &&
       key.domain != DRAWING_UI_AUTHORING_FONT_THEME))
    return;
  SDL_Rect visible = clip;
  if (SDL_RenderIsClipEnabled(renderer)) {
    SDL_Rect parent;
    SDL_RenderGetClipRect(renderer, &parent);
    if (!SDL_IntersectRect(&clip, &parent, &visible))
      return;
  }
  if (visible.w <= 0 || visible.h <= 0 || rect.w <= 0 || rect.h <= 0)
    return;
  KitRenderRect bounds = {(float)rect.x, (float)rect.y, (float)rect.w,
                          (float)rect.h};
  KitRenderRect clipped = {(float)visible.x, (float)visible.y, (float)visible.w,
                           (float)visible.h};
  KitUiInteractionControl c;
  if (kit_ui_surface_register(&surface, key, bounds, &clipped,
                              pending_enabled && !spec->state.disabled, &c)
          .code != CORE_OK)
    return;
  int selected = spec->state.selected,
      disabled = spec->state.disabled || !pending_enabled;
  spec->state =
      kit_ui_interaction_button_state(&surface.interaction, &c, selected);
  spec->state.disabled = disabled;
}
CoreResult drawing_program_ui_controls_end(SDL_Renderer *renderer) {
  CoreResult r = kit_ui_surface_end(&surface);
  if (r.code != CORE_OK)
    return r;
  geometry_valid = 1;
  kit_ui_focus_scope_restore(&focus, &surface);
  kit_ui_interaction_sdl_draw_focus(renderer, &surface.interaction,
                                    surface.controls, surface.count,
                                    (KitRenderColor){170, 200, 255, 255});
  return r;
}
void drawing_program_ui_controls_invalidate(void) {
  KitUiInteractionEvent e = {.type = KIT_UI_INTERACTION_CANCEL};
  KitUiInteractionResult result;
  (void)kit_ui_surface_route(&surface, &e, &result);
  geometry_valid = 0;
  surface.activation_count = 0;
}
int drawing_program_ui_controls_route(const DrawingProgramAppContext *app,
                                      const SDL_Event *event, int x, int y,
                                      int *ax, int *ay, int *activated) {
  *activated = 0;
  if (controls_scope(app) != surface.scope) {
    (void)kit_ui_focus_scope_sync(&focus, &surface, controls_scope(app),
                                  drawing_program_authoring_host_active(app));
    return event->type == SDL_MOUSEBUTTONUP || event->type == SDL_KEYUP;
  }
  if (!geometry_valid)
    return event->type == SDL_MOUSEBUTTONDOWN ||
           event->type == SDL_MOUSEBUTTONUP || event->type == SDL_KEYUP;
  KitUiInteractionEvent e;
  KitUiInteractionResult result;
  if (!kit_ui_interaction_event_from_sdl(event, &e))
    return 0;
  if (e.type == KIT_UI_INTERACTION_POINTER_MOVE ||
      e.type == KIT_UI_INTERACTION_POINTER_DOWN ||
      e.type == KIT_UI_INTERACTION_POINTER_UP) {
    e.x = (float)x;
    e.y = (float)y;
  }
  surface.claimed = 0;
  if (kit_ui_surface_route(&surface, &e, &result).code != CORE_OK)
    return 1;
  if (result.activated_id &&
      kit_ui_surface_take_activation(&surface, result.activated_id)) {
    for (uint32_t i = 0; i < surface.count; i++)
      if (surface.controls[i].id == result.activated_id) {
        KitRenderRect b = surface.controls[i].bounds;
        *ax = (int)(b.x + b.width / 2);
        *ay = (int)(b.y + b.height / 2);
        *activated = 1;
        break;
      }
  }
  if (*activated)
    geometry_valid = 0;
  return result.consumed;
}

const KitUiSurface *drawing_program_ui_controls_snapshot(void) {
  return &surface;
}
