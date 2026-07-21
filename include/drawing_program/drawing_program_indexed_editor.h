#ifndef DRAWING_PROGRAM_INDEXED_EDITOR_H
#define DRAWING_PROGRAM_INDEXED_EDITOR_H

#include <stdint.h>

#include "core_base.h"
#include "drawing_program/drawing_program_app_main.h"

#ifdef __cplusplus
extern "C" {
#endif

int drawing_program_indexed_editor_is_active(const DrawingProgramAppContext *ctx);
int drawing_program_indexed_editor_tool_allowed(DrawingProgramToolKind tool);
CoreResult drawing_program_indexed_editor_select_slot(DrawingProgramAppContext *ctx, uint8_t slot_index);
CoreResult drawing_program_indexed_editor_apply_at(DrawingProgramAppContext *ctx,
                                                   DrawingProgramToolKind tool,
                                                   uint32_t x,
                                                   uint32_t y);
CoreResult drawing_program_indexed_editor_undo(DrawingProgramAppContext *ctx);
CoreResult drawing_program_indexed_editor_redo(DrawingProgramAppContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
