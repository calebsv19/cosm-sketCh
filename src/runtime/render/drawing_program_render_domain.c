#include "drawing_program/drawing_program_render_domain.h"

#include <string.h>

#include "core_base.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_editor_state.h"
#include "drawing_program/drawing_program_layer_raster.h"
#include "drawing_program/drawing_program_color_model.h"

static CoreResult render_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static uint32_t render_legacy_surface_layer_id(const DrawingProgramDocument *document) {
    uint32_t i;
    if (!document || document->layer_count == 0u) {
        return 0u;
    }
    for (i = 0u; i < document->layer_count; ++i) {
        if (document->layers[i].layer_id == 1u) {
            return 1u;
        }
    }
    return document->layers[0].layer_id;
}

static uint32_t render_layer_views_resolve(const DrawingProgramDocument *document,
                                           const DrawingProgramLayerRasterStore *layer_rasters,
                                           const DrawingProgramRasterSample **out_layer_views) {
    uint32_t i;
    uint32_t legacy_layer_id;
    if (!document || !out_layer_views) {
        return 0u;
    }
    for (i = 0u; i < DRAWING_PROGRAM_MAX_LAYERS; ++i) {
        out_layer_views[i] = 0;
    }
    legacy_layer_id = render_legacy_surface_layer_id(document);
    for (i = 0u; i < document->layer_count; ++i) {
        uint32_t layer_id = document->layers[i].layer_id;
        if (layer_rasters &&
            layer_rasters->sample_count == document->raster_sample_count &&
            layer_rasters->raster_width == document->raster_width &&
            layer_rasters->raster_height == document->raster_height) {
            const DrawingProgramRasterSample *samples = 0;
            uint32_t sample_count = 0u;
            CoreResult result = drawing_program_layer_raster_store_export_layer(layer_rasters,
                                                                                layer_id,
                                                                                &samples,
                                                                                &sample_count);
            if (result.code == CORE_OK && samples && sample_count == document->raster_sample_count) {
                out_layer_views[i] = samples;
                continue;
            }
        }
        if (layer_id == legacy_layer_id) {
            out_layer_views[i] = document->raster_samples;
        }
    }
    return legacy_layer_id;
}

typedef struct DrawingProgramRenderEffectiveLayer {
    const DrawingProgramRasterSample *samples;
    uint8_t opacity;
} DrawingProgramRenderEffectiveLayer;

static uint32_t render_effective_layers_build(
    const DrawingProgramDocument *document,
    const DrawingProgramRasterSample **layer_views,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    DrawingProgramRenderEffectiveLayer *out_layers,
    uint8_t *out_all_full_opacity,
    uint32_t *out_partial_count,
    uint32_t *out_first_partial_index) {
    uint32_t i;
    uint32_t effective_count = 0u;
    uint32_t partial_count = 0u;
    uint32_t first_partial_index = 0u;
    uint8_t all_full_opacity = 1u;
    if (!document || !layer_views || !out_layers) {
        if (out_all_full_opacity) {
            *out_all_full_opacity = 0u;
        }
        if (out_partial_count) {
            *out_partial_count = 0u;
        }
        if (out_first_partial_index) {
            *out_first_partial_index = 0u;
        }
        return 0u;
    }
    for (i = 0u; i < document->layer_count; ++i) {
        const DrawingProgramRasterSample *samples = layer_views[i];
        uint8_t opacity = 100u;
        if (!document->layers[i].visible || !samples) {
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
        out_layers[effective_count].samples = samples;
        out_layers[effective_count].opacity = opacity;
        if (opacity < 100u) {
            all_full_opacity = 0u;
            if (partial_count == 0u) {
                first_partial_index = effective_count;
            }
            partial_count += 1u;
        }
        effective_count += 1u;
    }
    if (out_all_full_opacity) {
        *out_all_full_opacity = (effective_count > 0u) ? all_full_opacity : 0u;
    }
    if (out_partial_count) {
        *out_partial_count = partial_count;
    }
    if (out_first_partial_index) {
        *out_first_partial_index = first_partial_index;
    }
    return effective_count;
}

static DrawingProgramRasterSample render_compose_sample_for_index(
    const DrawingProgramDocument *document,
    const DrawingProgramRenderEffectiveLayer *effective_layers,
    uint32_t effective_layer_count,
    uint8_t all_full_opacity,
    uint32_t sample_index) {
    uint32_t i;
    DrawingProgramRasterSample composed = drawing_program_color_eraser_value();
    if (!document || !effective_layers || sample_index >= document->raster_sample_count) {
        return composed;
    }
    if (all_full_opacity) {
        for (i = effective_layer_count; i > 0u; --i) {
            DrawingProgramRasterSample sample = effective_layers[i - 1u].samples[sample_index];
            if (!drawing_program_color_sample_is_transparent(sample)) {
                return sample;
            }
        }
        return composed;
    }
    for (i = effective_layer_count; i > 0u; --i) {
        DrawingProgramRasterSample sample = effective_layers[i - 1u].samples[sample_index];
        if (drawing_program_color_sample_is_transparent(sample)) {
            continue;
        }
        if (effective_layers[i - 1u].opacity >= 100u) {
            return sample;
        }
        break;
    }
    for (i = 0u; i < effective_layer_count; ++i) {
        DrawingProgramRasterSample sample = effective_layers[i].samples[sample_index];
        if (!drawing_program_color_sample_is_transparent(sample)) {
            if (effective_layers[i].opacity >= 100u) {
                composed = sample;
            } else {
                composed = drawing_program_color_blend_samples(composed, sample, effective_layers[i].opacity);
            }
        }
    }
    return composed;
}

static DrawingProgramRasterSample render_compose_single_partial_layer_for_index(
    const DrawingProgramRenderEffectiveLayer *effective_layers,
    uint32_t effective_layer_count,
    uint32_t partial_layer_index,
    uint32_t sample_index) {
    DrawingProgramRasterSample transparent = drawing_program_color_eraser_value();
    DrawingProgramRasterSample lower_base = transparent;
    DrawingProgramRasterSample partial_sample;
    uint8_t partial_opacity;
    uint32_t i;
    if (!effective_layers || partial_layer_index >= effective_layer_count) {
        return transparent;
    }
    for (i = effective_layer_count; i > partial_layer_index + 1u; --i) {
        DrawingProgramRasterSample sample = effective_layers[i - 1u].samples[sample_index];
        if (!drawing_program_color_sample_is_transparent(sample)) {
            return sample;
        }
    }
    for (i = partial_layer_index; i > 0u; --i) {
        DrawingProgramRasterSample sample = effective_layers[i - 1u].samples[sample_index];
        if (!drawing_program_color_sample_is_transparent(sample)) {
            lower_base = sample;
            break;
        }
    }
    partial_sample = effective_layers[partial_layer_index].samples[sample_index];
    partial_opacity = effective_layers[partial_layer_index].opacity;
    if (drawing_program_color_sample_is_transparent(partial_sample)) {
        return lower_base;
    }
    return drawing_program_color_blend_samples(lower_base, partial_sample, partial_opacity);
}

static void render_apply_uniform_opacity_samples(const DrawingProgramRasterSample *source_samples,
                                                 uint32_t sample_count,
                                                 uint8_t opacity,
                                                 DrawingProgramRasterSample *out_samples) {
    uint32_t i;
    DrawingProgramRasterSample transparent = drawing_program_color_eraser_value();
    if (!source_samples || !out_samples) {
        return;
    }
    if (opacity >= 100u) {
        memcpy(out_samples, source_samples, (size_t)sample_count * sizeof(*out_samples));
        return;
    }
    for (i = 0u; i < sample_count; ++i) {
        DrawingProgramRasterSample sample = source_samples[i];
        if (drawing_program_color_sample_is_transparent(sample)) {
            out_samples[i] = transparent;
        } else {
            out_samples[i] = drawing_program_color_blend_samples(transparent, sample, opacity);
        }
    }
}

CoreResult drawing_program_render_resolve_direct_visible_samples_with_layer_opacity(
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    const DrawingProgramRasterSample **out_samples,
    uint32_t *out_sample_count) {
    const DrawingProgramRasterSample *layer_views[DRAWING_PROGRAM_MAX_LAYERS];
    DrawingProgramRenderEffectiveLayer effective_layers[DRAWING_PROGRAM_MAX_LAYERS];
    const DrawingProgramRasterSample *direct_samples = 0;
    uint32_t effective_count;
    if (!document || !out_samples || !out_sample_count) {
        return render_invalid("null direct render resolve argument");
    }
    *out_samples = 0;
    *out_sample_count = 0u;
    if (document->raster_sample_count == 0u) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "no raster samples for direct render resolve" };
    }
    (void)render_layer_views_resolve(document, layer_rasters, layer_views);
    effective_count = render_effective_layers_build(document,
                                                    layer_views,
                                                    layer_opacity_percent,
                                                    layer_opacity_count,
                                                    effective_layers,
                                                    0,
                                                    0,
                                                    0);
    if (effective_count != 1u) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "direct render resolve requires compose" };
    }
    if (effective_layers[0].opacity < 100u) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "direct render resolve requires compose" };
    }
    direct_samples = effective_layers[0].samples;
    if (!direct_samples) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "direct render resolve found no direct source" };
    }
    *out_samples = direct_samples;
    *out_sample_count = document->raster_sample_count;
    return core_result_ok();
}

CoreResult drawing_program_render_compose_visible_samples_with_layer_opacity(
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    DrawingProgramRasterSample *out_samples,
    uint32_t out_capacity) {
    const DrawingProgramRasterSample *layer_views[DRAWING_PROGRAM_MAX_LAYERS];
    DrawingProgramRenderEffectiveLayer effective_layers[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t effective_count;
    uint32_t partial_count = 0u;
    uint32_t first_partial_index = 0u;
    uint8_t all_full_opacity = 0u;
    uint32_t i;
    if (!document || !out_samples) {
        return render_invalid("null render compose argument");
    }
    if (out_capacity < document->raster_sample_count) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "render compose output capacity too small" };
    }
    (void)render_layer_views_resolve(document, layer_rasters, layer_views);
    effective_count = render_effective_layers_build(document,
                                                    layer_views,
                                                    layer_opacity_percent,
                                                    layer_opacity_count,
                                                    effective_layers,
                                                    &all_full_opacity,
                                                    &partial_count,
                                                    &first_partial_index);
    if (effective_count == 0u) {
        for (i = 0u; i < document->raster_sample_count; ++i) {
            out_samples[i] = drawing_program_color_eraser_value();
        }
        return core_result_ok();
    }
    if (effective_count == 1u && effective_layers[0].opacity < 100u) {
        render_apply_uniform_opacity_samples(effective_layers[0].samples,
                                             document->raster_sample_count,
                                             effective_layers[0].opacity,
                                             out_samples);
        return core_result_ok();
    }
    if (partial_count == 1u) {
        for (i = 0u; i < document->raster_sample_count; ++i) {
            out_samples[i] = render_compose_single_partial_layer_for_index(effective_layers,
                                                                           effective_count,
                                                                           first_partial_index,
                                                                           i);
        }
        return core_result_ok();
    }
    for (i = 0u; i < document->raster_sample_count; ++i) {
        out_samples[i] = render_compose_sample_for_index(document,
                                                         effective_layers,
                                                         effective_count,
                                                         all_full_opacity,
                                                         i);
    }
    return core_result_ok();
}

CoreResult drawing_program_render_compose_visible_sample_with_layer_opacity(
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    uint32_t sample_x,
    uint32_t sample_y,
    DrawingProgramRasterSample *out_sample) {
    const DrawingProgramRasterSample *layer_views[DRAWING_PROGRAM_MAX_LAYERS];
    DrawingProgramRenderEffectiveLayer effective_layers[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t effective_count;
    uint32_t partial_count = 0u;
    uint32_t first_partial_index = 0u;
    uint8_t all_full_opacity = 0u;
    uint32_t sample_index;
    if (!document || !out_sample) {
        return render_invalid("null render compose single-sample argument");
    }
    if (sample_x >= document->raster_width || sample_y >= document->raster_height) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "render single-sample coordinates out of range" };
    }
    sample_index = sample_y * document->raster_width + sample_x;
    if (sample_index >= document->raster_sample_count) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "render single-sample index out of range" };
    }
    (void)render_layer_views_resolve(document, layer_rasters, layer_views);
    effective_count = render_effective_layers_build(document,
                                                    layer_views,
                                                    layer_opacity_percent,
                                                    layer_opacity_count,
                                                    effective_layers,
                                                    &all_full_opacity,
                                                    &partial_count,
                                                    &first_partial_index);
    if (effective_count == 0u) {
        *out_sample = drawing_program_color_eraser_value();
        return core_result_ok();
    }
    if (partial_count == 1u) {
        *out_sample = render_compose_single_partial_layer_for_index(effective_layers,
                                                                    effective_count,
                                                                    first_partial_index,
                                                                    sample_index);
        return core_result_ok();
    }
    *out_sample = render_compose_sample_for_index(document,
                                                  effective_layers,
                                                  effective_count,
                                                  all_full_opacity,
                                                  sample_index);
    return core_result_ok();
}

CoreResult drawing_program_render_compose_visible_sample(
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    uint32_t sample_x,
    uint32_t sample_y,
    DrawingProgramRasterSample *out_sample) {
    return drawing_program_render_compose_visible_sample_with_layer_opacity(document,
                                                                            layer_rasters,
                                                                            0,
                                                                            0u,
                                                                            sample_x,
                                                                            sample_y,
                                                                            out_sample);
}

CoreResult drawing_program_render_compose_visible_samples(
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    DrawingProgramRasterSample *out_samples,
    uint32_t out_capacity) {
    return drawing_program_render_compose_visible_samples_with_layer_opacity(document,
                                                                             layer_rasters,
                                                                             0,
                                                                             0u,
                                                                             out_samples,
                                                                             out_capacity);
}

CoreResult drawing_program_render_project_frame(
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const struct DrawingProgramEditorState *editor,
    const DrawingProgramRenderInvalidation *invalidation,
    DrawingProgramRenderFrameProjection *out_projection) {
    uint32_t i;
    if (!document || !editor || !invalidation || !out_projection) {
        return render_invalid("null render projection argument");
    }
    (void)layer_rasters;

    memset(out_projection, 0, sizeof(*out_projection));
    out_projection->logical_width = document->logical_width;
    out_projection->logical_height = document->logical_height;
    out_projection->sample_density = document->sample_density;
    out_projection->layer_count = document->layer_count;
    out_projection->active_layer_id = editor->active_layer_id;
    out_projection->raster_sample_count = document->raster_sample_count;
    out_projection->invalidation_reason_bits = invalidation->invalidation_reason_bits;
    out_projection->full_redraw =
        (invalidation->full_invalidate || invalidation->full_invalidation_count > 0u) ? 1u : 0u;
    out_projection->targeted_redraw =
        (!out_projection->full_redraw && invalidation->target_invalidation_count > 0u) ? 1u : 0u;

    for (i = 0u; i < document->layer_count; ++i) {
        if (document->layers[i].visible) {
            out_projection->visible_layer_count += 1u;
        }
        if (document->layers[i].layer_id == editor->active_layer_id) {
            out_projection->has_active_layer = 1u;
        }
    }
    out_projection->hidden_layer_count = out_projection->layer_count - out_projection->visible_layer_count;

    return core_result_ok();
}
