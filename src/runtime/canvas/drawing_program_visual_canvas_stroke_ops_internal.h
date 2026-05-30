#ifndef DRAWING_PROGRAM_VISUAL_CANVAS_STROKE_OPS_INTERNAL_H
#define DRAWING_PROGRAM_VISUAL_CANVAS_STROKE_OPS_INTERNAL_H

#include "drawing_program/drawing_program_visual_canvas_stroke_ops.h"

CoreResult drawing_program_visual_layer_sample_read_local(const DrawingProgramAppContext *ctx,
                                                          uint32_t layer_id,
                                                          uint32_t sample_x,
                                                          uint32_t sample_y,
                                                          DrawingProgramRasterSample *out_value);
CoreResult drawing_program_visual_record_raster_history_sample_change_local(
    DrawingProgramAppContext *ctx,
    DrawingProgramVisualRasterHistoryBatch *batch,
    uint32_t layer_id,
    uint32_t sample_x,
    uint32_t sample_y,
    DrawingProgramRasterSample value);
CoreResult drawing_program_visual_write_sample_without_history_on_layer_local(
    DrawingProgramAppContext *ctx,
    uint32_t layer_id,
    uint32_t sample_x,
    uint32_t sample_y,
    DrawingProgramRasterSample value);

#endif
