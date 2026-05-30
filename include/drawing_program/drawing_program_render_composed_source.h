#ifndef DRAWING_PROGRAM_RENDER_COMPOSED_SOURCE_H
#define DRAWING_PROGRAM_RENDER_COMPOSED_SOURCE_H

#include <stdint.h>

#include "core_base.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_document.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramDocument;
struct DrawingProgramLayerRasterStore;

typedef struct DrawingProgramRenderComposedSourceState {
    DrawingProgramRasterSample *composited_samples;
    DrawingProgramRasterSample *resolved_snapshot_samples;
    DrawingProgramRasterSample *lower_stack_samples;
    DrawingProgramRasterSample *previous_prefix_stack_samples;
    DrawingProgramRasterSample *prefix_stack_samples;
    DrawingProgramRasterSample *suffix_stack_samples;
    uint32_t composited_capacity;
    uint64_t cached_content_key;
    uint64_t cached_opacity_key;
    uint32_t cached_sample_count;
    uint32_t snapshot_width;
    uint32_t snapshot_height;
    uint64_t cached_lower_stack_key;
    uint64_t cached_prefix_stack_key;
    uint64_t cached_suffix_stack_key;
    uint64_t cached_lower_stack_layer_revisions[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t cached_lower_stack_sample_count;
    uint32_t cached_prefix_stack_sample_count;
    uint32_t cached_suffix_stack_sample_count;
    uint8_t has_cached_composed_samples;
    uint8_t has_resolved_snapshot;
    uint8_t has_cached_lower_stack;
    uint8_t has_cached_prefix_stack;
    uint8_t has_cached_suffix_stack;
} DrawingProgramRenderComposedSourceState;

typedef struct DrawingProgramRenderComposedSourceView {
    const DrawingProgramRasterSample *samples;
    uint32_t sample_count;
    uint8_t used_composed_storage;
    uint8_t reused_cached_compose;
    uint8_t reused_cached_lower_stack;
    uint8_t reused_cached_prefix_stack;
    uint8_t reused_cached_suffix_stack;
    uint8_t recomposed_dirty_rect_only;
    uint8_t rebuilt_lower_stack_full_raster;
    uint8_t recomposed_top_range_full_raster;
    uint8_t has_dirty_rect;
    uint8_t dirty_rect_is_full;
    uint32_t dirty_x;
    uint32_t dirty_y;
    uint32_t dirty_width;
    uint32_t dirty_height;
} DrawingProgramRenderComposedSourceView;

void drawing_program_render_composed_source_dispose(
    DrawingProgramRenderComposedSourceState *state);

CoreResult drawing_program_render_composed_source_resolve(
    DrawingProgramRenderComposedSourceState *state,
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    uint64_t content_key,
    uint64_t opacity_key,
    DrawingProgramRenderComposedSourceView *out_view);

#ifdef __cplusplus
}
#endif

#endif
