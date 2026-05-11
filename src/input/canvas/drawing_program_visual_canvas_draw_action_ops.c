#include "drawing_program/drawing_program_visual_canvas_draw_action_ops.h"

#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_visual_canvas_stroke_ops.h"

enum {
    DRAWING_PROGRAM_DIRECT_STROKE_HISTORY_GROUP_STEP_LIMIT = 256u
};

static void drawing_program_visual_note_direct_stroke_step(
    DrawingProgramAppContext *ctx,
    uint32_t active_layer_id,
    VisualCanvasInteractionState *state,
    const DrawingProgramVisualCanvasDrawActionOpsHooks *hooks) {
    if (!ctx || !state) {
        return;
    }
    state->direct_stroke_group_step_count =
        (uint16_t)(state->direct_stroke_group_step_count + 1u);
    if (state->direct_stroke_group_step_count <
            (uint16_t)DRAWING_PROGRAM_DIRECT_STROKE_HISTORY_GROUP_STEP_LIMIT ||
        !hooks ||
        !hooks->end_canvas_history_group ||
        !hooks->begin_canvas_history_group) {
        return;
    }
    (void)drawing_program_visual_flush_direct_stroke_history(ctx, state, active_layer_id);
    hooks->end_canvas_history_group(ctx);
    hooks->begin_canvas_history_group(ctx);
    state->direct_stroke_group_step_count = 0u;
}

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
    DrawingProgramRasterSample value;
    CoreResult result;
    if (!ctx || !state || !hooks || !hooks->screen_to_canvas_sample || !hooks->active_layer_allows_edits_visual ||
        !hooks->active_layer_query || !hooks->sample_value_for_tool || !hooks->tool_brush_radius_samples ||
        !hooks->tool_brush_spacing_samples || !hooks->tool_brush_hardness_percent ||
        !hooks->seeded_background_sample_for_coord || !hooks->apply_canvas_stamp_square_on_layer ||
        !hooks->apply_canvas_direct_stroke_stamp_square_on_layer) {
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
        DrawingProgramCanvasReflectionPoint points[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY];
        uint32_t point_count = drawing_program_canvas_reflection_collect_points(
            ctx, (int32_t)sample_x, (int32_t)sample_y, points);
        uint32_t point_index = 0u;
        if (point_count == 0u) {
            point_count = 1u;
            points[0].x = (int32_t)sample_x;
            points[0].y = (int32_t)sample_y;
        }
        for (point_index = 0u; point_index < point_count; ++point_index) {
            DrawingProgramRasterSample write_value = value;
            if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_ERASER) {
                write_value = hooks->seeded_background_sample_for_coord(
                    &ctx->document, (uint32_t)points[point_index].x, (uint32_t)points[point_index].y);
                hardness = 100u;
            }
            result = hooks->apply_canvas_direct_stroke_stamp_square_on_layer(ctx,
                                                                             state,
                                                                             active_layer_id,
                                                                             points[point_index].x,
                                                                             points[point_index].y,
                                                                             write_value,
                                                                             stamp_width,
                                                                             hardness);
            if (result.code != CORE_OK) {
                return result;
            }
        }
        state->has_last_sample = 1u;
        state->last_sample_x = sample_x;
        state->last_sample_y = sample_y;
        drawing_program_visual_note_direct_stroke_step(ctx, active_layer_id, state, hooks);
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
            DrawingProgramCanvasReflectionPoint points[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY];
            uint32_t point_count = 0u;
            uint32_t point_index = 0u;
            if (((uint32_t)i % stamp_every) != 0u && i != steps) {
                continue;
            }
            point_count = drawing_program_canvas_reflection_collect_points(ctx, ix, iy, points);
            if (point_count == 0u) {
                point_count = 1u;
                points[0].x = ix;
                points[0].y = iy;
            }
            for (point_index = 0u; point_index < point_count; ++point_index) {
                DrawingProgramRasterSample write_value = value;
                if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_ERASER) {
                    write_value = hooks->seeded_background_sample_for_coord(
                        &ctx->document, (uint32_t)points[point_index].x, (uint32_t)points[point_index].y);
                    hardness = 100u;
                }
                result = hooks->apply_canvas_direct_stroke_stamp_square_on_layer(ctx,
                                                                                 state,
                                                                                 active_layer_id,
                                                                                 points[point_index].x,
                                                                                 points[point_index].y,
                                                                                 write_value,
                                                                                 stamp_width,
                                                                                 hardness);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
            drawing_program_visual_note_direct_stroke_step(ctx, active_layer_id, state, hooks);
        }
    }

    state->has_last_sample = 1u;
    state->last_sample_x = sample_x;
    state->last_sample_y = sample_y;
    return core_result_ok();
}
