#include "drawing_program/drawing_program_visual_layer_opacity.h"

static uint8_t clamp_percent_u8(uint8_t value) {
    if (value > 100u) {
        return 100u;
    }
    return value;
}

static void visual_layer_opacity_compact(DrawingProgramAppContext *ctx) {
    uint32_t i;
    uint32_t compact_count = 0u;
    if (!ctx) {
        return;
    }
    for (i = 0u; i < ctx->ui.layer_opacity_entry_count; ++i) {
        uint32_t layer_id = ctx->ui.layer_opacity_layer_ids[i];
        uint32_t j;
        int layer_exists = 0;
        for (j = 0u; j < ctx->document.layer_count; ++j) {
            if (ctx->document.layers[j].layer_id == layer_id) {
                layer_exists = 1;
                break;
            }
        }
        if (layer_exists) {
            ctx->ui.layer_opacity_layer_ids[compact_count] = layer_id;
            ctx->ui.layer_opacity_values[compact_count] =
                clamp_percent_u8(ctx->ui.layer_opacity_values[i]);
            compact_count += 1u;
        }
    }
    ctx->ui.layer_opacity_entry_count = compact_count;
}

uint8_t drawing_program_visual_layer_opacity_get(const DrawingProgramAppContext *ctx, uint32_t layer_id) {
    uint32_t i;
    if (!ctx) {
        return 100u;
    }
    for (i = 0u; i < ctx->ui.layer_opacity_entry_count; ++i) {
        if (ctx->ui.layer_opacity_layer_ids[i] == layer_id) {
            return clamp_percent_u8(ctx->ui.layer_opacity_values[i]);
        }
    }
    return 100u;
}

void drawing_program_visual_layer_opacity_set(DrawingProgramAppContext *ctx,
                                              uint32_t layer_id,
                                              uint8_t opacity_percent) {
    uint32_t i;
    uint8_t opacity = clamp_percent_u8(opacity_percent);
    if (!ctx || layer_id == 0u) {
        return;
    }
    for (i = 0u; i < ctx->ui.layer_opacity_entry_count; ++i) {
        if (ctx->ui.layer_opacity_layer_ids[i] == layer_id) {
            ctx->ui.layer_opacity_values[i] = opacity;
            return;
        }
    }
    if (ctx->ui.layer_opacity_entry_count >= DRAWING_PROGRAM_MAX_LAYERS) {
        return;
    }
    ctx->ui.layer_opacity_layer_ids[ctx->ui.layer_opacity_entry_count] = layer_id;
    ctx->ui.layer_opacity_values[ctx->ui.layer_opacity_entry_count] = opacity;
    ctx->ui.layer_opacity_entry_count += 1u;
}

void drawing_program_visual_layer_opacity_sync_document(DrawingProgramAppContext *ctx) {
    uint32_t i;
    if (!ctx) {
        return;
    }
    visual_layer_opacity_compact(ctx);
    for (i = 0u; i < ctx->document.layer_count; ++i) {
        drawing_program_visual_layer_opacity_set(ctx, ctx->document.layers[i].layer_id, 100u);
    }
}

void drawing_program_visual_collect_layer_opacity_by_index(const DrawingProgramAppContext *ctx,
                                                           uint8_t *out_opacity,
                                                           uint32_t out_count) {
    uint32_t i;
    if (!ctx || !out_opacity) {
        return;
    }
    for (i = 0u; i < out_count; ++i) {
        uint8_t opacity = 100u;
        if (i < ctx->document.layer_count) {
            opacity = drawing_program_visual_layer_opacity_get(ctx, ctx->document.layers[i].layer_id);
        }
        out_opacity[i] = opacity;
    }
}
