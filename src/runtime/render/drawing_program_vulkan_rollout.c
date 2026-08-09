#include "drawing_program/drawing_program_vulkan_rollout.h"

#include <SDL2/SDL.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "drawing_program/drawing_program_render_backend.h"

static const char *rollout_path(const char *name, const char *fallback) {
    const char *value = getenv(name);
    return value && value[0] ? value : fallback;
}

static double rollout_minimum_scale(void) {
    const char *value = getenv("DRAWING_PROGRAM_VULKAN_ROLLOUT_MIN_SCALE");
    char *end = NULL;
    double scale = value && value[0] ? strtod(value, &end) : 1.0;
    if (!isfinite(scale) || scale < 1.0 || scale > 4.0 || !end || *end != '\0') {
        return 1.0;
    }
    return scale;
}

static int rollout_draw(SDL_Renderer *renderer, const char *capture_path) {
    int width = 0;
    int height = 0;
    SDL_Rect toolbar;
    SDL_Rect palette;
    SDL_Rect canvas;
    SDL_Rect indexed_cell;
    if (!renderer ||
        drawing_program_render_backend_output_size(renderer, &width, &height) != 0 ||
        width < 1 || height < 1) {
        return 0;
    }
    if (SDL_SetRenderDrawColor(renderer, 15u, 18u, 26u, 255u) != 0 ||
        SDL_RenderClear(renderer) != 0) {
        return 0;
    }
    toolbar = (SDL_Rect){width / 32, height / 28, width * 15 / 16, height / 10};
    palette = (SDL_Rect){width / 32, height / 6, width / 5, height * 3 / 4};
    canvas = (SDL_Rect){width / 4, height / 6, width * 23 / 32, height * 3 / 4};
    indexed_cell = (SDL_Rect){canvas.x + canvas.w / 4,
                              canvas.y + canvas.h / 4,
                              canvas.w / 2,
                              canvas.h / 2};
    SDL_SetRenderDrawColor(renderer, 42u, 54u, 78u, 255u);
    SDL_RenderFillRect(renderer, &toolbar);
    SDL_SetRenderDrawColor(renderer, 61u, 44u, 72u, 255u);
    SDL_RenderFillRect(renderer, &palette);
    SDL_SetRenderDrawColor(renderer, 226u, 220u, 199u, 255u);
    SDL_RenderFillRect(renderer, &canvas);
    SDL_SetRenderDrawColor(renderer, 45u, 94u, 126u, 255u);
    SDL_RenderFillRect(renderer, &indexed_cell);
    SDL_SetRenderDrawColor(renderer, 255u, 202u, 75u, 255u);
    SDL_RenderDrawRect(renderer, &indexed_cell);
    SDL_RenderDrawLine(renderer,
                       indexed_cell.x,
                       indexed_cell.y + indexed_cell.h,
                       indexed_cell.x + indexed_cell.w,
                       indexed_cell.y);
    if (capture_path &&
        !drawing_program_render_backend_request_capture(renderer, capture_path)) {
        return 0;
    }
    return drawing_program_render_backend_present(renderer);
}

static int rollout_metrics(SDL_Renderer *renderer, const char *stage) {
    int logical_width = 0;
    int logical_height = 0;
    int drawable_width = 0;
    int drawable_height = 0;
    double scale = 0.0;
    if (!drawing_program_render_backend_drawable_metrics(renderer,
                                                        &logical_width,
                                                        &logical_height,
                                                        &drawable_width,
                                                        &drawable_height,
                                                        &scale) ||
        scale + 0.01 < rollout_minimum_scale()) {
        return 0;
    }
    printf("DRAWING_PROGRAM_VULKAN_DRAWABLE schema=1 stage=%s status=pass logical=%dx%d drawable=%dx%d scale=%.2f\n",
           stage,
           logical_width,
           logical_height,
           drawable_width,
           drawable_height,
           scale);
    return 1;
}

static int rollout_open(SDL_Window **out_window, SDL_Renderer **out_renderer) {
    SDL_Window *window = SDL_CreateWindow(
        "sketCh Vulkan Rollout Proof",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        640,
        440,
        (int)drawing_program_render_backend_window_flags(
            DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT));
    SDL_Renderer *renderer;
    if (!window) {
        return 0;
    }
    renderer = drawing_program_render_backend_create(
        window, DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT);
    if (!renderer) {
        SDL_DestroyWindow(window);
        return 0;
    }
    *out_window = window;
    *out_renderer = renderer;
    return 1;
}

static void rollout_close(SDL_Window **window, SDL_Renderer **renderer) {
    if (*renderer) {
        drawing_program_render_backend_destroy(*renderer);
        *renderer = NULL;
    }
    if (*window) {
        SDL_DestroyWindow(*window);
        *window = NULL;
    }
}

int drawing_program_vulkan_rollout_self_test(void) {
    const char *initial_capture = rollout_path(
        "DRAWING_PROGRAM_VULKAN_ROLLOUT_INITIAL_CAPTURE",
        "drawing-program-vulkan-initial.bmp");
    const char *resized_capture = rollout_path(
        "DRAWING_PROGRAM_VULKAN_ROLLOUT_RESIZED_CAPTURE",
        "drawing-program-vulkan-resized.bmp");
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    int result = 1;

    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        return 1;
    }
    if (!rollout_open(&window, &renderer) ||
        drawing_program_render_backend_active_kind(renderer) !=
            DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT ||
        !drawing_program_render_backend_verify_runtime(renderer, "startup", 1) ||
        !rollout_metrics(renderer, "startup") ||
        !rollout_draw(renderer, initial_capture)) {
        goto cleanup;
    }

    SDL_SetWindowSize(window, 800, 540);
    SDL_PumpEvents();
    SDL_Delay(50u);
    if (!rollout_draw(renderer, resized_capture) ||
        !drawing_program_render_backend_verify_runtime(renderer, "resized", 1) ||
        !rollout_metrics(renderer, "resized")) {
        goto cleanup;
    }

    rollout_close(&window, &renderer);
    if (!rollout_open(&window, &renderer) ||
        !drawing_program_render_backend_verify_runtime(renderer, "restart", 1) ||
        !rollout_metrics(renderer, "restart") ||
        !rollout_draw(renderer, NULL)) {
        goto cleanup;
    }
    printf("DRAWING_PROGRAM_VULKAN_ROLLOUT schema=1 status=pass compatibility_canvas=sdl filter=nearest runtime=shared resize=recreated capture=native restart=pass\n");
    result = 0;

cleanup:
    rollout_close(&window, &renderer);
    SDL_Quit();
    return result;
}
