#ifndef DRAWING_PROGRAM_SNAPSHOT_UI_SETTINGS_INTERNAL_H
#define DRAWING_PROGRAM_SNAPSHOT_UI_SETTINGS_INTERNAL_H

#include "drawing_program/drawing_program_app_main.h"

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

typedef struct DrawingProgramUiSettingsV9 {
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
    uint8_t active_paint_r;
    uint8_t active_paint_g;
    uint8_t active_paint_b;
    uint8_t reserved0;
    uint32_t layer_opacity_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t layer_opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t recent_color_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3];
    uint8_t color_palette_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3];
} DrawingProgramUiSettingsV9;

typedef struct DrawingProgramUiSettingsV10 {
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
    uint8_t selected_recent_color_index;
    uint8_t color_hue;
    uint8_t color_saturation;
    uint8_t color_value;
    uint8_t active_paint_r;
    uint8_t active_paint_g;
    uint8_t active_paint_b;
    uint8_t reserved0;
    uint32_t layer_opacity_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t layer_opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t recent_color_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3];
    uint8_t color_palette_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3];
} DrawingProgramUiSettingsV10;

typedef struct DrawingProgramUiSettingsV11 {
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
    uint8_t canvas_control_mode;
    uint8_t layer_opacity_entry_count;
    uint8_t recent_color_count;
    uint8_t selected_recent_color_index;
    uint8_t color_hue;
    uint8_t color_saturation;
    uint8_t color_value;
    uint8_t active_paint_r;
    uint8_t active_paint_g;
    uint8_t active_paint_b;
    uint8_t reserved0;
    uint32_t layer_opacity_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t layer_opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t recent_color_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3];
    uint8_t color_palette_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3];
} DrawingProgramUiSettingsV11;

typedef struct DrawingProgramUiSettingsV12 {
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
    uint8_t canvas_control_mode;
    uint8_t canvas_guide_mode;
    uint8_t layer_opacity_entry_count;
    uint8_t recent_color_count;
    uint8_t selected_recent_color_index;
    uint8_t color_hue;
    uint8_t color_saturation;
    uint8_t color_value;
    uint8_t active_paint_r;
    uint8_t active_paint_g;
    uint8_t active_paint_b;
    uint8_t reserved0;
    uint32_t layer_opacity_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t layer_opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t recent_color_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3];
    uint8_t color_palette_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3];
} DrawingProgramUiSettingsV12;

typedef struct DrawingProgramUiSettingsV13 {
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
    uint8_t canvas_control_mode;
    uint8_t canvas_guide_mode;
    uint8_t canvas_reflection_center_valid;
    uint8_t layer_opacity_entry_count;
    uint8_t recent_color_count;
    uint8_t selected_recent_color_index;
    uint8_t color_hue;
    uint8_t color_saturation;
    uint8_t color_value;
    uint8_t active_paint_r;
    uint8_t active_paint_g;
    uint8_t active_paint_b;
    uint8_t reserved0;
    uint32_t canvas_reflection_center_x;
    uint32_t canvas_reflection_center_y;
    uint32_t layer_opacity_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t layer_opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t recent_color_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3];
    uint8_t color_palette_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3];
} DrawingProgramUiSettingsV13;

typedef struct DrawingProgramUiSettingsV14 {
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
    uint8_t canvas_control_mode;
    uint8_t canvas_guide_mode;
    uint8_t canvas_reflection_center_valid;
    uint8_t layer_opacity_entry_count;
    uint8_t recent_color_count;
    uint8_t selected_recent_color_index;
    uint8_t color_hue;
    uint8_t color_saturation;
    uint8_t color_value;
    uint8_t active_paint_r;
    uint8_t active_paint_g;
    uint8_t active_paint_b;
    uint8_t reserved0;
    uint32_t canvas_reflection_center_x;
    uint32_t canvas_reflection_center_y;
    DrawingProgramReflectionState reflection_state;
    uint32_t layer_opacity_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t layer_opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t recent_color_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3];
    uint8_t color_palette_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3];
} DrawingProgramUiSettingsV14;

enum {
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V1 = 1u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V2 = 2u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V3 = 3u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V4 = 4u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V5 = 5u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V6 = 6u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V7 = 7u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V8 = 8u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V9 = 9u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V10 = 10u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V11 = 11u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V12 = 12u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V13 = 13u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V14 = 14u
};

#endif
