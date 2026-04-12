#include "drawing_program/drawing_program_history.h"

#include <string.h>

#include "drawing_program/drawing_program_color_model.h"

static CoreResult drawing_program_history_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
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
                                             DrawingProgramLayerRasterStore *layer_rasters) {
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
    return core_result_ok();
}

static CoreResult history_apply_redo_command(const DrawingProgramCommand *command,
                                             DrawingProgramDocument *document,
                                             DrawingProgramLayerRasterStore *layer_rasters) {
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

CoreResult drawing_program_history_undo(DrawingProgramHistory *history,
                                        DrawingProgramDocument *document,
                                        DrawingProgramLayerRasterStore *layer_rasters) {
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
            result = history_apply_undo_command(command, document, layer_rasters);
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
    return history_apply_undo_command(command, document, layer_rasters);
}

CoreResult drawing_program_history_redo(DrawingProgramHistory *history,
                                        DrawingProgramDocument *document,
                                        DrawingProgramLayerRasterStore *layer_rasters) {
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
            result = history_apply_redo_command(command, document, layer_rasters);
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
    result = history_apply_redo_command(command, document, layer_rasters);
    if (result.code != CORE_OK) {
        return result;
    }
    history->cursor += 1u;
    return core_result_ok();
}
