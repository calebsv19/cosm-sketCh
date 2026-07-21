#include "drawing_program/drawing_program_indexed_editor.h"

#include <stdlib.h>

static CoreResult indexed_editor_invalid(const char *message) {
    return (CoreResult){ CORE_ERR_INVALID_ARG, message };
}

int drawing_program_indexed_editor_is_active(const DrawingProgramAppContext *ctx) {
    return ctx &&
           ctx->texture_project.profile_kind == DRAWING_PROGRAM_TEXTURE_PROJECT_PROFILE_INDEXED_ATLAS_V1 &&
           ctx->texture_project.surface_count == 1u &&
           ctx->texture_project.indexed_raster.indices &&
           ctx->texture_project.indexed_profile.slot_count > 0u &&
           ctx->texture_project.indexed_profile.transparent_slot_index <
               ctx->texture_project.indexed_profile.slot_count &&
           ctx->texture_project.indexed_raster.width == ctx->texture_project.indexed_profile.atlas_width &&
           ctx->texture_project.indexed_raster.height == ctx->texture_project.indexed_profile.atlas_height &&
           ctx->texture_project.indexed_raster.slot_count == ctx->texture_project.indexed_profile.slot_count;
}

int drawing_program_indexed_editor_tool_allowed(DrawingProgramToolKind tool) {
    return tool == DRAWING_PROGRAM_TOOL_BRUSH ||
           tool == DRAWING_PROGRAM_TOOL_ERASER ||
           tool == DRAWING_PROGRAM_TOOL_FILL;
}

CoreResult drawing_program_indexed_editor_select_slot(DrawingProgramAppContext *ctx, uint8_t slot_index) {
    if (!drawing_program_indexed_editor_is_active(ctx) ||
        (uint32_t)slot_index >= ctx->texture_project.indexed_profile.slot_count) {
        return indexed_editor_invalid("invalid indexed palette slot selection");
    }
    ctx->ui.indexed_selected_slot = slot_index;
    return core_result_ok();
}

static CoreResult indexed_editor_write(DrawingProgramAppContext *ctx,
                                       uint32_t x,
                                       uint32_t y,
                                       uint8_t value,
                                       DrawingProgramIndexedHistoryCommandKind kind) {
    DrawingProgramIndexedLayerRaster *raster = &ctx->texture_project.indexed_raster;
    DrawingProgramIndexedHistoryDelta delta;
    uint32_t offset;
    if (x >= raster->width || y >= raster->height || (uint32_t)value >= raster->slot_count) {
        return indexed_editor_invalid("indexed edit coordinate or slot out of range");
    }
    offset = y * raster->width + x;
    if (raster->indices[offset] == value) {
        return core_result_ok();
    }
    delta = (DrawingProgramIndexedHistoryDelta){ offset, raster->indices[offset], value, 0u };
    return drawing_program_indexed_history_apply_delta_block_typed(
        &ctx->texture_project.indexed_history, raster, &delta, 1u, kind);
}

static CoreResult indexed_editor_fill(DrawingProgramAppContext *ctx, uint32_t x, uint32_t y, uint8_t value) {
    DrawingProgramIndexedLayerRaster *raster = &ctx->texture_project.indexed_raster;
    DrawingProgramIndexedHistoryDelta *deltas = 0;
    uint32_t *queue = 0;
    uint8_t *visited = 0;
    uint32_t head = 0u;
    uint32_t tail = 0u;
    uint32_t delta_count = 0u;
    uint32_t start;
    uint8_t target;
    CoreResult result;
    if (x >= raster->width || y >= raster->height || (uint32_t)value >= raster->slot_count) {
        return indexed_editor_invalid("indexed fill coordinate or slot out of range");
    }
    start = y * raster->width + x;
    target = raster->indices[start];
    if (target == value) {
        return core_result_ok();
    }
    deltas = (DrawingProgramIndexedHistoryDelta *)malloc(
        (size_t)raster->index_count * sizeof(*deltas));
    queue = (uint32_t *)malloc((size_t)raster->index_count * sizeof(*queue));
    visited = (uint8_t *)calloc((size_t)raster->index_count, sizeof(*visited));
    if (!deltas || !queue || !visited) {
        free(deltas);
        free(queue);
        free(visited);
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate indexed fill workspace" };
    }
    queue[tail++] = start;
    visited[start] = 1u;
    while (head < tail) {
        uint32_t offset = queue[head++];
        uint32_t px = offset % raster->width;
        uint32_t py = offset / raster->width;
        if (raster->indices[offset] != target) {
            continue;
        }
        deltas[delta_count++] = (DrawingProgramIndexedHistoryDelta){ offset, target, value, 0u };
#define QUEUE_INDEXED_NEIGHBOR(next_offset) \
        do { \
            uint32_t neighbor_offset = (next_offset); \
            if (!visited[neighbor_offset]) { \
                visited[neighbor_offset] = 1u; \
                queue[tail++] = neighbor_offset; \
            } \
        } while (0)
        if (px > 0u) QUEUE_INDEXED_NEIGHBOR(offset - 1u);
        if (px + 1u < raster->width) QUEUE_INDEXED_NEIGHBOR(offset + 1u);
        if (py > 0u) QUEUE_INDEXED_NEIGHBOR(offset - raster->width);
        if (py + 1u < raster->height) QUEUE_INDEXED_NEIGHBOR(offset + raster->width);
#undef QUEUE_INDEXED_NEIGHBOR
    }
    result = drawing_program_indexed_history_apply_delta_block_typed(
        &ctx->texture_project.indexed_history,
        raster,
        deltas,
        delta_count,
        DRAWING_PROGRAM_INDEXED_HISTORY_COMMAND_FILL);
    free(deltas);
    free(queue);
    free(visited);
    return result;
}

CoreResult drawing_program_indexed_editor_apply_at(DrawingProgramAppContext *ctx,
                                                   DrawingProgramToolKind tool,
                                                   uint32_t x,
                                                   uint32_t y) {
    uint8_t value;
    if (!drawing_program_indexed_editor_is_active(ctx) ||
        !drawing_program_indexed_editor_tool_allowed(tool)) {
        return indexed_editor_invalid("tool is unavailable for indexed editing");
    }
    value = tool == DRAWING_PROGRAM_TOOL_ERASER
        ? ctx->texture_project.indexed_profile.transparent_slot_index
        : ctx->ui.indexed_selected_slot;
    if ((uint32_t)value >= ctx->texture_project.indexed_profile.slot_count) {
        return indexed_editor_invalid("selected indexed slot is out of range");
    }
    if (tool == DRAWING_PROGRAM_TOOL_FILL) {
        return indexed_editor_fill(ctx, x, y, value);
    }
    return indexed_editor_write(ctx,
                                x,
                                y,
                                value,
                                tool == DRAWING_PROGRAM_TOOL_ERASER
                                    ? DRAWING_PROGRAM_INDEXED_HISTORY_COMMAND_ERASER
                                    : DRAWING_PROGRAM_INDEXED_HISTORY_COMMAND_BRUSH);
}

CoreResult drawing_program_indexed_editor_undo(DrawingProgramAppContext *ctx) {
    if (!drawing_program_indexed_editor_is_active(ctx)) {
        return indexed_editor_invalid("indexed undo unavailable");
    }
    return drawing_program_indexed_history_undo(
        &ctx->texture_project.indexed_history, &ctx->texture_project.indexed_raster);
}

CoreResult drawing_program_indexed_editor_redo(DrawingProgramAppContext *ctx) {
    if (!drawing_program_indexed_editor_is_active(ctx)) {
        return indexed_editor_invalid("indexed redo unavailable");
    }
    return drawing_program_indexed_history_redo(
        &ctx->texture_project.indexed_history, &ctx->texture_project.indexed_raster);
}
