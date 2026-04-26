#include "drawing_program/drawing_program_ui_color_state.h"

static uint8_t drawing_program_ui_color_recent_slot_clamp(uint8_t slot) {
    return (slot < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT) ? slot : 0u;
}

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

static void drawing_program_ui_color_seed_recent_bank_from_palette(DrawingProgramAppContext *ctx) {
    uint8_t i;
    if (!ctx) {
        return;
    }
    for (i = 0u; i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++i) {
        ctx->ui.recent_color_rgb[i][0] = ctx->ui.color_palette_rgb[i][0];
        ctx->ui.recent_color_rgb[i][1] = ctx->ui.color_palette_rgb[i][1];
        ctx->ui.recent_color_rgb[i][2] = ctx->ui.color_palette_rgb[i][2];
    }
    ctx->ui.recent_color_count = DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT;
}

void drawing_program_ui_color_select_recent_slot(DrawingProgramAppContext *ctx, uint8_t recent_index) {
    uint8_t slot;
    if (!ctx) {
        return;
    }
    slot = drawing_program_ui_color_recent_slot_clamp(recent_index);
    ctx->ui.selected_recent_color_index = slot;
    drawing_program_ui_color_set_active_paint_rgb(ctx,
                                                  ctx->ui.recent_color_rgb[slot][0],
                                                  ctx->ui.recent_color_rgb[slot][1],
                                                  ctx->ui.recent_color_rgb[slot][2],
                                                  0u);
}

void drawing_program_ui_color_push_recent_rgb(DrawingProgramAppContext *ctx, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t target_index;
    if (!ctx) {
        return;
    }
    target_index = drawing_program_ui_color_recent_slot_clamp(ctx->ui.selected_recent_color_index);
    ctx->ui.recent_color_count = DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT;
    ctx->ui.recent_color_rgb[target_index][0] = r;
    ctx->ui.recent_color_rgb[target_index][1] = g;
    ctx->ui.recent_color_rgb[target_index][2] = b;
}

void drawing_program_ui_color_sync_selector_from_paint(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    drawing_program_color_rgb_to_hsv(ctx->ui.active_paint_r,
                                     ctx->ui.active_paint_g,
                                     ctx->ui.active_paint_b,
                                     &ctx->ui.color_hue,
                                     &ctx->ui.color_saturation,
                                     &ctx->ui.color_value);
}

void drawing_program_ui_color_set_active_paint_rgb(
    DrawingProgramAppContext *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t push_recent) {
    if (!ctx) {
        return;
    }
    ctx->ui.active_paint_r = r;
    ctx->ui.active_paint_g = g;
    ctx->ui.active_paint_b = b;
    drawing_program_ui_color_sync_selector_from_paint(ctx);
    if (push_recent) {
        drawing_program_ui_color_push_recent_rgb(ctx, r, g, b);
    }
}

void drawing_program_ui_color_set_active_paint_sample(
    DrawingProgramAppContext *ctx, DrawingProgramRasterSample sample, uint8_t push_recent) {
    uint8_t r = 0u;
    uint8_t g = 0u;
    uint8_t b = 0u;
    drawing_program_color_rgb_from_sample(sample, &r, &g, &b);
    drawing_program_ui_color_set_active_paint_rgb(ctx, r, g, b, push_recent);
}

void drawing_program_ui_color_load_active_paint_from_swatch(DrawingProgramAppContext *ctx, uint8_t swatch_index) {
    uint8_t index;
    if (!ctx) {
        return;
    }
    index = drawing_program_color_index_clamp(swatch_index);
    ctx->ui.active_color_index = index;
    drawing_program_ui_color_set_active_paint_rgb(ctx,
                                                  ctx->ui.color_palette_rgb[index][0],
                                                  ctx->ui.color_palette_rgb[index][1],
                                                  ctx->ui.color_palette_rgb[index][2],
                                                  1u);
}

void drawing_program_ui_color_save_active_paint_to_swatch(DrawingProgramAppContext *ctx, uint8_t swatch_index) {
    uint8_t index;
    if (!ctx) {
        return;
    }
    index = drawing_program_color_index_clamp(swatch_index);
    ctx->ui.active_color_index = index;
    ctx->ui.color_palette_rgb[index][0] = ctx->ui.active_paint_r;
    ctx->ui.color_palette_rgb[index][1] = ctx->ui.active_paint_g;
    ctx->ui.color_palette_rgb[index][2] = ctx->ui.active_paint_b;
    drawing_program_color_set_palette_entry(index,
                                            ctx->ui.active_paint_r,
                                            ctx->ui.active_paint_g,
                                            ctx->ui.active_paint_b);
    drawing_program_ui_color_sync_runtime_palette(ctx);
}

void drawing_program_ui_color_apply_hsv_to_paint(DrawingProgramAppContext *ctx, uint8_t push_recent) {
    uint8_t r = 0u;
    uint8_t g = 0u;
    uint8_t b = 0u;
    if (!ctx) {
        return;
    }
    drawing_program_color_hsv_to_rgb(
        ctx->ui.color_hue, ctx->ui.color_saturation, ctx->ui.color_value, &r, &g, &b);
    drawing_program_ui_color_set_active_paint_rgb(ctx, r, g, b, push_recent);
}

uint8_t drawing_program_ui_color_active_paint_index(const DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return drawing_program_color_default_index();
    }
    return drawing_program_color_index_from_rgb(
        ctx->ui.active_paint_r, ctx->ui.active_paint_g, ctx->ui.active_paint_b);
}

DrawingProgramRasterSample drawing_program_ui_color_active_paint_sample_value(
    const DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return drawing_program_color_value_from_index(drawing_program_color_default_index());
    }
    return drawing_program_color_value_from_rgb(ctx->ui.active_paint_r, ctx->ui.active_paint_g, ctx->ui.active_paint_b);
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
    ctx->ui.selected_recent_color_index = ctx->ui.active_color_index;
    drawing_program_color_rgb_from_index(ctx->ui.active_color_index, &active_r, &active_g, &active_b);
    drawing_program_ui_color_set_active_paint_rgb(ctx, active_r, active_g, active_b, 0u);
    for (i = 0u; i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++i) {
        ctx->ui.recent_color_rgb[i][0] = ctx->ui.color_palette_rgb[i][0];
        ctx->ui.recent_color_rgb[i][1] = ctx->ui.color_palette_rgb[i][1];
        ctx->ui.recent_color_rgb[i][2] = ctx->ui.color_palette_rgb[i][2];
    }
    ctx->ui.recent_color_count = DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT;
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
        drawing_program_ui_color_load_active_paint_from_swatch(ctx, preserved_active_color_index);
    }
    ctx->ui.active_color_index = drawing_program_color_index_clamp(ctx->ui.active_color_index);
    ctx->ui.selected_recent_color_index =
        drawing_program_ui_color_recent_slot_clamp(ctx->ui.selected_recent_color_index);
    if (ctx->ui.recent_color_count == 0u) {
        drawing_program_ui_color_seed_recent_bank_from_palette(ctx);
    } else if (ctx->ui.recent_color_count < DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT) {
        uint8_t i;
        for (i = ctx->ui.recent_color_count; i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++i) {
            ctx->ui.recent_color_rgb[i][0] = ctx->ui.color_palette_rgb[i][0];
            ctx->ui.recent_color_rgb[i][1] = ctx->ui.color_palette_rgb[i][1];
            ctx->ui.recent_color_rgb[i][2] = ctx->ui.color_palette_rgb[i][2];
        }
        ctx->ui.recent_color_count = DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT;
    } else if (ctx->ui.recent_color_count > DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT) {
        ctx->ui.recent_color_count = DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT;
    }
    drawing_program_ui_color_sync_selector_from_paint(ctx);
    drawing_program_ui_color_sync_runtime_palette(ctx);
}
