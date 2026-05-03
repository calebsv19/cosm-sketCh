#include "drawing_program/drawing_program_visual_authoring_chrome.h"

#include <stdio.h>
#include <string.h>

#include "core_font.h"
#include "core_layout.h"
#include "core_pane_module.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_visual_text_render.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_theme.h"
#include "kit_workspace_authoring_ui.h"

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

static int authoring_point_in_rect(SDL_Rect rect, int point_x, int point_y) {
    return point_x >= rect.x &&
           point_y >= rect.y &&
           point_x < rect.x + rect.w &&
           point_y < rect.y + rect.h;
}

static SDL_Rect authoring_rect_from_kit(KitRenderRect rect) {
    SDL_Rect out = {
        (int)(rect.x + 0.5f),
        (int)(rect.y + 0.5f),
        (int)(rect.width + 0.5f),
        (int)(rect.height + 0.5f)
    };
    if (out.w < 0) {
        out.w = 0;
    }
    if (out.h < 0) {
        out.h = 0;
    }
    return out;
}

static SDL_Rect authoring_rect_from_core(CorePaneRect rect) {
    SDL_Rect out = {
        (int)(rect.x + 0.5f),
        (int)(rect.y + 0.5f),
        (int)(rect.width + 0.5f),
        (int)(rect.height + 0.5f)
    };
    if (out.w < 0) {
        out.w = 0;
    }
    if (out.h < 0) {
        out.h = 0;
    }
    return out;
}

static int authoring_font_theme_preview_text_scale(const DrawingProgramAppContext *ctx) {
    int scale = ui_scale_for_text(ctx, 1);
    if (scale < 1) {
        scale = 1;
    }
    if (scale > 4) {
        scale = 4;
    }
    return scale;
}

static int authoring_font_theme_control_text_scale(const DrawingProgramAppContext *ctx) {
    int scale = authoring_font_theme_preview_text_scale(ctx);
    if (scale > 2) {
        scale = 2;
    }
    return scale;
}

static KitWorkspaceAuthoringFontThemeButtonId authoring_shared_font_button_id_at(uint32_t index) {
    switch (index) {
        case 0u:
            return KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_FONT_PRESET_DAW_DEFAULT;
        case 1u:
            return KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_FONT_PRESET_IDE;
        case 2u:
            return KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_FONT_PRESET_CUSTOM_STUB;
        default:
            return KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_NONE;
    }
}

static KitWorkspaceAuthoringFontThemeButtonId authoring_shared_theme_button_id_at(uint32_t index) {
    switch (index) {
        case 0u:
            return KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_THEME_PRESET_DAW_DEFAULT;
        case 1u:
            return KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_THEME_PRESET_STANDARD_GREY;
        case 2u:
            return KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_THEME_PRESET_MIDNIGHT_CONTRAST;
        case 3u:
            return KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_THEME_PRESET_SOFT_LIGHT;
        case 4u:
            return KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_THEME_PRESET_GREYSCALE;
        default:
            return KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_NONE;
    }
}

static DrawingProgramAuthoringChromeAction authoring_action_from_overlay_button(
    KitWorkspaceAuthoringOverlayButtonId button_id) {
    switch (button_id) {
        case KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_MODE:
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CYCLE_OVERLAY;
        case KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_APPLY:
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_APPLY;
        case KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_CANCEL:
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CANCEL;
        case KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_ADD:
        case KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_NONE:
        default:
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_NONE;
    }
}

static DrawingProgramAuthoringChromeAction authoring_action_from_shared_font_button(
    KitWorkspaceAuthoringFontThemeButtonId button_id) {
    KitWorkspaceAuthoringFontThemeAction action =
        kit_workspace_authoring_ui_font_theme_action_for_button(button_id);
    switch (action.type) {
        case KIT_WORKSPACE_AUTHORING_FONT_THEME_ACTION_TEXT_SIZE_DEC:
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_ZOOM_DEC;
        case KIT_WORKSPACE_AUTHORING_FONT_THEME_ACTION_TEXT_SIZE_INC:
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_ZOOM_INC;
        case KIT_WORKSPACE_AUTHORING_FONT_THEME_ACTION_TEXT_SIZE_RESET:
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_ZOOM_RESET;
        case KIT_WORKSPACE_AUTHORING_FONT_THEME_ACTION_SET_FONT_PRESET:
            if (action.font_preset_id == CORE_FONT_PRESET_DAW_DEFAULT) {
                return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_PRESET_DAW;
            }
            if (action.font_preset_id == CORE_FONT_PRESET_IDE) {
                return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_PRESET_IDE;
            }
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_NONE;
        case KIT_WORKSPACE_AUTHORING_FONT_THEME_ACTION_SET_THEME_PRESET:
            switch (action.theme_preset_id) {
                case CORE_THEME_PRESET_DAW_DEFAULT:
                    return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_DAW;
                case CORE_THEME_PRESET_IDE_GRAY:
                    return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_STANDARD_GREY;
                case CORE_THEME_PRESET_DARK_DEFAULT:
                    return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_MIDNIGHT;
                case CORE_THEME_PRESET_LIGHT_DEFAULT:
                    return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_SOFT_LIGHT;
                case CORE_THEME_PRESET_GREYSCALE:
                    return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_GREYSCALE;
                default:
                    return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_NONE;
            }
        case KIT_WORKSPACE_AUTHORING_FONT_THEME_ACTION_CUSTOM_THEME_STATUS:
            if (button_id == KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_CUSTOM_THEME_CREATE_STUB) {
                return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CUSTOM_THEME_CREATE;
            }
            if (button_id == KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_CUSTOM_THEME_EDIT_STUB) {
                return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CUSTOM_THEME_EDIT;
            }
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_NONE;
        case KIT_WORKSPACE_AUTHORING_FONT_THEME_ACTION_NONE:
        default:
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_NONE;
    }
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
    if (viewport_width <= 0 || viewport_height <= 0) {
        return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_NONE;
    }
    if (ctx && drawing_program_authoring_host_font_theme_overlay_active(ctx)) {
        KitWorkspaceAuthoringOverlayButton top_buttons[4];
        KitWorkspaceAuthoringOverlayButtonId top_hit = KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_NONE;
        KitWorkspaceAuthoringFontThemeLayout layout = {0};
        KitWorkspaceAuthoringFontThemeButtonId shared_hit = KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_NONE;
        uint32_t top_button_count = kit_workspace_authoring_ui_build_overlay_buttons(
            viewport_width,
            1,
            0,
            top_buttons,
            (uint32_t)(sizeof(top_buttons) / sizeof(top_buttons[0])));
        top_hit = kit_workspace_authoring_ui_overlay_hit_test(top_buttons,
                                                             top_button_count,
                                                             (float)point_x,
                                                             (float)point_y);
        if (top_hit != KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_NONE) {
            return authoring_action_from_overlay_button(top_hit);
        }
        if (!kit_workspace_authoring_ui_font_theme_build_layout(NULL,
                                                                viewport_width,
                                                                viewport_height,
                                                                &layout)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_NONE;
        }
        shared_hit = kit_workspace_authoring_ui_font_theme_hit_button(&layout, (float)point_x, (float)point_y);
        if (shared_hit != KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_NONE &&
            !kit_workspace_authoring_ui_font_theme_button_enabled(shared_hit)) {
            return DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_NONE;
        }
        return authoring_action_from_shared_font_button(shared_hit);
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
                                   int title_scale,
                                   int detail_scale,
                                   SDL_Color fill,
                                   SDL_Color border,
                                   SDL_Color text,
                                   SDL_Color muted) {
    int detail_y;
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, 224u);
    (void)SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, 230u);
    (void)SDL_RenderDrawRect(renderer, &rect);
    if (title_scale < 1) {
        title_scale = 1;
    }
    if (detail_scale < 1) {
        detail_scale = 1;
    }
    (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                  rect,
                                                  rect.x + 14,
                                                  rect.y + 12,
                                                  title,
                                                  text,
                                                  title_scale);
    if (detail) {
        detail_y = rect.y + 16 + (9 * title_scale);
        (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                      rect,
                                                      rect.x + 14,
                                                      detail_y,
                                                      detail,
                                                      muted,
                                                      detail_scale);
    }
}

static void authoring_draw_font_theme_button(SDL_Renderer *renderer,
                                             SDL_Rect rect,
                                             const char *label,
                                             int active,
                                             int enabled,
                                             int text_scale,
                                             SDL_Color fill,
                                             SDL_Color active_fill,
                                             SDL_Color border,
                                             SDL_Color text,
                                             SDL_Color muted) {
    SDL_Color button_fill = active ? active_fill : fill;
    SDL_Color button_text = enabled ? text : muted;
    int text_y;
    if (text_scale < 1) {
        text_scale = 1;
    }
    text_y = rect.y + ((rect.h - (7 * text_scale)) / 2);
    if (text_y < rect.y + 2) {
        text_y = rect.y + 2;
    }
    SDL_SetRenderDrawColor(renderer, button_fill.r, button_fill.g, button_fill.b, enabled ? 235u : 130u);
    (void)SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, enabled ? 238u : 120u);
    (void)SDL_RenderDrawRect(renderer, &rect);
    (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                  rect,
                                                  rect.x + 8,
                                                  text_y,
                                                  label,
                                                  button_text,
                                                  text_scale);
}

static void authoring_draw_font_theme_overlay(SDL_Renderer *renderer,
                                              int viewport_width,
                                              int viewport_height,
                                              const DrawingProgramAppContext *ctx,
                                              const CoreThemePreset *theme,
                                              const VisualThemePalette *palette) {
    SDL_Rect screen_clip = {0, 0, viewport_width, viewport_height};
    SDL_Rect panel_rect = {0, 0, 0, 0};
    SDL_Rect section_rect;
    SDL_Rect value_rect;
    SDL_Rect button_rect;
    char line[160];
    const char *font_name = "unknown";
    const char *theme_name = "unknown";
    uint32_t i;
    KitWorkspaceAuthoringFontThemeLayout layout = {0};
    KitWorkspaceAuthoringOverlayButton top_buttons[4];
    uint32_t top_button_count = 0u;
    int preview_text_scale = 1;
    int control_text_scale = 1;
    int title_text_scale = 2;
    (void)theme;
    if (!renderer || !ctx || !palette) {
        return;
    }
    preview_text_scale = authoring_font_theme_preview_text_scale(ctx);
    control_text_scale = authoring_font_theme_control_text_scale(ctx);
    title_text_scale = preview_text_scale > 2 ? 3 : 2;
    if (!kit_workspace_authoring_ui_font_theme_build_layout(NULL,
                                                            viewport_width,
                                                            viewport_height,
                                                            &layout)) {
        return;
    }
    panel_rect = authoring_rect_from_kit(layout.panel);
    top_button_count = kit_workspace_authoring_ui_build_overlay_buttons(
        viewport_width,
        1,
        0,
        top_buttons,
        (uint32_t)(sizeof(top_buttons) / sizeof(top_buttons[0])));
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
    (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                  screen_clip,
                                                  panel_rect.x + 12,
                                                  panel_rect.y + 12,
                                                  "Font/Theme Overlay",
                                                  palette->text_primary,
                                                  title_text_scale);
    for (i = 0u; i < top_button_count; ++i) {
        SDL_Color fill = palette->pane_background;
        SDL_Color text = palette->text_primary;
        if (top_buttons[i].id == KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_APPLY) {
            fill = palette->status_ok;
            text = palette->app_background;
        } else if (top_buttons[i].id == KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_CANCEL) {
            fill = palette->status_warn;
            text = palette->app_background;
        }
        authoring_draw_button(renderer,
                              authoring_rect_from_core(top_buttons[i].rect),
                              top_buttons[i].label,
                              fill,
                              palette->text_primary,
                              text);
    }

    section_rect = authoring_rect_from_kit(layout.font_preset_section);
    (void)snprintf(line, sizeof(line), "Font Preset: %s", font_name);
    authoring_draw_section(renderer,
                           section_rect,
                           line,
                           NULL,
                           title_text_scale,
                           control_text_scale,
                           palette->pane_background,
                           palette->accent_primary,
                           palette->text_primary,
                           palette->text_muted);
    for (i = 0u; i < layout.font_preset_button_count &&
                i < KIT_WORKSPACE_AUTHORING_FONT_THEME_FONT_PRESET_BUTTON_COUNT; ++i) {
        CoreFontPresetId preset_id = CORE_FONT_PRESET_IDE;
        KitWorkspaceAuthoringFontThemeButtonId button_id = authoring_shared_font_button_id_at(i);
        int enabled = (int)kit_workspace_authoring_ui_font_theme_button_enabled(button_id);
        int active = 0;
        if (kit_workspace_authoring_ui_font_theme_button_font_preset_id(button_id, &preset_id)) {
            active = ctx->ui.font_preset_id == (uint32_t)preset_id;
        }
        authoring_draw_font_theme_button(renderer,
                                         authoring_rect_from_kit(layout.font_preset_buttons[i]),
                                         kit_workspace_authoring_ui_font_theme_button_label(button_id),
                                         active,
                                         enabled,
                                         control_text_scale,
                                         palette->pane_background_alt,
                                         palette->accent_primary,
                                         palette->text_muted,
                                         palette->text_primary,
                                         palette->text_muted);
    }

    section_rect = authoring_rect_from_kit(layout.text_size_section);
    (void)snprintf(line, sizeof(line), "Text Size step:%+d (%d%%)", (int)ctx->ui.font_zoom_step,
                   100 + ((int)ctx->ui.font_zoom_step * 12));
    authoring_draw_section(renderer,
                           section_rect,
                           line,
                           NULL,
                           title_text_scale,
                           control_text_scale,
                           palette->pane_background,
                           palette->accent_primary,
                           palette->text_primary,
                           palette->text_muted);
    (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                  section_rect,
                                                  section_rect.x + 14,
                                                  section_rect.y + 28 + (10 * title_text_scale),
                                                  "Sample UI text 123",
                                                  palette->text_primary,
                                                  preview_text_scale);
    authoring_draw_font_theme_button(renderer,
                                     authoring_rect_from_kit(layout.text_size_dec_button),
                                     kit_workspace_authoring_ui_font_theme_button_label(
                                         KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_TEXT_SIZE_DEC),
                                     0,
                                     1,
                                     control_text_scale,
                                     palette->pane_background_alt,
                                     palette->accent_primary,
                                     palette->text_muted,
                                     palette->text_primary,
                                     palette->text_muted);
    authoring_draw_font_theme_button(renderer,
                                     authoring_rect_from_kit(layout.text_size_inc_button),
                                     kit_workspace_authoring_ui_font_theme_button_label(
                                         KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_TEXT_SIZE_INC),
                                     0,
                                     1,
                                     control_text_scale,
                                     palette->pane_background_alt,
                                     palette->accent_primary,
                                     palette->text_muted,
                                     palette->text_primary,
                                     palette->text_muted);
    value_rect = authoring_rect_from_kit(layout.text_size_value_chip);
    (void)snprintf(line, sizeof(line), "%+d", (int)ctx->ui.font_zoom_step);
    authoring_draw_font_theme_button(renderer,
                                     value_rect,
                                     line,
                                     1,
                                     1,
                                     control_text_scale,
                                     palette->pane_background_alt,
                                     palette->accent_primary,
                                     palette->text_muted,
                                     palette->text_primary,
                                     palette->text_muted);
    authoring_draw_font_theme_button(renderer,
                                     authoring_rect_from_kit(layout.text_size_reset_button),
                                     kit_workspace_authoring_ui_font_theme_button_label(
                                         KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_TEXT_SIZE_RESET),
                                     ctx->ui.font_zoom_step == 0,
                                     1,
                                     control_text_scale,
                                     palette->pane_background_alt,
                                     palette->accent_primary,
                                     palette->text_muted,
                                     palette->text_primary,
                                     palette->text_muted);

    section_rect = authoring_rect_from_kit(layout.theme_preset_section);
    (void)snprintf(line, sizeof(line), "Theme Preset: %s", theme_name);
    authoring_draw_section(renderer,
                           section_rect,
                           line,
                           NULL,
                           title_text_scale,
                           control_text_scale,
                           palette->pane_background,
                           palette->accent_primary,
                           palette->text_primary,
                           palette->text_muted);
    for (i = 0u; i < layout.theme_preset_button_count &&
                i < KIT_WORKSPACE_AUTHORING_FONT_THEME_THEME_PRESET_BUTTON_COUNT; ++i) {
        CoreThemePresetId preset_id = CORE_THEME_PRESET_DARK_DEFAULT;
        KitWorkspaceAuthoringFontThemeButtonId button_id = authoring_shared_theme_button_id_at(i);
        int active = 0;
        if (kit_workspace_authoring_ui_font_theme_button_theme_preset_id(button_id, &preset_id)) {
            active = ctx->ui.theme_preset_id == (uint32_t)preset_id;
        }
        authoring_draw_font_theme_button(renderer,
                                         authoring_rect_from_kit(layout.theme_preset_buttons[i]),
                                         kit_workspace_authoring_ui_font_theme_button_label(button_id),
                                         active,
                                         1,
                                         control_text_scale,
                                         palette->pane_background_alt,
                                         palette->accent_primary,
                                         palette->text_muted,
                                         palette->text_primary,
                                         palette->text_muted);
    }

    section_rect = authoring_rect_from_kit(layout.custom_theme_section);
    authoring_draw_section(renderer,
                           section_rect,
                           "Custom Presets",
                           ctx->authoring_host.custom_theme_status_active
                               ? ctx->authoring_host.custom_theme_status
                               : "Drawing host exposes the custom-preset lane as a stub until the theme editor is promoted.",
                           title_text_scale,
                           1,
                           palette->pane_background,
                           palette->accent_primary,
                           palette->text_primary,
                           palette->text_muted);
    for (i = 0u; i < layout.custom_theme_button_count &&
                i < KIT_WORKSPACE_AUTHORING_FONT_THEME_CUSTOM_THEME_BUTTON_COUNT; ++i) {
        KitWorkspaceAuthoringFontThemeButtonId button_id =
            (i == 0u) ? KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_CUSTOM_THEME_CREATE_STUB
                      : KIT_WORKSPACE_AUTHORING_FONT_THEME_BUTTON_CUSTOM_THEME_EDIT_STUB;
        button_rect = authoring_rect_from_kit(layout.custom_theme_buttons[i]);
        authoring_draw_font_theme_button(renderer,
                                         button_rect,
                                         kit_workspace_authoring_ui_font_theme_button_label(button_id),
                                         0,
                                         (int)kit_workspace_authoring_ui_font_theme_button_enabled(button_id),
                                         control_text_scale,
                                         palette->pane_background_alt,
                                         palette->accent_primary,
                                         palette->text_muted,
                                         palette->text_primary,
                                         palette->text_muted);
    }

    (void)drawing_program_visual_draw_bitmap_text(renderer,
                                                  screen_clip,
                                                  panel_rect.x + 16,
                                                  panel_rect.y + panel_rect.h - 26,
                                                  "Tab returns to pane authoring  Ctrl/Cmd +/- changes size  Enter applies  Esc cancels",
                                                  palette->text_muted,
                                                  control_text_scale);
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
