#include "drawing_program/drawing_program_render_composed_source.h"

#include <stdlib.h>
#include <string.h>

#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_layer_raster.h"
#include "drawing_program/drawing_program_render_domain.h"

typedef struct DrawingProgramComposedSourceEffectiveLayerInfo {
    uint32_t document_layer_index;
    uint32_t layer_id;
    uint64_t content_revision;
    const DrawingProgramRasterSample *samples;
    uint8_t opacity;
} DrawingProgramComposedSourceEffectiveLayerInfo;

static CoreResult render_composed_source_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static void render_composed_source_mark_full_dirty(
    const struct DrawingProgramDocument *document,
    DrawingProgramRenderComposedSourceView *out_view) {
    if (!document || !out_view || document->raster_width == 0u || document->raster_height == 0u) {
        return;
    }
    out_view->has_dirty_rect = 1u;
    out_view->dirty_rect_is_full = 1u;
    out_view->dirty_x = 0u;
    out_view->dirty_y = 0u;
    out_view->dirty_width = document->raster_width;
    out_view->dirty_height = document->raster_height;
}

static void render_composed_source_refresh_snapshot(
    DrawingProgramRenderComposedSourceState *state,
    const struct DrawingProgramDocument *document,
    const DrawingProgramRasterSample *samples,
    DrawingProgramRenderComposedSourceView *out_view) {
    uint32_t x = 0u;
    uint32_t y = 0u;
    uint32_t min_x = 0u;
    uint32_t min_y = 0u;
    uint32_t max_x = 0u;
    uint32_t max_y = 0u;
    uint8_t any_changed = 0u;
    if (!state || !document || !samples || !out_view || document->raster_sample_count == 0u) {
        return;
    }
    if (state->composited_capacity < document->raster_sample_count || !state->resolved_snapshot_samples) {
        render_composed_source_mark_full_dirty(document, out_view);
        if (state->resolved_snapshot_samples) {
            memcpy(state->resolved_snapshot_samples,
                   samples,
                   (size_t)document->raster_sample_count * sizeof(*samples));
        }
        state->snapshot_width = document->raster_width;
        state->snapshot_height = document->raster_height;
        state->has_resolved_snapshot = 1u;
        return;
    }
    if (!state->has_resolved_snapshot ||
        state->snapshot_width != document->raster_width ||
        state->snapshot_height != document->raster_height) {
        render_composed_source_mark_full_dirty(document, out_view);
        memcpy(state->resolved_snapshot_samples,
               samples,
               (size_t)document->raster_sample_count * sizeof(*samples));
        state->snapshot_width = document->raster_width;
        state->snapshot_height = document->raster_height;
        state->has_resolved_snapshot = 1u;
        return;
    }
    for (y = 0u; y < document->raster_height; ++y) {
        size_t row_offset = (size_t)y * (size_t)document->raster_width;
        for (x = 0u; x < document->raster_width; ++x) {
            size_t index = row_offset + x;
            if (state->resolved_snapshot_samples[index] == samples[index]) {
                continue;
            }
            state->resolved_snapshot_samples[index] = samples[index];
            if (!any_changed) {
                min_x = x;
                max_x = x;
                min_y = y;
                max_y = y;
                any_changed = 1u;
            } else {
                if (x < min_x) {
                    min_x = x;
                }
                if (x > max_x) {
                    max_x = x;
                }
                if (y < min_y) {
                    min_y = y;
                }
                if (y > max_y) {
                    max_y = y;
                }
            }
        }
    }
    state->snapshot_width = document->raster_width;
    state->snapshot_height = document->raster_height;
    state->has_resolved_snapshot = 1u;
    if (!any_changed) {
        return;
    }
    out_view->has_dirty_rect = 1u;
    out_view->dirty_rect_is_full = (min_x == 0u &&
                                    min_y == 0u &&
                                    max_x + 1u == document->raster_width &&
                                    max_y + 1u == document->raster_height)
                                       ? 1u
                                       : 0u;
    out_view->dirty_x = min_x;
    out_view->dirty_y = min_y;
    out_view->dirty_width = max_x - min_x + 1u;
    out_view->dirty_height = max_y - min_y + 1u;
}

static uint64_t render_composed_source_mix_key(uint64_t state, uint64_t value) {
    state ^= value + 0x9e3779b97f4a7c15ull + (state << 6u) + (state >> 2u);
    return state;
}

static uint32_t render_composed_source_effective_layers_build(
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

static uint64_t render_composed_source_lower_stack_key(
    const struct DrawingProgramDocument *document,
    const DrawingProgramComposedSourceEffectiveLayerInfo *effective_layers,
    uint32_t lower_layer_count) {
    uint64_t key = 1469598103934665603ull;
    uint32_t i;
    if (!document || !effective_layers) {
        return 0u;
    }
    key = render_composed_source_mix_key(key, document->raster_width);
    key = render_composed_source_mix_key(key, document->raster_height);
    key = render_composed_source_mix_key(key, document->raster_sample_count);
    for (i = 0u; i < lower_layer_count; ++i) {
        key = render_composed_source_mix_key(key, effective_layers[i].layer_id);
        key = render_composed_source_mix_key(key, effective_layers[i].content_revision);
        key = render_composed_source_mix_key(key, effective_layers[i].opacity);
    }
    return key;
}

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

static CoreResult render_composed_source_resolve_partial_suffix_start(
    const DrawingProgramComposedSourceEffectiveLayerInfo *effective_layers,
    uint32_t effective_count,
    uint32_t *out_suffix_start_index) {
    uint32_t suffix_start_index;
    if (!effective_layers || !out_suffix_start_index) {
        return render_composed_source_invalid("invalid partial suffix request");
    }
    if (effective_count < 2u) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "partial suffix reuse requires multiple effective layers" };
    }
    if (effective_layers[effective_count - 1u].opacity >= 100u) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "partial suffix reuse requires partial top layer" };
    }
    suffix_start_index = effective_count - 1u;
    if (effective_count >= 3u && effective_layers[effective_count - 2u].opacity < 100u) {
        suffix_start_index = effective_count - 2u;
        if (effective_count >= 4u && effective_layers[effective_count - 3u].opacity < 100u) {
            return (CoreResult){ CORE_ERR_NOT_FOUND, "partial suffix reuse limited to top two partial layers" };
        }
    }
    *out_suffix_start_index = suffix_start_index;
    return core_result_ok();
}

static CoreResult render_composed_source_compose_with_cached_lower_stack(
    DrawingProgramRenderComposedSourceState *state,
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    DrawingProgramRasterSample *out_samples,
    uint8_t *out_reused_lower_stack) {
    DrawingProgramComposedSourceEffectiveLayerInfo effective_layers[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t effective_count = 0u;
    uint32_t suffix_start_index = 0u;
    uint64_t lower_stack_key = 0u;
    CoreResult result;
    if (!state || !document || !layer_rasters || !layer_opacity_percent || !out_samples || !out_reused_lower_stack) {
        return render_composed_source_invalid("invalid cached lower-stack compose request");
    }
    *out_reused_lower_stack = 0u;
    effective_count = render_composed_source_effective_layers_build(document,
                                                                    layer_rasters,
                                                                    layer_opacity_percent,
                                                                    layer_opacity_count,
                                                                    effective_layers);
    result = render_composed_source_resolve_partial_suffix_start(effective_layers,
                                                                 effective_count,
                                                                 &suffix_start_index);
    if (result.code != CORE_OK) {
        return result;
    }
    lower_stack_key = render_composed_source_lower_stack_key(document, effective_layers, suffix_start_index);
    if (!state->has_cached_lower_stack ||
        state->cached_lower_stack_key != lower_stack_key ||
        state->cached_lower_stack_sample_count != document->raster_sample_count) {
        result = render_composed_source_rebuild_lower_stack(state,
                                                            document,
                                                            layer_rasters,
                                                            layer_opacity_percent,
                                                            layer_opacity_count,
                                                            effective_layers,
                                                            suffix_start_index,
                                                            effective_count);
        if (result.code != CORE_OK) {
            return result;
        }
        state->cached_lower_stack_key = lower_stack_key;
        state->cached_lower_stack_sample_count = document->raster_sample_count;
        state->has_cached_lower_stack = 1u;
    } else {
        *out_reused_lower_stack = 1u;
    }
    render_composed_source_compose_suffix_over_lower_stack(document,
                                                           effective_layers,
                                                           suffix_start_index,
                                                           effective_count,
                                                           state->lower_stack_samples,
                                                           out_samples);
    return core_result_ok();
}

static CoreResult render_composed_source_ensure_capacity(
    DrawingProgramRenderComposedSourceState *state,
    uint32_t sample_count) {
    DrawingProgramRasterSample *next_samples = 0;
    DrawingProgramRasterSample *next_snapshot_samples = 0;
    DrawingProgramRasterSample *next_lower_stack_samples = 0;
    if (!state || sample_count == 0u) {
        return render_composed_source_invalid("invalid composed source capacity request");
    }
    if (state->composited_capacity >= sample_count &&
        state->composited_samples &&
        state->resolved_snapshot_samples &&
        state->lower_stack_samples) {
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
    state->composited_samples = next_samples;
    state->resolved_snapshot_samples = next_snapshot_samples;
    state->lower_stack_samples = next_lower_stack_samples;
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
    state->composited_samples = 0;
    state->resolved_snapshot_samples = 0;
    state->lower_stack_samples = 0;
    state->composited_capacity = 0u;
    state->cached_content_key = 0u;
    state->cached_opacity_key = 0u;
    state->cached_sample_count = 0u;
    state->snapshot_width = 0u;
    state->snapshot_height = 0u;
    state->cached_lower_stack_key = 0u;
    state->cached_lower_stack_sample_count = 0u;
    state->has_cached_composed_samples = 0u;
    state->has_resolved_snapshot = 0u;
    state->has_cached_lower_stack = 0u;
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
        state->cached_content_key = 0u;
        state->cached_opacity_key = 0u;
        state->cached_sample_count = 0u;
        state->cached_lower_stack_key = 0u;
        state->cached_lower_stack_sample_count = 0u;
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
                                                                    &reused_lower_stack);
    if (result.code != CORE_OK) {
        result = drawing_program_render_compose_visible_samples_with_layer_opacity(document,
                                                                                   layer_rasters,
                                                                                   layer_opacity_percent,
                                                                                   layer_opacity_count,
                                                                                   state->composited_samples,
                                                                                   state->composited_capacity);
        state->has_cached_lower_stack = 0u;
        state->cached_lower_stack_key = 0u;
        state->cached_lower_stack_sample_count = 0u;
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
    render_composed_source_refresh_snapshot(state, document, out_view->samples, out_view);
    return core_result_ok();
}
