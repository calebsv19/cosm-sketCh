#ifndef DRAWING_PROGRAM_VISUAL_PANE_GEOMETRY_H
#define DRAWING_PROGRAM_VISUAL_PANE_GEOMETRY_H

#include "drawing_program/drawing_program_app_main.h"
#include <SDL2/SDL.h>

/* Product module policy supplies header height; kit_pane owns the partition.
 * Menu content keeps its existing top-level chrome and has no extra header. */
KitPaneCompositionSpec drawing_program_visual_pane_spec(
    const DrawingProgramAppContext *ctx, const CorePaneLeafRect *leaf);
CoreResult drawing_program_visual_pane_entry(
    const DrawingProgramAppContext *ctx, const CorePaneLeafRect *leaf,
    KitPaneCompositionEntry *out);
/* Same inward-rounded pixel bounds as the shared SDL clip adapter. Empty
 * content stays empty instead of being promoted to an interactive pixel. */
SDL_Rect drawing_program_visual_pane_pixel_rect(CorePaneRect bounds);

#endif
