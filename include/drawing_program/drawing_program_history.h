#ifndef DRAWING_PROGRAM_HISTORY_H
#define DRAWING_PROGRAM_HISTORY_H

#include <stdint.h>

#include "core_base.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_layer_raster.h"
#include "drawing_program/drawing_program_object_store.h"

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
    DRAWING_PROGRAM_COMMAND_SET_SAMPLE_SPAN_VALUE = 5,
    DRAWING_PROGRAM_COMMAND_SET_OBJECT_ORIGIN = 6,
    DRAWING_PROGRAM_COMMAND_SET_OBJECT_PATH_POINT = 7,
    DRAWING_PROGRAM_COMMAND_SET_OBJECT_STROKE_WIDTH = 8,
    DRAWING_PROGRAM_COMMAND_SET_OBJECT_STYLE_MODE = 9,
    DRAWING_PROGRAM_COMMAND_SET_OBJECT_PATH_CLOSED = 10,
    DRAWING_PROGRAM_COMMAND_SET_OBJECT_STROKE_COLOR = 11,
    DRAWING_PROGRAM_COMMAND_SET_OBJECT_FILL_COLOR = 12,
    DRAWING_PROGRAM_COMMAND_INSERT_OBJECT_PATH_POINT = 13,
    DRAWING_PROGRAM_COMMAND_REMOVE_OBJECT_PATH_POINT = 14,
    DRAWING_PROGRAM_COMMAND_SET_OBJECT_SIZE = 15
} DrawingProgramCommandType;

typedef struct DrawingProgramCommand {
    DrawingProgramCommandType type;
    uint32_t layer_id;
    uint32_t sample_x;
    uint32_t sample_y;
    uint8_t new_visibility;
    uint8_t previous_visibility;
    DrawingProgramRasterSample new_sample_value;
    DrawingProgramRasterSample previous_sample_value;
    uint32_t object_id;
    int32_t new_object_origin_x;
    int32_t new_object_origin_y;
    int32_t previous_object_origin_x;
    int32_t previous_object_origin_y;
    uint32_t new_object_width;
    uint32_t new_object_height;
    uint32_t previous_object_width;
    uint32_t previous_object_height;
    uint8_t new_object_stroke_width;
    uint8_t previous_object_stroke_width;
    uint8_t new_object_style_mode;
    uint8_t previous_object_style_mode;
    uint8_t new_object_path_closed;
    uint8_t previous_object_path_closed;
    DrawingProgramRasterSample new_object_stroke_color_value;
    DrawingProgramRasterSample previous_object_stroke_color_value;
    DrawingProgramRasterSample new_object_fill_color_value;
    DrawingProgramRasterSample previous_object_fill_color_value;
    uint16_t path_point_index;
    uint16_t path_point_reserved0;
    int32_t new_path_point_x;
    int32_t new_path_point_y;
    int32_t previous_path_point_x;
    int32_t previous_path_point_y;
    int32_t new_path_point_handle_in_dx;
    int32_t new_path_point_handle_in_dy;
    int32_t previous_path_point_handle_in_dx;
    int32_t previous_path_point_handle_in_dy;
    int32_t new_path_point_handle_out_dx;
    int32_t new_path_point_handle_out_dy;
    int32_t previous_path_point_handle_out_dx;
    int32_t previous_path_point_handle_out_dy;
    uint8_t new_path_point_bezier_enabled;
    uint8_t previous_path_point_bezier_enabled;
    uint8_t new_path_point_handle_linked;
    uint8_t previous_path_point_handle_linked;
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
                                                          DrawingProgramRasterSample value);
CoreResult drawing_program_history_apply_set_sample_span_value(DrawingProgramHistory *history,
                                                               DrawingProgramDocument *document,
                                                               DrawingProgramLayerRasterStore *layer_rasters,
                                                               uint32_t layer_id,
                                                               uint32_t span_start_index,
                                                               uint32_t span_length,
                                                               DrawingProgramRasterSample value);
CoreResult drawing_program_history_apply_set_object_origin(DrawingProgramHistory *history,
                                                           DrawingProgramObjectStore *object_store,
                                                           uint32_t object_id,
                                                           int32_t origin_x,
                                                           int32_t origin_y);
CoreResult drawing_program_history_apply_set_object_path_point(DrawingProgramHistory *history,
                                                               DrawingProgramObjectStore *object_store,
                                                               uint32_t object_id,
                                                               uint16_t point_index,
                                                               int32_t point_x,
                                                               int32_t point_y);
CoreResult drawing_program_history_apply_set_object_path_point_data(DrawingProgramHistory *history,
                                                                    DrawingProgramObjectStore *object_store,
                                                                    uint32_t object_id,
                                                                    uint16_t point_index,
                                                                    const DrawingProgramPathPoint *point);
CoreResult drawing_program_history_apply_set_object_stroke_width(DrawingProgramHistory *history,
                                                                 DrawingProgramObjectStore *object_store,
                                                                 uint32_t object_id,
                                                                 uint8_t stroke_width);
CoreResult drawing_program_history_apply_set_object_style_mode(DrawingProgramHistory *history,
                                                               DrawingProgramObjectStore *object_store,
                                                               uint32_t object_id,
                                                               uint8_t style_mode);
CoreResult drawing_program_history_apply_set_object_path_closed(DrawingProgramHistory *history,
                                                                DrawingProgramObjectStore *object_store,
                                                                uint32_t object_id,
                                                                uint8_t closed);
CoreResult drawing_program_history_apply_set_object_stroke_color(DrawingProgramHistory *history,
                                                                 DrawingProgramObjectStore *object_store,
                                                                 uint32_t object_id,
                                                                 DrawingProgramRasterSample color_value);
CoreResult drawing_program_history_apply_set_object_fill_color(DrawingProgramHistory *history,
                                                               DrawingProgramObjectStore *object_store,
                                                               uint32_t object_id,
                                                               DrawingProgramRasterSample color_value);
CoreResult drawing_program_history_apply_insert_object_path_point(DrawingProgramHistory *history,
                                                                  DrawingProgramObjectStore *object_store,
                                                                  uint32_t object_id,
                                                                  uint16_t insert_index,
                                                                  int32_t point_x,
                                                                  int32_t point_y);
CoreResult drawing_program_history_apply_remove_object_path_point(DrawingProgramHistory *history,
                                                                  DrawingProgramObjectStore *object_store,
                                                                  uint32_t object_id,
                                                                  uint16_t point_index);
CoreResult drawing_program_history_apply_set_object_size(DrawingProgramHistory *history,
                                                         DrawingProgramObjectStore *object_store,
                                                         uint32_t object_id,
                                                         uint32_t width,
                                                         uint32_t height);
CoreResult drawing_program_history_undo(DrawingProgramHistory *history,
                                        DrawingProgramDocument *document,
                                        DrawingProgramLayerRasterStore *layer_rasters,
                                        DrawingProgramObjectStore *object_store);
CoreResult drawing_program_history_redo(DrawingProgramHistory *history,
                                        DrawingProgramDocument *document,
                                        DrawingProgramLayerRasterStore *layer_rasters,
                                        DrawingProgramObjectStore *object_store);

#ifdef __cplusplus
}
#endif

#endif
