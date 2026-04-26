#ifndef DRAWING_PROGRAM_UI_COLOR_STATE_H
#define DRAWING_PROGRAM_UI_COLOR_STATE_H

#include "drawing_program/drawing_program_app_main.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawing_program_ui_color_seed_defaults(DrawingProgramAppContext *ctx);
void drawing_program_ui_color_sync_selector_from_paint(DrawingProgramAppContext *ctx);
void drawing_program_ui_color_sync_runtime_palette(DrawingProgramAppContext *ctx);
void drawing_program_ui_color_normalize_state(DrawingProgramAppContext *ctx);
void drawing_program_ui_color_select_recent_slot(DrawingProgramAppContext *ctx, uint8_t recent_index);
void drawing_program_ui_color_push_recent_rgb(DrawingProgramAppContext *ctx, uint8_t r, uint8_t g, uint8_t b);
void drawing_program_ui_color_set_active_paint_rgb(
    DrawingProgramAppContext *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t push_recent);
void drawing_program_ui_color_set_active_paint_sample(
    DrawingProgramAppContext *ctx, DrawingProgramRasterSample sample, uint8_t push_recent);
void drawing_program_ui_color_load_active_paint_from_swatch(DrawingProgramAppContext *ctx, uint8_t swatch_index);
void drawing_program_ui_color_save_active_paint_to_swatch(DrawingProgramAppContext *ctx, uint8_t swatch_index);
void drawing_program_ui_color_apply_hsv_to_paint(DrawingProgramAppContext *ctx, uint8_t push_recent);
uint8_t drawing_program_ui_color_active_paint_index(const DrawingProgramAppContext *ctx);
DrawingProgramRasterSample drawing_program_ui_color_active_paint_sample_value(
    const DrawingProgramAppContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
