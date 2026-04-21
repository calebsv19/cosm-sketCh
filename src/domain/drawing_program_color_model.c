#include "drawing_program/drawing_program_color_model.h"

static const uint8_t k_drawing_program_default_palette_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3] = {
    { 24u, 24u, 24u },
    { 48u, 48u, 48u },
    { 72u, 72u, 72u },
    { 104u, 104u, 104u },
    { 136u, 136u, 136u },
    { 168u, 168u, 168u },
    { 200u, 200u, 200u },
    { 232u, 232u, 232u }
};
static uint8_t g_drawing_program_palette_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3] = {
    { 24u, 24u, 24u },
    { 48u, 48u, 48u },
    { 72u, 72u, 72u },
    { 104u, 104u, 104u },
    { 136u, 136u, 136u },
    { 168u, 168u, 168u },
    { 200u, 200u, 200u },
    { 232u, 232u, 232u }
};

static void drawing_program_color_rgb_from_sample_internal(uint8_t sample,
                                                           uint8_t *out_r,
                                                           uint8_t *out_g,
                                                           uint8_t *out_b) {
    if (sample < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT) {
        uint8_t index = drawing_program_color_index_clamp(sample);
        if (out_r) {
            *out_r = g_drawing_program_palette_rgb[index][0];
        }
        if (out_g) {
            *out_g = g_drawing_program_palette_rgb[index][1];
        }
        if (out_b) {
            *out_b = g_drawing_program_palette_rgb[index][2];
        }
        return;
    }
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
    return drawing_program_color_index_clamp(index);
}

uint8_t drawing_program_color_index_from_sample(uint8_t sample) {
    uint8_t best_index = drawing_program_color_default_index();
    uint8_t src_r = 0u;
    uint8_t src_g = 0u;
    uint8_t src_b = 0u;
    uint32_t best_score = UINT32_MAX;
    uint8_t i;
    if (sample < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT) {
        return sample;
    }
    if (sample == drawing_program_color_eraser_value()) {
        return drawing_program_color_default_index();
    }
    drawing_program_color_rgb_from_sample_internal(sample, &src_r, &src_g, &src_b);
    for (i = 0u; i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++i) {
        int dr = (int)g_drawing_program_palette_rgb[i][0] - (int)src_r;
        int dg = (int)g_drawing_program_palette_rgb[i][1] - (int)src_g;
        int db = (int)g_drawing_program_palette_rgb[i][2] - (int)src_b;
        uint32_t score = (uint32_t)(dr * dr) + (uint32_t)(dg * dg) + (uint32_t)(db * db);
        if (score < best_score) {
            best_score = score;
            best_index = i;
        }
    }
    return best_index;
}

uint8_t drawing_program_color_normalize_legacy_sample(uint8_t sample) {
    if (sample == drawing_program_color_eraser_value()) {
        return sample;
    }
    if (sample < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT) {
        return sample;
    }
    return drawing_program_color_index_from_sample(sample);
}

uint8_t drawing_program_color_sample_distance(uint8_t a, uint8_t b) {
    uint8_t ar = 0u;
    uint8_t ag = 0u;
    uint8_t ab = 0u;
    uint8_t br = 0u;
    uint8_t bg = 0u;
    uint8_t bb = 0u;
    uint8_t dr;
    uint8_t dg;
    uint8_t db;
    drawing_program_color_rgb_from_sample_internal(a, &ar, &ag, &ab);
    drawing_program_color_rgb_from_sample_internal(b, &br, &bg, &bb);
    dr = (ar > br) ? (uint8_t)(ar - br) : (uint8_t)(br - ar);
    dg = (ag > bg) ? (uint8_t)(ag - bg) : (uint8_t)(bg - ag);
    db = (ab > bb) ? (uint8_t)(ab - bb) : (uint8_t)(bb - ab);
    if (dg > dr) {
        dr = dg;
    }
    if (db > dr) {
        dr = db;
    }
    return dr;
}

uint8_t drawing_program_color_blend_samples(uint8_t dst, uint8_t src, uint8_t opacity_percent) {
    uint32_t alpha = (opacity_percent > 100u) ? 100u : (uint32_t)opacity_percent;
    uint8_t dst_r = 0u;
    uint8_t dst_g = 0u;
    uint8_t dst_b = 0u;
    uint8_t src_r = 0u;
    uint8_t src_g = 0u;
    uint8_t src_b = 0u;
    uint8_t out_r;
    uint8_t out_g;
    uint8_t out_b;
    uint8_t best_index = drawing_program_color_default_index();
    uint32_t best_score = UINT32_MAX;
    uint8_t i;
    if (alpha >= 100u) {
        return src;
    }
    drawing_program_color_rgb_from_sample_internal(dst, &dst_r, &dst_g, &dst_b);
    drawing_program_color_rgb_from_sample_internal(src, &src_r, &src_g, &src_b);
    out_r = (uint8_t)(((uint32_t)src_r * alpha + (uint32_t)dst_r * (100u - alpha) + 50u) / 100u);
    out_g = (uint8_t)(((uint32_t)src_g * alpha + (uint32_t)dst_g * (100u - alpha) + 50u) / 100u);
    out_b = (uint8_t)(((uint32_t)src_b * alpha + (uint32_t)dst_b * (100u - alpha) + 50u) / 100u);
    if (out_r == drawing_program_color_eraser_value() &&
        out_g == drawing_program_color_eraser_value() &&
        out_b == drawing_program_color_eraser_value()) {
        return drawing_program_color_eraser_value();
    }
    for (i = 0u; i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++i) {
        int dr = (int)g_drawing_program_palette_rgb[i][0] - (int)out_r;
        int dg = (int)g_drawing_program_palette_rgb[i][1] - (int)out_g;
        int db = (int)g_drawing_program_palette_rgb[i][2] - (int)out_b;
        uint32_t score = (uint32_t)(dr * dr) + (uint32_t)(dg * dg) + (uint32_t)(db * db);
        if (score < best_score) {
            best_score = score;
            best_index = i;
        }
    }
    return best_index;
}

void drawing_program_color_reset_palette_defaults(void) {
    uint8_t i;
    for (i = 0u; i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++i) {
        g_drawing_program_palette_rgb[i][0] = k_drawing_program_default_palette_rgb[i][0];
        g_drawing_program_palette_rgb[i][1] = k_drawing_program_default_palette_rgb[i][1];
        g_drawing_program_palette_rgb[i][2] = k_drawing_program_default_palette_rgb[i][2];
    }
}

void drawing_program_color_copy_palette_rgb(uint8_t out_palette[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3]) {
    uint8_t i;
    if (!out_palette) {
        return;
    }
    for (i = 0u; i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++i) {
        out_palette[i][0] = g_drawing_program_palette_rgb[i][0];
        out_palette[i][1] = g_drawing_program_palette_rgb[i][1];
        out_palette[i][2] = g_drawing_program_palette_rgb[i][2];
    }
}

void drawing_program_color_load_palette_rgb(
    const uint8_t palette[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3]) {
    uint8_t i;
    if (!palette) {
        return;
    }
    for (i = 0u; i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++i) {
        g_drawing_program_palette_rgb[i][0] = palette[i][0];
        g_drawing_program_palette_rgb[i][1] = palette[i][1];
        g_drawing_program_palette_rgb[i][2] = palette[i][2];
    }
}

void drawing_program_color_set_palette_entry(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t clamped = drawing_program_color_index_clamp(index);
    g_drawing_program_palette_rgb[clamped][0] = r;
    g_drawing_program_palette_rgb[clamped][1] = g;
    g_drawing_program_palette_rgb[clamped][2] = b;
}

void drawing_program_color_hsv_to_rgb(uint8_t hue,
                                      uint8_t saturation,
                                      uint8_t value,
                                      uint8_t *out_r,
                                      uint8_t *out_g,
                                      uint8_t *out_b) {
    uint8_t region;
    uint8_t remainder;
    uint8_t p;
    uint8_t q;
    uint8_t t;
    uint8_t r = value;
    uint8_t g = value;
    uint8_t b = value;
    if (saturation == 0u) {
        if (out_r) {
            *out_r = value;
        }
        if (out_g) {
            *out_g = value;
        }
        if (out_b) {
            *out_b = value;
        }
        return;
    }
    region = (uint8_t)(hue / 43u);
    remainder = (uint8_t)((hue - (region * 43u)) * 6u);
    p = (uint8_t)(((uint16_t)value * (uint16_t)(255u - saturation)) / 255u);
    q = (uint8_t)(((uint16_t)value * (uint16_t)(255u - (((uint16_t)saturation * remainder) / 255u))) / 255u);
    t = (uint8_t)(((uint16_t)value *
                   (uint16_t)(255u - (((uint16_t)saturation * (uint16_t)(255u - remainder)) / 255u))) / 255u);
    switch (region) {
        case 0u: r = value; g = t; b = p; break;
        case 1u: r = q; g = value; b = p; break;
        case 2u: r = p; g = value; b = t; break;
        case 3u: r = p; g = q; b = value; break;
        case 4u: r = t; g = p; b = value; break;
        default: r = value; g = p; b = q; break;
    }
    if (out_r) {
        *out_r = r;
    }
    if (out_g) {
        *out_g = g;
    }
    if (out_b) {
        *out_b = b;
    }
}

void drawing_program_color_rgb_to_hsv(uint8_t r,
                                      uint8_t g,
                                      uint8_t b,
                                      uint8_t *out_hue,
                                      uint8_t *out_saturation,
                                      uint8_t *out_value) {
    uint8_t max = r;
    uint8_t min = r;
    uint8_t delta;
    uint8_t hue = 0u;
    uint8_t saturation = 0u;
    uint8_t value = max;
    if (g > max) {
        max = g;
    }
    if (b > max) {
        max = b;
    }
    if (g < min) {
        min = g;
    }
    if (b < min) {
        min = b;
    }
    value = max;
    delta = (uint8_t)(max - min);
    if (max != 0u) {
        saturation = (uint8_t)(((uint16_t)delta * 255u) / max);
    }
    if (delta != 0u) {
        int h;
        if (max == r) {
            h = (43 * ((int)g - (int)b)) / (int)delta;
        } else if (max == g) {
            h = 85 + (43 * ((int)b - (int)r)) / (int)delta;
        } else {
            h = 171 + (43 * ((int)r - (int)g)) / (int)delta;
        }
        if (h < 0) {
            h += 256;
        }
        hue = (uint8_t)h;
    }
    if (out_hue) {
        *out_hue = hue;
    }
    if (out_saturation) {
        *out_saturation = saturation;
    }
    if (out_value) {
        *out_value = value;
    }
}

void drawing_program_color_rgb_from_sample(uint8_t sample, uint8_t *out_r, uint8_t *out_g, uint8_t *out_b) {
    drawing_program_color_rgb_from_sample_internal(sample, out_r, out_g, out_b);
}

void drawing_program_color_rgb_from_index(uint8_t index, uint8_t *out_r, uint8_t *out_g, uint8_t *out_b) {
    uint8_t clamped = drawing_program_color_index_clamp(index);
    if (out_r) {
        *out_r = g_drawing_program_palette_rgb[clamped][0];
    }
    if (out_g) {
        *out_g = g_drawing_program_palette_rgb[clamped][1];
    }
    if (out_b) {
        *out_b = g_drawing_program_palette_rgb[clamped][2];
    }
}
