#ifndef DRAWING_PROGRAM_OBJECT_STORE_INTERNAL_H
#define DRAWING_PROGRAM_OBJECT_STORE_INTERNAL_H

#include "drawing_program/drawing_program_object_store.h"

CoreResult drawing_program_object_store_invalid(const char *message);
int drawing_program_object_store_selection_contains_object_id(
    const DrawingProgramObjectSelectionState *selection,
    uint32_t object_id);

#endif
