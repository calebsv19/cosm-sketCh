#include <SDL2/SDL.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "core_font.h"
#include "core_theme.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_object_transform.h"
#include "drawing_program/drawing_program_render_backend.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_visual_canvas_action_ops.h"
#include "drawing_program/drawing_program_visual_canvas_coords.h"
#include "drawing_program/drawing_program_visual_canvas_draw_action_ops.h"
#include "drawing_program/drawing_program_visual_canvas_stroke_ops.h"
#include "drawing_program/drawing_program_visual_canvas_world_render.h"
#include "drawing_program/drawing_program_visual_frame_render.h"
#include "drawing_program/drawing_program_visual_input_core.h"
#include "drawing_program/drawing_program_visual_transform_ops.h"
#include "drawing_program/drawing_program_visual_input_handlers.h"
#include "drawing_program/drawing_program_visual_input_keymap.h"
#include "drawing_program/drawing_program_visual_input_selection_ops.h"
#include "drawing_program/drawing_program_visual_input_support.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_layer_actions.h"
#include "drawing_program/drawing_program_visual_overlay_render.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_visual_panel_render.h"
#include "drawing_program/drawing_program_visual_resources.h"
#include "drawing_program/drawing_program_visual_runtime_debug.h"
#include "drawing_program/drawing_program_visual_shape_ops.h"
#include "drawing_program/drawing_program_visual_loop_timing.h"
#include "drawing_program/drawing_program_visual_state.h"
#include "drawing_program/drawing_program_visual_text_render.h"
#include "drawing_program/drawing_program_visual_theme.h"
#include "drawing_program/drawing_program_visual_tool_options.h"

static void cancel_all_transient_interactions(DrawingProgramAppContext *ctx,
                                              VisualCanvasInteractionState *interaction,
                                              VisualSelectionState *selection,
                                              int clear_pan_state);

static int active_layer_query(const DrawingProgramAppContext *ctx,
                              uint32_t *out_layer_id,
                              uint32_t *out_index,
                              uint8_t *out_visible,
                              uint8_t *out_locked) {
    CoreResult result;
    result = drawing_program_runtime_orchestration_resolve_active_layer(ctx,
                                                                        out_layer_id,
                                                                        out_index,
                                                                        out_visible,
                                                                        out_locked);
    if (result.code != CORE_OK) {
        return 0;
    }
    return 1;
}

static int active_layer_allows_edits_visual(const DrawingProgramAppContext *ctx) {
    uint8_t visible = 0u;
    uint8_t locked = 0u;
    if (!active_layer_query(ctx, 0, 0, &visible, &locked)) {
        return 0;
    }
    return (visible && !locked) ? 1 : 0;
}

static CoreResult active_layer_sample_read_visual(const DrawingProgramAppContext *ctx,
                                                  uint32_t sample_x,
                                                  uint32_t sample_y,
                                                  uint8_t *out_value) {
    CoreResult result;
    if (!ctx || !out_value) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid active-layer sample read request" };
    }
    result = drawing_program_layer_raster_store_sample_read(&ctx->layer_rasters,
                                                            ctx->editor.active_layer_id,
                                                            sample_x,
                                                            sample_y,
                                                            out_value);
    if (result.code == CORE_OK) {
        return core_result_ok();
    }
    return drawing_program_document_sample_read(&ctx->document, sample_x, sample_y, out_value);
}

static VisualMarqueeCommitMode visual_marquee_commit_mode_from_mods(SDL_Keymod mods) {
    if ((mods & KMOD_ALT) != 0) {
        return VISUAL_MARQUEE_COMMIT_SUBTRACT;
    }
    if ((mods & KMOD_SHIFT) != 0) {
        return VISUAL_MARQUEE_COMMIT_ADD;
    }
    return VISUAL_MARQUEE_COMMIT_REPLACE;
}

static VisualMarqueeCommitMode visual_marquee_commit_mode_clamp(uint8_t raw) {
    if (raw > (uint8_t)VISUAL_MARQUEE_COMMIT_SUBTRACT) {
        return VISUAL_MARQUEE_COMMIT_REPLACE;
    }
    return (VisualMarqueeCommitMode)raw;
}

static int visual_selection_capture_from_marquee(DrawingProgramAppContext *ctx,
                                                 VisualSelectionState *selection,
                                                 VisualMarqueeCommitMode mode) {
    if (!ctx) {
        return 0;
    }
    if (mode == VISUAL_MARQUEE_COMMIT_ADD) {
        return drawing_program_selection_add_from_marquee(
            &ctx->document, &ctx->layer_rasters, ctx->editor.active_layer_id, selection);
    }
    if (mode == VISUAL_MARQUEE_COMMIT_SUBTRACT) {
        return drawing_program_selection_subtract_from_marquee(
            &ctx->document, &ctx->layer_rasters, ctx->editor.active_layer_id, selection);
    }
    return drawing_program_selection_capture_from_marquee(&ctx->document,
                                                          &ctx->layer_rasters,
                                                          ctx->editor.active_layer_id,
                                                          selection);
}

static int visual_selection_begin_move(const VisualSelectionState *selection, uint32_t sample_x, uint32_t sample_y) {
    return drawing_program_selection_begin_move(selection, sample_x, sample_y);
}

static CoreResult visual_selection_commit_move(DrawingProgramAppContext *ctx, VisualSelectionState *selection) {
    if (!ctx) {
        return core_result_ok();
    }
    return drawing_program_selection_commit_move(&ctx->document,
                                                 &ctx->layer_rasters,
                                                 ctx->editor.active_layer_id,
                                                 &ctx->history,
                                                 selection);
}

static CoreResult visual_object_commit_move(DrawingProgramAppContext *ctx,
                                            int32_t requested_dx,
                                            int32_t requested_dy) {
    return drawing_program_object_transform_commit_move(&ctx->history,
                                                        &ctx->object_store,
                                                        &ctx->document,
                                                        &ctx->object_selection,
                                                        requested_dx,
                                                        requested_dy,
                                                        0,
                                                        0);
}

static CoreResult visual_object_commit_path_point(DrawingProgramAppContext *ctx,
                                                  uint32_t object_id,
                                                  uint16_t point_index,
                                                  int32_t point_x,
                                                  int32_t point_y) {
    return drawing_program_history_apply_set_object_path_point(
        &ctx->history, &ctx->object_store, object_id, point_index, point_x, point_y);
}

static CoreResult visual_object_commit_path_point_data(DrawingProgramAppContext *ctx,
                                                       uint32_t object_id,
                                                       uint16_t point_index,
                                                       const DrawingProgramPathPoint *point) {
    return drawing_program_history_apply_set_object_path_point_data(
        &ctx->history, &ctx->object_store, object_id, point_index, point);
}

static CoreResult visual_object_insert_path_point(DrawingProgramAppContext *ctx,
                                                  uint32_t object_id,
                                                  uint16_t insert_index,
                                                  int32_t point_x,
                                                  int32_t point_y) {
    return drawing_program_history_apply_insert_object_path_point(
        &ctx->history, &ctx->object_store, object_id, insert_index, point_x, point_y);
}

static int object_path_point_hit_test_selected(const DrawingProgramAppContext *ctx,
                                               uint32_t sample_x,
                                               uint32_t sample_y,
                                               uint32_t *out_object_id,
                                               uint16_t *out_point_index) {
    CoreResult result;
    float zoom = 1.0f;
    uint32_t pick_radius = 6u;
    if (!ctx || !out_object_id || !out_point_index) {
        return 0;
    }
    zoom = ctx->editor.viewport.zoom;
    if (zoom < 0.2f) {
        zoom = 0.2f;
    }
    pick_radius = (uint32_t)(12.0f / zoom);
    if (pick_radius < 4u) {
        pick_radius = 4u;
    }
    if (pick_radius > 18u) {
        pick_radius = 18u;
    }
    result = drawing_program_object_store_hit_test_selected_path_point(&ctx->object_store,
                                                                       &ctx->document,
                                                                       &ctx->object_selection,
                                                                       sample_x,
                                                                       sample_y,
                                                                       pick_radius,
                                                                       out_object_id,
                                                                       out_point_index);
    return (result.code == CORE_OK) ? 1 : 0;
}

static int object_path_handle_hit_test_selected(const DrawingProgramAppContext *ctx,
                                                uint32_t sample_x,
                                                uint32_t sample_y,
                                                uint32_t *out_object_id,
                                                uint16_t *out_point_index,
                                                uint8_t *out_handle_kind) {
    CoreResult result;
    float zoom = 1.0f;
    uint32_t pick_radius = 6u;
    if (!ctx || !out_object_id || !out_point_index || !out_handle_kind) {
        return 0;
    }
    zoom = ctx->editor.viewport.zoom;
    if (zoom < 0.2f) {
        zoom = 0.2f;
    }
    pick_radius = (uint32_t)(14.0f / zoom);
    if (pick_radius < 6u) {
        pick_radius = 6u;
    }
    if (pick_radius > 24u) {
        pick_radius = 24u;
    }
    result = drawing_program_object_store_hit_test_selected_path_handle(&ctx->object_store,
                                                                        &ctx->document,
                                                                        &ctx->object_selection,
                                                                        sample_x,
                                                                        sample_y,
                                                                        pick_radius,
                                                                        out_object_id,
                                                                        out_point_index,
                                                                        out_handle_kind);
    return (result.code == CORE_OK) ? 1 : 0;
}

static int object_path_edge_hit_test_selected(const DrawingProgramAppContext *ctx,
                                              uint32_t sample_x,
                                              uint32_t sample_y,
                                              uint32_t *out_object_id,
                                              uint16_t *out_insert_index,
                                              int32_t *out_point_x,
                                              int32_t *out_point_y) {
    CoreResult result;
    float zoom = 1.0f;
    uint32_t pick_radius = 6u;
    if (!ctx || !out_object_id || !out_insert_index || !out_point_x || !out_point_y) {
        return 0;
    }
    zoom = ctx->editor.viewport.zoom;
    if (zoom < 0.2f) {
        zoom = 0.2f;
    }
    pick_radius = (uint32_t)(14.0f / zoom);
    if (pick_radius < 6u) {
        pick_radius = 6u;
    }
    if (pick_radius > 24u) {
        pick_radius = 24u;
    }
    result = drawing_program_object_store_hit_test_selected_path_edge(&ctx->object_store,
                                                                      &ctx->document,
                                                                      &ctx->object_selection,
                                                                      sample_x,
                                                                      sample_y,
                                                                      pick_radius,
                                                                      out_object_id,
                                                                      out_insert_index,
                                                                      out_point_x,
                                                                      out_point_y);
    return (result.code == CORE_OK) ? 1 : 0;
}

static const DrawingProgramVisualCanvasActionOpsHooks *visual_canvas_action_ops_hooks(void) {
    static const DrawingProgramVisualCanvasActionOpsHooks hooks = {
        .screen_to_canvas_sample = drawing_program_visual_screen_to_canvas_sample,
        .active_layer_allows_edits_visual = active_layer_allows_edits_visual,
        .active_layer_query = active_layer_query,
        .sample_value_for_tool = drawing_program_visual_sample_value_for_tool,
        .tool_fill_tolerance_setting = drawing_program_visual_tool_fill_tolerance_setting,
        .fill_sample_matches_tolerance = drawing_program_visual_fill_sample_matches_tolerance,
        .active_layer_sample_read_visual = active_layer_sample_read_visual,
        .color_index_for_sample = drawing_program_visual_color_index_for_sample
    };
    return &hooks;
}

static CoreResult apply_canvas_picker_at_screen(DrawingProgramAppContext *ctx,
                                                SDL_Rect pane_rect,
                                                int sx,
                                                int sy) {
    return drawing_program_visual_apply_canvas_picker_at_screen(ctx, pane_rect, sx, sy, visual_canvas_action_ops_hooks());
}

static CoreResult apply_canvas_fill_at_screen(DrawingProgramAppContext *ctx,
                                              SDL_Rect pane_rect,
                                              int sx,
                                              int sy) {
    return drawing_program_visual_apply_canvas_fill_at_screen(ctx, pane_rect, sx, sy, visual_canvas_action_ops_hooks());
}

static CoreResult apply_canvas_shape_commit(DrawingProgramAppContext *ctx,
                                            DrawingProgramToolKind tool,
                                            uint32_t start_x,
                                            uint32_t start_y,
                                            uint32_t end_x,
                                            uint32_t end_y) {
    return drawing_program_visual_apply_canvas_shape_commit(ctx, tool, start_x, start_y, end_x, end_y);
}

CoreResult drawing_program_app_shape_commit_samples(DrawingProgramAppContext *ctx,
                                                    DrawingProgramToolKind tool,
                                                    uint32_t start_x,
                                                    uint32_t start_y,
                                                    uint32_t end_x,
                                                    uint32_t end_y) {
    return apply_canvas_shape_commit(ctx, tool, start_x, start_y, end_x, end_y);
}

static int selection_move_handle_hit(const DrawingProgramAppContext *ctx,
                                     SDL_Rect pane_rect,
                                     const VisualSelectionState *selection,
                                     int x,
                                     int y) {
    VisualCanvasSheetMetrics metrics;
    if (!ctx || !selection) {
        return 0;
    }
    drawing_program_visual_compute_canvas_sheet_metrics(ctx, pane_rect, &metrics);
    return drawing_program_visual_selection_move_handle_hit(&metrics, selection, x, y);
}

static uint32_t selection_component_count(const VisualSelectionState *selection) {
    return drawing_program_visual_selection_component_count(selection);
}

static const DrawingProgramVisualShapePreviewHooks *visual_shape_preview_hooks(void) {
    static const DrawingProgramVisualShapePreviewHooks hooks = {
        .tool_uses_shape_commit = drawing_program_visual_tool_uses_shape_commit,
        .tool_shape_mode = drawing_program_visual_tool_shape_mode,
        .tool_shape_stroke_width = drawing_program_visual_tool_shape_stroke_width,
        .shape_mode_includes_fill = drawing_program_visual_shape_mode_includes_fill,
        .shape_mode_includes_outline = drawing_program_visual_shape_mode_includes_outline,
        .screen_to_canvas_sample = drawing_program_visual_screen_to_canvas_sample
    };
    return &hooks;
}

static void draw_selection_overlay(SDL_Renderer *renderer,
                                   SDL_Rect pane_rect,
                                   const DrawingProgramAppContext *ctx,
                                   const CoreThemePreset *theme,
                                   const VisualCanvasSheetMetrics *metrics,
                                   const VisualSelectionState *selection) {
    drawing_program_visual_draw_selection_overlay(renderer, pane_rect, ctx, theme, metrics, selection);
}

static void draw_object_overlay(SDL_Renderer *renderer,
                                SDL_Rect pane_rect,
                                const DrawingProgramAppContext *ctx,
                                const CoreThemePreset *theme,
                                const VisualCanvasSheetMetrics *metrics,
                                const VisualCanvasInteractionState *interaction,
                                const VisualPanelUiState *ui) {
    drawing_program_visual_draw_object_overlay(renderer, pane_rect, ctx, theme, metrics, interaction, ui);
}

static void draw_shape_preview_overlay(SDL_Renderer *renderer,
                                       SDL_Rect pane_rect,
                                       const DrawingProgramAppContext *ctx,
                                       const CoreThemePreset *theme,
                                       const VisualCanvasSheetMetrics *metrics,
                                       const VisualCanvasInteractionState *interaction,
                                       const VisualPanelUiState *ui) {
    drawing_program_visual_draw_shape_preview_overlay(
        renderer, pane_rect, ctx, theme, metrics, interaction, ui, visual_shape_preview_hooks());
}

static const DrawingProgramVisualCanvasDrawActionOpsHooks *visual_canvas_draw_action_ops_hooks(void) {
    static const DrawingProgramVisualCanvasDrawActionOpsHooks hooks = {
        .screen_to_canvas_sample = drawing_program_visual_screen_to_canvas_sample,
        .active_layer_allows_edits_visual = active_layer_allows_edits_visual,
        .active_layer_query = active_layer_query,
        .sample_value_for_tool = drawing_program_visual_sample_value_for_tool,
        .tool_brush_radius_samples = drawing_program_visual_tool_brush_radius_samples,
        .tool_brush_spacing_samples = drawing_program_visual_tool_brush_spacing_samples,
        .tool_brush_hardness_percent = drawing_program_visual_tool_brush_hardness_percent,
        .seeded_background_sample_for_coord = drawing_program_visual_seeded_background_sample_for_coord,
        .apply_canvas_stamp_square_on_layer = drawing_program_visual_apply_canvas_stamp_square_on_layer
    };
    return &hooks;
}

static CoreResult apply_canvas_draw_at_screen(DrawingProgramAppContext *ctx,
                                              SDL_Rect pane_rect,
                                              int sx,
                                              int sy,
                                              VisualCanvasInteractionState *state) {
    return drawing_program_visual_apply_canvas_draw_at_screen(
        ctx, pane_rect, sx, sy, state, visual_canvas_draw_action_ops_hooks());
}

static const char *tool_name(DrawingProgramToolKind tool) {
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_BRUSH: return "BRUSH";
        case DRAWING_PROGRAM_TOOL_ERASER: return "ERASER";
        case DRAWING_PROGRAM_TOOL_FILL: return "FILL";
        case DRAWING_PROGRAM_TOOL_LINE: return "LINE";
        case DRAWING_PROGRAM_TOOL_RECT: return "RECT";
        case DRAWING_PROGRAM_TOOL_CIRCLE: return "CIRCLE";
        case DRAWING_PROGRAM_TOOL_SELECT: return "SELECT";
        case DRAWING_PROGRAM_TOOL_MOVE: return "MOVE";
        case DRAWING_PROGRAM_TOOL_PICKER: return "PICKER";
        case DRAWING_PROGRAM_TOOL_PATH: return "PATH";
        default: return "UNKNOWN";
    }
}

static const DrawingProgramVisualPanelRenderHooks *visual_panel_render_hooks(void) {
    static const DrawingProgramVisualPanelRenderHooks hooks = {
        .draw_bitmap_text = drawing_program_visual_draw_bitmap_text,
        .measure_bitmap_text_width = drawing_program_visual_measure_bitmap_text_width,
        .tool_name = tool_name,
        .clamp_left_slot = drawing_program_visual_clamp_left_slot,
        .clamp_right_slot = drawing_program_visual_clamp_right_slot,
        .visual_tool_count = drawing_program_visual_tool_count,
        .visual_tool_at = drawing_program_visual_tool_at,
        .visual_tool_option_count = drawing_program_visual_tool_option_count,
        .visual_tool_option_kind_for_index_raw = drawing_program_visual_tool_option_kind_for_index_raw,
        .visual_tool_option_is_action_button_raw = drawing_program_visual_tool_option_is_action_button_raw,
        .visual_tool_option_label_raw = drawing_program_visual_tool_option_label_raw,
        .visual_tool_option_value_text_raw = drawing_program_visual_tool_option_value_text_raw,
        .tool_brush_radius_samples = drawing_program_visual_tool_brush_radius_samples,
        .tool_brush_spacing_samples = drawing_program_visual_tool_brush_spacing_samples,
        .active_layer_allows_edits_visual = active_layer_allows_edits_visual,
        .tool_uses_shape_commit = drawing_program_visual_tool_uses_shape_commit,
        .clamp_setting_u8 = drawing_program_visual_clamp_setting_u8,
        .shape_mode_name = drawing_program_visual_shape_mode_name,
        .tool_shape_mode = drawing_program_visual_tool_shape_mode,
        .tool_fill_tolerance_setting = drawing_program_visual_tool_fill_tolerance_setting,
        .fill_tolerance_sample_delta = drawing_program_visual_fill_tolerance_sample_delta,
        .selection_component_count = selection_component_count,
        .pane_rect_for_module_type = drawing_program_visual_pane_rect_for_module_type,
        .color_index_clamp = drawing_program_color_index_clamp,
        .color_rgb_from_index = drawing_program_color_rgb_from_index,
        .point_in_rect = drawing_program_visual_point_in_rect
    };
    return &hooks;
}

static void draw_menu_bar_chrome(SDL_Renderer *renderer,
                                 SDL_Rect rect,
                                 const DrawingProgramAppContext *ctx,
                                 const CoreThemePreset *theme) {
    drawing_program_visual_render_menu_bar_chrome(renderer, rect, ctx, theme, visual_panel_render_hooks());
}

static void draw_left_panel_chrome(SDL_Renderer *renderer,
                                   SDL_Rect rect,
                                   const DrawingProgramAppContext *ctx,
                                   const CoreThemePreset *theme,
                                   const VisualPanelUiState *ui) {
    drawing_program_visual_render_left_panel_chrome(renderer, rect, ctx, theme, ui, visual_panel_render_hooks());
}

static void draw_right_panel_chrome(SDL_Renderer *renderer,
                                    SDL_Rect rect,
                                    const DrawingProgramAppContext *ctx,
                                    const CoreThemePreset *theme,
                                    const VisualPanelUiState *ui,
                                    const VisualSelectionState *selection,
                                    const VisualCanvasInteractionState *interaction) {
    drawing_program_visual_render_right_panel_chrome(
        renderer, rect, ctx, theme, ui, selection, interaction, visual_panel_render_hooks());
}

static void draw_canvas_viewport_chrome(SDL_Renderer *renderer,
                                        SDL_Rect rect,
                                        const DrawingProgramAppContext *ctx,
                                        const CoreThemePreset *theme) {
    VisualPaneLayoutMetrics m;
    char line[96];
    VisualThemePalette p;
    int y;
    if (!renderer || !ctx) {
        return;
    }
    m = make_pane_layout_metrics(ctx);
    resolve_visual_theme_palette(theme, &p);
    y = rect.y + m.pad_y;
    drawing_program_visual_draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "VIEWPORT", p.text_primary, m.title_scale);
    y += m.title_glyph_h + m.section_gap;
    (void)snprintf(line, sizeof(line), "WORLD VIEW  ZOOM: %.2fx", (double)ctx->editor.viewport.zoom);
    drawing_program_visual_draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
}

static const DrawingProgramVisualCanvasWorldRenderHooks *visual_canvas_world_render_hooks(void) {
    static const DrawingProgramVisualCanvasWorldRenderHooks hooks = {
        .compute_canvas_sheet_metrics = drawing_program_visual_compute_canvas_sheet_metrics,
        .draw_selection_overlay = draw_selection_overlay,
        .draw_object_overlay = draw_object_overlay,
        .draw_shape_preview_overlay = draw_shape_preview_overlay
    };
    return &hooks;
}

static void draw_canvas_world_view(SDL_Renderer *renderer,
                                   SDL_Rect pane_rect,
                                   const DrawingProgramAppContext *ctx,
                                   const CoreThemePreset *theme,
                                   const VisualSelectionState *selection,
                                   const VisualPanelUiState *ui,
                                   const VisualCanvasInteractionState *interaction) {
    drawing_program_visual_draw_canvas_world_view(
        renderer,
        pane_rect,
        ctx,
        theme,
        selection,
        ui,
        interaction,
        visual_canvas_world_render_hooks());
}

static const DrawingProgramVisualFrameRenderHooks *visual_frame_render_hooks(void) {
    static const DrawingProgramVisualFrameRenderHooks hooks = {
        .module_type_for_pane = drawing_program_visual_module_type_for_pane,
        .draw_menu_bar_chrome = draw_menu_bar_chrome,
        .draw_left_panel_chrome = draw_left_panel_chrome,
        .draw_right_panel_chrome = draw_right_panel_chrome,
        .draw_canvas_world_view = draw_canvas_world_view,
        .draw_canvas_viewport_chrome = draw_canvas_viewport_chrome
    };
    return &hooks;
}

int drawing_program_visual_draw_debug_frame(SDL_Window *window,
                                            SDL_Renderer *renderer,
                                            const DrawingProgramAppContext *ctx,
                                            const CoreThemePreset *theme,
                                            const VisualPanelUiState *ui,
                                            const VisualSelectionState *selection,
                                            const VisualCanvasInteractionState *interaction) {
    return drawing_program_visual_draw_frame(
        window, renderer, ctx, theme, ui, selection, interaction, visual_frame_render_hooks());
}

static const DrawingProgramVisualTransformOpsHooks *visual_transform_ops_hooks(void) {
    static const DrawingProgramVisualTransformOpsHooks hooks = {
        .visual_selection_commit_move = visual_selection_commit_move,
        .visual_object_commit_move = visual_object_commit_move,
        .visual_object_commit_path_point = visual_object_commit_path_point,
        .visual_object_commit_path_point_data = visual_object_commit_path_point_data
    };
    return &hooks;
}

static void cancel_canvas_draw_and_shape(VisualCanvasInteractionState *interaction) {
    drawing_program_visual_cancel_canvas_draw_and_shape(interaction);
}

static void cancel_selection_transient(VisualSelectionState *selection) {
    drawing_program_visual_cancel_selection_transient(selection);
}

static int visual_transform_session_is_move_active(const VisualCanvasInteractionState *interaction) {
    return drawing_program_visual_transform_session_is_move_active(interaction);
}

static int visual_transform_session_is_object_move_active(const VisualCanvasInteractionState *interaction) {
    return drawing_program_visual_transform_session_is_object_move_active(interaction);
}

static int visual_transform_session_is_object_path_point_move_active(
    const VisualCanvasInteractionState *interaction) {
    return drawing_program_visual_transform_session_is_object_path_point_move_active(interaction);
}

static int visual_transform_session_is_object_path_handle_move_active(
    const VisualCanvasInteractionState *interaction) {
    return drawing_program_visual_transform_session_is_object_path_handle_move_active(interaction);
}

static void visual_transform_session_begin_move(VisualCanvasInteractionState *interaction,
                                                VisualSelectionState *selection,
                                                uint32_t sample_x,
                                                uint32_t sample_y) {
    drawing_program_visual_transform_session_begin_move(interaction, selection, sample_x, sample_y);
}

static void visual_transform_session_update_move(const DrawingProgramAppContext *ctx,
                                                 VisualCanvasInteractionState *interaction,
                                                 VisualSelectionState *selection,
                                                 uint32_t sample_x,
                                                 uint32_t sample_y,
                                                         SDL_Keymod mods) {
    drawing_program_visual_transform_session_update_move(ctx, interaction, selection, sample_x, sample_y, mods);
}

static void visual_transform_session_begin_object_move(VisualCanvasInteractionState *interaction,
                                                       uint32_t sample_x,
                                                       uint32_t sample_y) {
    drawing_program_visual_transform_session_begin_object_move(interaction, sample_x, sample_y);
}

static void visual_transform_session_begin_object_path_point_move(VisualCanvasInteractionState *interaction,
                                                                  uint32_t object_id,
                                                                  uint16_t point_index,
                                                                  uint32_t sample_x,
                                                                  uint32_t sample_y) {
    drawing_program_visual_transform_session_begin_object_path_point_move(
        interaction, object_id, point_index, sample_x, sample_y);
}

static void visual_transform_session_begin_object_path_handle_move(VisualCanvasInteractionState *interaction,
                                                                   uint32_t object_id,
                                                                   uint16_t point_index,
                                                                   uint8_t handle_kind,
                                                                   const DrawingProgramPathPoint *point) {
    drawing_program_visual_transform_session_begin_object_path_handle_move(
        interaction, object_id, point_index, handle_kind, point);
}

static void visual_transform_session_update_object_move(const DrawingProgramAppContext *ctx,
                                                        VisualCanvasInteractionState *interaction,
                                                        uint32_t sample_x,
                                                        uint32_t sample_y,
                                                        SDL_Keymod mods) {
    drawing_program_visual_transform_session_update_object_move(ctx, interaction, sample_x, sample_y, mods);
}

static void visual_transform_session_update_object_path_point_move(const DrawingProgramAppContext *ctx,
                                                                   VisualCanvasInteractionState *interaction,
                                                                   uint32_t sample_x,
                                                                   uint32_t sample_y,
                                                                   SDL_Keymod mods) {
    drawing_program_visual_transform_session_update_object_path_point_move(
        ctx, interaction, sample_x, sample_y, mods);
}

static void visual_transform_session_update_object_path_handle_move(const DrawingProgramAppContext *ctx,
                                                                    VisualCanvasInteractionState *interaction,
                                                                    uint32_t sample_x,
                                                                    uint32_t sample_y) {
    drawing_program_visual_transform_session_update_object_path_handle_move(
        ctx, interaction, sample_x, sample_y);
}

static CoreResult visual_transform_session_commit_move(DrawingProgramAppContext *ctx,
                                                       VisualCanvasInteractionState *interaction,
                                                       VisualSelectionState *selection) {
    return drawing_program_visual_transform_session_commit_move(ctx, interaction, selection, visual_transform_ops_hooks());
}

static CoreResult visual_transform_session_commit_object_move(DrawingProgramAppContext *ctx,
                                                              VisualCanvasInteractionState *interaction) {
    return drawing_program_visual_transform_session_commit_object_move(ctx, interaction, visual_transform_ops_hooks());
}

static CoreResult visual_transform_session_commit_object_path_point_move(
    DrawingProgramAppContext *ctx,
    VisualCanvasInteractionState *interaction) {
    return drawing_program_visual_transform_session_commit_object_path_point_move(
        ctx, interaction, visual_transform_ops_hooks());
}

static CoreResult visual_transform_session_commit_object_path_handle_move(
    DrawingProgramAppContext *ctx,
    VisualCanvasInteractionState *interaction) {
    return drawing_program_visual_transform_session_commit_object_path_handle_move(
        ctx, interaction, visual_transform_ops_hooks());
}

static CoreResult visual_transform_session_nudge_move(DrawingProgramAppContext *ctx,
                                                      VisualCanvasInteractionState *interaction,
                                                      VisualSelectionState *selection,
                                                      int32_t dx,
                                                      int32_t dy) {
    return drawing_program_visual_transform_session_nudge_move(
        ctx, interaction, selection, dx, dy, visual_transform_ops_hooks());
}

static CoreResult visual_transform_session_nudge_object_move(DrawingProgramAppContext *ctx,
                                                             VisualCanvasInteractionState *interaction,
                                                             int32_t dx,
                                                             int32_t dy) {
    return drawing_program_visual_transform_session_nudge_object_move(
        ctx, interaction, dx, dy, visual_transform_ops_hooks());
}

static void begin_canvas_history_group(DrawingProgramAppContext *ctx) {
    drawing_program_visual_begin_canvas_history_group(ctx);
}

static void cancel_all_transient_interactions(DrawingProgramAppContext *ctx,
                                              VisualCanvasInteractionState *interaction,
                                              VisualSelectionState *selection,
                                              int clear_pan_state) {
    drawing_program_visual_cancel_all_transient_interactions(ctx, interaction, selection, clear_pan_state);
}

const DrawingProgramVisualInputHandlersHooks *drawing_program_visual_input_handlers_hooks(void) {
    static const DrawingProgramVisualInputHandlersHooks hooks = {
        .cancel_all_transient_interactions = cancel_all_transient_interactions,
        .screen_to_canvas_sample = drawing_program_visual_screen_to_canvas_sample,
        .screen_to_canvas_sample_clamped = drawing_program_visual_screen_to_canvas_sample_clamped,
        .visual_transform_session_is_move_active = visual_transform_session_is_move_active,
        .visual_transform_session_is_object_move_active = visual_transform_session_is_object_move_active,
        .visual_transform_session_is_object_path_point_move_active =
            visual_transform_session_is_object_path_point_move_active,
        .visual_transform_session_is_object_path_handle_move_active =
            visual_transform_session_is_object_path_handle_move_active,
        .object_path_point_hit_test_selected = object_path_point_hit_test_selected,
        .object_path_handle_hit_test_selected = object_path_handle_hit_test_selected,
        .object_path_edge_hit_test_selected = object_path_edge_hit_test_selected,
        .visual_transform_session_update_move = visual_transform_session_update_move,
        .visual_transform_session_update_object_move = visual_transform_session_update_object_move,
        .visual_transform_session_update_object_path_point_move =
            visual_transform_session_update_object_path_point_move,
        .visual_transform_session_update_object_path_handle_move =
            visual_transform_session_update_object_path_handle_move,
        .visual_transform_session_commit_move = visual_transform_session_commit_move,
        .visual_transform_session_commit_object_move = visual_transform_session_commit_object_move,
        .visual_transform_session_commit_object_path_point_move =
            visual_transform_session_commit_object_path_point_move,
        .visual_transform_session_commit_object_path_handle_move =
            visual_transform_session_commit_object_path_handle_move,
        .visual_marquee_commit_mode_clamp = visual_marquee_commit_mode_clamp,
        .visual_selection_capture_from_marquee = visual_selection_capture_from_marquee,
        .tool_uses_shape_commit = drawing_program_visual_tool_uses_shape_commit,
        .apply_canvas_shape_commit = apply_canvas_shape_commit,
        .apply_canvas_draw_at_screen = apply_canvas_draw_at_screen,
        .visual_marquee_commit_mode_from_mods = visual_marquee_commit_mode_from_mods,
        .visual_selection_begin_move = visual_selection_begin_move,
        .selection_move_handle_hit = selection_move_handle_hit,
        .visual_transform_session_begin_move = visual_transform_session_begin_move,
        .visual_transform_session_begin_object_move = visual_transform_session_begin_object_move,
        .visual_transform_session_begin_object_path_point_move =
            visual_transform_session_begin_object_path_point_move,
        .visual_transform_session_begin_object_path_handle_move =
            visual_transform_session_begin_object_path_handle_move,
        .apply_canvas_picker_at_screen = apply_canvas_picker_at_screen,
        .apply_canvas_fill_at_screen = apply_canvas_fill_at_screen,
        .begin_canvas_history_group = begin_canvas_history_group,
        .delete_active_selection_payload_or_objects = delete_active_selection_payload_or_objects,
        .path_draft_commit = path_draft_commit,
        .apply_insert_object_path_point = visual_object_insert_path_point,
        .path_draft_reset = path_draft_reset,
        .path_draft_pop_point = path_draft_pop_point,
        .cancel_selection_transient = cancel_selection_transient,
        .cancel_canvas_draw_and_shape = cancel_canvas_draw_and_shape,
        .tool_uses_direct_sample_stroke = drawing_program_visual_tool_uses_direct_sample_stroke,
        .set_module_type_for_pane = drawing_program_visual_set_module_type_for_pane,
        .visual_tool_count = drawing_program_visual_tool_count,
        .visual_tool_at = drawing_program_visual_tool_at,
        .workflow_control_for_tool = drawing_program_visual_workflow_control_for_tool,
        .visual_tool_option_count = drawing_program_visual_tool_option_count,
        .visual_tool_option_kind_for_index_raw = drawing_program_visual_tool_option_kind_for_index_raw,
        .visual_tool_option_is_action_button_raw = drawing_program_visual_tool_option_is_action_button_raw,
        .visual_tool_option_is_select_delete_raw = drawing_program_visual_tool_option_is_select_delete_raw,
        .visual_tool_option_adjust_raw = drawing_program_visual_tool_option_adjust_raw,
        .sync_panel_ui_from_app = drawing_program_visual_sync_panel_ui_from_app,
        .clamp_left_slot = drawing_program_visual_clamp_left_slot,
        .clamp_right_slot = drawing_program_visual_clamp_right_slot,
        .apply_layer_rename_auto = drawing_program_visual_apply_layer_rename_auto,
        .apply_layer_duplicate_active = drawing_program_visual_apply_layer_duplicate_active,
        .active_layer_query = active_layer_query,
        .clamp_font_zoom_step = clamp_font_zoom_step,
        .apply_workflow_control_if_valid = drawing_program_visual_apply_workflow_control_if_valid,
        .active_layer_allows_edits_visual = active_layer_allows_edits_visual,
        .point_in_rect = drawing_program_visual_point_in_rect,
        .clamp_theme_preset_id = clamp_theme_preset_id,
        .cycle_theme_preset = cycle_theme_preset,
        .visual_transform_session_nudge_move = visual_transform_session_nudge_move,
        .visual_transform_session_nudge_object_move = visual_transform_session_nudge_object_move
    };
    return &hooks;
}
