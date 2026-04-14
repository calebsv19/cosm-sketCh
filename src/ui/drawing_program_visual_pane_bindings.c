#include "drawing_program/drawing_program_visual_pane_bindings.h"

uint32_t drawing_program_visual_module_type_for_pane(const DrawingProgramAppContext *ctx, uint32_t pane_node_id) {
    uint32_t i;
    if (!ctx) {
        return 0u;
    }
    for (i = 0u; i < ctx->pane_host.module_binding_count; ++i) {
        if (ctx->pane_host.module_bindings[i].pane_node_id == pane_node_id) {
            return ctx->pane_host.module_bindings[i].module_type_id;
        }
    }
    return 0u;
}

int drawing_program_visual_set_module_type_for_pane(DrawingProgramAppContext *ctx,
                                                     uint32_t pane_node_id,
                                                     uint32_t module_type_id) {
    uint32_t i;
    if (!ctx) {
        return 0;
    }
    for (i = 0u; i < ctx->pane_host.module_binding_count; ++i) {
        if (ctx->pane_host.module_bindings[i].pane_node_id == pane_node_id) {
            ctx->pane_host.module_bindings[i].module_type_id = module_type_id;
            return 1;
        }
    }
    return 0;
}

int drawing_program_visual_pane_rect_for_module_type(const DrawingProgramAppContext *ctx,
                                                      uint32_t module_type_id,
                                                      SDL_Rect *out_rect) {
    uint32_t i;
    if (!ctx || !out_rect) {
        return 0;
    }
    for (i = 0u; i < ctx->pane_host.leaf_count; ++i) {
        const CorePaneLeafRect *leaf = &ctx->pane_host.leaves[i];
        if (drawing_program_visual_module_type_for_pane(ctx, (uint32_t)leaf->id) == module_type_id) {
            out_rect->x = (int)leaf->rect.x;
            out_rect->y = (int)leaf->rect.y;
            out_rect->w = (int)leaf->rect.width;
            out_rect->h = (int)leaf->rect.height;
            if (out_rect->w < 1) {
                out_rect->w = 1;
            }
            if (out_rect->h < 1) {
                out_rect->h = 1;
            }
            return 1;
        }
    }
    return 0;
}

int drawing_program_visual_point_in_rect(SDL_Rect rect, int x, int y) {
    return (x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h) ? 1 : 0;
}
