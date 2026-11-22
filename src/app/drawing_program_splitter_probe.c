#include "drawing_program/drawing_program_ui_pilot_probe.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_render_backend.h"
#include "drawing_program/drawing_program_ui_controls.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Opt-in finite actual-loop proof; never runs during normal editing. */
static int pointer(SDL_Window *window, SDL_Renderer *renderer, Uint32 type,
                   float x, float y) {
    int w,h,lw,lh;
    if (drawing_program_render_backend_output_size(renderer,&w,&h)!=0 || w<=0 || h<=0) return 0;
    SDL_GetWindowSize(window,&lw,&lh);
    SDL_Event e={0}; e.type=type;
    if (type==SDL_MOUSEMOTION) { e.motion.x=(int)(x*lw/w); e.motion.y=(int)(y*lh/h); e.motion.state=SDL_BUTTON_LMASK; }
    else { e.button.button=SDL_BUTTON_LEFT; e.button.x=(int)(x*lw/w); e.button.y=(int)(y*lh/h); }
    return SDL_PushEvent(&e)==1;
}
static void key(SDL_Keycode sym, Uint16 mod) {
    SDL_Event e={0}; e.type=SDL_KEYDOWN; e.key.keysym.sym=sym; e.key.keysym.mod=mod; SDL_PushEvent(&e);
    e.type=SDL_KEYUP; SDL_PushEvent(&e);
}
static int escape(Uint32 type, Uint8 repeat) {
    SDL_Event event = {0};
    event.type = type;
    event.key.keysym.sym = SDLK_ESCAPE;
    event.key.keysym.scancode = SDL_SCANCODE_ESCAPE;
    event.key.repeat = repeat;
    return SDL_PushEvent(&event) == 1;
}
static int capture(SDL_Renderer *r,const char *dir,const char *name) {
    char path[4096];
    if (snprintf(path,sizeof(path),"%s/%s.bmp",dir,name)>=(int)sizeof(path)) return 0;
    return drawing_program_render_backend_request_capture(r,path);
}
static const CorePaneSplitterHit *left_divider(const DrawingProgramAppContext *app) {
    const CorePaneSplitterHit *hit=NULL;
    for (uint32_t i=0;i<app->pane_host.splitter_hit_count;++i) {
        const CorePaneSplitterHit *h=&app->pane_host.splitter_hits[i];
        if (h->axis==CORE_PANE_AXIS_HORIZONTAL && (!hit || h->splitter_bounds.x<hit->splitter_bounds.x)) hit=h;
    }
    return hit;
}
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"SPLITTER_PROOF status=fail phase=%d condition=%s\n",phase,#c); return -1; } } while (0)
int drawing_program_splitter_probe(SDL_Window *window, SDL_Renderer *renderer,
                                   const DrawingProgramAppContext *app) {
    const char *dir=getenv("DRAWING_PROGRAM_SPLITTER_PROOF"); if (!dir || !*dir) return 0;
    static int phase, hover_attempts, resized_width, resized_height;
    static uint64_t start; static float x,y,moved_x;
    static CoreLayoutState before; static CorePaneNode nodes[DRAWING_PROGRAM_PANE_NODE_CAPACITY];
    struct stat st; CHECK(stat(dir,&st)==0 && S_ISDIR(st.st_mode));
    if (!start) start=SDL_GetTicks64(); CHECK(SDL_GetTicks64()-start<30000);
    /* Initial shown/focus/drawable notifications legitimately cancel hover.
     * Begin the input proof only after the newly launched window settles. */
    if (phase==0 && SDL_GetTicks64()-start<250) return 0;
    int active=drawing_program_pane_host_splitter_drag_active(app);
    if (phase == 13)
        CHECK(!strstr(SDL_GetWindowTitle(window), "[Authoring]"));
    if (phase == 17 || phase == 20 || phase == 24) {
        CHECK(!active && !drawing_program_authoring_host_active(app));
        CHECK(!memcmp(nodes, app->pane_host.nodes, sizeof(nodes)));
        CHECK(!memcmp(&before, &app->pane_host.layout_state, sizeof(before)));
    }
    if (phase>=2 && phase<=4) {
        CHECK(active && !drawing_program_authoring_host_active(app));
        CHECK(!strstr(SDL_GetWindowTitle(window), "[Authoring]"));
        const KitUiSurface *s=drawing_program_ui_controls_snapshot();
        for (uint32_t i=0;i<s->count;++i)
            CHECK(s->keys[i].domain!=DRAWING_UI_AUTHORING_ACTION && s->keys[i].domain!=DRAWING_UI_AUTHORING_FONT_THEME);
    }
    switch (phase) {
    case 0: {
        const CorePaneSplitterHit *hit=left_divider(app);
        CHECK(hit);
        x=hit->splitter_bounds.x+hit->splitter_bounds.width/2-6*app->pane_host.splitter_scale_x;
        /* The widened band overlaps an ordinary content tab beside the edge;
         * explicit header action slots retain their own visible click bounds. */
        const KitPaneCompositionEntry *side=NULL;
        for (uint32_t i=0;i<app->pane_host.composition_host.view.count;++i) {
            const KitPaneCompositionEntry *p=&app->pane_host.composition_host.view.entries[i];
            if (p->header.height>0 && p->header.x<x && p->header.x+p->header.width>x) { side=p; break; }
        }
        CHECK(side); y=side->content.y+make_pane_layout_metrics(app).tab_h/2;
        before=app->pane_host.layout_state; memcpy(nodes,app->pane_host.nodes,sizeof(nodes));
        moved_x=x+40*app->pane_host.splitter_scale_x;
        CHECK(capture(renderer,dir,"resize-initial")); CHECK(pointer(window,renderer,SDL_MOUSEMOTION,x,y)); break;
    }
    case 1: {
        CorePaneRect line; int hover,drag;
        /* SDL may deliver a late real pointer/window notification after our
         * synthetic motion. Synchronize on observed hover, with a finite bound. */
        if (!drawing_program_pane_host_visible_splitter(app,&line,&hover,&drag)) {
            CHECK(++hover_attempts<12);
            CHECK(pointer(window,renderer,SDL_MOUSEMOTION,x,y));
            return 0;
        }
        CHECK(hover && !drag && line.width==2);
        CHECK(pointer(window,renderer,SDL_MOUSEBUTTONDOWN,x,y)); break;
    }
    case 2:
        CHECK(!app->authoring_host.draft_baseline_valid);
        CHECK(capture(renderer,dir,"resize-pressed"));
        CHECK(pointer(window,renderer,SDL_MOUSEMOTION,moved_x,y)); break;
    case 3:
        CHECK(memcmp(nodes,app->pane_host.nodes,sizeof(nodes)));
        CHECK(app->pane_host.layout_state.active_revision==before.active_revision);
        CHECK(capture(renderer,dir,"resize-dragged")); break;
    case 4: CHECK(pointer(window,renderer,SDL_MOUSEBUTTONUP,moved_x,y)); break;
    case 5: {
        CHECK(!active && !drawing_program_authoring_host_active(app));
        CHECK(app->pane_host.layout_state.active_revision==before.active_revision+1);
        CHECK(capture(renderer,dir,"resize-committed"));
        puts("SPLITTER_PROOF wide-band-quiet-runtime-commit status=pass");
        before=app->pane_host.layout_state; memcpy(nodes,app->pane_host.nodes,sizeof(nodes));
        /* Accepted constraints may adjust the divider; a new gesture uses its
         * current hit registry, rather than the previous draft coordinates. */
        const CorePaneSplitterHit *hit=left_divider(app); CHECK(hit);
        moved_x=hit->splitter_bounds.x+hit->splitter_bounds.width/2-6*app->pane_host.splitter_scale_x;
        CHECK(!drawing_program_ui_controls_header_at((int)moved_x,(int)y));
        CHECK(pointer(window,renderer,SDL_MOUSEBUTTONDOWN,moved_x,y)); break;
    }
    case 6:
        if (!active) fprintf(stderr,"SPLITTER_PROOF second-press x=%g y=%g mode=%u paused=%u overlay=%u\n",moved_x,y,app->pane_host.layout_state.mode,app->overlay_adapter.runtime_paused,app->overlay_adapter.lifecycle_state);
        CHECK(active); CHECK(pointer(window,renderer,SDL_MOUSEMOTION,moved_x+30*app->pane_host.splitter_scale_x,y)); break;
    case 7:
        CHECK(active && escape(SDL_KEYDOWN, 0));
        break;
    case 8:
        CHECK(!active && !memcmp(nodes,app->pane_host.nodes,sizeof(nodes)));
        CHECK(!memcmp(&before,&app->pane_host.layout_state,sizeof(before)));
        CHECK(capture(renderer,dir,"resize-cancelled"));
        CHECK(escape(SDL_KEYDOWN, 1));
        CHECK(escape(SDL_KEYUP, 0));
        CHECK(pointer(window,renderer,SDL_MOUSEBUTTONUP,moved_x,y));
        break;
    case 9:
        CHECK(!active && !drawing_program_authoring_host_active(app));
        CHECK(!memcmp(nodes,app->pane_host.nodes,sizeof(nodes)));
        CHECK(!memcmp(&before,&app->pane_host.layout_state,sizeof(before)));
        puts("SPLITTER_PROOF held-escape-and-late-release status=pass");
        /* The established explicit Alt+C then Alt+V entry remains available. */
        key(SDLK_c,KMOD_ALT); key(SDLK_v,KMOD_ALT); break;
    case 10:
        CHECK(drawing_program_authoring_host_active(app));
        CHECK(app->authoring_host.draft_baseline_valid);
        CHECK(capture(renderer,dir,"resize-explicit-authoring")); break;
    case 11:
        /* Window title updates after present; observe the completed frame. */
        CHECK(strstr(SDL_GetWindowTitle(window), "[Authoring]"));
        key(SDLK_ESCAPE,0); break;
    case 12:
        CHECK(!drawing_program_authoring_host_active(app));
        CHECK(!memcmp(nodes,app->pane_host.nodes,sizeof(nodes)));
        puts("SPLITTER_PROOF cancellation-and-explicit-authoring status=pass"); break;
    case 13:
    case 21:
        CHECK(!active);
        CHECK(pointer(window, renderer, SDL_MOUSEBUTTONDOWN, moved_x, y));
        break;
    case 17:
        puts("SPLITTER_PROOF round-trip-no-op status=pass");
        CHECK(pointer(window, renderer, SDL_MOUSEBUTTONDOWN, moved_x, y));
        break;
    case 14:
    case 18:
    case 22:
        CHECK(active);
        CHECK(pointer(window, renderer, SDL_MOUSEMOTION,
                      moved_x + 30 * app->pane_host.splitter_scale_x, y));
        break;
    case 15:
        CHECK(active && memcmp(nodes, app->pane_host.nodes, sizeof(nodes)));
        CHECK(pointer(window, renderer, SDL_MOUSEMOTION, moved_x, y));
        break;
    case 16:
        CHECK(active);
        CHECK(pointer(window, renderer, SDL_MOUSEBUTTONUP, moved_x, y));
        break;
    case 19: {
        CHECK(active && memcmp(nodes, app->pane_host.nodes, sizeof(nodes)));
        SDL_Event event = {0};
        event.type = SDL_WINDOWEVENT;
        event.window.windowID = SDL_GetWindowID(window);
        event.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
        CHECK(SDL_PushEvent(&event) == 1);
        CHECK(pointer(window, renderer, SDL_MOUSEBUTTONUP, moved_x, y));
        break;
    }
    case 20:
        puts("SPLITTER_PROOF focus-loss-and-late-release status=pass");
        break;
    case 23: {
        CHECK(active && memcmp(nodes, app->pane_host.nodes, sizeof(nodes)));
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        resized_width = width - 100;
        resized_height = height - 80;
        SDL_SetWindowSize(window, resized_width, resized_height);
        break;
    }
    case 24: {
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        CHECK(width == resized_width && height == resized_height);
        CHECK(pointer(window, renderer, SDL_MOUSEBUTTONUP, moved_x, y));
        break;
    }
    case 25:
        CHECK(!active && !drawing_program_authoring_host_active(app));
        CHECK(!memcmp(nodes, app->pane_host.nodes, sizeof(nodes)));
        CHECK(!memcmp(&before, &app->pane_host.layout_state, sizeof(before)));
        puts("SPLITTER_PROOF resize-and-late-release status=pass");
        break;
    case 26:
        puts("SPLITTER_PROOF status=complete"); return 1;
    default:
        CHECK(0);
    }
    ++phase; return 0;
}
