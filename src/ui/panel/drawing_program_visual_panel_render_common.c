#include "drawing_program/drawing_program_visual_panel_render_common.h"

#include "drawing_program_ui_button.h"
#include "drawing_program/drawing_program_visual_theme.h"

int drawing_program_visual_panel_ui_hovered(const VisualPanelUiState *ui,
                                            SDL_Rect rect,
                                            const DrawingProgramVisualPanelRenderHooks *hooks) {
    if (!ui || !ui->mouse_known || !hooks || !hooks->point_in_rect) {
        return 0;
    }
    return hooks->point_in_rect(rect, ui->mouse_x, ui->mouse_y);
}

void drawing_program_visual_panel_draw_tab_button(SDL_Renderer *renderer,
                                                  SDL_Rect clip_rect,
                                                  SDL_Rect rect,
                                                  const char *label,
                                                  SDL_Color fill,
                                                  SDL_Color fill_hover,
                                                  SDL_Color fill_active,
                                                  SDL_Color border,
                                                  SDL_Color text,
                                                  int text_scale,
                                                  int selected,
                                                  int hovered,
                                                  const DrawingProgramVisualPanelRenderHooks *hooks) {
    drawing_program_visual_panel_draw_tab_button_variant(renderer,
                                                         clip_rect,
                                                         rect,
                                                         label,
                                                         fill,
                                                         fill_hover,
                                                         fill_active,
                                                         border,
                                                         text,
                                                         text_scale,
                                                         selected,
                                                         hovered,
                                                         0,
                                                         hooks);
}

void drawing_program_visual_panel_draw_tab_button_variant(SDL_Renderer *renderer,
                                                          SDL_Rect clip_rect,
                                                          SDL_Rect rect,
                                                          const char *label,
                                                          SDL_Color fill,
                                                          SDL_Color fill_hover,
                                                          SDL_Color fill_active,
                                                          SDL_Color border,
                                                          SDL_Color text,
                                                          int text_scale,
                                                          int selected,
                                                          int hovered,
                                                          int positive,
                                                          const DrawingProgramVisualPanelRenderHooks *hooks) {
    DrawingProgramUiButtonVariant variant = positive ? KIT_UI_BUTTON_VARIANT_POSITIVE
                                                     : KIT_UI_BUTTON_VARIANT_DEFAULT;
    (void)drawing_program_ui_button_draw(renderer,
                                         clip_rect,
                                         rect,
                                         label,
                                         fill,
                                         fill_hover,
                                         fill_active,
                                         border,
                                         text,
                                         text,
                                         text_scale,
                                         selected,
                                         hovered,
                                         variant,
                                         hooks);
}

void drawing_program_visual_panel_draw_themed_button(SDL_Renderer *renderer,
                                                     SDL_Rect clip_rect,
                                                     SDL_Rect rect,
                                                     const char *label,
                                                     SDL_Color text_color,
                                                     int selected,
                                                     const VisualPanelUiState *ui,
                                                     VisualPaneLayoutMetrics metrics,
                                                     VisualThemePalette palette,
                                                     const DrawingProgramVisualPanelRenderHooks *hooks) {
    drawing_program_visual_panel_draw_tab_button(renderer,
                                                 clip_rect,
                                                 rect,
                                                 label,
                                                 palette.button_fill,
                                                 palette.button_fill_hover,
                                                 palette.button_fill_active,
                                                 palette.button_border,
                                                 text_color,
                                                 metrics.body_scale,
                                                 selected,
                                                 drawing_program_visual_panel_ui_hovered(ui, rect, hooks),
                                                 hooks);
}

void drawing_program_visual_panel_draw_row_button_variant(SDL_Renderer *renderer,
                                                          SDL_Rect clip_rect,
                                                          SDL_Rect rect,
                                                          const char *label,
                                                          SDL_Color fill,
                                                          SDL_Color fill_hover,
                                                          SDL_Color fill_active,
                                                          SDL_Color border,
                                                          SDL_Color text_primary,
                                                          SDL_Color text_muted,
                                                          int text_scale,
                                                          int selected,
                                                          int hovered,
                                                          int disabled,
                                                          int positive,
                                                          const DrawingProgramVisualPanelRenderHooks *hooks) {
    DrawingProgramUiButtonSpec spec;
    DrawingProgramUiButtonStyle style;
    SDL_Color text_color;
    SDL_Color fill_color;
    int glyph_h;
    int text_y;

    if (!renderer || !label || !hooks || !hooks->draw_bitmap_text || rect.w <= 0 || rect.h <= 0) {
        return;
    }

    drawing_program_ui_button_spec_init(&spec, label);
    spec.variant = positive ? KIT_UI_BUTTON_VARIANT_POSITIVE : KIT_UI_BUTTON_VARIANT_DEFAULT;
    spec.state.selected = selected ? 1 : 0;
    spec.state.focused = selected ? 1 : 0;
    spec.state.hovered = hovered ? 1 : 0;
    spec.state.disabled = disabled ? 1 : 0;
    if (drawing_program_ui_button_style_resolve(fill,
                                                fill_hover,
                                                fill_active,
                                                border,
                                                text_primary,
                                                text_muted,
                                                &spec,
                                                &style) != 0) {
        return;
    }
    if (drawing_program_ui_button_draw_frame(renderer, rect, &style) != 0) {
        return;
    }

    fill_color = (SDL_Color){ style.fill.r, style.fill.g, style.fill.b, style.fill.a };
    text_color = sdl_color_ensure_contrast((SDL_Color){ style.text.r, style.text.g, style.text.b, style.text.a },
                                           fill_color);
    glyph_h = 7 * text_scale;
    text_y = rect.y + ((rect.h - glyph_h) / 2);
    if (text_y < rect.y + 2) {
        text_y = rect.y + 2;
    }
    hooks->draw_bitmap_text(renderer, clip_rect, rect.x + 6, text_y, label, text_color, text_scale);
}

const char *drawing_program_visual_shape_target_mode_name(uint8_t mode) {
    return (mode == (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_OBJECT) ? "OBJECT" : "PIXEL";
}

const char *drawing_program_visual_select_mode_name(uint8_t mode) {
    switch (mode) {
        case (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_ADD: return "ADD";
        case (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_SUBTRACT: return "SUBTRACT";
        case (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE:
        default:
            return "REPLACE";
    }
}
