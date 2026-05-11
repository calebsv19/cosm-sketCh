#include "drawing_program/drawing_program_texture_canvas_ops.h"

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_texture_project_session.h"

static CoreResult texture_canvas_ops_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

CoreResult drawing_program_texture_canvas_add_blank_from_active(
    DrawingProgramAppContext *ctx,
    uint32_t *out_surface_index) {
    CoreResult result;
    uint32_t surface_index = 0u;
    if (!ctx) {
        return texture_canvas_ops_invalid("invalid texture canvas add blank request");
    }
    result = drawing_program_texture_project_session_add_surface_from_active(ctx, 0, &surface_index);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_texture_project_session_select_surface(ctx, surface_index);
    if (result.code != CORE_OK) {
        return result;
    }
    if (out_surface_index) {
        *out_surface_index = surface_index;
    }
    return core_result_ok();
}

CoreResult drawing_program_texture_canvas_duplicate_active(
    DrawingProgramAppContext *ctx,
    uint32_t *out_surface_index) {
    CoreResult result;
    uint32_t surface_index = 0u;
    if (!ctx) {
        return texture_canvas_ops_invalid("invalid texture canvas duplicate request");
    }
    result = drawing_program_texture_project_session_duplicate_active_surface(ctx, &surface_index);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_texture_project_session_select_surface(ctx, surface_index);
    if (result.code != CORE_OK) {
        return result;
    }
    if (out_surface_index) {
        *out_surface_index = surface_index;
    }
    return core_result_ok();
}

CoreResult drawing_program_texture_canvas_delete_active(
    DrawingProgramAppContext *ctx,
    uint32_t *out_surface_index) {
    CoreResult result;
    uint32_t surface_index = 0u;
    if (!ctx) {
        return texture_canvas_ops_invalid("invalid texture canvas delete request");
    }
    result = drawing_program_texture_project_session_delete_active_surface(ctx, &surface_index);
    if (result.code != CORE_OK) {
        return result;
    }
    if (out_surface_index) {
        *out_surface_index = surface_index;
    }
    return core_result_ok();
}

CoreResult drawing_program_texture_canvas_reset_object_layout(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return texture_canvas_ops_invalid("invalid texture canvas reset object layout request");
    }
    return drawing_program_texture_project_session_reset_object_layout(ctx);
}

void drawing_program_texture_canvas_toggle_control_mode(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    ctx->ui.canvas_control_mode =
        (ctx->ui.canvas_control_mode == (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT)
            ? (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_PAINT
            : (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT;
}

void drawing_program_texture_canvas_cycle_guide_mode(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    switch ((DrawingProgramUiCanvasGuideMode)ctx->ui.canvas_guide_mode) {
        case DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_OFF:
            ctx->ui.canvas_guide_mode = (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_CORNERS;
            break;
        case DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_CORNERS:
            ctx->ui.canvas_guide_mode = (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_CORNERS_AND_EDGES;
            break;
        case DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_CORNERS_AND_EDGES:
        default:
            ctx->ui.canvas_guide_mode = (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_OFF;
            break;
    }
}
