#include "drawing_program/drawing_program_history.h"
#include "drawing_program_history_internal.h"

#include <string.h>

#include "drawing_program/drawing_program_color_model.h"

static CoreResult history_raster_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static int history_layer_raster_store_usable_local(const DrawingProgramDocument *document,
                                                   const DrawingProgramLayerRasterStore *layer_rasters) {
    return document &&
           layer_rasters &&
           layer_rasters->sample_count == document->raster_sample_count &&
           layer_rasters->raster_width == document->raster_width &&
           layer_rasters->raster_height == document->raster_height;
}

static CoreResult history_compose_sample_from_layers_local(const DrawingProgramDocument *document,
                                                           const DrawingProgramLayerRasterStore *layer_rasters,
                                                           uint32_t sample_x,
                                                           uint32_t sample_y,
                                                           DrawingProgramRasterSample *out_value) {
    uint32_t i;
    DrawingProgramRasterSample composed = drawing_program_color_eraser_value();
    if (!document || !out_value) {
        return history_raster_invalid("invalid raster-delta compose request");
    }
    for (i = 0u; i < document->layer_count; ++i) {
        DrawingProgramRasterSample sample = drawing_program_color_eraser_value();
        if (!document->layers[i].visible) {
            continue;
        }
        if (history_layer_raster_store_usable_local(document, layer_rasters) &&
            drawing_program_layer_raster_store_raster_sample_read(layer_rasters,
                                                                  document->layers[i].layer_id,
                                                                  sample_x,
                                                                  sample_y,
                                                                  &sample).code == CORE_OK) {
            if (!drawing_program_color_sample_is_transparent(sample)) {
                composed = sample;
            }
        }
    }
    *out_value = composed;
    return core_result_ok();
}

static CoreResult history_sample_write_local(DrawingProgramDocument *document,
                                             DrawingProgramLayerRasterStore *layer_rasters,
                                             uint32_t layer_id,
                                             uint32_t sample_x,
                                             uint32_t sample_y,
                                             DrawingProgramRasterSample value) {
    CoreResult result;
    if (!document || layer_id == 0u) {
        return history_raster_invalid("invalid raster-delta sample write request");
    }
    if (history_layer_raster_store_usable_local(document, layer_rasters)) {
        DrawingProgramRasterSample composed = drawing_program_color_eraser_value();
        result = drawing_program_layer_raster_store_sample_write(
            layer_rasters, layer_id, sample_x, sample_y, value, 0);
        if (result.code != CORE_OK) {
            return result;
        }
        result = history_compose_sample_from_layers_local(document, layer_rasters, sample_x, sample_y, &composed);
        if (result.code != CORE_OK) {
            return result;
        }
        return drawing_program_document_sample_write(document, sample_x, sample_y, composed, 0);
    }
    return drawing_program_document_sample_write(document, sample_x, sample_y, value, 0);
}

static CoreResult history_sample_write_index_local(DrawingProgramDocument *document,
                                                   DrawingProgramLayerRasterStore *layer_rasters,
                                                   uint32_t layer_id,
                                                   uint32_t sample_index,
                                                   DrawingProgramRasterSample value) {
    uint32_t x;
    uint32_t y;
    if (!document || document->raster_width == 0u) {
        return history_raster_invalid("invalid raster-delta sample-index write request");
    }
    if (sample_index >= document->raster_sample_count) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "sample index out of bounds" };
    }
    x = sample_index % document->raster_width;
    y = sample_index / document->raster_width;
    return history_sample_write_local(document, layer_rasters, layer_id, x, y, value);
}

int drawing_program_history_command_uses_raster_delta(const DrawingProgramCommand *command) {
    return command && command->type == DRAWING_PROGRAM_COMMAND_SET_RASTER_DELTA_BLOCK;
}

uint32_t drawing_program_history_raster_delta_count_for_limit(const DrawingProgramHistory *history, uint32_t limit) {
    uint32_t i;
    uint32_t delta_count = 0u;
    if (!history) {
        return 0u;
    }
    if (limit > history->count) {
        limit = history->count;
    }
    for (i = 0u; i < limit; ++i) {
        const DrawingProgramCommand *command = &history->entries[i];
        if (drawing_program_history_command_uses_raster_delta(command)) {
            uint32_t delta_end = command->sample_x + command->sample_y;
            if (delta_end > delta_count) {
                delta_count = delta_end;
            }
        }
    }
    return delta_count;
}

void drawing_program_history_raster_delta_trim_to_count(DrawingProgramHistory *history, uint32_t kept_count) {
    if (!history) {
        return;
    }
    if (kept_count > history->count) {
        kept_count = history->count;
    }
    history->count = kept_count;
    if (history->cursor > history->count) {
        history->cursor = history->count;
    }
    history->raster_delta_count = drawing_program_history_raster_delta_count_for_limit(history, history->count);
}

void drawing_program_history_raster_delta_drop_prefix(DrawingProgramHistory *history, uint32_t drop_count) {
    uint32_t drop_delta_count;
    uint32_t i;
    if (!history || drop_count == 0u) {
        return;
    }
    drop_delta_count = drawing_program_history_raster_delta_count_for_limit(history, drop_count);
    if (drop_delta_count > history->raster_delta_count) {
        drop_delta_count = history->raster_delta_count;
    }
    if (drop_delta_count > 0u && drop_delta_count < history->raster_delta_count) {
        memmove(history->raster_delta_entries,
                history->raster_delta_entries + drop_delta_count,
                (history->raster_delta_count - drop_delta_count) * sizeof(history->raster_delta_entries[0]));
    }
    if (drop_delta_count > 0u) {
        for (i = 0u; i < history->count; ++i) {
            DrawingProgramCommand *command = &history->entries[i];
            if (drawing_program_history_command_uses_raster_delta(command)) {
                if (command->sample_x >= drop_delta_count) {
                    command->sample_x -= drop_delta_count;
                } else {
                    command->sample_x = 0u;
                    command->sample_y = 0u;
                }
            }
        }
    }
    if (history->raster_delta_count >= drop_delta_count) {
        history->raster_delta_count -= drop_delta_count;
    } else {
        history->raster_delta_count = 0u;
    }
}

static uint32_t history_oldest_unit_end_local(const DrawingProgramHistory *history, uint32_t start_index) {
    uint32_t index = start_index;
    if (!history || index >= history->count) {
        return start_index;
    }
    if (history->entries[index].type == DRAWING_PROGRAM_COMMAND_GROUP_BEGIN) {
        index += 1u;
        while (index < history->count) {
            if (history->entries[index].type == DRAWING_PROGRAM_COMMAND_GROUP_END) {
                return index + 1u;
            }
            index += 1u;
        }
        return history->count;
    }
    return index + 1u;
}

static void history_drop_oldest_unit_until_space(DrawingProgramHistory *history, uint32_t required_delta_count) {
    while (history &&
           (history->count >= DRAWING_PROGRAM_HISTORY_CAPACITY ||
            history->raster_delta_count + required_delta_count > DRAWING_PROGRAM_HISTORY_RASTER_DELTA_CAPACITY)) {
        uint32_t drop_count = history_oldest_unit_end_local(history, 0u);
        uint32_t keep_count;
        if (drop_count == 0u || drop_count > history->count) {
            drop_count = 1u;
        }
        keep_count = history->count - drop_count;
        memmove(history->entries, history->entries + drop_count, keep_count * sizeof(history->entries[0]));
        history->count = keep_count;
        if (history->cursor <= drop_count) {
            history->cursor = 0u;
        } else {
            history->cursor -= drop_count;
        }
        drawing_program_history_raster_delta_drop_prefix(history, drop_count);
    }
}

CoreResult drawing_program_history_apply_raster_delta_block(
    DrawingProgramHistory *history,
    DrawingProgramDocument *document,
    DrawingProgramLayerRasterStore *layer_rasters,
    uint32_t layer_id,
    const DrawingProgramHistoryRasterDeltaEntry *entries,
    uint32_t entry_count) {
    DrawingProgramCommand command;
    uint32_t start_index;
    uint32_t i;
    CoreResult result;
    if (!history || !document || layer_id == 0u || (!entries && entry_count > 0u)) {
        return history_raster_invalid("invalid raster-delta block apply request");
    }
    if (entry_count == 0u) {
        return core_result_ok();
    }
    if (entry_count > DRAWING_PROGRAM_HISTORY_RASTER_DELTA_CAPACITY) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "raster-delta block exceeds history storage capacity" };
    }
    if (history->cursor < history->count) {
        drawing_program_history_raster_delta_trim_to_count(history, history->cursor);
    }
    history_drop_oldest_unit_until_space(history, entry_count);
    if (history->count >= DRAWING_PROGRAM_HISTORY_CAPACITY ||
        history->raster_delta_count + entry_count > DRAWING_PROGRAM_HISTORY_RASTER_DELTA_CAPACITY) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to reclaim history raster-delta headroom" };
    }
    start_index = history->raster_delta_count;
    for (i = 0u; i < entry_count; ++i) {
        result = history_sample_write_index_local(document,
                                                  layer_rasters,
                                                  layer_id,
                                                  entries[i].sample_index,
                                                  entries[i].new_sample_value);
        if (result.code != CORE_OK) {
            return result;
        }
        history->raster_delta_entries[start_index + i] = entries[i];
    }
    history->raster_delta_count += entry_count;
    memset(&command, 0, sizeof(command));
    command.type = DRAWING_PROGRAM_COMMAND_SET_RASTER_DELTA_BLOCK;
    command.layer_id = layer_id;
    command.sample_x = start_index;
    command.sample_y = entry_count;
    drawing_program_history_push(history, &command);
    return core_result_ok();
}

CoreResult drawing_program_history_apply_raster_delta_command(const DrawingProgramHistory *history,
                                                              const DrawingProgramCommand *command,
                                                              DrawingProgramDocument *document,
                                                              DrawingProgramLayerRasterStore *layer_rasters,
                                                              int use_previous_values) {
    uint32_t delta_start;
    uint32_t delta_count;
    uint32_t i;
    if (!history || !command || !document) {
        return history_raster_invalid("invalid raster-delta command replay request");
    }
    if (!drawing_program_history_command_uses_raster_delta(command)) {
        return core_result_ok();
    }
    delta_start = command->sample_x;
    delta_count = command->sample_y;
    if (delta_start > history->raster_delta_count ||
        delta_count > history->raster_delta_count ||
        delta_start > (history->raster_delta_count - delta_count)) {
        return (CoreResult){ CORE_ERR_FORMAT, "raster-delta command range exceeds history storage" };
    }
    for (i = 0u; i < delta_count; ++i) {
        const DrawingProgramHistoryRasterDeltaEntry *entry = &history->raster_delta_entries[delta_start + i];
        CoreResult result = history_sample_write_index_local(document,
                                                             layer_rasters,
                                                             command->layer_id,
                                                             entry->sample_index,
                                                             use_previous_values ? entry->previous_sample_value
                                                                                 : entry->new_sample_value);
        if (result.code != CORE_OK) {
            return result;
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_history_validate_raster_delta_storage(const DrawingProgramHistory *history) {
    uint32_t i;
    uint32_t last_end = 0u;
    if (!history) {
        return history_raster_invalid("invalid raster-delta validation request");
    }
    if (history->raster_delta_count > DRAWING_PROGRAM_HISTORY_RASTER_DELTA_CAPACITY) {
        return (CoreResult){ CORE_ERR_FORMAT, "raster-delta history count exceeds capacity" };
    }
    for (i = 0u; i < history->count; ++i) {
        const DrawingProgramCommand *command = &history->entries[i];
        if (!drawing_program_history_command_uses_raster_delta(command)) {
            continue;
        }
        if (command->sample_x < last_end) {
            return (CoreResult){ CORE_ERR_FORMAT, "raster-delta command order is not monotonic" };
        }
        if (command->sample_x > history->raster_delta_count ||
            command->sample_y > history->raster_delta_count ||
            command->sample_x > (history->raster_delta_count - command->sample_y)) {
            return (CoreResult){ CORE_ERR_FORMAT, "raster-delta command range exceeds storage bounds" };
        }
        last_end = command->sample_x + command->sample_y;
    }
    return core_result_ok();
}
