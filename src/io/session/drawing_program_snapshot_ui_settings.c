#include "drawing_program/drawing_program_snapshot_ui_settings.h"

#include <string.h>

#include "core_font.h"
#include "core_theme.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_selection.h"

typedef struct DrawingProgramUiSettingsV1 {
    uint32_t version;
    uint32_t theme_preset_id;
    uint8_t left_panel_slot;
    uint8_t right_panel_slot;
    uint8_t reserved0;
    uint8_t reserved1;
} DrawingProgramUiSettingsV1;

typedef struct DrawingProgramUiSettingsV2 {
    uint32_t version;
    uint32_t theme_preset_id;
    uint32_t font_preset_id;
    int32_t font_zoom_step;
    uint8_t left_panel_slot;
    uint8_t right_panel_slot;
    uint8_t reserved0;
    uint8_t reserved1;
} DrawingProgramUiSettingsV2;

typedef struct DrawingProgramUiSettingsV3 {
    uint32_t version;
    uint32_t theme_preset_id;
    uint32_t font_preset_id;
    int32_t font_zoom_step;
    uint8_t left_panel_slot;
    uint8_t right_panel_slot;
    uint8_t active_color_index;
    uint8_t reserved0;
} DrawingProgramUiSettingsV3;

typedef struct DrawingProgramUiSettingsV4 {
    uint32_t version;
    uint32_t theme_preset_id;
    uint32_t font_preset_id;
    int32_t font_zoom_step;
    uint8_t left_panel_slot;
    uint8_t right_panel_slot;
    uint8_t active_color_index;
    uint8_t selection_has_payload;
    uint32_t selection_origin_x;
    uint32_t selection_origin_y;
    uint32_t selection_width;
    uint32_t selection_height;
} DrawingProgramUiSettingsV4;

typedef struct DrawingProgramUiSettingsV5 {
    uint32_t version;
    uint32_t theme_preset_id;
    uint32_t font_preset_id;
    int32_t font_zoom_step;
    uint8_t left_panel_slot;
    uint8_t right_panel_slot;
    uint8_t active_color_index;
    uint8_t selection_has_payload;
    uint32_t selection_origin_x;
    uint32_t selection_origin_y;
    uint32_t selection_width;
    uint32_t selection_height;
    uint8_t tool_brush_size;
    uint8_t tool_brush_opacity;
    uint8_t tool_eraser_size;
    uint8_t tool_shape_stroke_width;
    uint8_t tool_shape_mode;
    uint8_t tool_fill_tolerance;
    uint8_t reserved0;
    uint8_t reserved1;
} DrawingProgramUiSettingsV5;

typedef struct DrawingProgramUiSettingsV6 {
    uint32_t version;
    uint32_t theme_preset_id;
    uint32_t font_preset_id;
    int32_t font_zoom_step;
    uint8_t left_panel_slot;
    uint8_t right_panel_slot;
    uint8_t active_color_index;
    uint8_t selection_has_payload;
    uint32_t selection_origin_x;
    uint32_t selection_origin_y;
    uint32_t selection_width;
    uint32_t selection_height;
    uint8_t tool_brush_size;
    uint8_t tool_brush_opacity;
    uint8_t tool_brush_spacing;
    uint8_t tool_brush_hardness;
    uint8_t tool_eraser_size;
    uint8_t tool_shape_stroke_width;
    uint8_t tool_shape_mode;
    uint8_t tool_fill_tolerance;
    uint8_t layer_opacity_entry_count;
    uint8_t reserved0;
    uint8_t reserved1;
    uint8_t reserved2;
    uint32_t layer_opacity_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t layer_opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
} DrawingProgramUiSettingsV6;

typedef struct DrawingProgramUiSettingsV7 {
    uint32_t version;
    uint32_t theme_preset_id;
    uint32_t font_preset_id;
    int32_t font_zoom_step;
    uint8_t left_panel_slot;
    uint8_t right_panel_slot;
    uint8_t active_color_index;
    uint8_t selection_has_payload;
    uint32_t selection_origin_x;
    uint32_t selection_origin_y;
    uint32_t selection_width;
    uint32_t selection_height;
    uint8_t tool_brush_size;
    uint8_t tool_brush_opacity;
    uint8_t tool_brush_spacing;
    uint8_t tool_brush_hardness;
    uint8_t tool_eraser_size;
    uint8_t tool_shape_stroke_width;
    uint8_t tool_shape_mode;
    uint8_t tool_shape_target_mode;
    uint8_t tool_fill_tolerance;
    uint8_t tool_select_mode;
    uint8_t layer_opacity_entry_count;
    uint8_t reserved0;
    uint8_t reserved1;
    uint8_t reserved2;
    uint32_t layer_opacity_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t layer_opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
} DrawingProgramUiSettingsV7;

typedef struct DrawingProgramUiSettingsV8 {
    uint32_t version;
    uint32_t theme_preset_id;
    uint32_t font_preset_id;
    int32_t font_zoom_step;
    uint8_t left_panel_slot;
    uint8_t right_panel_slot;
    uint8_t active_color_index;
    uint8_t selection_has_payload;
    uint32_t selection_origin_x;
    uint32_t selection_origin_y;
    uint32_t selection_width;
    uint32_t selection_height;
    uint8_t tool_brush_size;
    uint8_t tool_brush_opacity;
    uint8_t tool_brush_spacing;
    uint8_t tool_brush_hardness;
    uint8_t tool_eraser_size;
    uint8_t tool_shape_stroke_width;
    uint8_t tool_shape_mode;
    uint8_t tool_shape_target_mode;
    uint8_t tool_fill_tolerance;
    uint8_t tool_select_mode;
    uint8_t layer_opacity_entry_count;
    uint8_t recent_color_count;
    uint8_t color_hue;
    uint8_t color_saturation;
    uint8_t color_value;
    uint32_t layer_opacity_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t layer_opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t recent_color_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3];
    uint8_t color_palette_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3];
} DrawingProgramUiSettingsV8;

enum {
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V1 = 1u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V2 = 2u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V3 = 3u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V4 = 4u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V5 = 5u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V6 = 6u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V7 = 7u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V8 = 8u
};

static void drawing_program_snapshot_apply_selection_payload(
    DrawingProgramAppContext *ctx,
    uint8_t has_payload,
    uint32_t origin_x,
    uint32_t origin_y,
    uint32_t width,
    uint32_t height) {
    if (has_payload) {
        (void)drawing_program_selection_capture_from_rect(&ctx->document,
                                                          &ctx->layer_rasters,
                                                          ctx->editor.active_layer_id,
                                                          &ctx->selection,
                                                          (int32_t)origin_x,
                                                          (int32_t)origin_y,
                                                          width,
                                                          height);
        return;
    }
    drawing_program_selection_reset(&ctx->selection);
}

static void drawing_program_snapshot_derive_active_selector_hsv(const DrawingProgramAppContext *ctx,
                                                                uint8_t *out_hue,
                                                                uint8_t *out_saturation,
                                                                uint8_t *out_value) {
    uint8_t active_index;
    uint8_t r = 0u;
    uint8_t g = 0u;
    uint8_t b = 0u;
    if (!ctx) {
        return;
    }
    active_index = drawing_program_color_index_clamp(ctx->ui.active_color_index);
    r = ctx->ui.color_palette_rgb[active_index][0];
    g = ctx->ui.color_palette_rgb[active_index][1];
    b = ctx->ui.color_palette_rgb[active_index][2];
    drawing_program_color_rgb_to_hsv(r, g, b, out_hue, out_saturation, out_value);
}

CoreResult drawing_program_snapshot_write_ui_settings_chunk(
    CorePackWriter *writer,
    const struct DrawingProgramAppContext *ctx) {
    DrawingProgramUiSettingsV8 ui_settings;
    if (!writer || !ctx) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid snapshot ui settings write request" };
    }
    memset(&ui_settings, 0, sizeof(ui_settings));
    ui_settings.version = DRAWING_PROGRAM_UI_SETTINGS_VERSION_V8;
    ui_settings.theme_preset_id = ctx->ui.theme_preset_id;
    ui_settings.font_preset_id = ctx->ui.font_preset_id;
    ui_settings.font_zoom_step = (int32_t)ctx->ui.font_zoom_step;
    ui_settings.left_panel_slot = ctx->ui.left_panel_slot;
    ui_settings.right_panel_slot = ctx->ui.right_panel_slot;
    ui_settings.active_color_index = ctx->ui.active_color_index;
    ui_settings.selection_has_payload =
        (ctx->selection.has_payload && ctx->selection.width > 0u && ctx->selection.height > 0u) ? 1u : 0u;
    ui_settings.selection_origin_x = ctx->selection.origin_x;
    ui_settings.selection_origin_y = ctx->selection.origin_y;
    ui_settings.selection_width = ctx->selection.width;
    ui_settings.selection_height = ctx->selection.height;
    ui_settings.tool_brush_size = ctx->ui.tool_brush_size;
    ui_settings.tool_brush_opacity = ctx->ui.tool_brush_opacity;
    ui_settings.tool_brush_spacing = ctx->ui.tool_brush_spacing;
    ui_settings.tool_brush_hardness = ctx->ui.tool_brush_hardness;
    ui_settings.tool_eraser_size = ctx->ui.tool_eraser_size;
    ui_settings.tool_shape_stroke_width = ctx->ui.tool_shape_stroke_width;
    ui_settings.tool_shape_mode = ctx->ui.tool_shape_mode;
    ui_settings.tool_shape_target_mode = ctx->ui.tool_shape_target_mode;
    ui_settings.tool_fill_tolerance = ctx->ui.tool_fill_tolerance;
    ui_settings.tool_select_mode = ctx->ui.tool_select_mode;
    ui_settings.layer_opacity_entry_count = ctx->ui.layer_opacity_entry_count;
    ui_settings.recent_color_count = ctx->ui.recent_color_count;
    drawing_program_snapshot_derive_active_selector_hsv(
        ctx, &ui_settings.color_hue, &ui_settings.color_saturation, &ui_settings.color_value);
    if (ui_settings.layer_opacity_entry_count > DRAWING_PROGRAM_MAX_LAYERS) {
        ui_settings.layer_opacity_entry_count = DRAWING_PROGRAM_MAX_LAYERS;
    }
    memcpy(ui_settings.layer_opacity_layer_ids,
           ctx->ui.layer_opacity_layer_ids,
           sizeof(ui_settings.layer_opacity_layer_ids));
    memcpy(ui_settings.layer_opacity_values,
           ctx->ui.layer_opacity_values,
           sizeof(ui_settings.layer_opacity_values));
    memcpy(ui_settings.recent_color_rgb,
           ctx->ui.recent_color_rgb,
           sizeof(ui_settings.recent_color_rgb));
    memcpy(ui_settings.color_palette_rgb,
           ctx->ui.color_palette_rgb,
           sizeof(ui_settings.color_palette_rgb));
    return core_pack_writer_add_chunk(writer, "DPUI", &ui_settings, (uint64_t)sizeof(ui_settings));
}

CoreResult drawing_program_snapshot_apply_ui_settings_chunk(
    struct DrawingProgramAppContext *ctx,
    CorePackReader *reader,
    const CorePackChunkInfo *chunk) {
    DrawingProgramUiSettingsV1 ui_settings_v1;
    DrawingProgramUiSettingsV2 ui_settings_v2;
    DrawingProgramUiSettingsV3 ui_settings_v3;
    DrawingProgramUiSettingsV4 ui_settings_v4;
    DrawingProgramUiSettingsV5 ui_settings_v5;
    DrawingProgramUiSettingsV6 ui_settings_v6;
    DrawingProgramUiSettingsV7 ui_settings_v7;
    DrawingProgramUiSettingsV8 ui_settings_v8;
    CoreResult result;
    if (!ctx || !reader || !chunk) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid snapshot ui settings apply request" };
    }

    memset(&ui_settings_v1, 0, sizeof(ui_settings_v1));
    memset(&ui_settings_v2, 0, sizeof(ui_settings_v2));
    memset(&ui_settings_v3, 0, sizeof(ui_settings_v3));
    memset(&ui_settings_v4, 0, sizeof(ui_settings_v4));
    memset(&ui_settings_v5, 0, sizeof(ui_settings_v5));
    memset(&ui_settings_v6, 0, sizeof(ui_settings_v6));
    memset(&ui_settings_v7, 0, sizeof(ui_settings_v7));
    memset(&ui_settings_v8, 0, sizeof(ui_settings_v8));

    if (chunk->size == (uint64_t)sizeof(ui_settings_v8)) {
        result = core_pack_reader_read_chunk_data(reader, chunk, &ui_settings_v8, (uint64_t)sizeof(ui_settings_v8));
        if (result.code == CORE_OK && ui_settings_v8.version == DRAWING_PROGRAM_UI_SETTINGS_VERSION_V8) {
            uint8_t entry_count = ui_settings_v8.layer_opacity_entry_count;
            uint8_t recent_count = ui_settings_v8.recent_color_count;
            if (ui_settings_v8.theme_preset_id < (uint32_t)CORE_THEME_PRESET_COUNT) {
                ctx->ui.theme_preset_id = ui_settings_v8.theme_preset_id;
            }
            if (ui_settings_v8.font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
                ctx->ui.font_preset_id = ui_settings_v8.font_preset_id;
            }
            ctx->ui.font_zoom_step = (int8_t)ui_settings_v8.font_zoom_step;
            ctx->ui.left_panel_slot = ui_settings_v8.left_panel_slot;
            ctx->ui.right_panel_slot = ui_settings_v8.right_panel_slot;
            ctx->ui.active_color_index = ui_settings_v8.active_color_index;
            ctx->ui.tool_brush_size = ui_settings_v8.tool_brush_size;
            ctx->ui.tool_brush_opacity = ui_settings_v8.tool_brush_opacity;
            ctx->ui.tool_brush_spacing = ui_settings_v8.tool_brush_spacing;
            ctx->ui.tool_brush_hardness = ui_settings_v8.tool_brush_hardness;
            ctx->ui.tool_eraser_size = ui_settings_v8.tool_eraser_size;
            ctx->ui.tool_shape_stroke_width = ui_settings_v8.tool_shape_stroke_width;
            ctx->ui.tool_shape_mode = ui_settings_v8.tool_shape_mode;
            ctx->ui.tool_shape_target_mode = ui_settings_v8.tool_shape_target_mode;
            ctx->ui.tool_fill_tolerance = ui_settings_v8.tool_fill_tolerance;
            ctx->ui.tool_select_mode = ui_settings_v8.tool_select_mode;
            ctx->ui.color_hue = ui_settings_v8.color_hue;
            ctx->ui.color_saturation = ui_settings_v8.color_saturation;
            ctx->ui.color_value = ui_settings_v8.color_value;
            if (entry_count > DRAWING_PROGRAM_MAX_LAYERS) {
                entry_count = DRAWING_PROGRAM_MAX_LAYERS;
            }
            if (recent_count > DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT) {
                recent_count = DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT;
            }
            ctx->ui.layer_opacity_entry_count = entry_count;
            ctx->ui.recent_color_count = recent_count;
            memcpy(ctx->ui.layer_opacity_layer_ids,
                   ui_settings_v8.layer_opacity_layer_ids,
                   sizeof(ctx->ui.layer_opacity_layer_ids));
            memcpy(ctx->ui.layer_opacity_values,
                   ui_settings_v8.layer_opacity_values,
                   sizeof(ctx->ui.layer_opacity_values));
            memcpy(ctx->ui.recent_color_rgb,
                   ui_settings_v8.recent_color_rgb,
                   sizeof(ctx->ui.recent_color_rgb));
            memcpy(ctx->ui.color_palette_rgb,
                   ui_settings_v8.color_palette_rgb,
                   sizeof(ctx->ui.color_palette_rgb));
            drawing_program_snapshot_apply_selection_payload(ctx,
                                                             ui_settings_v8.selection_has_payload,
                                                             ui_settings_v8.selection_origin_x,
                                                             ui_settings_v8.selection_origin_y,
                                                             ui_settings_v8.selection_width,
                                                             ui_settings_v8.selection_height);
        }
        return result;
    }

    if (chunk->size == (uint64_t)sizeof(ui_settings_v7)) {
        result = core_pack_reader_read_chunk_data(reader, chunk, &ui_settings_v7, (uint64_t)sizeof(ui_settings_v7));
        if (result.code == CORE_OK && ui_settings_v7.version == DRAWING_PROGRAM_UI_SETTINGS_VERSION_V7) {
            uint8_t entry_count = ui_settings_v7.layer_opacity_entry_count;
            if (ui_settings_v7.theme_preset_id < (uint32_t)CORE_THEME_PRESET_COUNT) {
                ctx->ui.theme_preset_id = ui_settings_v7.theme_preset_id;
            }
            if (ui_settings_v7.font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
                ctx->ui.font_preset_id = ui_settings_v7.font_preset_id;
            }
            ctx->ui.font_zoom_step = (int8_t)ui_settings_v7.font_zoom_step;
            ctx->ui.left_panel_slot = ui_settings_v7.left_panel_slot;
            ctx->ui.right_panel_slot = ui_settings_v7.right_panel_slot;
            ctx->ui.active_color_index = ui_settings_v7.active_color_index;
            ctx->ui.tool_brush_size = ui_settings_v7.tool_brush_size;
            ctx->ui.tool_brush_opacity = ui_settings_v7.tool_brush_opacity;
            ctx->ui.tool_brush_spacing = ui_settings_v7.tool_brush_spacing;
            ctx->ui.tool_brush_hardness = ui_settings_v7.tool_brush_hardness;
            ctx->ui.tool_eraser_size = ui_settings_v7.tool_eraser_size;
            ctx->ui.tool_shape_stroke_width = ui_settings_v7.tool_shape_stroke_width;
            ctx->ui.tool_shape_mode = ui_settings_v7.tool_shape_mode;
            ctx->ui.tool_shape_target_mode = ui_settings_v7.tool_shape_target_mode;
            ctx->ui.tool_fill_tolerance = ui_settings_v7.tool_fill_tolerance;
            ctx->ui.tool_select_mode = ui_settings_v7.tool_select_mode;
            if (entry_count > DRAWING_PROGRAM_MAX_LAYERS) {
                entry_count = DRAWING_PROGRAM_MAX_LAYERS;
            }
            ctx->ui.layer_opacity_entry_count = entry_count;
            memcpy(ctx->ui.layer_opacity_layer_ids,
                   ui_settings_v7.layer_opacity_layer_ids,
                   sizeof(ctx->ui.layer_opacity_layer_ids));
            memcpy(ctx->ui.layer_opacity_values,
                   ui_settings_v7.layer_opacity_values,
                   sizeof(ctx->ui.layer_opacity_values));
            drawing_program_snapshot_apply_selection_payload(ctx,
                                                             ui_settings_v7.selection_has_payload,
                                                             ui_settings_v7.selection_origin_x,
                                                             ui_settings_v7.selection_origin_y,
                                                             ui_settings_v7.selection_width,
                                                             ui_settings_v7.selection_height);
        }
        return result;
    }
    if (chunk->size == (uint64_t)sizeof(ui_settings_v6)) {
        result = core_pack_reader_read_chunk_data(reader, chunk, &ui_settings_v6, (uint64_t)sizeof(ui_settings_v6));
        if (result.code == CORE_OK && ui_settings_v6.version == DRAWING_PROGRAM_UI_SETTINGS_VERSION_V6) {
            uint8_t entry_count = ui_settings_v6.layer_opacity_entry_count;
            if (ui_settings_v6.theme_preset_id < (uint32_t)CORE_THEME_PRESET_COUNT) {
                ctx->ui.theme_preset_id = ui_settings_v6.theme_preset_id;
            }
            if (ui_settings_v6.font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
                ctx->ui.font_preset_id = ui_settings_v6.font_preset_id;
            }
            ctx->ui.font_zoom_step = (int8_t)ui_settings_v6.font_zoom_step;
            ctx->ui.left_panel_slot = ui_settings_v6.left_panel_slot;
            ctx->ui.right_panel_slot = ui_settings_v6.right_panel_slot;
            ctx->ui.active_color_index = ui_settings_v6.active_color_index;
            ctx->ui.tool_brush_size = ui_settings_v6.tool_brush_size;
            ctx->ui.tool_brush_opacity = ui_settings_v6.tool_brush_opacity;
            ctx->ui.tool_brush_spacing = ui_settings_v6.tool_brush_spacing;
            ctx->ui.tool_brush_hardness = ui_settings_v6.tool_brush_hardness;
            ctx->ui.tool_eraser_size = ui_settings_v6.tool_eraser_size;
            ctx->ui.tool_shape_stroke_width = ui_settings_v6.tool_shape_stroke_width;
            ctx->ui.tool_shape_mode = ui_settings_v6.tool_shape_mode;
            ctx->ui.tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
            ctx->ui.tool_fill_tolerance = ui_settings_v6.tool_fill_tolerance;
            ctx->ui.tool_select_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
            if (entry_count > DRAWING_PROGRAM_MAX_LAYERS) {
                entry_count = DRAWING_PROGRAM_MAX_LAYERS;
            }
            ctx->ui.layer_opacity_entry_count = entry_count;
            memcpy(ctx->ui.layer_opacity_layer_ids,
                   ui_settings_v6.layer_opacity_layer_ids,
                   sizeof(ctx->ui.layer_opacity_layer_ids));
            memcpy(ctx->ui.layer_opacity_values,
                   ui_settings_v6.layer_opacity_values,
                   sizeof(ctx->ui.layer_opacity_values));
            drawing_program_snapshot_apply_selection_payload(ctx,
                                                             ui_settings_v6.selection_has_payload,
                                                             ui_settings_v6.selection_origin_x,
                                                             ui_settings_v6.selection_origin_y,
                                                             ui_settings_v6.selection_width,
                                                             ui_settings_v6.selection_height);
        }
        return result;
    }
    if (chunk->size == (uint64_t)sizeof(ui_settings_v5)) {
        result = core_pack_reader_read_chunk_data(reader, chunk, &ui_settings_v5, (uint64_t)sizeof(ui_settings_v5));
        if (result.code == CORE_OK && ui_settings_v5.version == DRAWING_PROGRAM_UI_SETTINGS_VERSION_V5) {
            if (ui_settings_v5.theme_preset_id < (uint32_t)CORE_THEME_PRESET_COUNT) {
                ctx->ui.theme_preset_id = ui_settings_v5.theme_preset_id;
            }
            if (ui_settings_v5.font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
                ctx->ui.font_preset_id = ui_settings_v5.font_preset_id;
            }
            ctx->ui.font_zoom_step = (int8_t)ui_settings_v5.font_zoom_step;
            ctx->ui.left_panel_slot = ui_settings_v5.left_panel_slot;
            ctx->ui.right_panel_slot = ui_settings_v5.right_panel_slot;
            ctx->ui.active_color_index = ui_settings_v5.active_color_index;
            ctx->ui.tool_brush_size = ui_settings_v5.tool_brush_size;
            ctx->ui.tool_brush_opacity = ui_settings_v5.tool_brush_opacity;
            ctx->ui.tool_brush_spacing = 2u;
            ctx->ui.tool_brush_hardness = 100u;
            ctx->ui.tool_eraser_size = ui_settings_v5.tool_eraser_size;
            ctx->ui.tool_shape_stroke_width = ui_settings_v5.tool_shape_stroke_width;
            ctx->ui.tool_shape_mode = ui_settings_v5.tool_shape_mode;
            ctx->ui.tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
            ctx->ui.tool_fill_tolerance = ui_settings_v5.tool_fill_tolerance;
            ctx->ui.tool_select_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
            drawing_program_snapshot_apply_selection_payload(ctx,
                                                             ui_settings_v5.selection_has_payload,
                                                             ui_settings_v5.selection_origin_x,
                                                             ui_settings_v5.selection_origin_y,
                                                             ui_settings_v5.selection_width,
                                                             ui_settings_v5.selection_height);
        }
        return result;
    }
    if (chunk->size == (uint64_t)sizeof(ui_settings_v4)) {
        result = core_pack_reader_read_chunk_data(reader, chunk, &ui_settings_v4, (uint64_t)sizeof(ui_settings_v4));
        if (result.code == CORE_OK && ui_settings_v4.version == DRAWING_PROGRAM_UI_SETTINGS_VERSION_V4) {
            if (ui_settings_v4.theme_preset_id < (uint32_t)CORE_THEME_PRESET_COUNT) {
                ctx->ui.theme_preset_id = ui_settings_v4.theme_preset_id;
            }
            if (ui_settings_v4.font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
                ctx->ui.font_preset_id = ui_settings_v4.font_preset_id;
            }
            ctx->ui.font_zoom_step = (int8_t)ui_settings_v4.font_zoom_step;
            ctx->ui.left_panel_slot = ui_settings_v4.left_panel_slot;
            ctx->ui.right_panel_slot = ui_settings_v4.right_panel_slot;
            ctx->ui.active_color_index = ui_settings_v4.active_color_index;
            ctx->ui.tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
            ctx->ui.tool_select_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
            drawing_program_snapshot_apply_selection_payload(ctx,
                                                             ui_settings_v4.selection_has_payload,
                                                             ui_settings_v4.selection_origin_x,
                                                             ui_settings_v4.selection_origin_y,
                                                             ui_settings_v4.selection_width,
                                                             ui_settings_v4.selection_height);
        }
        return result;
    }
    if (chunk->size == (uint64_t)sizeof(ui_settings_v3)) {
        result = core_pack_reader_read_chunk_data(reader, chunk, &ui_settings_v3, (uint64_t)sizeof(ui_settings_v3));
        if (result.code == CORE_OK && ui_settings_v3.version == DRAWING_PROGRAM_UI_SETTINGS_VERSION_V3) {
            if (ui_settings_v3.theme_preset_id < (uint32_t)CORE_THEME_PRESET_COUNT) {
                ctx->ui.theme_preset_id = ui_settings_v3.theme_preset_id;
            }
            if (ui_settings_v3.font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
                ctx->ui.font_preset_id = ui_settings_v3.font_preset_id;
            }
            ctx->ui.font_zoom_step = (int8_t)ui_settings_v3.font_zoom_step;
            ctx->ui.left_panel_slot = ui_settings_v3.left_panel_slot;
            ctx->ui.right_panel_slot = ui_settings_v3.right_panel_slot;
            ctx->ui.active_color_index = ui_settings_v3.active_color_index;
            ctx->ui.tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
            ctx->ui.tool_select_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
            drawing_program_selection_reset(&ctx->selection);
        }
        return result;
    }
    if (chunk->size == (uint64_t)sizeof(ui_settings_v2)) {
        result = core_pack_reader_read_chunk_data(reader, chunk, &ui_settings_v2, (uint64_t)sizeof(ui_settings_v2));
        if (result.code == CORE_OK && ui_settings_v2.version == DRAWING_PROGRAM_UI_SETTINGS_VERSION_V2) {
            if (ui_settings_v2.theme_preset_id < (uint32_t)CORE_THEME_PRESET_COUNT) {
                ctx->ui.theme_preset_id = ui_settings_v2.theme_preset_id;
            }
            if (ui_settings_v2.font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
                ctx->ui.font_preset_id = ui_settings_v2.font_preset_id;
            }
            ctx->ui.font_zoom_step = (int8_t)ui_settings_v2.font_zoom_step;
            ctx->ui.left_panel_slot = ui_settings_v2.left_panel_slot;
            ctx->ui.right_panel_slot = ui_settings_v2.right_panel_slot;
            ctx->ui.active_color_index = drawing_program_color_default_index();
            ctx->ui.tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
            ctx->ui.tool_select_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
            drawing_program_selection_reset(&ctx->selection);
        }
        return result;
    }
    if (chunk->size == (uint64_t)sizeof(ui_settings_v1)) {
        result = core_pack_reader_read_chunk_data(reader, chunk, &ui_settings_v1, (uint64_t)sizeof(ui_settings_v1));
        if (result.code == CORE_OK && ui_settings_v1.version == DRAWING_PROGRAM_UI_SETTINGS_VERSION_V1) {
            if (ui_settings_v1.theme_preset_id < (uint32_t)CORE_THEME_PRESET_COUNT) {
                ctx->ui.theme_preset_id = ui_settings_v1.theme_preset_id;
            }
            ctx->ui.left_panel_slot = ui_settings_v1.left_panel_slot;
            ctx->ui.right_panel_slot = ui_settings_v1.right_panel_slot;
            ctx->ui.active_color_index = drawing_program_color_default_index();
            ctx->ui.tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
            ctx->ui.tool_select_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
            drawing_program_selection_reset(&ctx->selection);
        }
        return result;
    }
    return core_result_ok();
}
