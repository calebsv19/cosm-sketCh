#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_layer_raster.h"
#include "drawing_program/drawing_program_render_composed_source.h"
#include "drawing_program_lifecycle_composed_source_suite.h"

static int composed_source_expect_ok(CoreResult result, const char *label) {
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: %s failed: %s\n", label, result.message ? result.message : "(null)");
        return 0;
    }
    return 1;
}

static int composed_source_assert_direct_view_uses_no_storage(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramRenderComposedSourceState state;
    DrawingProgramRenderComposedSourceView view;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(&state, 0, sizeof(state));
    memset(&view, 0, sizeof(view));
    memset(opacity, 100, sizeof(opacity));
    if (!composed_source_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                   "composed_source_init_shape_direct")) {
        return 1;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                   "composed_source_init_layer_rasters_direct")) {
        return 1;
    }
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  101u,
                                                                                  201u,
                                                                                  &view),
                                   "composed_source_resolve_direct")) {
        status = 0;
        goto cleanup;
    }
    if (!view.samples || view.sample_count != document.raster_sample_count || view.used_composed_storage != 0u) {
        fprintf(stderr, "lifecycle_test: expected direct composed source view without scratch storage\n");
        status = 0;
    }
cleanup:
    drawing_program_render_composed_source_dispose(&state);
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

static int composed_source_assert_composed_view_uses_storage(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramRenderComposedSourceState state;
    DrawingProgramRenderComposedSourceView view;
    uint32_t layer_id = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(&state, 0, sizeof(state));
    memset(&view, 0, sizeof(view));
    memset(opacity, 100, sizeof(opacity));
    if (!composed_source_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                   "composed_source_init_shape_compose")) {
        return 1;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                   "composed_source_init_layer_rasters_compose")) {
        return 1;
    }
    if (!composed_source_expect_ok(drawing_program_document_add_layer(&document, "Top", &layer_id),
                                   "composed_source_add_layer_compose")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sync_document_layers(&layer_rasters, &document),
                                   "composed_source_sync_layers_compose")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   document.layers[0].layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       1u),
                                                                                   0),
                                   "composed_source_write_base_compose")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       7u),
                                                                                   0),
                                   "composed_source_write_top_compose")) {
        status = 0;
        goto cleanup;
    }
    opacity[1] = 50u;
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  102u,
                                                                                  202u,
                                                                                  &view),
                                   "composed_source_resolve_compose")) {
        status = 0;
        goto cleanup;
    }
    if (!view.samples || view.sample_count != document.raster_sample_count || view.used_composed_storage != 1u) {
        fprintf(stderr, "lifecycle_test: expected composed source view to use scratch storage\n");
        status = 0;
        goto cleanup;
    }
    if (state.composited_capacity != document.raster_sample_count) {
        fprintf(stderr, "lifecycle_test: expected composed source scratch capacity to match sample count\n");
        status = 0;
        goto cleanup;
    }
    if (view.samples[0] != drawing_program_color_blend_samples(drawing_program_color_value_from_index(1u),
                                                               drawing_program_color_value_from_index(7u),
                                                               50u)) {
        fprintf(stderr, "lifecycle_test: expected composed source scratch result to preserve blended sample\n");
        status = 0;
    }
cleanup:
    drawing_program_render_composed_source_dispose(&state);
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

static int composed_source_assert_cached_compose_reuses_until_keys_change(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramRenderComposedSourceState state;
    DrawingProgramRenderComposedSourceView first_view;
    DrawingProgramRenderComposedSourceView second_view;
    DrawingProgramRenderComposedSourceView third_view;
    uint32_t layer_id = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    DrawingProgramRasterSample expected_blend;
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(&state, 0, sizeof(state));
    memset(&first_view, 0, sizeof(first_view));
    memset(&second_view, 0, sizeof(second_view));
    memset(&third_view, 0, sizeof(third_view));
    memset(opacity, 100, sizeof(opacity));
    if (!composed_source_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                   "composed_source_init_shape_cached")) {
        return 1;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                   "composed_source_init_layer_rasters_cached")) {
        return 1;
    }
    if (!composed_source_expect_ok(drawing_program_document_add_layer(&document, "Top", &layer_id),
                                   "composed_source_add_layer_cached")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sync_document_layers(&layer_rasters, &document),
                                   "composed_source_sync_layers_cached")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   document.layers[0].layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       1u),
                                                                                   0),
                                   "composed_source_write_base_cached")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       7u),
                                                                                   0),
                                   "composed_source_write_top_cached")) {
        status = 0;
        goto cleanup;
    }
    opacity[1] = 50u;
    expected_blend = drawing_program_color_blend_samples(drawing_program_color_value_from_index(1u),
                                                         drawing_program_color_value_from_index(7u),
                                                         50u);
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  500u,
                                                                                  600u,
                                                                                  &first_view),
                                   "composed_source_resolve_cached_first")) {
        status = 0;
        goto cleanup;
    }
    if (first_view.reused_cached_compose != 0u || first_view.used_composed_storage != 1u) {
        fprintf(stderr, "lifecycle_test: expected first composed resolve to build fresh scratch storage\n");
        status = 0;
        goto cleanup;
    }
    state.composited_samples[0] = drawing_program_color_value_from_index(11u);
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  500u,
                                                                                  600u,
                                                                                  &second_view),
                                   "composed_source_resolve_cached_second")) {
        status = 0;
        goto cleanup;
    }
    if (second_view.reused_cached_compose != 1u || second_view.samples[0] != drawing_program_color_value_from_index(11u)) {
        fprintf(stderr, "lifecycle_test: expected second composed resolve to reuse cached scratch samples\n");
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  501u,
                                                                                  600u,
                                                                                  &third_view),
                                   "composed_source_resolve_cached_third")) {
        status = 0;
        goto cleanup;
    }
    if (third_view.reused_cached_compose != 0u || third_view.samples[0] != expected_blend) {
        fprintf(stderr, "lifecycle_test: expected changed content key to invalidate cached composed samples\n");
        status = 0;
    }
cleanup:
    drawing_program_render_composed_source_dispose(&state);
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

static int composed_source_assert_lower_stack_reuses_until_lower_layers_change(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramRenderComposedSourceState state;
    DrawingProgramRenderComposedSourceView first_view;
    DrawingProgramRenderComposedSourceView second_view;
    DrawingProgramRenderComposedSourceView third_view;
    uint32_t middle_layer_id = 0u;
    uint32_t top_layer_id = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    DrawingProgramRasterSample expected_second;
    DrawingProgramRasterSample expected_third;
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(&state, 0, sizeof(state));
    memset(&first_view, 0, sizeof(first_view));
    memset(&second_view, 0, sizeof(second_view));
    memset(&third_view, 0, sizeof(third_view));
    memset(opacity, 100, sizeof(opacity));
    if (!composed_source_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                   "composed_source_init_shape_lower_stack")) {
        return 1;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                   "composed_source_init_layer_rasters_lower_stack")) {
        return 1;
    }
    if (!composed_source_expect_ok(drawing_program_document_add_layer(&document, "Middle", &middle_layer_id),
                                   "composed_source_add_middle_lower_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_document_add_layer(&document, "Top", &top_layer_id),
                                   "composed_source_add_top_lower_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sync_document_layers(&layer_rasters, &document),
                                   "composed_source_sync_layers_lower_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   document.layers[0].layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       1u),
                                                                                   0),
                                   "composed_source_write_base_lower_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   middle_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       4u),
                                                                                   0),
                                   "composed_source_write_middle_lower_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   top_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       7u),
                                                                                   0),
                                   "composed_source_write_top_lower_stack")) {
        status = 0;
        goto cleanup;
    }
    opacity[2] = 50u;
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  700u,
                                                                                  800u,
                                                                                  &first_view),
                                   "composed_source_resolve_lower_stack_first")) {
        status = 0;
        goto cleanup;
    }
    if (first_view.reused_cached_lower_stack != 0u || state.has_cached_lower_stack == 0u) {
        fprintf(stderr, "lifecycle_test: expected first lower-stack resolve to populate but not reuse cache\n");
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   top_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       8u),
                                                                                   0),
                                   "composed_source_rewrite_top_lower_stack")) {
        status = 0;
        goto cleanup;
    }
    expected_second = drawing_program_color_blend_samples(drawing_program_color_value_from_index(4u),
                                                          drawing_program_color_value_from_index(8u),
                                                          50u);
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  701u,
                                                                                  800u,
                                                                                  &second_view),
                                   "composed_source_resolve_lower_stack_second")) {
        status = 0;
        goto cleanup;
    }
    if (second_view.reused_cached_compose != 0u || second_view.reused_cached_lower_stack != 1u) {
        fprintf(stderr, "lifecycle_test: expected top-only change to reuse cached lower stack without reusing full compose\n");
        status = 0;
        goto cleanup;
    }
    if (second_view.samples[0] != expected_second) {
        fprintf(stderr, "lifecycle_test: expected top-only change to reblend over cached lower stack\n");
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   middle_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       5u),
                                                                                   0),
                                   "composed_source_rewrite_middle_lower_stack")) {
        status = 0;
        goto cleanup;
    }
    expected_third = drawing_program_color_blend_samples(drawing_program_color_value_from_index(5u),
                                                         drawing_program_color_value_from_index(8u),
                                                         50u);
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  702u,
                                                                                  800u,
                                                                                  &third_view),
                                   "composed_source_resolve_lower_stack_third")) {
        status = 0;
        goto cleanup;
    }
    if (third_view.reused_cached_compose != 0u || third_view.reused_cached_lower_stack != 0u) {
        fprintf(stderr, "lifecycle_test: expected lower-layer change to invalidate cached lower stack\n");
        status = 0;
        goto cleanup;
    }
    if (third_view.samples[0] != expected_third) {
        fprintf(stderr, "lifecycle_test: expected lower-layer change to rebuild cached lower stack output\n");
        status = 0;
    }
cleanup:
    drawing_program_render_composed_source_dispose(&state);
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

static int composed_source_assert_two_partial_suffix_reuses_lower_stack(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramRenderComposedSourceState state;
    DrawingProgramRenderComposedSourceView first_view;
    DrawingProgramRenderComposedSourceView second_view;
    DrawingProgramRenderComposedSourceView third_view;
    uint32_t middle_layer_id = 0u;
    uint32_t upper_layer_id = 0u;
    uint32_t top_layer_id = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    DrawingProgramRasterSample middle_over_base;
    DrawingProgramRasterSample expected_second;
    DrawingProgramRasterSample expected_third;
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(&state, 0, sizeof(state));
    memset(&first_view, 0, sizeof(first_view));
    memset(&second_view, 0, sizeof(second_view));
    memset(&third_view, 0, sizeof(third_view));
    memset(opacity, 100, sizeof(opacity));
    if (!composed_source_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                   "composed_source_init_shape_two_partial")) {
        return 1;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                   "composed_source_init_layer_rasters_two_partial")) {
        return 1;
    }
    if (!composed_source_expect_ok(drawing_program_document_add_layer(&document, "Middle", &middle_layer_id),
                                   "composed_source_add_middle_two_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_document_add_layer(&document, "Upper", &upper_layer_id),
                                   "composed_source_add_upper_two_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_document_add_layer(&document, "Top", &top_layer_id),
                                   "composed_source_add_top_two_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sync_document_layers(&layer_rasters, &document),
                                   "composed_source_sync_layers_two_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   document.layers[0].layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       1u),
                                                                                   0),
                                   "composed_source_write_base_two_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   middle_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       4u),
                                                                                   0),
                                   "composed_source_write_middle_two_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   upper_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       6u),
                                                                                   0),
                                   "composed_source_write_upper_two_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   top_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       8u),
                                                                                   0),
                                   "composed_source_write_top_two_partial")) {
        status = 0;
        goto cleanup;
    }
    opacity[2] = 60u;
    opacity[3] = 50u;
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  900u,
                                                                                  901u,
                                                                                  &first_view),
                                   "composed_source_resolve_two_partial_first")) {
        status = 0;
        goto cleanup;
    }
    if (first_view.reused_cached_lower_stack != 0u || state.has_cached_lower_stack == 0u) {
        fprintf(stderr, "lifecycle_test: expected first two-partial resolve to populate but not reuse lower stack\n");
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   top_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       9u),
                                                                                   0),
                                   "composed_source_rewrite_top_two_partial")) {
        status = 0;
        goto cleanup;
    }
    middle_over_base = drawing_program_color_blend_samples(drawing_program_color_value_from_index(4u),
                                                           drawing_program_color_value_from_index(6u),
                                                           60u);
    expected_second = drawing_program_color_blend_samples(middle_over_base,
                                                          drawing_program_color_value_from_index(9u),
                                                          50u);
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  902u,
                                                                                  901u,
                                                                                  &second_view),
                                   "composed_source_resolve_two_partial_second")) {
        status = 0;
        goto cleanup;
    }
    if (second_view.reused_cached_compose != 0u || second_view.reused_cached_lower_stack != 1u) {
        fprintf(stderr, "lifecycle_test: expected top partial suffix change to reuse cached lower stack\n");
        status = 0;
        goto cleanup;
    }
    if (second_view.samples[0] != expected_second) {
        fprintf(stderr, "lifecycle_test: expected top partial suffix change to rebuild correct visible blend\n");
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   middle_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       5u),
                                                                                   0),
                                   "composed_source_rewrite_middle_two_partial")) {
        status = 0;
        goto cleanup;
    }
    middle_over_base = drawing_program_color_blend_samples(drawing_program_color_value_from_index(5u),
                                                           drawing_program_color_value_from_index(6u),
                                                           60u);
    expected_third = drawing_program_color_blend_samples(middle_over_base,
                                                         drawing_program_color_value_from_index(9u),
                                                         50u);
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  903u,
                                                                                  901u,
                                                                                  &third_view),
                                   "composed_source_resolve_two_partial_third")) {
        status = 0;
        goto cleanup;
    }
    if (third_view.reused_cached_compose != 0u || third_view.reused_cached_lower_stack != 0u) {
        fprintf(stderr, "lifecycle_test: expected lower-layer change below two-partial suffix to invalidate lower stack\n");
        status = 0;
        goto cleanup;
    }
    if (third_view.samples[0] != expected_third) {
        fprintf(stderr, "lifecycle_test: expected lower-layer change below two-partial suffix to rebuild correct visible blend\n");
        status = 0;
    }
cleanup:
    drawing_program_render_composed_source_dispose(&state);
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

static int composed_source_assert_three_partial_suffix_reuses_lower_stack(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramRenderComposedSourceState state;
    DrawingProgramRenderComposedSourceView first_view;
    DrawingProgramRenderComposedSourceView second_view;
    DrawingProgramRenderComposedSourceView third_view;
    uint32_t lower_partial_layer_id = 0u;
    uint32_t upper_partial_layer_id = 0u;
    uint32_t top_partial_layer_id = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    DrawingProgramRasterSample lower_over_base;
    DrawingProgramRasterSample upper_over_lower;
    DrawingProgramRasterSample expected_second;
    DrawingProgramRasterSample expected_third;
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(&state, 0, sizeof(state));
    memset(&first_view, 0, sizeof(first_view));
    memset(&second_view, 0, sizeof(second_view));
    memset(&third_view, 0, sizeof(third_view));
    memset(opacity, 100, sizeof(opacity));
    if (!composed_source_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                   "composed_source_init_shape_three_partial")) {
        return 1;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                   "composed_source_init_layer_rasters_three_partial")) {
        return 1;
    }
    if (!composed_source_expect_ok(drawing_program_document_add_layer(&document,
                                                                      "LowerPartial",
                                                                      &lower_partial_layer_id),
                                   "composed_source_add_lower_partial_three_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_document_add_layer(&document,
                                                                      "UpperPartial",
                                                                      &upper_partial_layer_id),
                                   "composed_source_add_upper_partial_three_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_document_add_layer(&document,
                                                                      "TopPartial",
                                                                      &top_partial_layer_id),
                                   "composed_source_add_top_partial_three_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sync_document_layers(&layer_rasters, &document),
                                   "composed_source_sync_layers_three_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   document.layers[0].layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       1u),
                                                                                   0),
                                   "composed_source_write_base_three_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   lower_partial_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       4u),
                                                                                   0),
                                   "composed_source_write_lower_partial_three_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   upper_partial_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       6u),
                                                                                   0),
                                   "composed_source_write_upper_partial_three_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   top_partial_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       8u),
                                                                                   0),
                                   "composed_source_write_top_partial_three_partial")) {
        status = 0;
        goto cleanup;
    }
    opacity[1] = 70u;
    opacity[2] = 60u;
    opacity[3] = 50u;
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  1000u,
                                                                                  1001u,
                                                                                  &first_view),
                                   "composed_source_resolve_three_partial_first")) {
        status = 0;
        goto cleanup;
    }
    if (first_view.reused_cached_lower_stack != 0u || state.has_cached_lower_stack == 0u) {
        fprintf(stderr, "lifecycle_test: expected first three-partial resolve to populate but not reuse lower stack\n");
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   top_partial_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       9u),
                                                                                   0),
                                   "composed_source_rewrite_top_three_partial")) {
        status = 0;
        goto cleanup;
    }
    lower_over_base = drawing_program_color_blend_samples(drawing_program_color_value_from_index(1u),
                                                          drawing_program_color_value_from_index(4u),
                                                          70u);
    upper_over_lower = drawing_program_color_blend_samples(lower_over_base,
                                                           drawing_program_color_value_from_index(6u),
                                                           60u);
    expected_second = drawing_program_color_blend_samples(upper_over_lower,
                                                          drawing_program_color_value_from_index(9u),
                                                          50u);
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  1002u,
                                                                                  1001u,
                                                                                  &second_view),
                                   "composed_source_resolve_three_partial_second")) {
        status = 0;
        goto cleanup;
    }
    if (second_view.reused_cached_compose != 0u || second_view.reused_cached_lower_stack != 1u) {
        fprintf(stderr, "lifecycle_test: expected three-partial top change to reuse cached lower stack\n");
        status = 0;
        goto cleanup;
    }
    if (second_view.samples[0] != expected_second) {
        fprintf(stderr, "lifecycle_test: expected three-partial top change to rebuild correct visible blend\n");
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   document.layers[0].layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       2u),
                                                                                   0),
                                   "composed_source_rewrite_base_three_partial")) {
        status = 0;
        goto cleanup;
    }
    lower_over_base = drawing_program_color_blend_samples(drawing_program_color_value_from_index(2u),
                                                          drawing_program_color_value_from_index(4u),
                                                          70u);
    upper_over_lower = drawing_program_color_blend_samples(lower_over_base,
                                                           drawing_program_color_value_from_index(6u),
                                                           60u);
    expected_third = drawing_program_color_blend_samples(upper_over_lower,
                                                         drawing_program_color_value_from_index(9u),
                                                         50u);
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  1003u,
                                                                                  1001u,
                                                                                  &third_view),
                                   "composed_source_resolve_three_partial_third")) {
        status = 0;
        goto cleanup;
    }
    if (third_view.reused_cached_compose != 0u || third_view.reused_cached_lower_stack != 0u) {
        fprintf(stderr,
                "lifecycle_test: expected lower-layer change below three-partial suffix to invalidate lower stack\n");
        status = 0;
        goto cleanup;
    }
    if (third_view.samples[0] != expected_third) {
        fprintf(stderr,
                "lifecycle_test: expected lower-layer change below three-partial suffix to rebuild correct visible blend\n");
        status = 0;
    }
cleanup:
    drawing_program_render_composed_source_dispose(&state);
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

static int composed_source_assert_prefix_stack_reuses_until_lower_suffix_changes(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramRenderComposedSourceState state;
    DrawingProgramRenderComposedSourceView first_view;
    DrawingProgramRenderComposedSourceView second_view;
    DrawingProgramRenderComposedSourceView third_view;
    uint32_t lower_partial_layer_id = 0u;
    uint32_t upper_partial_layer_id = 0u;
    uint32_t top_partial_layer_id = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    DrawingProgramRasterSample lower_over_base;
    DrawingProgramRasterSample upper_over_lower;
    DrawingProgramRasterSample expected_second;
    DrawingProgramRasterSample expected_third;
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(&state, 0, sizeof(state));
    memset(&first_view, 0, sizeof(first_view));
    memset(&second_view, 0, sizeof(second_view));
    memset(&third_view, 0, sizeof(third_view));
    memset(opacity, 100, sizeof(opacity));
    if (!composed_source_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                   "composed_source_init_shape_prefix_stack")) {
        return 1;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                   "composed_source_init_layer_rasters_prefix_stack")) {
        return 1;
    }
    if (!composed_source_expect_ok(drawing_program_document_add_layer(&document,
                                                                      "LowerPartial",
                                                                      &lower_partial_layer_id),
                                   "composed_source_add_lower_partial_prefix_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_document_add_layer(&document,
                                                                      "UpperPartial",
                                                                      &upper_partial_layer_id),
                                   "composed_source_add_upper_partial_prefix_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_document_add_layer(&document,
                                                                      "TopPartial",
                                                                      &top_partial_layer_id),
                                   "composed_source_add_top_partial_prefix_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sync_document_layers(&layer_rasters, &document),
                                   "composed_source_sync_layers_prefix_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   document.layers[0].layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       1u),
                                                                                   0),
                                   "composed_source_write_base_prefix_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   lower_partial_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       4u),
                                                                                   0),
                                   "composed_source_write_lower_partial_prefix_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   upper_partial_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       6u),
                                                                                   0),
                                   "composed_source_write_upper_partial_prefix_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   top_partial_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       8u),
                                                                                   0),
                                   "composed_source_write_top_partial_prefix_stack")) {
        status = 0;
        goto cleanup;
    }
    opacity[1] = 70u;
    opacity[2] = 60u;
    opacity[3] = 50u;
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  1100u,
                                                                                  1101u,
                                                                                  &first_view),
                                   "composed_source_resolve_prefix_stack_first")) {
        status = 0;
        goto cleanup;
    }
    if (first_view.reused_cached_lower_stack != 0u ||
        first_view.reused_cached_prefix_stack != 0u ||
        state.has_cached_prefix_stack == 0u) {
        fprintf(stderr, "lifecycle_test: expected first prefix-stack resolve to populate but not reuse caches\n");
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   top_partial_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       9u),
                                                                                   0),
                                   "composed_source_rewrite_top_prefix_stack")) {
        status = 0;
        goto cleanup;
    }
    lower_over_base = drawing_program_color_blend_samples(drawing_program_color_value_from_index(1u),
                                                          drawing_program_color_value_from_index(4u),
                                                          70u);
    upper_over_lower = drawing_program_color_blend_samples(lower_over_base,
                                                           drawing_program_color_value_from_index(6u),
                                                           60u);
    expected_second = drawing_program_color_blend_samples(upper_over_lower,
                                                          drawing_program_color_value_from_index(9u),
                                                          50u);
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  1102u,
                                                                                  1101u,
                                                                                  &second_view),
                                   "composed_source_resolve_prefix_stack_second")) {
        status = 0;
        goto cleanup;
    }
    if (second_view.reused_cached_compose != 0u ||
        second_view.reused_cached_lower_stack != 1u ||
        second_view.reused_cached_prefix_stack != 1u) {
        fprintf(stderr, "lifecycle_test: expected top-only prefix-stack change to reuse lower and prefix caches\n");
        status = 0;
        goto cleanup;
    }
    if (second_view.samples[0] != expected_second) {
        fprintf(stderr, "lifecycle_test: expected top-only prefix-stack change to preserve visible blend\n");
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   upper_partial_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       7u),
                                                                                   0),
                                   "composed_source_rewrite_upper_partial_prefix_stack")) {
        status = 0;
        goto cleanup;
    }
    lower_over_base = drawing_program_color_blend_samples(drawing_program_color_value_from_index(1u),
                                                          drawing_program_color_value_from_index(4u),
                                                          70u);
    upper_over_lower = drawing_program_color_blend_samples(lower_over_base,
                                                           drawing_program_color_value_from_index(7u),
                                                           60u);
    expected_third = drawing_program_color_blend_samples(upper_over_lower,
                                                         drawing_program_color_value_from_index(9u),
                                                         50u);
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  1103u,
                                                                                  1101u,
                                                                                  &third_view),
                                   "composed_source_resolve_prefix_stack_third")) {
        status = 0;
        goto cleanup;
    }
    if (third_view.reused_cached_compose != 0u ||
        third_view.reused_cached_lower_stack != 1u ||
        third_view.reused_cached_prefix_stack != 0u) {
        fprintf(stderr, "lifecycle_test: expected lower suffix change to reuse lower stack but invalidate prefix cache\n");
        status = 0;
        goto cleanup;
    }
    if (third_view.samples[0] != expected_third) {
        fprintf(stderr, "lifecycle_test: expected lower suffix change to rebuild correct prefix-stack blend\n");
        status = 0;
    }
cleanup:
    drawing_program_render_composed_source_dispose(&state);
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

static int composed_source_assert_suffix_stack_reuses_until_suffix_changes(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramRenderComposedSourceState state;
    DrawingProgramRenderComposedSourceView first_view;
    DrawingProgramRenderComposedSourceView second_view;
    DrawingProgramRenderComposedSourceView third_view;
    uint32_t lower_partial_layer_id = 0u;
    uint32_t upper_partial_layer_id = 0u;
    uint32_t top_partial_layer_id = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    DrawingProgramRasterSample lower_over_base;
    DrawingProgramRasterSample upper_over_lower;
    DrawingProgramRasterSample expected_second;
    DrawingProgramRasterSample expected_third;
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(&state, 0, sizeof(state));
    memset(&first_view, 0, sizeof(first_view));
    memset(&second_view, 0, sizeof(second_view));
    memset(&third_view, 0, sizeof(third_view));
    memset(opacity, 100, sizeof(opacity));
    if (!composed_source_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                   "composed_source_init_shape_suffix_stack")) {
        return 1;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                   "composed_source_init_layer_rasters_suffix_stack")) {
        return 1;
    }
    if (!composed_source_expect_ok(drawing_program_document_add_layer(&document,
                                                                      "LowerPartial",
                                                                      &lower_partial_layer_id),
                                   "composed_source_add_lower_partial_suffix_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_document_add_layer(&document,
                                                                      "UpperPartial",
                                                                      &upper_partial_layer_id),
                                   "composed_source_add_upper_partial_suffix_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_document_add_layer(&document,
                                                                      "TopPartial",
                                                                      &top_partial_layer_id),
                                   "composed_source_add_top_partial_suffix_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sync_document_layers(&layer_rasters, &document),
                                   "composed_source_sync_layers_suffix_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   document.layers[0].layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       1u),
                                                                                   0),
                                   "composed_source_write_base_suffix_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   lower_partial_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       4u),
                                                                                   0),
                                   "composed_source_write_lower_partial_suffix_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   upper_partial_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       6u),
                                                                                   0),
                                   "composed_source_write_upper_partial_suffix_stack")) {
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   top_partial_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       8u),
                                                                                   0),
                                   "composed_source_write_top_partial_suffix_stack")) {
        status = 0;
        goto cleanup;
    }
    opacity[1] = 70u;
    opacity[2] = 60u;
    opacity[3] = 50u;
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  1200u,
                                                                                  1201u,
                                                                                  &first_view),
                                   "composed_source_resolve_suffix_stack_first")) {
        status = 0;
        goto cleanup;
    }
    if (first_view.reused_cached_suffix_stack != 0u || state.has_cached_suffix_stack == 0u) {
        fprintf(stderr, "lifecycle_test: expected first suffix-stack resolve to populate but not reuse suffix cache\n");
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   document.layers[0].layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       2u),
                                                                                   0),
                                   "composed_source_rewrite_base_suffix_stack")) {
        status = 0;
        goto cleanup;
    }
    lower_over_base = drawing_program_color_blend_samples(drawing_program_color_value_from_index(2u),
                                                          drawing_program_color_value_from_index(4u),
                                                          70u);
    upper_over_lower = drawing_program_color_blend_samples(lower_over_base,
                                                           drawing_program_color_value_from_index(6u),
                                                           60u);
    expected_second = drawing_program_color_blend_samples(upper_over_lower,
                                                          drawing_program_color_value_from_index(8u),
                                                          50u);
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  1202u,
                                                                                  1201u,
                                                                                  &second_view),
                                   "composed_source_resolve_suffix_stack_second")) {
        status = 0;
        goto cleanup;
    }
    if (second_view.reused_cached_lower_stack != 0u ||
        second_view.reused_cached_prefix_stack != 0u ||
        second_view.reused_cached_suffix_stack != 1u) {
        fprintf(stderr, "lifecycle_test: expected lower-band change to rebuild lower stack but reuse suffix cache\n");
        status = 0;
        goto cleanup;
    }
    if (second_view.samples[0] != expected_second) {
        fprintf(stderr, "lifecycle_test: expected lower-band change to preserve visible blend through suffix cache\n");
        status = 0;
        goto cleanup;
    }
    if (!composed_source_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                   top_partial_layer_id,
                                                                                   0u,
                                                                                   0u,
                                                                                   drawing_program_color_value_from_index(
                                                                                       9u),
                                                                                   0),
                                   "composed_source_rewrite_top_suffix_stack")) {
        status = 0;
        goto cleanup;
    }
    lower_over_base = drawing_program_color_blend_samples(drawing_program_color_value_from_index(2u),
                                                          drawing_program_color_value_from_index(4u),
                                                          70u);
    upper_over_lower = drawing_program_color_blend_samples(lower_over_base,
                                                           drawing_program_color_value_from_index(6u),
                                                           60u);
    expected_third = drawing_program_color_blend_samples(upper_over_lower,
                                                         drawing_program_color_value_from_index(9u),
                                                         50u);
    if (!composed_source_expect_ok(drawing_program_render_composed_source_resolve(&state,
                                                                                  &document,
                                                                                  &layer_rasters,
                                                                                  opacity,
                                                                                  DRAWING_PROGRAM_MAX_LAYERS,
                                                                                  1203u,
                                                                                  1201u,
                                                                                  &third_view),
                                   "composed_source_resolve_suffix_stack_third")) {
        status = 0;
        goto cleanup;
    }
    if (third_view.reused_cached_lower_stack != 1u ||
        third_view.reused_cached_prefix_stack != 1u ||
        third_view.reused_cached_suffix_stack != 0u) {
        fprintf(stderr,
                "lifecycle_test: expected top-band change to reuse lower and prefix caches while invalidating suffix cache\n");
        status = 0;
        goto cleanup;
    }
    if (third_view.samples[0] != expected_third) {
        fprintf(stderr, "lifecycle_test: expected top-band change to rebuild correct suffix-cache blend\n");
        status = 0;
    }
cleanup:
    drawing_program_render_composed_source_dispose(&state);
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

int drawing_program_lifecycle_run_composed_source_suite(void) {
    if (composed_source_assert_direct_view_uses_no_storage() != 0) {
        return 1;
    }
    if (composed_source_assert_composed_view_uses_storage() != 0) {
        return 1;
    }
    if (composed_source_assert_cached_compose_reuses_until_keys_change() != 0) {
        return 1;
    }
    if (composed_source_assert_lower_stack_reuses_until_lower_layers_change() != 0) {
        return 1;
    }
    if (composed_source_assert_two_partial_suffix_reuses_lower_stack() != 0) {
        return 1;
    }
    if (composed_source_assert_three_partial_suffix_reuses_lower_stack() != 0) {
        return 1;
    }
    if (composed_source_assert_prefix_stack_reuses_until_lower_suffix_changes() != 0) {
        return 1;
    }
    if (composed_source_assert_suffix_stack_reuses_until_suffix_changes() != 0) {
        return 1;
    }
    return 0;
}
