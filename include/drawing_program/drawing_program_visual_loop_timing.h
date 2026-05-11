#ifndef DRAWING_PROGRAM_VISUAL_LOOP_TIMING_H
#define DRAWING_PROGRAM_VISUAL_LOOP_TIMING_H

#include <stdint.h>

typedef struct DrawingProgramVisualLoopWaitPolicyInput {
    uint8_t high_intensity_mode;
    uint8_t interaction_active;
    uint8_t background_busy;
    uint8_t resize_pending;
} DrawingProgramVisualLoopWaitPolicyInput;

typedef struct DrawingProgramVisualLoopRenderPolicyInput {
    uint8_t background_busy;
    uint8_t frame_dirty;
    uint8_t force_render;
    uint32_t now_ms;
    uint32_t last_present_ms;
} DrawingProgramVisualLoopRenderPolicyInput;

int drawing_program_visual_loop_compute_wait_timeout_ms(
    const DrawingProgramVisualLoopWaitPolicyInput *input);
uint32_t drawing_program_visual_loop_render_heartbeat_ms(void);
int drawing_program_visual_loop_should_render_frame(
    const DrawingProgramVisualLoopRenderPolicyInput *input);
void drawing_program_visual_loop_diag_tick(double frame_elapsed_sec,
                                           uint32_t wait_blocked_ms,
                                           uint32_t wait_call_count);

#endif
