#include "drawing_program/drawing_program_visual_loop_timing.h"

#include <stdlib.h>

enum {
    DRAWING_PROGRAM_LOOP_WAIT_IDLE_DEFAULT_MS = 120,
    DRAWING_PROGRAM_LOOP_WAIT_BUSY_MS = 2,
    DRAWING_PROGRAM_LOOP_WAIT_MIN_MS = 1,
    DRAWING_PROGRAM_LOOP_WAIT_MAX_MS = 5000,
    DRAWING_PROGRAM_LOOP_RENDER_HEARTBEAT_DEFAULT_MS = 0,
    DRAWING_PROGRAM_LOOP_RENDER_HEARTBEAT_MIN_MS = 0,
    DRAWING_PROGRAM_LOOP_RENDER_HEARTBEAT_MAX_MS = 5000
};

static int drawing_program_visual_loop_env_wait_override_ms(void) {
    static int initialized = 0;
    static int cached_value = -1;
    const char *wait_env = getenv("DRAWING_PROGRAM_LOOP_MAX_WAIT_MS");
    char *end = NULL;
    long parsed = 0;
    if (initialized) {
        return cached_value;
    }
    initialized = 1;
    if (!wait_env || wait_env[0] == '\0') {
        return cached_value;
    }
    parsed = strtol(wait_env, &end, 10);
    if (end == wait_env ||
        parsed < DRAWING_PROGRAM_LOOP_WAIT_MIN_MS ||
        parsed > DRAWING_PROGRAM_LOOP_WAIT_MAX_MS) {
        return cached_value;
    }
    cached_value = (int)parsed;
    return cached_value;
}

static int drawing_program_visual_loop_env_render_heartbeat_override_ms(void) {
    static int initialized = 0;
    static int cached_value = -1;
    const char *heartbeat_env = getenv("DRAWING_PROGRAM_IDLE_RENDER_HEARTBEAT_MS");
    char *end = NULL;
    long parsed = 0;
    if (initialized) {
        return cached_value;
    }
    initialized = 1;
    if (!heartbeat_env || heartbeat_env[0] == '\0') {
        return cached_value;
    }
    parsed = strtol(heartbeat_env, &end, 10);
    if (end == heartbeat_env ||
        parsed < 0 ||
        parsed > DRAWING_PROGRAM_LOOP_RENDER_HEARTBEAT_MAX_MS) {
        return cached_value;
    }
    cached_value = (int)parsed;
    return cached_value;
}

int drawing_program_visual_loop_compute_wait_timeout_ms(
    const DrawingProgramVisualLoopWaitPolicyInput *input) {
    int timeout_ms = DRAWING_PROGRAM_LOOP_WAIT_IDLE_DEFAULT_MS;
    int env_override = -1;
    if (!input) {
        return 0;
    }
    if (input->high_intensity_mode) {
        return 0;
    }
    if (input->interaction_active ||
        input->background_busy ||
        input->resize_pending) {
        timeout_ms = DRAWING_PROGRAM_LOOP_WAIT_BUSY_MS;
    }
    env_override = drawing_program_visual_loop_env_wait_override_ms();
    if (env_override > 0 && timeout_ms > env_override) {
        timeout_ms = env_override;
    }
    if (timeout_ms < DRAWING_PROGRAM_LOOP_WAIT_MIN_MS) {
        timeout_ms = DRAWING_PROGRAM_LOOP_WAIT_MIN_MS;
    }
    if (timeout_ms > DRAWING_PROGRAM_LOOP_WAIT_MAX_MS) {
        timeout_ms = DRAWING_PROGRAM_LOOP_WAIT_MAX_MS;
    }
    return timeout_ms;
}

uint32_t drawing_program_visual_loop_render_heartbeat_ms(void) {
    int heartbeat_ms = DRAWING_PROGRAM_LOOP_RENDER_HEARTBEAT_DEFAULT_MS;
    int env_override = drawing_program_visual_loop_env_render_heartbeat_override_ms();
    if (env_override > 0) {
        heartbeat_ms = env_override;
    }
    if (heartbeat_ms > DRAWING_PROGRAM_LOOP_RENDER_HEARTBEAT_MAX_MS) {
        heartbeat_ms = DRAWING_PROGRAM_LOOP_RENDER_HEARTBEAT_MAX_MS;
    }
    return (uint32_t)heartbeat_ms;
}

int drawing_program_visual_loop_should_render_frame(
    const DrawingProgramVisualLoopRenderPolicyInput *input) {
    uint32_t heartbeat_ms = 0u;
    if (!input) {
        return 1;
    }
    if (input->force_render || input->frame_dirty || input->background_busy || input->last_present_ms == 0u) {
        return 1;
    }
    heartbeat_ms = drawing_program_visual_loop_render_heartbeat_ms();
    if (heartbeat_ms == 0u) {
        return 0;
    }
    return (input->now_ms - input->last_present_ms) >= heartbeat_ms ? 1 : 0;
}
