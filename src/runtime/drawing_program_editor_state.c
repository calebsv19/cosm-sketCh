#include "drawing_program/drawing_program_editor_state.h"

#include <string.h>

void drawing_program_editor_state_init(DrawingProgramEditorState *editor,
                                       const DrawingProgramDocument *document) {
    if (!editor) {
        return;
    }
    memset(editor, 0, sizeof(*editor));
    editor->active_tool = DRAWING_PROGRAM_TOOL_BRUSH;
    editor->viewport.pan_x = 0.0f;
    editor->viewport.pan_y = 0.0f;
    editor->viewport.zoom = 1.0f;
    if (document && document->layer_count > 0u) {
        editor->active_layer_id = document->layers[0].layer_id;
    } else {
        editor->active_layer_id = 0u;
    }
}
