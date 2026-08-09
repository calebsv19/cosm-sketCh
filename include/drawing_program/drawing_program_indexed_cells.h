#ifndef DRAWING_PROGRAM_INDEXED_CELLS_H
#define DRAWING_PROGRAM_INDEXED_CELLS_H

#include <stdint.h>

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_INDEXED_CELL_ID_CAPACITY 64u
#define DRAWING_PROGRAM_INDEXED_CELL_CAPACITY 64u

typedef struct DrawingProgramIndexedCell {
    char id[DRAWING_PROGRAM_INDEXED_CELL_ID_CAPACITY];
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} DrawingProgramIndexedCell;

typedef struct DrawingProgramIndexedCellTable {
    uint32_t count;
    DrawingProgramIndexedCell cells[DRAWING_PROGRAM_INDEXED_CELL_CAPACITY];
} DrawingProgramIndexedCellTable;

typedef struct DrawingProgramIndexedCellHistoryCommand {
    DrawingProgramIndexedCellTable before;
    DrawingProgramIndexedCellTable after;
} DrawingProgramIndexedCellHistoryCommand;

typedef struct DrawingProgramIndexedCellHistory {
    DrawingProgramIndexedCellHistoryCommand *commands;
    uint32_t command_count;
    uint32_t command_cursor;
    uint32_t command_capacity;
} DrawingProgramIndexedCellHistory;

void drawing_program_indexed_cell_table_clear(DrawingProgramIndexedCellTable *table);
CoreResult drawing_program_indexed_cell_table_validate(
    const DrawingProgramIndexedCellTable *table,
    uint32_t atlas_width,
    uint32_t atlas_height,
    uint32_t logical_cell_width,
    uint32_t logical_cell_height);
const DrawingProgramIndexedCell *drawing_program_indexed_cell_table_find(
    const DrawingProgramIndexedCellTable *table,
    const char *id);
CoreResult drawing_program_indexed_cell_create(
    DrawingProgramIndexedCellTable *table,
    DrawingProgramIndexedCellHistory *history,
    const char *id,
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height);
CoreResult drawing_program_indexed_cell_rename(
    DrawingProgramIndexedCellTable *table,
    DrawingProgramIndexedCellHistory *history,
    uint32_t index,
    const char *id);
CoreResult drawing_program_indexed_cell_set_rect(
    DrawingProgramIndexedCellTable *table,
    DrawingProgramIndexedCellHistory *history,
    uint32_t index,
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height);
CoreResult drawing_program_indexed_cell_reorder(
    DrawingProgramIndexedCellTable *table,
    DrawingProgramIndexedCellHistory *history,
    uint32_t from_index,
    uint32_t to_index);
void drawing_program_indexed_cell_history_dispose(DrawingProgramIndexedCellHistory *history);
void drawing_program_indexed_cell_history_clear(DrawingProgramIndexedCellHistory *history);
CoreResult drawing_program_indexed_cell_history_undo(
    DrawingProgramIndexedCellHistory *history,
    DrawingProgramIndexedCellTable *table);
CoreResult drawing_program_indexed_cell_history_redo(
    DrawingProgramIndexedCellHistory *history,
    DrawingProgramIndexedCellTable *table);

#ifdef __cplusplus
}
#endif

#endif
