#include <stdlib.h>
#include <string.h>

#include "drawing_program_render_composed_source_internal.h"
#include "drawing_program/drawing_program_layer_raster.h"
#include "drawing_program/drawing_program_render_domain.h"

static CoreResult render_composed_source_rebuild_lower_stack(
    DrawingProgramRenderComposedSourceState *state,
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    const DrawingProgramComposedSourceEffectiveLayerInfo *effective_layers,
    uint32_t suffix_start_index,
    uint32_t effective_count) {
    uint8_t lower_opacity[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t i;
    if (!state || !document || !layer_rasters || !layer_opacity_percent || !effective_layers ||
        suffix_start_index >= effective_count) {
        return render_composed_source_invalid("invalid lower-stack rebuild request");
    }
    memcpy(lower_opacity, layer_opacity_percent, sizeof(lower_opacity));
    for (i = layer_opacity_count; i < DRAWING_PROGRAM_MAX_LAYERS; ++i) {
        lower_opacity[i] = 100u;
    }
    for (i = suffix_start_index; i < effective_count; ++i) {
        lower_opacity[effective_layers[i].document_layer_index] = 0u;
    }
    return drawing_program_render_compose_visible_samples_with_layer_opacity(document,
                                                                             layer_rasters,
                                                                             lower_opacity,
                                                                             DRAWING_PROGRAM_MAX_LAYERS,
                                                                             state->lower_stack_samples,
                                                                             state->composited_capacity);
}

static CoreResult render_composed_source_collect_lower_stack_dirty_rect(
    DrawingProgramRenderComposedSourceState *state,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const DrawingProgramRenderComposedSourcePlan *plan,
    uint32_t *out_x,
    uint32_t *out_y,
    uint32_t *out_width,
    uint32_t *out_height) {
    uint32_t i;
    uint8_t have_rect = 0u;
    if (!state || !layer_rasters || !plan || !out_x || !out_y || !out_width || !out_height) {
        return render_composed_source_invalid("invalid lower-stack dirty rect query");
    }
    for (i = 0u; i < plan->partial_suffix_start_index; ++i) {
        const DrawingProgramComposedSourceEffectiveLayerInfo *layer = &plan->effective_layers[i];
        uint64_t revision = 0u;
        CoreResult result = drawing_program_layer_raster_store_export_layer_revision(layer_rasters,
                                                                                     layer->layer_id,
                                                                                     &revision);
        if (result.code != CORE_OK) {
            return result;
        }
        if (revision == state->cached_lower_stack_layer_revisions[layer->document_layer_index]) {
            continue;
        }
        {
            uint32_t layer_x = 0u;
            uint32_t layer_y = 0u;
            uint32_t layer_width = 0u;
            uint32_t layer_height = 0u;
            result = drawing_program_layer_raster_store_export_layer_dirty_rect(layer_rasters,
                                                                                layer->layer_id,
                                                                                &layer_x,
                                                                                &layer_y,
                                                                                &layer_width,
                                                                                &layer_height);
            if (result.code != CORE_OK) {
                return result;
            }
            if (!have_rect) {
                *out_x = layer_x;
                *out_y = layer_y;
                *out_width = layer_width;
                *out_height = layer_height;
                have_rect = 1u;
            } else {
                uint32_t min_x = *out_x < layer_x ? *out_x : layer_x;
                uint32_t min_y = *out_y < layer_y ? *out_y : layer_y;
                uint32_t max_x = (*out_x + *out_width - 1u) > (layer_x + layer_width - 1u)
                                     ? (*out_x + *out_width - 1u)
                                     : (layer_x + layer_width - 1u);
                uint32_t max_y = (*out_y + *out_height - 1u) > (layer_y + layer_height - 1u)
                                     ? (*out_y + *out_height - 1u)
                                     : (layer_y + layer_height - 1u);
                *out_x = min_x;
                *out_y = min_y;
                *out_width = max_x - min_x + 1u;
                *out_height = max_y - min_y + 1u;
            }
        }
    }
    return have_rect ? core_result_ok() : (CoreResult){ CORE_ERR_NOT_FOUND, "lower-stack dirty rect unavailable" };
}

static CoreResult render_composed_source_collect_layer_range_dirty_rect(
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const DrawingProgramRenderComposedSourcePlan *plan,
    uint32_t start_index,
    uint32_t end_index,
    uint32_t *out_x,
    uint32_t *out_y,
    uint32_t *out_width,
    uint32_t *out_height) {
    uint32_t i;
    uint8_t have_rect = 0u;
    if (!layer_rasters || !plan || !out_x || !out_y || !out_width || !out_height || start_index >= end_index ||
        end_index > plan->effective_count) {
        return render_composed_source_invalid("invalid layer-range dirty rect query");
    }
    for (i = start_index; i < end_index; ++i) {
        uint32_t layer_x = 0u;
        uint32_t layer_y = 0u;
        uint32_t layer_width = 0u;
        uint32_t layer_height = 0u;
        CoreResult result = drawing_program_layer_raster_store_export_layer_dirty_rect(
            layer_rasters,
            plan->effective_layers[i].layer_id,
            &layer_x,
            &layer_y,
            &layer_width,
            &layer_height);
        if (result.code != CORE_OK) {
            return result;
        }
        if (!have_rect) {
            *out_x = layer_x;
            *out_y = layer_y;
            *out_width = layer_width;
            *out_height = layer_height;
            have_rect = 1u;
        } else {
            uint32_t min_x = *out_x < layer_x ? *out_x : layer_x;
            uint32_t min_y = *out_y < layer_y ? *out_y : layer_y;
            uint32_t max_x = (*out_x + *out_width - 1u) > (layer_x + layer_width - 1u)
                                 ? (*out_x + *out_width - 1u)
                                 : (layer_x + layer_width - 1u);
            uint32_t max_y = (*out_y + *out_height - 1u) > (layer_y + layer_height - 1u)
                                 ? (*out_y + *out_height - 1u)
                                 : (layer_y + layer_height - 1u);
            *out_x = min_x;
            *out_y = min_y;
            *out_width = max_x - min_x + 1u;
            *out_height = max_y - min_y + 1u;
        }
    }
    return have_rect ? core_result_ok() : (CoreResult){ CORE_ERR_NOT_FOUND, "layer-range dirty rect unavailable" };
}

static void render_composed_source_snapshot_lower_stack_layer_revisions(
    DrawingProgramRenderComposedSourceState *state,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const DrawingProgramRenderComposedSourcePlan *plan) {
    uint32_t i;
    if (!state || !layer_rasters || !plan) {
        return;
    }
    memset(state->cached_lower_stack_layer_revisions, 0, sizeof(state->cached_lower_stack_layer_revisions));
    for (i = 0u; i < plan->partial_suffix_start_index; ++i) {
        const DrawingProgramComposedSourceEffectiveLayerInfo *layer = &plan->effective_layers[i];
        uint64_t revision = 0u;
        if (drawing_program_layer_raster_store_export_layer_revision(layer_rasters, layer->layer_id, &revision).code ==
            CORE_OK) {
            state->cached_lower_stack_layer_revisions[layer->document_layer_index] = revision;
        }
    }
}

static void render_composed_source_clear_layer_range_dirty_rects(
    const struct DrawingProgramLayerRasterStore *layer_rasters_const,
    const DrawingProgramRenderComposedSourcePlan *plan) {
    DrawingProgramLayerRasterStore *layer_rasters = (DrawingProgramLayerRasterStore *)layer_rasters_const;
    uint32_t i;
    if (!layer_rasters || !plan) {
        return;
    }
    for (i = 0u; i < plan->effective_count; ++i) {
        (void)drawing_program_layer_raster_store_clear_layer_dirty_rect(layer_rasters,
                                                                        plan->effective_layers[i].layer_id);
    }
}

static void render_composed_source_compose_suffix_over_lower_stack(
    const struct DrawingProgramDocument *document,
    const DrawingProgramComposedSourceEffectiveLayerInfo *effective_layers,
    uint32_t suffix_start_index,
    uint32_t effective_count,
    const DrawingProgramRasterSample *lower_stack_samples,
    DrawingProgramRasterSample *out_samples) {
    uint32_t sample_index;
    uint32_t layer_index;
    if (!document || !effective_layers || !lower_stack_samples || !out_samples ||
        suffix_start_index >= effective_count) {
        return;
    }
    memcpy(out_samples,
           lower_stack_samples,
           (size_t)document->raster_sample_count * sizeof(*out_samples));
    for (layer_index = suffix_start_index; layer_index < effective_count; ++layer_index) {
        const DrawingProgramComposedSourceEffectiveLayerInfo *layer = &effective_layers[layer_index];
        for (sample_index = 0u; sample_index < document->raster_sample_count; ++sample_index) {
            DrawingProgramRasterSample sample = layer->samples[sample_index];
            if (drawing_program_color_sample_is_transparent(sample)) {
                continue;
            }
            if (layer->opacity >= 100u) {
                out_samples[sample_index] = sample;
            } else {
                out_samples[sample_index] = drawing_program_color_blend_samples(out_samples[sample_index],
                                                                                sample,
                                                                                layer->opacity);
            }
        }
    }
}

static void render_composed_source_compose_layer_range_over_base(
    const struct DrawingProgramDocument *document,
    const DrawingProgramComposedSourceEffectiveLayerInfo *effective_layers,
    uint32_t start_index,
    uint32_t end_index,
    const DrawingProgramRasterSample *base_samples,
    DrawingProgramRasterSample *out_samples) {
    uint32_t sample_index;
    uint32_t layer_index;
    if (!document || !effective_layers || !base_samples || !out_samples || start_index >= end_index) {
        return;
    }
    memcpy(out_samples,
           base_samples,
           (size_t)document->raster_sample_count * sizeof(*out_samples));
    for (layer_index = start_index; layer_index < end_index; ++layer_index) {
        const DrawingProgramComposedSourceEffectiveLayerInfo *layer = &effective_layers[layer_index];
        for (sample_index = 0u; sample_index < document->raster_sample_count; ++sample_index) {
            DrawingProgramRasterSample sample = layer->samples[sample_index];
            if (drawing_program_color_sample_is_transparent(sample)) {
                continue;
            }
            if (layer->opacity >= 100u) {
                out_samples[sample_index] = sample;
            } else {
                out_samples[sample_index] = drawing_program_color_blend_samples(out_samples[sample_index],
                                                                                sample,
                                                                                layer->opacity);
            }
        }
    }
}

static void render_composed_source_compose_layer_range_in_rect_over_base(
    const struct DrawingProgramDocument *document,
    const DrawingProgramComposedSourceEffectiveLayerInfo *effective_layers,
    uint32_t start_index,
    uint32_t end_index,
    const DrawingProgramRasterSample *base_samples,
    uint32_t rect_x,
    uint32_t rect_y,
    uint32_t rect_width,
    uint32_t rect_height,
    DrawingProgramRasterSample *out_samples) {
    uint32_t x;
    uint32_t y;
    uint32_t layer_index;
    if (!document || !effective_layers || !base_samples || !out_samples || start_index >= end_index ||
        rect_width == 0u || rect_height == 0u) {
        return;
    }
    for (y = rect_y; y < rect_y + rect_height; ++y) {
        size_t row_offset = (size_t)y * (size_t)document->raster_width;
        for (x = rect_x; x < rect_x + rect_width; ++x) {
            size_t index = row_offset + x;
            DrawingProgramRasterSample composed = base_samples[index];
            for (layer_index = start_index; layer_index < end_index; ++layer_index) {
                const DrawingProgramComposedSourceEffectiveLayerInfo *layer = &effective_layers[layer_index];
                DrawingProgramRasterSample sample = layer->samples[index];
                if (drawing_program_color_sample_is_transparent(sample)) {
                    continue;
                }
                if (layer->opacity >= 100u) {
                    composed = sample;
                } else {
                    composed = drawing_program_color_blend_samples(composed, sample, layer->opacity);
                }
            }
            out_samples[index] = composed;
        }
    }
}

static void render_composed_source_compose_lower_stack_in_rect(
    const struct DrawingProgramDocument *document,
    const DrawingProgramComposedSourceEffectiveLayerInfo *effective_layers,
    uint32_t end_index,
    uint32_t rect_x,
    uint32_t rect_y,
    uint32_t rect_width,
    uint32_t rect_height,
    DrawingProgramRasterSample *out_samples) {
    uint32_t x;
    uint32_t y;
    uint32_t layer_index;
    if (!document || !effective_layers || !out_samples || end_index == 0u || rect_width == 0u || rect_height == 0u) {
        return;
    }
    for (y = rect_y; y < rect_y + rect_height; ++y) {
        size_t row_offset = (size_t)y * (size_t)document->raster_width;
        for (x = rect_x; x < rect_x + rect_width; ++x) {
            size_t index = row_offset + x;
            DrawingProgramRasterSample composed = drawing_program_color_eraser_value();
            for (layer_index = 0u; layer_index < end_index; ++layer_index) {
                const DrawingProgramComposedSourceEffectiveLayerInfo *layer = &effective_layers[layer_index];
                DrawingProgramRasterSample sample = layer->samples[index];
                if (drawing_program_color_sample_is_transparent(sample)) {
                    continue;
                }
                if (layer->opacity >= 100u) {
                    composed = sample;
                } else {
                    composed = drawing_program_color_blend_samples(composed, sample, layer->opacity);
                }
            }
            out_samples[index] = composed;
        }
    }
}

static void render_composed_source_apply_cached_suffix_in_rect(
    const struct DrawingProgramDocument *document,
    const DrawingProgramRasterSample *lower_stack_samples,
    const DrawingProgramRasterSample *suffix_stack_samples,
    uint32_t rect_x,
    uint32_t rect_y,
    uint32_t rect_width,
    uint32_t rect_height,
    DrawingProgramRasterSample *out_samples) {
    uint32_t x;
    uint32_t y;
    if (!document || !lower_stack_samples || !suffix_stack_samples || !out_samples || rect_width == 0u ||
        rect_height == 0u) {
        return;
    }
    for (y = rect_y; y < rect_y + rect_height; ++y) {
        size_t row_offset = (size_t)y * (size_t)document->raster_width;
        for (x = rect_x; x < rect_x + rect_width; ++x) {
            size_t index = row_offset + x;
            out_samples[index] = lower_stack_samples[index];
            if (drawing_program_color_sample_is_transparent(suffix_stack_samples[index])) {
                continue;
            }
            out_samples[index] = drawing_program_color_blend_samples(out_samples[index],
                                                                     suffix_stack_samples[index],
                                                                     100u);
        }
    }
}

static CoreResult render_composed_source_resolve_prefix_stack(
    DrawingProgramRenderComposedSourceState *state,
    const struct DrawingProgramDocument *document,
    const DrawingProgramRenderComposedSourcePlan *plan,
    uint8_t *out_reused_prefix_stack) {
    if (!state || !document || !plan || !out_reused_prefix_stack) {
        return render_composed_source_invalid("invalid prefix-stack resolve request");
    }
    *out_reused_prefix_stack = 0u;
    if (!plan->supports_prefix_stack_cache) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "prefix stack cache not applicable" };
    }
    if (!state->has_cached_prefix_stack ||
        state->cached_prefix_stack_key != plan->prefix_stack_key ||
        state->cached_prefix_stack_sample_count != document->raster_sample_count) {
        if (state->has_cached_prefix_stack && state->cached_prefix_stack_sample_count == document->raster_sample_count &&
            state->prefix_stack_samples && state->previous_prefix_stack_samples) {
            memcpy(state->previous_prefix_stack_samples,
                   state->prefix_stack_samples,
                   (size_t)document->raster_sample_count * sizeof(*state->previous_prefix_stack_samples));
        }
        render_composed_source_compose_layer_range_over_base(document,
                                                             plan->effective_layers,
                                                             plan->partial_suffix_start_index,
                                                             plan->effective_count - 1u,
                                                             state->lower_stack_samples,
                                                             state->prefix_stack_samples);
        state->cached_prefix_stack_key = plan->prefix_stack_key;
        state->cached_prefix_stack_sample_count = document->raster_sample_count;
        state->has_cached_prefix_stack = 1u;
        return core_result_ok();
    }
    *out_reused_prefix_stack = 1u;
    return core_result_ok();
}

static CoreResult render_composed_source_resolve_suffix_stack(
    DrawingProgramRenderComposedSourceState *state,
    const struct DrawingProgramDocument *document,
    const DrawingProgramRenderComposedSourcePlan *plan,
    uint8_t *out_reused_suffix_stack) {
    if (!state || !document || !plan || !out_reused_suffix_stack) {
        return render_composed_source_invalid("invalid suffix-stack resolve request");
    }
    *out_reused_suffix_stack = 0u;
    if (!plan->supports_suffix_stack_cache) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "suffix stack cache not applicable" };
    }
    if (!state->has_cached_suffix_stack ||
        state->cached_suffix_stack_key != plan->suffix_stack_key ||
        state->cached_suffix_stack_sample_count != document->raster_sample_count) {
        memset(state->suffix_stack_samples, 0, (size_t)document->raster_sample_count * sizeof(*state->suffix_stack_samples));
        render_composed_source_compose_layer_range_over_base(document,
                                                             plan->effective_layers,
                                                             plan->partial_suffix_start_index,
                                                             plan->effective_count,
                                                             state->suffix_stack_samples,
                                                             state->suffix_stack_samples);
        state->cached_suffix_stack_key = plan->suffix_stack_key;
        state->cached_suffix_stack_sample_count = document->raster_sample_count;
        state->has_cached_suffix_stack = 1u;
        return core_result_ok();
    }
    *out_reused_suffix_stack = 1u;
    return core_result_ok();
}

static CoreResult render_composed_source_compose_with_cached_lower_stack(
    DrawingProgramRenderComposedSourceState *state,
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    DrawingProgramRasterSample *out_samples,
    uint8_t *out_reused_lower_stack,
    uint8_t *out_reused_prefix_stack,
    uint8_t *out_reused_suffix_stack,
    uint8_t *out_recomposed_dirty_rect_only,
    uint8_t *out_rebuilt_lower_stack_full_raster,
    uint8_t *out_recomposed_top_range_full_raster) {
    DrawingProgramRenderComposedSourcePlan plan;
    CoreResult result;
    if (!state || !document || !layer_rasters || !layer_opacity_percent || !out_samples || !out_reused_lower_stack ||
        !out_reused_prefix_stack || !out_reused_suffix_stack || !out_recomposed_dirty_rect_only ||
        !out_rebuilt_lower_stack_full_raster || !out_recomposed_top_range_full_raster) {
        return render_composed_source_invalid("invalid cached lower-stack compose request");
    }
    *out_reused_lower_stack = 0u;
    *out_reused_prefix_stack = 0u;
    *out_reused_suffix_stack = 0u;
    *out_recomposed_dirty_rect_only = 0u;
    *out_rebuilt_lower_stack_full_raster = 0u;
    *out_recomposed_top_range_full_raster = 0u;
    result = drawing_program_render_composed_source_plan_build(document,
                                                               layer_rasters,
                                                               layer_opacity_percent,
                                                               layer_opacity_count,
                                                               &plan);
    if (result.code != CORE_OK) {
        return result;
    }
    if (!state->has_cached_lower_stack ||
        state->cached_lower_stack_key != plan.lower_stack_key ||
        state->cached_lower_stack_sample_count != document->raster_sample_count) {
        uint32_t dirty_x = 0u;
        uint32_t dirty_y = 0u;
        uint32_t dirty_width = 0u;
        uint32_t dirty_height = 0u;
        uint8_t rebuilt_in_rect_only = 0u;
        if (state->has_cached_lower_stack &&
            render_composed_source_collect_lower_stack_dirty_rect(state,
                                                                  layer_rasters,
                                                                  &plan,
                                                                  &dirty_x,
                                                                  &dirty_y,
                                                                  &dirty_width,
                                                                  &dirty_height).code == CORE_OK &&
            !(dirty_x == 0u && dirty_y == 0u &&
              dirty_width == document->raster_width &&
              dirty_height == document->raster_height)) {
            render_composed_source_compose_lower_stack_in_rect(document,
                                                               plan.effective_layers,
                                                               plan.partial_suffix_start_index,
                                                               dirty_x,
                                                               dirty_y,
                                                               dirty_width,
                                                               dirty_height,
                                                               state->lower_stack_samples);
            rebuilt_in_rect_only = 1u;
        } else {
            result = render_composed_source_rebuild_lower_stack(state,
                                                                document,
                                                                layer_rasters,
                                                                layer_opacity_percent,
                                                                layer_opacity_count,
                                                                plan.effective_layers,
                                                                plan.partial_suffix_start_index,
                                                                plan.effective_count);
            if (result.code != CORE_OK) {
                return result;
            }
        }
        state->cached_lower_stack_key = plan.lower_stack_key;
        state->cached_lower_stack_sample_count = document->raster_sample_count;
        state->has_cached_lower_stack = 1u;
        render_composed_source_snapshot_lower_stack_layer_revisions(state, layer_rasters, &plan);
        *out_rebuilt_lower_stack_full_raster = rebuilt_in_rect_only ? 0u : 1u;
    } else {
        *out_reused_lower_stack = 1u;
    }
    if (plan.supports_prefix_stack_cache) {
        result = render_composed_source_resolve_prefix_stack(state,
                                                             document,
                                                             &plan,
                                                             out_reused_prefix_stack);
        if (result.code == CORE_OK) {
            if (*out_reused_prefix_stack) {
                uint32_t dirty_x = 0u;
                uint32_t dirty_y = 0u;
                uint32_t dirty_width = 0u;
                uint32_t dirty_height = 0u;
                if (render_composed_source_collect_layer_range_dirty_rect(layer_rasters,
                                                                          &plan,
                                                                          plan.effective_count - 1u,
                                                                          plan.effective_count,
                                                                          &dirty_x,
                                                                          &dirty_y,
                                                                          &dirty_width,
                                                                          &dirty_height)
                        .code == CORE_OK &&
                    !(dirty_x == 0u && dirty_y == 0u &&
                      dirty_width == document->raster_width &&
                      dirty_height == document->raster_height)) {
                    render_composed_source_compose_layer_range_in_rect_over_base(document,
                                                                                 plan.effective_layers,
                                                                                 plan.effective_count - 1u,
                                                                                 plan.effective_count,
                                                                                 state->prefix_stack_samples,
                                                                                 dirty_x,
                                                                                 dirty_y,
                                                                                 dirty_width,
                                                                                 dirty_height,
                                                                                 out_samples);
                    *out_recomposed_dirty_rect_only = 1u;
                    render_composed_source_clear_layer_range_dirty_rects(layer_rasters, &plan);
                    return core_result_ok();
                }
            } else if (plan.supports_suffix_stack_cache) {
                result = render_composed_source_resolve_suffix_stack(state,
                                                                     document,
                                                                     &plan,
                                                                     out_reused_suffix_stack);
                if (result.code != CORE_OK) {
                    state->has_cached_suffix_stack = 0u;
                    state->cached_suffix_stack_key = 0u;
                    state->cached_suffix_stack_sample_count = 0u;
                }
            }
            if (!*out_reused_prefix_stack && state->previous_prefix_stack_samples && state->has_cached_composed_samples) {
                uint32_t dirty_x = 0u;
                uint32_t dirty_y = 0u;
                uint32_t dirty_width = 0u;
                uint32_t dirty_height = 0u;
                if (render_composed_source_diff_dirty_rect(document,
                                                           state->previous_prefix_stack_samples,
                                                           state->prefix_stack_samples,
                                                           &dirty_x,
                                                           &dirty_y,
                                                           &dirty_width,
                                                           &dirty_height)) {
                    render_composed_source_compose_layer_range_in_rect_over_base(document,
                                                                                 plan.effective_layers,
                                                                                 plan.effective_count - 1u,
                                                                                 plan.effective_count,
                                                                                 state->prefix_stack_samples,
                                                                                 dirty_x,
                                                                                 dirty_y,
                                                                                 dirty_width,
                                                                                 dirty_height,
                                                                                 out_samples);
                    *out_recomposed_dirty_rect_only = 1u;
                    render_composed_source_clear_layer_range_dirty_rects(layer_rasters, &plan);
                    return core_result_ok();
                }
            }
            render_composed_source_compose_layer_range_over_base(document,
                                                                 plan.effective_layers,
                                                                 plan.effective_count - 1u,
                                                                 plan.effective_count,
                                                                 state->prefix_stack_samples,
                                                                 out_samples);
            *out_recomposed_top_range_full_raster = 1u;
            render_composed_source_clear_layer_range_dirty_rects(layer_rasters, &plan);
            return core_result_ok();
        }
        state->has_cached_prefix_stack = 0u;
        state->cached_prefix_stack_key = 0u;
        state->cached_prefix_stack_sample_count = 0u;
    }
    if (plan.supports_suffix_stack_cache) {
        result = render_composed_source_resolve_suffix_stack(state,
                                                             document,
                                                             &plan,
                                                             out_reused_suffix_stack);
        if (result.code != CORE_OK) {
            state->has_cached_suffix_stack = 0u;
            state->cached_suffix_stack_key = 0u;
            state->cached_suffix_stack_sample_count = 0u;
        }
    }
    if (plan.supports_suffix_stack_cache) {
        if (state->has_cached_suffix_stack &&
            state->cached_suffix_stack_key == plan.suffix_stack_key &&
            state->cached_suffix_stack_sample_count == document->raster_sample_count) {
            memcpy(out_samples,
                   state->lower_stack_samples,
                   (size_t)document->raster_sample_count * sizeof(*out_samples));
            render_composed_source_apply_cached_suffix_in_rect(document,
                                                               state->lower_stack_samples,
                                                               state->suffix_stack_samples,
                                                               0u,
                                                               0u,
                                                               document->raster_width,
                                                               document->raster_height,
                                                               out_samples);
            render_composed_source_clear_layer_range_dirty_rects(layer_rasters, &plan);
            return core_result_ok();
        }
    }
    render_composed_source_compose_suffix_over_lower_stack(document,
                                                           plan.effective_layers,
                                                           plan.partial_suffix_start_index,
                                                           plan.effective_count,
                                                           state->lower_stack_samples,
                                                           out_samples);
    *out_recomposed_top_range_full_raster = 1u;
    render_composed_source_clear_layer_range_dirty_rects(layer_rasters, &plan);
    return core_result_ok();
}

static CoreResult render_composed_source_ensure_capacity(
    DrawingProgramRenderComposedSourceState *state,
    uint32_t sample_count) {
    DrawingProgramRasterSample *next_samples = 0;
    DrawingProgramRasterSample *next_snapshot_samples = 0;
    DrawingProgramRasterSample *next_lower_stack_samples = 0;
    DrawingProgramRasterSample *next_previous_prefix_stack_samples = 0;
    DrawingProgramRasterSample *next_prefix_stack_samples = 0;
    DrawingProgramRasterSample *next_suffix_stack_samples = 0;
    if (!state || sample_count == 0u) {
        return render_composed_source_invalid("invalid composed source capacity request");
    }
    if (state->composited_capacity >= sample_count &&
        state->composited_samples &&
        state->resolved_snapshot_samples &&
        state->lower_stack_samples &&
        state->previous_prefix_stack_samples &&
        state->prefix_stack_samples &&
        state->suffix_stack_samples) {
        return core_result_ok();
    }
    next_samples = (DrawingProgramRasterSample *)realloc(state->composited_samples,
                                                         (size_t)sample_count * sizeof(*next_samples));
    if (!next_samples) {
        return (CoreResult){ CORE_ERR_IO, "composed source storage allocation failed" };
    }
    next_snapshot_samples = (DrawingProgramRasterSample *)realloc(state->resolved_snapshot_samples,
                                                                  (size_t)sample_count *
                                                                      sizeof(*next_snapshot_samples));
    if (!next_snapshot_samples) {
        return (CoreResult){ CORE_ERR_IO, "composed source snapshot allocation failed" };
    }
    next_lower_stack_samples = (DrawingProgramRasterSample *)realloc(state->lower_stack_samples,
                                                                     (size_t)sample_count *
                                                                         sizeof(*next_lower_stack_samples));
    if (!next_lower_stack_samples) {
        return (CoreResult){ CORE_ERR_IO, "composed source lower-stack allocation failed" };
    }
    next_previous_prefix_stack_samples =
        (DrawingProgramRasterSample *)realloc(state->previous_prefix_stack_samples,
                                              (size_t)sample_count * sizeof(*next_previous_prefix_stack_samples));
    if (!next_previous_prefix_stack_samples) {
        return (CoreResult){ CORE_ERR_IO, "composed source previous-prefix-stack allocation failed" };
    }
    next_prefix_stack_samples = (DrawingProgramRasterSample *)realloc(state->prefix_stack_samples,
                                                                      (size_t)sample_count *
                                                                          sizeof(*next_prefix_stack_samples));
    if (!next_prefix_stack_samples) {
        return (CoreResult){ CORE_ERR_IO, "composed source prefix-stack allocation failed" };
    }
    next_suffix_stack_samples = (DrawingProgramRasterSample *)realloc(state->suffix_stack_samples,
                                                                      (size_t)sample_count *
                                                                          sizeof(*next_suffix_stack_samples));
    if (!next_suffix_stack_samples) {
        return (CoreResult){ CORE_ERR_IO, "composed source suffix-stack allocation failed" };
    }
    state->composited_samples = next_samples;
    state->resolved_snapshot_samples = next_snapshot_samples;
    state->lower_stack_samples = next_lower_stack_samples;
    state->previous_prefix_stack_samples = next_previous_prefix_stack_samples;
    state->prefix_stack_samples = next_prefix_stack_samples;
    state->suffix_stack_samples = next_suffix_stack_samples;
    state->composited_capacity = sample_count;
    return core_result_ok();
}

void drawing_program_render_composed_source_dispose(
    DrawingProgramRenderComposedSourceState *state) {
    if (!state) {
        return;
    }
    free(state->composited_samples);
    free(state->resolved_snapshot_samples);
    free(state->lower_stack_samples);
    free(state->previous_prefix_stack_samples);
    free(state->prefix_stack_samples);
    free(state->suffix_stack_samples);
    state->composited_samples = 0;
    state->resolved_snapshot_samples = 0;
    state->lower_stack_samples = 0;
    state->previous_prefix_stack_samples = 0;
    state->prefix_stack_samples = 0;
    state->suffix_stack_samples = 0;
    state->composited_capacity = 0u;
    state->cached_content_key = 0u;
    state->cached_opacity_key = 0u;
    state->cached_sample_count = 0u;
    state->snapshot_width = 0u;
    state->snapshot_height = 0u;
    state->cached_lower_stack_key = 0u;
    state->cached_prefix_stack_key = 0u;
    state->cached_suffix_stack_key = 0u;
    state->cached_lower_stack_sample_count = 0u;
    state->cached_prefix_stack_sample_count = 0u;
    state->cached_suffix_stack_sample_count = 0u;
    state->has_cached_composed_samples = 0u;
    state->has_resolved_snapshot = 0u;
    state->has_cached_lower_stack = 0u;
    state->has_cached_prefix_stack = 0u;
    state->has_cached_suffix_stack = 0u;
}

CoreResult drawing_program_render_composed_source_resolve(
    DrawingProgramRenderComposedSourceState *state,
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    uint64_t content_key,
    uint64_t opacity_key,
    DrawingProgramRenderComposedSourceView *out_view) {
    const DrawingProgramRasterSample *direct_samples = 0;
    uint32_t direct_sample_count = 0u;
    uint8_t reused_lower_stack = 0u;
    uint8_t reused_prefix_stack = 0u;
    uint8_t reused_suffix_stack = 0u;
    uint8_t recomposed_dirty_rect_only = 0u;
    uint8_t rebuilt_lower_stack_full_raster = 0u;
    uint8_t recomposed_top_range_full_raster = 0u;
    CoreResult result;
    if (!state || !document || !layer_rasters || !layer_opacity_percent || !out_view) {
        return render_composed_source_invalid("null composed source argument");
    }
    memset(out_view, 0, sizeof(*out_view));
    if (document->raster_sample_count == 0u) {
        return render_composed_source_invalid("empty document composed source");
    }
    result = render_composed_source_ensure_capacity(state, document->raster_sample_count);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_render_resolve_direct_visible_samples_with_layer_opacity(document,
                                                                                      layer_rasters,
                                                                                      layer_opacity_percent,
                                                                                      layer_opacity_count,
                                                                                      &direct_samples,
                                                                                      &direct_sample_count);
    if (result.code == CORE_OK) {
        state->has_cached_composed_samples = 0u;
        state->has_cached_lower_stack = 0u;
        state->has_cached_prefix_stack = 0u;
        state->has_cached_suffix_stack = 0u;
        state->cached_content_key = 0u;
        state->cached_opacity_key = 0u;
        state->cached_sample_count = 0u;
        state->cached_lower_stack_key = 0u;
        state->cached_prefix_stack_key = 0u;
        state->cached_suffix_stack_key = 0u;
        state->cached_lower_stack_sample_count = 0u;
        state->cached_prefix_stack_sample_count = 0u;
        state->cached_suffix_stack_sample_count = 0u;
        out_view->samples = direct_samples;
        out_view->sample_count = direct_sample_count;
        out_view->used_composed_storage = 0u;
        render_composed_source_refresh_snapshot(state, document, out_view->samples, out_view);
        return core_result_ok();
    }
    if (state->has_cached_composed_samples &&
        state->cached_content_key == content_key &&
        state->cached_opacity_key == opacity_key &&
        state->cached_sample_count == document->raster_sample_count &&
        state->composited_samples) {
        out_view->samples = state->composited_samples;
        out_view->sample_count = document->raster_sample_count;
        out_view->used_composed_storage = 1u;
        out_view->reused_cached_compose = 1u;
        return core_result_ok();
    }
    result = render_composed_source_compose_with_cached_lower_stack(state,
                                                                    document,
                                                                    layer_rasters,
                                                                    layer_opacity_percent,
                                                                    layer_opacity_count,
                                                                    state->composited_samples,
                                                                    &reused_lower_stack,
                                                                    &reused_prefix_stack,
                                                                    &reused_suffix_stack,
                                                                    &recomposed_dirty_rect_only,
                                                                    &rebuilt_lower_stack_full_raster,
                                                                    &recomposed_top_range_full_raster);
    if (result.code != CORE_OK) {
        result = drawing_program_render_compose_visible_samples_with_layer_opacity(document,
                                                                                   layer_rasters,
                                                                                   layer_opacity_percent,
                                                                                   layer_opacity_count,
                                                                                   state->composited_samples,
                                                                                   state->composited_capacity);
        state->has_cached_lower_stack = 0u;
        state->has_cached_prefix_stack = 0u;
        state->has_cached_suffix_stack = 0u;
        state->cached_lower_stack_key = 0u;
        state->cached_prefix_stack_key = 0u;
        state->cached_suffix_stack_key = 0u;
        state->cached_lower_stack_sample_count = 0u;
        state->cached_prefix_stack_sample_count = 0u;
        state->cached_suffix_stack_sample_count = 0u;
    }
    if (result.code != CORE_OK) {
        return result;
    }
    state->cached_content_key = content_key;
    state->cached_opacity_key = opacity_key;
    state->cached_sample_count = document->raster_sample_count;
    state->has_cached_composed_samples = 1u;
    out_view->samples = state->composited_samples;
    out_view->sample_count = document->raster_sample_count;
    out_view->used_composed_storage = 1u;
    out_view->reused_cached_lower_stack = reused_lower_stack;
    out_view->reused_cached_prefix_stack = reused_prefix_stack;
    out_view->reused_cached_suffix_stack = reused_suffix_stack;
    out_view->recomposed_dirty_rect_only = recomposed_dirty_rect_only;
    out_view->rebuilt_lower_stack_full_raster = rebuilt_lower_stack_full_raster;
    out_view->recomposed_top_range_full_raster = recomposed_top_range_full_raster;
    render_composed_source_refresh_snapshot(state, document, out_view->samples, out_view);
    return core_result_ok();
}
