#ifndef DRAWING_PROGRAM_VISUAL_PANE_HEADER_H
#define DRAWING_PROGRAM_VISUAL_PANE_HEADER_H

#include "drawing_program/drawing_program_visual_state.h"
#include "kit_ui_surface.h"

/* Fixed product policy, not a dynamic provider registry. Only actions which fit
 * the shared priority layout are painted, registered and dispatched. */
CoreResult drawing_program_visual_pane_header_layout(
    const DrawingProgramAppContext *ctx, const KitPaneCompositionEntry *pane,
    KitPaneHeaderLayout *out);
int drawing_program_visual_pane_header_draw(
    SDL_Renderer *renderer, const DrawingProgramAppContext *ctx,
    const KitPaneCompositionEntry *pane, const CoreThemePreset *theme);
int drawing_program_visual_pane_header_action(
    DrawingProgramAppContext *ctx, KitUiSurfaceKey key);

#endif
