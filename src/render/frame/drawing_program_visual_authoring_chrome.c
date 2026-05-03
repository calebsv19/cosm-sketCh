#include "drawing_program/drawing_program_visual_authoring_chrome.h"

#include <stdio.h>
#include <string.h>

#include "core_font.h"
#include "core_layout.h"
#include "core_pane_module.h"
#include "drawing_program/drawing_program_authoring_host.h"
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

static SDL_Rect authoring_mode_rect(SDL_Rect panel_rect) {
    return (SDL_Rect){panel_rect.x + panel_rect.w - 242, panel_rect.y + 7, 82, 22};
}

static SDL_Rect authoring_font_panel_rect(int viewport_width, int viewport_height) {
    int margin = 20;
    int panel_width = viewport_width - (margin * 2);
    int panel_height = viewport_height - 84;
    if (panel_width < 300) {
        panel_width = viewport_width - 16;
        margin = 8;
    }
    if (panel_width > 920) {
        panel_width = 920;
    }
    if (panel_height < 300) {
        panel_height = viewport_height - 48;
    }
    if (panel_height > 620) {
        panel_height = 620;
    }
    return (SDL_Rect){(viewport_width - panel_width) / 2, 54, panel_width, panel_height};
}

static SDL_Rect authoring_font_section_rect(SDL_Rect panel_rect, uint32_t section_index) {
    int pad = 18;
    int gap = 14;
    int section_h = 98;
    return (SDL_Rect){
        panel_rect.x + pad,
        panel_rect.y + pad + (int)section_index * (section_h + gap),
        panel_rect.w - (pad * 2),
        section_h
    };
}

static SDL_Rect authoring_font_button_rect(SDL_Rect section_rect, uint32_t index, uint32_t count) {
    int pad = 14;
    int gap = 8;
    int button_h = 24;
    int button_w;
    if (count == 0u) {
        count = 1u;
    }
    button_w = (section_rect.w - (pad * 2) - (gap * ((int)count - 1))) / (int)count;
    if (button_w < 42) {
        button_w = 42;
    }
    return (SDL_Rect){
        section_rect.x + pad + (int)index * (button_w + gap),
        section_rect.y + section_rect.h - button_h - 12,
        button_w,
        button_h
    };
}

static SDL_Rect authoring_font_value_rect(SDL_Rect section_rect) {
    SDL_Rect inc = authoring_font_button_rect(section_rect, 1u, 4u);
    return (SDL_Rect){inc.x + inc.w + 8, inc.y, 86, inc.h};
}

static SDL_Rect authoring_font_reset_rect(SDL_Rect section_rect) {
    SDL_Rect value = authoring_font_value_rect(section_rect);
    return (SDL_Rect){value.x + value.w + 8, value.y, 74, value.h};
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
    const DrawingProgramAppContext *ctx,
    int point_x,
    int point_y) {
    SDL_Rect panel_rect;
    SDL_Rect section_rect;
    if (viewport_width <= 0 || viewport_height <= 0) {
        return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_NONE;
    }
    if (ctx && drawing_program_authoring_host_font_theme_overlay_active(ctx)) {
        panel_rect = authoring_font_panel_rect(viewport_width, viewport_height);
        if (authoring_point_in_rect(authoring_mode_rect(panel_rect), point_x, point_y)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CYCLE_OVERLAY;
        }
        if (authoring_point_in_rect(authoring_apply_rect(panel_rect), point_x, point_y)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_APPLY;
        }
        if (authoring_point_in_rect(authoring_cancel_rect(panel_rect), point_x, point_y)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CANCEL;
        }

        section_rect = authoring_font_section_rect(panel_rect, 1u);
        if (authoring_point_in_rect(authoring_font_button_rect(section_rect, 0u, 4u), point_x, point_y)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_ZOOM_DEC;
        }
        if (authoring_point_in_rect(authoring_font_button_rect(section_rect, 1u, 4u), point_x, point_y)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_ZOOM_INC;
        }
        if (authoring_point_in_rect(authoring_font_reset_rect(section_rect), point_x, point_y)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_ZOOM_RESET;
        }

        section_rect = authoring_font_section_rect(panel_rect, 0u);
        if (authoring_point_in_rect(authoring_font_button_rect(section_rect, 0u, 3u), point_x, point_y)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_PRESET_DAW;
        }
        if (authoring_point_in_rect(authoring_font_button_rect(section_rect, 1u, 3u), point_x, point_y)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_PRESET_IDE;
        }
        if (authoring_point_in_rect(authoring_font_button_rect(section_rect, 2u, 3u), point_x, point_y)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_PRESET_CUSTOM;
        }

        section_rect = authoring_font_section_rect(panel_rect, 2u);
        if (authoring_point_in_rect(authoring_font_button_rect(section_rect, 0u, 5u), point_x, point_y)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_DAW;
        }
        if (authoring_point_in_rect(authoring_font_button_rect(section_rect, 1u, 5u), point_x, point_y)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_STANDARD_GREY;
        }
        if (authoring_point_in_rect(authoring_font_button_rect(section_rect, 2u, 5u), point_x, point_y)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_MIDNIGHT;
        }
        if (authoring_point_in_rect(authoring_font_button_rect(section_rect, 3u, 5u), point_x, point_y)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_SOFT_LIGHT;
        }
        if (authoring_point_in_rect(authoring_font_button_rect(section_rect, 4u, 5u), point_x, point_y)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_GREYSCALE;
        }

        section_rect = authoring_font_section_rect(panel_rect, 3u);
        if (authoring_point_in_rect(authoring_font_button_rect(section_rect, 0u, 2u), point_x, point_y)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CUSTOM_THEME_CREATE;
        }
        if (authoring_point_in_rect(authoring_font_button_rect(section_rect, 1u, 2u), point_x, point_y)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CUSTOM_THEME_EDIT;
        }
        return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_NONE;
    }
    panel_rect = authoring_panel_rect(viewport_width, viewport_height, 4u);
    if (authoring_point_in_rect(authoring_mode_rect(panel_rect), point_x, point_y)) {
        return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CYCLE_OVERLAY;
    }
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

static void authoring_draw_section(SDL_Renderer *renderer,
                                   SDL_Rect rect,
                                   const char *title,
                                   const char *detail,
                                   SDL_Color fill,
                                   SDL_Color border,
                                   SDL_Color text,
                                   SDL_Color muted) {
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, 224u);
    (void)SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, 230u);
    (void)SDL_RenderDrawRect(renderer, &rect);
    (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                  rect,
                                                  rect.x + 14,
                                                  rect.y + 12,
                                                  title,
                                                  text,
                                                  2);
    if (detail) {
        (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                      rect,
                                                      rect.x + 14,
                                                      rect.y + 38,
                                                      detail,
                                                      muted,
                                                      1);
    }
}

static void authoring_draw_font_theme_button(SDL_Renderer *renderer,
                                             SDL_Rect rect,
                                             const char *label,
                                             int active,
                                             int enabled,
                                             SDL_Color fill,
                                             SDL_Color active_fill,
                                             SDL_Color border,
                                             SDL_Color text,
                                             SDL_Color muted) {
    SDL_Color button_fill = active ? active_fill : fill;
    SDL_Color button_text = enabled ? text : muted;
    SDL_SetRenderDrawColor(renderer, button_fill.r, button_fill.g, button_fill.b, enabled ? 235u : 130u);
    (void)SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, enabled ? 238u : 120u);
    (void)SDL_RenderDrawRect(renderer, &rect);
    (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                  rect,
                                                  rect.x + 8,
                                                  rect.y + 6,
                                                  label,
                                                  button_text,
                                                  1);
}

static void authoring_draw_font_theme_overlay(SDL_Renderer *renderer,
                                              int viewport_width,
                                              int viewport_height,
                                              const DrawingProgramAppContext *ctx,
                                              const CoreThemePreset *theme,
                                              const VisualThemePalette *palette) {
    SDL_Rect screen_clip = {0, 0, viewport_width, viewport_height};
    SDL_Rect panel_rect = authoring_font_panel_rect(viewport_width, viewport_height);
    SDL_Rect title_band = {panel_rect.x, panel_rect.y, panel_rect.w, 30};
    SDL_Rect section_rect;
    SDL_Rect value_rect;
    char line[160];
    const char *font_name = "unknown";
    const char *theme_name = "unknown";
    uint32_t i;
    static const struct {
        DrawingProgramAuthoringChromeAction action;
        CoreFontPresetId preset_id;
        const char *label;
        int enabled;
    } k_font_buttons[] = {
        { DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_PRESET_DAW, CORE_FONT_PRESET_DAW_DEFAULT, "daw_default", 1 },
        { DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_PRESET_IDE, CORE_FONT_PRESET_IDE, "ide", 1 },
        { DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_PRESET_CUSTOM, CORE_FONT_PRESET_IDE, "custom slot", 0 }
    };
    static const struct {
        DrawingProgramAuthoringChromeAction action;
        CoreThemePresetId preset_id;
        const char *label;
    } k_theme_buttons[] = {
        { DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_DAW, CORE_THEME_PRESET_DAW_DEFAULT, "daw" },
        { DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_STANDARD_GREY, CORE_THEME_PRESET_IDE_GRAY, "grey" },
        { DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_MIDNIGHT, CORE_THEME_PRESET_DARK_DEFAULT, "midnight" },
        { DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_SOFT_LIGHT, CORE_THEME_PRESET_LIGHT_DEFAULT, "light" },
        { DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_GREYSCALE, CORE_THEME_PRESET_GREYSCALE, "mono" }
    };
    (void)theme;
    if (!renderer || !ctx || !palette) {
        return;
    }
    font_name = core_font_preset_name((CoreFontPresetId)ctx->ui.font_preset_id);
    if (!font_name) {
        font_name = "unknown";
    }
    theme_name = core_theme_preset_name((CoreThemePresetId)ctx->ui.theme_preset_id);
    if (!theme_name) {
        theme_name = "unknown";
    }

    SDL_SetRenderDrawColor(renderer,
                           palette->app_background.r,
                           palette->app_background.g,
                           palette->app_background.b,
                           255u);
    (void)SDL_RenderClear(renderer);
    authoring_draw_panel(renderer, panel_rect, palette->pane_background_alt, palette->accent_primary);
    SDL_SetRenderDrawColor(renderer,
                           palette->accent_primary.r,
                           palette->accent_primary.g,
                           palette->accent_primary.b,
                           238u);
    (void)SDL_RenderFillRect(renderer, &title_band);
    (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                  screen_clip,
                                                  panel_rect.x + 12,
                                                  panel_rect.y + 7,
                                                  "AUTHORING: FONT + THEME",
                                                  palette->app_background,
                                                  1);
    authoring_draw_button(renderer,
                          authoring_mode_rect(panel_rect),
                          "PANE",
                          palette->pane_background,
                          palette->text_primary,
                          palette->text_primary);
    authoring_draw_button(renderer,
                          authoring_apply_rect(panel_rect),
                          "APPLY",
                          palette->status_ok,
                          palette->text_primary,
                          palette->app_background);
    authoring_draw_button(renderer,
                          authoring_cancel_rect(panel_rect),
                          "CANCEL",
                          palette->status_warn,
                          palette->text_primary,
                          palette->app_background);

    section_rect = authoring_font_section_rect(panel_rect, 0u);
    (void)snprintf(line, sizeof(line), "Current font preset: %s", font_name);
    authoring_draw_section(renderer,
                           section_rect,
                           "Font Preset",
                           line,
                           palette->pane_background,
                           palette->accent_primary,
                           palette->text_primary,
                           palette->text_muted);
    for (i = 0u; i < 3u; ++i) {
        authoring_draw_font_theme_button(renderer,
                                         authoring_font_button_rect(section_rect, i, 3u),
                                         k_font_buttons[i].label,
                                         ctx->ui.font_preset_id == (uint32_t)k_font_buttons[i].preset_id,
                                         k_font_buttons[i].enabled,
                                         palette->pane_background_alt,
                                         palette->accent_primary,
                                         palette->text_muted,
                                         palette->text_primary,
                                         palette->text_muted);
    }

    section_rect = authoring_font_section_rect(panel_rect, 1u);
    (void)snprintf(line, sizeof(line), "Current size step: %+d  (%d%%)", (int)ctx->ui.font_zoom_step,
                   100 + ((int)ctx->ui.font_zoom_step * 12));
    authoring_draw_section(renderer,
                           section_rect,
                           "Font Size",
                           line,
                           palette->pane_background,
                           palette->accent_primary,
                           palette->text_primary,
                           palette->text_muted);
    authoring_draw_font_theme_button(renderer,
                                     authoring_font_button_rect(section_rect, 0u, 4u),
                                     "-",
                                     0,
                                     1,
                                     palette->pane_background_alt,
                                     palette->accent_primary,
                                     palette->text_muted,
                                     palette->text_primary,
                                     palette->text_muted);
    authoring_draw_font_theme_button(renderer,
                                     authoring_font_button_rect(section_rect, 1u, 4u),
                                     "+",
                                     0,
                                     1,
                                     palette->pane_background_alt,
                                     palette->accent_primary,
                                     palette->text_muted,
                                     palette->text_primary,
                                     palette->text_muted);
    value_rect = authoring_font_value_rect(section_rect);
    (void)snprintf(line, sizeof(line), "%+d", (int)ctx->ui.font_zoom_step);
    authoring_draw_font_theme_button(renderer,
                                     value_rect,
                                     line,
                                     1,
                                     1,
                                     palette->pane_background_alt,
                                     palette->accent_primary,
                                     palette->text_muted,
                                     palette->text_primary,
                                     palette->text_muted);
    authoring_draw_font_theme_button(renderer,
                                     authoring_font_reset_rect(section_rect),
                                     "reset",
                                     ctx->ui.font_zoom_step == 0,
                                     1,
                                     palette->pane_background_alt,
                                     palette->accent_primary,
                                     palette->text_muted,
                                     palette->text_primary,
                                     palette->text_muted);

    section_rect = authoring_font_section_rect(panel_rect, 2u);
    (void)snprintf(line, sizeof(line), "Current theme preset: %s", theme_name);
    authoring_draw_section(renderer,
                           section_rect,
                           "Theme Preset",
                           line,
                           palette->pane_background,
                           palette->accent_primary,
                           palette->text_primary,
                           palette->text_muted);
    for (i = 0u; i < 5u; ++i) {
        authoring_draw_font_theme_button(renderer,
                                         authoring_font_button_rect(section_rect, i, 5u),
                                         k_theme_buttons[i].label,
                                         ctx->ui.theme_preset_id == (uint32_t)k_theme_buttons[i].preset_id,
                                         1,
                                         palette->pane_background_alt,
                                         palette->accent_primary,
                                         palette->text_muted,
                                         palette->text_primary,
                                         palette->text_muted);
    }

    section_rect = authoring_font_section_rect(panel_rect, 3u);
    authoring_draw_section(renderer,
                           section_rect,
                           "Custom Presets",
                           ctx->authoring_host.custom_theme_status_active
                               ? ctx->authoring_host.custom_theme_status
                               : "Drawing host exposes the custom-preset lane as a stub until the theme editor is promoted.",
                           palette->pane_background,
                           palette->accent_primary,
                           palette->text_primary,
                           palette->text_muted);
    authoring_draw_font_theme_button(renderer,
                                     authoring_font_button_rect(section_rect, 0u, 2u),
                                     "create slot",
                                     0,
                                     1,
                                     palette->pane_background_alt,
                                     palette->accent_primary,
                                     palette->text_muted,
                                     palette->text_primary,
                                     palette->text_muted);
    authoring_draw_font_theme_button(renderer,
                                     authoring_font_button_rect(section_rect, 1u, 2u),
                                     "edit slot",
                                     0,
                                     1,
                                     palette->pane_background_alt,
                                     palette->accent_primary,
                                     palette->text_muted,
                                     palette->text_primary,
                                     palette->text_muted);

    (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                  screen_clip,
                                                  panel_rect.x + 18,
                                                  panel_rect.y + panel_rect.h - 26,
                                                  "Tab returns to pane authoring  Ctrl/Cmd +/- changes size  Enter applies  Esc cancels",
                                                  palette->text_muted,
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

    (void)SDL_GetRenderDrawBlendMode(renderer, &previous_blend);
    (void)SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    resolve_visual_theme_palette(theme, &palette);
    if (drawing_program_authoring_host_font_theme_overlay_active(ctx)) {
        authoring_draw_font_theme_overlay(renderer, viewport_width, viewport_height, ctx, theme, &palette);
        (void)SDL_SetRenderDrawBlendMode(renderer, previous_blend);
        return;
    }

    panel_fill = palette.pane_background_alt;
    row_count = drawing_program_visual_authoring_chrome_build_pane_rows(ctx, rows, MAX_ROWS);
    screen_clip = (SDL_Rect){0, 0, viewport_width, viewport_height};
    panel_rect = authoring_panel_rect(viewport_width, viewport_height, row_count);
    title_band = (SDL_Rect){panel_rect.x, panel_rect.y, panel_rect.w, 30};

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
                          authoring_mode_rect(panel_rect),
                          "FONT",
                          palette.pane_background,
                          palette.text_primary,
                          palette.text_primary);
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
                                                  "Tab opens font/theme  Enter applies  Esc cancels  Alt+C then Alt+V toggles",
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
