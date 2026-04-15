#ifndef DRAWING_PROGRAM_OBJECT_STORE_H
#define DRAWING_PROGRAM_OBJECT_STORE_H

#include <stdint.h>

#include "core_base.h"
#include "drawing_program/drawing_program_document.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_OBJECT_STORE_SCHEMA_VERSION 1u
#define DRAWING_PROGRAM_MAX_OBJECTS 256u
#define DRAWING_PROGRAM_OBJECT_NAME_CAPACITY 32u
#define DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS 128u

typedef enum DrawingProgramObjectType {
    DRAWING_PROGRAM_OBJECT_TYPE_INVALID = 0u,
    DRAWING_PROGRAM_OBJECT_TYPE_RECT = 1u,
    DRAWING_PROGRAM_OBJECT_TYPE_ELLIPSE = 2u,
    DRAWING_PROGRAM_OBJECT_TYPE_PATH = 3u
} DrawingProgramObjectType;

typedef struct DrawingProgramPathPoint {
    int32_t x;
    int32_t y;
} DrawingProgramPathPoint;

typedef struct DrawingProgramPathPayload {
    uint32_t point_count;
    uint8_t closed;
    uint8_t reserved0;
    uint8_t reserved1;
    uint8_t reserved2;
    DrawingProgramPathPoint points[DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS];
} DrawingProgramPathPayload;

typedef struct DrawingProgramObjectRecord {
    uint32_t object_id;
    uint32_t layer_id;
    uint8_t type;
    uint8_t visible;
    uint8_t locked;
    uint8_t stroke_color_index;
    uint8_t fill_color_index;
    uint8_t stroke_width;
    uint8_t style_mode;
    uint8_t reserved0;
    uint8_t reserved1;
    int32_t origin_x;
    int32_t origin_y;
    uint32_t width;
    uint32_t height;
    char name[DRAWING_PROGRAM_OBJECT_NAME_CAPACITY];
    uint16_t path_point_count;
    uint8_t path_closed;
    uint8_t path_reserved0;
    DrawingProgramPathPoint path_points[DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS];
} DrawingProgramObjectRecord;

typedef struct DrawingProgramObjectStore {
    uint32_t schema_version;
    uint32_t object_count;
    uint32_t next_object_id;
    DrawingProgramObjectRecord objects[DRAWING_PROGRAM_MAX_OBJECTS];
} DrawingProgramObjectStore;

typedef struct DrawingProgramObjectSelectionState {
    uint32_t count;
    uint32_t active_object_id;
    uint32_t object_ids[DRAWING_PROGRAM_MAX_OBJECTS];
} DrawingProgramObjectSelectionState;

void drawing_program_object_store_reset(DrawingProgramObjectStore *store);
CoreResult drawing_program_object_store_find_index_for_id(const DrawingProgramObjectStore *store,
                                                          uint32_t object_id,
                                                          uint32_t *out_index);
const DrawingProgramObjectRecord *drawing_program_object_store_get_by_id(const DrawingProgramObjectStore *store,
                                                                         uint32_t object_id);
CoreResult drawing_program_object_store_add(DrawingProgramObjectStore *store,
                                            const DrawingProgramObjectRecord *seed,
                                            uint32_t *out_object_id);
CoreResult drawing_program_object_store_add_path(DrawingProgramObjectStore *store,
                                                 const DrawingProgramObjectRecord *seed_style,
                                                 const DrawingProgramPathPayload *payload,
                                                 uint32_t *out_object_id);
CoreResult drawing_program_object_store_insert_with_id(DrawingProgramObjectStore *store,
                                                       const DrawingProgramObjectRecord *seed,
                                                       uint32_t object_id);
CoreResult drawing_program_object_store_remove_by_id(DrawingProgramObjectStore *store,
                                                     uint32_t object_id,
                                                     uint32_t *out_removed_index);
CoreResult drawing_program_object_store_replace_all(DrawingProgramObjectStore *store,
                                                    const DrawingProgramObjectRecord *records,
                                                    uint32_t record_count,
                                                    uint32_t next_object_id);
CoreResult drawing_program_object_store_set_origin(DrawingProgramObjectStore *store,
                                                   uint32_t object_id,
                                                   int32_t origin_x,
                                                   int32_t origin_y,
                                                   int32_t *out_previous_x,
                                                   int32_t *out_previous_y);
CoreResult drawing_program_object_store_set_path_payload(DrawingProgramObjectStore *store,
                                                         uint32_t object_id,
                                                         const DrawingProgramPathPayload *payload);
CoreResult drawing_program_object_store_set_path_point(DrawingProgramObjectStore *store,
                                                       uint32_t object_id,
                                                       uint16_t point_index,
                                                       int32_t point_x,
                                                       int32_t point_y,
                                                       int32_t *out_previous_x,
                                                       int32_t *out_previous_y);
CoreResult drawing_program_object_store_get_path_payload(const DrawingProgramObjectStore *store,
                                                         uint32_t object_id,
                                                         DrawingProgramPathPayload *out_payload);
CoreResult drawing_program_object_store_hit_test_selected_path_point(
    const DrawingProgramObjectStore *store,
    const DrawingProgramDocument *document,
    const DrawingProgramObjectSelectionState *selection,
    uint32_t sample_x,
    uint32_t sample_y,
    uint32_t pick_radius_samples,
    uint32_t *out_object_id,
    uint16_t *out_point_index);
CoreResult drawing_program_object_store_clamp_selection_delta(const DrawingProgramObjectStore *store,
                                                              const DrawingProgramDocument *document,
                                                              const DrawingProgramObjectSelectionState *selection,
                                                              int32_t requested_dx,
                                                              int32_t requested_dy,
                                                              int32_t *out_dx,
                                                              int32_t *out_dy);
CoreResult drawing_program_object_store_apply_selection_delta(DrawingProgramObjectStore *store,
                                                              const DrawingProgramObjectSelectionState *selection,
                                                              int32_t dx,
                                                              int32_t dy);
CoreResult drawing_program_object_store_promote_selection(DrawingProgramObjectStore *store,
                                                          const DrawingProgramObjectSelectionState *selection);
CoreResult drawing_program_object_store_hit_test_topmost(const DrawingProgramObjectStore *store,
                                                         const DrawingProgramDocument *document,
                                                         uint32_t sample_x,
                                                         uint32_t sample_y,
                                                         uint32_t *out_object_id,
                                                         uint32_t *out_index);
uint32_t drawing_program_object_store_collect_visible_indices(const DrawingProgramObjectStore *store,
                                                              const DrawingProgramDocument *document,
                                                              uint32_t *out_indices,
                                                              uint32_t out_capacity);
void drawing_program_object_selection_reset(DrawingProgramObjectSelectionState *selection);
int drawing_program_object_selection_contains(const DrawingProgramObjectSelectionState *selection,
                                              uint32_t object_id);
void drawing_program_object_selection_replace_single(DrawingProgramObjectSelectionState *selection,
                                                     uint32_t object_id);
int drawing_program_object_selection_add(DrawingProgramObjectSelectionState *selection,
                                         uint32_t object_id);

#ifdef __cplusplus
}
#endif

#endif
