#include "drawing_program/drawing_program_ui_pilot_probe.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_render_backend.h"
#include "drawing_program/drawing_program_ui_controls.h"
#include "drawing_program/drawing_program_visual_input_workspace_view.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int push_pointer(SDL_Window *window, SDL_Renderer *renderer, Uint32 type,
                         Uint8 button, int x, int y) {
    int w,h,lw,lh;
    if (drawing_program_render_backend_output_size(renderer,&w,&h)!=0)
        return 0;
    SDL_GetWindowSize(window,&lw,&lh);
    if (w<=0 || h<=0) return 0;
    SDL_Event e={0}; e.type=type;
    if (type==SDL_MOUSEMOTION) {
        e.motion.state=SDL_BUTTON_RMASK;
        e.motion.x=x*lw/w; e.motion.y=y*lh/h;
    } else {
        e.button.button=button; e.button.x=x*lw/w; e.button.y=y*lh/h;
    }
    return SDL_PushEvent(&e)==1;
}
static void push_key(SDL_Keycode sym, int up) {
    SDL_Event e={0}; e.type=up ? SDL_KEYUP : SDL_KEYDOWN;
    e.key.keysym.sym=sym; e.key.keysym.scancode=SDL_GetScancodeFromKey(sym);
    SDL_PushEvent(&e);
}
static int find_control(uint32_t op, KitUiSurfaceKey *key, KitRenderRect *bounds) {
    const KitUiSurface *s=drawing_program_ui_controls_snapshot();
    for (uint32_t i=0; i<s->count; ++i)
        if (s->keys[i].domain==op && s->controls[i].enabled) {
            *key=s->keys[i]; *bounds=s->controls[i].bounds; return 1;
        }
    return 0;
}
static int capture(SDL_Renderer *r, const char *dir, const char *name) {
    char path[4096];
    if (snprintf(path,sizeof(path),"%s/%s.bmp",dir,name)>=(int)sizeof(path))
        return 0;
    return drawing_program_render_backend_request_capture(r,path);
}
#define REQUIRE(c) do { if (!(c)) { \
    fprintf(stderr,"PANE_HEADER_PROOF status=fail phase=%d condition=%s\n",phase,#c); \
    return -1; } } while (0)

int drawing_program_pane_header_probe(SDL_Window *window, SDL_Renderer *renderer,
                                      const DrawingProgramAppContext *app) {
    const char *dir=getenv("DRAWING_PROGRAM_PANE_HEADER_PROOF");
    if (!dir || !*dir) return 0;
    static int phase, tabs;
    static uint64_t start, revision;
    static DrawingProgramViewportState initial, panned, expected;
    static CorePaneNode nodes[DRAWING_PROGRAM_PANE_NODE_CAPACITY];
    static KitUiSurfaceKey fit, layout;
    static KitRenderRect fit_bounds;
    static int cx,cy;
    struct stat out;
    REQUIRE(stat(dir,&out)==0 && S_ISDIR(out.st_mode));
    if (!start) start=SDL_GetTicks64();
    REQUIRE(SDL_GetTicks64()-start<30000);
    int fx=(int)(fit_bounds.x+fit_bounds.width/2),
        fy=(int)(fit_bounds.y+fit_bounds.height/2);
    switch (phase) {
    case 0: {
        KitRenderRect ignored;
        REQUIRE(find_control(DRAWING_UI_PANE_HEADER_FIT,&fit,&fit_bounds));
        REQUIRE(find_control(DRAWING_UI_PANE_HEADER_LAYOUT,&layout,&ignored));
        SDL_Rect content;
        REQUIRE(drawing_program_visual_pane_rect_for_module_type(app,1,&content));
        cx=content.x+content.w/2; cy=content.y+content.h/2;
        initial=app->editor.viewport;
        revision=app->pane_host.layout_state.active_revision;
        memcpy(nodes,app->pane_host.nodes,sizeof(nodes));
        REQUIRE(capture(renderer,dir,"headers-initial"));
        REQUIRE(push_pointer(window,renderer,SDL_MOUSEBUTTONDOWN,SDL_BUTTON_RIGHT,cx,cy));
        break;
    }
    case 1:
        REQUIRE(push_pointer(window,renderer,SDL_MOUSEMOTION,0,cx+80,cy+40)); break;
    case 2:
        REQUIRE(push_pointer(window,renderer,SDL_MOUSEBUTTONUP,SDL_BUTTON_RIGHT,cx+80,cy+40)); break;
    case 3: {
        panned=app->editor.viewport;
        REQUIRE(panned.pan_x!=initial.pan_x || panned.pan_y!=initial.pan_y);
        DrawingProgramAppContext *oracle=malloc(sizeof(*oracle));
        REQUIRE(oracle);
        *oracle=*app;
        (void)drawing_program_visual_input_workspace_view_fit_all_or_reset(oracle);
        expected=oracle->editor.viewport;
        free(oracle);
        REQUIRE(push_pointer(window,renderer,SDL_MOUSEBUTTONDOWN,SDL_BUTTON_LEFT,fx,fy)); break;
    }
    case 4:
        REQUIRE(!memcmp(&panned,&app->editor.viewport,sizeof(panned)));
        REQUIRE(push_pointer(window,renderer,SDL_MOUSEBUTTONUP,SDL_BUTTON_LEFT,-20,-20)); break;
    case 5:
        REQUIRE(!memcmp(&panned,&app->editor.viewport,sizeof(panned)));
        REQUIRE(!app->pane_host.composition_host.pointer.captured_id);
        REQUIRE(push_pointer(window,renderer,SDL_MOUSEBUTTONDOWN,SDL_BUTTON_LEFT,fx,fy)); break;
    case 6:
        REQUIRE(push_pointer(window,renderer,SDL_MOUSEBUTTONUP,SDL_BUTTON_LEFT,fx,fy)); break;
    case 7:
        REQUIRE(!memcmp(&expected,&app->editor.viewport,sizeof(expected)));
        REQUIRE(!app->pane_host.composition_host.pointer.captured_id);
        REQUIRE(capture(renderer,dir,"headers-fit"));
        puts("PANE_HEADER_PROOF fit-release-outside-and-domain status=pass"); break;
    case 8: {
        const KitUiSurface *s=drawing_program_ui_controls_snapshot();
        for (uint32_t i=0; i<s->count; ++i)
            if (s->keys[i].domain==layout.domain && s->keys[i].value==layout.value &&
                s->controls[i].id==s->interaction.focused_id) {
                push_key(SDLK_SPACE,0); phase=9; return 0;
            }
        REQUIRE(++tabs<=(int)s->count+1);
        push_key(SDLK_TAB,0); push_key(SDLK_TAB,1); return 0;
    }
    case 9:
        REQUIRE(!drawing_program_authoring_host_active(app));
        push_key(SDLK_SPACE,1); break;
    case 10: {
        REQUIRE(drawing_program_authoring_host_active(app));
        const KitUiSurface *s=drawing_program_ui_controls_snapshot();
        for (uint32_t i=0; i<s->count; ++i)
            REQUIRE(s->keys[i].domain==DRAWING_UI_AUTHORING_ACTION ||
                    s->keys[i].domain==DRAWING_UI_AUTHORING_FONT_THEME);
        REQUIRE(capture(renderer,dir,"headers-authoring"));
        push_key(SDLK_ESCAPE,0); push_key(SDLK_ESCAPE,1); break;
    }
    case 11: {
        REQUIRE(!drawing_program_authoring_host_active(app));
        REQUIRE(revision==app->pane_host.layout_state.active_revision &&
                !memcmp(nodes,app->pane_host.nodes,sizeof(nodes)));
        const KitUiSurface *s=drawing_program_ui_controls_snapshot();
        int restored=0;
        for (uint32_t i=0; i<s->count; ++i)
            if (s->keys[i].domain==layout.domain && s->keys[i].value==layout.value &&
                s->controls[i].id==s->interaction.focused_id) restored=1;
        REQUIRE(restored);
        REQUIRE(capture(renderer,dir,"headers-return"));
        puts("PANE_HEADER_PROOF keyboard-layout-modal-cancel-and-focus status=pass"); break;
    }
    default:
        puts("PANE_HEADER_PROOF status=complete"); return 1;
    }
    ++phase;
    return 0;
}
