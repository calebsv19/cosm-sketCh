#include "drawing_program/drawing_program_visual_canvas_draw_action_ops.h"

CoreResult drawing_program_visual_apply_canvas_draw_at_screen(DrawingProgramAppContext *ctx,
                                                              SDL_Rect pane_rect,
                                                              int sx,
                                                              int sy,
                                                              VisualCanvasInteractionState *state,
                                                              const DrawingProgramVisualCanvasDrawActionOpsHooks *hooks) {
    uint32_t sample_x;
    uint32_t sample_y;
    uint32_t active_layer_id = 0u;
    uint32_t radius;
    uint32_t spacing;
    uint32_t stamp_width;
    uint8_t hardness;
    uint8_t value;
    CoreResult result;
    if (!ctx || !state || !hooks || !hooks->screen_to_canvas_sample || !hooks->active_layer_allows_edits_visual ||
        !hooks->active_layer_query || !hooks->sample_value_for_tool || !hooks->tool_brush_radius_samples ||
        !hooks->tool_brush_spacing_samples || !hooks->tool_brush_hardness_percent ||
        !hooks->seeded_background_sample_for_coord || !hooks->apply_canvas_stamp_square_on_layer) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid canvas draw request" };
    }
    if (!hooks->screen_to_canvas_sample(ctx, pane_rect, sx, sy, &sample_x, &sample_y)) {
        return core_result_ok();
    }
    if (!hooks->active_layer_allows_edits_visual(ctx)) {
        return core_result_ok();
    }
    if (!hooks->active_layer_query(ctx, &active_layer_id, 0, 0, 0) || active_layer_id == 0u) {
        return core_result_ok();
    }

    value = hooks->sample_value_for_tool(ctx, ctx->editor.active_tool);
    radius = hooks->tool_brush_radius_samples(ctx, ctx->editor.active_tool);
    spacing = hooks->tool_brush_spacing_samples(ctx, ctx->editor.active_tool, radius);
    hardness = hooks->tool_brush_hardness_percent(ctx, ctx->editor.active_tool);
    stamp_width = (radius * 2u) + 1u;

    if (!state->has_last_sample) {
        uint8_t write_value = value;
        if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_ERASER) {
            write_value = hooks->seeded_background_sample_for_coord(&ctx->document, sample_x, sample_y);
            hardness = 100u;
        }
        result = hooks->apply_canvas_stamp_square_on_layer(ctx,
                                                           active_layer_id,
                                                           (int32_t)sample_x,
                                                           (int32_t)sample_y,
                                                           write_value,
                                                           stamp_width,
                                                           hardness);
        if (result.code != CORE_OK) {
            return result;
        }
        state->has_last_sample = 1u;
        state->last_sample_x = sample_x;
        state->last_sample_y = sample_y;
        return core_result_ok();
    }

    if (state->last_sample_x == sample_x &&
        state->last_sample_y == sample_y) {
        return core_result_ok();
    }

    {
        int32_t x0 = (int32_t)state->last_sample_x;
        int32_t y0 = (int32_t)state->last_sample_y;
        int32_t x1 = (int32_t)sample_x;
        int32_t y1 = (int32_t)sample_y;
        int32_t dx = x1 - x0;
        int32_t dy = y1 - y0;
        int32_t adx = (dx < 0) ? -dx : dx;
        int32_t ady = (dy < 0) ? -dy : dy;
        int32_t steps = (adx > ady) ? adx : ady;
        uint32_t stamp_every = (spacing < 1u) ? 1u : spacing;
        int32_t i = 0;
        for (i = 1; i <= steps; ++i) {
            int32_t ix = x0 + (dx * i) / ((steps > 0) ? steps : 1);
            int32_t iy = y0 + (dy * i) / ((steps > 0) ? steps : 1);
            uint8_t write_value = value;
            if (((uint32_t)i % stamp_every) != 0u && i != steps) {
                continue;
            }
            if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_ERASER) {
                write_value = hooks->seeded_background_sample_for_coord(&ctx->document, (uint32_t)ix, (uint32_t)iy);
                hardness = 100u;
            }
            result = hooks->apply_canvas_stamp_square_on_layer(ctx,
                                                               active_layer_id,
                                                               ix,
                                                               iy,
                                                               write_value,
                                                               stamp_width,
                                                               hardness);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    }

    state->has_last_sample = 1u;
    state->last_sample_x = sample_x;
    state->last_sample_y = sample_y;
    return core_result_ok();
}
