#ifndef DRAWING_PROGRAM_AUTHORING_HOST_H
#define DRAWING_PROGRAM_AUTHORING_HOST_H

#include <SDL2/SDL.h>

#include "core_base.h"
#include "drawing_program/drawing_program_app_main.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawing_program_authoring_host_reset(DrawingProgramAppContext *ctx);
int drawing_program_authoring_host_active(const DrawingProgramAppContext *ctx);
CoreResult drawing_program_authoring_host_enter(DrawingProgramAppContext *ctx);
CoreResult drawing_program_authoring_host_exit(DrawingProgramAppContext *ctx);
CoreResult drawing_program_authoring_host_apply(DrawingProgramAppContext *ctx);
CoreResult drawing_program_authoring_host_cancel(DrawingProgramAppContext *ctx);
CoreResult drawing_program_authoring_host_mark_draft_changed(DrawingProgramAppContext *ctx);
CoreResult drawing_program_authoring_host_export_accepted_pane_state(
    const DrawingProgramAppContext *ctx,
    CoreLayoutState *out_layout_state,
    CorePaneNode out_nodes[DRAWING_PROGRAM_PANE_NODE_CAPACITY],
    uint32_t *out_node_count,
    uint32_t *out_root_index,
    CorePaneModuleBinding out_module_bindings[DRAWING_PROGRAM_MODULE_BINDING_CAPACITY],
    uint32_t *out_module_binding_count);
int drawing_program_authoring_host_handle_sdl_event(DrawingProgramAppContext *ctx,
                                                    const SDL_Event *event);

#ifdef __cplusplus
}
#endif

#endif
