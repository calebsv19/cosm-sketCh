#ifndef DRAWING_PROGRAM_VISUAL_INPUT_SELECTION_OPS_H
#define DRAWING_PROGRAM_VISUAL_INPUT_SELECTION_OPS_H

#include "drawing_program/drawing_program_visual_input_handlers.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawing_program_visual_input_object_selection_clear(DrawingProgramAppContext *ctx);
void drawing_program_visual_input_object_selection_replace(DrawingProgramAppContext *ctx, uint32_t object_id);
void drawing_program_visual_input_object_selection_add(DrawingProgramAppContext *ctx, uint32_t object_id);
void drawing_program_visual_input_object_selection_remove(DrawingProgramAppContext *ctx, uint32_t object_id);
int drawing_program_visual_input_object_selection_hit_test(const DrawingProgramAppContext *ctx,
                                                           uint32_t sample_x,
                                                           uint32_t sample_y,
                                                           uint32_t *out_object_id);
VisualMarqueeCommitMode drawing_program_visual_input_resolve_select_commit_mode(
    const DrawingProgramAppContext *ctx,
    SDL_Keymod mods);
int delete_active_selection_payload_or_objects(DrawingProgramAppContext *ctx,
                                               DrawingProgramSelectionState *selection_state,
                                               const DrawingProgramVisualInputHandlersHooks *hooks);
int drawing_program_visual_input_toggle_selected_path_point_bezier(DrawingProgramAppContext *ctx);
int drawing_program_visual_input_toggle_selected_path_point_handle_link(DrawingProgramAppContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
