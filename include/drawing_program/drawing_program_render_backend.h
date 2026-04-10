#ifndef DRAWING_PROGRAM_RENDER_BACKEND_H
#define DRAWING_PROGRAM_RENDER_BACKEND_H

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DrawingProgramRenderBackendKind {
    DRAWING_PROGRAM_RENDER_BACKEND_SDL_DEBUG = 1,
    DRAWING_PROGRAM_RENDER_BACKEND_VULKAN_KIT = 2
} DrawingProgramRenderBackendKind;

CoreResult drawing_program_render_backend_parse_flag(int argc,
                                                     char **argv,
                                                     DrawingProgramRenderBackendKind *out_kind);
const char *drawing_program_render_backend_kind_string(DrawingProgramRenderBackendKind kind);
int drawing_program_render_backend_is_supported_now(DrawingProgramRenderBackendKind kind);

#ifdef __cplusplus
}
#endif

#endif

