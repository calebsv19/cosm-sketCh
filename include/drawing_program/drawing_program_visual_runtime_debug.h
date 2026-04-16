#ifndef DRAWING_PROGRAM_VISUAL_RUNTIME_DEBUG_H
#define DRAWING_PROGRAM_VISUAL_RUNTIME_DEBUG_H

#include <stdint.h>

#include <SDL2/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;
struct DrawingProgramSelectionState;

int drawing_program_visual_trace_ui_state_enabled(void);
void drawing_program_visual_trace_after_runtime_start(const struct DrawingProgramAppContext *ctx);
void drawing_program_visual_trace_after_ui_resolve(const struct DrawingProgramAppContext *ctx,
                                                   int env_override_applied);
void drawing_program_visual_update_window_title(SDL_Window *window,
                                                const struct DrawingProgramAppContext *ctx,
                                                const struct DrawingProgramSelectionState *selection,
                                                uint64_t present_count);

#ifdef __cplusplus
}
#endif

#endif
