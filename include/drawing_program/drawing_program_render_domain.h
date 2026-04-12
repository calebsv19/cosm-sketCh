#ifndef DRAWING_PROGRAM_RENDER_DOMAIN_H
#define DRAWING_PROGRAM_RENDER_DOMAIN_H

#include <stdint.h>

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramDocument;
struct DrawingProgramEditorState;
struct DrawingProgramLayerRasterStore;

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
    uint32_t raster_sample_count;
    uint32_t raster_nonzero_count;
    uint32_t raster_hash32;
    uint8_t has_active_layer;
    uint8_t full_redraw;
    uint8_t targeted_redraw;
    uint32_t invalidation_reason_bits;
} DrawingProgramRenderFrameProjection;

CoreResult drawing_program_render_project_frame(
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const struct DrawingProgramEditorState *editor,
    const DrawingProgramRenderInvalidation *invalidation,
    DrawingProgramRenderFrameProjection *out_projection);

CoreResult drawing_program_render_compose_visible_samples(
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    uint8_t *out_samples,
    uint32_t out_capacity);

CoreResult drawing_program_render_compose_visible_samples_with_layer_opacity(
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    uint8_t *out_samples,
    uint32_t out_capacity);

#ifdef __cplusplus
}
#endif

#endif
