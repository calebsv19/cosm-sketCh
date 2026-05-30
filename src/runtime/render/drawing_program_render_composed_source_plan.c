#include "drawing_program_render_composed_source_internal.h"

#include <string.h>

#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_layer_raster.h"

static uint64_t render_composed_source_plan_mix_key(uint64_t state, uint64_t value) {
    state ^= value + 0x9e3779b97f4a7c15ull + (state << 6u) + (state >> 2u);
    return state;
}

static uint32_t render_composed_source_plan_build_effective_layers(
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    DrawingProgramComposedSourceEffectiveLayerInfo *out_layers) {
    uint32_t i;
    uint32_t effective_count = 0u;
    if (!document || !out_layers) {
        return 0u;
    }
    for (i = 0u; i < document->layer_count; ++i) {
        const DrawingProgramRasterSample *samples = 0;
        uint32_t sample_count = 0u;
        uint64_t content_revision = 0u;
        uint8_t opacity = 100u;
        CoreResult result;
        if (!document->layers[i].visible) {
            continue;
        }
        if (layer_opacity_percent && i < layer_opacity_count) {
            opacity = layer_opacity_percent[i];
            if (opacity > 100u) {
                opacity = 100u;
            }
        }
        if (opacity == 0u) {
            continue;
        }
        result = drawing_program_layer_raster_store_export_layer(layer_rasters,
                                                                 document->layers[i].layer_id,
                                                                 &samples,
                                                                 &sample_count);
        if (result.code != CORE_OK || !samples || sample_count != document->raster_sample_count) {
            continue;
        }
        if (drawing_program_layer_raster_store_export_layer_revision(layer_rasters,
                                                                     document->layers[i].layer_id,
                                                                     &content_revision)
            .code != CORE_OK) {
            content_revision = 0u;
        }
        out_layers[effective_count].document_layer_index = i;
        out_layers[effective_count].layer_id = document->layers[i].layer_id;
        out_layers[effective_count].content_revision = content_revision;
        out_layers[effective_count].samples = samples;
        out_layers[effective_count].opacity = opacity;
        effective_count += 1u;
    }
    return effective_count;
}

static uint64_t render_composed_source_plan_lower_stack_key(
    const struct DrawingProgramDocument *document,
    const DrawingProgramComposedSourceEffectiveLayerInfo *effective_layers,
    uint32_t lower_layer_count) {
    uint64_t key = 1469598103934665603ull;
    uint32_t i;
    if (!document || !effective_layers) {
        return 0u;
    }
    key = render_composed_source_plan_mix_key(key, document->raster_width);
    key = render_composed_source_plan_mix_key(key, document->raster_height);
    key = render_composed_source_plan_mix_key(key, document->raster_sample_count);
    for (i = 0u; i < lower_layer_count; ++i) {
        key = render_composed_source_plan_mix_key(key, effective_layers[i].layer_id);
        key = render_composed_source_plan_mix_key(key, effective_layers[i].content_revision);
        key = render_composed_source_plan_mix_key(key, effective_layers[i].opacity);
    }
    return key;
}

static uint64_t render_composed_source_plan_prefix_stack_key(
    const struct DrawingProgramDocument *document,
    const DrawingProgramComposedSourceEffectiveLayerInfo *effective_layers,
    uint32_t prefix_layer_count,
    uint64_t lower_stack_key) {
    uint64_t key = 1099511628211ull;
    uint32_t i;
    if (!document || !effective_layers) {
        return 0u;
    }
    key = render_composed_source_plan_mix_key(key, document->raster_width);
    key = render_composed_source_plan_mix_key(key, document->raster_height);
    key = render_composed_source_plan_mix_key(key, document->raster_sample_count);
    for (i = 0u; i < prefix_layer_count; ++i) {
        key = render_composed_source_plan_mix_key(key, effective_layers[i].layer_id);
        key = render_composed_source_plan_mix_key(key, effective_layers[i].content_revision);
        key = render_composed_source_plan_mix_key(key, effective_layers[i].opacity);
    }
    key = render_composed_source_plan_mix_key(key, lower_stack_key);
    return key;
}

static uint64_t render_composed_source_plan_suffix_stack_key(
    const struct DrawingProgramDocument *document,
    const DrawingProgramComposedSourceEffectiveLayerInfo *effective_layers,
    uint32_t start_index,
    uint32_t end_index) {
    uint64_t key = 7809847782465536322ull;
    uint32_t i;
    if (!document || !effective_layers || start_index >= end_index) {
        return 0u;
    }
    key = render_composed_source_plan_mix_key(key, document->raster_width);
    key = render_composed_source_plan_mix_key(key, document->raster_height);
    key = render_composed_source_plan_mix_key(key, document->raster_sample_count);
    for (i = start_index; i < end_index; ++i) {
        key = render_composed_source_plan_mix_key(key, effective_layers[i].layer_id);
        key = render_composed_source_plan_mix_key(key, effective_layers[i].content_revision);
        key = render_composed_source_plan_mix_key(key, effective_layers[i].opacity);
    }
    return key;
}

CoreResult drawing_program_render_composed_source_plan_build(
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    DrawingProgramRenderComposedSourcePlan *out_plan) {
    uint32_t suffix_start_index;
    if (!document || !layer_rasters || !out_plan) {
        CoreResult invalid = { CORE_ERR_INVALID_ARG, "invalid composed source plan request" };
        return invalid;
    }
    memset(out_plan, 0, sizeof(*out_plan));
    out_plan->effective_count = render_composed_source_plan_build_effective_layers(document,
                                                                                   layer_rasters,
                                                                                   layer_opacity_percent,
                                                                                   layer_opacity_count,
                                                                                   out_plan->effective_layers);
    if (out_plan->effective_count < 2u) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "partial suffix reuse requires multiple effective layers" };
    }
    if (out_plan->effective_layers[out_plan->effective_count - 1u].opacity >= 100u) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "partial suffix reuse requires partial top layer" };
    }
    suffix_start_index = out_plan->effective_count;
    while (suffix_start_index > 0u &&
           out_plan->effective_layers[suffix_start_index - 1u].opacity < 100u) {
        suffix_start_index -= 1u;
    }
    out_plan->partial_suffix_start_index = suffix_start_index;
    out_plan->partial_suffix_count = out_plan->effective_count - suffix_start_index;
    out_plan->lower_stack_key = render_composed_source_plan_lower_stack_key(document,
                                                                            out_plan->effective_layers,
                                                                            suffix_start_index);
    out_plan->supports_prefix_stack_cache = out_plan->partial_suffix_count >= 2u ? 1u : 0u;
    if (out_plan->supports_prefix_stack_cache) {
        out_plan->prefix_stack_key = render_composed_source_plan_prefix_stack_key(document,
                                                                                  out_plan->effective_layers,
                                                                                  out_plan->effective_count - 1u,
                                                                                  out_plan->lower_stack_key);
    }
    out_plan->supports_suffix_stack_cache = out_plan->partial_suffix_count >= 2u ? 1u : 0u;
    if (out_plan->supports_suffix_stack_cache) {
        out_plan->suffix_stack_key = render_composed_source_plan_suffix_stack_key(document,
                                                                                  out_plan->effective_layers,
                                                                                  out_plan->partial_suffix_start_index,
                                                                                  out_plan->effective_count);
    }
    out_plan->has_partial_suffix = out_plan->partial_suffix_count > 0u ? 1u : 0u;
    return core_result_ok();
}
