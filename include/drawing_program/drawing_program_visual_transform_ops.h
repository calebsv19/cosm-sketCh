#ifndef DRAWING_PROGRAM_VISUAL_TRANSFORM_OPS_H
#define DRAWING_PROGRAM_VISUAL_TRANSFORM_OPS_H

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_visual_state.h"

typedef struct DrawingProgramVisualTransformOpsHooks {
    CoreResult (*visual_selection_commit_move)(DrawingProgramAppContext *ctx, VisualSelectionState *selection);
} DrawingProgramVisualTransformOpsHooks;

void drawing_program_visual_cancel_canvas_draw_and_shape(VisualCanvasInteractionState *interaction);

void drawing_program_visual_cancel_selection_transient(VisualSelectionState *selection);

void drawing_program_visual_apply_selection_move_axis_lock(VisualSelectionState *selection,
                                                           VisualCanvasInteractionState *interaction,
                                                           SDL_Keymod mods);

void drawing_program_visual_apply_selection_move_canvas_bounds(const DrawingProgramDocument *document,
                                                               VisualSelectionState *selection);

void drawing_program_visual_transform_session_reset(VisualCanvasInteractionState *interaction);

int drawing_program_visual_transform_session_is_move_active(const VisualCanvasInteractionState *interaction);

void drawing_program_visual_transform_session_begin_move(VisualCanvasInteractionState *interaction,
                                                         VisualSelectionState *selection,
                                                         uint32_t sample_x,
                                                         uint32_t sample_y);

void drawing_program_visual_transform_session_update_move(const DrawingProgramAppContext *ctx,
                                                          VisualCanvasInteractionState *interaction,
                                                          VisualSelectionState *selection,
                                                          uint32_t sample_x,
                                                          uint32_t sample_y,
                                                          SDL_Keymod mods);

CoreResult drawing_program_visual_transform_session_commit_move(DrawingProgramAppContext *ctx,
                                                                VisualCanvasInteractionState *interaction,
                                                                VisualSelectionState *selection,
                                                                const DrawingProgramVisualTransformOpsHooks *hooks);

CoreResult drawing_program_visual_transform_session_nudge_move(DrawingProgramAppContext *ctx,
                                                               VisualCanvasInteractionState *interaction,
                                                               VisualSelectionState *selection,
                                                               int32_t dx,
                                                               int32_t dy,
                                                               const DrawingProgramVisualTransformOpsHooks *hooks);

void drawing_program_visual_begin_canvas_history_group(DrawingProgramAppContext *ctx);

void drawing_program_visual_end_canvas_history_group(DrawingProgramAppContext *ctx);

void drawing_program_visual_cancel_all_transient_interactions(DrawingProgramAppContext *ctx,
                                                              VisualCanvasInteractionState *interaction,
                                                              VisualSelectionState *selection,
                                                              int clear_pan_state);

#endif
