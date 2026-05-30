#ifndef DRAWING_PROGRAM_REFLECTION_STATE_H
#define DRAWING_PROGRAM_REFLECTION_STATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_REFLECTION_REFLECTOR_CAPACITY 8u
#define DRAWING_PROGRAM_REFLECTION_LABEL_SLOT_HORIZONTAL 0u
#define DRAWING_PROGRAM_REFLECTION_LABEL_SLOT_VERTICAL 1u
#define DRAWING_PROGRAM_REFLECTION_LABEL_SLOT_NONE 255u

typedef struct DrawingProgramReflectorLine {
    int32_t anchor_x;
    int32_t anchor_y;
    int32_t direction_dx;
    int32_t direction_dy;
    uint8_t enabled;
    uint8_t label_slot;
    uint8_t reserved0;
    uint8_t reserved1;
} DrawingProgramReflectorLine;

typedef struct DrawingProgramReflectionState {
    uint8_t center_valid;
    uint8_t reflector_count;
    uint8_t active_reflector_index;
    uint8_t reserved0;
    uint32_t center_x;
    uint32_t center_y;
    DrawingProgramReflectorLine reflectors[DRAWING_PROGRAM_REFLECTION_REFLECTOR_CAPACITY];
} DrawingProgramReflectionState;

void drawing_program_reflection_state_init(DrawingProgramReflectionState *state);
void drawing_program_reflection_state_seed_crosshair(DrawingProgramReflectionState *state,
                                                     uint32_t center_x,
                                                     uint32_t center_y);
void drawing_program_reflection_state_set_center(DrawingProgramReflectionState *state,
                                                 uint32_t center_x,
                                                 uint32_t center_y);
void drawing_program_reflection_state_set_crosshair_enabled(DrawingProgramReflectionState *state,
                                                            uint8_t horizontal_enabled,
                                                            uint8_t vertical_enabled);
void drawing_program_reflection_state_crosshair_enabled(const DrawingProgramReflectionState *state,
                                                        uint8_t *out_horizontal_enabled,
                                                        uint8_t *out_vertical_enabled);
int drawing_program_reflection_state_add_reflector(DrawingProgramReflectionState *state,
                                                   int32_t direction_dx,
                                                   int32_t direction_dy);
int drawing_program_reflection_state_cycle_active_reflector(DrawingProgramReflectionState *state, int delta);
int drawing_program_reflection_state_toggle_active_reflector_enabled(DrawingProgramReflectionState *state);
int drawing_program_reflection_state_delete_active_reflector(DrawingProgramReflectionState *state);
int drawing_program_reflection_state_set_active_reflector_direction(DrawingProgramReflectionState *state,
                                                                    int32_t direction_dx,
                                                                    int32_t direction_dy);
void drawing_program_reflection_state_clamp(DrawingProgramReflectionState *state,
                                            uint32_t raster_width,
                                            uint32_t raster_height);
uint32_t drawing_program_reflection_state_collect_point_variants(
    const DrawingProgramReflectionState *state,
    int32_t sample_x,
    int32_t sample_y,
    int32_t *out_x,
    int32_t *out_y,
    uint32_t out_capacity);
uint32_t drawing_program_reflection_state_collect_segment_variants(
    const DrawingProgramReflectionState *state,
    int32_t start_x,
    int32_t start_y,
    int32_t end_x,
    int32_t end_y,
    int32_t *out_start_x,
    int32_t *out_start_y,
    int32_t *out_end_x,
    int32_t *out_end_y,
    uint32_t out_capacity);

#ifdef __cplusplus
}
#endif

#endif
