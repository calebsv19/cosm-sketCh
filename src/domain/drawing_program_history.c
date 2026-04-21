#include "drawing_program/drawing_program_history.h"

#include <string.h>

#include "drawing_program/drawing_program_color_model.h"

static CoreResult drawing_program_history_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static int history_path_point_equals(const DrawingProgramPathPoint *a, const DrawingProgramPathPoint *b) {
    if (!a || !b) {
        return 0;
    }
    return a->x == b->x &&
           a->y == b->y &&
           a->handle_in_dx == b->handle_in_dx &&
           a->handle_in_dy == b->handle_in_dy &&
           a->handle_out_dx == b->handle_out_dx &&
           a->handle_out_dy == b->handle_out_dy &&
           a->bezier_enabled == b->bezier_enabled &&
           a->handle_linked == b->handle_linked;
}

static void history_command_fill_path_point(DrawingProgramCommand *command,
                                            int is_previous,
                                            const DrawingProgramPathPoint *point) {
    if (!command || !point) {
        return;
    }
    if (is_previous) {
        command->previous_path_point_x = point->x;
        command->previous_path_point_y = point->y;
        command->previous_path_point_handle_in_dx = point->handle_in_dx;
        command->previous_path_point_handle_in_dy = point->handle_in_dy;
        command->previous_path_point_handle_out_dx = point->handle_out_dx;
        command->previous_path_point_handle_out_dy = point->handle_out_dy;
        command->previous_path_point_bezier_enabled = point->bezier_enabled ? 1u : 0u;
        command->previous_path_point_handle_linked = point->handle_linked ? 1u : 0u;
    } else {
        command->new_path_point_x = point->x;
        command->new_path_point_y = point->y;
        command->new_path_point_handle_in_dx = point->handle_in_dx;
        command->new_path_point_handle_in_dy = point->handle_in_dy;
        command->new_path_point_handle_out_dx = point->handle_out_dx;
        command->new_path_point_handle_out_dy = point->handle_out_dy;
        command->new_path_point_bezier_enabled = point->bezier_enabled ? 1u : 0u;
        command->new_path_point_handle_linked = point->handle_linked ? 1u : 0u;
    }
}

static DrawingProgramPathPoint history_command_make_path_point(const DrawingProgramCommand *command, int use_previous) {
    DrawingProgramPathPoint point;
    memset(&point, 0, sizeof(point));
    if (!command) {
        return point;
    }
    if (use_previous) {
        point.x = command->previous_path_point_x;
        point.y = command->previous_path_point_y;
        point.handle_in_dx = command->previous_path_point_handle_in_dx;
        point.handle_in_dy = command->previous_path_point_handle_in_dy;
        point.handle_out_dx = command->previous_path_point_handle_out_dx;
        point.handle_out_dy = command->previous_path_point_handle_out_dy;
        point.bezier_enabled = command->previous_path_point_bezier_enabled ? 1u : 0u;
        point.handle_linked = command->previous_path_point_handle_linked ? 1u : 0u;
    } else {
        point.x = command->new_path_point_x;
        point.y = command->new_path_point_y;
        point.handle_in_dx = command->new_path_point_handle_in_dx;
        point.handle_in_dy = command->new_path_point_handle_in_dy;
        point.handle_out_dx = command->new_path_point_handle_out_dx;
        point.handle_out_dy = command->new_path_point_handle_out_dy;
        point.bezier_enabled = command->new_path_point_bezier_enabled ? 1u : 0u;
        point.handle_linked = command->new_path_point_handle_linked ? 1u : 0u;
    }
    return point;
}

static void drawing_program_history_push(DrawingProgramHistory *history, const DrawingProgramCommand *command);

static uint32_t history_legacy_surface_layer_id(const DrawingProgramDocument *document) {
    uint32_t i;
    if (!document || document->layer_count == 0u) {
        return 0u;
    }
    for (i = 0u; i < document->layer_count; ++i) {
        if (document->layers[i].layer_id == 1u) {
            return 1u;
        }
    }
    return document->layers[0].layer_id;
}

static int history_layer_raster_store_usable(const DrawingProgramDocument *document,
                                             const DrawingProgramLayerRasterStore *layer_rasters) {
    return document &&
           layer_rasters &&
           layer_rasters->sample_count == document->raster_sample_count &&
           layer_rasters->raster_width == document->raster_width &&
           layer_rasters->raster_height == document->raster_height;
}

static CoreResult history_sample_read(const DrawingProgramDocument *document,
                                      const DrawingProgramLayerRasterStore *layer_rasters,
                                      uint32_t layer_id,
                                      uint32_t sample_x,
                                      uint32_t sample_y,
                                      uint8_t *out_value) {
    CoreResult result;
    if (!document || !out_value || layer_id == 0u) {
        return drawing_program_history_invalid("invalid history sample read request");
    }
    if (history_layer_raster_store_usable(document, layer_rasters)) {
        result = drawing_program_layer_raster_store_sample_read(layer_rasters, layer_id, sample_x, sample_y, out_value);
        if (result.code == CORE_OK) {
            return core_result_ok();
        }
    }
    return drawing_program_document_sample_read(document, sample_x, sample_y, out_value);
}

static CoreResult history_sample_read_index(const DrawingProgramDocument *document,
                                            const DrawingProgramLayerRasterStore *layer_rasters,
                                            uint32_t layer_id,
                                            uint32_t sample_index,
                                            uint8_t *out_value) {
    uint32_t x;
    uint32_t y;
    if (!document || document->raster_width == 0u) {
        return drawing_program_history_invalid("invalid sample-index read request");
    }
    if (sample_index >= document->raster_sample_count) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "sample index out of bounds" };
    }
    x = sample_index % document->raster_width;
    y = sample_index / document->raster_width;
    return history_sample_read(document, layer_rasters, layer_id, x, y, out_value);
}

static CoreResult history_compose_sample_from_layers(const DrawingProgramDocument *document,
                                                     const DrawingProgramLayerRasterStore *layer_rasters,
                                                     uint32_t sample_x,
                                                     uint32_t sample_y,
                                                     uint8_t *out_value) {
    uint32_t i;
    uint32_t legacy_layer_id;
    uint8_t composed = drawing_program_color_eraser_value();
    if (!document || !out_value) {
        return drawing_program_history_invalid("invalid history compose sample request");
    }
    legacy_layer_id = history_legacy_surface_layer_id(document);
    for (i = 0u; i < document->layer_count; ++i) {
        uint32_t layer_id = document->layers[i].layer_id;
        uint8_t sample = drawing_program_color_eraser_value();
        if (!document->layers[i].visible) {
            continue;
        }
        if (history_layer_raster_store_usable(document, layer_rasters) &&
            drawing_program_layer_raster_store_sample_read(layer_rasters, layer_id, sample_x, sample_y, &sample).code == CORE_OK) {
            if (sample != drawing_program_color_eraser_value()) {
                composed = sample;
            }
            continue;
        }
        if (layer_id == legacy_layer_id &&
            drawing_program_document_sample_read(document, sample_x, sample_y, &sample).code == CORE_OK &&
            sample != drawing_program_color_eraser_value()) {
            composed = sample;
        }
    }
    *out_value = composed;
    return core_result_ok();
}

static CoreResult history_sample_write(DrawingProgramDocument *document,
                                       DrawingProgramLayerRasterStore *layer_rasters,
                                       uint32_t layer_id,
                                       uint32_t sample_x,
                                       uint32_t sample_y,
                                       uint8_t value) {
    CoreResult result;
    if (!document || layer_id == 0u) {
        return drawing_program_history_invalid("invalid history sample write request");
    }
    if (history_layer_raster_store_usable(document, layer_rasters)) {
        uint8_t composed = drawing_program_color_eraser_value();
        result = drawing_program_layer_raster_store_sample_write(layer_rasters, layer_id, sample_x, sample_y, value, 0);
        if (result.code != CORE_OK) {
            return result;
        }
        result = history_compose_sample_from_layers(document, layer_rasters, sample_x, sample_y, &composed);
        if (result.code != CORE_OK) {
            return result;
        }
        return drawing_program_document_sample_write(document, sample_x, sample_y, composed, 0);
    }
    return drawing_program_document_sample_write(document, sample_x, sample_y, value, 0);
}

static CoreResult history_sample_write_index(DrawingProgramDocument *document,
                                             DrawingProgramLayerRasterStore *layer_rasters,
                                             uint32_t layer_id,
                                             uint32_t sample_index,
                                             uint8_t value) {
    uint32_t x;
    uint32_t y;
    if (!document || document->raster_width == 0u) {
        return drawing_program_history_invalid("invalid sample-index write request");
    }
    if (sample_index >= document->raster_sample_count) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "sample index out of bounds" };
    }
    x = sample_index % document->raster_width;
    y = sample_index / document->raster_width;
    return history_sample_write(document, layer_rasters, layer_id, x, y, value);
}

static CoreResult history_apply_undo_command(const DrawingProgramCommand *command,
                                             DrawingProgramDocument *document,
                                             DrawingProgramLayerRasterStore *layer_rasters,
                                             DrawingProgramObjectStore *object_store) {
    if (!command || !document) {
        return drawing_program_history_invalid("invalid undo command request");
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_LAYER_VISIBILITY) {
        return drawing_program_document_set_layer_visibility(document,
                                                             command->layer_id,
                                                             command->previous_visibility,
                                                             0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_SAMPLE_VALUE) {
        return history_sample_write(document,
                                    layer_rasters,
                                    command->layer_id,
                                    command->sample_x,
                                    command->sample_y,
                                    command->previous_sample_value);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_SAMPLE_SPAN_VALUE) {
        uint32_t i;
        uint32_t start_index = command->sample_x;
        uint32_t span_length = command->sample_y;
        for (i = 0u; i < span_length; ++i) {
            CoreResult result = history_sample_write_index(document,
                                                           layer_rasters,
                                                           command->layer_id,
                                                           start_index + i,
                                                           command->previous_sample_value);
            if (result.code != CORE_OK) {
                return result;
            }
        }
        return core_result_ok();
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_OBJECT_ORIGIN) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-origin undo");
        }
        return drawing_program_object_store_set_origin(object_store,
                                                       command->object_id,
                                                       command->previous_object_origin_x,
                                                       command->previous_object_origin_y,
                                                       0,
                                                       0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_OBJECT_SIZE) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-size undo");
        }
        return drawing_program_object_store_set_size(object_store,
                                                     command->object_id,
                                                     command->previous_object_width,
                                                     command->previous_object_height,
                                                     0,
                                                     0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_OBJECT_PATH_POINT) {
        DrawingProgramPathPoint point;
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-path-point undo");
        }
        point = history_command_make_path_point(command, 1);
        return drawing_program_object_store_set_path_point_data(object_store,
                                                                command->object_id,
                                                                command->path_point_index,
                                                                &point,
                                                                0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_OBJECT_STROKE_WIDTH) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-stroke-width undo");
        }
        return drawing_program_object_store_set_stroke_width(object_store,
                                                             command->object_id,
                                                             command->previous_object_stroke_width,
                                                             0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_OBJECT_STYLE_MODE) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-style-mode undo");
        }
        return drawing_program_object_store_set_style_mode(object_store,
                                                           command->object_id,
                                                           command->previous_object_style_mode,
                                                           0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_OBJECT_PATH_CLOSED) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-path-closed undo");
        }
        return drawing_program_object_store_set_path_closed(object_store,
                                                            command->object_id,
                                                            command->previous_object_path_closed,
                                                            0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_OBJECT_STROKE_COLOR) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-stroke-color undo");
        }
        return drawing_program_object_store_set_stroke_color_index(object_store,
                                                                   command->object_id,
                                                                   command->previous_object_stroke_color_index,
                                                                   0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_OBJECT_FILL_COLOR) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-fill-color undo");
        }
        return drawing_program_object_store_set_fill_color_index(object_store,
                                                                 command->object_id,
                                                                 command->previous_object_fill_color_index,
                                                                 0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_INSERT_OBJECT_PATH_POINT) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-path-point insert undo");
        }
        return drawing_program_object_store_remove_path_point(object_store,
                                                              command->object_id,
                                                              command->path_point_index,
                                                              0,
                                                              0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_REMOVE_OBJECT_PATH_POINT) {
        DrawingProgramPathPoint point;
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-path-point remove undo");
        }
        point = history_command_make_path_point(command, 1);
        return drawing_program_object_store_insert_path_point_data(object_store,
                                                                   command->object_id,
                                                                   command->path_point_index,
                                                                   &point);
    }
    return core_result_ok();
}

static CoreResult history_apply_redo_command(const DrawingProgramCommand *command,
                                             DrawingProgramDocument *document,
                                             DrawingProgramLayerRasterStore *layer_rasters,
                                             DrawingProgramObjectStore *object_store) {
    if (!command || !document) {
        return drawing_program_history_invalid("invalid redo command request");
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_LAYER_VISIBILITY) {
        return drawing_program_document_set_layer_visibility(document,
                                                             command->layer_id,
                                                             command->new_visibility,
                                                             0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_SAMPLE_VALUE) {
        return history_sample_write(document,
                                    layer_rasters,
                                    command->layer_id,
                                    command->sample_x,
                                    command->sample_y,
                                    command->new_sample_value);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_SAMPLE_SPAN_VALUE) {
        uint32_t i;
        uint32_t start_index = command->sample_x;
        uint32_t span_length = command->sample_y;
        for (i = 0u; i < span_length; ++i) {
            CoreResult result = history_sample_write_index(document,
                                                           layer_rasters,
                                                           command->layer_id,
                                                           start_index + i,
                                                           command->new_sample_value);
            if (result.code != CORE_OK) {
                return result;
            }
        }
        return core_result_ok();
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_OBJECT_ORIGIN) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-origin redo");
        }
        return drawing_program_object_store_set_origin(object_store,
                                                       command->object_id,
                                                       command->new_object_origin_x,
                                                       command->new_object_origin_y,
                                                       0,
                                                       0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_OBJECT_SIZE) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-size redo");
        }
        return drawing_program_object_store_set_size(object_store,
                                                     command->object_id,
                                                     command->new_object_width,
                                                     command->new_object_height,
                                                     0,
                                                     0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_OBJECT_PATH_POINT) {
        DrawingProgramPathPoint point;
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-path-point redo");
        }
        point = history_command_make_path_point(command, 0);
        return drawing_program_object_store_set_path_point_data(object_store,
                                                                command->object_id,
                                                                command->path_point_index,
                                                                &point,
                                                                0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_OBJECT_STROKE_WIDTH) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-stroke-width redo");
        }
        return drawing_program_object_store_set_stroke_width(object_store,
                                                             command->object_id,
                                                             command->new_object_stroke_width,
                                                             0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_OBJECT_STYLE_MODE) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-style-mode redo");
        }
        return drawing_program_object_store_set_style_mode(object_store,
                                                           command->object_id,
                                                           command->new_object_style_mode,
                                                           0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_OBJECT_PATH_CLOSED) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-path-closed redo");
        }
        return drawing_program_object_store_set_path_closed(object_store,
                                                            command->object_id,
                                                            command->new_object_path_closed,
                                                            0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_OBJECT_STROKE_COLOR) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-stroke-color redo");
        }
        return drawing_program_object_store_set_stroke_color_index(object_store,
                                                                   command->object_id,
                                                                   command->new_object_stroke_color_index,
                                                                   0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_OBJECT_FILL_COLOR) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-fill-color redo");
        }
        return drawing_program_object_store_set_fill_color_index(object_store,
                                                                 command->object_id,
                                                                 command->new_object_fill_color_index,
                                                                 0);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_INSERT_OBJECT_PATH_POINT) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-path-point insert redo");
        }
        return drawing_program_object_store_insert_path_point(object_store,
                                                              command->object_id,
                                                              command->path_point_index,
                                                              command->new_path_point_x,
                                                              command->new_path_point_y);
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_REMOVE_OBJECT_PATH_POINT) {
        if (!object_store) {
            return drawing_program_history_invalid("missing object store for object-path-point remove redo");
        }
        return drawing_program_object_store_remove_path_point(object_store,
                                                              command->object_id,
                                                              command->path_point_index,
                                                              0,
                                                              0);
    }
    return core_result_ok();
}

static uint32_t history_count_units_upto(const DrawingProgramHistory *history, uint32_t limit) {
    uint32_t i;
    uint32_t units = 0u;
    int in_group = 0;
    int group_has_commands = 0;
    if (!history) {
        return 0u;
    }
    if (limit > history->count) {
        limit = history->count;
    }
    for (i = 0u; i < limit; ++i) {
        DrawingProgramCommandType type = history->entries[i].type;
        if (type == DRAWING_PROGRAM_COMMAND_GROUP_BEGIN) {
            in_group = 1;
            group_has_commands = 0;
            continue;
        }
        if (type == DRAWING_PROGRAM_COMMAND_GROUP_END) {
            if (in_group && group_has_commands) {
                units += 1u;
            }
            in_group = 0;
            group_has_commands = 0;
            continue;
        }
        if (in_group) {
            group_has_commands = 1;
        } else {
            units += 1u;
        }
    }
    if (in_group && group_has_commands) {
        units += 1u;
    }
    return units;
}

void drawing_program_history_init(DrawingProgramHistory *history) {
    if (!history) {
        return;
    }
    memset(history, 0, sizeof(*history));
}

void drawing_program_history_clear(DrawingProgramHistory *history) {
    if (!history) {
        return;
    }
    memset(history, 0, sizeof(*history));
}

CoreResult drawing_program_history_begin_group(DrawingProgramHistory *history) {
    DrawingProgramCommand marker;
    if (!history) {
        return drawing_program_history_invalid("invalid begin group request");
    }
    if (history->cursor < history->count) {
        history->count = history->cursor;
    }
    memset(&marker, 0, sizeof(marker));
    marker.type = DRAWING_PROGRAM_COMMAND_GROUP_BEGIN;
    drawing_program_history_push(history, &marker);
    return core_result_ok();
}

CoreResult drawing_program_history_end_group(DrawingProgramHistory *history) {
    DrawingProgramCommand marker;
    if (!history) {
        return drawing_program_history_invalid("invalid end group request");
    }
    if (history->cursor < history->count) {
        history->count = history->cursor;
    }
    if (history->count > 0u &&
        history->cursor == history->count &&
        history->entries[history->count - 1u].type == DRAWING_PROGRAM_COMMAND_GROUP_BEGIN) {
        history->count -= 1u;
        history->cursor = history->count;
        return core_result_ok();
    }
    memset(&marker, 0, sizeof(marker));
    marker.type = DRAWING_PROGRAM_COMMAND_GROUP_END;
    drawing_program_history_push(history, &marker);
    return core_result_ok();
}

void drawing_program_history_query_units(const DrawingProgramHistory *history,
                                         uint32_t *out_cursor_units,
                                         uint32_t *out_count_units) {
    if (out_cursor_units) {
        *out_cursor_units = history_count_units_upto(history, history ? history->cursor : 0u);
    }
    if (out_count_units) {
        *out_count_units = history_count_units_upto(history, history ? history->count : 0u);
    }
}

static void drawing_program_history_push(DrawingProgramHistory *history, const DrawingProgramCommand *command) {
    uint32_t i;
    if (!history || !command) {
        return;
    }

    if (history->cursor < history->count) {
        history->count = history->cursor;
    }

    if (history->count >= DRAWING_PROGRAM_HISTORY_CAPACITY) {
        for (i = 1u; i < history->count; ++i) {
            history->entries[i - 1u] = history->entries[i];
        }
        history->count = DRAWING_PROGRAM_HISTORY_CAPACITY - 1u;
        if (history->cursor > 0u) {
            history->cursor -= 1u;
        }
    }

    history->entries[history->count] = *command;
    history->count += 1u;
    history->cursor = history->count;
}

CoreResult drawing_program_history_apply_set_layer_visibility(DrawingProgramHistory *history,
                                                              DrawingProgramDocument *document,
                                                              uint32_t layer_id,
                                                              uint8_t visible) {
    DrawingProgramCommand command;
    CoreResult result;
    uint8_t prev = 0u;
    if (!history || !document || layer_id == 0u) {
        return drawing_program_history_invalid("invalid apply command request");
    }

    result = drawing_program_document_set_layer_visibility(document, layer_id, visible, &prev);
    if (result.code != CORE_OK) {
        return result;
    }
    if ((prev ? 1u : 0u) == (visible ? 1u : 0u)) {
        return core_result_ok();
    }
    memset(&command, 0, sizeof(command));
    command.type = DRAWING_PROGRAM_COMMAND_SET_LAYER_VISIBILITY;
    command.layer_id = layer_id;
    command.new_visibility = visible ? 1u : 0u;
    command.previous_visibility = prev;
    drawing_program_history_push(history, &command);
    return core_result_ok();
}

CoreResult drawing_program_history_apply_set_sample_value(DrawingProgramHistory *history,
                                                          DrawingProgramDocument *document,
                                                          DrawingProgramLayerRasterStore *layer_rasters,
                                                          uint32_t layer_id,
                                                          uint32_t sample_x,
                                                          uint32_t sample_y,
                                                          uint8_t value) {
    DrawingProgramCommand command;
    CoreResult result;
    uint8_t prev = 0u;
    if (!history || !document || layer_id == 0u) {
        return drawing_program_history_invalid("invalid sample command request");
    }

    result = history_sample_read(document, layer_rasters, layer_id, sample_x, sample_y, &prev);
    if (result.code != CORE_OK) {
        return result;
    }
    if (prev == value) {
        return core_result_ok();
    }
    result = history_sample_write(document, layer_rasters, layer_id, sample_x, sample_y, value);
    if (result.code != CORE_OK) {
        return result;
    }
    memset(&command, 0, sizeof(command));
    command.type = DRAWING_PROGRAM_COMMAND_SET_SAMPLE_VALUE;
    command.layer_id = layer_id;
    command.sample_x = sample_x;
    command.sample_y = sample_y;
    command.new_sample_value = value;
    command.previous_sample_value = prev;
    drawing_program_history_push(history, &command);
    return core_result_ok();
}

CoreResult drawing_program_history_apply_set_sample_span_value(DrawingProgramHistory *history,
                                                               DrawingProgramDocument *document,
                                                               DrawingProgramLayerRasterStore *layer_rasters,
                                                               uint32_t layer_id,
                                                               uint32_t span_start_index,
                                                               uint32_t span_length,
                                                               uint8_t value) {
    DrawingProgramCommand command;
    CoreResult result;
    uint8_t prev = 0u;
    uint32_t i;
    if (!history || !document || layer_id == 0u) {
        return drawing_program_history_invalid("invalid sample-span command request");
    }
    if (span_length == 0u) {
        return core_result_ok();
    }
    if (span_start_index >= document->raster_sample_count ||
        span_length > document->raster_sample_count ||
        span_start_index > (document->raster_sample_count - span_length)) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "sample-span bounds exceed raster range" };
    }
    result = history_sample_read_index(document, layer_rasters, layer_id, span_start_index, &prev);
    if (result.code != CORE_OK) {
        return result;
    }
    if (prev == value) {
        return core_result_ok();
    }
    for (i = 1u; i < span_length; ++i) {
        uint8_t probe = 0u;
        result = history_sample_read_index(document, layer_rasters, layer_id, span_start_index + i, &probe);
        if (result.code != CORE_OK) {
            return result;
        }
        if (probe != prev) {
            return (CoreResult){ CORE_ERR_INVALID_ARG, "sample-span previous values are not uniform" };
        }
    }
    for (i = 0u; i < span_length; ++i) {
        result = history_sample_write_index(document, layer_rasters, layer_id, span_start_index + i, value);
        if (result.code != CORE_OK) {
            return result;
        }
    }
    memset(&command, 0, sizeof(command));
    command.type = DRAWING_PROGRAM_COMMAND_SET_SAMPLE_SPAN_VALUE;
    command.layer_id = layer_id;
    command.sample_x = span_start_index;
    command.sample_y = span_length;
    command.new_sample_value = value;
    command.previous_sample_value = prev;
    drawing_program_history_push(history, &command);
    return core_result_ok();
}

CoreResult drawing_program_history_apply_set_object_origin(DrawingProgramHistory *history,
                                                           DrawingProgramObjectStore *object_store,
                                                           uint32_t object_id,
                                                           int32_t origin_x,
                                                           int32_t origin_y) {
    DrawingProgramCommand command;
    CoreResult result;
    int32_t previous_x = 0;
    int32_t previous_y = 0;
    if (!history || !object_store || object_id == 0u) {
        return drawing_program_history_invalid("invalid object-origin command request");
    }
    result = drawing_program_object_store_set_origin(
        object_store, object_id, origin_x, origin_y, &previous_x, &previous_y);
    if (result.code != CORE_OK) {
        return result;
    }
    if (previous_x == origin_x && previous_y == origin_y) {
        return core_result_ok();
    }
    memset(&command, 0, sizeof(command));
    command.type = DRAWING_PROGRAM_COMMAND_SET_OBJECT_ORIGIN;
    command.object_id = object_id;
    command.new_object_origin_x = origin_x;
    command.new_object_origin_y = origin_y;
    command.previous_object_origin_x = previous_x;
    command.previous_object_origin_y = previous_y;
    drawing_program_history_push(history, &command);
    return core_result_ok();
}

CoreResult drawing_program_history_apply_set_object_path_point(DrawingProgramHistory *history,
                                                               DrawingProgramObjectStore *object_store,
                                                               uint32_t object_id,
                                                               uint16_t point_index,
                                                               int32_t point_x,
                                                               int32_t point_y) {
    DrawingProgramPathPayload payload;
    DrawingProgramPathPoint point;
    if (!history || !object_store || object_id == 0u) {
        return drawing_program_history_invalid("invalid object-path-point command request");
    }
    if (drawing_program_object_store_get_path_payload(object_store, object_id, &payload).code != CORE_OK ||
        (uint32_t)point_index >= payload.point_count) {
        return drawing_program_history_invalid("object-path-point command target missing");
    }
    point = payload.points[point_index];
    point.x = point_x;
    point.y = point_y;
    return drawing_program_history_apply_set_object_path_point_data(
        history, object_store, object_id, point_index, &point);
}

CoreResult drawing_program_history_apply_set_object_path_point_data(DrawingProgramHistory *history,
                                                                    DrawingProgramObjectStore *object_store,
                                                                    uint32_t object_id,
                                                                    uint16_t point_index,
                                                                    const DrawingProgramPathPoint *point) {
    DrawingProgramCommand command;
    DrawingProgramPathPoint previous_point;
    CoreResult result;
    if (!history || !object_store || object_id == 0u || !point) {
        return drawing_program_history_invalid("invalid object-path-point data command request");
    }
    memset(&previous_point, 0, sizeof(previous_point));
    result = drawing_program_object_store_set_path_point_data(object_store,
                                                              object_id,
                                                              point_index,
                                                              point,
                                                              &previous_point);
    if (result.code != CORE_OK) {
        return result;
    }
    if (history_path_point_equals(&previous_point, point)) {
        return core_result_ok();
    }
    memset(&command, 0, sizeof(command));
    command.type = DRAWING_PROGRAM_COMMAND_SET_OBJECT_PATH_POINT;
    command.object_id = object_id;
    command.path_point_index = point_index;
    history_command_fill_path_point(&command, 0, point);
    history_command_fill_path_point(&command, 1, &previous_point);
    drawing_program_history_push(history, &command);
    return core_result_ok();
}

CoreResult drawing_program_history_apply_set_object_size(DrawingProgramHistory *history,
                                                         DrawingProgramObjectStore *object_store,
                                                         uint32_t object_id,
                                                         uint32_t width,
                                                         uint32_t height) {
    DrawingProgramCommand command;
    CoreResult result;
    uint32_t previous_width = 0u;
    uint32_t previous_height = 0u;
    if (!history || !object_store || object_id == 0u) {
        return drawing_program_history_invalid("invalid object-size command request");
    }
    result = drawing_program_object_store_set_size(
        object_store, object_id, width, height, &previous_width, &previous_height);
    if (result.code != CORE_OK) {
        return result;
    }
    if (previous_width == width && previous_height == height) {
        return core_result_ok();
    }
    memset(&command, 0, sizeof(command));
    command.type = DRAWING_PROGRAM_COMMAND_SET_OBJECT_SIZE;
    command.object_id = object_id;
    command.new_object_width = width;
    command.new_object_height = height;
    command.previous_object_width = previous_width;
    command.previous_object_height = previous_height;
    drawing_program_history_push(history, &command);
    return core_result_ok();
}

CoreResult drawing_program_history_apply_set_object_stroke_width(DrawingProgramHistory *history,
                                                                 DrawingProgramObjectStore *object_store,
                                                                 uint32_t object_id,
                                                                 uint8_t stroke_width) {
    DrawingProgramCommand command;
    CoreResult result;
    uint8_t previous_stroke_width = 0u;
    if (!history || !object_store || object_id == 0u) {
        return drawing_program_history_invalid("invalid object-stroke-width command request");
    }
    result = drawing_program_object_store_set_stroke_width(
        object_store, object_id, stroke_width, &previous_stroke_width);
    if (result.code != CORE_OK) {
        return result;
    }
    if (previous_stroke_width == stroke_width) {
        return core_result_ok();
    }
    memset(&command, 0, sizeof(command));
    command.type = DRAWING_PROGRAM_COMMAND_SET_OBJECT_STROKE_WIDTH;
    command.object_id = object_id;
    command.new_object_stroke_width = stroke_width;
    command.previous_object_stroke_width = previous_stroke_width;
    drawing_program_history_push(history, &command);
    return core_result_ok();
}

CoreResult drawing_program_history_apply_set_object_style_mode(DrawingProgramHistory *history,
                                                               DrawingProgramObjectStore *object_store,
                                                               uint32_t object_id,
                                                               uint8_t style_mode) {
    DrawingProgramCommand command;
    CoreResult result;
    uint8_t previous_style_mode = 0u;
    if (!history || !object_store || object_id == 0u) {
        return drawing_program_history_invalid("invalid object-style-mode command request");
    }
    result = drawing_program_object_store_set_style_mode(
        object_store, object_id, style_mode, &previous_style_mode);
    if (result.code != CORE_OK) {
        return result;
    }
    if (previous_style_mode == style_mode) {
        return core_result_ok();
    }
    memset(&command, 0, sizeof(command));
    command.type = DRAWING_PROGRAM_COMMAND_SET_OBJECT_STYLE_MODE;
    command.object_id = object_id;
    command.new_object_style_mode = style_mode;
    command.previous_object_style_mode = previous_style_mode;
    drawing_program_history_push(history, &command);
    return core_result_ok();
}

CoreResult drawing_program_history_apply_set_object_path_closed(DrawingProgramHistory *history,
                                                                DrawingProgramObjectStore *object_store,
                                                                uint32_t object_id,
                                                                uint8_t closed) {
    DrawingProgramCommand command;
    CoreResult result;
    uint8_t previous_closed = 0u;
    uint8_t closed_normalized = closed ? 1u : 0u;
    if (!history || !object_store || object_id == 0u) {
        return drawing_program_history_invalid("invalid object-path-closed command request");
    }
    result = drawing_program_object_store_set_path_closed(
        object_store, object_id, closed_normalized, &previous_closed);
    if (result.code != CORE_OK) {
        return result;
    }
    if (previous_closed == closed_normalized) {
        return core_result_ok();
    }
    memset(&command, 0, sizeof(command));
    command.type = DRAWING_PROGRAM_COMMAND_SET_OBJECT_PATH_CLOSED;
    command.object_id = object_id;
    command.new_object_path_closed = closed_normalized;
    command.previous_object_path_closed = previous_closed;
    drawing_program_history_push(history, &command);
    return core_result_ok();
}

CoreResult drawing_program_history_apply_set_object_stroke_color(DrawingProgramHistory *history,
                                                                 DrawingProgramObjectStore *object_store,
                                                                 uint32_t object_id,
                                                                 uint8_t color_index) {
    DrawingProgramCommand command;
    CoreResult result;
    uint8_t previous_color_index = 0u;
    if (!history || !object_store || object_id == 0u) {
        return drawing_program_history_invalid("invalid object-stroke-color command request");
    }
    result = drawing_program_object_store_set_stroke_color_index(
        object_store, object_id, color_index, &previous_color_index);
    if (result.code != CORE_OK) {
        return result;
    }
    if (previous_color_index == color_index) {
        return core_result_ok();
    }
    memset(&command, 0, sizeof(command));
    command.type = DRAWING_PROGRAM_COMMAND_SET_OBJECT_STROKE_COLOR;
    command.object_id = object_id;
    command.new_object_stroke_color_index = color_index;
    command.previous_object_stroke_color_index = previous_color_index;
    drawing_program_history_push(history, &command);
    return core_result_ok();
}

CoreResult drawing_program_history_apply_set_object_fill_color(DrawingProgramHistory *history,
                                                               DrawingProgramObjectStore *object_store,
                                                               uint32_t object_id,
                                                               uint8_t color_index) {
    DrawingProgramCommand command;
    CoreResult result;
    uint8_t previous_color_index = 0u;
    if (!history || !object_store || object_id == 0u) {
        return drawing_program_history_invalid("invalid object-fill-color command request");
    }
    result = drawing_program_object_store_set_fill_color_index(
        object_store, object_id, color_index, &previous_color_index);
    if (result.code != CORE_OK) {
        return result;
    }
    if (previous_color_index == color_index) {
        return core_result_ok();
    }
    memset(&command, 0, sizeof(command));
    command.type = DRAWING_PROGRAM_COMMAND_SET_OBJECT_FILL_COLOR;
    command.object_id = object_id;
    command.new_object_fill_color_index = color_index;
    command.previous_object_fill_color_index = previous_color_index;
    drawing_program_history_push(history, &command);
    return core_result_ok();
}

CoreResult drawing_program_history_apply_insert_object_path_point(DrawingProgramHistory *history,
                                                                  DrawingProgramObjectStore *object_store,
                                                                  uint32_t object_id,
                                                                  uint16_t insert_index,
                                                                  int32_t point_x,
                                                                  int32_t point_y) {
    DrawingProgramCommand command;
    CoreResult result;
    if (!history || !object_store || object_id == 0u) {
        return drawing_program_history_invalid("invalid object-path-point insert command request");
    }
    result = drawing_program_object_store_insert_path_point(
        object_store, object_id, insert_index, point_x, point_y);
    if (result.code != CORE_OK) {
        return result;
    }
    memset(&command, 0, sizeof(command));
    command.type = DRAWING_PROGRAM_COMMAND_INSERT_OBJECT_PATH_POINT;
    command.object_id = object_id;
    command.path_point_index = insert_index;
    command.new_path_point_x = point_x;
    command.new_path_point_y = point_y;
    drawing_program_history_push(history, &command);
    return core_result_ok();
}

CoreResult drawing_program_history_apply_remove_object_path_point(DrawingProgramHistory *history,
                                                                  DrawingProgramObjectStore *object_store,
                                                                  uint32_t object_id,
                                                                  uint16_t point_index) {
    DrawingProgramCommand command;
    DrawingProgramPathPayload payload;
    CoreResult result;
    DrawingProgramPathPoint removed_point;
    memset(&removed_point, 0, sizeof(removed_point));
    if (!history || !object_store || object_id == 0u) {
        return drawing_program_history_invalid("invalid object-path-point remove command request");
    }
    result = drawing_program_object_store_get_path_payload(object_store, object_id, &payload);
    if (result.code != CORE_OK) {
        return result;
    }
    if ((uint32_t)point_index >= payload.point_count) {
        return drawing_program_history_invalid("object-path-point remove index out of range");
    }
    removed_point = payload.points[point_index];
    result = drawing_program_object_store_remove_path_point(
        object_store, object_id, point_index, 0, 0);
    if (result.code != CORE_OK) {
        return result;
    }
    memset(&command, 0, sizeof(command));
    command.type = DRAWING_PROGRAM_COMMAND_REMOVE_OBJECT_PATH_POINT;
    command.object_id = object_id;
    command.path_point_index = point_index;
    history_command_fill_path_point(&command, 1, &removed_point);
    drawing_program_history_push(history, &command);
    return core_result_ok();
}

CoreResult drawing_program_history_undo(DrawingProgramHistory *history,
                                        DrawingProgramDocument *document,
                                        DrawingProgramLayerRasterStore *layer_rasters,
                                        DrawingProgramObjectStore *object_store) {
    const DrawingProgramCommand *command;
    CoreResult result;
    if (!history || !document) {
        return drawing_program_history_invalid("invalid undo request");
    }
    if (history->cursor == 0u) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "nothing to undo" };
    }
    command = &history->entries[history->cursor - 1u];
    if (command->type == DRAWING_PROGRAM_COMMAND_GROUP_END) {
        history->cursor -= 1u;
        while (history->cursor > 0u) {
            command = &history->entries[history->cursor - 1u];
            history->cursor -= 1u;
            if (command->type == DRAWING_PROGRAM_COMMAND_GROUP_BEGIN) {
                return core_result_ok();
            }
            result = history_apply_undo_command(command, document, layer_rasters, object_store);
            if (result.code != CORE_OK) {
                return result;
            }
        }
        return core_result_ok();
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_GROUP_BEGIN) {
        history->cursor -= 1u;
        return core_result_ok();
    }
    history->cursor -= 1u;
    return history_apply_undo_command(command, document, layer_rasters, object_store);
}

CoreResult drawing_program_history_redo(DrawingProgramHistory *history,
                                        DrawingProgramDocument *document,
                                        DrawingProgramLayerRasterStore *layer_rasters,
                                        DrawingProgramObjectStore *object_store) {
    const DrawingProgramCommand *command;
    CoreResult result;
    if (!history || !document) {
        return drawing_program_history_invalid("invalid redo request");
    }
    if (history->cursor >= history->count) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "nothing to redo" };
    }
    command = &history->entries[history->cursor];
    if (command->type == DRAWING_PROGRAM_COMMAND_GROUP_BEGIN) {
        history->cursor += 1u;
        while (history->cursor < history->count) {
            command = &history->entries[history->cursor];
            if (command->type == DRAWING_PROGRAM_COMMAND_GROUP_END) {
                history->cursor += 1u;
                return core_result_ok();
            }
            result = history_apply_redo_command(command, document, layer_rasters, object_store);
            if (result.code != CORE_OK) {
                return result;
            }
            history->cursor += 1u;
        }
        return core_result_ok();
    }
    if (command->type == DRAWING_PROGRAM_COMMAND_GROUP_END) {
        history->cursor += 1u;
        return core_result_ok();
    }
    result = history_apply_redo_command(command, document, layer_rasters, object_store);
    if (result.code != CORE_OK) {
        return result;
    }
    history->cursor += 1u;
    return core_result_ok();
}
