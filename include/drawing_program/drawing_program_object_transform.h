#ifndef DRAWING_PROGRAM_OBJECT_TRANSFORM_H
#define DRAWING_PROGRAM_OBJECT_TRANSFORM_H

#include <stdint.h>

#include "core_base.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_history.h"
#include "drawing_program/drawing_program_object_store.h"

#ifdef __cplusplus
extern "C" {
#endif

CoreResult drawing_program_object_transform_commit_move(DrawingProgramHistory *history,
                                                        DrawingProgramObjectStore *object_store,
                                                        const DrawingProgramDocument *document,
                                                        const DrawingProgramObjectSelectionState *selection,
                                                        int32_t requested_dx,
                                                        int32_t requested_dy,
                                                        int32_t *out_applied_dx,
                                                        int32_t *out_applied_dy);

#ifdef __cplusplus
}
#endif

#endif
