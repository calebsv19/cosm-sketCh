#ifndef DRAWING_PROGRAM_VISUAL_INPUT_HANDLERS_H
#define DRAWING_PROGRAM_VISUAL_INPUT_HANDLERS_H

#include <SDL2/SDL.h>

#include "core_theme.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_visual_state.h"

typedef struct DrawingProgramVisualInputHandlersHooks {
    void (*cancel_all_transient_interactions)(DrawingProgramAppContext *ctx,
                                              VisualCanvasInteractionState *interaction,
                                              DrawingProgramSelectionState *selection,
                                              int clear_pan_state);
    int (*screen_to_canvas_sample)(const DrawingProgramAppContext *ctx,
                                   SDL_Rect pane_rect,
                                   int sx,
                                   int sy,
                                   uint32_t *out_sample_x,
                                   uint32_t *out_sample_y);
    int (*screen_to_canvas_sample_clamped)(const DrawingProgramAppContext *ctx,
                                           SDL_Rect pane_rect,
                                           int sx,
                                           int sy,
                                           uint32_t *out_sample_x,
                                           uint32_t *out_sample_y);
    int (*visual_transform_session_is_move_active)(const VisualCanvasInteractionState *interaction);
    int (*visual_transform_session_is_object_move_active)(const VisualCanvasInteractionState *interaction);
    int (*visual_transform_session_is_object_path_point_move_active)(const VisualCanvasInteractionState *interaction);
    int (*visual_transform_session_is_object_path_handle_move_active)(const VisualCanvasInteractionState *interaction);
    int (*object_path_point_hit_test_selected)(const DrawingProgramAppContext *ctx,
                                               uint32_t sample_x,
                                               uint32_t sample_y,
                                               uint32_t *out_object_id,
                                               uint16_t *out_point_index);
    int (*object_path_handle_hit_test_selected)(const DrawingProgramAppContext *ctx,
                                                uint32_t sample_x,
                                                uint32_t sample_y,
                                                uint32_t *out_object_id,
                                                uint16_t *out_point_index,
                                                uint8_t *out_handle_kind);
    int (*object_path_edge_hit_test_selected)(const DrawingProgramAppContext *ctx,
                                              uint32_t sample_x,
                                              uint32_t sample_y,
                                              uint32_t *out_object_id,
                                              uint16_t *out_insert_index,
                                              int32_t *out_point_x,
                                              int32_t *out_point_y);
    void (*visual_transform_session_update_move)(const DrawingProgramAppContext *ctx,
                                                 VisualCanvasInteractionState *interaction,
                                                 DrawingProgramSelectionState *selection,
                                                 uint32_t sample_x,
                                                 uint32_t sample_y,
                                                 SDL_Keymod mods);
    void (*visual_transform_session_update_object_move)(const DrawingProgramAppContext *ctx,
                                                        VisualCanvasInteractionState *interaction,
                                                        uint32_t sample_x,
                                                        uint32_t sample_y,
                                                        SDL_Keymod mods);
    void (*visual_transform_session_update_object_path_point_move)(const DrawingProgramAppContext *ctx,
                                                                   VisualCanvasInteractionState *interaction,
                                                                   uint32_t sample_x,
                                                                   uint32_t sample_y,
                                                                   SDL_Keymod mods);
    void (*visual_transform_session_update_object_path_handle_move)(const DrawingProgramAppContext *ctx,
                                                                    VisualCanvasInteractionState *interaction,
                                                                    uint32_t sample_x,
                                                                    uint32_t sample_y);
    CoreResult (*visual_transform_session_commit_move)(DrawingProgramAppContext *ctx,
                                                       VisualCanvasInteractionState *interaction,
                                                       DrawingProgramSelectionState *selection);
    CoreResult (*visual_transform_session_commit_object_move)(DrawingProgramAppContext *ctx,
                                                              VisualCanvasInteractionState *interaction);
    CoreResult (*visual_transform_session_commit_object_path_point_move)(DrawingProgramAppContext *ctx,
                                                                         VisualCanvasInteractionState *interaction);
    CoreResult (*visual_transform_session_commit_object_path_handle_move)(DrawingProgramAppContext *ctx,
                                                                          VisualCanvasInteractionState *interaction);
    VisualMarqueeCommitMode (*visual_marquee_commit_mode_clamp)(uint8_t raw);
    int (*visual_selection_capture_from_marquee)(DrawingProgramAppContext *ctx,
                                                 DrawingProgramSelectionState *selection,
                                                 VisualMarqueeCommitMode mode);
    int (*tool_uses_shape_commit)(DrawingProgramToolKind tool);
    CoreResult (*apply_canvas_shape_commit)(DrawingProgramAppContext *ctx,
                                            DrawingProgramToolKind tool,
                                            uint32_t start_sample_x,
                                            uint32_t start_sample_y,
                                            uint32_t end_sample_x,
                                            uint32_t end_sample_y);
    CoreResult (*apply_canvas_draw_at_screen)(DrawingProgramAppContext *ctx,
                                              SDL_Rect pane_rect,
                                              int sx,
                                              int sy,
                                              VisualCanvasInteractionState *interaction);
    VisualMarqueeCommitMode (*visual_marquee_commit_mode_from_mods)(SDL_Keymod mods);
    int (*visual_selection_begin_move)(const DrawingProgramSelectionState *selection,
                                       uint32_t sample_x,
                                       uint32_t sample_y);
    int (*selection_move_handle_hit)(const DrawingProgramAppContext *ctx,
                                     SDL_Rect pane_rect,
                                     const DrawingProgramSelectionState *selection,
                                     int sx,
                                     int sy);
    void (*visual_transform_session_begin_move)(VisualCanvasInteractionState *interaction,
                                                DrawingProgramSelectionState *selection,
                                                uint32_t sample_x,
                                                uint32_t sample_y);
    void (*visual_transform_session_begin_object_move)(VisualCanvasInteractionState *interaction,
                                                       uint32_t sample_x,
                                                       uint32_t sample_y);
    void (*visual_transform_session_begin_object_path_point_move)(VisualCanvasInteractionState *interaction,
                                                                  uint32_t object_id,
                                                                  uint16_t point_index,
                                                                  uint32_t sample_x,
                                                                  uint32_t sample_y);
    void (*visual_transform_session_begin_object_path_handle_move)(VisualCanvasInteractionState *interaction,
                                                                   uint32_t object_id,
                                                                   uint16_t point_index,
                                                                   uint8_t handle_kind,
                                                                   const DrawingProgramPathPoint *point);
    CoreResult (*apply_canvas_picker_at_screen)(DrawingProgramAppContext *ctx,
                                                SDL_Rect pane_rect,
                                                int sx,
                                                int sy);
    CoreResult (*apply_canvas_fill_at_screen)(DrawingProgramAppContext *ctx,
                                              SDL_Rect pane_rect,
                                              int sx,
                                              int sy);
    void (*begin_canvas_history_group)(DrawingProgramAppContext *ctx);
    int (*delete_active_selection_payload_or_objects)(DrawingProgramAppContext *ctx,
                                                      DrawingProgramSelectionState *selection_state,
                                                      const struct DrawingProgramVisualInputHandlersHooks *hooks);
    CoreResult (*path_draft_commit)(DrawingProgramAppContext *ctx,
                                    VisualCanvasInteractionState *interaction,
                                    uint8_t closed);
    CoreResult (*apply_insert_object_path_point)(DrawingProgramAppContext *ctx,
                                                 uint32_t object_id,
                                                 uint16_t insert_index,
                                                 int32_t point_x,
                                                 int32_t point_y);
    void (*path_draft_reset)(VisualCanvasInteractionState *interaction);
    int (*path_draft_pop_point)(VisualCanvasInteractionState *interaction);
    void (*cancel_selection_transient)(DrawingProgramSelectionState *selection);
    void (*cancel_canvas_draw_and_shape)(VisualCanvasInteractionState *interaction);
    int (*tool_uses_direct_sample_stroke)(DrawingProgramToolKind tool);
    int (*set_module_type_for_pane)(DrawingProgramAppContext *ctx,
                                    uint32_t pane_node_id,
                                    uint32_t module_type_id);
    uint32_t (*visual_tool_count)(void);
    DrawingProgramToolKind (*visual_tool_at)(uint32_t index);
    DrawingProgramWorkflowControl (*workflow_control_for_tool)(DrawingProgramToolKind tool);
    uint32_t (*visual_tool_option_count)(const DrawingProgramAppContext *ctx, DrawingProgramToolKind tool);
    uint32_t (*visual_tool_option_kind_for_index_raw)(const DrawingProgramAppContext *ctx,
                                                      DrawingProgramToolKind tool,
                                                      uint32_t index);
    int (*visual_tool_option_is_action_button_raw)(uint32_t option_kind_raw);
    int (*visual_tool_option_is_select_delete_raw)(uint32_t option_kind_raw);
    void (*visual_tool_option_adjust_raw)(DrawingProgramAppContext *ctx, uint32_t option_kind_raw, int delta);
    void (*sync_panel_ui_from_app)(const DrawingProgramAppContext *ctx, VisualPanelUiState *ui);
    uint8_t (*clamp_left_slot)(uint8_t slot);
    uint8_t (*clamp_right_slot)(uint8_t slot);
    void (*apply_layer_rename_auto)(DrawingProgramAppContext *ctx);
    void (*apply_layer_duplicate_active)(DrawingProgramAppContext *ctx);
    int (*active_layer_query)(const DrawingProgramAppContext *ctx,
                              uint32_t *out_layer_id,
                              uint32_t *out_index,
                              uint8_t *out_visible,
                              uint8_t *out_locked);
    int (*clamp_font_zoom_step)(int step);
    void (*apply_workflow_control_if_valid)(DrawingProgramAppContext *ctx, DrawingProgramWorkflowControl control);
    int (*active_layer_allows_edits_visual)(const DrawingProgramAppContext *ctx);
    int (*point_in_rect)(SDL_Rect rect, int x, int y);
    CoreThemePresetId (*clamp_theme_preset_id)(uint32_t raw);
    int (*cycle_theme_preset)(CoreThemePresetId current, int delta, CoreThemePresetId *out_next);
    CoreResult (*visual_transform_session_nudge_move)(DrawingProgramAppContext *ctx,
                                                      VisualCanvasInteractionState *interaction,
                                                      DrawingProgramSelectionState *selection,
                                                      int32_t dx,
                                                      int32_t dy);
    CoreResult (*visual_transform_session_nudge_object_move)(DrawingProgramAppContext *ctx,
                                                             VisualCanvasInteractionState *interaction,
                                                             int32_t dx,
                                                             int32_t dy);
} DrawingProgramVisualInputHandlersHooks;

int drawing_program_visual_input_handle_mouse_button_up_payload(const SDL_Event *event,
                                                                int event_has_position,
                                                                int event_x,
                                                                int event_y,
                                                                int has_canvas_pane,
                                                                SDL_Rect canvas_pane,
                                                                DrawingProgramAppContext *app,
                                                                VisualCanvasInteractionState *canvas_interaction,
                                                                DrawingProgramSelectionState *selection_state,
                                                                const DrawingProgramVisualInputHandlersHooks *hooks);

int drawing_program_visual_input_handle_mouse_motion_payload(const SDL_Event *event,
                                                             int event_has_position,
                                                             int event_x,
                                                             int event_y,
                                                             int has_canvas_pane,
                                                             SDL_Rect canvas_pane,
                                                             DrawingProgramAppContext *app,
                                                             VisualCanvasInteractionState *canvas_interaction,
                                                             DrawingProgramSelectionState *selection_state,
                                                             VisualPanelUiState *panel_ui,
                                                             const DrawingProgramVisualInputHandlersHooks *hooks);

int drawing_program_visual_input_handle_mouse_button_down_payload(const SDL_Event *event,
                                                                  int event_has_position,
                                                                  int event_x,
                                                                  int event_y,
                                                                  int has_left_pane,
                                                                  SDL_Rect left_pane,
                                                                  int has_right_pane,
                                                                  SDL_Rect right_pane,
                                                                  int has_canvas_pane,
                                                                  SDL_Rect canvas_pane,
                                                                  DrawingProgramAppContext *app,
                                                                  VisualCanvasInteractionState *canvas_interaction,
                                                                  DrawingProgramSelectionState *selection_state,
                                                                  VisualPanelUiState *panel_ui,
                                                                  const DrawingProgramVisualInputHandlersHooks *hooks);

int drawing_program_visual_input_handle_keydown_payload(const SDL_Event *event,
                                                        int has_canvas_pane,
                                                        SDL_Rect canvas_pane,
                                                        CoreThemePresetId *io_selected_theme,
                                                        CoreThemePreset *io_theme_preset,
                                                        DrawingProgramAppContext *app,
                                                        VisualCanvasInteractionState *canvas_interaction,
                                                        DrawingProgramSelectionState *selection_state,
                                                        VisualPanelUiState *panel_ui,
                                                        const DrawingProgramVisualInputHandlersHooks *hooks);

#endif
