#ifndef DRAWING_PROGRAM_VISUAL_CANVAS_ACTION_OPS_H
#define DRAWING_PROGRAM_VISUAL_CANVAS_ACTION_OPS_H

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_app_main.h"

typedef struct DrawingProgramVisualCanvasActionOpsHooks {
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
    uint8_t (*tool_fill_tolerance_setting)(const DrawingProgramAppContext *ctx);
    int (*fill_sample_matches_tolerance)(uint8_t sample, uint8_t target, uint8_t tolerance_setting);
    CoreResult (*active_layer_sample_read_visual)(const DrawingProgramAppContext *ctx,
                                                  uint32_t sample_x,
                                                  uint32_t sample_y,
                                                  uint8_t *out_value);
    uint8_t (*color_index_for_sample)(uint8_t sample);
} DrawingProgramVisualCanvasActionOpsHooks;

CoreResult drawing_program_visual_apply_canvas_picker_at_screen(DrawingProgramAppContext *ctx,
                                                                SDL_Rect pane_rect,
                                                                int sx,
                                                                int sy,
                                                                const DrawingProgramVisualCanvasActionOpsHooks *hooks);

CoreResult drawing_program_visual_apply_canvas_fill_at_screen(DrawingProgramAppContext *ctx,
                                                              SDL_Rect pane_rect,
                                                              int sx,
                                                              int sy,
                                                              const DrawingProgramVisualCanvasActionOpsHooks *hooks);

#endif
