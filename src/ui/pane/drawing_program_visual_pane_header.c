#include "drawing_program/drawing_program_visual_pane_header.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_ui_controls.h"
#include "drawing_program/drawing_program_visual_input_workspace_view.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_visual_pane_geometry.h"
#include "drawing_program/drawing_program_visual_text_render.h"
#include "drawing_program/drawing_program_visual_theme.h"
#include "kit_pane_composition_sdl.h"
#include "../panel/drawing_program_ui_button.h"

typedef struct PaneHeaderPolicy {
    uint32_t module, action;
    const char *title, *label;
} PaneHeaderPolicy;

static const PaneHeaderPolicy policies[] = {
    {1u, DRAWING_UI_PANE_HEADER_FIT, "VIEWPORT", "FIT"},
    {2u, DRAWING_UI_PANE_HEADER_LAYOUT, "LEFT PANEL", "LAYOUT"},
    {4u, DRAWING_UI_PANE_HEADER_LAYOUT, "RIGHT PANEL", "LAYOUT"}
};

static const PaneHeaderPolicy *policy_for(
    const DrawingProgramAppContext *ctx, CorePaneId id) {
    uint32_t module = drawing_program_visual_module_type_for_pane(ctx, id);
    for (unsigned i = 0; i < sizeof(policies) / sizeof(policies[0]); ++i)
        if (policies[i].module == module)
            return &policies[i];
    return NULL;
}

CoreResult drawing_program_visual_pane_header_layout(
    const DrawingProgramAppContext *ctx, const KitPaneCompositionEntry *pane,
    KitPaneHeaderLayout *out) {
    if (!ctx || !pane || !out)
        return (CoreResult){CORE_ERR_INVALID_ARG, "missing pane header"};
    const PaneHeaderPolicy *policy = policy_for(ctx, pane->id);
    VisualPaneLayoutMetrics m = make_pane_layout_metrics(ctx);
    KitPaneHeaderAction action = {0};
    float title_min = 0;
    if (policy) {
        title_min = (float)drawing_program_visual_measure_bitmap_text_width(
            policy->title, m.title_scale);
        action.id = policy->action;
        action.width = (float)(drawing_program_visual_measure_bitmap_text_width(
            policy->label, m.body_scale) + 2 * m.pad_x);
        action.enabled = pane->enabled &&
            !drawing_program_authoring_host_active(ctx) &&
            !drawing_program_pane_host_splitter_drag_active(ctx);
    }
    return kit_pane_header_layout(out, pane->header, (float)m.section_gap,
                                  (float)m.tab_gap, title_min,
                                  policy ? &action : NULL, policy ? 1u : 0u);
}

int drawing_program_visual_pane_header_draw(
    SDL_Renderer *renderer, const DrawingProgramAppContext *ctx,
    const KitPaneCompositionEntry *pane, const CoreThemePreset *theme) {
    if (!renderer || !ctx || !pane)
        return 0;
    const PaneHeaderPolicy *policy = policy_for(ctx, pane->id);
    if (!policy || pane->visible_header.width <= 0 || pane->visible_header.height <= 0)
        return 1;
    KitPaneHeaderLayout layout;
    if (drawing_program_visual_pane_header_layout(ctx, pane, &layout).code != CORE_OK)
        return 0;
    /* Reuse the existing nested SDL scope with the validated header region. */
    KitPaneCompositionEntry region = *pane;
    region.content = pane->header;
    region.visible_content = pane->visible_header;
    KitPaneSdlClip saved;
    if (kit_pane_content_begin_sdl(renderer, &region, &saved).code != CORE_OK)
        return 0;
    Uint8 r, g, b, a;
    SDL_BlendMode blend;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
    SDL_GetRenderDrawBlendMode(renderer, &blend);
    VisualThemePalette p;
    resolve_visual_theme_palette(theme, &p);
    VisualPaneLayoutMetrics m = make_pane_layout_metrics(ctx);
    SDL_Rect header = drawing_program_visual_pane_pixel_rect(pane->visible_header);
    SDL_SetRenderDrawColor(renderer, p.pane_background_alt.r, p.pane_background_alt.g,
                          p.pane_background_alt.b, p.pane_background_alt.a);
    SDL_RenderFillRect(renderer, &header);
    SDL_Rect title = drawing_program_visual_pane_pixel_rect(layout.title);
    int text_height = drawing_program_visual_text_line_height(m.title_scale);
    drawing_program_visual_draw_bitmap_text(renderer, title, title.x,
        header.y + (header.h - text_height) / 2, policy->title,
        p.text_primary, m.title_scale);
    static const DrawingProgramVisualPanelRenderHooks hooks = {
        .measure_bitmap_text_width = drawing_program_visual_measure_bitmap_text_width,
        .draw_bitmap_text = drawing_program_visual_draw_bitmap_text
    };
    for (uint32_t i = 0; i < layout.count; ++i) {
        KitPaneHeaderSlot slot = layout.actions[i];
        drawing_program_ui_controls_key(slot.id, pane->id, slot.enabled);
        (void)drawing_program_ui_button_draw(renderer, header,
            drawing_program_visual_pane_pixel_rect(slot.bounds), policy->label,
            p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
            p.text_primary, p.text_muted, m.body_scale, 0, 0,
            KIT_UI_BUTTON_VARIANT_DEFAULT, &hooks);
    }
    SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g,
                          p.button_border.b, p.button_border.a);
    SDL_RenderDrawLine(renderer, header.x, header.y + header.h - 1,
                       header.x + header.w - 1, header.y + header.h - 1);
    SDL_SetRenderDrawBlendMode(renderer, blend);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    return kit_pane_content_end_sdl(renderer, &saved).code == CORE_OK;
}

int drawing_program_visual_pane_header_action(
    DrawingProgramAppContext *ctx, KitUiSurfaceKey key) {
    if (!ctx || (key.domain != DRAWING_UI_PANE_HEADER_FIT &&
                 key.domain != DRAWING_UI_PANE_HEADER_LAYOUT))
        return 0;
    /* Recognized header actions never fall through to coordinate-based content
     * dispatch, even when the domain owner disappeared or became ineligible. */
    const KitPaneCompositionEntry *pane = kit_pane_composition_find(
        &ctx->pane_host.composition_host.view, (CorePaneId)key.value);
    const PaneHeaderPolicy *policy = policy_for(ctx, (CorePaneId)key.value);
    if (key.value > UINT32_MAX || !pane || !policy || policy->action != key.domain ||
        pane->visible_header.width <= 0 || pane->visible_header.height <= 0 ||
        ctx->pane_host.composition_host.blocked)
        return 1;
    KitPaneHeaderLayout layout;
    if (drawing_program_visual_pane_header_layout(ctx, pane, &layout).code != CORE_OK)
        return 1;
    for (uint32_t i = 0; i < layout.count; ++i) {
        const KitPaneHeaderSlot *slot = &layout.actions[i];
        if (slot->id != key.domain || !slot->enabled)
            continue;
        if (key.domain == DRAWING_UI_PANE_HEADER_LAYOUT)
            (void)drawing_program_authoring_host_enter(ctx);
        else
            (void)drawing_program_visual_input_workspace_view_fit_all_or_reset(ctx);
        break;
    }
    return 1;
}
