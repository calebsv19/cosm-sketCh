#ifndef DRAWING_PROGRAM_HISTORY_H
#define DRAWING_PROGRAM_HISTORY_H

#include <stdint.h>

#include "core_base.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_layer_raster.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_HISTORY_CAPACITY 16384u

typedef enum DrawingProgramCommandType {
    DRAWING_PROGRAM_COMMAND_NONE = 0,
    DRAWING_PROGRAM_COMMAND_SET_LAYER_VISIBILITY = 1,
    DRAWING_PROGRAM_COMMAND_SET_SAMPLE_VALUE = 2,
    DRAWING_PROGRAM_COMMAND_GROUP_BEGIN = 3,
    DRAWING_PROGRAM_COMMAND_GROUP_END = 4,
    DRAWING_PROGRAM_COMMAND_SET_SAMPLE_SPAN_VALUE = 5
} DrawingProgramCommandType;

typedef struct DrawingProgramCommand {
    DrawingProgramCommandType type;
    uint32_t layer_id;
    uint32_t sample_x;
    uint32_t sample_y;
    uint8_t new_visibility;
    uint8_t previous_visibility;
    uint8_t new_sample_value;
    uint8_t previous_sample_value;
} DrawingProgramCommand;

typedef struct DrawingProgramHistory {
    DrawingProgramCommand entries[DRAWING_PROGRAM_HISTORY_CAPACITY];
    uint32_t count;
    uint32_t cursor;
} DrawingProgramHistory;

void drawing_program_history_init(DrawingProgramHistory *history);
void drawing_program_history_clear(DrawingProgramHistory *history);
CoreResult drawing_program_history_begin_group(DrawingProgramHistory *history);
CoreResult drawing_program_history_end_group(DrawingProgramHistory *history);
void drawing_program_history_query_units(const DrawingProgramHistory *history,
                                         uint32_t *out_cursor_units,
                                         uint32_t *out_count_units);
CoreResult drawing_program_history_apply_set_layer_visibility(DrawingProgramHistory *history,
                                                              DrawingProgramDocument *document,
                                                              uint32_t layer_id,
                                                              uint8_t visible);
CoreResult drawing_program_history_apply_set_sample_value(DrawingProgramHistory *history,
                                                          DrawingProgramDocument *document,
                                                          DrawingProgramLayerRasterStore *layer_rasters,
                                                          uint32_t layer_id,
                                                          uint32_t sample_x,
                                                          uint32_t sample_y,
                                                          uint8_t value);
CoreResult drawing_program_history_apply_set_sample_span_value(DrawingProgramHistory *history,
                                                               DrawingProgramDocument *document,
                                                               DrawingProgramLayerRasterStore *layer_rasters,
                                                               uint32_t layer_id,
                                                               uint32_t span_start_index,
                                                               uint32_t span_length,
                                                               uint8_t value);
CoreResult drawing_program_history_undo(DrawingProgramHistory *history,
                                        DrawingProgramDocument *document,
                                        DrawingProgramLayerRasterStore *layer_rasters);
CoreResult drawing_program_history_redo(DrawingProgramHistory *history,
                                        DrawingProgramDocument *document,
                                        DrawingProgramLayerRasterStore *layer_rasters);

#ifdef __cplusplus
}
#endif

#endif
