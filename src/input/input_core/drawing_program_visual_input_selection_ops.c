#include "drawing_program/drawing_program_visual_input_selection_ops.h"

#include <string.h>

void drawing_program_visual_input_object_selection_clear(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    drawing_program_object_selection_reset(&ctx->object_selection);
}

void drawing_program_visual_input_object_selection_replace(DrawingProgramAppContext *ctx, uint32_t object_id) {
    if (!ctx) {
        return;
    }
    drawing_program_object_selection_replace_single(&ctx->object_selection, object_id);
}

void drawing_program_visual_input_object_selection_add(DrawingProgramAppContext *ctx, uint32_t object_id) {
    if (!ctx) {
        return;
    }
    (void)drawing_program_object_selection_add(&ctx->object_selection, object_id);
}

void drawing_program_visual_input_object_selection_remove(DrawingProgramAppContext *ctx, uint32_t object_id) {
    uint32_t i;
    uint32_t write_index = 0u;
    uint32_t new_active = 0u;
    if (!ctx || object_id == 0u || ctx->object_selection.count == 0u) {
        return;
    }
    for (i = 0u; i < ctx->object_selection.count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        uint32_t id = ctx->object_selection.object_ids[i];
        if (id == 0u || id == object_id) {
            continue;
        }
        ctx->object_selection.object_ids[write_index] = id;
        new_active = id;
        write_index += 1u;
    }
    for (i = write_index; i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        ctx->object_selection.object_ids[i] = 0u;
    }
    ctx->object_selection.count = write_index;
    ctx->object_selection.active_object_id = new_active;
}

int drawing_program_visual_input_object_selection_hit_test(const DrawingProgramAppContext *ctx,
                                                           uint32_t sample_x,
                                                           uint32_t sample_y,
                                                           uint32_t *out_object_id) {
    CoreResult result;
    if (!ctx || !out_object_id) {
        return 0;
    }
    result = drawing_program_object_store_hit_test_topmost(
        &ctx->object_store, &ctx->document, sample_x, sample_y, out_object_id, 0);
    return (result.code == CORE_OK) ? 1 : 0;
}

VisualMarqueeCommitMode drawing_program_visual_input_resolve_select_commit_mode(
    const DrawingProgramAppContext *ctx,
    SDL_Keymod mods) {
    uint8_t ui_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
    if ((mods & KMOD_ALT) != 0) {
        return VISUAL_MARQUEE_COMMIT_SUBTRACT;
    }
    if ((mods & KMOD_SHIFT) != 0) {
        return VISUAL_MARQUEE_COMMIT_ADD;
    }
    if (ctx) {
        ui_mode = ctx->ui.tool_select_mode;
    }
    if (ui_mode > (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_SUBTRACT) {
        ui_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
    }
    return (VisualMarqueeCommitMode)ui_mode;
}

static uint32_t delete_selected_objects(DrawingProgramAppContext *ctx) {
    uint32_t removed = 0u;
    uint32_t i;
    uint32_t ids[DRAWING_PROGRAM_MAX_OBJECTS];
    if (!ctx || ctx->object_selection.count == 0u) {
        return 0u;
    }
    memset(ids, 0, sizeof(ids));
    for (i = 0u; i < ctx->object_selection.count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        ids[i] = ctx->object_selection.object_ids[i];
    }
    for (i = 0u; i < ctx->object_selection.count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        if (ids[i] == 0u) {
            continue;
        }
        if (drawing_program_object_store_remove_by_id(&ctx->object_store, ids[i], 0).code == CORE_OK) {
            removed += 1u;
        }
    }
    if (removed > 0u) {
        drawing_program_object_selection_reset(&ctx->object_selection);
    }
    return removed;
}

int delete_active_selection_payload_or_objects(DrawingProgramAppContext *ctx,
                                               DrawingProgramSelectionState *selection_state,
                                               const DrawingProgramVisualInputHandlersHooks *hooks) {
    if (!ctx || !selection_state || !hooks) {
        return 0;
    }
    if (ctx->object_selection.count > 0u) {
        if (delete_selected_objects(ctx) > 0u) {
            drawing_program_selection_reset(selection_state);
            return 1;
        }
    }
    if (selection_state->has_payload && hooks->active_layer_allows_edits_visual(ctx)) {
        (void)drawing_program_selection_delete_payload(&ctx->document,
                                                       &ctx->layer_rasters,
                                                       ctx->editor.active_layer_id,
                                                       &ctx->history,
                                                       selection_state);
        return 1;
    }
    return 0;
}
