#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_snapshot_shell.h"
#include "drawing_program_lifecycle_test_support.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr,"Pane lifecycle line %d: %s\n",__LINE__,#c); return 1; } } while (0)
static KitPaneHostEvent events[128];
static CorePaneModuleBinding event_bindings[128];
static uint32_t event_count;
static int swap_calls, old_store_seen;
static DrawingProgramAppContext *swap_context;
static DrawingProgramRasterSample *swap_samples;
static void before_swap(void *user) {
    (void)user; ++swap_calls;
    old_store_seen=swap_context->layer_rasters.slot_samples==swap_samples;
}
static void observe(void *user, const CorePaneModuleBinding *b, const KitPaneHostEvent *e) {
    (void)user;
    if (event_count<128) { events[event_count]=*e; event_bindings[event_count++]=*b; }
}
static DrawingProgramPaneController *find(DrawingProgramAppContext *ctx, uint32_t id) {
    for (uint32_t i=0; i<ctx->pane_host.controller_count; ++i)
        if (ctx->pane_host.controllers[i].mounted && ctx->pane_host.controllers[i].binding.pane_node_id==id)
            return &ctx->pane_host.controllers[i];
    return NULL;
}
static int seed(DrawingProgramAppContext *ctx) {
    char *args[]={"pane-lifecycle","--headless","--no-persist",NULL};
    CHECK(drawing_program_app_bootstrap(ctx,3,args).code==CORE_OK);
    CHECK(drawing_program_app_config_load(ctx).code==CORE_OK);
    CHECK(drawing_program_app_state_seed(ctx).code==CORE_OK);
    CHECK(drawing_program_app_subsystems_init(ctx).code==CORE_OK);
    CHECK(drawing_program_runtime_start(ctx).code==CORE_OK);
    return 0;
}
static int lifecycle(DrawingProgramAppContext *ctx) {
    DrawingProgramPaneHost *h=&ctx->pane_host;
    drawing_program_pane_host_observe(ctx,observe,NULL); event_count=0;
    CHECK(drawing_program_pane_host_compose(ctx,0).code==CORE_OK);
    CHECK(event_count==4 && h->controller_count==4);
    uint64_t generation=h->next_controller_generation;
    CHECK(drawing_program_pane_host_compose(ctx,0).code==CORE_OK);
    CHECK(event_count==4 && h->next_controller_generation==generation);
    const KitPaneCompositionEntry *p=kit_pane_composition_find(&h->composition_host.view,6);
    CHECK(p); float x=p->content.x+10, y=p->content.y+10;
    CHECK(drawing_program_pane_host_route_pointer(ctx,KIT_PANE_HOST_POINTER_DOWN,x,y)==6);
    CHECK(find(ctx,6)->captured && find(ctx,6)->focused);
    uint64_t old_generation=find(ctx,6)->generation;
    event_count=0;
    /* Same pane, new instance: old ownership is revoked before replacement. */
    h->module_bindings[2].instance_id=30;
    CHECK(drawing_program_pane_host_rebuild(ctx).code==CORE_OK);
    CHECK(drawing_program_pane_host_compose(ctx,0).code==CORE_OK);
    CHECK(event_count==4 && events[0].type==KIT_PANE_HOST_CANCEL &&
          events[1].type==KIT_PANE_HOST_BLUR && events[2].type==KIT_PANE_HOST_UNMOUNT &&
          events[3].type==KIT_PANE_HOST_MOUNT);
    CHECK(event_bindings[2].instance_id==3 && event_bindings[3].instance_id==30);
    CHECK(find(ctx,6)->generation>old_generation && !find(ctx,6)->captured && !find(ctx,6)->focused);
    KitPaneComposition view=h->composition_host.view;
    DrawingProgramPaneController controllers[DRAWING_PROGRAM_MODULE_BINDING_CAPACITY];
    memcpy(controllers,h->controllers,sizeof(controllers)); event_count=0;
    h->module_bindings[2].module_type_id=999;
    CHECK(drawing_program_pane_host_sync_controllers(ctx,&view,0).code!=CORE_OK);
    CHECK(!event_count && !memcmp(controllers,h->controllers,sizeof(controllers)));
    CHECK(!memcmp(&view,&h->composition_host.view,sizeof(view)));
    CHECK(drawing_program_pane_host_rebuild(ctx).code!=CORE_OK);
    CHECK(h->module_bindings[2].module_type_id==1); /* Rejected candidate rolled back. */
    /* Hidden/disabled view unmounts; its model and borrowed resources survive. */
    h->module_bindings[2].runtime_flags=DRAWING_PROGRAM_PANE_HIDDEN;
    CHECK(drawing_program_pane_host_rebuild(ctx).code==CORE_OK);
    CHECK(drawing_program_pane_host_compose(ctx,0).code==CORE_OK);
    CHECK(!find(ctx,6));
    SDL_Rect hidden; CHECK(!drawing_program_visual_pane_rect_for_module_type(ctx,1,&hidden));
    h->module_bindings[2].runtime_flags=0;
    CHECK(drawing_program_pane_host_rebuild(ctx).code==CORE_OK);
    CHECK(drawing_program_pane_host_compose(ctx,0).code==CORE_OK && find(ctx,6));
    CHECK(h->module_bindings[2].instance_id==30);
    CHECK(drawing_program_pane_host_route_pointer(ctx,KIT_PANE_HOST_POINTER_DOWN,x,y)==6);
    generation=find(ctx,6)->generation;
    CHECK(drawing_program_pane_host_compose(ctx,1).code==CORE_OK);
    CHECK(find(ctx,6)->generation==generation && !find(ctx,6)->captured && !find(ctx,6)->focused);
    CHECK(drawing_program_pane_host_compose(ctx,0).code==CORE_OK);
    /* An invalid Apply retains the accepted layout/controllers and allows Cancel. */
    CorePaneNode nodes[DRAWING_PROGRAM_PANE_NODE_CAPACITY]; memcpy(nodes,h->nodes,sizeof(nodes));
    CHECK(drawing_program_authoring_host_enter(ctx).code==CORE_OK);
    h->nodes[0].child_a=h->root_index;
    CHECK(drawing_program_authoring_host_apply(ctx).code!=CORE_OK);
    CHECK(drawing_program_authoring_host_active(ctx));
    CHECK(!memcmp(nodes,h->nodes,sizeof(nodes)) && find(ctx,6)->generation==generation);
    CHECK(drawing_program_authoring_host_cancel(ctx).code==CORE_OK);
    CHECK(drawing_program_pane_host_compose(ctx,0).code==CORE_OK);
    CHECK(find(ctx,6)->generation==generation);
    h->module_bindings[2].instance_id=3;
    CHECK(drawing_program_pane_host_rebuild(ctx).code==CORE_OK);
    CHECK(drawing_program_pane_host_compose(ctx,0).code==CORE_OK);
    return 0;
}
static int roundtrip(DrawingProgramAppContext *ctx, int indexed) {
    static DrawingProgramAppContext load;
    memset(&load,0,sizeof(load)); CHECK(!seed(&load));
    char path[4096]; CHECK(lifecycle_test_artifact_path(path,sizeof(path),indexed ? "pane-indexed.pack" : "pane-standard.pack"));
    DrawingProgramPaneHost *h=&ctx->pane_host;
    CorePaneSplitterHit hit=h->splitter_hits[1];
    float x=hit.splitter_bounds.x+hit.splitter_bounds.width/2,
          y=hit.splitter_bounds.y+hit.splitter_bounds.height/2;
    float ratio=h->nodes[2].ratio_01; uint64_t revision=h->layout_state.active_revision;
    CHECK(drawing_program_pane_host_begin_splitter_drag(ctx,x,y));
    CHECK(drawing_program_pane_host_update_splitter_drag(ctx,x+40,y));
    CHECK(h->nodes[2].ratio_01!=ratio);
    CHECK(drawing_program_snapshot_save(ctx,path).code==CORE_OK);
    CHECK(drawing_program_snapshot_load(&load,path).code==CORE_OK);
    CHECK(load.pane_host.nodes[2].ratio_01==ratio && load.pane_host.layout_state.active_revision==revision);
    CHECK(h->splitter_edit.active); /* Saving cannot complete the user's gesture. */
    drawing_program_pane_host_end_splitter_drag(ctx);
    ratio=h->nodes[2].ratio_01;
    CHECK(h->layout_state.active_revision==revision+1);
    CHECK(drawing_program_snapshot_save(ctx,path).code==CORE_OK);
    CHECK(drawing_program_snapshot_load(&load,path).code==CORE_OK);
    CHECK(load.pane_host.nodes[2].ratio_01==ratio && load.pane_host.layout_state.active_revision==revision+1);
    CHECK(load.texture_project.profile_kind==ctx->texture_project.profile_kind);
    /* Applied module identity/configuration survives reopen without default repair. */
    CHECK(drawing_program_authoring_host_enter(ctx).code==CORE_OK);
    h->module_bindings[1].module_type_id=4; h->module_bindings[3].module_type_id=2;
    h->module_bindings[1].config_variant=7;
    CHECK(drawing_program_authoring_host_mark_draft_changed(ctx).code==CORE_OK);
    CHECK(drawing_program_snapshot_save(ctx,path).code==CORE_OK);
    CHECK(drawing_program_snapshot_load(&load,path).code==CORE_OK);
    CHECK(load.pane_host.module_bindings[1].module_type_id==2);
    CHECK(drawing_program_authoring_host_apply(ctx).code==CORE_OK);
    CHECK(drawing_program_snapshot_save(ctx,path).code==CORE_OK);
    CHECK(drawing_program_snapshot_load(&load,path).code==CORE_OK);
    CHECK(!memcmp(load.pane_host.module_bindings,h->module_bindings,sizeof(h->module_bindings)));
    CHECK(drawing_program_pane_host_compose(ctx,0).code==CORE_OK);
    DrawingProgramPaneHost before=*h;
    swap_context=ctx; swap_samples=ctx->layer_rasters.slot_samples;
    swap_calls=old_store_seen=0;
    drawing_program_pane_host_document_swap_hook(ctx,before_swap);
    before=*h;
    CHECK(drawing_program_snapshot_load(ctx,"/nonexistent/sketch-pane-failure.pack").code!=CORE_OK);
    CHECK(!memcmp(h,&before,sizeof(before)));
    /* Fail after a different shell was read, rather than only at open(). */
    CorePackWriter writer; unsigned char invalid_ui=0;
    CHECK(core_pack_writer_open(path,&writer).code==CORE_OK);
    CHECK(drawing_program_snapshot_shell_write_current(&writer,&load).code==CORE_OK);
    CHECK(core_pack_writer_add_chunk(&writer,"DPLR",&invalid_ui,1).code==CORE_OK);
    CHECK(core_pack_writer_close(&writer).code==CORE_OK);
    uint64_t content=ctx->document.content_revision;
    DrawingProgramRasterSample *samples=ctx->layer_rasters.slot_samples;
    uint32_t count=ctx->texture_project.surface_count;
    CHECK(drawing_program_snapshot_load(ctx,path).code!=CORE_OK);
    CHECK(!memcmp(h,&before,sizeof(before)) && ctx->document.content_revision==content &&
          ctx->layer_rasters.slot_samples==samples && ctx->texture_project.surface_count==count);
    CHECK(!swap_calls); /* Failure cannot cancel/release the live resources. */
    /* Nonzero graph roots round-trip through the existing reserved header word. */
    CorePaneNode n=h->nodes[0]; h->nodes[0]=h->nodes[1]; h->nodes[1]=n;
    for (uint32_t i=0; i<h->node_count; ++i) if (h->nodes[i].type==CORE_PANE_NODE_SPLIT) {
        if (h->nodes[i].child_a<2) h->nodes[i].child_a=1-h->nodes[i].child_a;
        if (h->nodes[i].child_b<2) h->nodes[i].child_b=1-h->nodes[i].child_b;
    }
    h->root_index=1;
    h->module_bindings[2].runtime_flags=DRAWING_PROGRAM_PANE_HIDDEN;
    CHECK(drawing_program_pane_host_rebuild(ctx).code==CORE_OK);
    CHECK(drawing_program_snapshot_save(ctx,path).code==CORE_OK);
    CHECK(drawing_program_snapshot_load(&load,path).code==CORE_OK);
    CHECK(load.pane_host.root_index==1 && !memcmp(load.pane_host.nodes,h->nodes,sizeof(h->nodes)));
    CHECK(load.pane_host.module_bindings[2].runtime_flags==DRAWING_PROGRAM_PANE_HIDDEN);
    CHECK(drawing_program_snapshot_load(ctx,path).code==CORE_OK);
    CHECK(swap_calls==1 && old_store_seen && ctx->layer_rasters.slot_samples!=swap_samples);
    CHECK(drawing_program_app_shutdown(&load).code==CORE_OK); unlink(path);
    return 0;
}
int drawing_program_pane_lifecycle_suite(void) {
    static DrawingProgramAppContext ctx;
    for (int indexed=0; indexed<2; ++indexed) {
        memset(&ctx,0,sizeof(ctx)); CHECK(!seed(&ctx));
        if (indexed) {
            DrawingProgramIndexedTilesetProfile p={0};
            p.contract_revision=CORE_AUTHORED_TEXTURE_INDEXED_CONTRACT_REVISION_V1;
            p.atlas_width=ctx.document.logical_width; p.atlas_height=ctx.document.logical_height;
            p.logical_cell_width=8; p.logical_cell_height=8; p.slot_count=2;
            snprintf(p.tileset_id,sizeof(p.tileset_id),"pane.fixture");
            snprintf(p.slots[0].id,sizeof(p.slots[0].id),"transparent");
            snprintf(p.slots[1].id,sizeof(p.slots[1].id),"solid");
            p.slots[1].source_rgba=(CoreAuthoredTextureRgba8){255,0,0,255};
            p.slots[1].preview_rgba=p.slots[1].source_rgba;
            CHECK(drawing_program_texture_project_enable_indexed_atlas(&ctx.texture_project,&p,0).code==CORE_OK);
        }
        CHECK(!lifecycle(&ctx)); CHECK(!roundtrip(&ctx,indexed));
        CHECK(drawing_program_app_shutdown(&ctx).code==CORE_OK);
        CHECK(!ctx.pane_host.composition_host.view.count && !ctx.pane_host.controller_count);
    }
    puts("Pane lifecycle, accepted persistence and root conformance passed (standard/indexed)");
    return 0;
}
