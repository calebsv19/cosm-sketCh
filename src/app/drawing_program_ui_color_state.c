#include "drawing_program/drawing_program_ui_color_state.h"

static int drawing_program_ui_color_palette_missing(const DrawingProgramAppContext *ctx) {
    uint8_t i;
    if (!ctx) {
        return 1;
    }
    for (i = 0u; i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++i) {
        if (ctx->ui.color_palette_rgb[i][0] != 0u ||
            ctx->ui.color_palette_rgb[i][1] != 0u ||
            ctx->ui.color_palette_rgb[i][2] != 0u) {
            return 0;
        }
    }
    return 1;
}

void drawing_program_ui_color_push_recent_rgb(DrawingProgramAppContext *ctx, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t i;
    if (!ctx) {
        return;
    }
    for (i = 0u; i < ctx->ui.recent_color_count; ++i) {
        if (ctx->ui.recent_color_rgb[i][0] == r &&
            ctx->ui.recent_color_rgb[i][1] == g &&
            ctx->ui.recent_color_rgb[i][2] == b) {
            uint8_t j;
            for (j = i; j > 0u; --j) {
                ctx->ui.recent_color_rgb[j][0] = ctx->ui.recent_color_rgb[j - 1u][0];
                ctx->ui.recent_color_rgb[j][1] = ctx->ui.recent_color_rgb[j - 1u][1];
                ctx->ui.recent_color_rgb[j][2] = ctx->ui.recent_color_rgb[j - 1u][2];
            }
            ctx->ui.recent_color_rgb[0][0] = r;
            ctx->ui.recent_color_rgb[0][1] = g;
            ctx->ui.recent_color_rgb[0][2] = b;
            return;
        }
    }
    for (i = (ctx->ui.recent_color_count >= DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT
                  ? (uint8_t)(DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT - 1u)
                  : ctx->ui.recent_color_count);
         i > 0u;
         --i) {
        ctx->ui.recent_color_rgb[i][0] = ctx->ui.recent_color_rgb[i - 1u][0];
        ctx->ui.recent_color_rgb[i][1] = ctx->ui.recent_color_rgb[i - 1u][1];
        ctx->ui.recent_color_rgb[i][2] = ctx->ui.recent_color_rgb[i - 1u][2];
    }
    ctx->ui.recent_color_rgb[0][0] = r;
    ctx->ui.recent_color_rgb[0][1] = g;
    ctx->ui.recent_color_rgb[0][2] = b;
    if (ctx->ui.recent_color_count < DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT) {
        ctx->ui.recent_color_count += 1u;
    }
}

void drawing_program_ui_color_apply_active_rgb(DrawingProgramAppContext *ctx, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t index;
    if (!ctx) {
        return;
    }
    index = drawing_program_color_index_clamp(ctx->ui.active_color_index);
    ctx->ui.color_palette_rgb[index][0] = r;
    ctx->ui.color_palette_rgb[index][1] = g;
    ctx->ui.color_palette_rgb[index][2] = b;
    drawing_program_color_set_palette_entry(index, r, g, b);
    drawing_program_ui_color_push_recent_rgb(ctx, r, g, b);
}

void drawing_program_ui_color_apply_hsv_to_active(DrawingProgramAppContext *ctx) {
    uint8_t r = 0u;
    uint8_t g = 0u;
    uint8_t b = 0u;
    if (!ctx) {
        return;
    }
    drawing_program_color_hsv_to_rgb(
        ctx->ui.color_hue, ctx->ui.color_saturation, ctx->ui.color_value, &r, &g, &b);
    drawing_program_ui_color_apply_active_rgb(ctx, r, g, b);
}

void drawing_program_ui_color_seed_defaults(DrawingProgramAppContext *ctx) {
    uint8_t active_r = 0u;
    uint8_t active_g = 0u;
    uint8_t active_b = 0u;
    uint8_t i;
    if (!ctx) {
        return;
    }
    drawing_program_color_reset_palette_defaults();
    drawing_program_color_copy_palette_rgb(ctx->ui.color_palette_rgb);
    ctx->ui.active_color_index = drawing_program_color_default_index();
    drawing_program_color_rgb_from_index(ctx->ui.active_color_index, &active_r, &active_g, &active_b);
    drawing_program_color_rgb_to_hsv(active_r,
                                     active_g,
                                     active_b,
                                     &ctx->ui.color_hue,
                                     &ctx->ui.color_saturation,
                                     &ctx->ui.color_value);
    ctx->ui.recent_color_count = 1u;
    ctx->ui.recent_color_rgb[0][0] = active_r;
    ctx->ui.recent_color_rgb[0][1] = active_g;
    ctx->ui.recent_color_rgb[0][2] = active_b;
    for (i = 1u; i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++i) {
        ctx->ui.recent_color_rgb[i][0] = 0u;
        ctx->ui.recent_color_rgb[i][1] = 0u;
        ctx->ui.recent_color_rgb[i][2] = 0u;
    }
}

void drawing_program_ui_color_sync_selector_from_active(DrawingProgramAppContext *ctx) {
    uint8_t active_index;
    uint8_t active_r = 0u;
    uint8_t active_g = 0u;
    uint8_t active_b = 0u;
    if (!ctx) {
        return;
    }
    active_index = drawing_program_color_index_clamp(ctx->ui.active_color_index);
    active_r = ctx->ui.color_palette_rgb[active_index][0];
    active_g = ctx->ui.color_palette_rgb[active_index][1];
    active_b = ctx->ui.color_palette_rgb[active_index][2];
    drawing_program_color_rgb_to_hsv(active_r,
                                     active_g,
                                     active_b,
                                     &ctx->ui.color_hue,
                                     &ctx->ui.color_saturation,
                                     &ctx->ui.color_value);
}

void drawing_program_ui_color_sync_runtime_palette(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    drawing_program_color_load_palette_rgb(ctx->ui.color_palette_rgb);
}

void drawing_program_ui_color_normalize_state(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    if (drawing_program_ui_color_palette_missing(ctx)) {
        uint8_t preserved_active_color_index = ctx->ui.active_color_index;
        drawing_program_ui_color_seed_defaults(ctx);
        ctx->ui.active_color_index = preserved_active_color_index;
    }
    ctx->ui.active_color_index = drawing_program_color_index_clamp(ctx->ui.active_color_index);
    if (ctx->ui.recent_color_count > DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT) {
        ctx->ui.recent_color_count = DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT;
    }
    drawing_program_ui_color_sync_selector_from_active(ctx);
    drawing_program_ui_color_sync_runtime_palette(ctx);
}
