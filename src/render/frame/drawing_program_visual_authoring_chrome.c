#include "drawing_program/drawing_program_visual_authoring_chrome.h"

#include <stdio.h>
#include <string.h>

#include "core_layout.h"
#include "core_pane_module.h"
#include "drawing_program/drawing_program_visual_text_render.h"
#include "drawing_program/drawing_program_visual_theme.h"

static const CorePaneModuleBinding *authoring_binding_for_pane(const DrawingProgramAppContext *ctx,
                                                               uint32_t pane_node_id) {
    uint32_t i;
    if (!ctx) {
        return 0;
    }
    for (i = 0u; i < ctx->pane_host.module_binding_count; ++i) {
        const CorePaneModuleBinding *binding = &ctx->pane_host.module_bindings[i];
        if (binding->pane_node_id == pane_node_id) {
            return binding;
        }
    }
    return 0;
}

static const char *authoring_module_label(const DrawingProgramAppContext *ctx,
                                          const CorePaneModuleBinding *binding) {
    const CorePaneModuleDescriptor *descriptor = 0;
    if (!ctx || !binding) {
        return "Unbound";
    }
    if (core_pane_module_find_by_type_id(&ctx->pane_host.module_registry,
                                         binding->module_type_id,
                                         &descriptor) != CORE_PANE_MODULE_OK ||
        !descriptor || !descriptor->display_name) {
        return "Unknown";
    }
    return descriptor->display_name;
}

static SDL_Rect authoring_panel_rect(int viewport_width, int viewport_height, uint32_t row_count) {
    int panel_width = 420;
    int panel_height = 102 + (int)(row_count * 18u);
    if (viewport_width < panel_width + 24) {
        panel_width = viewport_width - 24;
    }
    if (panel_width < 260) {
        panel_width = 260;
    }
    if (panel_height > viewport_height - 24) {
        panel_height = viewport_height - 24;
    }
    if (panel_height < 118) {
        panel_height = 118;
    }
    return (SDL_Rect){12, 12, panel_width, panel_height};
}

static SDL_Rect authoring_apply_rect(SDL_Rect panel_rect) {
    return (SDL_Rect){panel_rect.x + panel_rect.w - 154, panel_rect.y + 7, 66, 22};
}

static SDL_Rect authoring_cancel_rect(SDL_Rect panel_rect) {
    return (SDL_Rect){panel_rect.x + panel_rect.w - 82, panel_rect.y + 7, 70, 22};
}

static int authoring_point_in_rect(SDL_Rect rect, int point_x, int point_y) {
    return point_x >= rect.x &&
           point_y >= rect.y &&
           point_x < rect.x + rect.w &&
           point_y < rect.y + rect.h;
}

uint32_t drawing_program_visual_authoring_chrome_build_pane_rows(
    const DrawingProgramAppContext *ctx,
    char rows[][DRAWING_PROGRAM_AUTHORING_CHROME_ROW_TEXT_MAX],
    uint32_t row_capacity) {
    uint32_t count = 0u;
    uint32_t i;
    if (!ctx || !rows || row_capacity == 0u) {
        return 0u;
    }
    for (i = 0u; i < ctx->pane_host.leaf_count && count < row_capacity; ++i) {
        const CorePaneLeafRect *leaf = &ctx->pane_host.leaves[i];
        const CorePaneModuleBinding *binding = authoring_binding_for_pane(ctx, (uint32_t)leaf->id);
        const char *label = authoring_module_label(ctx, binding);
        (void)snprintf(rows[count],
                       DRAWING_PROGRAM_AUTHORING_CHROME_ROW_TEXT_MAX,
                       "Pane %u: %s",
                       (unsigned)leaf->id,
                       label);
        count += 1u;
    }
    return count;
}

DrawingProgramAuthoringChromeAction drawing_program_visual_authoring_chrome_hit_test(
    int viewport_width,
    int viewport_height,
    int point_x,
    int point_y) {
    SDL_Rect panel_rect;
    if (viewport_width <= 0 || viewport_height <= 0) {
        return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_NONE;
    }
    panel_rect = authoring_panel_rect(viewport_width, viewport_height, 4u);
    if (authoring_point_in_rect(authoring_apply_rect(panel_rect), point_x, point_y)) {
        return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_APPLY;
    }
    if (authoring_point_in_rect(authoring_cancel_rect(panel_rect), point_x, point_y)) {
        return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CANCEL;
    }
    return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_NONE;
}

static void authoring_draw_panel(SDL_Renderer *renderer,
                                 SDL_Rect rect,
                                 SDL_Color fill,
                                 SDL_Color border) {
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
    (void)SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
    (void)SDL_RenderDrawRect(renderer, &rect);
}

static void authoring_draw_button(SDL_Renderer *renderer,
                                  SDL_Rect rect,
                                  const char *label,
                                  SDL_Color fill,
                                  SDL_Color border,
                                  SDL_Color text) {
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
    (void)SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
    (void)SDL_RenderDrawRect(renderer, &rect);
    (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                  rect,
                                                  rect.x + 8,
                                                  rect.y + 5,
                                                  label,
                                                  text,
                                                  1);
}

static void authoring_draw_leaf_labels(SDL_Renderer *renderer,
                                       const DrawingProgramAppContext *ctx,
                                       SDL_Color accent,
                                       SDL_Color text,
                                       SDL_Color fill) {
    uint32_t i;
    if (!renderer || !ctx) {
        return;
    }
    for (i = 0u; i < ctx->pane_host.leaf_count; ++i) {
        const CorePaneLeafRect *leaf = &ctx->pane_host.leaves[i];
        const CorePaneModuleBinding *binding = authoring_binding_for_pane(ctx, (uint32_t)leaf->id);
        const char *label = authoring_module_label(ctx, binding);
        SDL_Rect pane_rect = {
            (int)leaf->rect.x,
            (int)leaf->rect.y,
            (int)leaf->rect.width,
            (int)leaf->rect.height
        };
        SDL_Rect label_rect;
        char line[DRAWING_PROGRAM_AUTHORING_CHROME_ROW_TEXT_MAX];
        if (pane_rect.w < 8 || pane_rect.h < 8) {
            continue;
        }
        label_rect.x = pane_rect.x + 8;
        label_rect.y = pane_rect.y + 8;
        label_rect.w = 164;
        label_rect.h = 22;
        if (label_rect.x + label_rect.w > pane_rect.x + pane_rect.w - 8) {
            label_rect.w = pane_rect.x + pane_rect.w - label_rect.x - 8;
        }
        if (label_rect.w < 72) {
            label_rect.w = 72;
        }
        (void)snprintf(line, sizeof(line), "P%u  %s", (unsigned)leaf->id, label);
        SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 220u);
        (void)SDL_RenderDrawRect(renderer, &pane_rect);
        SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, 210u);
        (void)SDL_RenderFillRect(renderer, &label_rect);
        SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 230u);
        (void)SDL_RenderDrawRect(renderer, &label_rect);
        (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                      label_rect,
                                                      label_rect.x + 7,
                                                      label_rect.y + 4,
                                                      line,
                                                      text,
                                                      1);
    }
}

void drawing_program_visual_authoring_chrome_draw(SDL_Renderer *renderer,
                                                  int viewport_width,
                                                  int viewport_height,
                                                  const DrawingProgramAppContext *ctx,
                                                  const CoreThemePreset *theme) {
    enum { MAX_ROWS = 8 };
    VisualThemePalette palette;
    SDL_BlendMode previous_blend = SDL_BLENDMODE_NONE;
    SDL_Rect screen_clip;
    SDL_Rect panel_rect;
    SDL_Rect title_band;
    SDL_Color panel_fill;
    char rows[MAX_ROWS][DRAWING_PROGRAM_AUTHORING_CHROME_ROW_TEXT_MAX];
    uint32_t row_count = 0u;
    uint32_t i;
    int y = 0;

    if (!renderer || !ctx ||
        ctx->pane_host.layout_state.mode != CORE_LAYOUT_MODE_AUTHORING ||
        viewport_width <= 0 || viewport_height <= 0) {
        return;
    }

    resolve_visual_theme_palette(theme, &palette);
    panel_fill = palette.pane_background_alt;
    row_count = drawing_program_visual_authoring_chrome_build_pane_rows(ctx, rows, MAX_ROWS);
    screen_clip = (SDL_Rect){0, 0, viewport_width, viewport_height};
    panel_rect = authoring_panel_rect(viewport_width, viewport_height, row_count);
    title_band = (SDL_Rect){panel_rect.x, panel_rect.y, panel_rect.w, 30};

    (void)SDL_GetRenderDrawBlendMode(renderer, &previous_blend);
    (void)SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    panel_fill.a = 232u;
    authoring_draw_panel(renderer, panel_rect, panel_fill, palette.accent_primary);
    SDL_SetRenderDrawColor(renderer,
                           palette.accent_primary.r,
                           palette.accent_primary.g,
                           palette.accent_primary.b,
                           238u);
    (void)SDL_RenderFillRect(renderer, &title_band);

    y = panel_rect.y + 7;
    (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                  screen_clip,
                                                  panel_rect.x + 12,
                                                  y,
                                                  "AUTHORING",
                                                  palette.app_background,
                                                  1);
    authoring_draw_button(renderer,
                          authoring_apply_rect(panel_rect),
                          "APPLY",
                          palette.status_ok,
                          palette.text_primary,
                          palette.app_background);
    authoring_draw_button(renderer,
                          authoring_cancel_rect(panel_rect),
                          "CANCEL",
                          palette.status_warn,
                          palette.text_primary,
                          palette.app_background);
    y += 30;
    (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                  screen_clip,
                                                  panel_rect.x + 12,
                                                  y,
                                                  "Enter applies  Esc cancels  Alt+C then Alt+V toggles",
                                                  palette.text_primary,
                                                  1);
    y += 20;
    (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                  screen_clip,
                                                  panel_rect.x + 12,
                                                  y,
                                                  ctx->pane_host.layout_state.has_pending_changes ?
                                                      "Draft: pending changes" :
                                                      "Draft: no pending pane edits",
                                                  palette.text_muted,
                                                  1);
    y += 20;
    for (i = 0u; i < row_count && y < panel_rect.y + panel_rect.h - 14; ++i) {
        (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                      screen_clip,
                                                      panel_rect.x + 12,
                                                      y,
                                                      rows[i],
                                                      palette.text_primary,
                                                      1);
        y += 18;
    }

    authoring_draw_leaf_labels(renderer,
                               ctx,
                               palette.accent_primary,
                               palette.text_primary,
                               panel_fill);

    (void)SDL_SetRenderDrawBlendMode(renderer, previous_blend);
}
