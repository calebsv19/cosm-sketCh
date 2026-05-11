#include "drawing_program/drawing_program_texture_canvas_move.h"

#include <math.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_texture_project.h"
#include "drawing_program/drawing_program_texture_project_session.h"
#include "drawing_program/drawing_program_texture_workspace.h"
#include "drawing_program/drawing_program_viewport.h"

static int texture_canvas_move_surface_movable(const DrawingProgramTextureSurface *surface) {
    return surface && surface->storage;
}

int drawing_program_texture_canvas_move_begin(const DrawingProgramAppContext *ctx,
                                              SDL_Rect pane_rect,
                                              VisualCanvasInteractionState *interaction,
                                              uint32_t surface_index,
                                              int sx,
                                              int sy) {
    const DrawingProgramTextureSurface *surface = 0;
    VisualCanvasSheetMetrics metrics;
    float zoom = 0.0f;
    if (!ctx || !interaction) {
        return 0;
    }
    surface = drawing_program_texture_project_surface_at(&ctx->texture_project, surface_index);
    if (!texture_canvas_move_surface_movable(surface) ||
        !drawing_program_texture_workspace_surface_sheet_metrics(ctx, pane_rect, surface_index, &metrics)) {
        return 0;
    }
    zoom = metrics.pixel_size / drawing_program_viewport_base_pixel_size();
    if (!isfinite(zoom) || zoom <= 0.0f) {
        return 0;
    }
    interaction->canvas_move_active = 1u;
    interaction->canvas_move_surface_index = surface_index;
    interaction->canvas_move_start_offset_x = surface->layout_offset_x;
    interaction->canvas_move_start_offset_y = surface->layout_offset_y;
    interaction->canvas_move_zoom = zoom;
    interaction->canvas_move_anchor_mouse_x = sx;
    interaction->canvas_move_anchor_mouse_y = sy;
    return 1;
}

int drawing_program_texture_canvas_move_update(DrawingProgramAppContext *ctx,
                                               VisualCanvasInteractionState *interaction,
                                               int sx,
                                               int sy) {
    float delta_x = 0.0f;
    float delta_y = 0.0f;
    float next_offset_x = 0.0f;
    float next_offset_y = 0.0f;
    if (!ctx || !interaction || !interaction->canvas_move_active) {
        return 0;
    }
    if (!isfinite(interaction->canvas_move_zoom) || interaction->canvas_move_zoom <= 0.0f) {
        return 0;
    }
    delta_x = (float)(sx - interaction->canvas_move_anchor_mouse_x) / interaction->canvas_move_zoom;
    delta_y = (float)(sy - interaction->canvas_move_anchor_mouse_y) / interaction->canvas_move_zoom;
    next_offset_x = interaction->canvas_move_start_offset_x + delta_x;
    next_offset_y = interaction->canvas_move_start_offset_y + delta_y;
    if (!isfinite(next_offset_x) || !isfinite(next_offset_y)) {
        return 0;
    }
    return drawing_program_texture_project_session_move_surface(ctx,
                                                                interaction->canvas_move_surface_index,
                                                                next_offset_x,
                                                                next_offset_y)
                   .code == CORE_OK
               ? 1
               : 0;
}

void drawing_program_texture_canvas_move_end(VisualCanvasInteractionState *interaction) {
    if (!interaction) {
        return;
    }
    interaction->canvas_move_active = 0u;
    interaction->canvas_move_surface_index = 0u;
    interaction->canvas_move_start_offset_x = 0.0f;
    interaction->canvas_move_start_offset_y = 0.0f;
    interaction->canvas_move_zoom = 0.0f;
    interaction->canvas_move_anchor_mouse_x = 0;
    interaction->canvas_move_anchor_mouse_y = 0;
}
