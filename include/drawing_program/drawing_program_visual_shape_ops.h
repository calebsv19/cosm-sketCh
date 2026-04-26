#ifndef DRAWING_PROGRAM_VISUAL_SHAPE_OPS_H
#define DRAWING_PROGRAM_VISUAL_SHAPE_OPS_H

#include "drawing_program/drawing_program_app_main.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t drawing_program_visual_clamp_setting_u8(uint8_t value, uint8_t min_v, uint8_t max_v);
DrawingProgramRasterSample drawing_program_visual_sample_value_for_tool(const DrawingProgramAppContext *ctx,
                                                                       DrawingProgramToolKind tool);
uint32_t drawing_program_visual_tool_brush_radius_samples(const DrawingProgramAppContext *ctx, DrawingProgramToolKind tool);
uint32_t drawing_program_visual_tool_brush_spacing_samples(const DrawingProgramAppContext *ctx,
                                                           DrawingProgramToolKind tool,
                                                           uint32_t radius);
uint8_t drawing_program_visual_tool_brush_hardness_percent(const DrawingProgramAppContext *ctx,
                                                           DrawingProgramToolKind tool);
int drawing_program_visual_tool_uses_direct_sample_stroke(DrawingProgramToolKind tool);
int drawing_program_visual_tool_uses_shape_commit(DrawingProgramToolKind tool);
uint8_t drawing_program_visual_tool_shape_mode(const DrawingProgramAppContext *ctx);
uint32_t drawing_program_visual_tool_shape_stroke_width(const DrawingProgramAppContext *ctx);
uint8_t drawing_program_visual_tool_fill_tolerance_setting(const DrawingProgramAppContext *ctx);
uint8_t drawing_program_visual_fill_tolerance_sample_delta(uint8_t tolerance_setting);
int drawing_program_visual_fill_sample_matches_tolerance(DrawingProgramRasterSample sample,
                                                         DrawingProgramRasterSample target,
                                                         uint8_t tolerance_setting);
const char *drawing_program_visual_shape_mode_name(uint8_t mode);
int drawing_program_visual_shape_mode_includes_fill(DrawingProgramToolKind tool, uint8_t mode);
int drawing_program_visual_shape_mode_includes_outline(DrawingProgramToolKind tool, uint8_t mode);
DrawingProgramRasterSample drawing_program_visual_seeded_background_sample_for_coord(
    const DrawingProgramDocument *document,
    uint32_t x,
    uint32_t y);
CoreResult drawing_program_visual_apply_canvas_shape_commit(DrawingProgramAppContext *ctx,
                                                            DrawingProgramToolKind tool,
                                                            uint32_t start_x,
                                                            uint32_t start_y,
                                                            uint32_t end_x,
                                                            uint32_t end_y);

#ifdef __cplusplus
}
#endif

#endif
