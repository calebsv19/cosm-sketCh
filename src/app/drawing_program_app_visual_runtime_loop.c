#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_font.h"
#include "core_theme.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_render_backend.h"
#include "drawing_program/drawing_program_visual_authoring_chrome.h"
#include "drawing_program/drawing_program_visual_canvas_world_render.h"
#include "drawing_program/drawing_program_visual_input_core.h"
#include "drawing_program/drawing_program_visual_input_handlers.h"
#include "drawing_program/drawing_program_visual_input_keymap.h"
#include "drawing_program/drawing_program_visual_input_support.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_loop_timing.h"
#include "drawing_program/drawing_program_visual_overlay_render.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_visual_resources.h"
#include "drawing_program/drawing_program_visual_runtime_debug.h"
#include "drawing_program/drawing_program_visual_text_render.h"
#include "drawing_program/drawing_program_visual_theme.h"
#include "drawing_program/drawing_program_visual_tool_options.h"
#include "drawing_program_app_visual_runtime.h"
#include "drawing_program_app_visual_runtime_support.h"

enum {
    DRAWING_PROGRAM_LOOP_INPUT_RESPONSE_BOOST_MS = 180
};

typedef struct DrawingProgramVisualLoopEventContext {
    SDL_Window *window;
    SDL_Renderer *renderer;
    DrawingProgramAppContext *app;
    VisualCanvasInteractionState *canvas_interaction;
    VisualSelectionState *selection;
    VisualPanelUiState *panel_ui;
    const DrawingProgramVisualInputHandlersHooks *input_handlers;
    CoreThemePresetId *selected_theme;
    CoreThemePreset *theme_preset;
    int *quit;
    int *resize_pending;
} DrawingProgramVisualLoopEventContext;

static double drawing_program_visual_loop_elapsed_sec(Uint64 begin_counter,
                                                      Uint64 end_counter,
                                                      Uint64 perf_freq) {
    if (perf_freq == 0u || end_counter <= begin_counter) {
        return 0.0;
    }
    return (double)(end_counter - begin_counter) / (double)perf_freq;
}

static int drawing_program_visual_loop_has_continuous_interaction(
    const VisualCanvasInteractionState *interaction) {
    if (!interaction) {
        return 0;
    }
    return interaction->drawing_active ||
           interaction->panning_active ||
           interaction->shape_active ||
           interaction->path_draft_drag_active ||
           interaction->transform_active ||
           interaction->object_move_active ||
           interaction->object_path_point_move_active ||
           interaction->object_path_handle_move_active;
}

static void drawing_program_visual_loop_update_wait_policy(
    DrawingProgramVisualLoopWaitPolicyInput *policy_input,
    const VisualCanvasInteractionState *interaction,
    uint32_t event_count,
    int recent_input_active,
    int resize_pending) {
    int high_intensity_mode = drawing_program_visual_loop_has_continuous_interaction(interaction);
    if (!policy_input) {
        return;
    }
    policy_input->high_intensity_mode = high_intensity_mode ? 1u : 0u;
    policy_input->interaction_active =
        (high_intensity_mode || event_count > 0u || recent_input_active) ? 1u : 0u;
    policy_input->background_busy = 0u;
    policy_input->resize_pending = resize_pending ? 1u : 0u;
}

static void drawing_program_visual_loop_handle_event(DrawingProgramVisualLoopEventContext *ctx,
                                                     const SDL_Event *event) {
    int event_x = 0;
    int event_y = 0;
    int event_has_position = 0;
    SDL_Rect canvas_pane = {0, 0, 0, 0};
    SDL_Rect left_pane = {0, 0, 0, 0};
    SDL_Rect right_pane = {0, 0, 0, 0};
    int has_canvas_pane = 0;
    int has_left_pane = 0;
    int has_right_pane = 0;
    uint8_t clear_mouse_known = 0u;
    uint8_t cancel_transients = 0u;
    int pointer_x = 0;
    int pointer_y = 0;

    if (!ctx || !event) {
        return;
    }

    has_canvas_pane = drawing_program_visual_pane_rect_for_module_type(ctx->app, 1u, &canvas_pane);
    has_left_pane = drawing_program_visual_pane_rect_for_module_type(ctx->app, 2u, &left_pane);
    has_right_pane = drawing_program_visual_pane_rect_for_module_type(ctx->app, 4u, &right_pane);
    drawing_program_visual_input_map_event_position(ctx->window,
                                                    ctx->renderer,
                                                    event,
                                                    &event_x,
                                                    &event_y,
                                                    &event_has_position);
    if (event_has_position) {
        ctx->panel_ui->mouse_known = 1u;
        ctx->panel_ui->mouse_x = event_x;
        ctx->panel_ui->mouse_y = event_y;
    }
    pointer_x = event_has_position ? event_x : 0;
    pointer_y = event_has_position ? event_y : 0;
    if (event->type == SDL_MOUSEMOTION) {
        pointer_x = event_has_position ? event_x : event->motion.x;
        pointer_y = event_has_position ? event_y : event->motion.y;
    }
    if (event->type == SDL_QUIT) {
        *(ctx->quit) = 1;
    }
    if (drawing_program_authoring_host_handle_sdl_event(ctx->app, event)) {
        return;
    }
    if (drawing_program_authoring_host_active(ctx->app) &&
        event->type == SDL_MOUSEBUTTONDOWN &&
        event->button.button == SDL_BUTTON_LEFT &&
        event_has_position) {
        int viewport_w = 0;
        int viewport_h = 0;
        DrawingProgramAuthoringChromeAction action = DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_NONE;
        if (SDL_GetRendererOutputSize(ctx->renderer, &viewport_w, &viewport_h) == 0) {
            action = drawing_program_visual_authoring_chrome_hit_test(viewport_w,
                                                                      viewport_h,
                                                                      event_x,
                                                                      event_y);
        }
        if (action == DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_APPLY) {
            (void)drawing_program_authoring_host_apply(ctx->app);
            return;
        }
        if (action == DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CANCEL) {
            (void)drawing_program_authoring_host_cancel(ctx->app);
            return;
        }
    }
    if (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_ESCAPE) {
        *(ctx->quit) = 1;
    }
    if (event->type == SDL_WINDOWEVENT) {
        drawing_program_visual_input_window_event_flags(event,
                                                        &clear_mouse_known,
                                                        &cancel_transients);
        if (clear_mouse_known) {
            ctx->panel_ui->mouse_known = 0u;
            drawing_program_pane_host_end_splitter_drag(ctx->app);
        }
        if (cancel_transients && ctx->input_handlers &&
            ctx->input_handlers->cancel_all_transient_interactions) {
            ctx->input_handlers->cancel_all_transient_interactions(
                ctx->app, ctx->canvas_interaction, ctx->selection, 1);
        }
        *(ctx->resize_pending) = 1;
    }
    drawing_program_visual_input_apply_mouse_wheel_zoom(ctx->app,
                                                         ctx->window,
                                                         ctx->renderer,
                                                         has_canvas_pane,
                                                         canvas_pane,
                                                         event);
    if (event->type == SDL_MOUSEBUTTONDOWN &&
        event->button.button == SDL_BUTTON_LEFT &&
        event_has_position &&
        drawing_program_pane_host_begin_splitter_drag(ctx->app, (float)event_x, (float)event_y)) {
        return;
    }
    if (event->type == SDL_MOUSEBUTTONUP &&
        event->button.button == SDL_BUTTON_LEFT &&
        drawing_program_pane_host_splitter_drag_active(ctx->app)) {
        drawing_program_pane_host_end_splitter_drag(ctx->app);
        if (event_has_position) {
            (void)drawing_program_pane_host_update_pointer(ctx->app, (float)event_x, (float)event_y);
        }
        return;
    }
    if (event->type == SDL_MOUSEMOTION) {
        if (drawing_program_pane_host_splitter_drag_active(ctx->app)) {
            (void)drawing_program_pane_host_update_splitter_drag(ctx->app,
                                                                 (float)pointer_x,
                                                                 (float)pointer_y);
            return;
        }
        (void)drawing_program_pane_host_update_pointer(ctx->app, (float)pointer_x, (float)pointer_y);
    }
    if (drawing_program_visual_input_handle_mouse_button_down_payload(event,
                                                                      event_has_position,
                                                                      event_x,
                                                                      event_y,
                                                                      has_left_pane,
                                                                      left_pane,
                                                                      has_right_pane,
                                                                      right_pane,
                                                                      has_canvas_pane,
                                                                      canvas_pane,
                                                                      ctx->app,
                                                                      ctx->canvas_interaction,
                                                                      ctx->selection,
                                                                      ctx->panel_ui,
                                                                      ctx->input_handlers)) {
        return;
    }
    if (event->type == SDL_MOUSEBUTTONUP) {
        if (drawing_program_visual_input_handle_mouse_button_up_payload(event,
                                                                        event_has_position,
                                                                        event_x,
                                                                        event_y,
                                                                        has_canvas_pane,
                                                                        canvas_pane,
                                                                        ctx->app,
                                                                        ctx->canvas_interaction,
                                                                        ctx->selection,
                                                                        ctx->input_handlers)) {
            return;
        }
    }
    if (event->type == SDL_MOUSEMOTION) {
        if (drawing_program_visual_input_handle_mouse_motion_payload(event,
                                                                     event_has_position,
                                                                     event_x,
                                                                     event_y,
                                                                     has_canvas_pane,
                                                                     canvas_pane,
                                                                     ctx->app,
                                                                     ctx->canvas_interaction,
                                                                     ctx->selection,
                                                                     ctx->panel_ui,
                                                                     ctx->input_handlers)) {
            return;
        }
    }
    (void)drawing_program_visual_input_handle_keydown_payload(event,
                                                              has_canvas_pane,
                                                              canvas_pane,
                                                              ctx->selected_theme,
                                                              ctx->theme_preset,
                                                              ctx->app,
                                                              ctx->canvas_interaction,
                                                              ctx->selection,
                                                              ctx->panel_ui,
                                                              ctx->input_handlers);
}

static int drawing_program_visual_loop_event_requires_runtime_tick(const SDL_Event *event) {
    if (!event) {
        return 0;
    }
    return event->type == SDL_MOUSEMOTION ? 0 : 1;
}

int drawing_program_app_visual_run_mode(int argc, char **argv) {
    CoreResult result = core_result_ok();
    DrawingProgramRenderBackendKind backend_kind = DRAWING_PROGRAM_RENDER_BACKEND_SDL_DEBUG;
    CoreThemePreset theme_preset;
    CoreResult theme_env_result;
    CoreThemePresetId selected_theme = CORE_THEME_PRESET_DARK_DEFAULT;
    CoreFontPresetId selected_font = CORE_FONT_PRESET_IDE;
    DrawingProgramAppContext *app_ptr = 0;
    SDL_Window *window = 0;
    SDL_Renderer *renderer = 0;
    int quit = 0;
    int exit_code = 1;
    Uint64 perf_freq = 0u;
    uint32_t last_present_ms = 0u;
    uint32_t last_input_event_ms = 0u;
    uint64_t present_count = 0u;
    uint8_t layer_rasters_initialized = 0u;
    VisualCanvasInteractionState canvas_interaction;
    VisualPanelUiState panel_ui;
    DrawingProgramVisualLoopWaitPolicyInput wait_policy_input = {0};
    const DrawingProgramVisualInputHandlersHooks *input_handlers =
        drawing_program_visual_input_handlers_hooks();
#define app_ctx (*app_ptr)
#define selection_state app_ctx.selection

    result = drawing_program_render_backend_parse_flag(argc, argv, &backend_kind);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: %s\n", result.message);
        return 1;
    }
    if (!drawing_program_render_backend_is_supported_now(backend_kind)) {
        fprintf(stderr,
                "drawing_program: render backend '%s' is defined but not implemented in P4-S1 (use --render-backend sdl-debug)\n",
                drawing_program_render_backend_kind_string(backend_kind));
        return 2;
    }
    theme_env_result = core_theme_apply_env_override("CORE_THEME_PRESET", &selected_theme);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "drawing_program: SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("sketCh",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              1280,
                              800,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        fprintf(stderr, "drawing_program: SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        fprintf(stderr, "drawing_program: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    app_ptr = (DrawingProgramAppContext *)calloc(1u, sizeof(*app_ptr));
    if (!app_ptr) {
        fprintf(stderr, "drawing_program: failed to allocate visual app context\n");
        goto cleanup;
    }

    result = drawing_program_app_bootstrap(&app_ctx, argc, argv);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: bootstrap failed: %s\n", result.message);
        goto cleanup;
    }
    result = drawing_program_app_config_load(&app_ctx);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: config_load failed: %s\n", result.message);
        goto cleanup;
    }
    result = drawing_program_app_state_seed(&app_ctx);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: state_seed failed: %s\n", result.message);
        goto cleanup;
    }
    result = drawing_program_app_subsystems_init(&app_ctx);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: subsystems_init failed: %s\n", result.message);
        goto cleanup;
    }
    layer_rasters_initialized = 1u;
    result = drawing_program_runtime_start(&app_ctx);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: runtime_start failed: %s\n", result.message);
        goto cleanup;
    }
    drawing_program_visual_trace_after_runtime_start(&app_ctx);
    if (theme_env_result.code == CORE_OK && !app_ctx.runtime.snapshot_loaded_from_preset) {
        app_ctx.ui.theme_preset_id = (uint32_t)selected_theme;
    }
    selected_theme = clamp_theme_preset_id(app_ctx.ui.theme_preset_id);
    app_ctx.ui.theme_preset_id = (uint32_t)selected_theme;
    if (core_theme_get_preset(selected_theme, &theme_preset).code != CORE_OK) {
        selected_theme = CORE_THEME_PRESET_DARK_DEFAULT;
        app_ctx.ui.theme_preset_id = (uint32_t)selected_theme;
        if (core_theme_get_preset(selected_theme, &theme_preset).code != CORE_OK) {
            fprintf(stderr, "drawing_program: failed to resolve core theme preset\n");
            result = (CoreResult){CORE_ERR_INVALID_ARG, "failed to resolve core theme preset"};
            goto cleanup;
        }
    }
    if (app_ctx.ui.font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
        selected_font = (CoreFontPresetId)app_ctx.ui.font_preset_id;
    } else {
        app_ctx.ui.font_preset_id = (uint32_t)selected_font;
    }
    if (!core_font_preset_name(selected_font)) {
        selected_font = CORE_FONT_PRESET_IDE;
        app_ctx.ui.font_preset_id = (uint32_t)selected_font;
    }
    app_ctx.ui.font_zoom_step = (int8_t)clamp_font_zoom_step((int)app_ctx.ui.font_zoom_step);
    memset(&canvas_interaction, 0, sizeof(canvas_interaction));
    canvas_interaction.marquee_commit_mode = (uint8_t)VISUAL_MARQUEE_COMMIT_REPLACE;
    memset(&panel_ui, 0, sizeof(panel_ui));
    drawing_program_selection_cancel_transient(&selection_state);
    drawing_program_visual_sync_panel_ui_from_app(&app_ctx, &panel_ui);
    drawing_program_visual_trace_after_ui_resolve(&app_ctx, theme_env_result.code == CORE_OK ? 1 : 0);
    perf_freq = SDL_GetPerformanceFrequency();
    wait_policy_input.interaction_active = 1u;
    {
        int initial_w = 0;
        int initial_h = 0;
        if (SDL_GetRendererOutputSize(renderer, &initial_w, &initial_h) == 0 &&
            initial_w > 0 && initial_h > 0) {
            result = drawing_program_app_set_pane_host_bounds(&app_ctx, (float)initial_w, (float)initial_h);
            if (result.code != CORE_OK) {
                fprintf(stderr, "drawing_program: set pane host bounds failed: %s\n", result.message);
                goto cleanup;
            }
        }
    }

    while (!quit) {
        SDL_Event event;
        DrawingProgramVisualLoopEventContext event_ctx;
        int current_w = 0;
        int current_h = 0;
        int wait_timeout_ms = 0;
        int resize_pending = 0;
        int have_wait_event = 0;
        int frame_dirty = 0;
        int force_render = 0;
        int recent_input_active = 0;
        int should_run_runtime_tick = 0;
        int high_intensity_mode = 0;
        int should_render = 0;
        int require_window_sync = 0;
        uint32_t frame_event_count = 0u;
        uint32_t frame_runtime_tick_event_count = 0u;
        uint32_t wait_blocked_ms = 0u;
        uint32_t wait_call_count = 0u;
        uint32_t frame_now_ms = 0u;
        Uint64 frame_begin_counter = SDL_GetPerformanceCounter();
        double frame_elapsed_sec = 0.0;
        DrawingProgramVisualLoopRenderPolicyInput render_policy_input = {0};
        event_ctx.window = window;
        event_ctx.renderer = renderer;
        event_ctx.app = &app_ctx;
        event_ctx.canvas_interaction = &canvas_interaction;
        event_ctx.selection = &selection_state;
        event_ctx.panel_ui = &panel_ui;
        event_ctx.input_handlers = input_handlers;
        event_ctx.selected_theme = &selected_theme;
        event_ctx.theme_preset = &theme_preset;
        event_ctx.quit = &quit;
        event_ctx.resize_pending = &resize_pending;

        wait_timeout_ms = drawing_program_visual_loop_compute_wait_timeout_ms(&wait_policy_input);
        if (wait_timeout_ms > 0) {
            uint32_t wait_start = SDL_GetTicks();
            have_wait_event = SDL_WaitEventTimeout(&event, wait_timeout_ms) == 1;
            wait_blocked_ms += (SDL_GetTicks() - wait_start);
            wait_call_count += 1u;
        }
        if (have_wait_event) {
            drawing_program_visual_loop_handle_event(&event_ctx, &event);
            frame_event_count += 1u;
            if (drawing_program_visual_loop_event_requires_runtime_tick(&event)) {
                frame_runtime_tick_event_count += 1u;
            }
        }
        while (SDL_PollEvent(&event)) {
            drawing_program_visual_loop_handle_event(&event_ctx, &event);
            frame_event_count += 1u;
            if (drawing_program_visual_loop_event_requires_runtime_tick(&event)) {
                frame_runtime_tick_event_count += 1u;
            }
        }
        if (frame_event_count > 0u) {
            last_input_event_ms = (uint32_t)SDL_GetTicks();
        }
        if (quit) {
            break;
        }
        high_intensity_mode = drawing_program_visual_loop_has_continuous_interaction(&canvas_interaction);
        require_window_sync = resize_pending || high_intensity_mode || present_count == 0u;
        if (require_window_sync &&
            SDL_GetRendererOutputSize(renderer, &current_w, &current_h) == 0 &&
            current_w > 0 && current_h > 0) {
            result = drawing_program_app_set_pane_host_bounds(&app_ctx, (float)current_w, (float)current_h);
            if (result.code != CORE_OK) {
                fprintf(stderr, "drawing_program: set pane host bounds failed: %s\n", result.message);
                break;
            }
        }
        frame_now_ms = (uint32_t)SDL_GetTicks();
        if (last_input_event_ms != 0u &&
            (uint32_t)(frame_now_ms - last_input_event_ms) <= DRAWING_PROGRAM_LOOP_INPUT_RESPONSE_BOOST_MS) {
            recent_input_active = 1;
        }
        frame_dirty =
            (resize_pending || high_intensity_mode || frame_event_count > 0u || recent_input_active) ? 1 : 0;
        force_render = (present_count == 0u) ? 1 : 0;
        should_run_runtime_tick =
            (force_render || resize_pending || high_intensity_mode || frame_runtime_tick_event_count > 0u) ? 1 : 0;
        render_policy_input.frame_dirty = (uint8_t)frame_dirty;
        render_policy_input.force_render = (uint8_t)force_render;
        render_policy_input.now_ms = frame_now_ms;
        render_policy_input.last_present_ms = last_present_ms;
        should_render = drawing_program_visual_loop_should_render_frame(&render_policy_input);
        if (!should_render) {
            frame_elapsed_sec = drawing_program_visual_loop_elapsed_sec(frame_begin_counter,
                                                                        SDL_GetPerformanceCounter(),
                                                                        perf_freq);
            drawing_program_visual_loop_diag_tick(frame_elapsed_sec, wait_blocked_ms, wait_call_count);
            drawing_program_visual_loop_update_wait_policy(&wait_policy_input,
                                                           &canvas_interaction,
                                                           frame_event_count,
                                                           recent_input_active,
                                                           resize_pending);
            continue;
        }

        if (should_run_runtime_tick) {
            result = drawing_program_app_run_loop(&app_ctx);
            if (result.code != CORE_OK) {
                fprintf(stderr, "drawing_program: run_loop failed: %s\n", result.message);
                break;
            }
        }
        drawing_program_visual_layer_opacity_sync_document(&app_ctx);
        drawing_program_visual_text_set_font_preset_id(app_ctx.ui.font_preset_id);
        if (!drawing_program_visual_draw_debug_frame(window,
                                                     renderer,
                                                     &app_ctx,
                                                     &theme_preset,
                                                     &panel_ui,
                                                     &selection_state,
                                                     &canvas_interaction)) {
            fprintf(stderr, "drawing_program: visual debug frame draw failed\n");
            result = (CoreResult){CORE_ERR_IO, "visual debug frame draw failed"};
            break;
        }
        SDL_RenderPresent(renderer);
        present_count += 1u;
        last_present_ms = (uint32_t)SDL_GetTicks();
        drawing_program_visual_update_window_title(window, &app_ctx, &selection_state, present_count);
        frame_elapsed_sec = drawing_program_visual_loop_elapsed_sec(frame_begin_counter,
                                                                    SDL_GetPerformanceCounter(),
                                                                    perf_freq);
        drawing_program_visual_loop_diag_tick(frame_elapsed_sec, wait_blocked_ms, wait_call_count);
        drawing_program_visual_loop_update_wait_policy(&wait_policy_input,
                                                       &canvas_interaction,
                                                       frame_event_count,
                                                       recent_input_active,
                                                       resize_pending);
    }

    result = drawing_program_app_shutdown(&app_ctx);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: shutdown failed: %s\n", result.message);
    }
    layer_rasters_initialized = 0u;
    exit_code = (result.code == CORE_OK) ? 0 : 1;

cleanup:
    drawing_program_visual_text_cache_shutdown();
    drawing_program_visual_font_cache_shutdown();
    drawing_program_visual_canvas_world_backdrop_cache_shutdown();
    drawing_program_visual_object_overlay_cache_shutdown();
    drawing_program_visual_canvas_texture_shutdown();
    if (layer_rasters_initialized && app_ptr) {
        drawing_program_layer_raster_store_dispose(&app_ctx.layer_rasters);
    }
    free(app_ptr);
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
    return exit_code;
}

#undef selection_state
#undef app_ctx
