#ifndef DRAWING_PROGRAM_INDEXED_HISTORY_H
#define DRAWING_PROGRAM_INDEXED_HISTORY_H

#include <stdint.h>

#include "core_base.h"
#include "drawing_program/drawing_program_indexed_layer_raster.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DrawingProgramIndexedHistoryDelta {
    uint32_t index_offset;
    uint8_t previous_index;
    uint8_t new_index;
    uint16_t reserved0;
} DrawingProgramIndexedHistoryDelta;

typedef enum DrawingProgramIndexedHistoryCommandKind {
    DRAWING_PROGRAM_INDEXED_HISTORY_COMMAND_BRUSH = 1u,
    DRAWING_PROGRAM_INDEXED_HISTORY_COMMAND_ERASER = 2u,
    DRAWING_PROGRAM_INDEXED_HISTORY_COMMAND_FILL = 3u
} DrawingProgramIndexedHistoryCommandKind;

typedef struct DrawingProgramIndexedHistoryCommand {
    uint32_t delta_offset;
    uint32_t delta_count;
    uint32_t kind;
} DrawingProgramIndexedHistoryCommand;

typedef struct DrawingProgramIndexedHistory {
    DrawingProgramIndexedHistoryCommand *commands;
    uint32_t command_count;
    uint32_t command_cursor;
    uint32_t command_capacity;
    DrawingProgramIndexedHistoryDelta *deltas;
    uint32_t delta_count;
    uint32_t delta_capacity;
} DrawingProgramIndexedHistory;

void drawing_program_indexed_history_dispose(DrawingProgramIndexedHistory *history);
void drawing_program_indexed_history_clear(DrawingProgramIndexedHistory *history);
CoreResult drawing_program_indexed_history_apply_write(
    DrawingProgramIndexedHistory *history,
    DrawingProgramIndexedLayerRaster *raster,
    uint32_t x,
    uint32_t y,
    uint8_t new_index);
CoreResult drawing_program_indexed_history_apply_delta_block(
    DrawingProgramIndexedHistory *history,
    DrawingProgramIndexedLayerRaster *raster,
    const DrawingProgramIndexedHistoryDelta *deltas,
    uint32_t delta_count);
CoreResult drawing_program_indexed_history_apply_delta_block_typed(
    DrawingProgramIndexedHistory *history,
    DrawingProgramIndexedLayerRaster *raster,
    const DrawingProgramIndexedHistoryDelta *deltas,
    uint32_t delta_count,
    DrawingProgramIndexedHistoryCommandKind kind);
CoreResult drawing_program_indexed_history_undo(
    DrawingProgramIndexedHistory *history,
    DrawingProgramIndexedLayerRaster *raster);
CoreResult drawing_program_indexed_history_redo(
    DrawingProgramIndexedHistory *history,
    DrawingProgramIndexedLayerRaster *raster);

#ifdef __cplusplus
}
#endif

#endif
