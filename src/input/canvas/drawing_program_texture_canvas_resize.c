#include "drawing_program/drawing_program_texture_canvas_resize.h"

#include <math.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_texture_project.h"
#include "drawing_program/drawing_program_texture_project_session.h"
#include "drawing_program/drawing_program_texture_workspace.h"
#include "drawing_program/drawing_program_visual_state.h"

#define DRAWING_PROGRAM_TEXTURE_CANVAS_RESIZE_HANDLE_MIN 12
#define DRAWING_PROGRAM_TEXTURE_CANVAS_RESIZE_HANDLE_MAX 18

static int texture_canvas_resize_surface_resizable(const DrawingProgramTextureSurface *surface) {
    return surface && surface->storage && surface->is_blank && !surface->resize_locked;
}

static int texture_canvas_resize_handle_size_for_metrics(const VisualCanvasSheetMetrics *metrics) {
    int size = DRAWING_PROGRAM_TEXTURE_CANVAS_RESIZE_HANDLE_MIN;
    if (!metrics) {
        return size;
    }
    if (metrics->sheet_rect.w > 0 && metrics->sheet_rect.h > 0) {
        int candidate = metrics->sheet_rect.w < metrics->sheet_rect.h ? metrics->sheet_rect.w : metrics->sheet_rect.h;
        candidate /= 8;
        if (candidate > size) {
            size = candidate;
        }
    }
    if (size > DRAWING_PROGRAM_TEXTURE_CANVAS_RESIZE_HANDLE_MAX) {
        size = DRAWING_PROGRAM_TEXTURE_CANVAS_RESIZE_HANDLE_MAX;
    }
    return size;
}

int drawing_program_texture_canvas_resize_handle_rect_for_surface(
    const DrawingProgramAppContext *ctx,
    SDL_Rect pane_rect,
    uint32_t surface_index,
    SDL_Rect *out_rect) {
    const DrawingProgramTextureSurface *surface = 0;
    VisualCanvasSheetMetrics metrics;
    int handle_size;
    if (!ctx || !out_rect) {
        return 0;
    }
    surface = drawing_program_texture_project_surface_at(&ctx->texture_project, surface_index);
    if (!texture_canvas_resize_surface_resizable(surface) ||
        !drawing_program_texture_workspace_surface_sheet_metrics(ctx, pane_rect, surface_index, &metrics)) {
        return 0;
    }
    handle_size = texture_canvas_resize_handle_size_for_metrics(&metrics);
    if (metrics.sheet_rect.w < handle_size || metrics.sheet_rect.h < handle_size) {
        return 0;
    }
    *out_rect = (SDL_Rect){ metrics.sheet_rect.x + metrics.sheet_rect.w - handle_size,
                            metrics.sheet_rect.y + metrics.sheet_rect.h - handle_size,
                            handle_size,
                            handle_size };
    return 1;
}

int drawing_program_texture_canvas_resize_hit_test_handle(const DrawingProgramAppContext *ctx,
                                                          SDL_Rect pane_rect,
                                                          int sx,
                                                          int sy,
                                                          uint32_t *out_surface_index) {
    int32_t index = 0;
    if (!ctx || !out_surface_index) {
        return 0;
    }
    for (index = (int32_t)ctx->texture_project.surface_count - 1; index >= 0; --index) {
        SDL_Rect handle_rect;
        if (!drawing_program_texture_canvas_resize_handle_rect_for_surface(
                ctx, pane_rect, (uint32_t)index, &handle_rect)) {
            continue;
        }
        if (sx >= handle_rect.x &&
            sy >= handle_rect.y &&
            sx < (handle_rect.x + handle_rect.w) &&
            sy < (handle_rect.y + handle_rect.h)) {
            *out_surface_index = (uint32_t)index;
            return 1;
        }
    }
    return 0;
}

int drawing_program_texture_canvas_resize_begin(DrawingProgramAppContext *ctx,
                                                SDL_Rect pane_rect,
                                                VisualCanvasInteractionState *interaction,
                                                uint32_t surface_index,
                                                int sx,
                                                int sy) {
    const DrawingProgramTextureSurface *surface = 0;
    VisualCanvasSheetMetrics metrics;
    float pixels_per_logical = 0.0f;
    if (!ctx || !interaction) {
        return 0;
    }
    surface = drawing_program_texture_project_surface_at(&ctx->texture_project, surface_index);
    if (!texture_canvas_resize_surface_resizable(surface) || !surface->storage) {
        return 0;
    }
    if (!drawing_program_texture_workspace_surface_sheet_metrics(ctx, pane_rect, surface_index, &metrics)) {
        return 0;
    }
    pixels_per_logical = metrics.pixel_size * (float)surface->storage->document.sample_density;
    if (!isfinite(pixels_per_logical) || pixels_per_logical <= 0.0f) {
        return 0;
    }
    interaction->canvas_resize_active = 1u;
    interaction->canvas_resize_surface_index = surface_index;
    interaction->canvas_resize_start_logical_width = surface->storage->document.logical_width;
    interaction->canvas_resize_start_logical_height = surface->storage->document.logical_height;
    interaction->canvas_resize_sample_density = surface->storage->document.sample_density;
    interaction->canvas_resize_pixels_per_logical = pixels_per_logical;
    interaction->canvas_resize_anchor_mouse_x = sx;
    interaction->canvas_resize_anchor_mouse_y = sy;
    return 1;
}

int drawing_program_texture_canvas_resize_update(DrawingProgramAppContext *ctx,
                                                 VisualCanvasInteractionState *interaction,
                                                 int sx,
                                                 int sy) {
    int delta_x;
    int delta_y;
    int width_delta = 0;
    int height_delta = 0;
    uint32_t next_width = 0u;
    uint32_t next_height = 0u;
    if (!ctx || !interaction || !interaction->canvas_resize_active) {
        return 0;
    }
    if (ctx->texture_project.active_surface_index != interaction->canvas_resize_surface_index) {
        return 0;
    }
    if (!isfinite(interaction->canvas_resize_pixels_per_logical) ||
        interaction->canvas_resize_pixels_per_logical <= 0.0f) {
        return 0;
    }
    delta_x = sx - interaction->canvas_resize_anchor_mouse_x;
    delta_y = sy - interaction->canvas_resize_anchor_mouse_y;
    width_delta = (int)lroundf((float)delta_x / interaction->canvas_resize_pixels_per_logical);
    height_delta = (int)lroundf((float)delta_y / interaction->canvas_resize_pixels_per_logical);
    next_width = interaction->canvas_resize_start_logical_width;
    next_height = interaction->canvas_resize_start_logical_height;
    if (width_delta < 0 && (uint32_t)(-width_delta) >= next_width) {
        next_width = 1u;
    } else {
        next_width = (uint32_t)((int32_t)next_width + width_delta);
        if (next_width == 0u) {
            next_width = 1u;
        }
    }
    if (height_delta < 0 && (uint32_t)(-height_delta) >= next_height) {
        next_height = 1u;
    } else {
        next_height = (uint32_t)((int32_t)next_height + height_delta);
        if (next_height == 0u) {
            next_height = 1u;
        }
    }
    if (next_width == ctx->document.logical_width && next_height == ctx->document.logical_height) {
        return 0;
    }
    return drawing_program_texture_project_session_resize_active_blank_surface(ctx, next_width, next_height).code ==
                   CORE_OK
               ? 1
               : 0;
}

void drawing_program_texture_canvas_resize_end(VisualCanvasInteractionState *interaction) {
    if (!interaction) {
        return;
    }
    interaction->canvas_resize_active = 0u;
    interaction->canvas_resize_surface_index = 0u;
    interaction->canvas_resize_start_logical_width = 0u;
    interaction->canvas_resize_start_logical_height = 0u;
    interaction->canvas_resize_sample_density = 0u;
    interaction->canvas_resize_pixels_per_logical = 0.0f;
    interaction->canvas_resize_anchor_mouse_x = 0;
    interaction->canvas_resize_anchor_mouse_y = 0;
}
