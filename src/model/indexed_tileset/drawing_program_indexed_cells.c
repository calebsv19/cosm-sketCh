#include "drawing_program/drawing_program_indexed_cells.h"

#include <stdlib.h>
#include <string.h>

#include "core_authored_texture.h"

static CoreResult indexed_cells_invalid(const char *message) {
    return (CoreResult){ CORE_ERR_INVALID_ARG, message };
}

static CoreResult indexed_cells_record(DrawingProgramIndexedCellHistory *history,
                                       const DrawingProgramIndexedCellTable *before,
                                       const DrawingProgramIndexedCellTable *after) {
    DrawingProgramIndexedCellHistoryCommand *next;
    uint32_t capacity;
    if (!history || !before || !after) {
        return indexed_cells_invalid("invalid indexed cell history request");
    }
    history->command_count = history->command_cursor;
    if (history->command_count == history->command_capacity) {
        capacity = history->command_capacity ? history->command_capacity * 2u : 8u;
        if (capacity < history->command_capacity ||
            (uint64_t)capacity * sizeof(*next) > (uint64_t)SIZE_MAX) {
            return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "indexed cell history overflow" };
        }
        next = (DrawingProgramIndexedCellHistoryCommand *)realloc(
            history->commands, (size_t)capacity * sizeof(*next));
        if (!next) {
            return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate indexed cell history" };
        }
        history->commands = next;
        history->command_capacity = capacity;
    }
    history->commands[history->command_count].before = *before;
    history->commands[history->command_count].after = *after;
    history->command_count += 1u;
    history->command_cursor = history->command_count;
    return core_result_ok();
}

void drawing_program_indexed_cell_table_clear(DrawingProgramIndexedCellTable *table) {
    if (table) memset(table, 0, sizeof(*table));
}

CoreResult drawing_program_indexed_cell_table_validate(
    const DrawingProgramIndexedCellTable *table,
    uint32_t atlas_width,
    uint32_t atlas_height,
    uint32_t logical_cell_width,
    uint32_t logical_cell_height) {
    CoreAuthoredTextureAtlasCell cells[DRAWING_PROGRAM_INDEXED_CELL_CAPACITY];
    CoreAuthoredTextureIndexedAtlasContract contract;
    uint32_t i;
    if (!table || table->count == 0u || table->count > DRAWING_PROGRAM_INDEXED_CELL_CAPACITY) {
        return indexed_cells_invalid("indexed atlas requires one or more named cells");
    }
    memset(&contract, 0, sizeof(contract));
    for (i = 0u; i < table->count; ++i) {
        cells[i].id = table->cells[i].id;
        cells[i].x = table->cells[i].x;
        cells[i].y = table->cells[i].y;
        cells[i].width = table->cells[i].width;
        cells[i].height = table->cells[i].height;
    }
    contract.revision = CORE_AUTHORED_TEXTURE_INDEXED_CONTRACT_REVISION_V1;
    contract.atlas_width = atlas_width;
    contract.atlas_height = atlas_height;
    contract.logical_cell_width = logical_cell_width;
    contract.logical_cell_height = logical_cell_height;
    contract.output_kind = CORE_AUTHORED_TEXTURE_OUTPUT_KIND_INDEX_ATLAS;
    contract.image_ref = "pending-index-atlas.png";
    contract.cells = cells;
    contract.cell_count = table->count;
    if (!core_authored_texture_indexed_atlas_validate(&contract)) {
        return indexed_cells_invalid("indexed cell table violates atlas contract");
    }
    return core_result_ok();
}

const DrawingProgramIndexedCell *drawing_program_indexed_cell_table_find(
    const DrawingProgramIndexedCellTable *table,
    const char *id) {
    uint32_t i;
    if (!table || !id || !core_authored_texture_identifier_validate(id)) return 0;
    for (i = 0u; i < table->count; ++i) {
        if (strcmp(table->cells[i].id, id) == 0) return &table->cells[i];
    }
    return 0;
}

CoreResult drawing_program_indexed_cell_create(
    DrawingProgramIndexedCellTable *table,
    DrawingProgramIndexedCellHistory *history,
    const char *id,
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height) {
    DrawingProgramIndexedCellTable before;
    DrawingProgramIndexedCellTable after;
    size_t length;
    CoreResult result;
    if (!table || !history || !id || table->count >= DRAWING_PROGRAM_INDEXED_CELL_CAPACITY ||
        !core_authored_texture_identifier_validate(id) ||
        drawing_program_indexed_cell_table_find(table, id)) {
        return indexed_cells_invalid("invalid indexed cell create request");
    }
    length = strlen(id);
    before = *table;
    after = before;
    memset(&after.cells[after.count], 0, sizeof(after.cells[after.count]));
    memcpy(after.cells[after.count].id, id, length + 1u);
    after.cells[after.count].x = x;
    after.cells[after.count].y = y;
    after.cells[after.count].width = width;
    after.cells[after.count].height = height;
    after.count += 1u;
    result = indexed_cells_record(history, &before, &after);
    if (result.code == CORE_OK) *table = after;
    return result;
}

CoreResult drawing_program_indexed_cell_rename(
    DrawingProgramIndexedCellTable *table,
    DrawingProgramIndexedCellHistory *history,
    uint32_t index,
    const char *id) {
    DrawingProgramIndexedCellTable before;
    DrawingProgramIndexedCellTable after;
    const DrawingProgramIndexedCell *existing;
    CoreResult result;
    if (!table || !history || index >= table->count || !id ||
        !core_authored_texture_identifier_validate(id)) {
        return indexed_cells_invalid("invalid indexed cell rename request");
    }
    existing = drawing_program_indexed_cell_table_find(table, id);
    if (existing && existing != &table->cells[index]) {
        return indexed_cells_invalid("indexed cell id already exists");
    }
    before = *table;
    after = before;
    memset(after.cells[index].id, 0, sizeof(after.cells[index].id));
    memcpy(after.cells[index].id, id, strlen(id) + 1u);
    result = indexed_cells_record(history, &before, &after);
    if (result.code == CORE_OK) *table = after;
    return result;
}

CoreResult drawing_program_indexed_cell_set_rect(
    DrawingProgramIndexedCellTable *table,
    DrawingProgramIndexedCellHistory *history,
    uint32_t index,
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height) {
    DrawingProgramIndexedCellTable before;
    DrawingProgramIndexedCellTable after;
    CoreResult result;
    if (!table || !history || index >= table->count || width == 0u || height == 0u) {
        return indexed_cells_invalid("invalid indexed cell rectangle request");
    }
    before = *table;
    after = before;
    after.cells[index].x = x;
    after.cells[index].y = y;
    after.cells[index].width = width;
    after.cells[index].height = height;
    result = indexed_cells_record(history, &before, &after);
    if (result.code == CORE_OK) *table = after;
    return result;
}

CoreResult drawing_program_indexed_cell_reorder(
    DrawingProgramIndexedCellTable *table,
    DrawingProgramIndexedCellHistory *history,
    uint32_t from_index,
    uint32_t to_index) {
    DrawingProgramIndexedCellTable before;
    DrawingProgramIndexedCellTable after;
    DrawingProgramIndexedCell moved;
    CoreResult result;
    if (!table || !history || from_index >= table->count || to_index >= table->count) {
        return indexed_cells_invalid("invalid indexed cell reorder request");
    }
    if (from_index == to_index) return core_result_ok();
    before = *table;
    after = before;
    moved = after.cells[from_index];
    if (from_index < to_index) {
        memmove(&after.cells[from_index], &after.cells[from_index + 1u],
                (size_t)(to_index - from_index) * sizeof(after.cells[0]));
    } else {
        memmove(&after.cells[to_index + 1u], &after.cells[to_index],
                (size_t)(from_index - to_index) * sizeof(after.cells[0]));
    }
    after.cells[to_index] = moved;
    result = indexed_cells_record(history, &before, &after);
    if (result.code == CORE_OK) *table = after;
    return result;
}

void drawing_program_indexed_cell_history_dispose(DrawingProgramIndexedCellHistory *history) {
    if (!history) return;
    free(history->commands);
    memset(history, 0, sizeof(*history));
}

void drawing_program_indexed_cell_history_clear(DrawingProgramIndexedCellHistory *history) {
    if (!history) return;
    history->command_count = 0u;
    history->command_cursor = 0u;
}

CoreResult drawing_program_indexed_cell_history_undo(
    DrawingProgramIndexedCellHistory *history,
    DrawingProgramIndexedCellTable *table) {
    if (!history || !table || history->command_cursor == 0u) {
        return indexed_cells_invalid("indexed cell history undo unavailable");
    }
    history->command_cursor -= 1u;
    *table = history->commands[history->command_cursor].before;
    return core_result_ok();
}

CoreResult drawing_program_indexed_cell_history_redo(
    DrawingProgramIndexedCellHistory *history,
    DrawingProgramIndexedCellTable *table) {
    if (!history || !table || history->command_cursor >= history->command_count) {
        return indexed_cells_invalid("indexed cell history redo unavailable");
    }
    *table = history->commands[history->command_cursor].after;
    history->command_cursor += 1u;
    return core_result_ok();
}
