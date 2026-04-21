#ifndef DRAWING_PROGRAM_UI_COLOR_STATE_H
#define DRAWING_PROGRAM_UI_COLOR_STATE_H

#include "drawing_program/drawing_program_app_main.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawing_program_ui_color_seed_defaults(DrawingProgramAppContext *ctx);
void drawing_program_ui_color_sync_selector_from_active(DrawingProgramAppContext *ctx);
void drawing_program_ui_color_sync_runtime_palette(DrawingProgramAppContext *ctx);
void drawing_program_ui_color_normalize_state(DrawingProgramAppContext *ctx);
void drawing_program_ui_color_push_recent_rgb(DrawingProgramAppContext *ctx, uint8_t r, uint8_t g, uint8_t b);
void drawing_program_ui_color_apply_active_rgb(DrawingProgramAppContext *ctx, uint8_t r, uint8_t g, uint8_t b);
void drawing_program_ui_color_apply_hsv_to_active(DrawingProgramAppContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
