#include "drawing_program/drawing_program_color_model.h"

static const uint8_t k_drawing_program_palette_values[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT] = {
    24u, 48u, 72u, 104u, 136u, 168u, 200u, 232u
};

uint8_t drawing_program_color_index_clamp(uint8_t index) {
    if (index >= DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT) {
        return (uint8_t)DRAWING_PROGRAM_UI_DEFAULT_COLOR_INDEX;
    }
    return index;
}

uint8_t drawing_program_color_default_index(void) {
    return (uint8_t)DRAWING_PROGRAM_UI_DEFAULT_COLOR_INDEX;
}

uint8_t drawing_program_color_eraser_value(void) {
    return 244u;
}

uint8_t drawing_program_color_value_from_index(uint8_t index) {
    return k_drawing_program_palette_values[drawing_program_color_index_clamp(index)];
}

void drawing_program_color_rgb_from_sample(uint8_t sample, uint8_t *out_r, uint8_t *out_g, uint8_t *out_b) {
    if (out_r) {
        *out_r = sample;
    }
    if (out_g) {
        *out_g = sample;
    }
    if (out_b) {
        *out_b = sample;
    }
}

void drawing_program_color_rgb_from_index(uint8_t index, uint8_t *out_r, uint8_t *out_g, uint8_t *out_b) {
    drawing_program_color_rgb_from_sample(drawing_program_color_value_from_index(index), out_r, out_g, out_b);
}
