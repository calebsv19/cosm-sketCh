#include "drawing_program/drawing_program_app_post_load.h"

#include <string.h>

#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_render_revision.h"

void drawing_program_app_rearm_after_document_swap(DrawingProgramAppContext *ctx) {
    uint32_t active_index = 0u;
    if (!ctx) {
        return;
    }

    if (ctx->document.layer_count > 0u &&
        drawing_program_document_layer_index_for_id(&ctx->document,
                                                    ctx->editor.active_layer_id,
                                                    &active_index).code != CORE_OK) {
        ctx->editor.active_layer_id = ctx->document.layers[0].layer_id;
    }
    drawing_program_canvas_reflection_sync_editor_from_active_surface(ctx);

    drawing_program_selection_cancel_transient(&ctx->selection);
    memset(&ctx->runtime.render_projection, 0, sizeof(ctx->runtime.render_projection));
    ctx->runtime.render_canvas_last_raster_hash = 0u;
    ctx->runtime.render_canvas_last_nonzero_samples = 0u;
    ctx->runtime.render_last_active_layer_id = 0u;
    ctx->runtime.render_last_has_active_layer = 0u;
    drawing_program_render_revision_refresh(ctx);
    drawing_program_render_revision_mark_layer_opacity_changed(ctx);
}
