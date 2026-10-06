#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_ui_controls.h"
#include "drawing_program/drawing_program_visual_frame_render.h"
#include "drawing_program/drawing_program_visual_input_core.h"
#include "drawing_program/drawing_program_visual_input_workspace_view.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_visual_pane_geometry.h"
#include "drawing_program/drawing_program_visual_pane_header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { \
    fprintf(stderr, "Pane header contract line %d: %s\n", __LINE__, #c); \
    return 1; } } while (0)

static SDL_Rect observed[5];
static void paint_content(SDL_Renderer *r, SDL_Rect content,
    const DrawingProgramAppContext *ctx, const CoreThemePreset *theme, uint32_t module) {
    (void)ctx; (void)theme;
    observed[module] = content;
    /* Deliberately overdraw: the production frame must confine this callback. */
    SDL_SetRenderDrawColor(r, 249, 1, 249, 255);
    SDL_RenderFillRect(r, NULL);
}
static void menu(SDL_Renderer *r, SDL_Rect p, const DrawingProgramAppContext *c,
                 const CoreThemePreset *t) { paint_content(r,p,c,t,3); }
static void left(SDL_Renderer *r, SDL_Rect p, const DrawingProgramAppContext *c,
                 const CoreThemePreset *t, const VisualPanelUiState *u) {
    (void)u; paint_content(r,p,c,t,2);
}
static void right(SDL_Renderer *r, SDL_Rect p, const DrawingProgramAppContext *c,
    const CoreThemePreset *t, const VisualPanelUiState *u,
    const VisualSelectionState *s, const VisualCanvasInteractionState *i) {
    (void)u; (void)s; (void)i; paint_content(r,p,c,t,4);
}
static void canvas(SDL_Renderer *r, SDL_Rect p, const DrawingProgramAppContext *c,
    const CoreThemePreset *t, const VisualSelectionState *s,
    const VisualPanelUiState *u, const VisualCanvasInteractionState *i) {
    (void)s; (void)u; (void)i; paint_content(r,p,c,t,1);
}
static void readout(SDL_Renderer *r, SDL_Rect p, const DrawingProgramAppContext *c,
    const CoreThemePreset *t) { (void)r; (void)p; (void)c; (void)t; }

static int render_contract(DrawingProgramAppContext *ctx, SDL_Window *window,
                           SDL_Renderer *renderer, SDL_Surface *pixels) {
    static const DrawingProgramVisualFrameRenderHooks hooks = {
        .module_type_for_pane = drawing_program_visual_module_type_for_pane,
        .draw_menu_bar_chrome = menu, .draw_left_panel_content = left,
        .draw_right_panel_content = right, .draw_canvas_world_view = canvas,
        .draw_canvas_content_readout = readout
    };
    CoreThemePreset theme;
    CHECK(core_theme_get_preset(CORE_THEME_PRESET_DARK_DEFAULT,&theme).code == CORE_OK);
    VisualPanelUiState ui = {0};
    VisualSelectionState selection = {0};
    VisualCanvasInteractionState interaction = {0};
    CHECK(drawing_program_ui_controls_begin(ctx).code == CORE_OK);
    CHECK(drawing_program_visual_draw_frame(window,renderer,ctx,&theme,&ui,
                                           &selection,&interaction,&hooks));
    CHECK(drawing_program_ui_controls_end(renderer).code == CORE_OK);
    SDL_RenderPresent(renderer);
    uint32_t eligible_slots=0;
    for (uint32_t n=0; n<ctx->pane_host.composition_host.view.count; ++n) {
        KitPaneHeaderLayout layout;
        CHECK(drawing_program_visual_pane_header_layout(ctx,
            &ctx->pane_host.composition_host.view.entries[n],&layout).code==CORE_OK);
        eligible_slots+=layout.count;
    }
    CHECK(drawing_program_ui_controls_snapshot()->count == eligible_slots);
    for (uint32_t n=0; n<ctx->pane_host.composition_host.view.count; ++n) {
        const KitPaneCompositionEntry *p=&ctx->pane_host.composition_host.view.entries[n];
        uint32_t module=drawing_program_visual_module_type_for_pane(ctx,p->id);
        SDL_Rect input;
        CHECK(drawing_program_visual_pane_rect_for_module_type(ctx,module,&input));
        CHECK(!memcmp(&input,&observed[module],sizeof(input)));
        if (module==3) { CHECK(p->header.height==0); continue; }
        CHECK(input.y >= p->header.y+p->header.height);
        int x=(int)(p->header.x+p->header.width/2), y=(int)(p->header.y+p->header.height)-2;
        Uint32 pixel=*(Uint32 *)((Uint8 *)pixels->pixels+y*pixels->pitch+x*4);
        Uint8 r,g,b,a; SDL_GetRGBA(pixel,pixels->format,&r,&g,&b,&a);
        CHECK(r!=249 || b!=249 || g!=1);
        KitPaneHit hit=kit_pane_composition_hit(&ctx->pane_host.composition_host.view,(float)x,(float)y);
        CHECK(hit.id==p->id && hit.region==KIT_PANE_REGION_HEADER);
        DrawingProgramVisualPaneHitState domain=drawing_program_visual_input_classify_hit(
            x,y,module==2,input,module==4,input,module==1,input);
        CHECK(!domain.on_left && !domain.on_right && !domain.on_canvas);
        if (module==2 || module==4) {
            VisualPaneLayoutMetrics m=make_pane_layout_metrics(ctx);
            SDL_Rect tab=module==2 ? left_panel_slot_tab_rect(input,m,0,2) :
                right_panel_slot_tab_rect(input,m,0,6);
            CHECK(tab.y==input.y); /* No duplicate header offset. */
        }
    }
    return 0;
}

static int activation_contract(DrawingProgramAppContext *ctx, SDL_Renderer *renderer) {
    const KitUiSurface *surface=drawing_program_ui_controls_snapshot();
    uint32_t index=0;
    while (index<surface->count && surface->keys[index].domain!=DRAWING_UI_PANE_HEADER_FIT) ++index;
    CHECK(index<surface->count);
    KitUiSurfaceKey fit=surface->keys[index];
    KitRenderRect b=surface->controls[index].bounds;
    SDL_Event e={0}; e.type=SDL_MOUSEBUTTONDOWN; e.button.button=SDL_BUTTON_LEFT;
    int x=(int)(b.x+b.width/2), y=(int)(b.y+b.height/2),ax,ay,activated;
    CHECK(drawing_program_ui_controls_header_at(x,y));
    CHECK(!drawing_program_ui_controls_header_at(-1,-1));
    CHECK(drawing_program_ui_controls_route(ctx,&e,x,y,&ax,&ay,&activated) && !activated);
    e.type=SDL_MOUSEBUTTONUP;
    CHECK(drawing_program_ui_controls_route(ctx,&e,-1,-1,&ax,&ay,&activated) && !activated);
    e.type=SDL_MOUSEBUTTONDOWN;
    CHECK(drawing_program_ui_controls_route(ctx,&e,x,y,&ax,&ay,&activated) && !activated);
    e.type=SDL_MOUSEBUTTONUP;
    CHECK(drawing_program_ui_controls_route(ctx,&e,x,y,&ax,&ay,&activated) && activated);
    KitUiSurfaceKey actual=drawing_program_ui_controls_last_activation();
    CHECK(actual.domain==fit.domain && actual.value==fit.value);
    static DrawingProgramAppContext expected;
    ctx->editor.viewport.pan_x=777.0f; ctx->editor.viewport.pan_y=-555.0f;
    expected=*ctx;
    (void)drawing_program_visual_input_workspace_view_fit_all_or_reset(&expected);
    CoreLayoutState revision=ctx->pane_host.layout_state;
    CHECK(drawing_program_visual_pane_header_action(ctx,actual));
    CHECK(!memcmp(&expected.editor.viewport,&ctx->editor.viewport,sizeof(ctx->editor.viewport)));
    CHECK(!memcmp(&revision,&ctx->pane_host.layout_state,sizeof(revision)));

    index=0;
    while (index<surface->count && surface->keys[index].domain!=DRAWING_UI_PANE_HEADER_LAYOUT) ++index;
    CHECK(index<surface->count);
    KitUiSurfaceKey layout=surface->keys[index];
    CHECK(drawing_program_visual_pane_header_action(ctx,layout));
    CHECK(drawing_program_authoring_host_active(ctx));
    CHECK(drawing_program_pane_host_compose(ctx,1).code==CORE_OK);
    CHECK(drawing_program_ui_controls_begin(ctx).code==CORE_OK);
    for (uint32_t i=0; i<ctx->pane_host.composition_host.view.count; ++i)
        CHECK(drawing_program_visual_pane_header_draw(renderer,ctx,
              &ctx->pane_host.composition_host.view.entries[i],NULL));
    CHECK(drawing_program_ui_controls_end(renderer).code==CORE_OK);
    CHECK(drawing_program_ui_controls_snapshot()->count==0);
    CHECK(drawing_program_authoring_host_cancel(ctx).code==CORE_OK);
    CHECK(!memcmp(&revision,&ctx->pane_host.layout_state,sizeof(revision)));
    CHECK(drawing_program_pane_host_compose(ctx,0).code==CORE_OK);

    KitPaneCompositionEntry *pane=NULL;
    for (uint32_t i=0; i<ctx->pane_host.composition_host.view.count; ++i)
        if (ctx->pane_host.composition_host.view.entries[i].id==layout.value)
            pane=&ctx->pane_host.composition_host.view.entries[i];
    CHECK(pane);
    KitPaneCompositionEntry saved=*pane;
    pane->header.width=30; pane->visible_header.width=30;
    KitPaneHeaderLayout slots;
    CHECK(drawing_program_visual_pane_header_layout(ctx,pane,&slots).code==CORE_OK && slots.count==0);
    CHECK(drawing_program_ui_controls_begin(ctx).code==CORE_OK);
    CHECK(drawing_program_visual_pane_header_draw(renderer,ctx,pane,NULL));
    CHECK(drawing_program_ui_controls_end(renderer).code==CORE_OK);
    CHECK(drawing_program_ui_controls_snapshot()->count==0);
    CHECK(drawing_program_visual_pane_header_action(ctx,layout));
    CHECK(!drawing_program_authoring_host_active(ctx));
    *pane=saved; pane->enabled=0;
    CHECK(drawing_program_ui_controls_begin(ctx).code==CORE_OK);
    CHECK(drawing_program_visual_pane_header_draw(renderer,ctx,pane,NULL));
    CHECK(drawing_program_ui_controls_end(renderer).code==CORE_OK);
    CHECK(drawing_program_ui_controls_snapshot()->count==1 &&
          !drawing_program_ui_controls_snapshot()->controls[0].enabled);
    CHECK(drawing_program_visual_pane_header_action(ctx,layout));
    CHECK(!drawing_program_authoring_host_active(ctx));
    *pane=saved; pane->visible_header.width=0;
    CHECK(drawing_program_ui_controls_begin(ctx).code==CORE_OK);
    CHECK(drawing_program_visual_pane_header_draw(renderer,ctx,pane,NULL));
    CHECK(drawing_program_ui_controls_end(renderer).code==CORE_OK);
    CHECK(drawing_program_ui_controls_snapshot()->count==0);
    CHECK(drawing_program_visual_pane_header_action(ctx,layout));
    CHECK(!drawing_program_authoring_host_active(ctx));
    *pane=saved;
    CHECK(drawing_program_visual_pane_header_action(ctx,
        (KitUiSurfaceKey){DRAWING_UI_PANE_HEADER_LAYOUT,999999}));
    CHECK(!drawing_program_authoring_host_active(ctx));
    CHECK(!drawing_program_visual_pane_header_action(ctx,
        (KitUiSurfaceKey){DRAWING_UI_RIGHT_PANEL_RENDER_TAB_CANVAS,0}));
    DrawingProgramViewportState before_view=ctx->editor.viewport;
    ctx->editor.viewport.pan_x=111.0f;
    CHECK(drawing_program_visual_set_module_type_for_pane(ctx,(uint32_t)fit.value,2));
    CHECK(drawing_program_pane_host_compose(ctx,0).code==CORE_OK);
    CHECK(drawing_program_visual_pane_header_action(ctx,fit));
    CHECK(ctx->editor.viewport.pan_x==111.0f);
    CHECK(drawing_program_visual_set_module_type_for_pane(ctx,(uint32_t)fit.value,1));
    CHECK(drawing_program_pane_host_compose(ctx,0).code==CORE_OK);
    CHECK(drawing_program_visual_pane_header_action(ctx,
        (KitUiSurfaceKey){DRAWING_UI_PANE_HEADER_FIT,fit.value+(1ULL<<32)}));
    CHECK(ctx->editor.viewport.pan_x==111.0f);
    ctx->editor.viewport=before_view;
    return 0;
}

int drawing_program_pane_header_suite(void) {
    static DrawingProgramAppContext ctx;
    char *args[]={"pane-header","--headless","--smoke-frames","1","--no-persist",NULL};
    CHECK(drawing_program_app_bootstrap(&ctx,5,args).code==CORE_OK);
    CHECK(drawing_program_app_config_load(&ctx).code==CORE_OK);
    CHECK(drawing_program_app_state_seed(&ctx).code==CORE_OK);
    CHECK(drawing_program_app_subsystems_init(&ctx).code==CORE_OK);
    CHECK(drawing_program_runtime_start(&ctx).code==CORE_OK);
    CHECK(drawing_program_app_set_pane_host_bounds(&ctx,1200,800).code==CORE_OK);
    CHECK(drawing_program_pane_host_compose(&ctx,0).code==CORE_OK);
    /* Dummy video keeps the production frame test contained and window-free. */
    const char *prior=SDL_getenv("SDL_VIDEODRIVER");
    char *saved_driver=prior ? SDL_strdup(prior) : NULL;
    int video_owned=!SDL_WasInit(SDL_INIT_VIDEO);
    if (video_owned) SDL_setenv("SDL_VIDEODRIVER","dummy",1);
    CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO)==0);
    SDL_Window *window=SDL_CreateWindow("pane-header-test",0,0,1200,800,SDL_WINDOW_HIDDEN);
    SDL_Surface *pixels=SDL_CreateRGBSurfaceWithFormat(0,1200,800,32,SDL_PIXELFORMAT_ARGB8888);
    CHECK(window && pixels);
    SDL_Renderer *renderer=SDL_CreateSoftwareRenderer(pixels);
    CHECK(renderer);
    drawing_program_ui_controls_reset();
    CHECK(!render_contract(&ctx,window,renderer,pixels));
    CHECK(!activation_contract(&ctx,renderer));
    CHECK(drawing_program_app_set_pane_host_bounds(&ctx,1200,800).code==CORE_OK);
    ctx.ui.font_zoom_step=2;
    CHECK(drawing_program_pane_host_compose(&ctx,0).code==CORE_OK);
    CHECK(!render_contract(&ctx,window,renderer,pixels));
    CorePaneLeafRect tiny={99999,{1.25f,2.25f,20.5f,0.0f}};
    KitPaneCompositionEntry empty;
    CHECK(drawing_program_visual_pane_entry(&ctx,&tiny,&empty).code==CORE_OK);
    SDL_Rect no_pixels=drawing_program_visual_pane_pixel_rect(empty.content);
    CHECK(no_pixels.h==0);
    SDL_DestroyRenderer(renderer); SDL_FreeSurface(pixels); SDL_DestroyWindow(window);
    if (video_owned) SDL_QuitSubSystem(SDL_INIT_VIDEO);
    if (saved_driver) { SDL_setenv("SDL_VIDEODRIVER",saved_driver,1); SDL_free(saved_driver); }
    else unsetenv("SDL_VIDEODRIVER");
    drawing_program_ui_controls_reset();
    CHECK(drawing_program_app_shutdown(&ctx).code==CORE_OK);
    puts("Drawing pane header/content production contracts passed");
    return 0;
}
