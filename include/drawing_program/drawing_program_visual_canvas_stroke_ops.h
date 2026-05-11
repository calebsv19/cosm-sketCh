#ifndef DRAWING_PROGRAM_VISUAL_CANVAS_STROKE_OPS_H
#define DRAWING_PROGRAM_VISUAL_CANVAS_STROKE_OPS_H

#include "drawing_program/drawing_program_visual_state.h"

typedef struct DrawingProgramVisualRasterHistoryBatch {
    uint32_t layer_id;
    uint32_t pending_delta_count;
    uint32_t group_delta_count;
    uint8_t group_open;
    DrawingProgramHistoryRasterDeltaEntry pending_deltas[DRAWING_PROGRAM_HISTORY_DELTA_BLOCK_FLUSH_CAPACITY];
} DrawingProgramVisualRasterHistoryBatch;

void drawing_program_visual_raster_history_batch_init(DrawingProgramVisualRasterHistoryBatch *batch,
                                                      uint32_t layer_id);
CoreResult drawing_program_visual_raster_history_batch_finish(DrawingProgramAppContext *ctx,
                                                              DrawingProgramVisualRasterHistoryBatch *batch);

CoreResult drawing_program_visual_apply_canvas_stamp_square_on_layer(DrawingProgramAppContext *ctx,
                                                                     uint32_t layer_id,
                                                                     int32_t sample_x,
                                                                     int32_t sample_y,
                                                                     DrawingProgramRasterSample value,
                                                                     uint32_t stroke_width,
                                                                     uint8_t hardness_percent);
CoreResult drawing_program_visual_apply_canvas_direct_stroke_stamp_square_on_layer(
    DrawingProgramAppContext *ctx,
    VisualCanvasInteractionState *interaction,
    uint32_t layer_id,
    int32_t sample_x,
    int32_t sample_y,
    DrawingProgramRasterSample value,
    uint32_t stroke_width,
    uint8_t hardness_percent);
CoreResult drawing_program_visual_flush_direct_stroke_history(DrawingProgramAppContext *ctx,
                                                              VisualCanvasInteractionState *interaction,
                                                              uint32_t layer_id);

CoreResult drawing_program_visual_apply_canvas_line_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                              uint32_t layer_id,
                                                                              int32_t x0,
                                                                              int32_t y0,
                                                                              int32_t x1,
                                                                              int32_t y1,
                                                                              DrawingProgramRasterSample value,
                                                                              uint32_t stroke_width,
                                                                              uint8_t hardness_percent);
CoreResult drawing_program_visual_apply_canvas_line_between_samples_with_history_batch(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *history_batch,
    uint32_t layer_id,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    DrawingProgramRasterSample value,
    uint32_t stroke_width,
    uint8_t hardness_percent);

CoreResult drawing_program_visual_apply_canvas_rect_fill_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                                   uint32_t layer_id,
                                                                                   int32_t x0,
                                                                                   int32_t y0,
                                                                                   int32_t x1,
                                                                                   int32_t y1,
                                                                                   DrawingProgramRasterSample value);
CoreResult drawing_program_visual_apply_canvas_rect_fill_between_samples_with_history_batch(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *history_batch,
    uint32_t layer_id,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    DrawingProgramRasterSample value);

CoreResult drawing_program_visual_apply_canvas_rect_outline_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                                      uint32_t layer_id,
                                                                                      int32_t x0,
                                                                                      int32_t y0,
                                                                                      int32_t x1,
                                                                                      int32_t y1,
                                                                                      DrawingProgramRasterSample value,
                                                                                      uint32_t stroke_width);
CoreResult drawing_program_visual_apply_canvas_rect_outline_between_samples_with_history_batch(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *history_batch,
    uint32_t layer_id,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    DrawingProgramRasterSample value,
    uint32_t stroke_width);

CoreResult drawing_program_visual_apply_canvas_circle_fill_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                                     uint32_t layer_id,
                                                                                     int32_t center_x,
                                                                                     int32_t center_y,
                                                                                     int32_t edge_x,
                                                                                     int32_t edge_y,
                                                                                     DrawingProgramRasterSample value);
CoreResult drawing_program_visual_apply_canvas_circle_fill_between_samples_with_history_batch(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *history_batch,
    uint32_t layer_id,
    int32_t center_x,
    int32_t center_y,
    int32_t edge_x,
    int32_t edge_y,
    DrawingProgramRasterSample value);

CoreResult drawing_program_visual_apply_canvas_circle_outline_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                                        uint32_t layer_id,
                                                                                        int32_t center_x,
                                                                                        int32_t center_y,
                                                                                        int32_t edge_x,
                                                                                        int32_t edge_y,
                                                                                        DrawingProgramRasterSample value,
                                                                                        uint32_t stroke_width);
CoreResult drawing_program_visual_apply_canvas_circle_outline_between_samples_with_history_batch(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *history_batch,
    uint32_t layer_id,
    int32_t center_x,
    int32_t center_y,
    int32_t edge_x,
    int32_t edge_y,
    DrawingProgramRasterSample value,
    uint32_t stroke_width);

#endif
