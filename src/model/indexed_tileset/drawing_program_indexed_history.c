#include "drawing_program/drawing_program_indexed_history.h"

#include <stdlib.h>
#include <string.h>

static CoreResult indexed_history_invalid(const char *message) {
    CoreResult result = { CORE_ERR_INVALID_ARG, message };
    return result;
}

static CoreResult indexed_history_reserve(void **buffer,
                                          uint32_t *capacity,
                                          uint32_t required,
                                          size_t element_size) {
    uint32_t next_capacity;
    void *next;
    if (!buffer || !capacity || element_size == 0u) {
        return indexed_history_invalid("invalid indexed history reserve request");
    }
    if (required <= *capacity) {
        return core_result_ok();
    }
    next_capacity = *capacity > 0u ? *capacity : 16u;
    while (next_capacity < required) {
        if (next_capacity > UINT32_MAX / 2u) {
            next_capacity = required;
            break;
        }
        next_capacity *= 2u;
    }
    if ((uint64_t)next_capacity * (uint64_t)element_size > (uint64_t)SIZE_MAX) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "indexed history allocation overflow" };
    }
    next = realloc(*buffer, (size_t)next_capacity * element_size);
    if (!next) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate indexed history" };
    }
    *buffer = next;
    *capacity = next_capacity;
    return core_result_ok();
}

static void indexed_history_trim_redo(DrawingProgramIndexedHistory *history) {
    if (!history || history->command_cursor >= history->command_count) {
        return;
    }
    history->command_count = history->command_cursor;
    history->delta_count = history->command_count > 0u
        ? history->commands[history->command_count - 1u].delta_offset +
          history->commands[history->command_count - 1u].delta_count
        : 0u;
}

void drawing_program_indexed_history_dispose(DrawingProgramIndexedHistory *history) {
    if (!history) {
        return;
    }
    free(history->commands);
    free(history->deltas);
    memset(history, 0, sizeof(*history));
}

void drawing_program_indexed_history_clear(DrawingProgramIndexedHistory *history) {
    if (!history) {
        return;
    }
    history->command_count = 0u;
    history->command_cursor = 0u;
    history->delta_count = 0u;
}

CoreResult drawing_program_indexed_history_apply_delta_block(
    DrawingProgramIndexedHistory *history,
    DrawingProgramIndexedLayerRaster *raster,
    const DrawingProgramIndexedHistoryDelta *deltas,
    uint32_t delta_count) {
    DrawingProgramIndexedHistoryCommand command;
    uint32_t i;
    CoreResult result;
    if (!history || !raster || !deltas || delta_count == 0u ||
        drawing_program_indexed_layer_raster_validate(raster).code != CORE_OK) {
        return indexed_history_invalid("invalid indexed history delta request");
    }
    if (history->command_count == UINT32_MAX ||
        delta_count > UINT32_MAX - history->delta_count) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "indexed history capacity overflow" };
    }
    for (i = 0u; i < delta_count; ++i) {
        if (deltas[i].index_offset >= raster->index_count ||
            deltas[i].previous_index != raster->indices[deltas[i].index_offset] ||
            (uint32_t)deltas[i].new_index >= raster->slot_count) {
            return indexed_history_invalid("indexed history delta does not match raster");
        }
    }
    indexed_history_trim_redo(history);
    result = indexed_history_reserve((void **)&history->commands,
                                     &history->command_capacity,
                                     history->command_count + 1u,
                                     sizeof(history->commands[0]));
    if (result.code != CORE_OK) {
        return result;
    }
    result = indexed_history_reserve((void **)&history->deltas,
                                     &history->delta_capacity,
                                     history->delta_count + delta_count,
                                     sizeof(history->deltas[0]));
    if (result.code != CORE_OK) {
        return result;
    }
    command.delta_offset = history->delta_count;
    command.delta_count = delta_count;
    for (i = 0u; i < delta_count; ++i) {
        raster->indices[deltas[i].index_offset] = deltas[i].new_index;
        history->deltas[history->delta_count + i] = deltas[i];
    }
    history->delta_count += delta_count;
    history->commands[history->command_count++] = command;
    history->command_cursor = history->command_count;
    return core_result_ok();
}

CoreResult drawing_program_indexed_history_apply_write(
    DrawingProgramIndexedHistory *history,
    DrawingProgramIndexedLayerRaster *raster,
    uint32_t x,
    uint32_t y,
    uint8_t new_index) {
    DrawingProgramIndexedHistoryDelta delta;
    if (!raster || !raster->indices || x >= raster->width || y >= raster->height ||
        (uint32_t)new_index >= raster->slot_count) {
        return indexed_history_invalid("invalid indexed history write request");
    }
    memset(&delta, 0, sizeof(delta));
    delta.index_offset = (y * raster->width) + x;
    delta.previous_index = raster->indices[delta.index_offset];
    delta.new_index = new_index;
    if (delta.previous_index == delta.new_index) {
        return core_result_ok();
    }
    return drawing_program_indexed_history_apply_delta_block(history, raster, &delta, 1u);
}

CoreResult drawing_program_indexed_history_undo(
    DrawingProgramIndexedHistory *history,
    DrawingProgramIndexedLayerRaster *raster) {
    const DrawingProgramIndexedHistoryCommand *command;
    uint32_t i;
    if (!history || !raster || history->command_cursor == 0u) {
        return indexed_history_invalid("indexed history undo unavailable");
    }
    command = &history->commands[history->command_cursor - 1u];
    for (i = command->delta_count; i > 0u; --i) {
        const DrawingProgramIndexedHistoryDelta *delta =
            &history->deltas[command->delta_offset + i - 1u];
        if (delta->index_offset >= raster->index_count) {
            return indexed_history_invalid("indexed history undo delta out of bounds");
        }
        raster->indices[delta->index_offset] = delta->previous_index;
    }
    history->command_cursor -= 1u;
    return core_result_ok();
}

CoreResult drawing_program_indexed_history_redo(
    DrawingProgramIndexedHistory *history,
    DrawingProgramIndexedLayerRaster *raster) {
    const DrawingProgramIndexedHistoryCommand *command;
    uint32_t i;
    if (!history || !raster || history->command_cursor >= history->command_count) {
        return indexed_history_invalid("indexed history redo unavailable");
    }
    command = &history->commands[history->command_cursor];
    for (i = 0u; i < command->delta_count; ++i) {
        const DrawingProgramIndexedHistoryDelta *delta =
            &history->deltas[command->delta_offset + i];
        if (delta->index_offset >= raster->index_count ||
            (uint32_t)delta->new_index >= raster->slot_count) {
            return indexed_history_invalid("indexed history redo delta invalid");
        }
        raster->indices[delta->index_offset] = delta->new_index;
    }
    history->command_cursor += 1u;
    return core_result_ok();
}
