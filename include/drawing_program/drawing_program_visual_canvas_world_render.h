#ifndef DRAWING_PROGRAM_VISUAL_CANVAS_WORLD_RENDER_H
#define DRAWING_PROGRAM_VISUAL_CANVAS_WORLD_RENDER_H

#include <SDL2/SDL.h>

#include "core_theme.h"
#include "drawing_program/drawing_program_visual_state.h"

typedef struct DrawingProgramVisualCanvasWorldRenderHooks {
    void (*compute_canvas_sheet_metrics)(const DrawingProgramAppContext *ctx,
                                         SDL_Rect pane_rect,
                                         VisualCanvasSheetMetrics *out_metrics);
    void (*draw_selection_overlay)(SDL_Renderer *renderer,
                                   SDL_Rect pane_rect,
                                   const DrawingProgramAppContext *ctx,
                                   const CoreThemePreset *theme,
                                   const VisualCanvasSheetMetrics *metrics,
                                   const VisualSelectionState *selection);
    void (*draw_object_overlay)(SDL_Renderer *renderer,
                                SDL_Rect pane_rect,
                                const DrawingProgramAppContext *ctx,
                                const CoreThemePreset *theme,
                                const VisualCanvasSheetMetrics *metrics,
                                const VisualCanvasInteractionState *interaction,
                                const VisualPanelUiState *ui);
    void (*draw_shape_preview_overlay)(SDL_Renderer *renderer,
                                       SDL_Rect pane_rect,
                                       const DrawingProgramAppContext *ctx,
                                       const CoreThemePreset *theme,
                                       const VisualCanvasSheetMetrics *metrics,
                                       const VisualCanvasInteractionState *interaction,
                                       const VisualPanelUiState *ui);
} DrawingProgramVisualCanvasWorldRenderHooks;

void drawing_program_visual_draw_canvas_world_view(
    SDL_Renderer *renderer,
    SDL_Rect pane_rect,
    const DrawingProgramAppContext *ctx,
    const CoreThemePreset *theme,
    const VisualSelectionState *selection,
    const VisualPanelUiState *ui,
    const VisualCanvasInteractionState *interaction,
    const DrawingProgramVisualCanvasWorldRenderHooks *hooks);

void drawing_program_visual_canvas_world_backdrop_cache_shutdown(void);

#endif
