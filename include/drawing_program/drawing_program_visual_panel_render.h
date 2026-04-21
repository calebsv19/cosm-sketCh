#ifndef DRAWING_PROGRAM_VISUAL_PANEL_RENDER_H
#define DRAWING_PROGRAM_VISUAL_PANEL_RENDER_H

#include <stddef.h>
#include <SDL2/SDL.h>

#include "core_theme.h"
#include "drawing_program/drawing_program_visual_state.h"

typedef struct DrawingProgramVisualPanelRenderHooks {
    int (*draw_bitmap_text)(SDL_Renderer *renderer,
                            SDL_Rect clip_rect,
                            int x,
                            int y,
                            const char *text,
                            SDL_Color color,
                            int scale);
    int (*measure_bitmap_text_width)(const char *text, int scale);
    const char *(*tool_name)(DrawingProgramToolKind tool);
    uint8_t (*clamp_left_slot)(uint8_t slot);
    uint8_t (*clamp_right_slot)(uint8_t slot);
    uint32_t (*visual_tool_count)(void);
    DrawingProgramToolKind (*visual_tool_at)(uint32_t index);
    uint32_t (*visual_tool_option_count)(const DrawingProgramAppContext *ctx, DrawingProgramToolKind tool);
    uint32_t (*visual_tool_option_kind_for_index_raw)(const DrawingProgramAppContext *ctx,
                                                      DrawingProgramToolKind tool,
                                                      uint32_t index);
    int (*visual_tool_option_is_action_button_raw)(uint32_t option_kind_raw);
    const char *(*visual_tool_option_label_raw)(uint32_t option_kind_raw);
    void (*visual_tool_option_value_text_raw)(const DrawingProgramAppContext *ctx,
                                              uint32_t option_kind_raw,
                                              char *out_text,
                                              size_t out_cap);
    uint32_t (*tool_brush_radius_samples)(const DrawingProgramAppContext *ctx, DrawingProgramToolKind tool);
    uint32_t (*tool_brush_spacing_samples)(const DrawingProgramAppContext *ctx,
                                           DrawingProgramToolKind tool,
                                           uint32_t radius);
    int (*active_layer_allows_edits_visual)(const DrawingProgramAppContext *ctx);
    int (*tool_uses_shape_commit)(DrawingProgramToolKind tool);
    uint8_t (*clamp_setting_u8)(uint8_t value, uint8_t min_v, uint8_t max_v);
    const char *(*shape_mode_name)(uint8_t mode);
    uint8_t (*tool_shape_mode)(const DrawingProgramAppContext *ctx);
    uint8_t (*tool_fill_tolerance_setting)(const DrawingProgramAppContext *ctx);
    uint8_t (*fill_tolerance_sample_delta)(uint8_t tolerance_setting);
    uint32_t (*selection_component_count)(const VisualSelectionState *selection);
    int (*pane_rect_for_module_type)(const DrawingProgramAppContext *ctx,
                                     uint32_t module_type_id,
                                     SDL_Rect *out_rect);
    uint8_t (*color_index_clamp)(uint8_t index);
    void (*color_rgb_from_index)(uint8_t index, uint8_t *out_r, uint8_t *out_g, uint8_t *out_b);
    int (*point_in_rect)(SDL_Rect rect, int x, int y);
} DrawingProgramVisualPanelRenderHooks;

void drawing_program_visual_render_menu_bar_chrome(SDL_Renderer *renderer,
                                                   SDL_Rect rect,
                                                   const DrawingProgramAppContext *ctx,
                                                   const CoreThemePreset *theme,
                                                   const DrawingProgramVisualPanelRenderHooks *hooks);

void drawing_program_visual_render_left_panel_chrome(SDL_Renderer *renderer,
                                                     SDL_Rect rect,
                                                     const DrawingProgramAppContext *ctx,
                                                     const CoreThemePreset *theme,
                                                     const VisualPanelUiState *ui,
                                                     const DrawingProgramVisualPanelRenderHooks *hooks);

void drawing_program_visual_render_right_panel_chrome(SDL_Renderer *renderer,
                                                      SDL_Rect rect,
                                                      const DrawingProgramAppContext *ctx,
                                                      const CoreThemePreset *theme,
                                                      const VisualPanelUiState *ui,
                                                      const VisualSelectionState *selection,
                                                      const VisualCanvasInteractionState *interaction,
                                                      const DrawingProgramVisualPanelRenderHooks *hooks);

#endif
