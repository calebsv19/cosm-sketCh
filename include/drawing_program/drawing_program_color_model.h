#ifndef DRAWING_PROGRAM_COLOR_MODEL_H
#define DRAWING_PROGRAM_COLOR_MODEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT 8u
#define DRAWING_PROGRAM_UI_DEFAULT_COLOR_INDEX 2u

uint8_t drawing_program_color_index_clamp(uint8_t index);
uint8_t drawing_program_color_default_index(void);
uint8_t drawing_program_color_eraser_value(void);
uint8_t drawing_program_color_value_from_index(uint8_t index);
void drawing_program_color_rgb_from_sample(uint8_t sample, uint8_t *out_r, uint8_t *out_g, uint8_t *out_b);
void drawing_program_color_rgb_from_index(uint8_t index, uint8_t *out_r, uint8_t *out_g, uint8_t *out_b);

#ifdef __cplusplus
}
#endif

#endif
