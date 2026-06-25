#include "drawing_program/drawing_program_visual_input_workspace_view.h"

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_texture_workspace.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_visual_right_panel_defs.h"
#include "drawing_program/drawing_program_visual_tool_options.h"
#include "drawing_program/drawing_program_viewport.h"

static int visual_input_workspace_canvas_pane_rect(const DrawingProgramAppContext *ctx, SDL_Rect *out_rect) {
    if (!ctx || !out_rect) {
        return 0;
    }
    return drawing_program_visual_pane_rect_for_module_type(ctx, 1u, out_rect);
}

int drawing_program_visual_input_workspace_view_fit_surface(DrawingProgramAppContext *ctx,
                                                            uint32_t surface_index) {
    SDL_Rect canvas_rect = {0, 0, 0, 0};
    if (!ctx || !visual_input_workspace_canvas_pane_rect(ctx, &canvas_rect)) {
        return 0;
    }
    return drawing_program_texture_workspace_fit_surface(ctx, canvas_rect, surface_index);
}

int drawing_program_visual_input_workspace_view_fit_all(DrawingProgramAppContext *ctx) {
    SDL_Rect canvas_rect = {0, 0, 0, 0};
    if (!ctx || !visual_input_workspace_canvas_pane_rect(ctx, &canvas_rect)) {
        return 0;
    }
    return drawing_program_texture_workspace_fit_all(ctx, canvas_rect);
}

int drawing_program_visual_input_workspace_view_fit_all_or_reset(DrawingProgramAppContext *ctx) {
    if (drawing_program_visual_input_workspace_view_fit_all(ctx)) {
        return 1;
    }
    if (ctx) {
        drawing_program_viewport_reset(&ctx->editor.viewport);
    }
    return 0;
}

int drawing_program_visual_input_workspace_view_show_canvas_fit_all(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return 0;
    }
    drawing_program_visual_set_right_panel_slot(ctx, (uint8_t)VISUAL_RIGHT_PANEL_SLOT_CANVAS);
    return drawing_program_visual_input_workspace_view_fit_all(ctx);
}
