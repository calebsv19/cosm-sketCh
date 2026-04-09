#ifndef DRAWING_PROGRAM_OVERLAY_ADAPTER_H
#define DRAWING_PROGRAM_OVERLAY_ADAPTER_H

#include <stdint.h>

#include "core_layout.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

typedef enum DrawingProgramOverlayLifecycleState {
    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE = 0,
    DRAWING_PROGRAM_OVERLAY_STATE_AUTHORING_ACTIVE = 1
} DrawingProgramOverlayLifecycleState;

typedef enum DrawingProgramOverlayAdapterErrorCode {
    DRAWING_PROGRAM_OVERLAY_ADAPTER_OK = 0,
    DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE = 1,
    DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT = 2,
    DRAWING_PROGRAM_OVERLAY_ADAPTER_DIRTY_EXIT_REQUIRES_APPLY_OR_CANCEL = 3,
    DRAWING_PROGRAM_OVERLAY_ADAPTER_IO_FAILURE = 4,
    DRAWING_PROGRAM_OVERLAY_ADAPTER_INTERNAL_FAILURE = 5
} DrawingProgramOverlayAdapterErrorCode;

typedef struct DrawingProgramOverlayAdapterResult {
    uint8_t ok;
    DrawingProgramOverlayAdapterErrorCode error_code;
    const char *reason;
    DrawingProgramOverlayLifecycleState state_before;
    DrawingProgramOverlayLifecycleState state_after;
} DrawingProgramOverlayAdapterResult;

typedef struct DrawingProgramOverlayAdapterState {
    DrawingProgramOverlayLifecycleState lifecycle_state;
    uint8_t session_active;
    uint8_t runtime_paused;
    uint8_t pending_apply;
    uint64_t pause_epoch;
    uint64_t resume_epoch;
} DrawingProgramOverlayAdapterState;

DrawingProgramOverlayAdapterResult drawing_program_overlay_adapter_init(
    struct DrawingProgramAppContext *ctx);
DrawingProgramOverlayAdapterResult drawing_program_adapter_overlay_session_begin(
    struct DrawingProgramAppContext *ctx);
DrawingProgramOverlayAdapterResult drawing_program_adapter_overlay_session_end(
    struct DrawingProgramAppContext *ctx);
DrawingProgramOverlayAdapterResult drawing_program_adapter_snapshot_capture_active(
    struct DrawingProgramAppContext *ctx);
DrawingProgramOverlayAdapterResult drawing_program_adapter_snapshot_stage_draft(
    struct DrawingProgramAppContext *ctx);
DrawingProgramOverlayAdapterResult drawing_program_adapter_snapshot_commit_draft(
    struct DrawingProgramAppContext *ctx);
DrawingProgramOverlayAdapterResult drawing_program_adapter_snapshot_discard_draft(
    struct DrawingProgramAppContext *ctx);
DrawingProgramOverlayAdapterResult drawing_program_adapter_runtime_pause(
    struct DrawingProgramAppContext *ctx);
DrawingProgramOverlayAdapterResult drawing_program_adapter_runtime_resume(
    struct DrawingProgramAppContext *ctx);
DrawingProgramOverlayAdapterResult drawing_program_adapter_runtime_tick(
    struct DrawingProgramAppContext *ctx);
DrawingProgramOverlayAdapterResult drawing_program_adapter_input_route_runtime(
    struct DrawingProgramAppContext *ctx);
DrawingProgramOverlayAdapterResult drawing_program_adapter_input_route_overlay(
    struct DrawingProgramAppContext *ctx);
DrawingProgramOverlayAdapterResult drawing_program_adapter_render_runtime_base(
    struct DrawingProgramAppContext *ctx);
DrawingProgramOverlayAdapterResult drawing_program_adapter_render_overlay_chrome(
    struct DrawingProgramAppContext *ctx);
DrawingProgramOverlayAdapterResult drawing_program_adapter_persist_save_session(
    struct DrawingProgramAppContext *ctx);
DrawingProgramOverlayAdapterResult drawing_program_adapter_persist_export_debug_json(
    struct DrawingProgramAppContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
