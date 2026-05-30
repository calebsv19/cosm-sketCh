#include "drawing_program_lifecycle_surface_cache_contract_suite.h"

#include <SDL2/SDL.h>

#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_texture_project.h"
#include "drawing_program/drawing_program_texture_project_session.h"
#include "drawing_program/drawing_program_visual_surface_cache.h"
#include "drawing_program_lifecycle_test_support.h"

static void surface_cache_fill_opacity(const DrawingProgramTextureSurface *surface,
                                       uint8_t opacity[DRAWING_PROGRAM_MAX_LAYERS],
                                       uint32_t *out_count) {
    uint32_t i;
    uint32_t count = 0u;
    if (!out_count) {
        return;
    }
    if (!surface || !surface->storage) {
        *out_count = 0u;
        return;
    }
    count = surface->storage->document.layer_count;
    if (count > DRAWING_PROGRAM_MAX_LAYERS) {
        count = DRAWING_PROGRAM_MAX_LAYERS;
    }
    for (i = 0u; i < count; ++i) {
        opacity[i] = 100u;
    }
    *out_count = count;
}

static int surface_cache_copy_texture_pixels(SDL_Texture *texture,
                                             uint32_t width,
                                             uint32_t height,
                                             uint32_t *out_pixels) {
    void *pixels = 0;
    int pitch = 0;
    uint32_t y;
    if (!texture || !out_pixels || width == 0u || height == 0u) {
        return 0;
    }
    if (SDL_LockTexture(texture, 0, &pixels, &pitch) != 0) {
        return 0;
    }
    for (y = 0u; y < height; ++y) {
        const uint32_t *row = (const uint32_t *)((const uint8_t *)pixels + ((size_t)y * (size_t)pitch));
        memcpy(out_pixels + ((size_t)y * width), row, (size_t)width * sizeof(*out_pixels));
    }
    SDL_UnlockTexture(texture);
    return 1;
}

int drawing_program_lifecycle_run_surface_cache_contract_suite(void) {
    static DrawingProgramAppContext ctx;
    SDL_Surface *surface = 0;
    SDL_Renderer *renderer = 0;
    const DrawingProgramTextureSurface *active_surface = 0;
    const DrawingProgramTextureSurface *inactive_surface = 0;
    DrawingProgramVisualSurfaceCacheRequest request;
    DrawingProgramVisualSurfaceCacheTelemetry telemetry;
    uint8_t active_opacity[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t inactive_opacity[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t active_opacity_count = 0u;
    uint32_t inactive_opacity_count = 0u;
    uint32_t second_surface_index = 0u;
    uint32_t lower_partial_layer_id = 0u;
    uint32_t upper_partial_layer_id = 0u;
    uint32_t top_partial_layer_id = 0u;
    SDL_Texture *texture = 0;
    uint32_t *before_pixels = 0;
    uint32_t *after_pixels = 0;
    uint32_t diff_count = 0u;
    uint32_t sample_x = 7u;
    uint32_t sample_y = 9u;
    uint32_t top_band_sample_x = 11u;
    uint32_t top_band_sample_y = 13u;
    uint32_t pixel_index = 0u;
    char arg0[] = "drawing_program_surface_cache_contract_test";
    char arg1[] = "--headless";
    char arg2[] = "--smoke-frames";
    char arg3[] = "1";
    char arg4[] = "--no-persist";
    char *argv[] = { arg0, arg1, arg2, arg3, arg4, 0 };
    int status = 1;

    if (!expect_ok(drawing_program_app_bootstrap(&ctx, 5, argv), "surface_cache_contract_bootstrap") ||
        !expect_ok(drawing_program_app_config_load(&ctx), "surface_cache_contract_config_load") ||
        !expect_ok(drawing_program_app_state_seed(&ctx), "surface_cache_contract_state_seed") ||
        !expect_ok(drawing_program_app_subsystems_init(&ctx), "surface_cache_contract_subsystems_init") ||
        !expect_ok(drawing_program_runtime_start(&ctx), "surface_cache_contract_runtime_start")) {
        goto cleanup;
    }
    if (!expect_ok(drawing_program_texture_project_session_seed_blank(
                       &ctx, 64u, 64u, DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD),
                   "surface_cache_contract_seed_blank") ||
        !expect_ok(drawing_program_texture_project_session_add_surface(&ctx,
                                                                       "Right",
                                                                       64u,
                                                                       64u,
                                                                       1u,
                                                                       DRAWING_PROGRAM_TEXTURE_FACE_ROLE_RIGHT,
                                                                       DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD,
                                                                       &second_surface_index),
                   "surface_cache_contract_add_surface")) {
        goto cleanup;
    }
    active_surface = drawing_program_texture_project_surface_at(&ctx.texture_project, 0u);
    inactive_surface = drawing_program_texture_project_surface_at(&ctx.texture_project, second_surface_index);
    if (!active_surface || !inactive_surface || !active_surface->storage || !inactive_surface->storage) {
        fprintf(stderr, "lifecycle_test: expected active and inactive cache contract surfaces\n");
        goto cleanup;
    }
    surface_cache_fill_opacity(active_surface, active_opacity, &active_opacity_count);
    surface_cache_fill_opacity(inactive_surface, inactive_opacity, &inactive_opacity_count);
    surface = SDL_CreateRGBSurfaceWithFormat(0, 256, 256, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) {
        fprintf(stderr, "lifecycle_test: expected software surface for cache contract suite\n");
        goto cleanup;
    }
    renderer = SDL_CreateSoftwareRenderer(surface);
    if (!renderer) {
        fprintf(stderr, "lifecycle_test: expected software renderer for cache contract suite\n");
        goto cleanup;
    }
    drawing_program_visual_surface_cache_shutdown();

    memset(&request, 0, sizeof(request));
    request.project_epoch = ctx.texture_project.runtime_cache_epoch;
    request.content_revision = active_surface->content_revision;
    request.layer_opacity_revision = 1u;
    request.surface_id = active_surface->surface_id;
    request.zoom_bucket_percent = 100u;
    request.active_surface = 1u;
    memset(&telemetry, 0, sizeof(telemetry));
    texture = drawing_program_visual_surface_cache_sync(renderer,
                                                        &request,
                                                        &active_surface->storage->document,
                                                        &active_surface->storage->layer_rasters,
                                                        active_opacity,
                                                        active_opacity_count,
                                                        &telemetry);
    if (!texture || !telemetry.cache_miss || !telemetry.cache_rebuilt || !telemetry.cache_copy_ready ||
        drawing_program_visual_surface_cache_pending_count() != 0u) {
        fprintf(stderr, "lifecycle_test: expected immediate active cache build on first sync\n");
        goto cleanup;
    }

    request.content_revision = inactive_surface->content_revision;
    request.surface_id = inactive_surface->surface_id;
    request.active_surface = 0u;
    memset(&telemetry, 0, sizeof(telemetry));
    texture = drawing_program_visual_surface_cache_sync(renderer,
                                                        &request,
                                                        &inactive_surface->storage->document,
                                                        &inactive_surface->storage->layer_rasters,
                                                        inactive_opacity,
                                                        inactive_opacity_count,
                                                        &telemetry);
    if (!texture || !telemetry.cache_miss || !telemetry.cache_rebuilt || !telemetry.cache_copy_ready ||
        drawing_program_visual_surface_cache_entry_count() != 2u) {
        fprintf(stderr, "lifecycle_test: expected immediate inactive cache build on first sync\n");
        goto cleanup;
    }

    request.content_revision = inactive_surface->content_revision + 1u;
    request.surface_id = inactive_surface->surface_id;
    request.active_surface = 0u;
    memset(&telemetry, 0, sizeof(telemetry));
    texture = drawing_program_visual_surface_cache_sync(renderer,
                                                        &request,
                                                        &inactive_surface->storage->document,
                                                        &inactive_surface->storage->layer_rasters,
                                                        inactive_opacity,
                                                        inactive_opacity_count,
                                                        &telemetry);
    if (!texture || !telemetry.cache_miss || !telemetry.cache_deferred || !telemetry.cache_copy_ready ||
        drawing_program_visual_surface_cache_pending_count() != 1u) {
        fprintf(stderr, "lifecycle_test: expected inactive cache revision change to defer one pending rebuild\n");
        goto cleanup;
    }

    request.content_revision = active_surface->content_revision + 1u;
    request.surface_id = active_surface->surface_id;
    request.active_surface = 1u;
    memset(&telemetry, 0, sizeof(telemetry));
    texture = drawing_program_visual_surface_cache_sync(renderer,
                                                        &request,
                                                        &active_surface->storage->document,
                                                        &active_surface->storage->layer_rasters,
                                                        active_opacity,
                                                        active_opacity_count,
                                                        &telemetry);
    if (!texture || !telemetry.cache_miss || !telemetry.cache_deferred || !telemetry.cache_copy_ready ||
        drawing_program_visual_surface_cache_pending_count() != 2u) {
        fprintf(stderr, "lifecycle_test: expected active cache revision change to join pending queue\n");
        goto cleanup;
    }

    memset(&telemetry, 0, sizeof(telemetry));
    if (drawing_program_visual_surface_cache_process_pending_step(1u, &telemetry) != 1u ||
        !telemetry.cache_rebuilt ||
        drawing_program_visual_surface_cache_pending_count() != 1u) {
        fprintf(stderr, "lifecycle_test: expected active-only queue step to rebuild exactly the active pending surface\n");
        goto cleanup;
    }
    if (drawing_program_visual_surface_cache_process_pending_step(1u, &telemetry) != 0u ||
        drawing_program_visual_surface_cache_pending_count() != 1u) {
        fprintf(stderr, "lifecycle_test: expected active-only queue step to leave inactive pending work untouched\n");
        goto cleanup;
    }

    request.content_revision = active_surface->content_revision + 1u;
    request.surface_id = active_surface->surface_id;
    request.active_surface = 1u;
    memset(&telemetry, 0, sizeof(telemetry));
    texture = drawing_program_visual_surface_cache_sync(renderer,
                                                        &request,
                                                        &active_surface->storage->document,
                                                        &active_surface->storage->layer_rasters,
                                                        active_opacity,
                                                        active_opacity_count,
                                                        &telemetry);
    if (!texture || !telemetry.cache_hit || !telemetry.cache_copy_ready) {
        fprintf(stderr, "lifecycle_test: expected rebuilt active cache to become a direct hit after queue drain\n");
        goto cleanup;
    }

    memset(&telemetry, 0, sizeof(telemetry));
    if (drawing_program_visual_surface_cache_process_pending_step(0u, &telemetry) != 1u ||
        !telemetry.cache_rebuilt ||
        drawing_program_visual_surface_cache_pending_count() != 0u) {
        fprintf(stderr, "lifecycle_test: expected general queue step to drain the remaining inactive pending surface\n");
        goto cleanup;
    }

    request.content_revision = inactive_surface->content_revision + 1u;
    request.surface_id = inactive_surface->surface_id;
    request.active_surface = 0u;
    memset(&telemetry, 0, sizeof(telemetry));
    texture = drawing_program_visual_surface_cache_sync(renderer,
                                                        &request,
                                                        &inactive_surface->storage->document,
                                                        &inactive_surface->storage->layer_rasters,
                                                        inactive_opacity,
                                                        inactive_opacity_count,
                                                        &telemetry);
    if (!texture || !telemetry.cache_hit || !telemetry.cache_copy_ready) {
        fprintf(stderr, "lifecycle_test: expected rebuilt inactive cache to become a direct hit after full queue drain\n");
        goto cleanup;
    }

    drawing_program_visual_surface_cache_shutdown();
    if (!expect_ok(drawing_program_document_add_layer(&active_surface->storage->document,
                                                      "LowerPartial",
                                                      &lower_partial_layer_id),
                   "surface_cache_contract_add_lower_partial") ||
        !expect_ok(drawing_program_document_add_layer(&active_surface->storage->document,
                                                      "UpperPartial",
                                                      &upper_partial_layer_id),
                   "surface_cache_contract_add_upper_partial") ||
        !expect_ok(drawing_program_document_add_layer(&active_surface->storage->document,
                                                      "TopPartial",
                                                      &top_partial_layer_id),
                   "surface_cache_contract_add_top_partial") ||
        !expect_ok(drawing_program_layer_raster_store_sync_document_layers(&active_surface->storage->layer_rasters,
                                                                           &active_surface->storage->document),
                   "surface_cache_contract_sync_partial_suffix_layers")) {
        goto cleanup;
    }
    surface_cache_fill_opacity(active_surface, active_opacity, &active_opacity_count);
    active_opacity[1] = 70u;
    active_opacity[2] = 60u;
    active_opacity[3] = 50u;
    if (!expect_ok(drawing_program_layer_raster_store_sample_write(&active_surface->storage->layer_rasters,
                                                                   active_surface->storage->document.layers[0].layer_id,
                                                                   sample_x,
                                                                   sample_y,
                                                                   drawing_program_color_value_from_rgba(
                                                                       25u, 20u, 160u, 255u),
                                                                   0),
                   "surface_cache_contract_write_base_first") ||
        !expect_ok(drawing_program_layer_raster_store_sample_write(&active_surface->storage->layer_rasters,
                                                                   lower_partial_layer_id,
                                                                   sample_x,
                                                                   sample_y,
                                                                   drawing_program_color_value_from_rgba(
                                                                       30u, 190u, 80u, 255u),
                                                                   0),
                   "surface_cache_contract_write_lower_partial") ||
        !expect_ok(drawing_program_layer_raster_store_sample_write(&active_surface->storage->layer_rasters,
                                                                   upper_partial_layer_id,
                                                                   sample_x,
                                                                   sample_y,
                                                                   drawing_program_color_value_from_rgba(
                                                                       210u, 70u, 40u, 255u),
                                                                   0),
                   "surface_cache_contract_write_upper_partial") ||
        !expect_ok(drawing_program_layer_raster_store_sample_write(&active_surface->storage->layer_rasters,
                                                                   top_partial_layer_id,
                                                                   sample_x,
                                                                   sample_y,
                                                                   drawing_program_color_value_from_rgba(
                                                                       230u, 230u, 250u, 255u),
                                                                   0),
                   "surface_cache_contract_write_top_partial") ||
        !expect_ok(drawing_program_layer_raster_store_sample_write(&active_surface->storage->layer_rasters,
                                                                   active_surface->storage->document.layers[0].layer_id,
                                                                   top_band_sample_x,
                                                                   top_band_sample_y,
                                                                   drawing_program_color_value_from_rgba(
                                                                       20u, 30u, 180u, 255u),
                                                                   0),
                   "surface_cache_contract_write_top_band_base") ||
        !expect_ok(drawing_program_layer_raster_store_sample_write(&active_surface->storage->layer_rasters,
                                                                   lower_partial_layer_id,
                                                                   top_band_sample_x,
                                                                   top_band_sample_y,
                                                                   drawing_program_color_value_from_rgba(
                                                                       40u, 200u, 70u, 255u),
                                                                   0),
                   "surface_cache_contract_write_top_band_lower_partial") ||
        !expect_ok(drawing_program_layer_raster_store_sample_write(&active_surface->storage->layer_rasters,
                                                                   upper_partial_layer_id,
                                                                   top_band_sample_x,
                                                                   top_band_sample_y,
                                                                   drawing_program_color_value_from_rgba(
                                                                       220u, 60u, 50u, 255u),
                                                                   0),
                   "surface_cache_contract_write_top_band_upper_partial") ||
        !expect_ok(drawing_program_layer_raster_store_sample_write(&active_surface->storage->layer_rasters,
                                                                   top_partial_layer_id,
                                                                   top_band_sample_x,
                                                                   top_band_sample_y,
                                                                   drawing_program_color_value_from_rgba(
                                                                       240u, 220u, 40u, 255u),
                                                                   0),
                   "surface_cache_contract_write_top_band_top_partial")) {
        goto cleanup;
    }
    memset(&request, 0, sizeof(request));
    request.project_epoch = ctx.texture_project.runtime_cache_epoch;
    request.content_revision = 9001u;
    request.layer_opacity_revision = 77u;
    request.surface_id = active_surface->surface_id;
    request.zoom_bucket_percent = 100u;
    request.active_surface = 1u;
    memset(&telemetry, 0, sizeof(telemetry));
    texture = drawing_program_visual_surface_cache_sync(renderer,
                                                        &request,
                                                        &active_surface->storage->document,
                                                        &active_surface->storage->layer_rasters,
                                                        active_opacity,
                                                        active_opacity_count,
                                                        &telemetry);
    if (!texture || !telemetry.cache_miss || !telemetry.cache_rebuilt || !telemetry.cache_copy_ready) {
        fprintf(stderr, "lifecycle_test: expected fresh active cache build for lower-band dirty-rect scenario\n");
        goto cleanup;
    }
    before_pixels = (uint32_t *)SDL_malloc((size_t)active_surface->storage->document.raster_sample_count *
                                           sizeof(*before_pixels));
    after_pixels = (uint32_t *)SDL_malloc((size_t)active_surface->storage->document.raster_sample_count *
                                          sizeof(*after_pixels));
    if (!before_pixels || !after_pixels ||
        !surface_cache_copy_texture_pixels(texture,
                                           active_surface->storage->document.raster_width,
                                           active_surface->storage->document.raster_height,
                                           before_pixels)) {
        fprintf(stderr, "lifecycle_test: expected to capture baseline cache texture pixels\n");
        goto cleanup;
    }
    if (!expect_ok(drawing_program_layer_raster_store_sample_write(&active_surface->storage->layer_rasters,
                                                                   active_surface->storage->document.layers[0].layer_id,
                                                                   sample_x,
                                                                   sample_y,
                                                                   drawing_program_color_value_from_rgba(
                                                                       210u, 210u, 35u, 255u),
                                                                   0),
                   "surface_cache_contract_rewrite_base_second")) {
        goto cleanup;
    }
    request.content_revision = 9002u;
    memset(&telemetry, 0, sizeof(telemetry));
    texture = drawing_program_visual_surface_cache_sync(renderer,
                                                        &request,
                                                        &active_surface->storage->document,
                                                        &active_surface->storage->layer_rasters,
                                                        active_opacity,
                                                        active_opacity_count,
                                                        &telemetry);
    if (!texture || !telemetry.cache_miss || !telemetry.cache_deferred || !telemetry.cache_copy_ready) {
        fprintf(stderr, "lifecycle_test: expected active lower-band cache edit to defer rebuild while keeping live texture\n");
        goto cleanup;
    }
    memset(&telemetry, 0, sizeof(telemetry));
    if (drawing_program_visual_surface_cache_process_pending_step(1u, &telemetry) != 1u ||
        !telemetry.cache_rebuilt ||
        drawing_program_visual_surface_cache_pending_count() != 0u) {
        fprintf(stderr, "lifecycle_test: expected pending active lower-band cache rebuild to drain cleanly\n");
        goto cleanup;
    }
    memset(&telemetry, 0, sizeof(telemetry));
    texture = drawing_program_visual_surface_cache_sync(renderer,
                                                        &request,
                                                        &active_surface->storage->document,
                                                        &active_surface->storage->layer_rasters,
                                                        active_opacity,
                                                        active_opacity_count,
                                                        &telemetry);
    if (!texture || !telemetry.cache_hit || !telemetry.cache_copy_ready ||
        !surface_cache_copy_texture_pixels(texture,
                                           active_surface->storage->document.raster_width,
                                           active_surface->storage->document.raster_height,
                                           after_pixels)) {
        fprintf(stderr, "lifecycle_test: expected rebuilt active lower-band cache to become a direct hit\n");
        goto cleanup;
    }
    for (pixel_index = 0u; pixel_index < active_surface->storage->document.raster_sample_count; ++pixel_index) {
        if (before_pixels[pixel_index] != after_pixels[pixel_index]) {
            diff_count += 1u;
        }
    }
    if (diff_count != 1u) {
        fprintf(stderr, "lifecycle_test: expected lower-band dirty-rect cache rebuild to change exactly one texture pixel got=%u\n",
                (unsigned)diff_count);
        goto cleanup;
    }
    pixel_index = sample_y * active_surface->storage->document.raster_width + sample_x;
    if (before_pixels[pixel_index] == after_pixels[pixel_index]) {
        fprintf(stderr, "lifecycle_test: expected lower-band dirty-rect cache rebuild to change the edited texture pixel\n");
        goto cleanup;
    }
    memcpy(before_pixels,
           after_pixels,
           (size_t)active_surface->storage->document.raster_sample_count * sizeof(*before_pixels));
    if (!expect_ok(drawing_program_layer_raster_store_sample_write(&active_surface->storage->layer_rasters,
                                                                   top_partial_layer_id,
                                                                   top_band_sample_x,
                                                                   top_band_sample_y,
                                                                   drawing_program_color_value_from_rgba(
                                                                       20u, 240u, 240u, 255u),
                                                                   0),
                   "surface_cache_contract_rewrite_top_partial_second")) {
        goto cleanup;
    }
    request.content_revision = 9003u;
    memset(&telemetry, 0, sizeof(telemetry));
    texture = drawing_program_visual_surface_cache_sync(renderer,
                                                        &request,
                                                        &active_surface->storage->document,
                                                        &active_surface->storage->layer_rasters,
                                                        active_opacity,
                                                        active_opacity_count,
                                                        &telemetry);
    if (!texture || !telemetry.cache_miss || !telemetry.cache_deferred || !telemetry.cache_copy_ready) {
        fprintf(stderr, "lifecycle_test: expected active top-band cache edit to defer rebuild while keeping live texture\n");
        goto cleanup;
    }
    memset(&telemetry, 0, sizeof(telemetry));
    if (drawing_program_visual_surface_cache_process_pending_step(1u, &telemetry) != 1u ||
        !telemetry.cache_rebuilt ||
        drawing_program_visual_surface_cache_pending_count() != 0u) {
        fprintf(stderr, "lifecycle_test: expected pending active top-band cache rebuild to drain cleanly\n");
        goto cleanup;
    }
    memset(&telemetry, 0, sizeof(telemetry));
    texture = drawing_program_visual_surface_cache_sync(renderer,
                                                        &request,
                                                        &active_surface->storage->document,
                                                        &active_surface->storage->layer_rasters,
                                                        active_opacity,
                                                        active_opacity_count,
                                                        &telemetry);
    if (!texture || !telemetry.cache_hit || !telemetry.cache_copy_ready ||
        !surface_cache_copy_texture_pixels(texture,
                                           active_surface->storage->document.raster_width,
                                           active_surface->storage->document.raster_height,
                                           after_pixels)) {
        fprintf(stderr, "lifecycle_test: expected rebuilt active top-band cache to become a direct hit\n");
        goto cleanup;
    }
    diff_count = 0u;
    for (pixel_index = 0u; pixel_index < active_surface->storage->document.raster_sample_count; ++pixel_index) {
        if (before_pixels[pixel_index] != after_pixels[pixel_index]) {
            diff_count += 1u;
        }
    }
    if (diff_count != 1u) {
        fprintf(stderr, "lifecycle_test: expected top-band dirty-rect cache rebuild to change exactly one texture pixel got=%u\n",
                (unsigned)diff_count);
        goto cleanup;
    }
    pixel_index = top_band_sample_y * active_surface->storage->document.raster_width + top_band_sample_x;
    if (before_pixels[pixel_index] == after_pixels[pixel_index]) {
        fprintf(stderr, "lifecycle_test: expected top-band dirty-rect cache rebuild to change the edited texture pixel\n");
        goto cleanup;
    }

    status = 0;

cleanup:
    if (before_pixels) {
        SDL_free(before_pixels);
    }
    if (after_pixels) {
        SDL_free(after_pixels);
    }
    drawing_program_visual_surface_cache_shutdown();
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (surface) {
        SDL_FreeSurface(surface);
    }
    (void)drawing_program_app_shutdown(&ctx);
    return status;
}
