#include "../src/ui/panel/drawing_program_ui_button.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_render_backend.h"
#include "drawing_program/drawing_program_ui_controls.h"
#include "drawing_program/drawing_program_visual_input_core.h"
#include "drawing_program/drawing_program_visual_text_render.h"
#include "drawing_program/drawing_program_visual_authoring_chrome.h"
#include "kit_ui_window_sdl.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c)                                                               \
  do {                                                                         \
    if (!(c)) {                                                                \
      fprintf(stderr, "UI contract failed line %d: %s\n", __LINE__, #c);       \
      return 1;                                                                \
    }                                                                          \
  } while (0)
static int text_x, text_y;
static int splitter_presentation_contract(DrawingProgramAppContext *ctx) {
  CHECK(drawing_program_pane_host_set_splitter_scale(ctx, 2.0f, 1.5f).code == CORE_OK);
  for (uint32_t i=0; i<ctx->pane_host.splitter_hit_count; ++i) {
    CorePaneSplitterHit hit=ctx->pane_host.splitter_hits[i], found;
    float x=hit.splitter_bounds.x+hit.splitter_bounds.width/2,
          y=hit.splitter_bounds.y+hit.splitter_bounds.height/2;
    CHECK(hit.axis==CORE_PANE_AXIS_HORIZONTAL ? hit.splitter_bounds.width==32 : hit.splitter_bounds.height==24);
    float dx=hit.axis==CORE_PANE_AXIS_HORIZONTAL ? 14 : 0,
          dy=hit.axis==CORE_PANE_AXIS_VERTICAL ? 10.5f : 0;
    CHECK(core_pane_hit_test_splitter_hits(&hit,1,x+dx,y+dy,&found));
    CHECK(core_pane_hit_test_splitter_hits(&hit,1,x-dx,y-dy,&found));
    CHECK(!core_pane_hit_test_splitter_hits(&hit,1,x+dx*2,y+dy*2,&found));
    CHECK(drawing_program_pane_host_update_pointer(ctx,x+dx,y+dy).code==CORE_OK);
    CorePaneRect line; int hovered,active;
    CHECK(drawing_program_pane_host_visible_splitter(ctx,&line,&hovered,&active));
    CHECK(hovered && !active);
    CHECK(hit.axis==CORE_PANE_AXIS_HORIZONTAL ? line.width==2 : line.height==2);
  }
  CHECK(drawing_program_pane_host_set_splitter_scale(ctx,1,1).code==CORE_OK);
  CorePaneSplitterHit hit=ctx->pane_host.splitter_hits[1];
  float x=hit.splitter_bounds.x+hit.splitter_bounds.width/2,
        y=hit.splitter_bounds.y+hit.splitter_bounds.height/2;
  CHECK(drawing_program_pane_host_begin_splitter_drag(ctx,x,y));
  CHECK(!drawing_program_authoring_host_active(ctx));
  CHECK(!drawing_program_authoring_host_pane_overlay_active(ctx));
  SDL_Surface *pixels=SDL_CreateRGBSurfaceWithFormat(0,100,80,32,SDL_PIXELFORMAT_ARGB8888);
  CHECK(pixels); SDL_Renderer *r=SDL_CreateSoftwareRenderer(pixels); CHECK(r);
  SDL_SetRenderDrawColor(r,1,2,3,255); SDL_RenderClear(r);
  CHECK(drawing_program_ui_controls_begin(ctx).code==CORE_OK);
  drawing_program_visual_authoring_chrome_draw(r,100,80,ctx,NULL);
  CHECK(drawing_program_ui_controls_end(r).code==CORE_OK);
  CHECK(drawing_program_ui_controls_snapshot()->count==0);
  Uint8 red,green,blue,alpha;
  SDL_GetRGBA(*(Uint32 *)((Uint8 *)pixels->pixels+10*pixels->pitch+10*4),pixels->format,&red,&green,&blue,&alpha);
  CHECK(red==1 && green==2 && blue==3); /* The HUD must not paint during a runtime drag. */
  SDL_DestroyRenderer(r); SDL_FreeSurface(pixels);
  drawing_program_pane_host_cancel_splitter_drag(ctx);
  /* Explicit authoring takeover cancels the pending resize before its baseline. */
  CoreLayoutState before=ctx->pane_host.layout_state;
  CorePaneNode nodes[DRAWING_PROGRAM_PANE_NODE_CAPACITY]; memcpy(nodes,ctx->pane_host.nodes,sizeof(nodes));
  CHECK(drawing_program_pane_host_begin_splitter_drag(ctx,x,y));
  CHECK(drawing_program_pane_host_update_splitter_drag(ctx,x+40,y+40));
  CHECK(drawing_program_authoring_host_enter(ctx).code==CORE_OK);
  CHECK(drawing_program_authoring_host_active(ctx) && !drawing_program_pane_host_splitter_drag_active(ctx));
  CHECK(!memcmp(nodes,ctx->pane_host.nodes,sizeof(nodes)));
  CHECK(drawing_program_authoring_host_cancel(ctx).code==CORE_OK);
  CHECK(!memcmp(&before,&ctx->pane_host.layout_state,sizeof(before)));
  return 0;
}
static int measure_text(const char *text, int scale) {
  (void)text;
  return 12 * scale;
}
static int draw_text(SDL_Renderer *renderer, SDL_Rect clip, int x, int y,
                     const char *text, SDL_Color color, int scale) {
  (void)renderer;
  (void)clip;
  (void)text;
  (void)color;
  (void)scale;
  text_x = x;
  text_y = y;
  return 12;
}
static int controls_contract(DrawingProgramAppContext *ctx) {
  SDL_Surface *pixels =
      SDL_CreateRGBSurfaceWithFormat(0, 100, 80, 32, SDL_PIXELFORMAT_ARGB8888);
  CHECK(pixels);
  SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(pixels);
  CHECK(renderer);
  SDL_Rect bounds = {10, 10, 60, 30}, clip = {0, 0, 100, 80},
           parent = {5, 5, 80, 60};
  SDL_SetRenderDrawColor(renderer, 2, 3, 4, 5);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
  SDL_RenderSetClipRect(renderer, &parent);
  DrawingProgramVisualPanelRenderHooks hooks = {0};
  hooks.measure_bitmap_text_width = measure_text;
  hooks.draw_bitmap_text = draw_text;
  KitUiButtonSpec spec;
  kit_ui_button_spec_init(&spec, "x");
  KitUiButtonTheme theme = {{20, 30, 40, 255},    {40, 50, 60, 255},
                            {30, 40, 50, 255},    {40, 50, 60, 255},
                            {100, 110, 120, 255}, {100, 110, 120, 255},
                            {255, 255, 255, 255}, {110, 110, 110, 255}};
  drawing_program_ui_controls_reset();
  CHECK(drawing_program_ui_controls_begin(ctx).code == CORE_OK);
  drawing_program_ui_controls_key(DRAWING_UI_RIGHT_PANEL_RENDER_TAB_CANVAS, 0,
                                  1);
  CHECK(!drawing_program_ui_button_draw_spec(renderer, clip, bounds, &spec,
                                             &theme, 1, &hooks));
  CHECK(drawing_program_ui_controls_end(renderer).code == CORE_OK);
  CHECK(text_x == 34 &&
        text_y == 10 + (30 - drawing_program_visual_text_line_height(1)) / 2);
  SDL_Rect restored;
  SDL_RenderGetClipRect(renderer, &restored);
  CHECK(!memcmp(&restored, &parent, sizeof(parent)));
  Uint8 a, b, c, d;
  SDL_BlendMode blend;
  SDL_GetRenderDrawColor(renderer, &a, &b, &c, &d);
  SDL_GetRenderDrawBlendMode(renderer, &blend);
  CHECK(a == 2 && b == 3 && c == 4 && d == 5 && blend == SDL_BLENDMODE_NONE);
  SDL_Event e = {0};
  e.type = SDL_MOUSEBUTTONDOWN;
  e.button.button = SDL_BUTTON_LEFT;
  KitUiSurfaceKey command;
  CHECK(drawing_program_ui_controls_route(ctx, &e, 20, 20, &command) &&
        !command.domain);
  e.type = SDL_MOUSEBUTTONUP;
  CHECK(drawing_program_ui_controls_route(ctx, &e, 90, 70, &command) &&
        !command.domain);
  e.type = SDL_MOUSEBUTTONDOWN;
  CHECK(drawing_program_ui_controls_route(ctx, &e, 20, 20, &command) &&
        !command.domain);
  e.type = SDL_MOUSEBUTTONUP;
  CHECK(drawing_program_ui_controls_route(ctx, &e, 20, 20, &command) &&
        command.domain == DRAWING_UI_RIGHT_PANEL_RENDER_TAB_CANVAS && command.value == 0);
  /* Modal takeover uses shared focus scope and excludes background controls. */
  CHECK(drawing_program_authoring_host_enter(ctx).code == CORE_OK);
  CHECK(drawing_program_ui_controls_begin(ctx).code == CORE_OK);
  drawing_program_ui_controls_key(DRAWING_UI_RIGHT_PANEL_RENDER_TAB_CANVAS, 0,
                                  1);
  CHECK(!drawing_program_ui_button_draw_spec(renderer, clip, bounds, &spec,
                                             &theme, 1, &hooks));
  drawing_program_ui_controls_key(DRAWING_UI_AUTHORING_ACTION, 3, 1);
  CHECK(!drawing_program_ui_button_draw_spec(renderer, clip, bounds, &spec,
                                             &theme, 1, &hooks));
  CHECK(drawing_program_ui_controls_end(renderer).code == CORE_OK);
  CHECK(drawing_program_ui_controls_snapshot()->count == 1);
  CHECK(drawing_program_authoring_host_cancel(ctx).code == CORE_OK);
  CHECK(drawing_program_ui_controls_begin(ctx).code == CORE_OK);
  drawing_program_ui_controls_key(DRAWING_UI_RIGHT_PANEL_RENDER_TAB_CANVAS, 0,
                                  1);
  CHECK(!drawing_program_ui_button_draw_spec(renderer, clip, bounds, &spec,
                                             &theme, 1, &hooks));
  CHECK(drawing_program_ui_controls_end(renderer).code == CORE_OK);
  CHECK(drawing_program_ui_controls_snapshot()->interaction.focused_id != 0);
  SDL_DestroyRenderer(renderer);
  SDL_FreeSurface(pixels);
  return 0;
}
int drawing_program_ui_contract_suite(void) {
  static DrawingProgramAppContext ctx;
  char *args[] = {"ui-contract", "--headless",   "--smoke-frames",
                  "1",           "--no-persist", NULL};
  CHECK(drawing_program_app_bootstrap(&ctx, 5, args).code == CORE_OK);
  CHECK(drawing_program_app_config_load(&ctx).code == CORE_OK);
  CHECK(drawing_program_app_state_seed(&ctx).code == CORE_OK);
  CHECK(drawing_program_app_subsystems_init(&ctx).code == CORE_OK);
  CHECK(drawing_program_runtime_start(&ctx).code == CORE_OK);
  CHECK(drawing_program_app_set_pane_host_bounds(&ctx, 1200, 800).code ==
        CORE_OK);
  CHECK(!splitter_presentation_contract(&ctx));
  KitUiWindowState window = {.logical_width = 2500, .logical_height = 720};
  int x, y;
  int rw, rh;
  CHECK(drawing_program_render_backend_canvas_extent(5000, 1440, &rw, &rh) &&
        rw == 4096 && rh == 1180);
  CHECK(kit_ui_window_map_point_sdl(&window, rw, rh, 1250, 360, &x, &y) &&
        x == 2048 && y == 590);
  CHECK(kit_ui_window_map_point_sdl(&window, rw, rh, -20, 725, &x, &y) &&
        x < 0 && y > rh);
  const Uint8 events[] = {
      SDL_WINDOWEVENT_RESIZED,   SDL_WINDOWEVENT_SIZE_CHANGED,
      SDL_WINDOWEVENT_MOVED,     SDL_WINDOWEVENT_DISPLAY_CHANGED,
      SDL_WINDOWEVENT_MINIMIZED, SDL_WINDOWEVENT_MAXIMIZED,
      SDL_WINDOWEVENT_RESTORED,  SDL_WINDOWEVENT_SHOWN,
      SDL_WINDOWEVENT_HIDDEN,    SDL_WINDOWEVENT_FOCUS_LOST};
  CorePaneNode before[DRAWING_PROGRAM_PANE_NODE_CAPACITY];
  for (unsigned i = 0; i < sizeof(events) / sizeof(events[0]); i++) {
    SDL_Event e = {0};
    e.type = SDL_WINDOWEVENT;
    e.window.event = events[i];
    Uint8 clear = 0, cancel = 0;
    drawing_program_visual_input_window_event_flags(&e, &clear, &cancel);
    CHECK(clear && cancel);
    CorePaneSplitterHit hit = ctx.pane_host.splitter_hits[1];
    float hx = hit.splitter_bounds.x + hit.splitter_bounds.width / 2,
          hy = hit.splitter_bounds.y + hit.splitter_bounds.height / 2;
    memcpy(before, ctx.pane_host.nodes, sizeof(before));
    CoreLayoutState revision = ctx.pane_host.layout_state;
    CHECK(drawing_program_pane_host_begin_splitter_drag(&ctx, hx, hy));
    CHECK(!drawing_program_authoring_host_active(&ctx));
    CHECK(
        drawing_program_pane_host_update_splitter_drag(&ctx, hx + 40, hy + 40));
    drawing_program_pane_host_cancel_splitter_drag(&ctx);
    CHECK(!memcmp(before, ctx.pane_host.nodes, sizeof(before)));
    CHECK(!memcmp(&revision, &ctx.pane_host.layout_state, sizeof(revision)));
    CHECK(!drawing_program_pane_host_splitter_drag_active(&ctx));
  }
  CorePaneSplitterHit hit = ctx.pane_host.splitter_hits[1];
  float hx = hit.splitter_bounds.x + hit.splitter_bounds.width / 2,
        hy = hit.splitter_bounds.y + hit.splitter_bounds.height / 2;
  uint64_t revision = ctx.pane_host.layout_state.active_revision;
  CHECK(drawing_program_pane_host_begin_splitter_drag(&ctx, hx, hy));
  drawing_program_pane_host_end_splitter_drag(&ctx);
  CHECK(ctx.pane_host.layout_state.active_revision == revision);
  CHECK(drawing_program_pane_host_begin_splitter_drag(&ctx, hx, hy));
  CHECK(drawing_program_pane_host_update_splitter_drag(&ctx, hx + 40, hy));
  drawing_program_pane_host_end_splitter_drag(&ctx);
  CHECK(ctx.pane_host.layout_state.active_revision == revision + 1);
  CHECK(drawing_program_pane_host_compose(&ctx, 0).code == CORE_OK);
  KitPaneHost *host = &ctx.pane_host.composition_host;
  CHECK(host->view.count == ctx.pane_host.leaf_count);
  const KitPaneCompositionEntry *pane = &host->view.entries[0];
  CHECK(kit_pane_host_pointer(host, KIT_PANE_HOST_POINTER_DOWN,
                              pane->shell.x + 2, pane->shell.y + 2, NULL,
                              NULL) == pane->id);
  CHECK(kit_pane_host_keyboard_owner(host) == pane->id);
  CHECK(drawing_program_pane_host_compose(&ctx, 1).code == CORE_OK);
  CHECK(!host->pointer.captured_id && !kit_pane_host_keyboard_owner(host));
  CHECK(drawing_program_authoring_host_enter(&ctx).code == CORE_OK);
  CoreLayoutState nested_before = ctx.pane_host.layout_state;
  KitPaneLayoutEdit nested = {0};
  CHECK(kit_pane_layout_edit_begin(&nested, &ctx.pane_host.layout_state));
  CHECK(kit_pane_layout_edit_update(&nested, &ctx.pane_host.layout_state, 1));
  CHECK(kit_pane_layout_edit_cancel(&nested, &ctx.pane_host.layout_state));
  CHECK(!memcmp(&nested_before, &ctx.pane_host.layout_state,
                sizeof(nested_before)));
  CHECK(drawing_program_authoring_host_cancel(&ctx).code == CORE_OK);
  CHECK(!controls_contract(&ctx));
  CHECK(drawing_program_app_shutdown(&ctx).code == CORE_OK);
  puts("Drawing UI window/pane production contracts passed");
  return 0;
}
