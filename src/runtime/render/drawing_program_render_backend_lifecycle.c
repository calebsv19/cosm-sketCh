#include "drawing_program/drawing_program_render_backend.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DRAWING_PROGRAM_USE_VULKAN
#define DRAWING_PROGRAM_USE_VULKAN 0
#endif

#if DRAWING_PROGRAM_USE_VULKAN
#include <SDL2/SDL_vulkan.h>
#include <unistd.h>

#include "vk_renderer.h"
#include "vk_runtime.h"
#endif

enum {
    DRAWING_PROGRAM_VULKAN_CANVAS_WIDTH = 4096,
    DRAWING_PROGRAM_VULKAN_CANVAS_HEIGHT = 4096
};

typedef struct DrawingProgramRenderBackendState {
    SDL_Window *window;
    SDL_Renderer *canvas;
    SDL_Surface *surface;
    DrawingProgramRenderBackendKind kind;
    int drawable_width;
    int drawable_height;
    unsigned long frame_count;
#if DRAWING_PROGRAM_USE_VULKAN
    VkRenderer vk;
    VkRendererTexture texture;
    int vk_initialized;
    int texture_initialized;
    char shader_root[4096];
#endif
} DrawingProgramRenderBackendState;

static DrawingProgramRenderBackendState g_backend;

static int backend_env_enabled(const char *name) {
    const char *value = getenv(name);
    return value && (strcmp(value, "1") == 0 || strcmp(value, "true") == 0 ||
                     strcmp(value, "yes") == 0);
}

static int backend_dummy_video_driver(void) {
    const char *value = getenv("SDL_VIDEODRIVER");
    return value && strcmp(value, "dummy") == 0;
}

static int backend_init_sdl(DrawingProgramRenderBackendState *backend) {
    backend->canvas = SDL_CreateRenderer(
        backend->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!backend->canvas) {
        backend->canvas = SDL_CreateRenderer(backend->window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!backend->canvas) {
        return 0;
    }
    backend->kind = DRAWING_PROGRAM_RENDER_BACKEND_SDL_DEBUG;
    (void)SDL_GetRendererOutputSize(backend->canvas,
                                    &backend->drawable_width,
                                    &backend->drawable_height);
    printf("DRAWING_PROGRAM_RENDERER_BACKEND schema=1 backend=sdl-debug status=ready\n");
    return 1;
}

#if DRAWING_PROGRAM_USE_VULKAN
static int backend_drawable_size(SDL_Window *window, int *width, int *height) {
    if (!window || !width || !height) {
        return 0;
    }
    SDL_Vulkan_GetDrawableSize(window, width, height);
    return *width > 0 && *height > 0 &&
           *width <= DRAWING_PROGRAM_VULKAN_CANVAS_WIDTH &&
           *height <= DRAWING_PROGRAM_VULKAN_CANVAS_HEIGHT;
}

static const char *backend_shader_root(void) {
    static char root[4096];
    const char *override = getenv("DRAWING_PROGRAM_VULKAN_SHADER_ROOT");
    const char *resources = getenv("DRAWING_PROGRAM_RESOURCES_DIR");
    char probe[4096];
    if (override && override[0] &&
        snprintf(probe, sizeof(probe), "%s/shaders/textured.vert.spv", override) > 0 &&
        access(probe, R_OK) == 0) {
        snprintf(root, sizeof(root), "%s", override);
        return root;
    }
    if (resources && resources[0] &&
        snprintf(probe, sizeof(probe), "%s/vk_renderer/shaders/textured.vert.spv", resources) > 0 &&
        access(probe, R_OK) == 0) {
        snprintf(root, sizeof(root), "%s/vk_renderer", resources);
        return root;
    }
    if (access("third_party/codework_shared/vk_renderer/shaders/textured.vert.spv", R_OK) == 0) {
        return "third_party/codework_shared/vk_renderer";
    }
    return NULL;
}

static int backend_run_at_shader_root(DrawingProgramRenderBackendState *backend,
                                      int operation) {
    char previous_cwd[4096];
    VkResult result;
    if (!backend || !backend->shader_root[0] ||
        !getcwd(previous_cwd, sizeof(previous_cwd)) ||
        chdir(backend->shader_root) != 0) {
        return 0;
    }
    if (operation == 0) {
        VkRendererConfig config;
        vk_renderer_config_set_defaults(&config);
        config.enable_validation = backend_env_enabled("DRAWING_PROGRAM_REQUIRE_VK_VALIDATION")
                                       ? VK_TRUE
                                       : VK_FALSE;
        result = vk_renderer_init(&backend->vk, backend->window, &config);
    } else {
        result = vk_renderer_recreate_swapchain(&backend->vk, backend->window);
    }
    if (chdir(previous_cwd) != 0) {
        if (operation == 0 && result == VK_SUCCESS) {
            vk_renderer_shutdown(&backend->vk);
        }
        return 0;
    }
    return result == VK_SUCCESS;
}

static int backend_init_vulkan(DrawingProgramRenderBackendState *backend) {
    const char *shader_root = backend_shader_root();
    if (!shader_root ||
        !backend_drawable_size(backend->window,
                               &backend->drawable_width,
                               &backend->drawable_height)) {
        return 0;
    }
    snprintf(backend->shader_root, sizeof(backend->shader_root), "%s", shader_root);
    backend->surface = SDL_CreateRGBSurfaceWithFormat(
        0,
        DRAWING_PROGRAM_VULKAN_CANVAS_WIDTH,
        DRAWING_PROGRAM_VULKAN_CANVAS_HEIGHT,
        32,
        SDL_PIXELFORMAT_ABGR8888);
    if (backend->surface) {
        backend->canvas = SDL_CreateSoftwareRenderer(backend->surface);
    }
    if (!backend->canvas || !backend_run_at_shader_root(backend, 0)) {
        if (backend->canvas) {
            SDL_DestroyRenderer(backend->canvas);
            backend->canvas = NULL;
        }
        if (backend->surface) {
            SDL_FreeSurface(backend->surface);
            backend->surface = NULL;
        }
        return 0;
    }
    backend->vk_initialized = 1;
    backend->kind = DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT;
    (void)SDL_SetRenderDrawBlendMode(backend->canvas, SDL_BLENDMODE_BLEND);
    printf("DRAWING_PROGRAM_RENDERER_BACKEND schema=1 backend=vulkan-kit status=ready drawable=%dx%d canvas=%dx%d\n",
           backend->drawable_width,
           backend->drawable_height,
           DRAWING_PROGRAM_VULKAN_CANVAS_WIDTH,
           DRAWING_PROGRAM_VULKAN_CANVAS_HEIGHT);
    return 1;
}

static int backend_sync_vulkan_size(DrawingProgramRenderBackendState *backend) {
    int width = 0;
    int height = 0;
    if (!backend || backend->kind != DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT ||
        !backend_drawable_size(backend->window, &width, &height)) {
        return 0;
    }
    if (width == backend->drawable_width && height == backend->drawable_height) {
        return 1;
    }
    vk_renderer_wait_idle(&backend->vk);
    if (!backend_run_at_shader_root(backend, 1)) {
        return 0;
    }
    backend->drawable_width = width;
    backend->drawable_height = height;
    printf("DRAWING_PROGRAM_VULKAN_RESIZE schema=1 status=pass drawable=%dx%d\n",
           width,
           height);
    return 1;
}
#endif

uint32_t drawing_program_render_backend_window_flags(DrawingProgramRenderBackendKind kind) {
    uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
#if DRAWING_PROGRAM_USE_VULKAN
    if (kind == DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT && !backend_dummy_video_driver()) {
        flags |= SDL_WINDOW_VULKAN;
    }
#else
    (void)kind;
#endif
    return flags;
}

SDL_Renderer *drawing_program_render_backend_create(SDL_Window *window,
                                                    DrawingProgramRenderBackendKind kind) {
    DrawingProgramRenderBackendState *backend = &g_backend;
    int require_vulkan = backend_env_enabled("DRAWING_PROGRAM_REQUIRE_VULKAN");
    if (!window || backend->canvas) {
        return NULL;
    }
    memset(backend, 0, sizeof(*backend));
    backend->window = window;
#if DRAWING_PROGRAM_USE_VULKAN
    if (kind == DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT &&
        !backend_dummy_video_driver() && backend_init_vulkan(backend)) {
        return backend->canvas;
    }
#endif
    if ((kind == DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT && require_vulkan) ||
        !backend_init_sdl(backend)) {
        memset(backend, 0, sizeof(*backend));
        return NULL;
    }
    if (kind == DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT) {
        fprintf(stderr,
                "DRAWING_PROGRAM_RENDERER_BACKEND schema=1 backend=vulkan-kit status=fallback backend_next=sdl-debug\n");
    }
    return backend->canvas;
}

void drawing_program_render_backend_destroy(SDL_Renderer *renderer) {
    DrawingProgramRenderBackendState *backend = &g_backend;
    if (!renderer || renderer != backend->canvas) {
        return;
    }
#if DRAWING_PROGRAM_USE_VULKAN
    if (backend->vk_initialized) {
        vk_renderer_wait_idle(&backend->vk);
        if (backend->texture_initialized) {
            vk_renderer_texture_destroy(&backend->vk, &backend->texture);
        }
    }
#endif
    SDL_DestroyRenderer(backend->canvas);
    if (backend->surface) {
        SDL_FreeSurface(backend->surface);
    }
#if DRAWING_PROGRAM_USE_VULKAN
    if (backend->vk_initialized) {
        vk_renderer_shutdown(&backend->vk);
    }
#endif
    printf("DRAWING_PROGRAM_RENDERER_SHUTDOWN schema=1 backend=%s frames=%lu status=pass\n",
           backend->kind == DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT
               ? "vulkan-kit"
               : "sdl-debug",
           backend->frame_count);
    memset(backend, 0, sizeof(*backend));
}

DrawingProgramRenderBackendKind drawing_program_render_backend_active_kind(SDL_Renderer *renderer) {
    return renderer && renderer == g_backend.canvas
               ? g_backend.kind
               : DRAWING_PROGRAM_RENDER_BACKEND_SDL_DEBUG;
}

int drawing_program_render_backend_output_size(SDL_Renderer *renderer,
                                               int *width,
                                               int *height) {
    DrawingProgramRenderBackendState *backend = &g_backend;
    if (!renderer || !width || !height) {
        return -1;
    }
    if (renderer != backend->canvas ||
        backend->kind == DRAWING_PROGRAM_RENDER_BACKEND_SDL_DEBUG) {
        return SDL_GetRendererOutputSize(renderer, width, height);
    }
#if DRAWING_PROGRAM_USE_VULKAN
    if (!backend_drawable_size(backend->window, width, height)) {
        return -1;
    }
    return 0;
#else
    return -1;
#endif
}

int drawing_program_render_backend_present(SDL_Renderer *renderer) {
    DrawingProgramRenderBackendState *backend = &g_backend;
    if (!renderer || renderer != backend->canvas) {
        return 0;
    }
    if (backend->kind == DRAWING_PROGRAM_RENDER_BACKEND_SDL_DEBUG) {
        SDL_RenderPresent(renderer);
        backend->frame_count += 1u;
        return 1;
    }
#if DRAWING_PROGRAM_USE_VULKAN
    {
        VkCommandBuffer command = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VkExtent2D extent = {0};
        SDL_Rect source;
        SDL_Rect destination;
        VkResult result;
        int locked = 0;
        if (!backend_sync_vulkan_size(backend)) {
            return 0;
        }
        if (backend->frame_count == 0u) {
            const char *automatic_capture = getenv("DRAWING_PROGRAM_VULKAN_CAPTURE");
            if (automatic_capture && automatic_capture[0] &&
                vk_renderer_request_capture(&backend->vk, automatic_capture) != VK_SUCCESS) {
                return 0;
            }
        }
        if (SDL_MUSTLOCK(backend->surface)) {
            if (SDL_LockSurface(backend->surface) != 0) {
                return 0;
            }
            locked = 1;
        }
        if (!backend->texture_initialized) {
            result = vk_renderer_upload_sdl_surface_with_filter(&backend->vk,
                                                                backend->surface,
                                                                &backend->texture,
                                                                VK_FILTER_NEAREST);
            if (result == VK_SUCCESS) {
                backend->texture_initialized = 1;
            }
        } else {
            result = vk_renderer_texture_update_rgba_subrect(
                &backend->vk,
                &backend->texture,
                backend->surface->pixels,
                (size_t)backend->surface->pitch,
                0u,
                0u,
                (uint32_t)backend->drawable_width,
                (uint32_t)backend->drawable_height);
        }
        if (locked) {
            SDL_UnlockSurface(backend->surface);
        }
        if (result != VK_SUCCESS) {
            return 0;
        }
        result = vk_renderer_begin_frame(&backend->vk, &command, &framebuffer, &extent);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            if (!backend_run_at_shader_root(backend, 1)) {
                return 0;
            }
            result = vk_renderer_begin_frame(&backend->vk, &command, &framebuffer, &extent);
        }
        if (result != VK_SUCCESS || command == VK_NULL_HANDLE ||
            framebuffer == VK_NULL_HANDLE || extent.width == 0u || extent.height == 0u) {
            return 0;
        }
        source = (SDL_Rect){0, 0, backend->drawable_width, backend->drawable_height};
        destination = (SDL_Rect){0, 0, (int)extent.width, (int)extent.height};
        vk_renderer_set_logical_size(&backend->vk, (float)extent.width, (float)extent.height);
        vk_renderer_set_draw_color(&backend->vk, 1.0f, 1.0f, 1.0f, 1.0f);
        vk_renderer_draw_texture(&backend->vk, &backend->texture, &source, &destination);
        result = vk_renderer_end_frame(&backend->vk, command);
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            return 0;
        }
        backend->frame_count += 1u;
        return 1;
    }
#else
    return 0;
#endif
}

int drawing_program_render_backend_request_capture(SDL_Renderer *renderer,
                                                   const char *path) {
#if DRAWING_PROGRAM_USE_VULKAN
    DrawingProgramRenderBackendState *backend = &g_backend;
    if (!renderer || renderer != backend->canvas ||
        backend->kind != DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT ||
        !path || !path[0] || !backend_sync_vulkan_size(backend)) {
        return 0;
    }
    return vk_renderer_request_capture(&backend->vk, path) == VK_SUCCESS;
#else
    (void)renderer;
    (void)path;
    return 0;
#endif
}

int drawing_program_render_backend_verify_runtime(SDL_Renderer *renderer,
                                                  const char *stage,
                                                  int require_validation) {
#if DRAWING_PROGRAM_USE_VULKAN
    DrawingProgramRenderBackendState *backend = &g_backend;
    const VkRendererDevice *device;
    const VkRuntimeCapabilityReport *report;
    if (!renderer || renderer != backend->canvas ||
        backend->kind != DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT ||
        !backend->vk_initialized || !backend->vk.context.device) {
        return 0;
    }
    device = backend->vk.context.device;
    report = vk_runtime_get_capability_report(&device->runtime);
    if (!report || report->status != VK_RUNTIME_STATUS_OK || report->device_count == 0u ||
        report->selected_device_index >= report->device_count ||
        device->instance != device->runtime.instance ||
        device->device != device->runtime.device ||
        device->graphics_queue != device->runtime.graphics_queue ||
        device->present_queue != device->runtime.present_queue ||
        (require_validation && (!report->validation_requested ||
                                !report->validation_available ||
                                !report->validation_enabled ||
                                report->validation_load_failed)) ||
        report->validation_warning_count != 0u || report->validation_error_count != 0u) {
        fprintf(stderr,
                "DRAWING_PROGRAM_VULKAN_RUNTIME schema=1 stage=%s status=fail\n",
                stage ? stage : "unknown");
        return 0;
    }
    printf("DRAWING_PROGRAM_VULKAN_RUNTIME schema=1 stage=%s status=pass runtime=%s device=%s validation_requested=%u validation_enabled=%u warnings=%u errors=%u handles=shared\n",
           stage ? stage : "unknown",
           vk_runtime_version_string(),
           report->devices[report->selected_device_index].device_name,
           report->validation_requested ? 1u : 0u,
           report->validation_enabled ? 1u : 0u,
           (unsigned int)report->validation_warning_count,
           (unsigned int)report->validation_error_count);
    return 1;
#else
    (void)renderer;
    (void)stage;
    (void)require_validation;
    return 0;
#endif
}

int drawing_program_render_backend_drawable_metrics(SDL_Renderer *renderer,
                                                    int *logical_width,
                                                    int *logical_height,
                                                    int *drawable_width,
                                                    int *drawable_height,
                                                    double *scale) {
    DrawingProgramRenderBackendState *backend = &g_backend;
    double scale_x;
    double scale_y;
    if (!renderer || renderer != backend->canvas || !backend->window ||
        !logical_width || !logical_height || !drawable_width || !drawable_height ||
        !scale) {
        return 0;
    }
    SDL_GetWindowSize(backend->window, logical_width, logical_height);
    if (drawing_program_render_backend_output_size(renderer,
                                                   drawable_width,
                                                   drawable_height) != 0 ||
        *logical_width < 1 || *logical_height < 1) {
        return 0;
    }
    scale_x = (double)(*drawable_width) / (double)(*logical_width);
    scale_y = (double)(*drawable_height) / (double)(*logical_height);
    if (!isfinite(scale_x) || !isfinite(scale_y) || fabs(scale_x - scale_y) > 0.01) {
        return 0;
    }
    *scale = scale_x;
    return 1;
}
