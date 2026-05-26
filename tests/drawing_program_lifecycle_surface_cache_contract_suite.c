#include "drawing_program_lifecycle_surface_cache_contract_suite.h"

#include <SDL2/SDL.h>

#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_app_main.h"
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
    SDL_Texture *texture = 0;
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

    status = 0;

cleanup:
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
