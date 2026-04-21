#ifndef DRAWING_PROGRAM_COLOR_MODEL_H
#define DRAWING_PROGRAM_COLOR_MODEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT 8u
#define DRAWING_PROGRAM_UI_DEFAULT_COLOR_INDEX 2u
#define DRAWING_PROGRAM_DOCUMENT_SCHEMA_VERSION_PALETTE_INDEX 3u

uint8_t drawing_program_color_index_clamp(uint8_t index);
uint8_t drawing_program_color_default_index(void);
uint8_t drawing_program_color_eraser_value(void);
uint8_t drawing_program_color_value_from_index(uint8_t index);
uint8_t drawing_program_color_index_from_sample(uint8_t sample);
uint8_t drawing_program_color_normalize_legacy_sample(uint8_t sample);
uint8_t drawing_program_color_sample_distance(uint8_t a, uint8_t b);
uint8_t drawing_program_color_blend_samples(uint8_t dst, uint8_t src, uint8_t opacity_percent);
void drawing_program_color_reset_palette_defaults(void);
void drawing_program_color_copy_palette_rgb(uint8_t out_palette[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3]);
void drawing_program_color_load_palette_rgb(
    const uint8_t palette[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3]);
void drawing_program_color_set_palette_entry(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void drawing_program_color_hsv_to_rgb(uint8_t hue,
                                      uint8_t saturation,
                                      uint8_t value,
                                      uint8_t *out_r,
                                      uint8_t *out_g,
                                      uint8_t *out_b);
void drawing_program_color_rgb_to_hsv(uint8_t r,
                                      uint8_t g,
                                      uint8_t b,
                                      uint8_t *out_hue,
                                      uint8_t *out_saturation,
                                      uint8_t *out_value);
void drawing_program_color_rgb_from_sample(uint8_t sample, uint8_t *out_r, uint8_t *out_g, uint8_t *out_b);
void drawing_program_color_rgb_from_index(uint8_t index, uint8_t *out_r, uint8_t *out_g, uint8_t *out_b);

#ifdef __cplusplus
}
#endif

#endif
