#include "drawing_program/drawing_program_history.h"

#include <string.h>

static CoreResult drawing_program_history_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

void drawing_program_history_init(DrawingProgramHistory *history) {
    if (!history) {
        return;
    }
    memset(history, 0, sizeof(*history));
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
    command.type = DRAWING_PROGRAM_COMMAND_SET_LAYER_VISIBILITY;
    command.layer_id = layer_id;
    command.new_visibility = visible ? 1u : 0u;
    command.previous_visibility = prev;
    drawing_program_history_push(history, &command);
    return core_result_ok();
}

CoreResult drawing_program_history_undo(DrawingProgramHistory *history, DrawingProgramDocument *document) {
    const DrawingProgramCommand *command;
    if (!history || !document) {
        return drawing_program_history_invalid("invalid undo request");
    }
    if (history->cursor == 0u) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "nothing to undo" };
    }
    history->cursor -= 1u;
    command = &history->entries[history->cursor];
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_LAYER_VISIBILITY) {
        return drawing_program_document_set_layer_visibility(document,
                                                             command->layer_id,
                                                             command->previous_visibility,
                                                             0);
    }
    return drawing_program_history_invalid("unsupported undo command");
}

CoreResult drawing_program_history_redo(DrawingProgramHistory *history, DrawingProgramDocument *document) {
    const DrawingProgramCommand *command;
    if (!history || !document) {
        return drawing_program_history_invalid("invalid redo request");
    }
    if (history->cursor >= history->count) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "nothing to redo" };
    }
    command = &history->entries[history->cursor];
    if (command->type == DRAWING_PROGRAM_COMMAND_SET_LAYER_VISIBILITY) {
        CoreResult result = drawing_program_document_set_layer_visibility(document,
                                                                          command->layer_id,
                                                                          command->new_visibility,
                                                                          0);
        if (result.code != CORE_OK) {
            return result;
        }
        history->cursor += 1u;
        return core_result_ok();
    }
    return drawing_program_history_invalid("unsupported redo command");
}
