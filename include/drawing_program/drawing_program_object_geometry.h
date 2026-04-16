#ifndef DRAWING_PROGRAM_OBJECT_GEOMETRY_H
#define DRAWING_PROGRAM_OBJECT_GEOMETRY_H

#include "core_base.h"
#include "drawing_program/drawing_program_object_store.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawing_program_object_path_payload_clear(DrawingProgramObjectRecord *object);
int drawing_program_object_path_payload_valid(const DrawingProgramObjectRecord *object);
CoreResult drawing_program_object_path_payload_bounds(const DrawingProgramPathPayload *payload,
                                                      int32_t *out_origin_x,
                                                      int32_t *out_origin_y,
                                                      uint32_t *out_width,
                                                      uint32_t *out_height);
int drawing_program_object_bounds_contains(const DrawingProgramObjectRecord *object,
                                           uint32_t sample_x,
                                           uint32_t sample_y);
int drawing_program_object_layer_allows_interaction(const DrawingProgramDocument *document,
                                                    uint32_t layer_id);
int drawing_program_object_ellipse_contains(const DrawingProgramObjectRecord *object,
                                            uint32_t sample_x,
                                            uint32_t sample_y);
int drawing_program_object_path_contains(const DrawingProgramObjectRecord *object,
                                         uint32_t sample_x,
                                         uint32_t sample_y);

#ifdef __cplusplus
}
#endif

#endif
