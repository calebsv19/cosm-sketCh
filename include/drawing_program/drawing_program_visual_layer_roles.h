#ifndef DRAWING_PROGRAM_VISUAL_LAYER_ROLES_H
#define DRAWING_PROGRAM_VISUAL_LAYER_ROLES_H

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_texture_layer_role.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DrawingProgramVisualLayerRolePreset {
    DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_BASE = 0,
    DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_MATERIAL_DETAIL,
    DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DECAL,
    DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_GRIME,
    DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DAMAGE,
    DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_COUNT,
    DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_CUSTOM = 255
} DrawingProgramVisualLayerRolePreset;

const char *drawing_program_visual_layer_role_name(DrawingProgramVisualLayerRolePreset role);
const char *drawing_program_visual_layer_role_button_label(DrawingProgramVisualLayerRolePreset role);
const char *drawing_program_visual_layer_role_lane_label(DrawingProgramVisualLayerRolePreset role);
int drawing_program_visual_layer_role_prefers_overlay(DrawingProgramVisualLayerRolePreset role);
DrawingProgramVisualLayerRolePreset drawing_program_visual_layer_role_detect_name(const char *name);
DrawingProgramVisualLayerRolePreset drawing_program_visual_layer_role_detect_active(
    const DrawingProgramAppContext *ctx);
void drawing_program_visual_apply_layer_role_preset_active(DrawingProgramAppContext *ctx,
                                                           DrawingProgramVisualLayerRolePreset role);
void drawing_program_visual_apply_layer_role_suggest_for_active_if_generic(DrawingProgramAppContext *ctx);
uint32_t drawing_program_visual_layer_role_detect_for_layer_id(const DrawingProgramAppContext *ctx,
                                                               uint32_t layer_id);

#ifdef __cplusplus
}
#endif

#endif
