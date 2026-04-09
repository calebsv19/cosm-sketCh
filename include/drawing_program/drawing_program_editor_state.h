#ifndef DRAWING_PROGRAM_EDITOR_STATE_H
#define DRAWING_PROGRAM_EDITOR_STATE_H

#include <stdint.h>

#include "drawing_program/drawing_program_document.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DrawingProgramToolKind {
    DRAWING_PROGRAM_TOOL_BRUSH = 0,
    DRAWING_PROGRAM_TOOL_ERASER = 1,
    DRAWING_PROGRAM_TOOL_FILL = 2,
    DRAWING_PROGRAM_TOOL_LINE = 3,
    DRAWING_PROGRAM_TOOL_RECT = 4,
    DRAWING_PROGRAM_TOOL_CIRCLE = 5,
    DRAWING_PROGRAM_TOOL_SELECT = 6,
    DRAWING_PROGRAM_TOOL_MOVE = 7,
    DRAWING_PROGRAM_TOOL_PICKER = 8
} DrawingProgramToolKind;

typedef struct DrawingProgramViewportState {
    float pan_x;
    float pan_y;
    float zoom;
} DrawingProgramViewportState;

typedef struct DrawingProgramEditorState {
    DrawingProgramToolKind active_tool;
    uint32_t active_layer_id;
    uint8_t symmetry_vertical;
    uint8_t symmetry_horizontal;
    DrawingProgramViewportState viewport;
} DrawingProgramEditorState;

void drawing_program_editor_state_init(DrawingProgramEditorState *editor,
                                       const DrawingProgramDocument *document);

#ifdef __cplusplus
}
#endif

#endif
