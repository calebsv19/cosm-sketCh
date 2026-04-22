#ifndef DRAWING_PROGRAM_VISUAL_OVERLAY_RENDER_H
#define DRAWING_PROGRAM_VISUAL_OVERLAY_RENDER_H

#include <SDL2/SDL.h>

#include "core_theme.h"
#include "drawing_program/drawing_program_visual_state.h"

typedef struct DrawingProgramVisualShapePreviewHooks {
    int (*tool_uses_shape_commit)(DrawingProgramToolKind tool);
    uint8_t (*tool_shape_mode)(const DrawingProgramAppContext *ctx);
    uint32_t (*tool_shape_stroke_width)(const DrawingProgramAppContext *ctx);
    int (*shape_mode_includes_fill)(DrawingProgramToolKind tool, uint8_t mode);
    int (*shape_mode_includes_outline)(DrawingProgramToolKind tool, uint8_t mode);
    int (*screen_to_canvas_sample)(const DrawingProgramAppContext *ctx,
                                   SDL_Rect pane_rect,
                                   int screen_x,
                                   int screen_y,
                                   uint32_t *out_sample_x,
                                   uint32_t *out_sample_y);
} DrawingProgramVisualShapePreviewHooks;

int drawing_program_visual_selection_move_handle_hit(const VisualCanvasSheetMetrics *metrics,
                                                     const VisualSelectionState *selection,
                                                     int x,
                                                     int y);

uint32_t drawing_program_visual_selection_component_count(const VisualSelectionState *selection);

void drawing_program_visual_draw_selection_overlay(SDL_Renderer *renderer,
                                                   SDL_Rect pane_rect,
                                                   const DrawingProgramAppContext *ctx,
                                                   const CoreThemePreset *theme,
                                                   const VisualCanvasSheetMetrics *metrics,
                                                   const VisualSelectionState *selection);

void drawing_program_visual_draw_object_overlay(SDL_Renderer *renderer,
                                                SDL_Rect pane_rect,
                                                const DrawingProgramAppContext *ctx,
                                                const CoreThemePreset *theme,
                                                const VisualCanvasSheetMetrics *metrics,
                                                const VisualCanvasInteractionState *interaction,
                                                const VisualPanelUiState *ui);

void drawing_program_visual_draw_shape_preview_overlay(
    SDL_Renderer *renderer,
    SDL_Rect pane_rect,
    const DrawingProgramAppContext *ctx,
    const CoreThemePreset *theme,
    const VisualCanvasSheetMetrics *metrics,
    const VisualCanvasInteractionState *interaction,
    const VisualPanelUiState *ui,
    const DrawingProgramVisualShapePreviewHooks *hooks);

void drawing_program_visual_object_overlay_cache_shutdown(void);

#endif
