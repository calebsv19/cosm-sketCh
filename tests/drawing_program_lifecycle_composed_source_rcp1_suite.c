#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_layer_raster.h"
#include "drawing_program/drawing_program_render_composed_source.h"
#include "drawing_program_lifecycle_composed_source_rcp1_suite.h"

static int composed_source_rcp1_expect_ok(CoreResult result, const char *label) {
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: %s failed: %s\n", label, result.message ? result.message : "(null)");
        return 0;
    }
    return 1;
}

static int composed_source_rcp1_seed_three_partial_suffix(DrawingProgramDocument *document,
                                                          DrawingProgramLayerRasterStore *layer_rasters,
                                                          uint32_t *out_lower_partial_layer_id,
                                                          uint32_t *out_upper_partial_layer_id,
                                                          uint32_t *out_top_partial_layer_id) {
    if (!document || !layer_rasters || !out_lower_partial_layer_id || !out_upper_partial_layer_id ||
        !out_top_partial_layer_id) {
        return 0;
    }
    if (!composed_source_rcp1_expect_ok(drawing_program_document_init_with_shape(document, 8u, 8u, 1u),
                                        "rcp1_init_shape")) {
        return 0;
    }
    if (!composed_source_rcp1_expect_ok(drawing_program_layer_raster_store_init_from_document(layer_rasters, document),
                                        "rcp1_init_layer_rasters")) {
        return 0;
    }
    if (!composed_source_rcp1_expect_ok(drawing_program_document_add_layer(document,
                                                                           "LowerPartial",
                                                                           out_lower_partial_layer_id),
                                        "rcp1_add_lower_partial")) {
        return 0;
    }
    if (!composed_source_rcp1_expect_ok(drawing_program_document_add_layer(document,
                                                                           "UpperPartial",
                                                                           out_upper_partial_layer_id),
                                        "rcp1_add_upper_partial")) {
        return 0;
    }
    if (!composed_source_rcp1_expect_ok(drawing_program_document_add_layer(document,
                                                                           "TopPartial",
                                                                           out_top_partial_layer_id),
                                        "rcp1_add_top_partial")) {
        return 0;
    }
    if (!composed_source_rcp1_expect_ok(drawing_program_layer_raster_store_sync_document_layers(layer_rasters,
                                                                                                 document),
                                        "rcp1_sync_layers")) {
        return 0;
    }
    return 1;
}

static int composed_source_rcp1_assert_top_band_edit_baseline(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramRenderComposedSourceState state;
    DrawingProgramRenderComposedSourceView first_view;
    DrawingProgramRenderComposedSourceView second_view;
    uint32_t lower_partial_layer_id = 0u;
    uint32_t upper_partial_layer_id = 0u;
    uint32_t top_partial_layer_id = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    const uint32_t sample_x = 5u;
    const uint32_t sample_y = 3u;
    const DrawingProgramRasterSample base_sample = drawing_program_color_value_from_rgba(20u, 30u, 180u, 255u);
    const DrawingProgramRasterSample lower_partial_sample =
        drawing_program_color_value_from_rgba(40u, 200u, 70u, 255u);
    const DrawingProgramRasterSample upper_partial_sample =
        drawing_program_color_value_from_rgba(220u, 60u, 50u, 255u);
    const DrawingProgramRasterSample first_top_partial_sample =
        drawing_program_color_value_from_rgba(240u, 220u, 40u, 255u);
    const DrawingProgramRasterSample second_top_partial_sample =
        drawing_program_color_value_from_rgba(20u, 240u, 240u, 255u);
    DrawingProgramRasterSample lower_over_base;
    DrawingProgramRasterSample upper_over_lower;
    DrawingProgramRasterSample expected_second;
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(&state, 0, sizeof(state));
    memset(&first_view, 0, sizeof(first_view));
    memset(&second_view, 0, sizeof(second_view));
    memset(opacity, 100, sizeof(opacity));
    if (!composed_source_rcp1_seed_three_partial_suffix(&document,
                                                        &layer_rasters,
                                                        &lower_partial_layer_id,
                                                        &upper_partial_layer_id,
                                                        &top_partial_layer_id)) {
        return 1;
    }
    if (!composed_source_rcp1_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                         document.layers[0].layer_id,
                                                                                         sample_x,
                                                                                         sample_y,
                                                                                         base_sample,
                                                                                         0),
                                        "rcp1_write_base_top_band")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_rcp1_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                         lower_partial_layer_id,
                                                                                         sample_x,
                                                                                         sample_y,
                                                                                         lower_partial_sample,
                                                                                         0),
                                        "rcp1_write_lower_partial_top_band")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_rcp1_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                         upper_partial_layer_id,
                                                                                         sample_x,
                                                                                         sample_y,
                                                                                         upper_partial_sample,
                                                                                         0),
                                        "rcp1_write_upper_partial_top_band")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_rcp1_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                         top_partial_layer_id,
                                                                                         sample_x,
                                                                                         sample_y,
                                                                                         first_top_partial_sample,
                                                                                         0),
                                        "rcp1_write_top_partial_top_band")) {
        status = 0;
        goto cleanup;
    }
    opacity[1] = 70u;
    opacity[2] = 60u;
    opacity[3] = 50u;
    if (!composed_source_rcp1_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                        &document,
                                                                                        &layer_rasters,
                                                                                        opacity,
                                                                                        DRAWING_PROGRAM_MAX_LAYERS,
                                                                                        2000u,
                                                                                        2100u,
                                                                                        &first_view),
                                        "rcp1_resolve_top_band_first")) {
        status = 0;
        goto cleanup;
    }
    if (!state.has_cached_lower_stack || !state.has_cached_prefix_stack || !state.has_cached_suffix_stack) {
        fprintf(stderr, "lifecycle_test: expected top-band baseline first resolve to populate all cached partial-suffix lanes\n");
        status = 0;
        goto cleanup;
    }
    if (!composed_source_rcp1_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                         top_partial_layer_id,
                                                                                         sample_x,
                                                                                         sample_y,
                                                                                         second_top_partial_sample,
                                                                                         0),
                                        "rcp1_rewrite_top_partial_top_band")) {
        status = 0;
        goto cleanup;
    }
    lower_over_base = drawing_program_color_blend_samples(base_sample, lower_partial_sample, 70u);
    upper_over_lower = drawing_program_color_blend_samples(lower_over_base, upper_partial_sample, 60u);
    expected_second = drawing_program_color_blend_samples(upper_over_lower, second_top_partial_sample, 50u);
    if (!composed_source_rcp1_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                        &document,
                                                                                        &layer_rasters,
                                                                                        opacity,
                                                                                        DRAWING_PROGRAM_MAX_LAYERS,
                                                                                        2001u,
                                                                                        2100u,
                                                                                        &second_view),
                                        "rcp1_resolve_top_band_second")) {
        status = 0;
        goto cleanup;
    }
    if (second_view.reused_cached_lower_stack != 1u ||
        second_view.reused_cached_prefix_stack != 1u ||
        second_view.reused_cached_suffix_stack != 0u ||
        second_view.recomposed_dirty_rect_only != 1u) {
        fprintf(stderr,
                "lifecycle_test: expected top-band baseline to reuse lower/prefix caches while narrowing the final top-range replay\n");
        status = 0;
        goto cleanup;
    }
    if (!second_view.has_dirty_rect || second_view.dirty_rect_is_full ||
        second_view.dirty_x != sample_x || second_view.dirty_y != sample_y ||
        second_view.dirty_width != 1u || second_view.dirty_height != 1u) {
        fprintf(stderr,
                "lifecycle_test: expected top-band baseline dirty rect to stay pinned to one edited sample got "
                "has=%u full=%u rect=%u,%u %ux%u\n",
                (unsigned)second_view.has_dirty_rect,
                (unsigned)second_view.dirty_rect_is_full,
                (unsigned)second_view.dirty_x,
                (unsigned)second_view.dirty_y,
                (unsigned)second_view.dirty_width,
                (unsigned)second_view.dirty_height);
        status = 0;
        goto cleanup;
    }
    if (second_view.samples[(size_t)sample_y * document.raster_width + sample_x] != expected_second) {
        fprintf(stderr, "lifecycle_test: expected top-band baseline to preserve the correct visible blend after top edit\n");
        status = 0;
    }
cleanup:
    drawing_program_render_composed_source_dispose(&state);
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

static int composed_source_rcp1_assert_lower_band_edit_baseline(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramRenderComposedSourceState state;
    DrawingProgramRenderComposedSourceView first_view;
    DrawingProgramRenderComposedSourceView second_view;
    uint32_t lower_partial_layer_id = 0u;
    uint32_t upper_partial_layer_id = 0u;
    uint32_t top_partial_layer_id = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    const uint32_t sample_x = 2u;
    const uint32_t sample_y = 6u;
    const DrawingProgramRasterSample first_base_sample = drawing_program_color_value_from_rgba(25u, 20u, 160u, 255u);
    const DrawingProgramRasterSample second_base_sample = drawing_program_color_value_from_rgba(210u, 210u, 35u, 255u);
    const DrawingProgramRasterSample lower_partial_sample =
        drawing_program_color_value_from_rgba(30u, 190u, 80u, 255u);
    const DrawingProgramRasterSample upper_partial_sample =
        drawing_program_color_value_from_rgba(210u, 70u, 40u, 255u);
    const DrawingProgramRasterSample top_partial_sample =
        drawing_program_color_value_from_rgba(230u, 230u, 250u, 255u);
    DrawingProgramRasterSample lower_over_base;
    DrawingProgramRasterSample upper_over_lower;
    DrawingProgramRasterSample expected_second;
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(&state, 0, sizeof(state));
    memset(&first_view, 0, sizeof(first_view));
    memset(&second_view, 0, sizeof(second_view));
    memset(opacity, 100, sizeof(opacity));
    if (!composed_source_rcp1_seed_three_partial_suffix(&document,
                                                        &layer_rasters,
                                                        &lower_partial_layer_id,
                                                        &upper_partial_layer_id,
                                                        &top_partial_layer_id)) {
        return 1;
    }
    if (!composed_source_rcp1_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                         document.layers[0].layer_id,
                                                                                         sample_x,
                                                                                         sample_y,
                                                                                         first_base_sample,
                                                                                         0),
                                        "rcp1_write_base_lower_band")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_rcp1_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                         lower_partial_layer_id,
                                                                                         sample_x,
                                                                                         sample_y,
                                                                                         lower_partial_sample,
                                                                                         0),
                                        "rcp1_write_lower_partial_lower_band")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_rcp1_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                         upper_partial_layer_id,
                                                                                         sample_x,
                                                                                         sample_y,
                                                                                         upper_partial_sample,
                                                                                         0),
                                        "rcp1_write_upper_partial_lower_band")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_rcp1_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                         top_partial_layer_id,
                                                                                         sample_x,
                                                                                         sample_y,
                                                                                         top_partial_sample,
                                                                                         0),
                                        "rcp1_write_top_partial_lower_band")) {
        status = 0;
        goto cleanup;
    }
    opacity[1] = 70u;
    opacity[2] = 60u;
    opacity[3] = 50u;
    if (!composed_source_rcp1_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                        &document,
                                                                                        &layer_rasters,
                                                                                        opacity,
                                                                                        DRAWING_PROGRAM_MAX_LAYERS,
                                                                                        3000u,
                                                                                        3100u,
                                                                                        &first_view),
                                        "rcp1_resolve_lower_band_first")) {
        status = 0;
        goto cleanup;
    }
    if (!state.has_cached_lower_stack || !state.has_cached_prefix_stack || !state.has_cached_suffix_stack) {
        fprintf(stderr, "lifecycle_test: expected lower-band baseline first resolve to populate all cached partial-suffix lanes\n");
        status = 0;
        goto cleanup;
    }
    if (!composed_source_rcp1_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                         document.layers[0].layer_id,
                                                                                         sample_x,
                                                                                         sample_y,
                                                                                         second_base_sample,
                                                                                         0),
                                        "rcp1_rewrite_base_lower_band")) {
        status = 0;
        goto cleanup;
    }
    lower_over_base = drawing_program_color_blend_samples(second_base_sample, lower_partial_sample, 70u);
    upper_over_lower = drawing_program_color_blend_samples(lower_over_base, upper_partial_sample, 60u);
    expected_second = drawing_program_color_blend_samples(upper_over_lower, top_partial_sample, 50u);
    if (!composed_source_rcp1_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                        &document,
                                                                                        &layer_rasters,
                                                                                        opacity,
                                                                                        DRAWING_PROGRAM_MAX_LAYERS,
                                                                                        3001u,
                                                                                        3100u,
                                                                                        &second_view),
                                        "rcp1_resolve_lower_band_second")) {
        status = 0;
        goto cleanup;
    }
    if (second_view.reused_cached_lower_stack != 0u ||
        second_view.reused_cached_prefix_stack != 0u ||
        second_view.reused_cached_suffix_stack != 1u ||
        second_view.recomposed_dirty_rect_only != 1u) {
        fprintf(stderr,
                "lifecycle_test: expected lower-band baseline to rebuild the base while reusing the cached upper suffix "
                "through dirty-rect-only recomposition got lower=%u prefix=%u suffix=%u dirty_only=%u\n",
                (unsigned)second_view.reused_cached_lower_stack,
                (unsigned)second_view.reused_cached_prefix_stack,
                (unsigned)second_view.reused_cached_suffix_stack,
                (unsigned)second_view.recomposed_dirty_rect_only);
        status = 0;
        goto cleanup;
    }
    if (!second_view.has_dirty_rect || second_view.dirty_rect_is_full ||
        second_view.dirty_x != sample_x || second_view.dirty_y != sample_y ||
        second_view.dirty_width != 1u || second_view.dirty_height != 1u) {
        fprintf(stderr, "lifecycle_test: expected lower-band baseline dirty rect to stay pinned to one edited sample\n");
        status = 0;
        goto cleanup;
    }
    if (second_view.samples[(size_t)sample_y * document.raster_width + sample_x] != expected_second) {
        fprintf(stderr, "lifecycle_test: expected lower-band baseline to preserve the correct visible blend after base edit\n");
        status = 0;
    }
cleanup:
    drawing_program_render_composed_source_dispose(&state);
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

int drawing_program_lifecycle_run_composed_source_rcp1_suite(void) {
    if (composed_source_rcp1_assert_top_band_edit_baseline() != 0) {
        return 1;
    }
    if (composed_source_rcp1_assert_lower_band_edit_baseline() != 0) {
        return 1;
    }
    return 0;
}
