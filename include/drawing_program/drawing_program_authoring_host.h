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
int drawing_program_authoring_host_pane_overlay_active(const DrawingProgramAppContext *ctx);
int drawing_program_authoring_host_font_theme_overlay_active(const DrawingProgramAppContext *ctx);
CoreResult drawing_program_authoring_host_enter(DrawingProgramAppContext *ctx);
CoreResult drawing_program_authoring_host_exit(DrawingProgramAppContext *ctx);
CoreResult drawing_program_authoring_host_apply(DrawingProgramAppContext *ctx);
CoreResult drawing_program_authoring_host_cancel(DrawingProgramAppContext *ctx);
CoreResult drawing_program_authoring_host_mark_draft_changed(DrawingProgramAppContext *ctx);
CoreResult drawing_program_authoring_host_cycle_overlay(DrawingProgramAppContext *ctx);
CoreResult drawing_program_authoring_host_adjust_font_zoom(DrawingProgramAppContext *ctx, int direction);
CoreResult drawing_program_authoring_host_reset_font_zoom(DrawingProgramAppContext *ctx);
CoreResult drawing_program_authoring_host_set_font_preset(DrawingProgramAppContext *ctx, uint32_t font_preset_id);
CoreResult drawing_program_authoring_host_set_theme_preset(DrawingProgramAppContext *ctx, uint32_t theme_preset_id);
CoreResult drawing_program_authoring_host_note_custom_theme_stub(DrawingProgramAppContext *ctx,
                                                                 const char *status);
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
