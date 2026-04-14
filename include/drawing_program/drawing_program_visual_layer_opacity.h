#ifndef DRAWING_PROGRAM_VISUAL_LAYER_OPACITY_H
#define DRAWING_PROGRAM_VISUAL_LAYER_OPACITY_H

#include <stdint.h>

#include "drawing_program/drawing_program_app_main.h"

uint8_t drawing_program_visual_layer_opacity_get(const DrawingProgramAppContext *ctx, uint32_t layer_id);
void drawing_program_visual_layer_opacity_set(DrawingProgramAppContext *ctx,
                                              uint32_t layer_id,
                                              uint8_t opacity_percent);
void drawing_program_visual_layer_opacity_sync_document(DrawingProgramAppContext *ctx);
void drawing_program_visual_collect_layer_opacity_by_index(const DrawingProgramAppContext *ctx,
                                                           uint8_t *out_opacity,
                                                           uint32_t out_count);

#endif
