#include "drawing_program/drawing_program_render_runtime_state.h"

#include <string.h>

#include "drawing_program/drawing_program_render_revision.h"

void drawing_program_render_runtime_reset_document_derived_state(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }

    memset(&ctx->runtime.render_projection, 0, sizeof(ctx->runtime.render_projection));
    ctx->runtime.render_canvas_last_raster_hash = 0u;
    ctx->runtime.render_canvas_last_nonzero_samples = 0u;
    ctx->runtime.render_last_active_layer_id = 0u;
    ctx->runtime.render_last_has_active_layer = 0u;
    drawing_program_render_revision_refresh(ctx);
    drawing_program_render_revision_mark_layer_opacity_changed(ctx);
}
