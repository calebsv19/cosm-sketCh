#include "drawing_program/drawing_program_render_backend.h"

#include <string.h>

static CoreResult render_backend_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

const char *drawing_program_render_backend_kind_string(DrawingProgramRenderBackendKind kind) {
    switch (kind) {
        case DRAWING_PROGRAM_RENDER_BACKEND_SDL_DEBUG:
            return "sdl-debug";
        case DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT:
            return "vulkan-kit";
        default:
            return "unknown";
    }
}

int drawing_program_render_backend_is_supported_now(DrawingProgramRenderBackendKind kind) {
    return kind == DRAWING_PROGRAM_RENDER_BACKEND_SDL_DEBUG ? 1 : 0;
}

CoreResult drawing_program_render_backend_parse_flag(int argc,
                                                     char **argv,
                                                     DrawingProgramRenderBackendKind *out_kind) {
    int i;
    if (!argv || !out_kind) {
        return render_backend_invalid("invalid render backend parse request");
    }
    *out_kind = DRAWING_PROGRAM_RENDER_BACKEND_SDL_DEBUG;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--render-backend") == 0 && i + 1 < argc) {
            const char *value = argv[++i];
            if (strcmp(value, "sdl-debug") == 0) {
                *out_kind = DRAWING_PROGRAM_RENDER_BACKEND_SDL_DEBUG;
                continue;
            }
            if (strcmp(value, "vulkan-kit") == 0) {
                *out_kind = DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT;
                continue;
            }
            return (CoreResult){ CORE_ERR_INVALID_ARG, "unsupported --render-backend value" };
        }
    }
    return core_result_ok();
}

