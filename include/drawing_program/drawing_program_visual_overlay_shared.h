#ifndef DRAWING_PROGRAM_VISUAL_OVERLAY_SHARED_H
#define DRAWING_PROGRAM_VISUAL_OVERLAY_SHARED_H

#include "drawing_program/drawing_program_visual_state.h"

#ifdef __cplusplus
extern "C" {
#endif

int drawing_program_visual_overlay_sample_rect_to_screen_rect(const VisualCanvasSheetMetrics *metrics,
                                                              int32_t sample_x,
                                                              int32_t sample_y,
                                                              uint32_t width,
                                                              uint32_t height,
                                                              SDL_Rect *out_rect);

#ifdef __cplusplus
}
#endif

#endif
