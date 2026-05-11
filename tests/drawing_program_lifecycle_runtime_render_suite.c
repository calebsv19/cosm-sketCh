#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core_theme.h"
#include "drawing_program/drawing_program_texture_project_session.h"
#include "drawing_program/drawing_program_texture_workspace.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_canvas_coords.h"
#include "drawing_program/drawing_program_visual_canvas_world_render.h"
#include "drawing_program/drawing_program_visual_input_handlers.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_visual_input_selection_ops.h"
#include "drawing_program/drawing_program_visual_input_support.h"
#include "drawing_program/drawing_program_visual_loop_timing.h"
#include "drawing_program/drawing_program_visual_surface_cache.h"
#include "drawing_program/drawing_program_visual_text_render.h"
#include "drawing_program/drawing_program_visual_tool_options.h"
#include "drawing_program_lifecycle_color_panel_suite.h"
#include "drawing_program_lifecycle_runtime_path_suite.h"
#include "drawing_program_lifecycle_runtime_render_suite.h"
#include "drawing_program_lifecycle_texture_workspace_suite.h"
#include "drawing_program_lifecycle_runtime_ui_suite.h"
#include "drawing_program_lifecycle_test_support.h"

static void lifecycle_render_noop_selection_overlay(SDL_Renderer *renderer,
                                                    SDL_Rect pane_rect,
                                                    const DrawingProgramAppContext *ctx,
                                                    const CoreThemePreset *theme,
                                                    const VisualCanvasSheetMetrics *metrics,
                                                    const VisualSelectionState *selection) {
    (void)renderer;
    (void)pane_rect;
    (void)ctx;
    (void)theme;
    (void)metrics;
    (void)selection;
}

static void lifecycle_render_noop_object_overlay(SDL_Renderer *renderer,
                                                 SDL_Rect pane_rect,
                                                 const DrawingProgramAppContext *ctx,
                                                 const CoreThemePreset *theme,
                                                 const VisualCanvasSheetMetrics *metrics,
                                                 const VisualCanvasInteractionState *interaction,
                                                 const VisualPanelUiState *ui) {
    (void)renderer;
    (void)pane_rect;
    (void)ctx;
    (void)theme;
    (void)metrics;
    (void)interaction;
    (void)ui;
}

static void lifecycle_render_noop_shape_preview_overlay(SDL_Renderer *renderer,
                                                        SDL_Rect pane_rect,
                                                        const DrawingProgramAppContext *ctx,
                                                        const CoreThemePreset *theme,
                                                        const VisualCanvasSheetMetrics *metrics,
                                                        const VisualCanvasInteractionState *interaction,
                                                        const VisualPanelUiState *ui) {
    (void)renderer;
    (void)pane_rect;
    (void)ctx;
    (void)theme;
    (void)metrics;
    (void)interaction;
    (void)ui;
}

static int write_runtime_render_prism_scene_fixture(const char *path) {
    const char *json =
        "{"
        "\"schema_family\":\"codework_scene\","
        "\"schema_variant\":\"scene_authoring_v1\","
        "\"schema_version\":1,"
        "\"scene_id\":\"scene_runtime_guides\","
        "\"space_mode_default\":\"3d\","
        "\"objects\":["
          "{"
            "\"object_id\":\"obj_prism_guides\","
            "\"object_type\":\"rect_prism_primitive\","
            "\"dimensional_mode\":\"full_3d\","
            "\"primitive\":{"
              "\"kind\":\"rect_prism_primitive\","
              "\"width\":3.0,"
              "\"height\":2.0,"
              "\"depth\":4.0,"
              "\"lock_to_construction_plane\":false,"
              "\"lock_to_bounds\":true,"
              "\"frame\":{"
                "\"origin\":{\"x\":1.0,\"y\":2.0,\"z\":3.0},"
                "\"axis_u\":{\"x\":1.0,\"y\":0.0,\"z\":0.0},"
                "\"axis_v\":{\"x\":0.0,\"y\":1.0,\"z\":0.0},"
                "\"normal\":{\"x\":0.0,\"y\":0.0,\"z\":1.0}"
              "}"
            "}"
          "}"
        "],"
        "\"materials\":[],"
        "\"lights\":[],"
        "\"cameras\":[]"
        "}";
    FILE *file = fopen(path, "wb");
    if (!file) {
        return 0;
    }
    if (fwrite(json, 1u, strlen(json), file) != strlen(json)) {
        fclose(file);
        return 0;
    }
    fclose(file);
    return 1;
}

static int lifecycle_sample_surface_rgba(SDL_Surface *surface,
                                         int x,
                                         int y,
                                         uint8_t *out_r,
                                         uint8_t *out_g,
                                         uint8_t *out_b,
                                         uint8_t *out_a) {
    uint8_t *pixel_ptr = 0;
    uint32_t pixel_value = 0u;
    if (!surface || !surface->pixels || x < 0 || y < 0 || x >= surface->w || y >= surface->h) {
        return 0;
    }
    pixel_ptr = (uint8_t *)surface->pixels + (y * surface->pitch) + (x * surface->format->BytesPerPixel);
    memcpy(&pixel_value, pixel_ptr, sizeof(pixel_value));
    SDL_GetRGBA(pixel_value, surface->format, out_r, out_g, out_b, out_a);
    return 1;
}

static int lifecycle_find_surface_index_by_face_role(const DrawingProgramTextureProject *project,
                                                     uint32_t face_role,
                                                     uint32_t *out_surface_index) {
    uint32_t i;
    if (!project || !out_surface_index) {
        return 0;
    }
    for (i = 0u; i < project->surface_count; ++i) {
        const DrawingProgramTextureSurface *surface = drawing_program_texture_project_surface_at(project, i);
        if (surface && surface->face_role == face_role) {
            *out_surface_index = i;
            return 1;
        }
    }
    return 0;
}

static void lifecycle_front_corner_sample_point(const VisualCanvasSheetMetrics *metrics,
                                                int *out_x,
                                                int *out_y) {
    int inset = 0;
    int chip_size = 0;
    if (!metrics || !out_x || !out_y) {
        return;
    }
    inset = metrics->sheet_rect.w < metrics->sheet_rect.h ? metrics->sheet_rect.w : metrics->sheet_rect.h;
    chip_size = inset / 14;
    if (chip_size < 6) {
        chip_size = 6;
    } else if (chip_size > 14) {
        chip_size = 14;
    }
    inset = chip_size / 2 + 2;
    *out_x = metrics->sheet_rect.x + inset + (chip_size / 2);
    *out_y = metrics->sheet_rect.y + inset + (chip_size / 2);
}

static void lifecycle_front_top_edge_sample_point(const VisualCanvasSheetMetrics *metrics,
                                                  int *out_x,
                                                  int *out_y) {
    int thickness = 0;
    int inset = 0;
    if (!metrics || !out_x || !out_y) {
        return;
    }
    thickness = metrics->sheet_rect.w < metrics->sheet_rect.h ? metrics->sheet_rect.w : metrics->sheet_rect.h;
    thickness /= 20;
    if (thickness < 2) {
        thickness = 2;
    } else if (thickness > 5) {
        thickness = 5;
    }
    inset = thickness + 2;
    *out_x = metrics->sheet_rect.x + (metrics->sheet_rect.w / 2);
    *out_y = metrics->sheet_rect.y + inset + (thickness / 2);
}

static uint32_t lifecycle_process_all_pending_surface_cache_steps(void) {
    uint32_t processed = 0u;
    while (drawing_program_visual_surface_cache_pending_count() > 0u) {
        if (drawing_program_visual_surface_cache_process_pending_step(0u, 0) == 0u) {
            break;
        }
        processed += 1u;
    }
    return processed;
}

static int lifecycle_render_profile_log_enabled(void) {
    const char *value = getenv("DRAWING_PROGRAM_RENDER_PROFILE_LOG");
    return value && value[0] != '\0' && value[0] != '0';
}

static int lifecycle_assert_canvas_world_cache_draw(DrawingProgramAppContext *ctx) {
    SDL_Surface *surface = 0;
    SDL_Renderer *renderer = 0;
    CoreThemePreset theme_preset;
    VisualSelectionState selection;
    VisualPanelUiState panel_ui;
    VisualCanvasInteractionState interaction;
    SDL_Rect pane_rect = {80, 60, 720, 420};
    uint32_t second_surface_index = 0u;
    DrawingProgramVisualCanvasWorldRenderHooks hooks;
    uint64_t miss_after_first_pass = 0u;
    uint64_t hit_after_same_bucket = 0u;
    uint64_t hit_after_pan = 0u;
    uint64_t miss_after_pan = 0u;
    uint64_t miss_after_active_defer = 0u;
    int profile_mode = lifecycle_render_profile_log_enabled();
    int status = 1;
    if (!ctx) {
        return 1;
    }
    if (!expect_ok(drawing_program_texture_project_session_seed_blank(
                       ctx,
                       profile_mode ? 512u : 128u,
                       profile_mode ? 384u : 96u,
                       DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD),
                   "runtime_render_seed_blank")) {
        return 1;
    }
    if (!expect_ok(drawing_program_texture_project_session_add_surface(ctx,
                                                                       "Right",
                                                                       profile_mode ? 384u : 96u,
                                                                       profile_mode ? 384u : 96u,
                                                                       1u,
                                                                       DRAWING_PROGRAM_TEXTURE_FACE_ROLE_RIGHT,
                                                                       DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD,
                                                                       &second_surface_index),
                   "runtime_render_add_surface")) {
        return 1;
    }
    if (!drawing_program_texture_workspace_fit_all(ctx, pane_rect)) {
        fprintf(stderr, "lifecycle_test: expected fit-all before canvas-world render cache test\n");
        return 1;
    }
    ctx->editor.viewport.zoom = 0.53f;
    ctx->runtime.render_surface_cache_hit_total = 0u;
    ctx->runtime.render_surface_cache_miss_total = 0u;
    ctx->runtime.render_surface_cache_active_hit_total = 0u;
    ctx->runtime.render_surface_cache_active_miss_total = 0u;
    ctx->runtime.render_surface_cache_inactive_hit_total = 0u;
    ctx->runtime.render_surface_cache_inactive_miss_total = 0u;
    ctx->runtime.render_surface_cache_rebuild_total = 0u;
    ctx->runtime.render_surface_cache_copy_total = 0u;
    ctx->runtime.render_surface_cache_unavailable_total = 0u;
    ctx->runtime.render_surface_cache_deferred_total = 0u;
    ctx->runtime.render_surface_cache_queue_step_total = 0u;
    ctx->runtime.render_surface_cache_compose_us_total = 0u;
    ctx->runtime.render_surface_cache_upload_us_total = 0u;
    ctx->runtime.render_surface_cache_rebuild_us_total = 0u;
    ctx->runtime.render_surface_cache_entry_peak = 0u;
    ctx->runtime.render_zoom_bucket_percent = 0u;
    ctx->runtime.render_zoom_bucket_switch_total = 0u;
    drawing_program_visual_surface_cache_shutdown();
    if (core_theme_get_preset(CORE_THEME_PRESET_DARK_DEFAULT, &theme_preset).code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: expected theme preset for canvas-world render cache test\n");
        return 1;
    }
    memset(&selection, 0, sizeof(selection));
    memset(&panel_ui, 0, sizeof(panel_ui));
    memset(&interaction, 0, sizeof(interaction));
    memset(&hooks, 0, sizeof(hooks));
    hooks.compute_canvas_sheet_metrics = drawing_program_visual_compute_canvas_sheet_metrics;
    hooks.draw_bitmap_text = drawing_program_visual_draw_bitmap_text;
    hooks.draw_selection_overlay = lifecycle_render_noop_selection_overlay;
    hooks.draw_object_overlay = lifecycle_render_noop_object_overlay;
    hooks.draw_shape_preview_overlay = lifecycle_render_noop_shape_preview_overlay;
    surface = SDL_CreateRGBSurfaceWithFormat(0, 960, 640, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) {
        fprintf(stderr, "lifecycle_test: expected software target surface for canvas-world render cache test\n");
        return 1;
    }
    renderer = SDL_CreateSoftwareRenderer(surface);
    if (!renderer) {
        fprintf(stderr, "lifecycle_test: expected software renderer for canvas-world render cache test\n");
        goto cleanup;
    }
    drawing_program_visual_draw_canvas_world_view(
        renderer, pane_rect, ctx, &theme_preset, &selection, &panel_ui, &interaction, &hooks);
    drawing_program_visual_draw_canvas_world_view(
        renderer, pane_rect, ctx, &theme_preset, &selection, &panel_ui, &interaction, &hooks);
    miss_after_first_pass = ctx->runtime.render_surface_cache_miss_total;
    hit_after_same_bucket = ctx->runtime.render_surface_cache_hit_total;
    if (ctx->runtime.render_surface_cache_unavailable_total != 0u) {
        fprintf(stderr,
                "lifecycle_test: expected atlas render cache path without unavailable surfaces got=%llu\n",
                (unsigned long long)ctx->runtime.render_surface_cache_unavailable_total);
        goto cleanup;
    }
    if (ctx->runtime.render_surface_cache_entry_peak < ctx->texture_project.surface_count ||
        drawing_program_visual_surface_cache_entry_count() != ctx->texture_project.surface_count) {
        fprintf(stderr,
                "lifecycle_test: expected one cache entry per surface peak=%u live=%u surfaces=%u\n",
                (unsigned)ctx->runtime.render_surface_cache_entry_peak,
                (unsigned)drawing_program_visual_surface_cache_entry_count(),
                (unsigned)ctx->texture_project.surface_count);
        goto cleanup;
    }
    if (ctx->runtime.render_surface_cache_hit_total < ctx->texture_project.surface_count ||
        ctx->runtime.render_surface_cache_copy_total < (ctx->texture_project.surface_count * 2u)) {
        fprintf(stderr,
                "lifecycle_test: expected second atlas draw to reuse caches hits=%llu copies=%llu surfaces=%u\n",
                (unsigned long long)ctx->runtime.render_surface_cache_hit_total,
                (unsigned long long)ctx->runtime.render_surface_cache_copy_total,
                (unsigned)ctx->texture_project.surface_count);
        goto cleanup;
    }
    if (ctx->runtime.render_zoom_bucket_percent != 50u || ctx->runtime.render_zoom_bucket_switch_total != 0u) {
        fprintf(stderr,
                "lifecycle_test: expected stable 50%% zoom bucket without switch got bucket=%u switches=%llu\n",
                (unsigned)ctx->runtime.render_zoom_bucket_percent,
                (unsigned long long)ctx->runtime.render_zoom_bucket_switch_total);
        goto cleanup;
    }
    {
        DrawingProgramVisualLoopWaitPolicyInput wait_idle = {0};
        DrawingProgramVisualLoopWaitPolicyInput wait_busy = {0};
        DrawingProgramVisualLoopRenderPolicyInput render_idle = {0};
        DrawingProgramVisualLoopRenderPolicyInput render_busy = {0};
        render_idle.last_present_ms = 1u;
        render_busy.last_present_ms = 1u;
        wait_busy.background_busy = 1u;
        render_busy.background_busy = 1u;
        if (drawing_program_visual_loop_compute_wait_timeout_ms(&wait_busy) >=
            drawing_program_visual_loop_compute_wait_timeout_ms(&wait_idle)) {
            fprintf(stderr, "lifecycle_test: expected background-busy wait policy to shorten wait timeout\n");
            goto cleanup;
        }
        if (drawing_program_visual_loop_should_render_frame(&render_idle)) {
            fprintf(stderr, "lifecycle_test: expected idle render policy without dirty/background work to skip render\n");
            goto cleanup;
        }
        if (!drawing_program_visual_loop_should_render_frame(&render_busy)) {
            fprintf(stderr, "lifecycle_test: expected background-busy render policy to request a frame\n");
            goto cleanup;
        }
    }
    ctx->editor.viewport.zoom = 0.72f;
    drawing_program_visual_draw_canvas_world_view(
        renderer, pane_rect, ctx, &theme_preset, &selection, &panel_ui, &interaction, &hooks);
    if (ctx->runtime.render_zoom_bucket_percent != 50u ||
        ctx->runtime.render_zoom_bucket_switch_total != 0u ||
        ctx->runtime.render_surface_cache_miss_total != miss_after_first_pass ||
        ctx->runtime.render_surface_cache_hit_total < hit_after_same_bucket + ctx->texture_project.surface_count) {
        fprintf(stderr,
                "lifecycle_test: expected same-bucket zoom reuse bucket=%u switches=%llu misses=%llu hits=%llu first_miss=%llu first_hit=%llu surfaces=%u\n",
                (unsigned)ctx->runtime.render_zoom_bucket_percent,
                (unsigned long long)ctx->runtime.render_zoom_bucket_switch_total,
                (unsigned long long)ctx->runtime.render_surface_cache_miss_total,
                (unsigned long long)ctx->runtime.render_surface_cache_hit_total,
                (unsigned long long)miss_after_first_pass,
                (unsigned long long)hit_after_same_bucket,
                (unsigned)ctx->texture_project.surface_count);
        goto cleanup;
    }
    ctx->editor.viewport.pan_x += 48.0f;
    ctx->editor.viewport.pan_y += 24.0f;
    drawing_program_visual_draw_canvas_world_view(
        renderer, pane_rect, ctx, &theme_preset, &selection, &panel_ui, &interaction, &hooks);
    hit_after_pan = ctx->runtime.render_surface_cache_hit_total;
    miss_after_pan = ctx->runtime.render_surface_cache_miss_total;
    if (ctx->runtime.render_surface_cache_miss_total != miss_after_first_pass ||
        ctx->runtime.render_surface_cache_hit_total < hit_after_same_bucket + (ctx->texture_project.surface_count * 2u)) {
        fprintf(stderr,
                "lifecycle_test: expected pan-only atlas move to stay cache-hot misses=%llu hits=%llu first_miss=%llu prior_hit=%llu surfaces=%u\n",
                (unsigned long long)ctx->runtime.render_surface_cache_miss_total,
                (unsigned long long)ctx->runtime.render_surface_cache_hit_total,
                (unsigned long long)miss_after_first_pass,
                (unsigned long long)hit_after_same_bucket,
                (unsigned)ctx->texture_project.surface_count);
        goto cleanup;
    }
    if (!expect_ok(drawing_program_history_apply_set_sample_value(&ctx->history,
                                                                  &ctx->document,
                                                                  &ctx->layer_rasters,
                                                                  ctx->document.layers[0].layer_id,
                                                                  4u,
                                                                  4u,
                                                                  drawing_program_color_value_from_index(7u)),
                   "runtime_render_active_surface_mutation")) {
        goto cleanup;
    }
    drawing_program_visual_draw_canvas_world_view(
        renderer, pane_rect, ctx, &theme_preset, &selection, &panel_ui, &interaction, &hooks);
    miss_after_active_defer = ctx->runtime.render_surface_cache_miss_total;
    if (ctx->runtime.render_surface_cache_miss_total != miss_after_pan + 1u ||
        ctx->runtime.render_surface_cache_hit_total < hit_after_pan + (ctx->texture_project.surface_count - 1u)) {
        fprintf(stderr,
                "lifecycle_test: expected active content mutation to defer one surface rebuild misses=%llu hits=%llu prior_miss=%llu prior_hit=%llu surfaces=%u\n",
                (unsigned long long)ctx->runtime.render_surface_cache_miss_total,
                (unsigned long long)ctx->runtime.render_surface_cache_hit_total,
                (unsigned long long)miss_after_pan,
                (unsigned long long)hit_after_pan,
                (unsigned)ctx->texture_project.surface_count);
        goto cleanup;
    }
    if (drawing_program_visual_surface_cache_pending_count() != 1u ||
        lifecycle_process_all_pending_surface_cache_steps() != 1u ||
        drawing_program_visual_surface_cache_pending_count() != 0u) {
        fprintf(stderr, "lifecycle_test: expected one queued active-surface rebuild step\n");
        goto cleanup;
    }
    drawing_program_visual_draw_canvas_world_view(
        renderer, pane_rect, ctx, &theme_preset, &selection, &panel_ui, &interaction, &hooks);
    if (ctx->runtime.render_surface_cache_miss_total != miss_after_active_defer) {
        fprintf(stderr, "lifecycle_test: expected queued active rebuild to avoid a second miss on present\n");
        goto cleanup;
    }
    drawing_program_visual_layer_opacity_set(ctx, ctx->document.layers[0].layer_id, 84u);
    drawing_program_visual_draw_canvas_world_view(
        renderer, pane_rect, ctx, &theme_preset, &selection, &panel_ui, &interaction, &hooks);
    if (drawing_program_visual_surface_cache_pending_count() != ctx->texture_project.surface_count ||
        ctx->runtime.render_surface_cache_miss_total <
            miss_after_active_defer + ctx->texture_project.surface_count) {
        fprintf(stderr,
                "lifecycle_test: expected opacity revision change to queue the visible atlas wave misses=%llu prior_miss=%llu pending=%u surfaces=%u\n",
                (unsigned long long)ctx->runtime.render_surface_cache_miss_total,
                (unsigned long long)miss_after_active_defer,
                (unsigned)drawing_program_visual_surface_cache_pending_count(),
                (unsigned)ctx->texture_project.surface_count);
        goto cleanup;
    }
    if (lifecycle_process_all_pending_surface_cache_steps() != ctx->texture_project.surface_count) {
        fprintf(stderr, "lifecycle_test: expected queued opacity rebuild to drain one step per surface\n");
        goto cleanup;
    }
    drawing_program_visual_draw_canvas_world_view(
        renderer, pane_rect, ctx, &theme_preset, &selection, &panel_ui, &interaction, &hooks);
    ctx->editor.viewport.zoom = 0.86f;
    drawing_program_visual_draw_canvas_world_view(
        renderer, pane_rect, ctx, &theme_preset, &selection, &panel_ui, &interaction, &hooks);
    if (ctx->runtime.render_zoom_bucket_percent != 100u ||
        ctx->runtime.render_zoom_bucket_switch_total != 1u ||
        drawing_program_visual_surface_cache_pending_count() != ctx->texture_project.surface_count ||
        ctx->runtime.render_surface_cache_miss_total < miss_after_first_pass + ctx->texture_project.surface_count) {
        fprintf(stderr,
                "lifecycle_test: expected bucket transition to queue a rebuild wave bucket=%u switches=%llu misses=%llu first_miss=%llu pending=%u surfaces=%u\n",
                (unsigned)ctx->runtime.render_zoom_bucket_percent,
                (unsigned long long)ctx->runtime.render_zoom_bucket_switch_total,
                (unsigned long long)ctx->runtime.render_surface_cache_miss_total,
                (unsigned long long)miss_after_first_pass,
                (unsigned)drawing_program_visual_surface_cache_pending_count(),
                (unsigned)ctx->texture_project.surface_count);
        goto cleanup;
    }
    if (lifecycle_process_all_pending_surface_cache_steps() != ctx->texture_project.surface_count) {
        fprintf(stderr, "lifecycle_test: expected queued bucket rebuild to drain one step per surface\n");
        goto cleanup;
    }
    drawing_program_visual_draw_canvas_world_view(
        renderer, pane_rect, ctx, &theme_preset, &selection, &panel_ui, &interaction, &hooks);
    if (ctx->runtime.render_surface_cache_compose_us_total == 0u ||
        ctx->runtime.render_surface_cache_upload_us_total == 0u ||
        ctx->runtime.render_surface_cache_rebuild_us_total == 0u) {
        fprintf(stderr,
                "lifecycle_test: expected rebuild timing telemetry compose=%llu upload=%llu rebuild=%llu\n",
                (unsigned long long)ctx->runtime.render_surface_cache_compose_us_total,
                (unsigned long long)ctx->runtime.render_surface_cache_upload_us_total,
                (unsigned long long)ctx->runtime.render_surface_cache_rebuild_us_total);
        goto cleanup;
    }
    if (status == 1 && lifecycle_render_profile_log_enabled()) {
        fprintf(stdout,
                "drawing_program render profile: hits=%llu misses=%llu deferred=%llu rebuilds=%llu copies=%llu compose_us=%llu upload_us=%llu rebuild_us=%llu bucket_switches=%llu\n",
                (unsigned long long)ctx->runtime.render_surface_cache_hit_total,
                (unsigned long long)ctx->runtime.render_surface_cache_miss_total,
                (unsigned long long)ctx->runtime.render_surface_cache_deferred_total,
                (unsigned long long)ctx->runtime.render_surface_cache_rebuild_total,
                (unsigned long long)ctx->runtime.render_surface_cache_copy_total,
                (unsigned long long)ctx->runtime.render_surface_cache_compose_us_total,
                (unsigned long long)ctx->runtime.render_surface_cache_upload_us_total,
                (unsigned long long)ctx->runtime.render_surface_cache_rebuild_us_total,
                (unsigned long long)ctx->runtime.render_zoom_bucket_switch_total);
    }
    status = 0;
cleanup:
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (surface) {
        SDL_FreeSurface(surface);
    }
    drawing_program_visual_surface_cache_shutdown();
    return status;
}

static int lifecycle_assert_canvas_world_guide_overlay(void) {
    static DrawingProgramAppContext ctx;
    const char *scene_path = "/tmp/drawing_program_runtime_render_guides_fixture.json";
    char arg0[] = "drawing_program_runtime_guides";
    char arg1[] = "--headless";
    char arg2[] = "--smoke-frames";
    char arg3[] = "1";
    char arg4[] = "--no-persist";
    char *argv[] = { arg0, arg1, arg2, arg3, arg4, 0 };
    SDL_Surface *surface = 0;
    SDL_Renderer *renderer = 0;
    CoreThemePreset theme_preset;
    VisualSelectionState selection;
    VisualPanelUiState panel_ui;
    VisualCanvasInteractionState interaction;
    VisualCanvasSheetMetrics metrics;
    SDL_Rect pane_rect = {80, 60, 1280, 820};
    DrawingProgramVisualCanvasWorldRenderHooks hooks;
    uint32_t front_surface_index = 0u;
    int corner_x = 0;
    int corner_y = 0;
    int edge_x = 0;
    int edge_y = 0;
    uint8_t r = 0u;
    uint8_t g = 0u;
    uint8_t b = 0u;
    uint8_t a = 0u;
    int status = 1;

    (void)unlink(scene_path);
    if (!write_runtime_render_prism_scene_fixture(scene_path)) {
        fprintf(stderr, "lifecycle_test: failed to write runtime render guide fixture\n");
        return 1;
    }
    if (!expect_ok(drawing_program_app_bootstrap(&ctx, 5, argv), "runtime_guides_bootstrap")) {
        goto cleanup;
    }
    if (!expect_ok(drawing_program_app_config_load(&ctx), "runtime_guides_config")) {
        goto cleanup;
    }
    if (!expect_ok(drawing_program_app_state_seed(&ctx), "runtime_guides_state_seed")) {
        goto cleanup;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&ctx), "runtime_guides_subsystems")) {
        goto cleanup;
    }
    if (!expect_ok(drawing_program_runtime_start(&ctx), "runtime_guides_runtime_start")) {
        goto cleanup;
    }
    if (!expect_ok(drawing_program_texture_project_session_import_scene_object(
                       &ctx,
                       scene_path,
                       "obj_prism_guides",
                       DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_HIGH),
                   "runtime_guides_import_prism")) {
        goto cleanup;
    }
    if (!drawing_program_texture_workspace_fit_all(&ctx, pane_rect)) {
        fprintf(stderr, "lifecycle_test: expected fit-all before guide overlay render test\n");
        goto cleanup;
    }
    if (!lifecycle_find_surface_index_by_face_role(&ctx.texture_project,
                                                   DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT,
                                                   &front_surface_index) ||
        !drawing_program_texture_workspace_surface_sheet_metrics(&ctx, pane_rect, front_surface_index, &metrics)) {
        fprintf(stderr, "lifecycle_test: expected front surface metrics for guide overlay render test\n");
        goto cleanup;
    }
    lifecycle_front_corner_sample_point(&metrics, &corner_x, &corner_y);
    lifecycle_front_top_edge_sample_point(&metrics, &edge_x, &edge_y);
    if (core_theme_get_preset(CORE_THEME_PRESET_DARK_DEFAULT, &theme_preset).code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: expected theme preset for guide overlay render test\n");
        goto cleanup;
    }
    memset(&selection, 0, sizeof(selection));
    memset(&panel_ui, 0, sizeof(panel_ui));
    memset(&interaction, 0, sizeof(interaction));
    memset(&hooks, 0, sizeof(hooks));
    hooks.compute_canvas_sheet_metrics = drawing_program_visual_compute_canvas_sheet_metrics;
    hooks.draw_bitmap_text = drawing_program_visual_draw_bitmap_text;
    hooks.draw_selection_overlay = lifecycle_render_noop_selection_overlay;
    hooks.draw_object_overlay = lifecycle_render_noop_object_overlay;
    hooks.draw_shape_preview_overlay = lifecycle_render_noop_shape_preview_overlay;
    surface = SDL_CreateRGBSurfaceWithFormat(0, 1600, 960, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) {
        fprintf(stderr, "lifecycle_test: expected software target surface for guide overlay render test\n");
        goto cleanup;
    }
    renderer = SDL_CreateSoftwareRenderer(surface);
    if (!renderer) {
        fprintf(stderr, "lifecycle_test: expected software renderer for guide overlay render test\n");
        goto cleanup;
    }

    ctx.ui.canvas_guide_mode = (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_OFF;
    (void)SDL_SetRenderDrawColor(renderer, 0u, 0u, 0u, 255u);
    (void)SDL_RenderClear(renderer);
    drawing_program_visual_draw_canvas_world_view(
        renderer, pane_rect, &ctx, &theme_preset, &selection, &panel_ui, &interaction, &hooks);
    (void)SDL_RenderPresent(renderer);
    if (!lifecycle_sample_surface_rgba(surface, corner_x, corner_y, &r, &g, &b, &a)) {
        fprintf(stderr, "lifecycle_test: expected off-mode corner sample inside software surface\n");
        goto cleanup;
    }
    if (r > 32u || g > 32u || b > 32u) {
        fprintf(stderr,
                "lifecycle_test: expected off-mode guide sample to stay dark got rgba=%u,%u,%u,%u\n",
                (unsigned)r,
                (unsigned)g,
                (unsigned)b,
                (unsigned)a);
        goto cleanup;
    }

    ctx.ui.canvas_guide_mode = (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_CORNERS;
    (void)SDL_SetRenderDrawColor(renderer, 0u, 0u, 0u, 255u);
    (void)SDL_RenderClear(renderer);
    drawing_program_visual_draw_canvas_world_view(
        renderer, pane_rect, &ctx, &theme_preset, &selection, &panel_ui, &interaction, &hooks);
    (void)SDL_RenderPresent(renderer);
    if (!lifecycle_sample_surface_rgba(surface, corner_x, corner_y, &r, &g, &b, &a) ||
        r < 200u || g > 150u || b > 150u) {
        fprintf(stderr,
                "lifecycle_test: expected corners guide sample to show front-top-left chip got rgba=%u,%u,%u,%u\n",
                (unsigned)r,
                (unsigned)g,
                (unsigned)b,
                (unsigned)a);
        goto cleanup;
    }
    if (!lifecycle_sample_surface_rgba(surface, edge_x, edge_y, &r, &g, &b, &a) ||
        r > 32u || g > 32u || b > 32u) {
        fprintf(stderr,
                "lifecycle_test: expected corners-only mode to leave top edge dark got rgba=%u,%u,%u,%u\n",
                (unsigned)r,
                (unsigned)g,
                (unsigned)b,
                (unsigned)a);
        goto cleanup;
    }

    ctx.ui.canvas_guide_mode = (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_CORNERS_AND_EDGES;
    (void)SDL_SetRenderDrawColor(renderer, 0u, 0u, 0u, 255u);
    (void)SDL_RenderClear(renderer);
    drawing_program_visual_draw_canvas_world_view(
        renderer, pane_rect, &ctx, &theme_preset, &selection, &panel_ui, &interaction, &hooks);
    (void)SDL_RenderPresent(renderer);
    if (!lifecycle_sample_surface_rgba(surface, edge_x, edge_y, &r, &g, &b, &a) ||
        r < 160u || g < 80u || b < 60u) {
        fprintf(stderr,
                "lifecycle_test: expected corners+edges mode to color the top edge guide got rgba=%u,%u,%u,%u\n",
                (unsigned)r,
                (unsigned)g,
                (unsigned)b,
                (unsigned)a);
        goto cleanup;
    }
    status = 0;

cleanup:
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (surface) {
        SDL_FreeSurface(surface);
    }
    drawing_program_visual_surface_cache_shutdown();
    (void)unlink(scene_path);
    return status;
}

int drawing_program_lifecycle_run_runtime_render_suite(DrawingProgramAppContext *ctx_ptr,
                                                       DrawingProgramAppContext *workflow_ctx_ptr,
                                                       uint32_t center_x,
                                                       uint32_t center_y,
                                                       uint8_t expected_draw_value,
                                                       uint8_t expected_eraser_value)
{
    (void)expected_eraser_value;
#define ctx (*ctx_ptr)
#define workflow_ctx (*workflow_ctx_ptr)

    {
        DrawingProgramVisualInputHandlersHooks hooks;
        VisualCanvasInteractionState interaction;
        VisualPanelUiState panel_ui;
        CoreThemePresetId selected_theme = CORE_THEME_PRESET_DARK_DEFAULT;
        CoreThemePreset theme_preset;
        SDL_Event event;
        memset(&hooks, 0, sizeof(hooks));
        hooks.apply_workflow_control_if_valid = lifecycle_test_apply_workflow_control;
        hooks.visual_transform_session_nudge_object_move = lifecycle_test_nudge_object_move;
        hooks.visual_transform_session_nudge_move = lifecycle_test_nudge_selection_move;
        hooks.cancel_canvas_draw_and_shape = lifecycle_test_cancel_canvas_draw_and_shape;
        hooks.cancel_selection_transient = lifecycle_test_cancel_selection_transient;
        hooks.cancel_all_transient_interactions = lifecycle_test_cancel_all_transient_interactions;
        hooks.delete_active_selection_payload_or_objects = delete_active_selection_payload_or_objects;
        hooks.set_module_type_for_pane = drawing_program_visual_set_module_type_for_pane;
        memset(&interaction, 0, sizeof(interaction));
        memset(&panel_ui, 0, sizeof(panel_ui));

        lifecycle_test_reset_input_handler_counters();
        workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_BRUSH;
        workflow_ctx.object_selection.count = 1u;
        workflow_ctx.object_selection.active_object_id = 99u;
        workflow_ctx.object_selection.object_ids[0] = 99u;
        memset(&event, 0, sizeof(event));
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_r;
        event.key.keysym.mod = KMOD_CTRL;
        if (!drawing_program_visual_input_handle_keydown_payload(&event,
                                                                 0,
                                                                 (SDL_Rect){ 0, 0, 0, 0 },
                                                                 &selected_theme,
                                                                 &theme_preset,
                                                                 &workflow_ctx,
                                                                 &interaction,
                                                                 &workflow_ctx.selection,
                                                                 &panel_ui,
                                                                 &hooks)) {
            fprintf(stderr, "lifecycle_test: expected ctrl+r keydown to be consumed by rasterize action\n");
            return 1;
        }
        if (g_test_apply_workflow_calls != 1u ||
            g_test_last_workflow_control != DRAWING_PROGRAM_WORKFLOW_CONTROL_RASTERIZE_SELECTED_OBJECTS) {
            fprintf(stderr, "lifecycle_test: ctrl+r did not route to rasterize workflow control\n");
            return 1;
        }
        if (workflow_ctx.editor.active_tool != DRAWING_PROGRAM_TOOL_BRUSH) {
            fprintf(stderr, "lifecycle_test: ctrl+r should not switch active tool to rect\n");
            return 1;
        }

        if (drawing_program_lifecycle_run_runtime_path_suite(&workflow_ctx) != 0) {
            return 1;
        }
        if (drawing_program_lifecycle_run_texture_workspace_suite(&workflow_ctx) != 0) {
            return 1;
        }
        if (lifecycle_assert_canvas_world_guide_overlay() != 0) {
            return 1;
        }
        if (lifecycle_assert_canvas_world_cache_draw(&workflow_ctx) != 0) {
            return 1;
        }

        {
            DrawingProgramObjectRecord object_seed;
            uint32_t object_id = 0u;
            memset(&object_seed, 0, sizeof(object_seed));
            drawing_program_object_store_reset(&workflow_ctx.object_store);
            drawing_program_object_selection_reset(&workflow_ctx.object_selection);
            object_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
            object_seed.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT;
            object_seed.visible = 1u;
            object_seed.locked = 0u;
            object_seed.origin_x = 32;
            object_seed.origin_y = 32;
            object_seed.width = 16u;
            object_seed.height = 16u;
            if (!expect_ok(drawing_program_object_store_add(&workflow_ctx.object_store, &object_seed, &object_id),
                           "object_delete_seed_add")) {
                return 1;
            }
            drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, object_id);
            memset(&event, 0, sizeof(event));
            event.type = SDL_KEYDOWN;
            event.key.keysym.sym = SDLK_DELETE;
            event.key.keysym.mod = KMOD_NONE;
            if (!drawing_program_visual_input_handle_keydown_payload(&event,
                                                                     0,
                                                                     (SDL_Rect){ 0, 0, 0, 0 },
                                                                     &selected_theme,
                                                                     &theme_preset,
                                                                     &workflow_ctx,
                                                                     &interaction,
                                                                     &workflow_ctx.selection,
                                                                     &panel_ui,
                                                                     &hooks)) {
                fprintf(stderr, "lifecycle_test: expected delete keydown to be consumed for object selection\n");
                return 1;
            }
            if (workflow_ctx.object_store.object_count != 0u ||
                workflow_ctx.object_selection.count != 0u ||
                workflow_ctx.object_selection.active_object_id != 0u) {
                fprintf(stderr,
                        "lifecycle_test: delete keydown failed to remove selected object count=%u sel_count=%u active=%u\n",
                        (unsigned)workflow_ctx.object_store.object_count,
                        (unsigned)workflow_ctx.object_selection.count,
                        (unsigned)workflow_ctx.object_selection.active_object_id);
                return 1;
            }
        }
        {
            DrawingProgramObjectRecord path_seed;
            DrawingProgramPathPayload path_payload;
            const DrawingProgramObjectRecord *path_object = 0;
            uint32_t object_id = 0u;
            uint32_t units_before = 0u;
            uint32_t units_after = 0u;
            memset(&path_seed, 0, sizeof(path_seed));
            memset(&path_payload, 0, sizeof(path_payload));
            drawing_program_object_store_reset(&workflow_ctx.object_store);
            drawing_program_object_selection_reset(&workflow_ctx.object_selection);
            drawing_program_selection_reset(&workflow_ctx.selection);
            path_seed.layer_id = workflow_ctx.document.layers[0].layer_id;
            path_seed.visible = 1u;
            path_seed.locked = 0u;
            path_seed.stroke_color_value = drawing_program_color_value_from_index(7u);
            path_seed.fill_color_value = drawing_program_color_value_from_index(7u);
            path_seed.stroke_width = 1u;
            path_seed.style_mode = 0u;
            path_payload.point_count = 4u;
            path_payload.closed = 1u;
            path_payload.points[0].x = 24;
            path_payload.points[0].y = 24;
            path_payload.points[1].x = 40;
            path_payload.points[1].y = 24;
            path_payload.points[2].x = 40;
            path_payload.points[2].y = 40;
            path_payload.points[3].x = 24;
            path_payload.points[3].y = 40;
            if (!expect_ok(drawing_program_object_store_add_path(
                               &workflow_ctx.object_store, &path_seed, &path_payload, &object_id),
                           "path_delete_selected_point_seed_add")) {
                return 1;
            }
            drawing_program_object_selection_replace_single(&workflow_ctx.object_selection, object_id);
            if (!drawing_program_object_selection_set_path_point(&workflow_ctx.object_selection, object_id, 1u)) {
                fprintf(stderr, "lifecycle_test: expected selected path point set to succeed\n");
                return 1;
            }
            drawing_program_history_query_units(&workflow_ctx.history, &units_before, 0);
            memset(&event, 0, sizeof(event));
            event.type = SDL_KEYDOWN;
            event.key.keysym.sym = SDLK_DELETE;
            event.key.keysym.mod = KMOD_NONE;
            if (!drawing_program_visual_input_handle_keydown_payload(&event,
                                                                     0,
                                                                     (SDL_Rect){ 0, 0, 0, 0 },
                                                                     &selected_theme,
                                                                     &theme_preset,
                                                                     &workflow_ctx,
                                                                     &interaction,
                                                                     &workflow_ctx.selection,
                                                                     &panel_ui,
                                                                     &hooks)) {
                fprintf(stderr,
                        "lifecycle_test: expected delete keydown to be consumed for selected path point\n");
                return 1;
            }
            drawing_program_history_query_units(&workflow_ctx.history, &units_after, 0);
            if (units_after != units_before + 1u) {
                fprintf(stderr,
                        "lifecycle_test: expected selected path point delete to append one history unit before=%u after=%u\n",
                        (unsigned)units_before,
                        (unsigned)units_after);
                return 1;
            }
            path_object = drawing_program_object_store_get_by_id(&workflow_ctx.object_store, object_id);
            if (!path_object ||
                path_object->path_point_count != 3u ||
                !path_object->path_closed ||
                path_object->path_points[1].x != 40 ||
                path_object->path_points[1].y != 40 ||
                !workflow_ctx.object_selection.selected_path_point_active ||
                workflow_ctx.object_selection.selected_path_point_object_id != object_id ||
                workflow_ctx.object_selection.selected_path_point_index != 1u) {
                fprintf(stderr,
                        "lifecycle_test: selected path point delete did not preserve object/update point selection count=%u closed=%u next=(%d,%d) selected=%u obj=%u idx=%u\n",
                        path_object ? (unsigned)path_object->path_point_count : 0u,
                        path_object ? (unsigned)path_object->path_closed : 0u,
                        path_object ? path_object->path_points[1].x : 0,
                        path_object ? path_object->path_points[1].y : 0,
                        (unsigned)workflow_ctx.object_selection.selected_path_point_active,
                        (unsigned)workflow_ctx.object_selection.selected_path_point_object_id,
                        (unsigned)workflow_ctx.object_selection.selected_path_point_index);
                return 1;
            }
        }

        lifecycle_test_reset_input_handler_counters();
        workflow_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_MOVE;
        workflow_ctx.object_selection.count = 1u;
        workflow_ctx.object_selection.active_object_id = 77u;
        workflow_ctx.object_selection.object_ids[0] = 77u;
        workflow_ctx.selection.has_payload = 1u;
        memset(&event, 0, sizeof(event));
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_RIGHT;
        event.key.keysym.mod = KMOD_NONE;
        if (!drawing_program_visual_input_handle_keydown_payload(&event,
                                                                 0,
                                                                 (SDL_Rect){ 0, 0, 0, 0 },
                                                                 &selected_theme,
                                                                 &theme_preset,
                                                                 &workflow_ctx,
                                                                 &interaction,
                                                                 &workflow_ctx.selection,
                                                                 &panel_ui,
                                                                 &hooks)) {
            fprintf(stderr, "lifecycle_test: expected move nudge keydown to be consumed\n");
            return 1;
        }
        if (g_test_object_nudge_calls != 1u || g_test_selection_nudge_calls != 0u) {
            fprintf(stderr,
                    "lifecycle_test: move nudge precedence should favor object selection when present object_calls=%u selection_calls=%u\n",
                    (unsigned)g_test_object_nudge_calls,
                    (unsigned)g_test_selection_nudge_calls);
            return 1;
        }

        lifecycle_test_reset_input_handler_counters();
        workflow_ctx.object_selection.count = 0u;
        workflow_ctx.object_selection.active_object_id = 0u;
        workflow_ctx.object_selection.object_ids[0] = 0u;
        workflow_ctx.selection.has_payload = 1u;
        memset(&event, 0, sizeof(event));
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_RIGHT;
        event.key.keysym.mod = KMOD_NONE;
        if (!drawing_program_visual_input_handle_keydown_payload(&event,
                                                                 0,
                                                                 (SDL_Rect){ 0, 0, 0, 0 },
                                                                 &selected_theme,
                                                                 &theme_preset,
                                                                 &workflow_ctx,
                                                                 &interaction,
                                                                 &workflow_ctx.selection,
                                                                 &panel_ui,
                                                                 &hooks)) {
            fprintf(stderr, "lifecycle_test: expected selection nudge keydown to be consumed\n");
            return 1;
        }
        if (g_test_object_nudge_calls != 0u || g_test_selection_nudge_calls != 1u) {
            fprintf(stderr,
                    "lifecycle_test: move nudge precedence should fallback to raster selection when object selection is empty object_calls=%u selection_calls=%u\n",
                    (unsigned)g_test_object_nudge_calls,
                    (unsigned)g_test_selection_nudge_calls);
            return 1;
        }

        workflow_ctx.ui.font_zoom_step = 3;
        workflow_ctx.editor.viewport.pan_x = 120.0f;
        workflow_ctx.editor.viewport.pan_y = -80.0f;
        workflow_ctx.editor.viewport.zoom = 3.0f;
        memset(&event, 0, sizeof(event));
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_0;
        event.key.keysym.mod = KMOD_CTRL | KMOD_SHIFT;
        if (!drawing_program_visual_input_handle_keydown_payload(&event,
                                                                 1,
                                                                 (SDL_Rect){ 100, 100, 600, 400 },
                                                                 &selected_theme,
                                                                 &theme_preset,
                                                                 &workflow_ctx,
                                                                 &interaction,
                                                                 &workflow_ctx.selection,
                                                                 &panel_ui,
                                                                 &hooks)) {
            fprintf(stderr, "lifecycle_test: expected ctrl+shift+0 to be consumed for fit reset\n");
            return 1;
        }
        if (workflow_ctx.ui.font_zoom_step != 3) {
            fprintf(stderr,
                    "lifecycle_test: ctrl+shift+0 should not reset font zoom step got=%d\n",
                    (int)workflow_ctx.ui.font_zoom_step);
            return 1;
        }
        if (workflow_ctx.editor.viewport.pan_x == 120.0f &&
            workflow_ctx.editor.viewport.pan_y == -80.0f &&
            workflow_ctx.editor.viewport.zoom == 3.0f) {
            fprintf(stderr, "lifecycle_test: ctrl+shift+0 should change viewport state\n");
            return 1;
        }

        if (workflow_ctx.pane_host.splitter_hit_count == 0u) {
            fprintf(stderr, "lifecycle_test: expected pane host splitter hits for runtime resize contract\n");
            return 1;
        }
        {
            const CorePaneSplitterHit *hit = &workflow_ctx.pane_host.splitter_hits[0];
            float hit_x = hit->splitter_bounds.x + (hit->splitter_bounds.width * 0.5f);
            float hit_y = hit->splitter_bounds.y + (hit->splitter_bounds.height * 0.5f);
            float drag_x = hit_x;
            float drag_y = hit_y;
            workflow_ctx.ui.theme_preset_id = (uint32_t)CORE_THEME_PRESET_LIGHT_DEFAULT;
            workflow_ctx.ui.font_preset_id = (uint32_t)CORE_FONT_PRESET_IDE;
            workflow_ctx.ui.font_zoom_step = 4;
            if (hit->axis == CORE_PANE_AXIS_HORIZONTAL) {
                drag_x += 24.0f;
            } else {
                drag_y += 24.0f;
            }
            if (!drawing_program_pane_host_begin_splitter_drag(&workflow_ctx, hit_x, hit_y)) {
                fprintf(stderr, "lifecycle_test: expected pane host splitter drag begin to succeed\n");
                return 1;
            }
            (void)drawing_program_pane_host_update_splitter_drag(&workflow_ctx, drag_x, drag_y);
            drawing_program_pane_host_end_splitter_drag(&workflow_ctx);
            if (workflow_ctx.ui.theme_preset_id != (uint32_t)CORE_THEME_PRESET_LIGHT_DEFAULT ||
                workflow_ctx.ui.font_preset_id != (uint32_t)CORE_FONT_PRESET_IDE ||
                workflow_ctx.ui.font_zoom_step != 4) {
                fprintf(stderr,
                        "lifecycle_test: pane resize should preserve ui font/theme state theme=%u font=%u zoom=%d\n",
                        (unsigned)workflow_ctx.ui.theme_preset_id,
                        (unsigned)workflow_ctx.ui.font_preset_id,
                        (int)workflow_ctx.ui.font_zoom_step);
                return 1;
            }
        }

        {
            uint32_t module_before = drawing_program_visual_module_type_for_pane(&workflow_ctx, 6u);
            memset(&event, 0, sizeof(event));
            event.type = SDL_KEYDOWN;
            event.key.keysym.sym = SDLK_1;
            event.key.keysym.mod = KMOD_CTRL | KMOD_SHIFT;
            if (drawing_program_visual_input_handle_keydown_payload(&event,
                                                                    1,
                                                                    (SDL_Rect){ 100, 100, 600, 400 },
                                                                    &selected_theme,
                                                                    &theme_preset,
                                                                    &workflow_ctx,
                                                                    &interaction,
                                                                    &workflow_ctx.selection,
                                                                    &panel_ui,
                                                                    &hooks)) {
                fprintf(stderr, "lifecycle_test: ctrl+shift+1 should no longer trigger debug pane swap\n");
                return 1;
            }
            if (drawing_program_visual_module_type_for_pane(&workflow_ctx, 6u) != module_before) {
                fprintf(stderr, "lifecycle_test: ctrl+shift+1 should preserve pane module binding\n");
                return 1;
            }
        }
    }
    if (drawing_program_lifecycle_run_runtime_ui_suite(&ctx, center_x, center_y, expected_draw_value) != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_color_panel_suite(&workflow_ctx) != 0) {
        return 1;
    }

#undef workflow_ctx
#undef ctx
    return 0;
}
