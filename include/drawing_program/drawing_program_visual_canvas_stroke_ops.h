#ifndef DRAWING_PROGRAM_VISUAL_CANVAS_STROKE_OPS_H
#define DRAWING_PROGRAM_VISUAL_CANVAS_STROKE_OPS_H

#include "drawing_program/drawing_program_app_main.h"

CoreResult drawing_program_visual_apply_canvas_stamp_square_on_layer(DrawingProgramAppContext *ctx,
                                                                     uint32_t layer_id,
                                                                     int32_t sample_x,
                                                                     int32_t sample_y,
                                                                     uint8_t value,
                                                                     uint32_t stroke_width,
                                                                     uint8_t hardness_percent);

CoreResult drawing_program_visual_apply_canvas_line_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                              uint32_t layer_id,
                                                                              uint32_t x0,
                                                                              uint32_t y0,
                                                                              uint32_t x1,
                                                                              uint32_t y1,
                                                                              uint8_t value,
                                                                              uint32_t stroke_width,
                                                                              uint8_t hardness_percent);

CoreResult drawing_program_visual_apply_canvas_rect_fill_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                                   uint32_t layer_id,
                                                                                   uint32_t x0,
                                                                                   uint32_t y0,
                                                                                   uint32_t x1,
                                                                                   uint32_t y1,
                                                                                   uint8_t value);

CoreResult drawing_program_visual_apply_canvas_rect_outline_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                                      uint32_t layer_id,
                                                                                      uint32_t x0,
                                                                                      uint32_t y0,
                                                                                      uint32_t x1,
                                                                                      uint32_t y1,
                                                                                      uint8_t value,
                                                                                      uint32_t stroke_width);

CoreResult drawing_program_visual_apply_canvas_circle_fill_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                                     uint32_t layer_id,
                                                                                     uint32_t center_x,
                                                                                     uint32_t center_y,
                                                                                     uint32_t edge_x,
                                                                                     uint32_t edge_y,
                                                                                     uint8_t value);

CoreResult drawing_program_visual_apply_canvas_circle_outline_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                                        uint32_t layer_id,
                                                                                        uint32_t center_x,
                                                                                        uint32_t center_y,
                                                                                        uint32_t edge_x,
                                                                                        uint32_t edge_y,
                                                                                        uint8_t value,
                                                                                        uint32_t stroke_width);

#endif
