#include "drawing_program/drawing_program_overlay_adapter.h"

#include <string.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_pane_host.h"
#include "drawing_program/drawing_program_snapshot.h"

static DrawingProgramOverlayAdapterResult adapter_result_ok(DrawingProgramOverlayLifecycleState before,
                                                            DrawingProgramOverlayLifecycleState after) {
    DrawingProgramOverlayAdapterResult result;
    result.ok = 1u;
    result.error_code = DRAWING_PROGRAM_OVERLAY_ADAPTER_OK;
    result.reason = "ok";
    result.state_before = before;
    result.state_after = after;
    return result;
}

static DrawingProgramOverlayAdapterResult adapter_result_error(
    DrawingProgramOverlayAdapterErrorCode error_code,
    const char *reason,
    DrawingProgramOverlayLifecycleState before) {
    DrawingProgramOverlayAdapterResult result;
    result.ok = 0u;
    result.error_code = error_code;
    result.reason = reason;
    result.state_before = before;
    result.state_after = before;
    return result;
}

DrawingProgramOverlayAdapterResult drawing_program_overlay_adapter_init(
    struct DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "null app context",
                                    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
    }
    memset(&ctx->overlay_adapter, 0, sizeof(ctx->overlay_adapter));
    ctx->overlay_adapter.lifecycle_state = DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE;
    return adapter_result_ok(DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE,
                             DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
}

DrawingProgramOverlayAdapterResult drawing_program_adapter_snapshot_capture_active(
    struct DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "null app context",
                                    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
    }
    return adapter_result_ok(ctx->overlay_adapter.lifecycle_state, ctx->overlay_adapter.lifecycle_state);
}

DrawingProgramOverlayAdapterResult drawing_program_adapter_runtime_pause(
    struct DrawingProgramAppContext *ctx) {
    DrawingProgramOverlayLifecycleState before;
    if (!ctx) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "null app context",
                                    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
    }
    before = ctx->overlay_adapter.lifecycle_state;
    ctx->overlay_adapter.runtime_paused = 1u;
    ctx->overlay_adapter.pause_epoch += 1u;
    return adapter_result_ok(before, before);
}

DrawingProgramOverlayAdapterResult drawing_program_adapter_runtime_resume(
    struct DrawingProgramAppContext *ctx) {
    DrawingProgramOverlayLifecycleState before;
    if (!ctx) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "null app context",
                                    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
    }
    before = ctx->overlay_adapter.lifecycle_state;
    ctx->overlay_adapter.runtime_paused = 0u;
    ctx->overlay_adapter.resume_epoch += 1u;
    return adapter_result_ok(before, before);
}

DrawingProgramOverlayAdapterResult drawing_program_adapter_overlay_session_begin(
    struct DrawingProgramAppContext *ctx) {
    DrawingProgramOverlayLifecycleState before;
    if (!ctx) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "null app context",
                                    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
    }
    before = ctx->overlay_adapter.lifecycle_state;
    if (before != DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE || ctx->overlay_adapter.session_active) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                    "overlay enter requires runtime_active",
                                    before);
    }
    if (!core_layout_enter_authoring(&ctx->pane_host.layout_state)) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INTERNAL_FAILURE,
                                    "core_layout_enter_authoring failed",
                                    before);
    }
    if (!drawing_program_adapter_snapshot_capture_active(ctx).ok) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INTERNAL_FAILURE,
                                    "snapshot capture failed",
                                    before);
    }
    if (!drawing_program_adapter_runtime_pause(ctx).ok) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INTERNAL_FAILURE,
                                    "runtime pause failed",
                                    before);
    }

    ctx->overlay_adapter.lifecycle_state = DRAWING_PROGRAM_OVERLAY_STATE_AUTHORING_ACTIVE;
    ctx->overlay_adapter.session_active = 1u;
    ctx->overlay_adapter.pending_apply = 0u;
    return adapter_result_ok(before, ctx->overlay_adapter.lifecycle_state);
}

DrawingProgramOverlayAdapterResult drawing_program_adapter_snapshot_stage_draft(
    struct DrawingProgramAppContext *ctx) {
    DrawingProgramOverlayLifecycleState before;
    if (!ctx) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "null app context",
                                    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
    }
    before = ctx->overlay_adapter.lifecycle_state;
    if (before != DRAWING_PROGRAM_OVERLAY_STATE_AUTHORING_ACTIVE) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                    "draft stage requires authoring state",
                                    before);
    }
    if (!core_layout_mark_draft_changed(&ctx->pane_host.layout_state)) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INTERNAL_FAILURE,
                                    "core_layout_mark_draft_changed failed",
                                    before);
    }
    ctx->overlay_adapter.pending_apply = 1u;
    return adapter_result_ok(before, before);
}

DrawingProgramOverlayAdapterResult drawing_program_adapter_snapshot_commit_draft(
    struct DrawingProgramAppContext *ctx) {
    DrawingProgramOverlayLifecycleState before;
    if (!ctx) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "null app context",
                                    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
    }
    before = ctx->overlay_adapter.lifecycle_state;
    if (before != DRAWING_PROGRAM_OVERLAY_STATE_AUTHORING_ACTIVE || !ctx->overlay_adapter.session_active) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                    "commit requires active authoring session",
                                    before);
    }
    if (!core_layout_apply_authoring(&ctx->pane_host.layout_state)) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INTERNAL_FAILURE,
                                    "core_layout_apply_authoring failed",
                                    before);
    }
    ctx->overlay_adapter.pending_apply = 0u;
    ctx->overlay_adapter.session_active = 0u;
    ctx->overlay_adapter.lifecycle_state = DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE;
    if (!drawing_program_adapter_runtime_resume(ctx).ok) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INTERNAL_FAILURE,
                                    "runtime resume failed",
                                    before);
    }
    return adapter_result_ok(before, ctx->overlay_adapter.lifecycle_state);
}

DrawingProgramOverlayAdapterResult drawing_program_adapter_snapshot_discard_draft(
    struct DrawingProgramAppContext *ctx) {
    DrawingProgramOverlayLifecycleState before;
    if (!ctx) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "null app context",
                                    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
    }
    before = ctx->overlay_adapter.lifecycle_state;
    if (before != DRAWING_PROGRAM_OVERLAY_STATE_AUTHORING_ACTIVE || !ctx->overlay_adapter.session_active) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                    "discard requires active authoring session",
                                    before);
    }
    if (!core_layout_cancel_authoring(&ctx->pane_host.layout_state)) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INTERNAL_FAILURE,
                                    "core_layout_cancel_authoring failed",
                                    before);
    }
    ctx->overlay_adapter.pending_apply = 0u;
    ctx->overlay_adapter.session_active = 0u;
    ctx->overlay_adapter.lifecycle_state = DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE;
    if (!drawing_program_adapter_runtime_resume(ctx).ok) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INTERNAL_FAILURE,
                                    "runtime resume failed",
                                    before);
    }
    return adapter_result_ok(before, ctx->overlay_adapter.lifecycle_state);
}

DrawingProgramOverlayAdapterResult drawing_program_adapter_overlay_session_end(
    struct DrawingProgramAppContext *ctx) {
    DrawingProgramOverlayLifecycleState before;
    if (!ctx) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "null app context",
                                    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
    }
    before = ctx->overlay_adapter.lifecycle_state;
    if (before != DRAWING_PROGRAM_OVERLAY_STATE_AUTHORING_ACTIVE || !ctx->overlay_adapter.session_active) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                    "overlay exit requires active authoring session",
                                    before);
    }
    if (ctx->overlay_adapter.pending_apply) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_DIRTY_EXIT_REQUIRES_APPLY_OR_CANCEL,
                                    "dirty exit requires apply or cancel",
                                    before);
    }
    ctx->overlay_adapter.session_active = 0u;
    ctx->overlay_adapter.lifecycle_state = DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE;
    if (!drawing_program_adapter_runtime_resume(ctx).ok) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INTERNAL_FAILURE,
                                    "runtime resume failed",
                                    before);
    }
    return adapter_result_ok(before, ctx->overlay_adapter.lifecycle_state);
}

DrawingProgramOverlayAdapterResult drawing_program_adapter_runtime_tick(
    struct DrawingProgramAppContext *ctx) {
    DrawingProgramOverlayLifecycleState before;
    if (!ctx) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "null app context",
                                    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
    }
    before = ctx->overlay_adapter.lifecycle_state;
    if (ctx->overlay_adapter.lifecycle_state != DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE ||
        ctx->overlay_adapter.runtime_paused) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                    "runtime tick not allowed while overlay is active",
                                    before);
    }
    return adapter_result_ok(before, before);
}

DrawingProgramOverlayAdapterResult drawing_program_adapter_input_route_runtime(
    struct DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "null app context",
                                    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
    }
    if (ctx->overlay_adapter.lifecycle_state != DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                    "runtime input route requires runtime_active",
                                    ctx->overlay_adapter.lifecycle_state);
    }
    if (ctx->overlay_adapter.runtime_paused) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                    "runtime input route not allowed while paused",
                                    ctx->overlay_adapter.lifecycle_state);
    }
    return adapter_result_ok(ctx->overlay_adapter.lifecycle_state, ctx->overlay_adapter.lifecycle_state);
}

DrawingProgramOverlayAdapterResult drawing_program_adapter_input_route_overlay(
    struct DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "null app context",
                                    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
    }
    if (ctx->overlay_adapter.lifecycle_state != DRAWING_PROGRAM_OVERLAY_STATE_AUTHORING_ACTIVE) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                    "overlay input route requires authoring_active",
                                    ctx->overlay_adapter.lifecycle_state);
    }
    return adapter_result_ok(ctx->overlay_adapter.lifecycle_state, ctx->overlay_adapter.lifecycle_state);
}

DrawingProgramOverlayAdapterResult drawing_program_adapter_render_runtime_base(
    struct DrawingProgramAppContext *ctx) {
    CoreResult result;
    if (!ctx) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "null app context",
                                    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
    }
    if (ctx->overlay_adapter.lifecycle_state != DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE ||
        ctx->overlay_adapter.runtime_paused) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                    "runtime base render requires runtime_active and unpaused state",
                                    ctx->overlay_adapter.lifecycle_state);
    }
    result = drawing_program_pane_host_render(ctx);
    if (result.code != CORE_OK) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INTERNAL_FAILURE,
                                    result.message,
                                    ctx->overlay_adapter.lifecycle_state);
    }
    return adapter_result_ok(ctx->overlay_adapter.lifecycle_state, ctx->overlay_adapter.lifecycle_state);
}

DrawingProgramOverlayAdapterResult drawing_program_adapter_render_overlay_chrome(
    struct DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "null app context",
                                    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
    }
    if (ctx->overlay_adapter.lifecycle_state != DRAWING_PROGRAM_OVERLAY_STATE_AUTHORING_ACTIVE) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                    "overlay chrome render requires authoring_active",
                                    ctx->overlay_adapter.lifecycle_state);
    }
    return adapter_result_ok(ctx->overlay_adapter.lifecycle_state, ctx->overlay_adapter.lifecycle_state);
}

DrawingProgramOverlayAdapterResult drawing_program_adapter_persist_save_session(
    struct DrawingProgramAppContext *ctx) {
    CoreResult result;
    DrawingProgramOverlayLifecycleState state;
    if (!ctx) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "null app context",
                                    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
    }
    state = ctx->overlay_adapter.lifecycle_state;
    if (state != DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE || ctx->overlay_adapter.runtime_paused) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                    "session save requires runtime_active and unpaused state",
                                    state);
    }
    result = drawing_program_snapshot_save(ctx, ctx->preset_path);
    if (result.code != CORE_OK) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_IO_FAILURE,
                                    result.message,
                                    state);
    }
    return adapter_result_ok(state, state);
}

DrawingProgramOverlayAdapterResult drawing_program_adapter_persist_export_debug_json(
    struct DrawingProgramAppContext *ctx) {
    CoreResult result;
    DrawingProgramOverlayLifecycleState state;
    if (!ctx) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "null app context",
                                    DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE);
    }
    state = ctx->overlay_adapter.lifecycle_state;
    if (state != DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE || ctx->overlay_adapter.runtime_paused) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_STATE,
                                    "debug export requires runtime_active and unpaused state",
                                    state);
    }
    if (!ctx->export_json_path) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_INVALID_ARGUMENT,
                                    "missing export json path",
                                    state);
    }
    result = drawing_program_snapshot_export_debug_json(ctx, ctx->export_json_path);
    if (result.code != CORE_OK) {
        return adapter_result_error(DRAWING_PROGRAM_OVERLAY_ADAPTER_IO_FAILURE,
                                    result.message,
                                    state);
    }
    return adapter_result_ok(state, state);
}
