#ifndef DRAWING_PROGRAM_RENDER_DOMAIN_H
#define DRAWING_PROGRAM_RENDER_DOMAIN_H

#include <stdint.h>

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramDocument;
struct DrawingProgramEditorState;

typedef struct DrawingProgramRenderInvalidation {
    uint8_t full_invalidate;
    uint32_t invalidation_reason_bits;
    uint32_t target_invalidation_count;
    uint32_t full_invalidation_count;
} DrawingProgramRenderInvalidation;

typedef struct DrawingProgramRenderFrameProjection {
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    uint32_t layer_count;
    uint32_t visible_layer_count;
    uint32_t hidden_layer_count;
    uint32_t active_layer_id;
    uint8_t has_active_layer;
    uint8_t full_redraw;
    uint8_t targeted_redraw;
    uint32_t invalidation_reason_bits;
} DrawingProgramRenderFrameProjection;

CoreResult drawing_program_render_project_frame(
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramEditorState *editor,
    const DrawingProgramRenderInvalidation *invalidation,
    DrawingProgramRenderFrameProjection *out_projection);

#ifdef __cplusplus
}
#endif

#endif
