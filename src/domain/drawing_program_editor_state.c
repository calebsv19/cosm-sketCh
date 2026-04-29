#include "drawing_program/drawing_program_editor_state.h"
#include "drawing_program/drawing_program_viewport.h"

#include <string.h>

void drawing_program_editor_state_init(DrawingProgramEditorState *editor,
                                       const DrawingProgramDocument *document) {
    if (!editor) {
        return;
    }
    memset(editor, 0, sizeof(*editor));
    editor->active_tool = DRAWING_PROGRAM_TOOL_BRUSH;
    drawing_program_viewport_state_init(&editor->viewport);
    if (document && document->layer_count > 0u) {
        editor->active_layer_id = document->layers[0].layer_id;
    } else {
        editor->active_layer_id = 0u;
    }
}
