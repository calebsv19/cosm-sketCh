#ifndef DRAWING_PROGRAM_TEXTURE_CANVAS_OPS_H
#define DRAWING_PROGRAM_TEXTURE_CANVAS_OPS_H

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

CoreResult drawing_program_texture_canvas_add_blank_from_active(
    struct DrawingProgramAppContext *ctx,
    uint32_t *out_surface_index);
CoreResult drawing_program_texture_canvas_duplicate_active(
    struct DrawingProgramAppContext *ctx,
    uint32_t *out_surface_index);
CoreResult drawing_program_texture_canvas_delete_active(
    struct DrawingProgramAppContext *ctx,
    uint32_t *out_surface_index);
CoreResult drawing_program_texture_canvas_reset_object_layout(
    struct DrawingProgramAppContext *ctx);
void drawing_program_texture_canvas_toggle_control_mode(
    struct DrawingProgramAppContext *ctx);
void drawing_program_texture_canvas_cycle_guide_mode(
    struct DrawingProgramAppContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
