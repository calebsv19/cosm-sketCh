#include "drawing_program/drawing_program_reflection_state.h"

#include <limits.h>
#include <math.h>
#include <string.h>

static void reflection_state_seed_canonical_crosshair_reflectors(DrawingProgramReflectionState *state) {
    if (!state) {
        return;
    }
    if (state->reflector_count < 2u) {
        state->reflector_count = 2u;
    }
    state->reflectors[0].direction_dx = 1;
    state->reflectors[0].direction_dy = 0;
    state->reflectors[0].label_slot = DRAWING_PROGRAM_REFLECTION_LABEL_SLOT_HORIZONTAL;
    state->reflectors[1].direction_dx = 0;
    state->reflectors[1].direction_dy = 1;
    state->reflectors[1].label_slot = DRAWING_PROGRAM_REFLECTION_LABEL_SLOT_VERTICAL;
}

static uint32_t reflection_state_clamp_axis(uint32_t axis, uint32_t limit) {
    if (limit == 0u) {
        return 0u;
    }
    if (axis >= limit) {
        return limit - 1u;
    }
    return axis;
}

static int reflection_state_reflector_is_valid(const DrawingProgramReflectorLine *line) {
    return line && (line->direction_dx != 0 || line->direction_dy != 0);
}

static void reflection_state_reflect_point(const DrawingProgramReflectorLine *line,
                                           int32_t src_x,
                                           int32_t src_y,
                                           int32_t *out_x,
                                           int32_t *out_y) {
    double anchor_x;
    double anchor_y;
    double dir_x;
    double dir_y;
    double length;
    double unit_x;
    double unit_y;
    double rel_x;
    double rel_y;
    double projection;
    double next_x;
    double next_y;
    if (!line || !out_x || !out_y || !reflection_state_reflector_is_valid(line)) {
        if (out_x) {
            *out_x = src_x;
        }
        if (out_y) {
            *out_y = src_y;
        }
        return;
    }
    anchor_x = (double)line->anchor_x;
    anchor_y = (double)line->anchor_y;
    dir_x = (double)line->direction_dx;
    dir_y = (double)line->direction_dy;
    length = sqrt((dir_x * dir_x) + (dir_y * dir_y));
    if (length <= 0.0) {
        *out_x = src_x;
        *out_y = src_y;
        return;
    }
    unit_x = dir_x / length;
    unit_y = dir_y / length;
    rel_x = (double)src_x - anchor_x;
    rel_y = (double)src_y - anchor_y;
    projection = (rel_x * unit_x) + (rel_y * unit_y);
    next_x = anchor_x + (2.0 * projection * unit_x) - rel_x;
    next_y = anchor_y + (2.0 * projection * unit_y) - rel_y;
    *out_x = (int32_t)lround(next_x);
    *out_y = (int32_t)lround(next_y);
}

static int reflection_state_duplicate_point(const int32_t *xs,
                                            const int32_t *ys,
                                            uint32_t point_count,
                                            int32_t x,
                                            int32_t y) {
    uint32_t i;
    for (i = 0u; i < point_count; ++i) {
        if (xs[i] == x && ys[i] == y) {
            return 1;
        }
    }
    return 0;
}

static int reflection_state_duplicate_segment(const int32_t *start_xs,
                                              const int32_t *start_ys,
                                              const int32_t *end_xs,
                                              const int32_t *end_ys,
                                              uint32_t segment_count,
                                              int32_t start_x,
                                              int32_t start_y,
                                              int32_t end_x,
                                              int32_t end_y) {
    uint32_t i;
    for (i = 0u; i < segment_count; ++i) {
        if ((start_xs[i] == start_x &&
             start_ys[i] == start_y &&
             end_xs[i] == end_x &&
             end_ys[i] == end_y) ||
            (start_xs[i] == end_x &&
             start_ys[i] == end_y &&
             end_xs[i] == start_x &&
             end_ys[i] == start_y)) {
            return 1;
        }
    }
    return 0;
}

static uint32_t reflection_state_mask_popcount(uint32_t mask) {
    uint32_t count = 0u;
    while (mask != 0u) {
        count += (mask & 1u);
        mask >>= 1u;
    }
    return count;
}

static uint32_t reflection_state_collect_enabled_indices(const DrawingProgramReflectionState *state,
                                                         uint32_t *out_indices,
                                                         uint32_t out_capacity) {
    uint32_t count = 0u;
    uint32_t i;
    if (!state || !out_indices || out_capacity == 0u) {
        return 0u;
    }
    for (i = 0u; i < state->reflector_count && i < DRAWING_PROGRAM_REFLECTION_REFLECTOR_CAPACITY; ++i) {
        const DrawingProgramReflectorLine *line = &state->reflectors[i];
        if (!line->enabled || !reflection_state_reflector_is_valid(line)) {
            continue;
        }
        if (count < out_capacity) {
            out_indices[count++] = i;
        }
    }
    return count;
}

void drawing_program_reflection_state_init(DrawingProgramReflectionState *state) {
    if (!state) {
        return;
    }
    memset(state, 0, sizeof(*state));
}

void drawing_program_reflection_state_seed_crosshair(DrawingProgramReflectionState *state,
                                                     uint32_t center_x,
                                                     uint32_t center_y) {
    if (!state) {
        return;
    }
    drawing_program_reflection_state_init(state);
    state->center_valid = 1u;
    state->center_x = center_x;
    state->center_y = center_y;
    reflection_state_seed_canonical_crosshair_reflectors(state);
    drawing_program_reflection_state_set_center(state, center_x, center_y);
}

void drawing_program_reflection_state_set_center(DrawingProgramReflectionState *state,
                                                 uint32_t center_x,
                                                 uint32_t center_y) {
    uint32_t i;
    if (!state) {
        return;
    }
    state->center_valid = 1u;
    state->center_x = center_x;
    state->center_y = center_y;
    for (i = 0u; i < state->reflector_count && i < DRAWING_PROGRAM_REFLECTION_REFLECTOR_CAPACITY; ++i) {
        state->reflectors[i].anchor_x = (int32_t)center_x;
        state->reflectors[i].anchor_y = (int32_t)center_y;
    }
}

void drawing_program_reflection_state_set_crosshair_enabled(DrawingProgramReflectionState *state,
                                                            uint8_t horizontal_enabled,
                                                            uint8_t vertical_enabled) {
    if (!state) {
        return;
    }
    reflection_state_seed_canonical_crosshair_reflectors(state);
    state->reflectors[0].enabled = horizontal_enabled ? 1u : 0u;
    state->reflectors[1].enabled = vertical_enabled ? 1u : 0u;
}

void drawing_program_reflection_state_crosshair_enabled(const DrawingProgramReflectionState *state,
                                                        uint8_t *out_horizontal_enabled,
                                                        uint8_t *out_vertical_enabled) {
    uint8_t horizontal_enabled = 0u;
    uint8_t vertical_enabled = 0u;
    if (state && state->reflector_count > 0u) {
        uint32_t i;
        for (i = 0u; i < state->reflector_count && i < DRAWING_PROGRAM_REFLECTION_REFLECTOR_CAPACITY; ++i) {
            const DrawingProgramReflectorLine *line = &state->reflectors[i];
            if (line->label_slot == DRAWING_PROGRAM_REFLECTION_LABEL_SLOT_HORIZONTAL && line->enabled) {
                horizontal_enabled = 1u;
            } else if (line->label_slot == DRAWING_PROGRAM_REFLECTION_LABEL_SLOT_VERTICAL && line->enabled) {
                vertical_enabled = 1u;
            }
        }
    }
    if (out_horizontal_enabled) {
        *out_horizontal_enabled = horizontal_enabled;
    }
    if (out_vertical_enabled) {
        *out_vertical_enabled = vertical_enabled;
    }
}

int drawing_program_reflection_state_add_reflector(DrawingProgramReflectionState *state,
                                                   int32_t direction_dx,
                                                   int32_t direction_dy) {
    DrawingProgramReflectorLine *line;
    uint32_t index;
    if (!state || (direction_dx == 0 && direction_dy == 0)) {
        return 0;
    }
    if (state->reflector_count >= DRAWING_PROGRAM_REFLECTION_REFLECTOR_CAPACITY) {
        return 0;
    }
    index = state->reflector_count;
    line = &state->reflectors[index];
    memset(line, 0, sizeof(*line));
    line->anchor_x = (int32_t)state->center_x;
    line->anchor_y = (int32_t)state->center_y;
    line->direction_dx = direction_dx;
    line->direction_dy = direction_dy;
    line->enabled = 1u;
    line->label_slot = DRAWING_PROGRAM_REFLECTION_LABEL_SLOT_NONE;
    state->reflector_count = (uint8_t)(index + 1u);
    state->active_reflector_index = (uint8_t)index;
    return 1;
}

int drawing_program_reflection_state_cycle_active_reflector(DrawingProgramReflectionState *state, int delta) {
    int next_index = 0;
    if (!state || state->reflector_count == 0u || delta == 0) {
        return 0;
    }
    next_index = (int)state->active_reflector_index + delta;
    while (next_index < 0) {
        next_index += (int)state->reflector_count;
    }
    state->active_reflector_index = (uint8_t)(next_index % (int)state->reflector_count);
    return 1;
}

int drawing_program_reflection_state_toggle_active_reflector_enabled(DrawingProgramReflectionState *state) {
    DrawingProgramReflectorLine *line;
    if (!state || state->reflector_count == 0u || state->active_reflector_index >= state->reflector_count) {
        return 0;
    }
    line = &state->reflectors[state->active_reflector_index];
    line->enabled = line->enabled ? 0u : 1u;
    return 1;
}

int drawing_program_reflection_state_delete_active_reflector(DrawingProgramReflectionState *state) {
    uint32_t index;
    if (!state || state->reflector_count == 0u || state->active_reflector_index >= state->reflector_count) {
        return 0;
    }
    index = state->active_reflector_index;
    if (state->reflectors[index].label_slot != DRAWING_PROGRAM_REFLECTION_LABEL_SLOT_NONE) {
        return 0;
    }
    if (index + 1u < state->reflector_count) {
        memmove(&state->reflectors[index],
                &state->reflectors[index + 1u],
                (size_t)(state->reflector_count - index - 1u) * sizeof(state->reflectors[0]));
    }
    memset(&state->reflectors[state->reflector_count - 1u], 0, sizeof(state->reflectors[0]));
    state->reflector_count -= 1u;
    if (state->reflector_count == 0u) {
        state->active_reflector_index = 0u;
    } else if (state->active_reflector_index >= state->reflector_count) {
        state->active_reflector_index = (uint8_t)(state->reflector_count - 1u);
    }
    return 1;
}

int drawing_program_reflection_state_set_active_reflector_direction(DrawingProgramReflectionState *state,
                                                                    int32_t direction_dx,
                                                                    int32_t direction_dy) {
    DrawingProgramReflectorLine *line;
    if (!state || state->reflector_count == 0u || state->active_reflector_index >= state->reflector_count ||
        (direction_dx == 0 && direction_dy == 0) || direction_dx == INT32_MIN || direction_dy == INT32_MIN) {
        return 0;
    }
    line = &state->reflectors[state->active_reflector_index];
    line->anchor_x = (int32_t)state->center_x;
    line->anchor_y = (int32_t)state->center_y;
    line->direction_dx = direction_dx;
    line->direction_dy = direction_dy;
    return 1;
}

void drawing_program_reflection_state_clamp(DrawingProgramReflectionState *state,
                                            uint32_t raster_width,
                                            uint32_t raster_height) {
    uint32_t i;
    if (!state) {
        return;
    }
    state->center_x = reflection_state_clamp_axis(state->center_x, raster_width);
    state->center_y = reflection_state_clamp_axis(state->center_y, raster_height);
    if (state->reflector_count > DRAWING_PROGRAM_REFLECTION_REFLECTOR_CAPACITY) {
        state->reflector_count = DRAWING_PROGRAM_REFLECTION_REFLECTOR_CAPACITY;
    }
    if (state->active_reflector_index >= state->reflector_count && state->reflector_count > 0u) {
        state->active_reflector_index = (uint8_t)(state->reflector_count - 1u);
    }
    for (i = 0u; i < state->reflector_count; ++i) {
        state->reflectors[i].anchor_x = (int32_t)reflection_state_clamp_axis((uint32_t)state->reflectors[i].anchor_x,
                                                                              raster_width);
        state->reflectors[i].anchor_y = (int32_t)reflection_state_clamp_axis((uint32_t)state->reflectors[i].anchor_y,
                                                                              raster_height);
        if (!reflection_state_reflector_is_valid(&state->reflectors[i])) {
            if (state->reflectors[i].label_slot == DRAWING_PROGRAM_REFLECTION_LABEL_SLOT_HORIZONTAL) {
                state->reflectors[i].direction_dx = 1;
                state->reflectors[i].direction_dy = 0;
            } else if (state->reflectors[i].label_slot == DRAWING_PROGRAM_REFLECTION_LABEL_SLOT_VERTICAL) {
                state->reflectors[i].direction_dx = 0;
                state->reflectors[i].direction_dy = 1;
            }
        }
    }
}

uint32_t drawing_program_reflection_state_collect_point_variants(
    const DrawingProgramReflectionState *state,
    int32_t sample_x,
    int32_t sample_y,
    int32_t *out_x,
    int32_t *out_y,
    uint32_t out_capacity) {
    uint32_t enabled_indices[DRAWING_PROGRAM_REFLECTION_REFLECTOR_CAPACITY];
    uint32_t enabled_count;
    uint32_t mask_limit;
    uint32_t mask;
    uint32_t point_count = 0u;
    if (!state || !out_x || !out_y || out_capacity == 0u) {
        return 0u;
    }
    enabled_count =
        reflection_state_collect_enabled_indices(state, enabled_indices, DRAWING_PROGRAM_REFLECTION_REFLECTOR_CAPACITY);
    mask_limit = (1u << enabled_count);
    /* Order variants by reflection depth first, then by reflector index mask.
     * That keeps bounded-capacity truncation stable and prioritizes source plus
     * single-reflector variants before deeper compositions. */
    for (mask = 0u; mask <= enabled_count && point_count < out_capacity; ++mask) {
        uint32_t subset_size = mask;
        uint32_t subset_mask;
        for (subset_mask = 0u; subset_mask < mask_limit && point_count < out_capacity; ++subset_mask) {
            int32_t next_x = sample_x;
            int32_t next_y = sample_y;
            uint32_t bit;
            if (reflection_state_mask_popcount(subset_mask) != subset_size) {
                continue;
            }
            for (bit = 0u; bit < enabled_count; ++bit) {
                if ((subset_mask & (1u << bit)) == 0u) {
                    continue;
                }
                reflection_state_reflect_point(&state->reflectors[enabled_indices[bit]],
                                               next_x,
                                               next_y,
                                               &next_x,
                                               &next_y);
            }
            if (reflection_state_duplicate_point(out_x, out_y, point_count, next_x, next_y)) {
                continue;
            }
            out_x[point_count] = next_x;
            out_y[point_count] = next_y;
            point_count += 1u;
        }
    }
    if (point_count == 0u) {
        out_x[0] = sample_x;
        out_y[0] = sample_y;
        point_count = 1u;
    }
    return point_count;
}

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
    uint32_t out_capacity) {
    uint32_t enabled_indices[DRAWING_PROGRAM_REFLECTION_REFLECTOR_CAPACITY];
    uint32_t enabled_count;
    uint32_t mask_limit;
    uint32_t mask;
    uint32_t segment_count = 0u;
    if (!state || !out_start_x || !out_start_y || !out_end_x || !out_end_y || out_capacity == 0u) {
        return 0u;
    }
    enabled_count =
        reflection_state_collect_enabled_indices(state, enabled_indices, DRAWING_PROGRAM_REFLECTION_REFLECTOR_CAPACITY);
    mask_limit = (1u << enabled_count);
    for (mask = 0u; mask <= enabled_count && segment_count < out_capacity; ++mask) {
        uint32_t subset_size = mask;
        uint32_t subset_mask;
        for (subset_mask = 0u; subset_mask < mask_limit && segment_count < out_capacity; ++subset_mask) {
            int32_t next_start_x = start_x;
            int32_t next_start_y = start_y;
            int32_t next_end_x = end_x;
            int32_t next_end_y = end_y;
            uint32_t bit;
            if (reflection_state_mask_popcount(subset_mask) != subset_size) {
                continue;
            }
            for (bit = 0u; bit < enabled_count; ++bit) {
                if ((subset_mask & (1u << bit)) == 0u) {
                    continue;
                }
                reflection_state_reflect_point(&state->reflectors[enabled_indices[bit]],
                                               next_start_x,
                                               next_start_y,
                                               &next_start_x,
                                               &next_start_y);
                reflection_state_reflect_point(&state->reflectors[enabled_indices[bit]],
                                               next_end_x,
                                               next_end_y,
                                               &next_end_x,
                                               &next_end_y);
            }
            if (reflection_state_duplicate_segment(out_start_x,
                                                   out_start_y,
                                                   out_end_x,
                                                   out_end_y,
                                                   segment_count,
                                                   next_start_x,
                                                   next_start_y,
                                                   next_end_x,
                                                   next_end_y)) {
                continue;
            }
            out_start_x[segment_count] = next_start_x;
            out_start_y[segment_count] = next_start_y;
            out_end_x[segment_count] = next_end_x;
            out_end_y[segment_count] = next_end_y;
            segment_count += 1u;
        }
    }
    if (segment_count == 0u) {
        out_start_x[0] = start_x;
        out_start_y[0] = start_y;
        out_end_x[0] = end_x;
        out_end_y[0] = end_y;
        segment_count = 1u;
    }
    return segment_count;
}
