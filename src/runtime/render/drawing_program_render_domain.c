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

static DrawingProgramRasterSample render_compose_sample_for_index(
    const DrawingProgramDocument *document,
    const DrawingProgramRasterSample **layer_views,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    uint32_t sample_index) {
    uint32_t i;
    DrawingProgramRasterSample composed = drawing_program_color_eraser_value();
    if (!document || !layer_views || sample_index >= document->raster_sample_count) {
        return composed;
    }
    for (i = 0u; i < document->layer_count; ++i) {
        const DrawingProgramRasterSample *samples;
        DrawingProgramRasterSample sample;
        uint8_t opacity = 100u;
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
        samples = layer_views[i];
        if (!samples) {
            continue;
        }
        sample = samples[sample_index];
        if (!drawing_program_color_sample_is_transparent(sample)) {
            if (opacity >= 100u) {
                composed = sample;
            } else {
                composed = drawing_program_color_blend_samples(composed, sample, opacity);
            }
        }
    }
    return composed;
}

CoreResult drawing_program_render_compose_visible_samples_with_layer_opacity(
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    DrawingProgramRasterSample *out_samples,
    uint32_t out_capacity) {
    const DrawingProgramRasterSample *layer_views[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t i;
    if (!document || !out_samples) {
        return render_invalid("null render compose argument");
    }
    if (out_capacity < document->raster_sample_count) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "render compose output capacity too small" };
    }
    (void)render_layer_views_resolve(document, layer_rasters, layer_views);
    for (i = 0u; i < document->raster_sample_count; ++i) {
        out_samples[i] = render_compose_sample_for_index(document,
                                                         layer_views,
                                                         layer_opacity_percent,
                                                         layer_opacity_count,
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
    *out_sample = render_compose_sample_for_index(document,
                                                  layer_views,
                                                  layer_opacity_percent,
                                                  layer_opacity_count,
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
