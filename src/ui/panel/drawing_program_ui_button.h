#ifndef DRAWING_PROGRAM_UI_BUTTON_H
#define DRAWING_PROGRAM_UI_BUTTON_H

#include <SDL2/SDL.h>

#include "kit_ui.h"
#include "drawing_program/drawing_program_visual_panel_render.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef KitUiButtonState DrawingProgramUiButtonState;
typedef KitUiButtonSpec DrawingProgramUiButtonSpec;
typedef KitUiButtonStyle DrawingProgramUiButtonStyle;
typedef KitUiButtonVariant DrawingProgramUiButtonVariant;

void drawing_program_ui_button_state_init(DrawingProgramUiButtonState *state);
void drawing_program_ui_button_spec_init(DrawingProgramUiButtonSpec *spec, const char *label);

int drawing_program_ui_button_style_resolve(SDL_Color fill,
                                            SDL_Color fill_hover,
                                            SDL_Color fill_active,
                                            SDL_Color border,
                                            SDL_Color text_primary,
                                            SDL_Color text_muted,
                                            const DrawingProgramUiButtonSpec *spec,
                                            DrawingProgramUiButtonStyle *out_style);

int drawing_program_ui_button_draw_frame(SDL_Renderer *renderer,
                                         SDL_Rect rect,
                                         const DrawingProgramUiButtonStyle *style);

int drawing_program_ui_button_draw(SDL_Renderer *renderer,
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
                                   DrawingProgramUiButtonVariant variant,
                                   const DrawingProgramVisualPanelRenderHooks *hooks);

#ifdef __cplusplus
}
#endif

#endif
