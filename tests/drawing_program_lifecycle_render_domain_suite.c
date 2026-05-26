#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_layer_raster.h"
#include "drawing_program/drawing_program_render_domain.h"
#include "drawing_program_lifecycle_render_domain_suite.h"

static int render_domain_expect_ok(CoreResult result, const char *label) {
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: %s failed: %s\n", label, result.message ? result.message : "(null)");
        return 0;
    }
    return 1;
}

static int render_domain_assert_direct_legacy_layer(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    const DrawingProgramRasterSample *expected = 0;
    const DrawingProgramRasterSample *resolved = 0;
    uint32_t expected_count = 0u;
    uint32_t resolved_count = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(opacity, 100, sizeof(opacity));
    if (!render_domain_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                 "render_domain_init_shape")) {
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                 "render_domain_init_layer_rasters")) {
        return 1;
    }
    document.raster_samples[0] = drawing_program_color_value_from_index(7u);
    if (!render_domain_expect_ok(
            drawing_program_layer_raster_store_export_layer(&layer_rasters,
                                                            document.layers[0].layer_id,
                                                            &expected,
                                                            &expected_count),
            "render_domain_export_base_layer")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(
            drawing_program_render_resolve_direct_visible_samples_with_layer_opacity(&document,
                                                                                     &layer_rasters,
                                                                                     opacity,
                                                                                     DRAWING_PROGRAM_MAX_LAYERS,
                                                                                     &resolved,
                                                                                     &resolved_count),
            "render_domain_resolve_direct_legacy")) {
        status = 0;
        goto cleanup;
    }
    if (resolved != expected || resolved_count != expected_count) {
        fprintf(stderr,
                "lifecycle_test: expected direct base layer pointer count=%u expected=%u\n",
                (unsigned)resolved_count,
                (unsigned)expected_count);
        status = 0;
    }
cleanup:
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

static int render_domain_assert_direct_layer_store_layer(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    const DrawingProgramRasterSample *expected = 0;
    const DrawingProgramRasterSample *resolved = 0;
    uint32_t expected_count = 0u;
    uint32_t resolved_count = 0u;
    uint32_t layer_id = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(opacity, 100, sizeof(opacity));
    if (!render_domain_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                 "render_domain_init_shape_store")) {
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                 "render_domain_init_layer_rasters_store")) {
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_document_add_layer(&document, "Top", &layer_id),
                                 "render_domain_add_layer")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sync_document_layers(&layer_rasters, &document),
                                 "render_domain_sync_layers")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                 layer_id,
                                                                                 0u,
                                                                                 0u,
                                                                                 drawing_program_color_value_from_index(
                                                                                     9u),
                                                                                 0),
                                 "render_domain_write_top_layer")) {
        status = 0;
        goto cleanup;
    }
    opacity[0] = 0u;
    opacity[1] = 100u;
    if (!render_domain_expect_ok(
            drawing_program_layer_raster_store_export_layer(&layer_rasters, layer_id, &expected, &expected_count),
            "render_domain_export_top_layer")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(
            drawing_program_render_resolve_direct_visible_samples_with_layer_opacity(&document,
                                                                                     &layer_rasters,
                                                                                     opacity,
                                                                                     DRAWING_PROGRAM_MAX_LAYERS,
                                                                                     &resolved,
                                                                                     &resolved_count),
            "render_domain_resolve_direct_store")) {
        status = 0;
        goto cleanup;
    }
    if (resolved != expected || resolved_count != expected_count) {
        fprintf(stderr,
                "lifecycle_test: expected direct layer-store pointer count=%u expected=%u\n",
                (unsigned)resolved_count,
                (unsigned)expected_count);
        status = 0;
    }
cleanup:
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

static int render_domain_assert_compose_required_for_multiple_layers(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    const DrawingProgramRasterSample *resolved = 0;
    uint32_t resolved_count = 0u;
    uint32_t layer_id = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    CoreResult result;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(opacity, 100, sizeof(opacity));
    if (!render_domain_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                 "render_domain_init_shape_multi")) {
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                 "render_domain_init_layer_rasters_multi")) {
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_document_add_layer(&document, "Top", &layer_id),
                                 "render_domain_add_layer_multi")) {
        drawing_program_layer_raster_store_dispose(&layer_rasters);
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sync_document_layers(&layer_rasters, &document),
                                 "render_domain_sync_layers_multi")) {
        drawing_program_layer_raster_store_dispose(&layer_rasters);
        return 1;
    }
    result = drawing_program_render_resolve_direct_visible_samples_with_layer_opacity(&document,
                                                                                      &layer_rasters,
                                                                                      opacity,
                                                                                      DRAWING_PROGRAM_MAX_LAYERS,
                                                                                      &resolved,
                                                                                      &resolved_count);
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    if (result.code == CORE_OK) {
        fprintf(stderr, "lifecycle_test: expected multiple visible layers to require compose\n");
        return 1;
    }
    return 0;
}

static int render_domain_assert_compose_required_for_partial_opacity(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    const DrawingProgramRasterSample *resolved = 0;
    uint32_t resolved_count = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    CoreResult result;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(opacity, 100, sizeof(opacity));
    opacity[0] = 84u;
    if (!render_domain_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                 "render_domain_init_shape_opacity")) {
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                 "render_domain_init_layer_rasters_opacity")) {
        return 1;
    }
    result = drawing_program_render_resolve_direct_visible_samples_with_layer_opacity(&document,
                                                                                      &layer_rasters,
                                                                                      opacity,
                                                                                      DRAWING_PROGRAM_MAX_LAYERS,
                                                                                      &resolved,
                                                                                      &resolved_count);
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    if (result.code == CORE_OK) {
        fprintf(stderr, "lifecycle_test: expected partial-opacity layer to require compose\n");
        return 1;
    }
    return 0;
}

static int render_domain_assert_multi_layer_full_opacity_prefers_topmost_nontransparent(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramRasterSample composed[64];
    uint32_t top_layer_id = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    CoreResult result;
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(composed, 0, sizeof(composed));
    memset(opacity, 100, sizeof(opacity));
    if (!render_domain_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                 "render_domain_init_shape_full_opacity")) {
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                 "render_domain_init_layer_rasters_full_opacity")) {
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_document_add_layer(&document, "Top", &top_layer_id),
                                 "render_domain_add_layer_full_opacity")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sync_document_layers(&layer_rasters, &document),
                                 "render_domain_sync_layers_full_opacity")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                 document.layers[0].layer_id,
                                                                                 0u,
                                                                                 0u,
                                                                                 drawing_program_color_value_from_index(
                                                                                     2u),
                                                                                 0),
                                 "render_domain_write_base_layer_sample0")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                 document.layers[0].layer_id,
                                                                                 1u,
                                                                                 0u,
                                                                                 drawing_program_color_value_from_index(
                                                                                     3u),
                                                                                 0),
                                 "render_domain_write_base_layer_sample1")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                 top_layer_id,
                                                                                 1u,
                                                                                 0u,
                                                                                 drawing_program_color_value_from_index(
                                                                                     6u),
                                                                                 0),
                                 "render_domain_write_top_layer_full_opacity")) {
        status = 0;
        goto cleanup;
    }
    result = drawing_program_render_compose_visible_samples_with_layer_opacity(&document,
                                                                               &layer_rasters,
                                                                               opacity,
                                                                               DRAWING_PROGRAM_MAX_LAYERS,
                                                                               composed,
                                                                               64u);
    if (!render_domain_expect_ok(result, "render_domain_compose_full_opacity")) {
        status = 0;
        goto cleanup;
    }
    if (composed[0] != drawing_program_color_value_from_index(2u)) {
        fprintf(stderr, "lifecycle_test: expected transparent top sample to preserve lower layer at sample 0\n");
        status = 0;
    }
    if (composed[1] != drawing_program_color_value_from_index(6u)) {
        fprintf(stderr, "lifecycle_test: expected opaque top sample to win at sample 1\n");
        status = 0;
    }
cleanup:
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

static int render_domain_assert_zero_opacity_layer_is_pruned_from_compose(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramRasterSample composed = 0u;
    uint32_t top_layer_id = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    CoreResult result;
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(opacity, 100, sizeof(opacity));
    if (!render_domain_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                 "render_domain_init_shape_zero_opacity")) {
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                 "render_domain_init_layer_rasters_zero_opacity")) {
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_document_add_layer(&document, "Top", &top_layer_id),
                                 "render_domain_add_layer_zero_opacity")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sync_document_layers(&layer_rasters, &document),
                                 "render_domain_sync_layers_zero_opacity")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                 document.layers[0].layer_id,
                                                                                 0u,
                                                                                 0u,
                                                                                 drawing_program_color_value_from_index(
                                                                                     4u),
                                                                                 0),
                                 "render_domain_write_base_layer_zero_opacity")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                 top_layer_id,
                                                                                 0u,
                                                                                 0u,
                                                                                 drawing_program_color_value_from_index(
                                                                                     7u),
                                                                                 0),
                                 "render_domain_write_top_layer_zero_opacity")) {
        status = 0;
        goto cleanup;
    }
    opacity[1] = 0u;
    result = drawing_program_render_compose_visible_sample_with_layer_opacity(&document,
                                                                              &layer_rasters,
                                                                              opacity,
                                                                              DRAWING_PROGRAM_MAX_LAYERS,
                                                                              0u,
                                                                              0u,
                                                                              &composed);
    if (!render_domain_expect_ok(result, "render_domain_compose_zero_opacity")) {
        status = 0;
        goto cleanup;
    }
    if (composed != drawing_program_color_value_from_index(4u)) {
        fprintf(stderr, "lifecycle_test: expected zero-opacity top layer to be pruned from compose\n");
        status = 0;
    }
cleanup:
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

static int render_domain_assert_single_partial_opacity_layer_composes_uniformly(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramRasterSample composed[64];
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    CoreResult result;
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(composed, 0, sizeof(composed));
    memset(opacity, 100, sizeof(opacity));
    opacity[0] = 40u;
    if (!render_domain_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                 "render_domain_init_shape_single_partial")) {
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                 "render_domain_init_layer_rasters_single_partial")) {
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                 document.layers[0].layer_id,
                                                                                 0u,
                                                                                 0u,
                                                                                 drawing_program_color_value_from_index(
                                                                                     5u),
                                                                                 0),
                                 "render_domain_write_partial_sample")) {
        status = 0;
        goto cleanup;
    }
    result = drawing_program_render_compose_visible_samples_with_layer_opacity(&document,
                                                                               &layer_rasters,
                                                                               opacity,
                                                                               DRAWING_PROGRAM_MAX_LAYERS,
                                                                               composed,
                                                                               64u);
    if (!render_domain_expect_ok(result, "render_domain_compose_single_partial")) {
        status = 0;
        goto cleanup;
    }
    if (composed[0] != drawing_program_color_blend_samples(drawing_program_color_eraser_value(),
                                                           drawing_program_color_value_from_index(5u),
                                                           40u)) {
        fprintf(stderr, "lifecycle_test: expected single partial-opacity layer to blend against transparent\n");
        status = 0;
    }
    if (composed[1] != drawing_program_color_eraser_value()) {
        fprintf(stderr, "lifecycle_test: expected untouched transparent sample to stay transparent\n");
        status = 0;
    }
cleanup:
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

static int render_domain_assert_single_partial_layer_in_mixed_stack_preserves_semantics(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramRasterSample composed[64];
    uint32_t middle_layer_id = 0u;
    uint32_t top_layer_id = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    DrawingProgramRasterSample expected_partial_over_base;
    CoreResult result;
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(composed, 0, sizeof(composed));
    memset(opacity, 100, sizeof(opacity));
    if (!render_domain_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                 "render_domain_init_shape_mixed_partial")) {
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                 "render_domain_init_layer_rasters_mixed_partial")) {
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_document_add_layer(&document, "Middle", &middle_layer_id),
                                 "render_domain_add_middle_layer")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_document_add_layer(&document, "Top", &top_layer_id),
                                 "render_domain_add_top_layer")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sync_document_layers(&layer_rasters, &document),
                                 "render_domain_sync_layers_mixed_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                 document.layers[0].layer_id,
                                                                                 0u,
                                                                                 0u,
                                                                                 drawing_program_color_value_from_index(
                                                                                     1u),
                                                                                 0),
                                 "render_domain_write_base_mixed_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                 middle_layer_id,
                                                                                 0u,
                                                                                 0u,
                                                                                 drawing_program_color_value_from_index(
                                                                                     6u),
                                                                                 0),
                                 "render_domain_write_middle_mixed_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                 top_layer_id,
                                                                                 1u,
                                                                                 0u,
                                                                                 drawing_program_color_value_from_index(
                                                                                     7u),
                                                                                 0),
                                 "render_domain_write_top_mixed_partial")) {
        status = 0;
        goto cleanup;
    }
    opacity[1] = 50u;
    expected_partial_over_base =
        drawing_program_color_blend_samples(drawing_program_color_value_from_index(1u),
                                            drawing_program_color_value_from_index(6u),
                                            50u);
    result = drawing_program_render_compose_visible_samples_with_layer_opacity(&document,
                                                                               &layer_rasters,
                                                                               opacity,
                                                                               DRAWING_PROGRAM_MAX_LAYERS,
                                                                               composed,
                                                                               64u);
    if (!render_domain_expect_ok(result, "render_domain_compose_mixed_partial")) {
        status = 0;
        goto cleanup;
    }
    if (composed[0] != expected_partial_over_base) {
        fprintf(stderr, "lifecycle_test: expected middle partial layer to blend over lower base at sample 0\n");
        status = 0;
    }
    if (composed[1] != drawing_program_color_value_from_index(7u)) {
        fprintf(stderr, "lifecycle_test: expected opaque top layer to override mixed stack at sample 1\n");
        status = 0;
    }
cleanup:
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

static int render_domain_assert_multi_partial_stack_preserves_semantics(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramRasterSample composed[64];
    uint32_t middle_layer_id = 0u;
    uint32_t upper_layer_id = 0u;
    uint32_t top_layer_id = 0u;
    uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS];
    DrawingProgramRasterSample expected;
    CoreResult result;
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(composed, 0, sizeof(composed));
    memset(opacity, 100, sizeof(opacity));
    if (!render_domain_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                                 "render_domain_init_shape_multi_partial")) {
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                                 "render_domain_init_layer_rasters_multi_partial")) {
        return 1;
    }
    if (!render_domain_expect_ok(drawing_program_document_add_layer(&document, "Middle", &middle_layer_id),
                                 "render_domain_add_middle_multi_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_document_add_layer(&document, "Upper", &upper_layer_id),
                                 "render_domain_add_upper_multi_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_document_add_layer(&document, "Top", &top_layer_id),
                                 "render_domain_add_top_multi_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sync_document_layers(&layer_rasters, &document),
                                 "render_domain_sync_layers_multi_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                 document.layers[0].layer_id,
                                                                                 0u,
                                                                                 0u,
                                                                                 drawing_program_color_value_from_index(
                                                                                     1u),
                                                                                 0),
                                 "render_domain_write_base_multi_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                 middle_layer_id,
                                                                                 0u,
                                                                                 0u,
                                                                                 drawing_program_color_value_from_index(
                                                                                     2u),
                                                                                 0),
                                 "render_domain_write_middle_multi_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                 upper_layer_id,
                                                                                 0u,
                                                                                 0u,
                                                                                 drawing_program_color_value_from_index(
                                                                                     6u),
                                                                                 0),
                                 "render_domain_write_upper_multi_partial")) {
        status = 0;
        goto cleanup;
    }
    if (!render_domain_expect_ok(drawing_program_layer_raster_store_sample_write(&layer_rasters,
                                                                                 top_layer_id,
                                                                                 1u,
                                                                                 0u,
                                                                                 drawing_program_color_value_from_index(
                                                                                     7u),
                                                                                 0),
                                 "render_domain_write_top_multi_partial")) {
        status = 0;
        goto cleanup;
    }
    opacity[1] = 50u;
    opacity[2] = 60u;
    expected = drawing_program_color_blend_samples(drawing_program_color_value_from_index(1u),
                                                   drawing_program_color_value_from_index(2u),
                                                   50u);
    expected = drawing_program_color_blend_samples(expected,
                                                   drawing_program_color_value_from_index(6u),
                                                   60u);
    result = drawing_program_render_compose_visible_samples_with_layer_opacity(&document,
                                                                               &layer_rasters,
                                                                               opacity,
                                                                               DRAWING_PROGRAM_MAX_LAYERS,
                                                                               composed,
                                                                               64u);
    if (!render_domain_expect_ok(result, "render_domain_compose_multi_partial")) {
        status = 0;
        goto cleanup;
    }
    if (composed[0] != expected) {
        fprintf(stderr, "lifecycle_test: expected multi-partial stack to preserve blended semantics at sample 0\n");
        status = 0;
    }
    if (composed[1] != drawing_program_color_value_from_index(7u)) {
        fprintf(stderr, "lifecycle_test: expected opaque top override to win at sample 1\n");
        status = 0;
    }
cleanup:
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

int drawing_program_lifecycle_run_render_domain_suite(void) {
    if (render_domain_assert_direct_legacy_layer() != 0) {
        return 1;
    }
    if (render_domain_assert_direct_layer_store_layer() != 0) {
        return 1;
    }
    if (render_domain_assert_compose_required_for_multiple_layers() != 0) {
        return 1;
    }
    if (render_domain_assert_compose_required_for_partial_opacity() != 0) {
        return 1;
    }
    if (render_domain_assert_multi_layer_full_opacity_prefers_topmost_nontransparent() != 0) {
        return 1;
    }
    if (render_domain_assert_zero_opacity_layer_is_pruned_from_compose() != 0) {
        return 1;
    }
    if (render_domain_assert_single_partial_opacity_layer_composes_uniformly() != 0) {
        return 1;
    }
    if (render_domain_assert_single_partial_layer_in_mixed_stack_preserves_semantics() != 0) {
        return 1;
    }
    if (render_domain_assert_multi_partial_stack_preserves_semantics() != 0) {
        return 1;
    }
    return 0;
}
