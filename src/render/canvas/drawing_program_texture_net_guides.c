#include "drawing_program/drawing_program_texture_net_guides.h"

#include <string.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_texture_net.h"

typedef struct DrawingProgramTextureNetGuidePalette {
    SDL_Color corner_colors[8];
    SDL_Color edge_colors[12];
    SDL_Color outline;
} DrawingProgramTextureNetGuidePalette;

static const DrawingProgramTextureNetGuidePalette g_texture_net_guide_palette = {
    {
        {244u, 91u, 91u, 255u},
        {243u, 167u, 64u, 255u},
        {236u, 212u, 77u, 255u},
        {102u, 214u, 124u, 255u},
        {88u, 201u, 221u, 255u},
        {94u, 132u, 255u, 255u},
        {218u, 104u, 230u, 255u},
        {244u, 125u, 178u, 255u},
    },
    {
        {245u, 131u, 107u, 255u},
        {236u, 165u, 86u, 255u},
        {222u, 198u, 92u, 255u},
        {174u, 210u, 91u, 255u},
        {103u, 207u, 173u, 255u},
        {84u, 173u, 226u, 255u},
        {112u, 133u, 238u, 255u},
        {165u, 112u, 236u, 255u},
        {213u, 103u, 204u, 255u},
        {232u, 99u, 170u, 255u},
        {238u, 118u, 134u, 255u},
        {222u, 149u, 110u, 255u},
    },
    {18u, 20u, 26u, 255u},
};

static int texture_net_guides_has_corner_ids(const DrawingProgramTextureSurfaceSemantic *semantic) {
    uint32_t i;
    if (!semantic) {
        return 0;
    }
    for (i = 0u; i < DRAWING_PROGRAM_TEXTURE_NET_FACE_CORNER_COUNT; ++i) {
        if (semantic->corner_ids[i] != DRAWING_PROGRAM_TEXTURE_NET_UNKNOWN_ID) {
            return 1;
        }
    }
    return 0;
}

static void texture_net_guides_draw_filled_border_rect(SDL_Renderer *renderer,
                                                       SDL_Rect rect,
                                                       SDL_Color fill,
                                                       SDL_Color outline) {
    if (!renderer || rect.w <= 0 || rect.h <= 0) {
        return;
    }
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
    (void)SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, outline.r, outline.g, outline.b, outline.a);
    (void)SDL_RenderDrawRect(renderer, &rect);
}

static void texture_net_guides_draw_corner_chips(SDL_Renderer *renderer,
                                                 const DrawingProgramTextureSurfaceSemantic *semantic,
                                                 const VisualCanvasSheetMetrics *metrics) {
    SDL_Rect chip_rect = {0, 0, 0, 0};
    int inset = 0;
    int chip_size = 0;
    int x_right = 0;
    int y_bottom = 0;
    uint32_t i;
    if (!renderer || !semantic || !metrics || metrics->sheet_rect.w <= 0 || metrics->sheet_rect.h <= 0) {
        return;
    }
    inset = metrics->sheet_rect.w < metrics->sheet_rect.h ? metrics->sheet_rect.w : metrics->sheet_rect.h;
    if (inset < 16) {
        return;
    }
    chip_size = inset / 14;
    if (chip_size < 6) {
        chip_size = 6;
    } else if (chip_size > 14) {
        chip_size = 14;
    }
    inset = chip_size / 2 + 2;
    x_right = metrics->sheet_rect.x + metrics->sheet_rect.w - inset - chip_size;
    y_bottom = metrics->sheet_rect.y + metrics->sheet_rect.h - inset - chip_size;
    for (i = 0u; i < DRAWING_PROGRAM_TEXTURE_NET_FACE_CORNER_COUNT; ++i) {
        uint8_t corner_id = semantic->corner_ids[i];
        SDL_Color fill;
        if (corner_id == DRAWING_PROGRAM_TEXTURE_NET_UNKNOWN_ID ||
            corner_id >= (uint8_t)(sizeof(g_texture_net_guide_palette.corner_colors) /
                                   sizeof(g_texture_net_guide_palette.corner_colors[0]))) {
            continue;
        }
        chip_rect.w = chip_size;
        chip_rect.h = chip_size;
        switch ((DrawingProgramTextureNetCornerSlot)i) {
            case DRAWING_PROGRAM_TEXTURE_NET_CORNER_TOP_LEFT:
                chip_rect.x = metrics->sheet_rect.x + inset;
                chip_rect.y = metrics->sheet_rect.y + inset;
                break;
            case DRAWING_PROGRAM_TEXTURE_NET_CORNER_TOP_RIGHT:
                chip_rect.x = x_right;
                chip_rect.y = metrics->sheet_rect.y + inset;
                break;
            case DRAWING_PROGRAM_TEXTURE_NET_CORNER_BOTTOM_RIGHT:
                chip_rect.x = x_right;
                chip_rect.y = y_bottom;
                break;
            case DRAWING_PROGRAM_TEXTURE_NET_CORNER_BOTTOM_LEFT:
            default:
                chip_rect.x = metrics->sheet_rect.x + inset;
                chip_rect.y = y_bottom;
                break;
        }
        fill = g_texture_net_guide_palette.corner_colors[corner_id];
        texture_net_guides_draw_filled_border_rect(renderer, chip_rect, fill, g_texture_net_guide_palette.outline);
    }
}

static void texture_net_guides_draw_edge_strip(SDL_Renderer *renderer,
                                               SDL_Rect rect,
                                               SDL_Color fill) {
    if (!renderer || rect.w <= 0 || rect.h <= 0) {
        return;
    }
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
    (void)SDL_RenderFillRect(renderer, &rect);
}

static void texture_net_guides_draw_edge_strips(SDL_Renderer *renderer,
                                                const DrawingProgramTextureSurfaceSemantic *semantic,
                                                const VisualCanvasSheetMetrics *metrics) {
    SDL_Rect strip_rect = {0, 0, 0, 0};
    int thickness = 0;
    int inset = 0;
    int span_margin = 0;
    uint32_t i;
    if (!renderer || !semantic || !metrics || metrics->sheet_rect.w <= 0 || metrics->sheet_rect.h <= 0) {
        return;
    }
    if (metrics->sheet_rect.w < 20 || metrics->sheet_rect.h < 20) {
        return;
    }
    thickness = metrics->sheet_rect.w < metrics->sheet_rect.h ? metrics->sheet_rect.w : metrics->sheet_rect.h;
    thickness /= 20;
    if (thickness < 2) {
        thickness = 2;
    } else if (thickness > 5) {
        thickness = 5;
    }
    inset = thickness + 2;
    span_margin = inset + thickness + 2;
    for (i = 0u; i < DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT; ++i) {
        uint8_t edge_id = semantic->edge_ids[i];
        SDL_Color fill;
        if (edge_id == DRAWING_PROGRAM_TEXTURE_NET_UNKNOWN_ID ||
            edge_id >= (uint8_t)(sizeof(g_texture_net_guide_palette.edge_colors) /
                                 sizeof(g_texture_net_guide_palette.edge_colors[0]))) {
            continue;
        }
        fill = g_texture_net_guide_palette.edge_colors[edge_id];
        switch ((DrawingProgramTextureNetEdgeSide)i) {
            case DRAWING_PROGRAM_TEXTURE_NET_EDGE_SIDE_TOP:
                strip_rect = (SDL_Rect){metrics->sheet_rect.x + span_margin,
                                        metrics->sheet_rect.y + inset,
                                        metrics->sheet_rect.w - (2 * span_margin),
                                        thickness};
                break;
            case DRAWING_PROGRAM_TEXTURE_NET_EDGE_SIDE_RIGHT:
                strip_rect = (SDL_Rect){metrics->sheet_rect.x + metrics->sheet_rect.w - inset - thickness,
                                        metrics->sheet_rect.y + span_margin,
                                        thickness,
                                        metrics->sheet_rect.h - (2 * span_margin)};
                break;
            case DRAWING_PROGRAM_TEXTURE_NET_EDGE_SIDE_BOTTOM:
                strip_rect = (SDL_Rect){metrics->sheet_rect.x + span_margin,
                                        metrics->sheet_rect.y + metrics->sheet_rect.h - inset - thickness,
                                        metrics->sheet_rect.w - (2 * span_margin),
                                        thickness};
                break;
            case DRAWING_PROGRAM_TEXTURE_NET_EDGE_SIDE_LEFT:
            default:
                strip_rect = (SDL_Rect){metrics->sheet_rect.x + inset,
                                        metrics->sheet_rect.y + span_margin,
                                        thickness,
                                        metrics->sheet_rect.h - (2 * span_margin)};
                break;
        }
        texture_net_guides_draw_edge_strip(renderer, strip_rect, fill);
    }
}

const char *drawing_program_ui_canvas_guide_mode_name(uint8_t guide_mode) {
    switch ((DrawingProgramUiCanvasGuideMode)guide_mode) {
        case DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_CORNERS: return "CORNERS";
        case DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_CORNERS_AND_EDGES: return "CORNERS+EDGES";
        case DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_OFF:
        default: return "OFF";
    }
}

void drawing_program_visual_draw_texture_net_guides(SDL_Renderer *renderer,
                                                    SDL_Rect pane_rect,
                                                    const DrawingProgramTextureSurface *surface,
                                                    const VisualCanvasSheetMetrics *metrics,
                                                    uint8_t guide_mode) {
    SDL_Rect clipped_sheet = {0, 0, 0, 0};
    if (!renderer || !surface || !metrics ||
        guide_mode == (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_OFF ||
        surface->semantic.layout_kind != DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_RECT_PRISM_CROSS ||
        !texture_net_guides_has_corner_ids(&surface->semantic)) {
        return;
    }
    clipped_sheet = metrics->sheet_rect;
    if (clipped_sheet.x < pane_rect.x) {
        clipped_sheet.w -= (pane_rect.x - clipped_sheet.x);
        clipped_sheet.x = pane_rect.x;
    }
    if (clipped_sheet.y < pane_rect.y) {
        clipped_sheet.h -= (pane_rect.y - clipped_sheet.y);
        clipped_sheet.y = pane_rect.y;
    }
    if (clipped_sheet.x + clipped_sheet.w > pane_rect.x + pane_rect.w) {
        clipped_sheet.w = pane_rect.x + pane_rect.w - clipped_sheet.x;
    }
    if (clipped_sheet.y + clipped_sheet.h > pane_rect.y + pane_rect.h) {
        clipped_sheet.h = pane_rect.y + pane_rect.h - clipped_sheet.y;
    }
    if (clipped_sheet.w <= 0 || clipped_sheet.h <= 0) {
        return;
    }
    (void)SDL_RenderSetClipRect(renderer, &clipped_sheet);
    if (guide_mode == (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_CORNERS_AND_EDGES) {
        texture_net_guides_draw_edge_strips(renderer, &surface->semantic, metrics);
    }
    texture_net_guides_draw_corner_chips(renderer, &surface->semantic, metrics);
    (void)SDL_RenderSetClipRect(renderer, &pane_rect);
}
