#ifndef DRAWING_PROGRAM_VISUAL_CANVAS_DRAW_ACTION_OPS_H
#define DRAWING_PROGRAM_VISUAL_CANVAS_DRAW_ACTION_OPS_H

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_visual_state.h"

typedef struct DrawingProgramVisualCanvasDrawActionOpsHooks {
    int (*screen_to_canvas_sample)(const DrawingProgramAppContext *ctx,
                                   SDL_Rect pane_rect,
                                   int sx,
                                   int sy,
                                   uint32_t *out_sample_x,
                                   uint32_t *out_sample_y);
    int (*active_layer_allows_edits_visual)(const DrawingProgramAppContext *ctx);
    int (*active_layer_query)(const DrawingProgramAppContext *ctx,
                              uint32_t *out_layer_id,
                              uint32_t *out_index,
                              uint8_t *out_visible,
                              uint8_t *out_locked);
    uint8_t (*sample_value_for_tool)(const DrawingProgramAppContext *ctx, DrawingProgramToolKind tool);
    uint32_t (*tool_brush_radius_samples)(const DrawingProgramAppContext *ctx, DrawingProgramToolKind tool);
    uint32_t (*tool_brush_spacing_samples)(const DrawingProgramAppContext *ctx,
                                           DrawingProgramToolKind tool,
                                           uint32_t radius);
    uint8_t (*tool_brush_hardness_percent)(const DrawingProgramAppContext *ctx, DrawingProgramToolKind tool);
    uint8_t (*seeded_background_sample_for_coord)(const DrawingProgramDocument *document, uint32_t x, uint32_t y);
    CoreResult (*apply_canvas_stamp_square_on_layer)(DrawingProgramAppContext *ctx,
                                                     uint32_t layer_id,
                                                     int32_t sample_x,
                                                     int32_t sample_y,
                                                     uint8_t value,
                                                     uint32_t stroke_width,
                                                     uint8_t hardness_percent);
} DrawingProgramVisualCanvasDrawActionOpsHooks;

CoreResult drawing_program_visual_apply_canvas_draw_at_screen(DrawingProgramAppContext *ctx,
                                                              SDL_Rect pane_rect,
                                                              int sx,
                                                              int sy,
                                                              VisualCanvasInteractionState *state,
                                                              const DrawingProgramVisualCanvasDrawActionOpsHooks *hooks);

#endif
