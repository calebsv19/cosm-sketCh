#include "drawing_program/drawing_program_render_composed_source.h"

#include <string.h>

#include "drawing_program_render_composed_source_internal.h"

CoreResult render_composed_source_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

void render_composed_source_mark_full_dirty(
    const struct DrawingProgramDocument *document,
    DrawingProgramRenderComposedSourceView *out_view) {
    if (!document || !out_view || document->raster_width == 0u || document->raster_height == 0u) {
        return;
    }
    out_view->has_dirty_rect = 1u;
    out_view->dirty_rect_is_full = 1u;
    out_view->dirty_x = 0u;
    out_view->dirty_y = 0u;
    out_view->dirty_width = document->raster_width;
    out_view->dirty_height = document->raster_height;
}

uint8_t render_composed_source_diff_dirty_rect(
    const struct DrawingProgramDocument *document,
    const DrawingProgramRasterSample *before_samples,
    const DrawingProgramRasterSample *after_samples,
    uint32_t *out_x,
    uint32_t *out_y,
    uint32_t *out_width,
    uint32_t *out_height) {
    uint32_t x = 0u;
    uint32_t y = 0u;
    uint32_t min_x = 0u;
    uint32_t min_y = 0u;
    uint32_t max_x = 0u;
    uint32_t max_y = 0u;
    uint8_t any_changed = 0u;
    if (!document || !before_samples || !after_samples || !out_x || !out_y || !out_width || !out_height) {
        return 0u;
    }
    for (y = 0u; y < document->raster_height; ++y) {
        size_t row_offset = (size_t)y * (size_t)document->raster_width;
        for (x = 0u; x < document->raster_width; ++x) {
            size_t index = row_offset + x;
            if (before_samples[index] == after_samples[index]) {
                continue;
            }
            if (!any_changed) {
                min_x = x;
                max_x = x;
                min_y = y;
                max_y = y;
                any_changed = 1u;
            } else {
                if (x < min_x) {
                    min_x = x;
                }
                if (x > max_x) {
                    max_x = x;
                }
                if (y < min_y) {
                    min_y = y;
                }
                if (y > max_y) {
                    max_y = y;
                }
            }
        }
    }
    if (!any_changed) {
        return 0u;
    }
    *out_x = min_x;
    *out_y = min_y;
    *out_width = max_x - min_x + 1u;
    *out_height = max_y - min_y + 1u;
    return 1u;
}

void render_composed_source_refresh_snapshot(
    DrawingProgramRenderComposedSourceState *state,
    const struct DrawingProgramDocument *document,
    const DrawingProgramRasterSample *samples,
    DrawingProgramRenderComposedSourceView *out_view) {
    uint32_t x = 0u;
    uint32_t y = 0u;
    uint32_t min_x = 0u;
    uint32_t min_y = 0u;
    uint32_t max_x = 0u;
    uint32_t max_y = 0u;
    uint8_t any_changed = 0u;
    if (!state || !document || !samples || !out_view || document->raster_sample_count == 0u) {
        return;
    }
    if (state->composited_capacity < document->raster_sample_count || !state->resolved_snapshot_samples) {
        render_composed_source_mark_full_dirty(document, out_view);
        if (state->resolved_snapshot_samples) {
            memcpy(state->resolved_snapshot_samples,
                   samples,
                   (size_t)document->raster_sample_count * sizeof(*samples));
        }
        state->snapshot_width = document->raster_width;
        state->snapshot_height = document->raster_height;
        state->has_resolved_snapshot = 1u;
        return;
    }
    if (!state->has_resolved_snapshot ||
        state->snapshot_width != document->raster_width ||
        state->snapshot_height != document->raster_height) {
        render_composed_source_mark_full_dirty(document, out_view);
        memcpy(state->resolved_snapshot_samples,
               samples,
               (size_t)document->raster_sample_count * sizeof(*samples));
        state->snapshot_width = document->raster_width;
        state->snapshot_height = document->raster_height;
        state->has_resolved_snapshot = 1u;
        return;
    }
    for (y = 0u; y < document->raster_height; ++y) {
        size_t row_offset = (size_t)y * (size_t)document->raster_width;
        for (x = 0u; x < document->raster_width; ++x) {
            size_t index = row_offset + x;
            if (state->resolved_snapshot_samples[index] == samples[index]) {
                continue;
            }
            state->resolved_snapshot_samples[index] = samples[index];
            if (!any_changed) {
                min_x = x;
                max_x = x;
                min_y = y;
                max_y = y;
                any_changed = 1u;
            } else {
                if (x < min_x) {
                    min_x = x;
                }
                if (x > max_x) {
                    max_x = x;
                }
                if (y < min_y) {
                    min_y = y;
                }
                if (y > max_y) {
                    max_y = y;
                }
            }
        }
    }
    state->snapshot_width = document->raster_width;
    state->snapshot_height = document->raster_height;
    state->has_resolved_snapshot = 1u;
    if (!any_changed) {
        return;
    }
    out_view->has_dirty_rect = 1u;
    out_view->dirty_rect_is_full = (min_x == 0u &&
                                    min_y == 0u &&
                                    max_x + 1u == document->raster_width &&
                                    max_y + 1u == document->raster_height)
                                       ? 1u
                                       : 0u;
    out_view->dirty_x = min_x;
    out_view->dirty_y = min_y;
    out_view->dirty_width = max_x - min_x + 1u;
    out_view->dirty_height = max_y - min_y + 1u;
}
