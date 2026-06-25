#include "drawing_program/drawing_program_app_post_load.h"

#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_render_runtime_state.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"

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
    drawing_program_render_runtime_reset_document_derived_state(ctx);
}

void drawing_program_app_rearm_after_snapshot_load(DrawingProgramAppContext *ctx,
                                                   DrawingProgramSelectionState *selection,
                                                   int preserve_project_clean_state) {
    if (!ctx) {
        return;
    }

    drawing_program_selection_reset(selection ? selection : &ctx->selection);
    drawing_program_object_selection_reset(&ctx->object_selection);
    if (!preserve_project_clean_state) {
        ctx->session.project_has_saved_state = 0u;
    }
    drawing_program_visual_layer_opacity_sync_document(ctx);
}
