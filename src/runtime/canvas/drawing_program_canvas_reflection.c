#include "drawing_program/drawing_program_canvas_reflection.h"

#include <string.h>

static DrawingProgramTextureSurface *active_surface_mut(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return 0;
    }
    return drawing_program_texture_project_surface_at_mut(&ctx->texture_project, ctx->texture_project.active_surface_index);
}

static const DrawingProgramTextureSurface *active_surface(const DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return 0;
    }
    return drawing_program_texture_project_surface_at(&ctx->texture_project, ctx->texture_project.active_surface_index);
}

static uint32_t clamp_sample_axis(uint32_t axis, uint32_t limit);

static void fallback_center_for_context(const DrawingProgramAppContext *ctx,
                                        uint32_t *out_x,
                                        uint32_t *out_y) {
    if (!ctx) {
        if (out_x) {
            *out_x = 0u;
        }
        if (out_y) {
            *out_y = 0u;
        }
        return;
    }
    if (ctx->ui.canvas_reflection_center_valid) {
        if (out_x) {
            *out_x = clamp_sample_axis(ctx->ui.canvas_reflection_center_x, ctx->document.raster_width);
        }
        if (out_y) {
            *out_y = clamp_sample_axis(ctx->ui.canvas_reflection_center_y, ctx->document.raster_height);
        }
        return;
    }
    drawing_program_canvas_reflection_default_center_for_document(&ctx->document, out_x, out_y);
}

static uint32_t clamp_sample_axis(uint32_t axis, uint32_t limit) {
    if (limit == 0u) {
        return 0u;
    }
    if (axis >= limit) {
        return limit - 1u;
    }
    return axis;
}

static int32_t reflect_axis(int32_t value, int32_t center) {
    return (center * 2) - value;
}

static uint32_t reflection_variant_count(uint8_t horizontal_enabled,
                                         uint8_t vertical_enabled,
                                         uint8_t variant_masks[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY]) {
    uint32_t count = 0u;
    if (!variant_masks) {
        return 0u;
    }
    variant_masks[count++] = 0u;
    if (horizontal_enabled) {
        variant_masks[count++] = 1u;
    }
    if (vertical_enabled) {
        variant_masks[count++] = 2u;
    }
    if (horizontal_enabled && vertical_enabled) {
        variant_masks[count++] = 3u;
    }
    return count;
}

static void transform_point(uint8_t variant_mask,
                            int32_t center_x,
                            int32_t center_y,
                            int32_t src_x,
                            int32_t src_y,
                            int32_t *out_x,
                            int32_t *out_y) {
    int32_t next_x = src_x;
    int32_t next_y = src_y;
    if (variant_mask & 2u) {
        next_x = reflect_axis(src_x, center_x);
    }
    if (variant_mask & 1u) {
        next_y = reflect_axis(src_y, center_y);
    }
    if (out_x) {
        *out_x = next_x;
    }
    if (out_y) {
        *out_y = next_y;
    }
}

void drawing_program_canvas_reflection_default_center_for_document(const DrawingProgramDocument *document,
                                                                   uint32_t *out_x,
                                                                   uint32_t *out_y) {
    uint32_t center_x = 0u;
    uint32_t center_y = 0u;
    if (document) {
        if (document->raster_width > 0u) {
            center_x = (document->raster_width - 1u) / 2u;
        }
        if (document->raster_height > 0u) {
            center_y = (document->raster_height - 1u) / 2u;
        }
    }
    if (out_x) {
        *out_x = center_x;
    }
    if (out_y) {
        *out_y = center_y;
    }
}

void drawing_program_canvas_reflection_surface_seed_defaults(DrawingProgramTextureSurface *surface,
                                                             const DrawingProgramDocument *document) {
    uint32_t center_x = 0u;
    uint32_t center_y = 0u;
    if (!surface) {
        return;
    }
    drawing_program_canvas_reflection_default_center_for_document(document, &center_x, &center_y);
    surface->reflection_center_x = center_x;
    surface->reflection_center_y = center_y;
    surface->reflection_horizontal = 0u;
    surface->reflection_vertical = 0u;
}

void drawing_program_canvas_reflection_surface_clamp_to_document(DrawingProgramTextureSurface *surface,
                                                                 const DrawingProgramDocument *document) {
    uint32_t center_x = 0u;
    uint32_t center_y = 0u;
    if (!surface || !document) {
        return;
    }
    drawing_program_canvas_reflection_default_center_for_document(document, &center_x, &center_y);
    if (document->raster_width == 0u || document->raster_height == 0u) {
        surface->reflection_center_x = center_x;
        surface->reflection_center_y = center_y;
        surface->reflection_horizontal = 0u;
        surface->reflection_vertical = 0u;
        return;
    }
    surface->reflection_center_x = clamp_sample_axis(surface->reflection_center_x, document->raster_width);
    surface->reflection_center_y = clamp_sample_axis(surface->reflection_center_y, document->raster_height);
}

void drawing_program_canvas_reflection_sync_editor_from_active_surface(DrawingProgramAppContext *ctx) {
    DrawingProgramTextureSurface *surface = active_surface_mut(ctx);
    if (!ctx) {
        return;
    }
    if (!surface) {
        uint32_t center_x = 0u;
        uint32_t center_y = 0u;
        fallback_center_for_context(ctx, &center_x, &center_y);
        ctx->ui.canvas_reflection_center_x = center_x;
        ctx->ui.canvas_reflection_center_y = center_y;
        ctx->ui.canvas_reflection_center_valid = 1u;
        return;
    }
    drawing_program_canvas_reflection_surface_clamp_to_document(surface, &ctx->document);
    ctx->editor.symmetry_horizontal = surface->reflection_horizontal ? 1u : 0u;
    ctx->editor.symmetry_vertical = surface->reflection_vertical ? 1u : 0u;
}

void drawing_program_canvas_reflection_sync_active_surface_from_editor(DrawingProgramAppContext *ctx) {
    DrawingProgramTextureSurface *surface = active_surface_mut(ctx);
    if (!ctx) {
        return;
    }
    if (!surface) {
        uint32_t center_x = 0u;
        uint32_t center_y = 0u;
        fallback_center_for_context(ctx, &center_x, &center_y);
        ctx->ui.canvas_reflection_center_x = center_x;
        ctx->ui.canvas_reflection_center_y = center_y;
        ctx->ui.canvas_reflection_center_valid = 1u;
        return;
    }
    drawing_program_canvas_reflection_surface_clamp_to_document(surface, &ctx->document);
    surface->reflection_horizontal = ctx->editor.symmetry_horizontal ? 1u : 0u;
    surface->reflection_vertical = ctx->editor.symmetry_vertical ? 1u : 0u;
}

int drawing_program_canvas_reflection_active_center(const DrawingProgramAppContext *ctx,
                                                    uint32_t *out_x,
                                                    uint32_t *out_y) {
    const DrawingProgramTextureSurface *surface = active_surface(ctx);
    uint32_t center_x = 0u;
    uint32_t center_y = 0u;
    if (!ctx || ctx->document.raster_width == 0u || ctx->document.raster_height == 0u) {
        return 0;
    }
    if (surface) {
        center_x = clamp_sample_axis(surface->reflection_center_x, ctx->document.raster_width);
        center_y = clamp_sample_axis(surface->reflection_center_y, ctx->document.raster_height);
    } else {
        fallback_center_for_context(ctx, &center_x, &center_y);
    }
    if (out_x) {
        *out_x = center_x;
    }
    if (out_y) {
        *out_y = center_y;
    }
    return 1;
}

void drawing_program_canvas_reflection_reset_active_center(DrawingProgramAppContext *ctx) {
    DrawingProgramTextureSurface *surface = active_surface_mut(ctx);
    if (!ctx) {
        return;
    }
    if (surface) {
        drawing_program_canvas_reflection_default_center_for_document(
            &ctx->document, &surface->reflection_center_x, &surface->reflection_center_y);
        return;
    }
    drawing_program_canvas_reflection_default_center_for_document(
        &ctx->document, &ctx->ui.canvas_reflection_center_x, &ctx->ui.canvas_reflection_center_y);
    ctx->ui.canvas_reflection_center_valid = 1u;
}

int drawing_program_canvas_reflection_set_active_center(DrawingProgramAppContext *ctx,
                                                        uint32_t sample_x,
                                                        uint32_t sample_y) {
    DrawingProgramTextureSurface *surface = active_surface_mut(ctx);
    if (!ctx || ctx->document.raster_width == 0u || ctx->document.raster_height == 0u) {
        return 0;
    }
    if (surface) {
        surface->reflection_center_x = clamp_sample_axis(sample_x, ctx->document.raster_width);
        surface->reflection_center_y = clamp_sample_axis(sample_y, ctx->document.raster_height);
        return 1;
    }
    ctx->ui.canvas_reflection_center_x = clamp_sample_axis(sample_x, ctx->document.raster_width);
    ctx->ui.canvas_reflection_center_y = clamp_sample_axis(sample_y, ctx->document.raster_height);
    ctx->ui.canvas_reflection_center_valid = 1u;
    return 1;
}

int drawing_program_canvas_reflection_enabled(const DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return 0;
    }
    return (ctx->editor.symmetry_horizontal || ctx->editor.symmetry_vertical) ? 1 : 0;
}

uint32_t drawing_program_canvas_reflection_collect_points(
    const DrawingProgramAppContext *ctx,
    int32_t sample_x,
    int32_t sample_y,
    DrawingProgramCanvasReflectionPoint out_points[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY]) {
    uint8_t variant_masks[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY];
    uint32_t center_x = 0u;
    uint32_t center_y = 0u;
    uint32_t variant_count = 0u;
    uint32_t out_count = 0u;
    uint32_t variant_index;
    if (!out_points || !ctx || !drawing_program_canvas_reflection_active_center(ctx, &center_x, &center_y)) {
        return 0u;
    }
    memset(out_points, 0, sizeof(*out_points) * DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY);
    variant_count = reflection_variant_count(ctx->editor.symmetry_horizontal ? 1u : 0u,
                                             ctx->editor.symmetry_vertical ? 1u : 0u,
                                             variant_masks);
    for (variant_index = 0u; variant_index < variant_count; ++variant_index) {
        int32_t next_x = 0;
        int32_t next_y = 0;
        uint32_t existing_index = 0u;
        int duplicate = 0;
        transform_point(variant_masks[variant_index],
                        (int32_t)center_x,
                        (int32_t)center_y,
                        sample_x,
                        sample_y,
                        &next_x,
                        &next_y);
        for (existing_index = 0u; existing_index < out_count; ++existing_index) {
            if (out_points[existing_index].x == next_x && out_points[existing_index].y == next_y) {
                duplicate = 1;
                break;
            }
        }
        if (!duplicate && out_count < DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY) {
            out_points[out_count].x = next_x;
            out_points[out_count].y = next_y;
            out_count += 1u;
        }
    }
    return out_count;
}

uint32_t drawing_program_canvas_reflection_collect_segments(
    const DrawingProgramAppContext *ctx,
    int32_t start_x,
    int32_t start_y,
    int32_t end_x,
    int32_t end_y,
    DrawingProgramCanvasReflectionSegment out_segments[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY]) {
    uint8_t variant_masks[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY];
    uint32_t center_x = 0u;
    uint32_t center_y = 0u;
    uint32_t variant_count = 0u;
    uint32_t out_count = 0u;
    uint32_t variant_index;
    if (!out_segments || !ctx || !drawing_program_canvas_reflection_active_center(ctx, &center_x, &center_y)) {
        return 0u;
    }
    memset(out_segments, 0, sizeof(*out_segments) * DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY);
    variant_count = reflection_variant_count(ctx->editor.symmetry_horizontal ? 1u : 0u,
                                             ctx->editor.symmetry_vertical ? 1u : 0u,
                                             variant_masks);
    for (variant_index = 0u; variant_index < variant_count; ++variant_index) {
        DrawingProgramCanvasReflectionSegment next = {0};
        uint32_t existing_index = 0u;
        int duplicate = 0;
        transform_point(variant_masks[variant_index],
                        (int32_t)center_x,
                        (int32_t)center_y,
                        start_x,
                        start_y,
                        &next.start_x,
                        &next.start_y);
        transform_point(variant_masks[variant_index],
                        (int32_t)center_x,
                        (int32_t)center_y,
                        end_x,
                        end_y,
                        &next.end_x,
                        &next.end_y);
        for (existing_index = 0u; existing_index < out_count; ++existing_index) {
            if (out_segments[existing_index].start_x == next.start_x &&
                out_segments[existing_index].start_y == next.start_y &&
                out_segments[existing_index].end_x == next.end_x &&
                out_segments[existing_index].end_y == next.end_y) {
                duplicate = 1;
                break;
            }
        }
        if (!duplicate && out_count < DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY) {
            out_segments[out_count] = next;
            out_count += 1u;
        }
    }
    return out_count;
}
