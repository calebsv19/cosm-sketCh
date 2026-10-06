#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_ui_controls.h"
#include <string.h>

void drawing_program_pane_host_capture_state(const DrawingProgramPaneHost *host,
    DrawingProgramPaneState *state) {
    state->layout = host->layout_state;
    memcpy(state->nodes, host->nodes, sizeof(state->nodes));
    state->node_count = host->node_count; state->root_index = host->root_index;
    memcpy(state->bindings, host->module_bindings, sizeof(state->bindings));
    state->binding_count = host->module_binding_count;
}
void drawing_program_pane_host_restore_state(DrawingProgramPaneHost *host,
    const DrawingProgramPaneState *state) {
    host->layout_state = state->layout;
    memcpy(host->nodes, state->nodes, sizeof(host->nodes));
    host->node_count = state->node_count; host->root_index = state->root_index;
    memcpy(host->module_bindings, state->bindings, sizeof(host->module_bindings));
    host->module_binding_count = state->binding_count;
}
CoreResult drawing_program_pane_host_validate_state(const DrawingProgramPaneHost *host,
    const DrawingProgramPaneState *state, CorePaneRect bounds) {
    if (!host || !state || !state->layout.active_revision || !state->node_count ||
        state->node_count > DRAWING_PROGRAM_PANE_NODE_CAPACITY ||
        state->binding_count > DRAWING_PROGRAM_MODULE_BINDING_CAPACITY)
        return (CoreResult){CORE_ERR_FORMAT, "invalid pane state capacity"};
    CorePaneValidationReport report;
    CorePaneLeafRect leaves[DRAWING_PROGRAM_PANE_LEAF_CAPACITY];
    uint32_t count = 0, ids[DRAWING_PROGRAM_PANE_LEAF_CAPACITY];
    if (!core_pane_validate_graph(state->nodes, state->node_count, state->root_index,
                                  bounds, &report) ||
        !core_pane_solve(state->nodes, state->node_count, state->root_index, bounds,
                         leaves, DRAWING_PROGRAM_PANE_LEAF_CAPACITY, &count))
        return (CoreResult){CORE_ERR_FORMAT, "invalid pane state graph"};
    for (uint32_t i=0; i<count; ++i) ids[i]=leaves[i].id;
    if (core_pane_module_validate_bindings(&host->module_registry, state->bindings,
            state->binding_count, ids, count) != CORE_PANE_MODULE_OK)
        return (CoreResult){CORE_ERR_FORMAT, "invalid pane state bindings"};
    return core_result_ok();
}

static DrawingProgramPaneController *controller(DrawingProgramPaneHost *host, CorePaneId id) {
    for (uint32_t i=0; i<host->controller_count; ++i)
        if (host->controllers[i].mounted && host->controllers[i].binding.pane_node_id==id)
            return &host->controllers[i];
    return NULL;
}
static const CorePaneModuleBinding *binding(const DrawingProgramPaneHost *host, CorePaneId id) {
    for (uint32_t i=0; i<host->module_binding_count; ++i)
        if (host->module_bindings[i].pane_node_id==id) return &host->module_bindings[i];
    return NULL;
}
static int same_binding(const CorePaneModuleBinding *a, const CorePaneModuleBinding *b) {
    return a && b && a->instance_id==b->instance_id && a->pane_node_id==b->pane_node_id &&
        a->module_type_id==b->module_type_id && a->config_variant==b->config_variant &&
        a->runtime_flags==b->runtime_flags;
}
static void dispatch(void *user, const KitPaneHostEvent *event) {
    DrawingProgramAppContext *ctx=user;
    DrawingProgramPaneHost *host=&ctx->pane_host;
    DrawingProgramPaneController *c=controller(host,event->id);
    if (event->type==KIT_PANE_HOST_MOUNT) {
        const CorePaneModuleBinding *b=binding(host,event->id);
        if (!b) return; /* The complete candidate was validated before dispatch. */
        for (uint32_t i=0; i<DRAWING_PROGRAM_MODULE_BINDING_CAPACITY; ++i) {
            if (host->controllers[i].mounted) continue;
            c=&host->controllers[i]; memset(c,0,sizeof(*c)); c->binding=*b;
            c->generation=++host->next_controller_generation; c->mounted=1;
            if (i>=host->controller_count) host->controller_count=i+1;
            break;
        }
    }
    if (!c) return;
    if (event->type==KIT_PANE_HOST_FOCUS) c->focused=1;
    if (event->type==KIT_PANE_HOST_BLUR) c->focused=0;
    if (event->type==KIT_PANE_HOST_POINTER_DOWN) c->captured=1;
    if (event->type==KIT_PANE_HOST_POINTER_UP || event->type==KIT_PANE_HOST_CANCEL)
        c->captured=0;
    if (event->type==KIT_PANE_HOST_CANCEL || event->type==KIT_PANE_HOST_UNMOUNT) {
        drawing_program_selection_cancel_transient(&ctx->selection);
        drawing_program_ui_controls_invalidate();
    }
    if (host->lifecycle_observer)
        host->lifecycle_observer(host->lifecycle_user,&c->binding,event);
    if (event->type==KIT_PANE_HOST_UNMOUNT) memset(c,0,sizeof(*c));
}
void drawing_program_pane_host_observe(DrawingProgramAppContext *ctx,
    DrawingProgramPaneLifecycleObserver observer, void *user) {
    if (!ctx) return;
    ctx->pane_host.lifecycle_observer=observer; ctx->pane_host.lifecycle_user=user;
}
void drawing_program_pane_host_document_swap_hook(DrawingProgramAppContext *ctx,
    void (*hook)(void *user)) {
    if (ctx) ctx->pane_host.document_swap_hook=hook;
}
void drawing_program_pane_host_before_document_swap(DrawingProgramAppContext *ctx) {
    if (!ctx) return;
    drawing_program_pane_host_cancel_input(ctx);
    if (ctx->pane_host.document_swap_hook)
        ctx->pane_host.document_swap_hook(ctx->pane_host.lifecycle_user);
}
CoreResult drawing_program_pane_host_sync_controllers(DrawingProgramAppContext *ctx,
    const KitPaneComposition *view, int blocked) {
    if (!ctx || !view || view->count>DRAWING_PROGRAM_MODULE_BINDING_CAPACITY)
        return (CoreResult){CORE_ERR_INVALID_ARG,"invalid pane lifecycle candidate"};
    DrawingProgramPaneHost *host=&ctx->pane_host;
    /* Prepare all fixed-module bindings before cancelling or releasing anything.
     * These controllers allocate no resources; renderer caches are lazy/host-owned. */
    uint32_t ids[DRAWING_PROGRAM_PANE_LEAF_CAPACITY];
    for (uint32_t i=0; i<host->leaf_count; ++i) ids[i]=host->leaves[i].id;
    if (core_pane_module_validate_bindings(&host->module_registry,host->module_bindings,
            host->module_binding_count,ids,host->leaf_count)!=CORE_PANE_MODULE_OK)
        return (CoreResult){CORE_ERR_FORMAT,"invalid pane lifecycle bindings"};
    KitPaneComposition visible=*view;
    visible.count=0;
    for (uint32_t i=0; i<view->count; ++i) {
        const KitPaneCompositionEntry *p=&view->entries[i];
        if (!binding(host,p->id)) continue;
        if (!p->enabled || p->visible_shell.width<=0 || p->visible_shell.height<=0) continue;
        visible.entries[visible.count++]=*p;
    }
    /* A stable pane ID may acquire a different instance/module/configuration.
     * Force its old controller through cancel/blur/unmount before replacement. */
    KitPaneComposition retained=host->composition_host.view;
    retained.count=0;
    int remount=0;
    for (uint32_t i=0; i<host->composition_host.view.count; ++i) {
        KitPaneCompositionEntry p=host->composition_host.view.entries[i];
        DrawingProgramPaneController *c=controller(host,p.id);
        if (c && same_binding(&c->binding,binding(host,p.id)))
            retained.entries[retained.count++]=p;
        else remount=1;
    }
    if (remount) {
        drawing_program_ui_controls_invalidate();
        (void)kit_pane_host_sync(&host->composition_host,&retained,blocked,dispatch,ctx);
    }
    return kit_pane_host_sync(&host->composition_host,&visible,blocked,dispatch,ctx);
}
void drawing_program_pane_host_cancel_input(DrawingProgramAppContext *ctx) {
    if (ctx) kit_pane_host_cancel(&ctx->pane_host.composition_host,dispatch,ctx);
}
CorePaneId drawing_program_pane_host_route_pointer(DrawingProgramAppContext *ctx,
    KitPaneHostEventType type, float x, float y) {
    return ctx ? kit_pane_host_pointer(&ctx->pane_host.composition_host,type,x,y,dispatch,ctx) : 0;
}
void drawing_program_pane_host_dispose(DrawingProgramAppContext *ctx) {
    if (!ctx) return;
    KitPaneComposition empty={0};
    (void)kit_pane_host_sync(&ctx->pane_host.composition_host,&empty,1,dispatch,ctx);
    ctx->pane_host.controller_count=0;
    drawing_program_pane_host_observe(ctx,NULL,NULL);
    drawing_program_pane_host_document_swap_hook(ctx,NULL);
}
