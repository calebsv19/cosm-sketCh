#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_ui_controls.h"
#include <stdlib.h>
#include <string.h>

CoreResult drawing_program_snapshot_load_payload(DrawingProgramAppContext *ctx, const char *path);

/* Stage existing formats/upgrades away from the live document, controllers and
 * renderer. Only the layer store and texture project have owned heap storage. */
CoreResult drawing_program_snapshot_load(DrawingProgramAppContext *ctx, const char *path) {
    if (!ctx || !path) return (CoreResult){CORE_ERR_INVALID_ARG,"invalid snapshot load"};
    DrawingProgramAppContext *candidate=malloc(sizeof(*candidate));
    if (!candidate) return (CoreResult){CORE_ERR_OUT_OF_MEMORY,"snapshot staging allocation failed"};
    *candidate=*ctx;
    memset(&candidate->layer_rasters,0,sizeof(candidate->layer_rasters));
    memset(&candidate->texture_project,0,sizeof(candidate->texture_project));
    candidate->pane_host.module_registry.entries=candidate->pane_host.module_entries;
    candidate->pane_host.lifecycle_observer=NULL;
    candidate->pane_host.lifecycle_user=NULL;
    candidate->pane_host.document_swap_hook=NULL;
    candidate->pane_host.controller_count=0;
    memset(candidate->pane_host.controllers,0,sizeof(candidate->pane_host.controllers));
    kit_pane_host_init(&candidate->pane_host.composition_host);
    CoreResult result=drawing_program_snapshot_load_payload(candidate,path);
    if (result.code!=CORE_OK) {
discard:
        drawing_program_layer_raster_store_dispose(&candidate->layer_rasters);
        drawing_program_texture_project_dispose(&candidate->texture_project);
        free(candidate);
        return result;
    }
    DrawingProgramPaneState accepted;
    drawing_program_pane_host_capture_state(&candidate->pane_host,&accepted);
    CorePaneRect bounds={0,0,ctx->pane_host_bounds_width,ctx->pane_host_bounds_height};
    if (bounds.width<64 || bounds.height<64) bounds=(CorePaneRect){0,0,1200,800};
    result=drawing_program_pane_host_validate_state(&candidate->pane_host,&accepted,bounds);
    if (result.code!=CORE_OK) goto discard;
    /* Cancel against the old document before releasing it or publishing new state. */
    drawing_program_pane_host_before_document_swap(ctx);
    DrawingProgramPaneHost live_host=ctx->pane_host;
    drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
    drawing_program_texture_project_dispose(&ctx->texture_project);
    *ctx=*candidate;
    free(candidate); /* Owned stores have moved to ctx. */
    ctx->pane_host=live_host;
    ctx->pane_host.module_registry.entries=ctx->pane_host.module_entries;
    drawing_program_pane_host_restore_state(&ctx->pane_host,&accepted);
    /* Persisted layouts are runtime state, never resumed gestures. */
    memset(&ctx->pane_host.splitter_edit,0,sizeof(ctx->pane_host.splitter_edit));
    kit_pane_splitter_interaction_end_drag(&ctx->pane_host.splitter_interaction);
    drawing_program_authoring_host_reset(ctx);
    result=drawing_program_pane_host_rebuild(ctx);
    drawing_program_ui_controls_invalidate();
    return result;
}
