#include "drawing_program/drawing_program_visual_overlay_render.h"

#include <stdlib.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_selection.h"
#include "drawing_program/drawing_program_visual_overlay_shared.h"
#include "drawing_program/drawing_program_visual_theme.h"

#define VISUAL_SELECTION_HANDLE_COUNT 8
#define VISUAL_SELECTION_MAX_COMPONENT_BOUNDS 128u

typedef struct VisualSelectionComponentBounds {
    uint32_t min_x;
    uint32_t min_y;
    uint32_t max_x;
    uint32_t max_y;
} VisualSelectionComponentBounds;

static int selection_handle_size_for_metrics(const VisualCanvasSheetMetrics *metrics) {
    int handle_size = 8;
    if (metrics) {
        handle_size = (int)(metrics->pixel_size * 1.75f);
    }
    if (handle_size < 6) {
        handle_size = 6;
    }
    if (handle_size > 16) {
        handle_size = 16;
    }
    return handle_size;
}

static void selection_populate_handle_rects(const SDL_Rect *selection_rect,
                                            int handle_size,
                                            SDL_Rect out_handles[VISUAL_SELECTION_HANDLE_COUNT]) {
    int x;
    int y;
    int w;
    int h;
    int cx;
    int cy;
    if (!selection_rect || !out_handles) {
        return;
    }
    x = selection_rect->x;
    y = selection_rect->y;
    w = selection_rect->w;
    h = selection_rect->h;
    cx = x + (w / 2);
    cy = y + (h / 2);
    out_handles[0] = (SDL_Rect){ x - (handle_size / 2), y - (handle_size / 2), handle_size, handle_size };
    out_handles[1] = (SDL_Rect){ cx - (handle_size / 2), y - (handle_size / 2), handle_size, handle_size };
    out_handles[2] = (SDL_Rect){ x + w - (handle_size / 2), y - (handle_size / 2), handle_size, handle_size };
    out_handles[3] = (SDL_Rect){ x + w - (handle_size / 2), cy - (handle_size / 2), handle_size, handle_size };
    out_handles[4] = (SDL_Rect){ x + w - (handle_size / 2), y + h - (handle_size / 2), handle_size, handle_size };
    out_handles[5] = (SDL_Rect){ cx - (handle_size / 2), y + h - (handle_size / 2), handle_size, handle_size };
    out_handles[6] = (SDL_Rect){ x - (handle_size / 2), y + h - (handle_size / 2), handle_size, handle_size };
    out_handles[7] = (SDL_Rect){ x - (handle_size / 2), cy - (handle_size / 2), handle_size, handle_size };
}

int drawing_program_visual_selection_move_handle_hit(const VisualCanvasSheetMetrics *metrics,
                                                     const VisualSelectionState *selection,
                                                     int x,
                                                     int y) {
    SDL_Rect selection_rect;
    SDL_Rect handles[VISUAL_SELECTION_HANDLE_COUNT];
    int handle_size;
    int i;
    if (!metrics || !selection || !selection->has_payload || selection->width == 0u || selection->height == 0u) {
        return 0;
    }
    if (!drawing_program_visual_overlay_sample_rect_to_screen_rect(metrics,
                                                                   (int32_t)selection->origin_x + selection->offset_x,
                                                                   (int32_t)selection->origin_y + selection->offset_y,
                                                                   selection->width,
                                                                   selection->height,
                                                                   &selection_rect)) {
        return 0;
    }
    handle_size = selection_handle_size_for_metrics(metrics);
    selection_populate_handle_rects(&selection_rect, handle_size, handles);
    for (i = 0; i < VISUAL_SELECTION_HANDLE_COUNT; ++i) {
        const SDL_Rect handle = handles[i];
        if (x >= handle.x && y >= handle.y && x < (handle.x + handle.w) && y < (handle.y + handle.h)) {
            return 1;
        }
    }
    return 0;
}

static void draw_selection_payload_preview(SDL_Renderer *renderer,
                                           const DrawingProgramAppContext *ctx,
                                           const VisualCanvasSheetMetrics *metrics,
                                           const VisualSelectionState *selection,
                                           int32_t origin_x,
                                           int32_t origin_y,
                                           uint8_t alpha) {
    uint32_t x;
    uint32_t y;
    if (!renderer || !ctx || !metrics || !selection || !selection->has_payload) {
        return;
    }
    if (selection->width == 0u || selection->height == 0u) {
        return;
    }
    for (y = 0u; y < selection->height; ++y) {
        for (x = 0u; x < selection->width; ++x) {
            int32_t sample_x;
            int32_t sample_y;
            SDL_Rect sample_rect;
            uint8_t value;
            uint8_t r = 0u;
            uint8_t g = 0u;
            uint8_t b = 0u;
            if (!drawing_program_selection_mask_at(selection, x, y)) {
                continue;
            }
            sample_x = origin_x + (int32_t)x;
            sample_y = origin_y + (int32_t)y;
            if (sample_x < 0 || sample_y < 0 ||
                sample_x >= (int32_t)ctx->document.raster_width ||
                sample_y >= (int32_t)ctx->document.raster_height) {
                continue;
            }
            if (!drawing_program_visual_overlay_sample_rect_to_screen_rect(
                    metrics, sample_x, sample_y, 1u, 1u, &sample_rect)) {
                continue;
            }
            value = drawing_program_selection_value_at(selection, x, y);
            drawing_program_color_rgb_from_sample(value, &r, &g, &b);
            SDL_SetRenderDrawColor(renderer, r, g, b, alpha);
            (void)SDL_RenderFillRect(renderer, &sample_rect);
        }
    }
}

static uint32_t collect_selection_component_bounds(const VisualSelectionState *selection,
                                                   VisualSelectionComponentBounds *out_bounds,
                                                   uint32_t max_bounds) {
    uint32_t width;
    uint32_t height;
    uint32_t area;
    uint8_t *visited = 0;
    uint32_t *queue = 0;
    uint32_t idx;
    uint32_t component_count = 0u;
    if (!selection || !selection->has_payload || !out_bounds || max_bounds == 0u) {
        return 0u;
    }
    width = selection->width;
    height = selection->height;
    if (width == 0u || height == 0u) {
        return 0u;
    }
    area = width * height;
    if (area == 0u || area > DRAWING_PROGRAM_SELECTION_MAX_AREA) {
        return 0u;
    }
    visited = (uint8_t *)calloc(area, sizeof(uint8_t));
    queue = (uint32_t *)malloc(area * sizeof(uint32_t));
    if (!visited || !queue) {
        free(visited);
        free(queue);
        return 0u;
    }
    for (idx = 0u; idx < area; ++idx) {
        uint32_t head;
        uint32_t tail;
        uint32_t seed_x;
        uint32_t seed_y;
        uint32_t min_x;
        uint32_t min_y;
        uint32_t max_x;
        uint32_t max_y;
        if (!selection->payload_mask[idx] || visited[idx]) {
            continue;
        }
        seed_x = idx % width;
        seed_y = idx / width;
        min_x = seed_x;
        min_y = seed_y;
        max_x = seed_x;
        max_y = seed_y;
        head = 0u;
        tail = 0u;
        queue[tail++] = idx;
        visited[idx] = 1u;
        while (head < tail) {
            uint32_t current = queue[head++];
            uint32_t sx = current % width;
            uint32_t sy = current / width;
            if (sx > 0u) {
                uint32_t neighbor = current - 1u;
                if (selection->payload_mask[neighbor] && !visited[neighbor]) {
                    visited[neighbor] = 1u;
                    queue[tail++] = neighbor;
                    if (sx - 1u < min_x) {
                        min_x = sx - 1u;
                    }
                }
            }
            if ((sx + 1u) < width) {
                uint32_t neighbor = current + 1u;
                if (selection->payload_mask[neighbor] && !visited[neighbor]) {
                    visited[neighbor] = 1u;
                    queue[tail++] = neighbor;
                    if (sx + 1u > max_x) {
                        max_x = sx + 1u;
                    }
                }
            }
            if (sy > 0u) {
                uint32_t neighbor = current - width;
                if (selection->payload_mask[neighbor] && !visited[neighbor]) {
                    visited[neighbor] = 1u;
                    queue[tail++] = neighbor;
                    if (sy - 1u < min_y) {
                        min_y = sy - 1u;
                    }
                }
            }
            if ((sy + 1u) < height) {
                uint32_t neighbor = current + width;
                if (selection->payload_mask[neighbor] && !visited[neighbor]) {
                    visited[neighbor] = 1u;
                    queue[tail++] = neighbor;
                    if (sy + 1u > max_y) {
                        max_y = sy + 1u;
                    }
                }
            }
        }
        if (component_count < max_bounds) {
            out_bounds[component_count].min_x = min_x;
            out_bounds[component_count].min_y = min_y;
            out_bounds[component_count].max_x = max_x;
            out_bounds[component_count].max_y = max_y;
            component_count += 1u;
        }
    }
    free(visited);
    free(queue);
    return component_count;
}

uint32_t drawing_program_visual_selection_component_count(const VisualSelectionState *selection) {
    VisualSelectionComponentBounds components[VISUAL_SELECTION_MAX_COMPONENT_BOUNDS];
    return collect_selection_component_bounds(selection, components, VISUAL_SELECTION_MAX_COMPONENT_BOUNDS);
}

static uint32_t draw_selection_component_outlines(SDL_Renderer *renderer,
                                                  const VisualCanvasSheetMetrics *metrics,
                                                  const VisualSelectionState *selection,
                                                  int32_t delta_x,
                                                  int32_t delta_y,
                                                  SDL_Color color,
                                                  uint8_t alpha) {
    VisualSelectionComponentBounds components[VISUAL_SELECTION_MAX_COMPONENT_BOUNDS];
    uint32_t component_count;
    uint32_t i;
    if (!renderer || !metrics || !selection || !selection->has_payload) {
        return 0u;
    }
    component_count = collect_selection_component_bounds(selection,
                                                         components,
                                                         VISUAL_SELECTION_MAX_COMPONENT_BOUNDS);
    if (component_count == 0u) {
        return 0u;
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
    for (i = 0u; i < component_count; ++i) {
        int32_t sample_x = (int32_t)selection->origin_x + (int32_t)components[i].min_x + delta_x;
        int32_t sample_y = (int32_t)selection->origin_y + (int32_t)components[i].min_y + delta_y;
        uint32_t region_w = components[i].max_x - components[i].min_x + 1u;
        uint32_t region_h = components[i].max_y - components[i].min_y + 1u;
        SDL_Rect rect;
        if (drawing_program_visual_overlay_sample_rect_to_screen_rect(
                metrics, sample_x, sample_y, region_w, region_h, &rect)) {
            (void)SDL_RenderDrawRect(renderer, &rect);
        }
    }
    return component_count;
}

void drawing_program_visual_draw_selection_overlay(SDL_Renderer *renderer,
                                                   SDL_Rect pane_rect,
                                                   const DrawingProgramAppContext *ctx,
                                                   const CoreThemePreset *theme,
                                                   const VisualCanvasSheetMetrics *metrics,
                                                   const VisualSelectionState *selection) {
    SDL_Color accent = { 120u, 160u, 220u, 255u };
    SDL_Color accent_alt = { 220u, 180u, 120u, 255u };
    SDL_Rect rect = { 0, 0, 0, 0 };
    if (!renderer || !ctx || !metrics || !selection) {
        return;
    }
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_ACCENT_PRIMARY, &accent);
    accent_alt = sdl_color_shift_by_luma(accent, 40);
    (void)SDL_RenderSetClipRect(renderer, &pane_rect);
    if (selection->selecting) {
        int32_t min_x = (selection->marquee_start_x < selection->marquee_end_x)
                            ? (int32_t)selection->marquee_start_x
                            : (int32_t)selection->marquee_end_x;
        int32_t min_y = (selection->marquee_start_y < selection->marquee_end_y)
                            ? (int32_t)selection->marquee_start_y
                            : (int32_t)selection->marquee_end_y;
        int32_t max_x = (selection->marquee_start_x > selection->marquee_end_x)
                            ? (int32_t)selection->marquee_start_x
                            : (int32_t)selection->marquee_end_x;
        int32_t max_y = (selection->marquee_start_y > selection->marquee_end_y)
                            ? (int32_t)selection->marquee_start_y
                            : (int32_t)selection->marquee_end_y;
        uint32_t w = (uint32_t)(max_x - min_x + 1);
        uint32_t h = (uint32_t)(max_y - min_y + 1);
        if (drawing_program_visual_overlay_sample_rect_to_screen_rect(metrics, min_x, min_y, w, h, &rect)) {
            SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 240u);
            (void)SDL_RenderDrawRect(renderer, &rect);
            if (rect.w > 2 && rect.h > 2) {
                SDL_Rect inner = { rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2 };
                SDL_SetRenderDrawColor(renderer, accent_alt.r, accent_alt.g, accent_alt.b, 220u);
                (void)SDL_RenderDrawRect(renderer, &inner);
            }
        }
    }
    if (selection->has_payload && selection->width > 0u && selection->height > 0u &&
        ctx->object_selection.count == 0u) {
        int32_t base_x = (int32_t)selection->origin_x;
        int32_t base_y = (int32_t)selection->origin_y;
        int32_t moved_x = base_x + selection->offset_x;
        int32_t moved_y = base_y + selection->offset_y;
        uint32_t moved_component_count = 0u;
        if (selection->moving && (selection->offset_x != 0 || selection->offset_y != 0)) {
            uint32_t base_component_count =
                draw_selection_component_outlines(renderer, metrics, selection, 0, 0, accent_alt, 170u);
            if (base_component_count == 0u &&
                drawing_program_visual_overlay_sample_rect_to_screen_rect(metrics,
                                                                          base_x,
                                                                          base_y,
                                                                          selection->width,
                                                                          selection->height,
                                                                          &rect)) {
                SDL_SetRenderDrawColor(renderer, accent_alt.r, accent_alt.g, accent_alt.b, 170u);
                (void)SDL_RenderDrawRect(renderer, &rect);
            }
        }
        moved_component_count =
            draw_selection_component_outlines(renderer,
                                              metrics,
                                              selection,
                                              selection->offset_x,
                                              selection->offset_y,
                                              accent,
                                              255u);
        if (moved_component_count == 0u &&
            drawing_program_visual_overlay_sample_rect_to_screen_rect(metrics,
                                                                      moved_x,
                                                                      moved_y,
                                                                      selection->width,
                                                                      selection->height,
                                                                      &rect)) {
            SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 255u);
            (void)SDL_RenderDrawRect(renderer, &rect);
        }
        if (drawing_program_visual_overlay_sample_rect_to_screen_rect(metrics,
                                                                      moved_x,
                                                                      moved_y,
                                                                      selection->width,
                                                                      selection->height,
                                                                      &rect)) {
            SDL_Rect handles[VISUAL_SELECTION_HANDLE_COUNT];
            int handle_size = selection_handle_size_for_metrics(metrics);
            int i;
            if (selection->moving && (selection->offset_x != 0 || selection->offset_y != 0)) {
                draw_selection_payload_preview(renderer, ctx, metrics, selection, moved_x, moved_y, 255u);
            }
            if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE || selection->moving) {
                selection_populate_handle_rects(&rect, handle_size, handles);
                for (i = 0; i < VISUAL_SELECTION_HANDLE_COUNT; ++i) {
                    SDL_SetRenderDrawColor(renderer, accent_alt.r, accent_alt.g, accent_alt.b, 255u);
                    (void)SDL_RenderFillRect(renderer, &handles[i]);
                    SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 255u);
                    (void)SDL_RenderDrawRect(renderer, &handles[i]);
                }
            }
        }
    }
    (void)SDL_RenderSetClipRect(renderer, 0);
}
