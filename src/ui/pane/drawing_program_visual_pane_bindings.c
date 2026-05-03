#include "drawing_program/drawing_program_visual_pane_bindings.h"

#include <math.h>

static uint32_t drawing_program_visual_infer_module_type_for_pane(const DrawingProgramAppContext *ctx,
                                                                  uint32_t pane_node_id) {
    uint32_t menu_idx = 0u;
    uint32_t i;
    uint32_t candidate_indices[DRAWING_PROGRAM_PANE_LEAF_CAPACITY];
    uint32_t candidate_count = 0u;
    uint32_t left_idx;
    uint32_t right_idx;
    uint32_t center_idx = 0u;
    float min_x = 0.0f;
    float max_x = 0.0f;
    float target_mid = 0.0f;
    float best_distance = 0.0f;
    int center_set = 0;
    if (!ctx || ctx->pane_host.leaf_count < 4u) {
        return 0u;
    }

    for (i = 1u; i < ctx->pane_host.leaf_count; ++i) {
        const CorePaneLeafRect *candidate = &ctx->pane_host.leaves[i];
        const CorePaneLeafRect *best = &ctx->pane_host.leaves[menu_idx];
        if (candidate->rect.y < best->rect.y ||
            (candidate->rect.y == best->rect.y && candidate->rect.width > best->rect.width)) {
            menu_idx = i;
        }
    }
    if ((uint32_t)ctx->pane_host.leaves[menu_idx].id == pane_node_id) {
        return 3u;
    }

    for (i = 0u; i < ctx->pane_host.leaf_count; ++i) {
        if (i == menu_idx) {
            continue;
        }
        candidate_indices[candidate_count++] = i;
    }
    if (candidate_count < 3u) {
        return 0u;
    }

    left_idx = candidate_indices[0];
    right_idx = candidate_indices[0];
    min_x = ctx->pane_host.leaves[left_idx].rect.x;
    max_x = min_x;
    for (i = 1u; i < candidate_count; ++i) {
        uint32_t idx = candidate_indices[i];
        float x = ctx->pane_host.leaves[idx].rect.x;
        if (x < min_x) {
            min_x = x;
            left_idx = idx;
        }
        if (x > max_x) {
            max_x = x;
            right_idx = idx;
        }
    }
    if ((uint32_t)ctx->pane_host.leaves[left_idx].id == pane_node_id) {
        return 2u;
    }
    if ((uint32_t)ctx->pane_host.leaves[right_idx].id == pane_node_id) {
        return 4u;
    }

    target_mid = (min_x + max_x) * 0.5f;
    for (i = 0u; i < candidate_count; ++i) {
        uint32_t idx = candidate_indices[i];
        float center_x;
        float distance;
        if (idx == left_idx || idx == right_idx) {
            continue;
        }
        center_x = ctx->pane_host.leaves[idx].rect.x + (ctx->pane_host.leaves[idx].rect.width * 0.5f);
        distance = fabsf(center_x - target_mid);
        if (!center_set || distance < best_distance) {
            center_set = 1;
            best_distance = distance;
            center_idx = idx;
        }
    }
    if (!center_set) {
        for (i = 0u; i < candidate_count; ++i) {
            uint32_t idx = candidate_indices[i];
            if (idx != left_idx && idx != right_idx) {
                center_idx = idx;
                center_set = 1;
                break;
            }
        }
    }
    if (center_set && (uint32_t)ctx->pane_host.leaves[center_idx].id == pane_node_id) {
        return 1u;
    }
    return 0u;
}

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
    return drawing_program_visual_infer_module_type_for_pane(ctx, pane_node_id);
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
