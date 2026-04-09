#include "drawing_program/drawing_program_render_domain.h"

#include <string.h>

#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_editor_state.h"

static CoreResult render_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

CoreResult drawing_program_render_project_frame(
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramEditorState *editor,
    const DrawingProgramRenderInvalidation *invalidation,
    DrawingProgramRenderFrameProjection *out_projection) {
    uint32_t i;
    if (!document || !editor || !invalidation || !out_projection) {
        return render_invalid("null render projection argument");
    }

    memset(out_projection, 0, sizeof(*out_projection));
    out_projection->logical_width = document->logical_width;
    out_projection->logical_height = document->logical_height;
    out_projection->sample_density = document->sample_density;
    out_projection->layer_count = document->layer_count;
    out_projection->active_layer_id = editor->active_layer_id;
    out_projection->invalidation_reason_bits = invalidation->invalidation_reason_bits;
    out_projection->full_redraw =
        (invalidation->full_invalidate || invalidation->full_invalidation_count > 0u) ? 1u : 0u;
    out_projection->targeted_redraw =
        (!out_projection->full_redraw && invalidation->target_invalidation_count > 0u) ? 1u : 0u;

    for (i = 0u; i < document->layer_count; ++i) {
        if (document->layers[i].visible) {
            out_projection->visible_layer_count += 1u;
        }
        if (document->layers[i].layer_id == editor->active_layer_id) {
            out_projection->has_active_layer = 1u;
        }
    }
    out_projection->hidden_layer_count = out_projection->layer_count - out_projection->visible_layer_count;

    return core_result_ok();
}
