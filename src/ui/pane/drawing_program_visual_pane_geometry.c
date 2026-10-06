#include "drawing_program/drawing_program_visual_pane_geometry.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include <math.h>
#include <limits.h>

KitPaneCompositionSpec drawing_program_visual_pane_spec(
    const DrawingProgramAppContext *ctx, const CorePaneLeafRect *leaf) {
    uint32_t module = drawing_program_visual_module_type_for_pane(ctx, leaf->id);
    VisualPaneLayoutMetrics m = make_pane_layout_metrics(ctx);
    float header = (module == 1u || module == 2u || module == 4u)
                       ? (float)(m.pad_y + m.title_glyph_h + m.section_gap)
                       : 0.0f;
    return (KitPaneCompositionSpec){leaf->id, leaf->rect, 0, header, 0, 1};
}

CoreResult drawing_program_visual_pane_entry(
    const DrawingProgramAppContext *ctx, const CorePaneLeafRect *leaf,
    KitPaneCompositionEntry *out) {
    if (!ctx || !leaf || !out)
        return (CoreResult){CORE_ERR_INVALID_ARG, "missing pane geometry"};
    KitPaneCompositionSpec spec = drawing_program_visual_pane_spec(ctx, leaf);
    KitPaneComposition view;
    CoreResult result = kit_pane_composition_build(&view, &spec, 1, leaf->rect);
    if (result.code == CORE_OK)
        *out = view.entries[0];
    return result;
}

SDL_Rect drawing_program_visual_pane_pixel_rect(CorePaneRect r) {
    if (!isfinite(r.x) || !isfinite(r.y) || !isfinite(r.width) ||
        !isfinite(r.height) || r.width < 0 || r.height < 0 ||
        (double)r.x < INT_MIN || (double)r.y < INT_MIN ||
        (double)r.x + r.width > INT_MAX || (double)r.y + r.height > INT_MAX ||
        (double)r.width > INT_MAX || (double)r.height > INT_MAX)
        return (SDL_Rect){0,0,0,0};
    int x = (int)ceil((double)r.x), y = (int)ceil((double)r.y);
    int right = (int)floor((double)r.x + r.width), bottom = (int)floor((double)r.y + r.height);
    return (SDL_Rect){x, y, right > x ? right - x : 0,
                     bottom > y ? bottom - y : 0};
}
