#ifndef DRAWING_PROGRAM_VISUAL_INPUT_WORKSPACE_SURFACE_H
#define DRAWING_PROGRAM_VISUAL_INPUT_WORKSPACE_SURFACE_H

#include <SDL2/SDL.h>

#include "core_base.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Atlas hit-testing may browse any visible sheet, but callers should route any
 * edit-surface commitment through this helper so active-surface selection and
 * optional fit-to-edit behavior stay owned in one place. */
CoreResult drawing_program_visual_input_commit_workspace_surface(
    DrawingProgramAppContext *ctx,
    SDL_Rect canvas_pane,
    uint32_t surface_index,
    uint8_t fit_after_commit,
    uint8_t *out_surface_ready);

#ifdef __cplusplus
}
#endif

#endif
