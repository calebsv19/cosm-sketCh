#ifndef DRAWING_PROGRAM_COLOR_MODEL_H
#define DRAWING_PROGRAM_COLOR_MODEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT 8u
#define DRAWING_PROGRAM_UI_DEFAULT_COLOR_INDEX 2u
#define DRAWING_PROGRAM_DOCUMENT_SCHEMA_VERSION_PALETTE_INDEX 3u
#define DRAWING_PROGRAM_DOCUMENT_SCHEMA_VERSION_TRUE_COLOR 4u

typedef uint32_t DrawingProgramRasterSample;

uint8_t drawing_program_color_index_clamp(uint8_t index);
uint8_t drawing_program_color_default_index(void);
DrawingProgramRasterSample drawing_program_color_eraser_value(void);
DrawingProgramRasterSample drawing_program_color_value_from_index(uint8_t index);
DrawingProgramRasterSample drawing_program_color_value_from_rgb(uint8_t r, uint8_t g, uint8_t b);
DrawingProgramRasterSample drawing_program_color_value_from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
uint8_t drawing_program_color_index_from_sample(DrawingProgramRasterSample sample);
uint8_t drawing_program_color_index_from_rgb(uint8_t r, uint8_t g, uint8_t b);
DrawingProgramRasterSample drawing_program_color_normalize_legacy_sample(uint8_t sample);
DrawingProgramRasterSample drawing_program_color_normalize_input_sample(DrawingProgramRasterSample sample);
uint8_t drawing_program_color_legacy_sample_from_sample(DrawingProgramRasterSample sample);
uint8_t drawing_program_color_sample_distance(DrawingProgramRasterSample a, DrawingProgramRasterSample b);
DrawingProgramRasterSample drawing_program_color_blend_samples(DrawingProgramRasterSample dst,
                                                               DrawingProgramRasterSample src,
                                                               uint8_t opacity_percent);
uint8_t drawing_program_color_sample_is_transparent(DrawingProgramRasterSample sample);
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
void drawing_program_color_rgb_from_sample(DrawingProgramRasterSample sample,
                                           uint8_t *out_r,
                                           uint8_t *out_g,
                                           uint8_t *out_b);
void drawing_program_color_rgba_from_sample(DrawingProgramRasterSample sample,
                                            uint8_t *out_r,
                                            uint8_t *out_g,
                                            uint8_t *out_b,
                                            uint8_t *out_a);
void drawing_program_color_rgb_from_index(uint8_t index, uint8_t *out_r, uint8_t *out_g, uint8_t *out_b);

#ifdef __cplusplus
}
#endif

#endif
