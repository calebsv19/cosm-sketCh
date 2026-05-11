#ifndef DRAWING_PROGRAM_HISTORY_INTERNAL_H
#define DRAWING_PROGRAM_HISTORY_INTERNAL_H

#include "drawing_program/drawing_program_history.h"

void drawing_program_history_push(DrawingProgramHistory *history, const DrawingProgramCommand *command);
int drawing_program_history_command_uses_raster_delta(const DrawingProgramCommand *command);
uint32_t drawing_program_history_raster_delta_count_for_limit(const DrawingProgramHistory *history, uint32_t limit);
void drawing_program_history_raster_delta_trim_to_count(DrawingProgramHistory *history, uint32_t kept_count);
void drawing_program_history_raster_delta_drop_prefix(DrawingProgramHistory *history, uint32_t drop_count);
CoreResult drawing_program_history_apply_raster_delta_command(const DrawingProgramHistory *history,
                                                              const DrawingProgramCommand *command,
                                                              DrawingProgramDocument *document,
                                                              DrawingProgramLayerRasterStore *layer_rasters,
                                                              int use_previous_values);
CoreResult drawing_program_history_validate_raster_delta_storage(const DrawingProgramHistory *history);

#endif
