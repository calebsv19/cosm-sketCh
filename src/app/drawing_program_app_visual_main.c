#include <SDL2/SDL.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_layer_actions.h"
#include "drawing_program/drawing_program_visual_overlay_render.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_visual_panel_render.h"
#include "drawing_program/drawing_program_visual_resources.h"
#include "drawing_program/drawing_program_visual_shape_ops.h"
#include "drawing_program/drawing_program_visual_state.h"
#include "drawing_program/drawing_program_visual_text_render.h"
#include "drawing_program/drawing_program_visual_theme.h"
#include "drawing_program/drawing_program_visual_tool_options.h"

static int has_flag(int argc, char **argv, const char *flag) {
    int i;
    if (!argv || !flag) {
        return 0;
    }
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], flag) == 0) {
            return 1;
        }
    }
    return 0;
}

static int visual_trace_ui_state_enabled(void) {
    const char *value = getenv("DRAWING_PROGRAM_TRACE_UI_STATE");
    if (!value || value[0] == '\0' || value[0] == '0') {
        return 0;
    }
    return 1;
}

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
        .color_value_from_index = drawing_program_color_value_from_index,
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

static void update_window_title(SDL_Window *window,
                                const DrawingProgramAppContext *ctx,
                                const VisualSelectionState *selection,
                                uint64_t present_count) {
    char title[256];
    uint32_t center_module_type_id;
    const char *font_name = "unknown";
    const char *text_backend = "bitmap";
    uint32_t selection_w = 0u;
    uint32_t selection_h = 0u;
    int32_t selection_dx = 0;
    int32_t selection_dy = 0;
    if (!window || !ctx) {
        return;
    }
    if (selection && selection->has_payload) {
        selection_w = selection->width;
        selection_h = selection->height;
        selection_dx = selection->offset_x;
        selection_dy = selection->offset_y;
    }
    center_module_type_id = drawing_program_visual_module_type_for_pane(ctx, 6u);
    if (ctx->ui_font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
        font_name = core_font_preset_name((CoreFontPresetId)ctx->ui_font_preset_id);
    }
    if (drawing_program_visual_font_backend_is_ready()) {
        text_backend = "ttf";
    }
    (void)snprintf(title,
                   sizeof(title),
                   "sketCh | backend=sdl-debug text_backend=%s frame=%llu present=%llu panes=%u center_module=%u active_tool=%u color=%u visible_layers=%u active_layer=%u sel=%ux%u d=%d,%d theme=%u font=%s zoomstep=%d",
                   text_backend,
                   (unsigned long long)ctx->frame_counter,
                   (unsigned long long)present_count,
                   ctx->pane_host.leaf_count,
                   center_module_type_id,
                   (unsigned)ctx->editor.active_tool,
                   (unsigned)drawing_program_color_index_clamp(ctx->ui_active_color_index),
                   ctx->render_projection.visible_layer_count,
                   ctx->render_projection.active_layer_id,
                   selection_w,
                   selection_h,
                   (int)selection_dx,
                   (int)selection_dy,
                   ctx->ui_theme_preset_id,
                   font_name ? font_name : "unknown",
                   (int)ctx->ui_font_zoom_step);
    SDL_SetWindowTitle(window, title);
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

static int draw_visual_debug_frame(SDL_Window *window,
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
        .visual_object_commit_move = visual_object_commit_move
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

static void visual_transform_session_update_object_move(const DrawingProgramAppContext *ctx,
                                                        VisualCanvasInteractionState *interaction,
                                                        uint32_t sample_x,
                                                        uint32_t sample_y,
                                                        SDL_Keymod mods) {
    drawing_program_visual_transform_session_update_object_move(ctx, interaction, sample_x, sample_y, mods);
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

static const DrawingProgramVisualInputHandlersHooks *visual_input_handlers_hooks(void) {
    static const DrawingProgramVisualInputHandlersHooks hooks = {
        .cancel_all_transient_interactions = cancel_all_transient_interactions,
        .screen_to_canvas_sample = drawing_program_visual_screen_to_canvas_sample,
        .screen_to_canvas_sample_clamped = drawing_program_visual_screen_to_canvas_sample_clamped,
        .visual_transform_session_is_move_active = visual_transform_session_is_move_active,
        .visual_transform_session_is_object_move_active = visual_transform_session_is_object_move_active,
        .visual_transform_session_update_move = visual_transform_session_update_move,
        .visual_transform_session_update_object_move = visual_transform_session_update_object_move,
        .visual_transform_session_commit_move = visual_transform_session_commit_move,
        .visual_transform_session_commit_object_move = visual_transform_session_commit_object_move,
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
        .apply_canvas_picker_at_screen = apply_canvas_picker_at_screen,
        .apply_canvas_fill_at_screen = apply_canvas_fill_at_screen,
        .begin_canvas_history_group = begin_canvas_history_group,
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

static int run_visual_mode(int argc, char **argv) {
    CoreResult result;
    DrawingProgramRenderBackendKind backend_kind = DRAWING_PROGRAM_RENDER_BACKEND_SDL_DEBUG;
    CoreThemePreset theme_preset;
    CoreResult theme_env_result;
    CoreThemePresetId selected_theme = CORE_THEME_PRESET_DARK_DEFAULT;
    CoreFontPresetId selected_font = CORE_FONT_PRESET_IDE;
    DrawingProgramAppContext app;
    SDL_Window *window = 0;
    SDL_Renderer *renderer = 0;
    int quit = 0;
    uint64_t present_count = 0u;
    VisualCanvasInteractionState canvas_interaction;
    VisualPanelUiState panel_ui;
    const DrawingProgramVisualInputHandlersHooks *input_handlers = visual_input_handlers_hooks();
#define selection_state app.selection

    result = drawing_program_render_backend_parse_flag(argc, argv, &backend_kind);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: %s\n", result.message);
        return 1;
    }
    if (!drawing_program_render_backend_is_supported_now(backend_kind)) {
        fprintf(stderr,
                "drawing_program: render backend '%s' is defined but not implemented in P4-S1 (use --render-backend sdl-debug)\n",
                drawing_program_render_backend_kind_string(backend_kind));
        return 2;
    }
    theme_env_result = core_theme_apply_env_override("CORE_THEME_PRESET", &selected_theme);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "drawing_program: SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("sketCh",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              1280,
                              800,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        fprintf(stderr, "drawing_program: SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        fprintf(stderr, "drawing_program: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    result = drawing_program_app_bootstrap(&app, argc, argv);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: bootstrap failed: %s\n", result.message);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    result = drawing_program_app_config_load(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: config_load failed: %s\n", result.message);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    result = drawing_program_app_state_seed(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: state_seed failed: %s\n", result.message);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    result = drawing_program_app_subsystems_init(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: subsystems_init failed: %s\n", result.message);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    result = drawing_program_runtime_start(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: runtime_start failed: %s\n", result.message);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    if (visual_trace_ui_state_enabled()) {
        fprintf(stderr,
                "drawing_program trace visual after_runtime_start tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u\n",
                (unsigned)app.editor.active_tool,
                (unsigned)app.ui_theme_preset_id,
                (unsigned)app.ui_font_preset_id,
                (int)app.ui_font_zoom_step,
                (unsigned)app.ui_left_panel_slot,
                (unsigned)app.ui_right_panel_slot);
    }
    if (theme_env_result.code == CORE_OK && !app.snapshot_loaded_from_preset) {
        app.ui_theme_preset_id = (uint32_t)selected_theme;
    }
    selected_theme = clamp_theme_preset_id(app.ui_theme_preset_id);
    app.ui_theme_preset_id = (uint32_t)selected_theme;
    if (core_theme_get_preset(selected_theme, &theme_preset).code != CORE_OK) {
        selected_theme = CORE_THEME_PRESET_DARK_DEFAULT;
        app.ui_theme_preset_id = (uint32_t)selected_theme;
        if (core_theme_get_preset(selected_theme, &theme_preset).code != CORE_OK) {
            fprintf(stderr, "drawing_program: failed to resolve core theme preset\n");
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
    }
    if (app.ui_font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
        selected_font = (CoreFontPresetId)app.ui_font_preset_id;
    } else {
        app.ui_font_preset_id = (uint32_t)selected_font;
    }
    if (!core_font_preset_name(selected_font)) {
        selected_font = CORE_FONT_PRESET_IDE;
        app.ui_font_preset_id = (uint32_t)selected_font;
    }
    app.ui_font_zoom_step = (int8_t)clamp_font_zoom_step((int)app.ui_font_zoom_step);
    memset(&canvas_interaction, 0, sizeof(canvas_interaction));
    canvas_interaction.marquee_commit_mode = (uint8_t)VISUAL_MARQUEE_COMMIT_REPLACE;
    memset(&panel_ui, 0, sizeof(panel_ui));
    drawing_program_selection_cancel_transient(&selection_state);
    drawing_program_visual_sync_panel_ui_from_app(&app, &panel_ui);
    if (visual_trace_ui_state_enabled()) {
        fprintf(stderr,
                "drawing_program trace visual after_ui_resolve tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u env_override=%d\n",
                (unsigned)app.editor.active_tool,
                (unsigned)app.ui_theme_preset_id,
                (unsigned)app.ui_font_preset_id,
                (int)app.ui_font_zoom_step,
                (unsigned)app.ui_left_panel_slot,
                (unsigned)app.ui_right_panel_slot,
                theme_env_result.code == CORE_OK ? 1 : 0);
    }
    {
        int initial_w = 0;
        int initial_h = 0;
        if (SDL_GetRendererOutputSize(renderer, &initial_w, &initial_h) == 0 &&
            initial_w > 0 && initial_h > 0) {
            result = drawing_program_app_set_pane_host_bounds(&app, (float)initial_w, (float)initial_h);
            if (result.code != CORE_OK) {
                fprintf(stderr, "drawing_program: set pane host bounds failed: %s\n", result.message);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 1;
            }
        }
    }

    while (!quit) {
        SDL_Event event;
        int current_w = 0;
        int current_h = 0;
        while (SDL_PollEvent(&event)) {
            int event_x = 0;
            int event_y = 0;
            int event_has_position = 0;
            SDL_Rect canvas_pane = { 0, 0, 0, 0 };
            SDL_Rect left_pane = { 0, 0, 0, 0 };
            SDL_Rect right_pane = { 0, 0, 0, 0 };
            int has_canvas_pane = drawing_program_visual_pane_rect_for_module_type(&app, 1u, &canvas_pane);
            int has_left_pane = drawing_program_visual_pane_rect_for_module_type(&app, 2u, &left_pane);
            int has_right_pane = drawing_program_visual_pane_rect_for_module_type(&app, 4u, &right_pane);
            drawing_program_visual_input_map_event_position(window,
                                                            renderer,
                                                            &event,
                                                            &event_x,
                                                            &event_y,
                                                            &event_has_position);
            if (event_has_position) {
                panel_ui.mouse_known = 1u;
                panel_ui.mouse_x = event_x;
                panel_ui.mouse_y = event_y;
            }
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                quit = 1;
            }
            if (event.type == SDL_WINDOWEVENT) {
                uint8_t clear_mouse_known = 0u;
                uint8_t cancel_transients = 0u;
                drawing_program_visual_input_window_event_flags(&event,
                                                                &clear_mouse_known,
                                                                &cancel_transients);
                if (clear_mouse_known) {
                    panel_ui.mouse_known = 0u;
                }
                if (cancel_transients) {
                    cancel_all_transient_interactions(&app, &canvas_interaction, &selection_state, 1);
                }
            }
            drawing_program_visual_input_apply_mouse_wheel_zoom(&app,
                                                                 window,
                                                                 renderer,
                                                                 has_canvas_pane,
                                                                 canvas_pane,
                                                                 &event);
            if (drawing_program_visual_input_handle_mouse_button_down_payload(&event,
                                                                              event_has_position,
                                                                              event_x,
                                                                              event_y,
                                                                              has_left_pane,
                                                                              left_pane,
                                                                              has_right_pane,
                                                                              right_pane,
                                                                              has_canvas_pane,
                                                                              canvas_pane,
                                                                              &app,
                                                                              &canvas_interaction,
                                                                              &selection_state,
                                                                              &panel_ui,
                                                                              input_handlers)) {
                continue;
            }
            if (event.type == SDL_MOUSEBUTTONUP) {
                if (drawing_program_visual_input_handle_mouse_button_up_payload(&event,
                                                                                 event_has_position,
                                                                                 event_x,
                                                                                 event_y,
                                                                                 has_canvas_pane,
                                                                                 canvas_pane,
                                                                                 &app,
                                                                                 &canvas_interaction,
                                                                                 &selection_state,
                                                                                 input_handlers)) {
                    continue;
                }
            }
            if (event.type == SDL_MOUSEMOTION) {
                if (drawing_program_visual_input_handle_mouse_motion_payload(&event,
                                                                             event_has_position,
                                                                             event_x,
                                                                             event_y,
                                                                             has_canvas_pane,
                                                                             canvas_pane,
                                                                             &app,
                                                                             &canvas_interaction,
                                                                             &selection_state,
                                                                             &panel_ui,
                                                                             input_handlers)) {
                    continue;
                }
            }
            if (drawing_program_visual_input_handle_keydown_payload(&event,
                                                                    has_canvas_pane,
                                                                    canvas_pane,
                                                                    &selected_theme,
                                                                    &theme_preset,
                                                                    &app,
                                                                    &canvas_interaction,
                                                                    &selection_state,
                                                                    &panel_ui,
                                                                    input_handlers)) {
                continue;
            }
        }
        if (SDL_GetRendererOutputSize(renderer, &current_w, &current_h) == 0 &&
            current_w > 0 && current_h > 0) {
            result = drawing_program_app_set_pane_host_bounds(&app, (float)current_w, (float)current_h);
            if (result.code != CORE_OK) {
                fprintf(stderr, "drawing_program: set pane host bounds failed: %s\n", result.message);
                break;
            }
        }

        result = drawing_program_app_run_loop(&app);
        if (result.code != CORE_OK) {
            fprintf(stderr, "drawing_program: run_loop failed: %s\n", result.message);
            break;
        }
        drawing_program_visual_layer_opacity_sync_document(&app);
        drawing_program_visual_text_set_font_preset_id(app.ui_font_preset_id);
        if (!draw_visual_debug_frame(window,
                                     renderer,
                                     &app,
                                     &theme_preset,
                                     &panel_ui,
                                     &selection_state,
                                     &canvas_interaction)) {
            fprintf(stderr, "drawing_program: visual debug frame draw failed\n");
            result = (CoreResult){ CORE_ERR_IO, "visual debug frame draw failed" };
            break;
        }
        SDL_RenderPresent(renderer);
        present_count += 1u;
        update_window_title(window, &app, &selection_state, present_count);
        SDL_Delay(16);
    }

#undef selection_state

    result = drawing_program_app_shutdown(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: shutdown failed: %s\n", result.message);
    }

    drawing_program_visual_font_cache_shutdown();
    drawing_program_visual_canvas_texture_shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return (result.code == CORE_OK) ? 0 : 1;
}

int drawing_program_app_visual_main(int argc, char **argv) {
    if (has_flag(argc, argv, "--headless")) {
        return drawing_program_app_main(argc, argv);
    }
    return run_visual_mode(argc, argv);
}
