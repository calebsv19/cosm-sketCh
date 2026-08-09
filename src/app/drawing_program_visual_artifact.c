#include "drawing_program_visual_artifact.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "drawing_program/drawing_program_render_backend.h"

static const char *default_visual_artifact_path(void) {
    return "visual_artifacts/sketch_first_frame.bmp";
}

static int visual_artifact_mkdir_if_needed(const char *path) {
    char dir[512];
    const char *slash = 0;
    size_t dir_len = 0u;

    if (!path || !path[0]) {
        return 0;
    }
    slash = strrchr(path, '/');
    if (!slash) {
        return 1;
    }
    dir_len = (size_t)(slash - path);
    if (dir_len == 0u || dir_len >= sizeof(dir)) {
        return 0;
    }
    memcpy(dir, path, dir_len);
    dir[dir_len] = '\0';
    if (mkdir(dir, 0755) == 0) {
        return 1;
    }
    return errno == EEXIST;
}

DrawingProgramVisualArtifactRequest drawing_program_visual_artifact_parse_request(int argc,
                                                                                   char **argv) {
    DrawingProgramVisualArtifactRequest request;
    int i;
    request.enabled = 0;
    request.path = default_visual_artifact_path();
    if (!argv) {
        return request;
    }
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--visual-artifact") == 0) {
            request.enabled = 1;
            if (i + 1 < argc && argv[i + 1] && argv[i + 1][0] != '-') {
                request.path = argv[i + 1];
                ++i;
            }
        }
    }
    return request;
}

int drawing_program_visual_artifact_filter_app_args(int argc,
                                                    char **argv,
                                                    char **filtered_argv,
                                                    int filtered_capacity) {
    int i;
    int out_argc = 0;
    if (!argv || !filtered_argv || filtered_capacity <= 0) {
        return 0;
    }
    for (i = 0; i < argc && out_argc + 1 < filtered_capacity; ++i) {
        if (i > 0 && strcmp(argv[i], "--visual-artifact") == 0) {
            if (i + 1 < argc && argv[i + 1] && argv[i + 1][0] != '-') {
                ++i;
            }
            continue;
        }
        filtered_argv[out_argc++] = argv[i];
    }
    filtered_argv[out_argc] = 0;
    return out_argc;
}

static int visual_artifact_pixels_are_nonblank(const uint32_t *pixels, int width, int height) {
    int x;
    int y;
    uint32_t first_rgb = 0u;
    int have_first = 0;
    int nonzero_rgb_count = 0;
    int varied_rgb = 0;

    if (!pixels || width <= 0 || height <= 0) {
        return 0;
    }
    for (y = 0; y < height; ++y) {
        const uint32_t *row = pixels + ((size_t)y * (size_t)width);
        for (x = 0; x < width; ++x) {
            uint32_t rgb = row[x] & 0x00FFFFFFu;
            if (!have_first) {
                first_rgb = rgb;
                have_first = 1;
            } else if (rgb != first_rgb) {
                varied_rgb = 1;
            }
            if (rgb != 0u) {
                ++nonzero_rgb_count;
            }
            if (varied_rgb && nonzero_rgb_count > 64) {
                return 1;
            }
        }
    }
    return varied_rgb && nonzero_rgb_count > 0;
}

int drawing_program_visual_artifact_write(SDL_Renderer *renderer, const char *path) {
    int width = 0;
    int height = 0;
    int pitch = 0;
    uint32_t *pixels = 0;
    SDL_Surface *surface = 0;
    int ok = 0;

    if (!renderer) {
        fprintf(stderr, "drawing_program: visual artifact failed: missing renderer\n");
        return 0;
    }
    if (!path || !path[0]) {
        path = default_visual_artifact_path();
    }
    if (drawing_program_render_backend_output_size(renderer, &width, &height) != 0 ||
        width <= 0 || height <= 0) {
        fprintf(stderr, "drawing_program: visual artifact failed: invalid renderer output size: %s\n", SDL_GetError());
        return 0;
    }
    pitch = width * (int)sizeof(uint32_t);
    pixels = (uint32_t *)calloc((size_t)height, (size_t)pitch);
    if (!pixels) {
        fprintf(stderr, "drawing_program: visual artifact failed: out of memory\n");
        return 0;
    }
    {
        SDL_Rect read_rect = {0, 0, width, height};
        if (SDL_RenderReadPixels(renderer,
                                &read_rect,
                                SDL_PIXELFORMAT_ARGB8888,
                                pixels,
                                pitch) != 0) {
            fprintf(stderr,
                    "drawing_program: visual artifact failed: SDL_RenderReadPixels: %s\n",
                    SDL_GetError());
            goto cleanup;
        }
    }
    if (!visual_artifact_pixels_are_nonblank(pixels, width, height)) {
        fprintf(stderr, "drawing_program: visual artifact failed: first frame was blank\n");
        goto cleanup;
    }
    if (drawing_program_render_backend_active_kind(renderer) ==
        DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT) {
        if (!visual_artifact_mkdir_if_needed(path) ||
            !drawing_program_render_backend_request_capture(renderer, path)) {
            fprintf(stderr, "drawing_program: visual artifact failed: Vulkan capture request failed\n");
            goto cleanup;
        }
        printf("Drawing Vulkan visual artifact requested: %s\n", path);
        ok = 1;
        goto cleanup;
    }
    if (!visual_artifact_mkdir_if_needed(path)) {
        fprintf(stderr, "drawing_program: visual artifact failed: could not create output directory for %s\n", path);
        goto cleanup;
    }
    surface = SDL_CreateRGBSurfaceWithFormatFrom(pixels,
                                                 width,
                                                 height,
                                                 32,
                                                 pitch,
                                                 SDL_PIXELFORMAT_ARGB8888);
    if (!surface) {
        fprintf(stderr, "drawing_program: visual artifact failed: SDL_CreateRGBSurfaceWithFormatFrom: %s\n",
                SDL_GetError());
        goto cleanup;
    }
    if (SDL_SaveBMP(surface, path) != 0) {
        fprintf(stderr, "drawing_program: visual artifact failed: SDL_SaveBMP: %s\n", SDL_GetError());
        goto cleanup;
    }
    printf("Drawing visual artifact written: %s\n", path);
    ok = 1;

cleanup:
    if (surface) {
        SDL_FreeSurface(surface);
    }
    free(pixels);
    return ok;
}
