#include "drawing_program/drawing_program_visual_input_workspace_surface.h"

#include "drawing_program/drawing_program_texture_project_session.h"
#include "drawing_program/drawing_program_texture_workspace.h"

CoreResult drawing_program_visual_input_commit_workspace_surface(
    DrawingProgramAppContext *ctx,
    SDL_Rect canvas_pane,
    uint32_t surface_index,
    uint8_t fit_after_commit,
    uint8_t *out_surface_ready) {
    CoreResult result = core_result_ok();
    if (out_surface_ready) {
        *out_surface_ready = 0u;
    }
    if (!ctx) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid workspace surface commit request" };
    }
    if (surface_index >= ctx->texture_project.surface_count) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "workspace surface index out of range" };
    }
    if (surface_index != ctx->texture_project.active_surface_index) {
        result = drawing_program_texture_project_session_select_surface(ctx, surface_index);
        if (result.code != CORE_OK) {
            return result;
        }
    }
    if (fit_after_commit) {
        (void)drawing_program_texture_workspace_fit_surface(ctx, canvas_pane, surface_index);
    }
    if (out_surface_ready) {
        *out_surface_ready = 1u;
    }
    return core_result_ok();
}
