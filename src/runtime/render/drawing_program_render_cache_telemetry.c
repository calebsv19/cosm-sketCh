#include "drawing_program/drawing_program_render_cache_telemetry.h"

#include "drawing_program/drawing_program_app_main.h"

static void drawing_program_render_cache_note_common(
    DrawingProgramAppContext *ctx,
    const DrawingProgramVisualSurfaceCacheTelemetry *telemetry) {
    if (!ctx || !telemetry) {
        return;
    }
    if (telemetry->cache_deferred) {
        ctx->runtime.render_surface_cache_deferred_total += 1u;
    }
    if (telemetry->cache_rebuilt) {
        ctx->runtime.render_surface_cache_rebuild_total += 1u;
    }
    if (telemetry->cache_copy_ready) {
        ctx->runtime.render_surface_cache_copy_total += 1u;
    }
    if (telemetry->cache_unavailable) {
        ctx->runtime.render_surface_cache_unavailable_total += 1u;
    }
    ctx->runtime.render_surface_cache_compose_us_total += (uint64_t)telemetry->cache_compose_us;
    ctx->runtime.render_surface_cache_upload_us_total += (uint64_t)telemetry->cache_upload_us;
    ctx->runtime.render_surface_cache_rebuild_us_total += (uint64_t)telemetry->cache_rebuild_us;
}

void drawing_program_render_cache_note_surface_sync(
    DrawingProgramAppContext *ctx,
    const DrawingProgramVisualSurfaceCacheTelemetry *telemetry,
    uint8_t active_surface) {
    if (!ctx || !telemetry) {
        return;
    }
    if (telemetry->cache_hit) {
        ctx->runtime.render_surface_cache_hit_total += 1u;
        if (active_surface) {
            ctx->runtime.render_surface_cache_active_hit_total += 1u;
        } else {
            ctx->runtime.render_surface_cache_inactive_hit_total += 1u;
        }
    }
    if (telemetry->cache_miss) {
        ctx->runtime.render_surface_cache_miss_total += 1u;
        if (active_surface) {
            ctx->runtime.render_surface_cache_active_miss_total += 1u;
        } else {
            ctx->runtime.render_surface_cache_inactive_miss_total += 1u;
        }
    }
    drawing_program_render_cache_note_common(ctx, telemetry);
}

void drawing_program_render_cache_note_queue_step(
    DrawingProgramAppContext *ctx,
    const DrawingProgramVisualSurfaceCacheTelemetry *telemetry) {
    if (!ctx || !telemetry) {
        return;
    }
    drawing_program_render_cache_note_common(ctx, telemetry);
    ctx->runtime.render_surface_cache_queue_step_total += 1u;
}
