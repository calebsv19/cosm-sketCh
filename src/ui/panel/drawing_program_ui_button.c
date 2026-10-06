#include "drawing_program_ui_button.h"
#include "kit_ui_sdl.h"
#include "drawing_program/drawing_program_ui_controls.h"
#include "drawing_program/drawing_program_visual_text_render.h"

#include "drawing_program/drawing_program_visual_theme.h"

static CoreThemeColor drawing_program_ui_button_core_color(SDL_Color color) {
    CoreThemeColor out;
    out.r = color.r;
    out.g = color.g;
    out.b = color.b;
    out.a = color.a;
    return out;
}

void drawing_program_ui_button_state_init(DrawingProgramUiButtonState *state) {
    kit_ui_button_state_init(state);
}

void drawing_program_ui_button_spec_init(DrawingProgramUiButtonSpec *spec, const char *label) {
    kit_ui_button_spec_init(spec, label);
}

int drawing_program_ui_button_style_resolve(SDL_Color fill,
                                            SDL_Color fill_hover,
                                            SDL_Color fill_active,
                                            SDL_Color border,
                                            SDL_Color text_primary,
                                            SDL_Color text_muted,
                                            const DrawingProgramUiButtonSpec *spec,
                                            DrawingProgramUiButtonStyle *out_style) {
    KitUiButtonTheme theme;

    if (!spec || !out_style) {
        return 1;
    }

    theme.idle_fill = drawing_program_ui_button_core_color(fill);
    theme.selected_fill = drawing_program_ui_button_core_color(fill_active);
    theme.hover_fill = drawing_program_ui_button_core_color(fill_hover);
    theme.positive_fill = drawing_program_ui_button_core_color(fill_active);
    theme.outline_idle = drawing_program_ui_button_core_color(border);
    theme.outline_highlight = drawing_program_ui_button_core_color(border);
    theme.text_primary = drawing_program_ui_button_core_color(text_primary);
    theme.text_muted = drawing_program_ui_button_core_color(text_muted);
    return kit_ui_button_style_resolve(&theme, spec, out_style) != 0 ? 0 : 1;
}

int drawing_program_ui_button_draw_frame(SDL_Renderer *renderer,
                                         SDL_Rect rect,
                                         const DrawingProgramUiButtonStyle *style) {
    if (!renderer || !style || rect.w<=0 || rect.h<=0) return 1;
    KitUiButtonAppearance appearance;
    kit_ui_button_appearance_preset(KIT_UI_BUTTON_APPEARANCE_COMPACT_ROUNDED,&appearance);
    kit_ui_sdl_fill_rounded_rect(renderer,&rect,(int)appearance.corner_radius,
        (KitRenderColor){style->outline.r,style->outline.g,style->outline.b,style->outline.a});
    SDL_Rect inner={rect.x+1,rect.y+1,rect.w-2,rect.h-2};
    kit_ui_sdl_fill_rounded_rect(renderer,&inner,(int)appearance.corner_radius-1,
        (KitRenderColor){style->fill.r,style->fill.g,style->fill.b,style->fill.a});
    return 0;
}

typedef struct ButtonText { const DrawingProgramVisualPanelRenderHooks *hooks; SDL_Rect clip; } ButtonText;
static int button_measure(void *user,const char *text,int scale,int *w,int *h) {
    ButtonText *host=user;
    *w=host->hooks->measure_bitmap_text_width(text,scale);
    *h=drawing_program_visual_text_line_height(scale);
    return 1;
}
static int button_line_height(void *user,int scale) {
    (void)user;return drawing_program_visual_text_line_height(scale);
}
static void button_text(void *user,SDL_Renderer *renderer,const SDL_Rect *clip,int x,int y,
                        const char *text,int scale,KitRenderColor color) {
    ButtonText *host=user; SDL_Rect visible;
    if (!SDL_IntersectRect(clip,&host->clip,&visible)) return;
    host->hooks->draw_bitmap_text(renderer,visible,x,y,text,(SDL_Color){color.r,color.g,color.b,color.a},scale);
}
int drawing_program_ui_button_draw_spec(SDL_Renderer *renderer,SDL_Rect clip,SDL_Rect rect,
    const DrawingProgramUiButtonSpec *spec,const KitUiButtonTheme *theme,int scale,
    const DrawingProgramVisualPanelRenderHooks *hooks) {
    if (!renderer || !spec || !theme || !hooks || !hooks->draw_bitmap_text || !hooks->measure_bitmap_text_width)
        return 1;
    SDL_Rect previous,visible=clip;SDL_bool clipped=SDL_RenderIsClipEnabled(renderer);
    SDL_RenderGetClipRect(renderer,&previous);
    if (clipped && !SDL_IntersectRect(&clip,&previous,&visible)) return 0;
    Uint8 cr,cg,cb,ca;SDL_BlendMode blend;
    SDL_GetRenderDrawColor(renderer,&cr,&cg,&cb,&ca);SDL_GetRenderDrawBlendMode(renderer,&blend);
    (void)SDL_RenderSetClipRect(renderer,&visible);
    KitUiButtonSpec current=*spec;
    drawing_program_ui_controls_button(renderer,clip,rect,&current);
    ButtonText host={hooks,clip};
    KitUiSdlTextApi text={&host,scale,4,button_measure,button_line_height,button_text};
    KitUiButtonAppearance appearance;
    kit_ui_button_appearance_preset(KIT_UI_BUTTON_APPEARANCE_COMPACT_ROUNDED,&appearance);
    if (scale>1) appearance.corner_radius*=scale;
    kit_ui_sdl_draw_button_spec_appearance(renderer,&rect,&current,theme,&appearance,&text);
    (void)SDL_RenderSetClipRect(renderer,clipped?&previous:NULL);
    (void)SDL_SetRenderDrawBlendMode(renderer,blend);(void)SDL_SetRenderDrawColor(renderer,cr,cg,cb,ca);
    return 0;
}

int drawing_program_ui_button_draw(SDL_Renderer *renderer,SDL_Rect clip,SDL_Rect rect,
    const char *label,SDL_Color fill,SDL_Color hover,SDL_Color active,SDL_Color border,
    SDL_Color primary,SDL_Color muted,int scale,int selected,int hovered,
    DrawingProgramUiButtonVariant variant,const DrawingProgramVisualPanelRenderHooks *hooks) {
    KitUiButtonTheme theme={drawing_program_ui_button_core_color(fill),
        drawing_program_ui_button_core_color(active),drawing_program_ui_button_core_color(hover),
        drawing_program_ui_button_core_color(active),drawing_program_ui_button_core_color(border),
        drawing_program_ui_button_core_color(border),drawing_program_ui_button_core_color(primary),
        drawing_program_ui_button_core_color(muted)};
    DrawingProgramUiButtonSpec spec;
    kit_ui_button_spec_init(&spec,label);spec.variant=variant;
    spec.state.selected=!!selected;spec.state.hovered=!!hovered;
    return drawing_program_ui_button_draw_spec(renderer,clip,rect,&spec,&theme,scale,hooks);
}
