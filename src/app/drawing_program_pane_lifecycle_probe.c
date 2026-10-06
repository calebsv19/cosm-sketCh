#include "drawing_program/drawing_program_ui_pilot_probe.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_visual_resources.h"
#include "drawing_program/drawing_program_visual_surface_cache.h"
#include "drawing_program/drawing_program_render_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define REQUIRE(c) do { if (!(c)) { fprintf(stderr,"PANE_LIFECYCLE_PROOF status=fail phase=%d condition=%s\n",phase,#c); return -1; } } while (0)
static DrawingProgramPaneController *canvas_controller(DrawingProgramAppContext *app) {
    for (uint32_t i=0; i<app->pane_host.controller_count; ++i) {
        DrawingProgramPaneController *c=&app->pane_host.controllers[i];
        if (c->mounted && c->binding.module_type_id==1) return c;
    }
    return NULL;
}
static int capture(SDL_Renderer *r, const char *dir, const char *name) {
    char path[4096];
    if (snprintf(path,sizeof(path),"%s/%s.bmp",dir,name)>=(int)sizeof(path)) return 0;
    return drawing_program_render_backend_request_capture(r,path);
}
static const void *active_resource(const DrawingProgramAppContext *app) {
    const DrawingProgramTextureProject *p=&app->texture_project;
    if (p->profile_kind==DRAWING_PROGRAM_TEXTURE_PROJECT_PROFILE_INDEXED_ATLAS_V1)
        return p->indexed_raster.indices;
    if (!p->surfaces || p->active_surface_index>=p->surface_count) return NULL;
    return drawing_program_visual_surface_cache_texture(p->runtime_cache_epoch,
        p->surfaces[p->active_surface_index].surface_id);
}
int drawing_program_pane_lifecycle_probe(SDL_Window *window, SDL_Renderer *renderer,
                                         DrawingProgramAppContext *app) {
    const char *dir=getenv("DRAWING_PROGRAM_PANE_LIFECYCLE_PROOF");
    if (!dir || !*dir) return 0;
    static int phase;
    static Uint64 start;
    static const void *texture;
    static CorePaneModuleBinding original;
    static uint64_t generation, revision;
    static float ratio;
    static char path[4096];
    struct stat out;
    REQUIRE(stat(dir,&out)==0 && S_ISDIR(out.st_mode));
    if (!start) start=SDL_GetTicks64();
    REQUIRE(SDL_GetTicks64()-start<30000);
    if (phase==0 && SDL_GetTicks64()-start<250) return 0;
    DrawingProgramPaneHost *h=&app->pane_host;
    switch (phase) {
    case 0: {
        DrawingProgramPaneController *c=canvas_controller(app); REQUIRE(c);
        original=c->binding; generation=c->generation;
        texture=active_resource(app); REQUIRE(texture);
        revision=h->layout_state.active_revision;
        REQUIRE(capture(renderer,dir,"lifecycle-initial"));
        REQUIRE(drawing_program_authoring_host_enter(app).code==CORE_OK);
        for (uint32_t i=0; i<h->module_binding_count; ++i)
            if (h->module_bindings[i].pane_node_id==original.pane_node_id)
                h->module_bindings[i].instance_id+=100;
        REQUIRE(drawing_program_pane_host_rebuild(app).code==CORE_OK);
        REQUIRE(drawing_program_authoring_host_mark_draft_changed(app).code==CORE_OK);
        break;
    }
    case 1: {
        DrawingProgramPaneController *c=canvas_controller(app); REQUIRE(c && c->generation>generation);
        REQUIRE(c->binding.instance_id==original.instance_id+100);
        REQUIRE(!h->composition_host.pointer.captured_id && !h->composition_host.focused_id);
        REQUIRE(active_resource(app)==texture);
        REQUIRE(capture(renderer,dir,"lifecycle-draft"));
        REQUIRE(drawing_program_authoring_host_cancel(app).code==CORE_OK);
        break;
    }
    case 2: {
        DrawingProgramPaneController *c=canvas_controller(app); REQUIRE(c);
        REQUIRE(c->binding.instance_id==original.instance_id && h->layout_state.active_revision==revision);
        REQUIRE(active_resource(app)==texture);
        REQUIRE(capture(renderer,dir,"lifecycle-cancelled"));
        for (uint32_t i=0; i<h->module_binding_count; ++i)
            if (h->module_bindings[i].pane_node_id==original.pane_node_id)
                h->module_bindings[i].runtime_flags|=DRAWING_PROGRAM_PANE_HIDDEN;
        REQUIRE(drawing_program_pane_host_rebuild(app).code==CORE_OK);
        break;
    }
    case 3:
        REQUIRE(!canvas_controller(app));
        REQUIRE(!kit_pane_composition_find(&h->composition_host.view,original.pane_node_id));
        REQUIRE(active_resource(app)==texture);
        REQUIRE(capture(renderer,dir,"lifecycle-hidden"));
        for (uint32_t i=0; i<h->module_binding_count; ++i)
            if (h->module_bindings[i].pane_node_id==original.pane_node_id)
                h->module_bindings[i].runtime_flags=original.runtime_flags;
        REQUIRE(drawing_program_pane_host_rebuild(app).code==CORE_OK);
        break;
    case 4: {
        REQUIRE(canvas_controller(app) && active_resource(app)==texture);
        REQUIRE(capture(renderer,dir,"lifecycle-restored"));
        CorePaneSplitterHit hit=h->splitter_hits[1];
        float x=hit.splitter_bounds.x+hit.splitter_bounds.width/2,
              y=hit.splitter_bounds.y+hit.splitter_bounds.height/2;
        REQUIRE(drawing_program_pane_host_begin_splitter_drag(app,x,y));
        REQUIRE(drawing_program_pane_host_update_splitter_drag(app,x+40,y));
        drawing_program_pane_host_end_splitter_drag(app);
        ratio=h->nodes[2].ratio_01;
        REQUIRE(h->layout_state.active_revision==revision+1);
        REQUIRE(snprintf(path,sizeof(path),"%s/accepted-layout.pack",dir)<(int)sizeof(path));
        REQUIRE(drawing_program_snapshot_save(app,path).code==CORE_OK);
        REQUIRE(drawing_program_snapshot_load(app,path).code==CORE_OK);
        break;
    }
    case 5: {
        REQUIRE(h->nodes[2].ratio_01==ratio && h->layout_state.active_revision==revision+1);
        REQUIRE(canvas_controller(app) && active_resource(app));
        texture=active_resource(app); /* Reopen may rebuild the document cache epoch. */
        REQUIRE(capture(renderer,dir,"lifecycle-reopened"));
        uint64_t current=canvas_controller(app)->generation;
        REQUIRE(drawing_program_snapshot_load(app,"/nonexistent/sketch-lifecycle-proof.pack").code!=CORE_OK);
        REQUIRE(canvas_controller(app)->generation==current && active_resource(app)==texture);
        break;
    }
    default:
        printf("PANE_LIFECYCLE_PROOF status=complete profile=%u captures=6 resource_retained=1 accepted_reopen=1 resource_kind=%s\n",
               (unsigned)app->texture_project.profile_kind,
               app->texture_project.profile_kind==DRAWING_PROGRAM_TEXTURE_PROJECT_PROFILE_INDEXED_ATLAS_V1 ? "indexed-raster" : "surface-texture");
        return 1;
    }
    ++phase;
    /* Request another ordinary event-loop frame without changing product state. */
    SDL_Event wake={0}; wake.type=SDL_WINDOWEVENT;
    wake.window.windowID=SDL_GetWindowID(window); wake.window.event=SDL_WINDOWEVENT_EXPOSED;
    REQUIRE(SDL_PushEvent(&wake)==1);
    return 0;
}
