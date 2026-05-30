#ifndef DRAWING_PROGRAM_RENDER_COMPOSED_SOURCE_INTERNAL_H
#define DRAWING_PROGRAM_RENDER_COMPOSED_SOURCE_INTERNAL_H

#include <stdint.h>

#include "core_base.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_render_composed_source.h"

struct DrawingProgramLayerRasterStore;

typedef struct DrawingProgramComposedSourceEffectiveLayerInfo {
    uint32_t document_layer_index;
    uint32_t layer_id;
    uint64_t content_revision;
    const DrawingProgramRasterSample *samples;
    uint8_t opacity;
} DrawingProgramComposedSourceEffectiveLayerInfo;

typedef struct DrawingProgramRenderComposedSourcePlan {
    DrawingProgramComposedSourceEffectiveLayerInfo effective_layers[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t effective_count;
    uint32_t partial_suffix_start_index;
    uint32_t partial_suffix_count;
    uint64_t lower_stack_key;
    uint64_t prefix_stack_key;
    uint64_t suffix_stack_key;
    uint8_t has_partial_suffix;
    uint8_t supports_prefix_stack_cache;
    uint8_t supports_suffix_stack_cache;
} DrawingProgramRenderComposedSourcePlan;

CoreResult drawing_program_render_composed_source_plan_build(
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    DrawingProgramRenderComposedSourcePlan *out_plan);

CoreResult render_composed_source_invalid(const char *message);
void render_composed_source_mark_full_dirty(
    const struct DrawingProgramDocument *document,
    DrawingProgramRenderComposedSourceView *out_view);
uint8_t render_composed_source_diff_dirty_rect(
    const struct DrawingProgramDocument *document,
    const DrawingProgramRasterSample *before_samples,
    const DrawingProgramRasterSample *after_samples,
    uint32_t *out_x,
    uint32_t *out_y,
    uint32_t *out_width,
    uint32_t *out_height);
void render_composed_source_refresh_snapshot(
    DrawingProgramRenderComposedSourceState *state,
    const struct DrawingProgramDocument *document,
    const DrawingProgramRasterSample *samples,
    DrawingProgramRenderComposedSourceView *out_view);

#endif
