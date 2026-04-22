#include "drawing_program/drawing_program_history.h"
#include "drawing_program_history_internal.h"

#include <string.h>

static CoreResult history_invalid(const char *message) {
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
        return history_invalid("invalid object-origin command request");
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
        return history_invalid("invalid object-path-point command request");
    }
    if (drawing_program_object_store_get_path_payload(object_store, object_id, &payload).code != CORE_OK ||
        (uint32_t)point_index >= payload.point_count) {
        return history_invalid("object-path-point command target missing");
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
        return history_invalid("invalid object-path-point data command request");
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
        return history_invalid("invalid object-size command request");
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
        return history_invalid("invalid object-stroke-width command request");
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
        return history_invalid("invalid object-style-mode command request");
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
        return history_invalid("invalid object-path-closed command request");
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
        return history_invalid("invalid object-stroke-color command request");
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
        return history_invalid("invalid object-fill-color command request");
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
        return history_invalid("invalid object-path-point insert command request");
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
        return history_invalid("invalid object-path-point remove command request");
    }
    result = drawing_program_object_store_get_path_payload(object_store, object_id, &payload);
    if (result.code != CORE_OK) {
        return result;
    }
    if ((uint32_t)point_index >= payload.point_count) {
        return history_invalid("object-path-point remove index out of range");
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
