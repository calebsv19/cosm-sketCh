#include "drawing_program/drawing_program_snapshot_ui_settings.h"

#include <string.h>

#include "drawing_program_snapshot_ui_settings_internal.h"

#include "drawing_program/drawing_program_authoring_host.h"

CoreResult drawing_program_snapshot_write_ui_settings_chunk(
    CorePackWriter *writer,
    const struct DrawingProgramAppContext *ctx) {
    DrawingProgramUiSettingsV14 ui_settings;
    DrawingProgramAppUiState accepted_ui;
    if (!writer || !ctx) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid snapshot ui settings write request" };
    }
    memset(&ui_settings, 0, sizeof(ui_settings));
    memset(&accepted_ui, 0, sizeof(accepted_ui));
    drawing_program_authoring_host_export_accepted_ui_state(ctx, &accepted_ui);
    ui_settings.version = DRAWING_PROGRAM_UI_SETTINGS_VERSION_V14;
    ui_settings.theme_preset_id = accepted_ui.theme_preset_id;
    ui_settings.font_preset_id = accepted_ui.font_preset_id;
    ui_settings.font_zoom_step = (int32_t)accepted_ui.font_zoom_step;
    ui_settings.left_panel_slot = accepted_ui.left_panel_slot;
    ui_settings.right_panel_slot = accepted_ui.right_panel_slot;
    ui_settings.active_color_index = accepted_ui.active_color_index;
    ui_settings.selection_has_payload =
        (ctx->selection.has_payload && ctx->selection.width > 0u && ctx->selection.height > 0u) ? 1u : 0u;
    ui_settings.selection_origin_x = ctx->selection.origin_x;
    ui_settings.selection_origin_y = ctx->selection.origin_y;
    ui_settings.selection_width = ctx->selection.width;
    ui_settings.selection_height = ctx->selection.height;
    ui_settings.tool_brush_size = accepted_ui.tool_brush_size;
    ui_settings.tool_brush_opacity = accepted_ui.tool_brush_opacity;
    ui_settings.tool_brush_spacing = accepted_ui.tool_brush_spacing;
    ui_settings.tool_brush_hardness = accepted_ui.tool_brush_hardness;
    ui_settings.tool_eraser_size = accepted_ui.tool_eraser_size;
    ui_settings.tool_shape_stroke_width = accepted_ui.tool_shape_stroke_width;
    ui_settings.tool_shape_mode = accepted_ui.tool_shape_mode;
    ui_settings.tool_shape_target_mode = accepted_ui.tool_shape_target_mode;
    ui_settings.tool_fill_tolerance = accepted_ui.tool_fill_tolerance;
    ui_settings.tool_select_mode = accepted_ui.tool_select_mode;
    ui_settings.canvas_control_mode = accepted_ui.canvas_control_mode;
    ui_settings.canvas_guide_mode = accepted_ui.canvas_guide_mode;
    ui_settings.canvas_reflection_center_valid = accepted_ui.canvas_reflection_center_valid;
    ui_settings.layer_opacity_entry_count = accepted_ui.layer_opacity_entry_count;
    ui_settings.recent_color_count = accepted_ui.recent_color_count;
    ui_settings.selected_recent_color_index = accepted_ui.selected_recent_color_index;
    ui_settings.color_hue = accepted_ui.color_hue;
    ui_settings.color_saturation = accepted_ui.color_saturation;
    ui_settings.color_value = accepted_ui.color_value;
    ui_settings.active_paint_r = accepted_ui.active_paint_r;
    ui_settings.active_paint_g = accepted_ui.active_paint_g;
    ui_settings.active_paint_b = accepted_ui.active_paint_b;
    ui_settings.canvas_reflection_center_x = accepted_ui.canvas_reflection_center_x;
    ui_settings.canvas_reflection_center_y = accepted_ui.canvas_reflection_center_y;
    ui_settings.reflection_state = accepted_ui.reflection_state;
    if (ui_settings.reflection_state.reflector_count == 0u) {
        drawing_program_reflection_state_seed_crosshair(&ui_settings.reflection_state,
                                                        accepted_ui.canvas_reflection_center_x,
                                                        accepted_ui.canvas_reflection_center_y);
        drawing_program_reflection_state_set_crosshair_enabled(&ui_settings.reflection_state, 0u, 0u);
    }
    memcpy(ui_settings.layer_opacity_layer_ids,
           accepted_ui.layer_opacity_layer_ids,
           sizeof(ui_settings.layer_opacity_layer_ids));
    memcpy(ui_settings.layer_opacity_values,
           accepted_ui.layer_opacity_values,
           sizeof(ui_settings.layer_opacity_values));
    memcpy(ui_settings.recent_color_rgb,
           ctx->ui.recent_color_rgb,
           sizeof(ui_settings.recent_color_rgb));
    memcpy(ui_settings.color_palette_rgb,
           ctx->ui.color_palette_rgb,
           sizeof(ui_settings.color_palette_rgb));
    return core_pack_writer_add_chunk(writer, "DPUI", &ui_settings, (uint64_t)sizeof(ui_settings));
}
