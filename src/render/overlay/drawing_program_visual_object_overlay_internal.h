#ifndef DRAWING_PROGRAM_VISUAL_OBJECT_OVERLAY_INTERNAL_H
#define DRAWING_PROGRAM_VISUAL_OBJECT_OVERLAY_INTERNAL_H

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_visual_overlay_render.h"
#include "drawing_program/drawing_program_visual_overlay_shared.h"

static inline SDL_Color drawing_program_visual_object_overlay_color_from_sample(
    DrawingProgramRasterSample sample) {
    SDL_Color color = {0u, 0u, 0u, 0u};
    drawing_program_color_rgba_from_sample(sample, &color.r, &color.g, &color.b, &color.a);
    return color;
}

static inline int drawing_program_visual_object_style_includes_fill(uint8_t style_mode) {
    return (style_mode == 1u || style_mode == 2u) ? 1 : 0;
}

static inline int drawing_program_visual_object_style_includes_outline(uint8_t style_mode) {
    return (style_mode == 1u) ? 0 : 1;
}

static inline int drawing_program_visual_object_overlay_draw_sample_cell(
    SDL_Renderer *renderer,
    const VisualCanvasSheetMetrics *metrics,
    int32_t sample_x,
    int32_t sample_y,
    SDL_Color color,
    uint8_t alpha) {
    SDL_Rect sample_rect;
    if (!renderer || !metrics) {
        return 0;
    }
    if (!drawing_program_visual_overlay_sample_rect_to_screen_rect(metrics, sample_x, sample_y, 1u, 1u, &sample_rect)) {
        return 0;
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
    (void)SDL_RenderFillRect(renderer, &sample_rect);
    return 1;
}

void drawing_program_visual_object_overlay_draw_rect(
    SDL_Renderer *renderer,
    const DrawingProgramAppContext *ctx,
    const VisualCanvasSheetMetrics *metrics,
    const DrawingProgramObjectRecord *object,
    int32_t origin_x,
    int32_t origin_y,
    uint8_t alpha);

void drawing_program_visual_object_overlay_draw_ellipse(
    SDL_Renderer *renderer,
    const DrawingProgramAppContext *ctx,
    const VisualCanvasSheetMetrics *metrics,
    const DrawingProgramObjectRecord *object,
    int32_t origin_x,
    int32_t origin_y,
    uint8_t alpha);

void drawing_program_visual_object_overlay_draw_path(
    SDL_Renderer *renderer,
    const DrawingProgramAppContext *ctx,
    const VisualCanvasSheetMetrics *metrics,
    const DrawingProgramObjectRecord *object,
    const VisualCanvasInteractionState *interaction,
    int32_t origin_x,
    int32_t origin_y,
    uint8_t alpha);

void drawing_program_visual_object_overlay_draw_selected_path_point_handles(
    SDL_Renderer *renderer,
    const DrawingProgramAppContext *ctx,
    const VisualCanvasSheetMetrics *metrics,
    const DrawingProgramObjectRecord *object,
    const VisualCanvasInteractionState *interaction,
    int show_handles);

void drawing_program_visual_object_overlay_draw_selected_bezier_point_handles(
    SDL_Renderer *renderer,
    const DrawingProgramAppContext *ctx,
    const VisualCanvasSheetMetrics *metrics,
    const DrawingProgramObjectRecord *object,
    const VisualCanvasInteractionState *interaction,
    int32_t draw_origin_x,
    int32_t draw_origin_y);

void drawing_program_visual_object_overlay_draw_active_path_point_edge_preview(
    SDL_Renderer *renderer,
    const VisualCanvasSheetMetrics *metrics,
    const DrawingProgramObjectRecord *object,
    const VisualCanvasInteractionState *interaction,
    int32_t draw_origin_x,
    int32_t draw_origin_y);

#endif
