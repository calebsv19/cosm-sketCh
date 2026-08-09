#ifndef DRAWING_PROGRAM_RENDER_BACKEND_H
#define DRAWING_PROGRAM_RENDER_BACKEND_H

#include <SDL2/SDL.h>
#include <stdint.h>

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
uint32_t drawing_program_render_backend_window_flags(DrawingProgramRenderBackendKind kind);
SDL_Renderer *drawing_program_render_backend_create(SDL_Window *window,
                                                    DrawingProgramRenderBackendKind kind);
void drawing_program_render_backend_destroy(SDL_Renderer *renderer);
DrawingProgramRenderBackendKind drawing_program_render_backend_active_kind(SDL_Renderer *renderer);
int drawing_program_render_backend_output_size(SDL_Renderer *renderer,
                                               int *width,
                                               int *height);
int drawing_program_render_backend_present(SDL_Renderer *renderer);
int drawing_program_render_backend_request_capture(SDL_Renderer *renderer,
                                                   const char *path);
int drawing_program_render_backend_verify_runtime(SDL_Renderer *renderer,
                                                  const char *stage,
                                                  int require_validation);
int drawing_program_render_backend_drawable_metrics(SDL_Renderer *renderer,
                                                    int *logical_width,
                                                    int *logical_height,
                                                    int *drawable_width,
                                                    int *drawable_height,
                                                    double *scale);

#ifdef __cplusplus
}
#endif

#endif
