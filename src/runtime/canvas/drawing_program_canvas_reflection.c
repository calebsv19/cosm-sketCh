#include "drawing_program/drawing_program_canvas_reflection.h"

#include <string.h>

static DrawingProgramTextureSurface *active_surface_mut(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return 0;
    }
    return drawing_program_texture_project_surface_at_mut(&ctx->texture_project, ctx->texture_project.active_surface_index);
}

static const DrawingProgramTextureSurface *active_surface(const DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return 0;
    }
    return drawing_program_texture_project_surface_at(&ctx->texture_project, ctx->texture_project.active_surface_index);
}

static uint32_t clamp_sample_axis(uint32_t axis, uint32_t limit) {
    if (limit == 0u) {
        return 0u;
    }
    if (axis >= limit) {
        return limit - 1u;
    }
    return axis;
}

static const DrawingProgramReflectionState *active_reflection_state(const DrawingProgramAppContext *ctx) {
    const DrawingProgramTextureSurface *surface = active_surface(ctx);
    if (surface) {
        return &surface->reflection_state;
    }
    return ctx ? &ctx->ui.reflection_state : 0;
}

void drawing_program_canvas_reflection_default_center_for_document(const DrawingProgramDocument *document,
                                                                   uint32_t *out_x,
                                                                   uint32_t *out_y) {
    uint32_t center_x = 0u;
    uint32_t center_y = 0u;
    if (document) {
        if (document->raster_width > 0u) {
            center_x = (document->raster_width - 1u) / 2u;
        }
        if (document->raster_height > 0u) {
            center_y = (document->raster_height - 1u) / 2u;
        }
    }
    if (out_x) {
        *out_x = center_x;
    }
    if (out_y) {
        *out_y = center_y;
    }
}

static void reflection_state_seed_disabled_crosshair(DrawingProgramReflectionState *state,
                                                     uint32_t center_x,
                                                     uint32_t center_y) {
    if (!state) {
        return;
    }
    drawing_program_reflection_state_seed_crosshair(state, center_x, center_y);
    drawing_program_reflection_state_set_crosshair_enabled(state, 0u, 0u);
}

static void reflection_state_seed_from_legacy(DrawingProgramReflectionState *state,
                                              uint32_t center_x,
                                              uint32_t center_y,
                                              uint8_t horizontal_enabled,
                                              uint8_t vertical_enabled) {
    if (!state) {
        return;
    }
    drawing_program_reflection_state_seed_crosshair(state, center_x, center_y);
    drawing_program_reflection_state_set_crosshair_enabled(
        state, horizontal_enabled ? 1u : 0u, vertical_enabled ? 1u : 0u);
}

static void reflection_state_sync_legacy_surface(DrawingProgramTextureSurface *surface) {
    uint8_t horizontal_enabled = 0u;
    uint8_t vertical_enabled = 0u;
    if (!surface) {
        return;
    }
    drawing_program_reflection_state_crosshair_enabled(
        &surface->reflection_state, &horizontal_enabled, &vertical_enabled);
    surface->reflection_center_x = surface->reflection_state.center_x;
    surface->reflection_center_y = surface->reflection_state.center_y;
    surface->reflection_horizontal = horizontal_enabled;
    surface->reflection_vertical = vertical_enabled;
}

static void reflection_state_sync_legacy_ui(DrawingProgramAppUiState *ui) {
    if (!ui) {
        return;
    }
    ui->canvas_reflection_center_valid = ui->reflection_state.center_valid ? 1u : 0u;
    ui->canvas_reflection_center_x = ui->reflection_state.center_x;
    ui->canvas_reflection_center_y = ui->reflection_state.center_y;
}

static void reflection_state_sync_legacy_editor(DrawingProgramEditorState *editor) {
    uint8_t horizontal_enabled = 0u;
    uint8_t vertical_enabled = 0u;
    if (!editor) {
        return;
    }
    drawing_program_reflection_state_crosshair_enabled(
        &editor->reflection_state, &horizontal_enabled, &vertical_enabled);
    editor->symmetry_horizontal = horizontal_enabled;
    editor->symmetry_vertical = vertical_enabled;
}

static void reflection_state_sync_all_from_state(DrawingProgramAppContext *ctx,
                                                 const DrawingProgramReflectionState *state) {
    DrawingProgramTextureSurface *surface = 0;
    if (!ctx || !state) {
        return;
    }
    ctx->editor.reflection_state = *state;
    drawing_program_reflection_state_clamp(
        &ctx->editor.reflection_state, ctx->document.raster_width, ctx->document.raster_height);
    reflection_state_sync_legacy_editor(&ctx->editor);
    ctx->ui.reflection_state = ctx->editor.reflection_state;
    reflection_state_sync_legacy_ui(&ctx->ui);
    surface = active_surface_mut(ctx);
    if (surface) {
        surface->reflection_state = ctx->editor.reflection_state;
        reflection_state_sync_legacy_surface(surface);
    }
}

static void ensure_surface_reflection_state(DrawingProgramTextureSurface *surface,
                                            const DrawingProgramDocument *document) {
    uint32_t center_x = 0u;
    uint32_t center_y = 0u;
    if (!surface) {
        return;
    }
    if (surface->reflection_state.reflector_count == 0u || !surface->reflection_state.center_valid) {
        if (surface->reflection_center_x > 0u ||
            surface->reflection_center_y > 0u ||
            surface->reflection_horizontal ||
            surface->reflection_vertical) {
            reflection_state_seed_from_legacy(&surface->reflection_state,
                                              surface->reflection_center_x,
                                              surface->reflection_center_y,
                                              surface->reflection_horizontal,
                                              surface->reflection_vertical);
        } else {
            drawing_program_canvas_reflection_default_center_for_document(document, &center_x, &center_y);
            reflection_state_seed_disabled_crosshair(&surface->reflection_state, center_x, center_y);
        }
    }
    drawing_program_reflection_state_clamp(
        &surface->reflection_state, document ? document->raster_width : 0u, document ? document->raster_height : 0u);
    reflection_state_sync_legacy_surface(surface);
}

static void ensure_ui_reflection_state(DrawingProgramAppContext *ctx) {
    uint32_t center_x = 0u;
    uint32_t center_y = 0u;
    if (!ctx) {
        return;
    }
    if (ctx->ui.reflection_state.reflector_count == 0u || !ctx->ui.reflection_state.center_valid) {
        if (ctx->ui.canvas_reflection_center_valid) {
            reflection_state_seed_disabled_crosshair(&ctx->ui.reflection_state,
                                                     ctx->ui.canvas_reflection_center_x,
                                                     ctx->ui.canvas_reflection_center_y);
        } else {
            drawing_program_canvas_reflection_default_center_for_document(&ctx->document, &center_x, &center_y);
            reflection_state_seed_disabled_crosshair(&ctx->ui.reflection_state, center_x, center_y);
        }
    }
    drawing_program_reflection_state_clamp(
        &ctx->ui.reflection_state, ctx->document.raster_width, ctx->document.raster_height);
    reflection_state_sync_legacy_ui(&ctx->ui);
}

static void editor_reflection_state_from_legacy(DrawingProgramAppContext *ctx) {
    uint32_t center_x = 0u;
    uint32_t center_y = 0u;
    if (!ctx) {
        return;
    }
    if (!drawing_program_canvas_reflection_active_center(ctx, &center_x, &center_y)) {
        drawing_program_canvas_reflection_default_center_for_document(&ctx->document, &center_x, &center_y);
    }
    reflection_state_seed_from_legacy(&ctx->editor.reflection_state,
                                      center_x,
                                      center_y,
                                      ctx->editor.symmetry_horizontal,
                                      ctx->editor.symmetry_vertical);
    reflection_state_sync_legacy_editor(&ctx->editor);
}

static void reflection_state_merge_editor_crosshair(const DrawingProgramAppContext *ctx,
                                                    DrawingProgramReflectionState *state) {
    if (!ctx || !state) {
        return;
    }
    drawing_program_reflection_state_set_crosshair_enabled(
        state, ctx->editor.symmetry_horizontal ? 1u : 0u, ctx->editor.symmetry_vertical ? 1u : 0u);
}

void drawing_program_canvas_reflection_surface_seed_defaults(DrawingProgramTextureSurface *surface,
                                                             const DrawingProgramDocument *document) {
    uint32_t center_x = 0u;
    uint32_t center_y = 0u;
    if (!surface) {
        return;
    }
    drawing_program_canvas_reflection_default_center_for_document(document, &center_x, &center_y);
    reflection_state_seed_disabled_crosshair(&surface->reflection_state, center_x, center_y);
    reflection_state_sync_legacy_surface(surface);
}

void drawing_program_canvas_reflection_surface_clamp_to_document(DrawingProgramTextureSurface *surface,
                                                                 const DrawingProgramDocument *document) {
    if (!surface) {
        return;
    }
    ensure_surface_reflection_state(surface, document);
}

void drawing_program_canvas_reflection_sync_editor_from_active_surface(DrawingProgramAppContext *ctx) {
    const DrawingProgramTextureSurface *surface = active_surface(ctx);
    if (!ctx) {
        return;
    }
    if (surface) {
        DrawingProgramTextureSurface *surface_mut = active_surface_mut(ctx);
        ensure_surface_reflection_state(surface_mut, &ctx->document);
        ctx->editor.reflection_state = surface_mut->reflection_state;
        ctx->ui.reflection_state = surface_mut->reflection_state;
        reflection_state_sync_legacy_ui(&ctx->ui);
        reflection_state_sync_legacy_editor(&ctx->editor);
        return;
    }
    ensure_ui_reflection_state(ctx);
    ctx->editor.reflection_state = ctx->ui.reflection_state;
    reflection_state_sync_legacy_editor(&ctx->editor);
}

void drawing_program_canvas_reflection_sync_active_surface_from_editor(DrawingProgramAppContext *ctx) {
    DrawingProgramTextureSurface *surface = active_surface_mut(ctx);
    if (!ctx) {
        return;
    }
    if (ctx->editor.reflection_state.reflector_count == 0u || !ctx->editor.reflection_state.center_valid) {
        editor_reflection_state_from_legacy(ctx);
    } else {
        reflection_state_merge_editor_crosshair(ctx, &ctx->editor.reflection_state);
        drawing_program_reflection_state_clamp(
            &ctx->editor.reflection_state, ctx->document.raster_width, ctx->document.raster_height);
        reflection_state_sync_legacy_editor(&ctx->editor);
    }
    if (!surface) {
        ctx->ui.reflection_state = ctx->editor.reflection_state;
        reflection_state_sync_legacy_ui(&ctx->ui);
        return;
    }
    surface->reflection_state = ctx->editor.reflection_state;
    drawing_program_reflection_state_clamp(
        &surface->reflection_state, ctx->document.raster_width, ctx->document.raster_height);
    reflection_state_sync_legacy_surface(surface);
}

int drawing_program_canvas_reflection_active_center(const DrawingProgramAppContext *ctx,
                                                    uint32_t *out_x,
                                                    uint32_t *out_y) {
    const DrawingProgramTextureSurface *surface = active_surface(ctx);
    uint32_t center_x = 0u;
    uint32_t center_y = 0u;
    if (!ctx || ctx->document.raster_width == 0u || ctx->document.raster_height == 0u) {
        return 0;
    }
    if (surface) {
        center_x = clamp_sample_axis(surface->reflection_state.center_x, ctx->document.raster_width);
        center_y = clamp_sample_axis(surface->reflection_state.center_y, ctx->document.raster_height);
    } else {
        center_x = clamp_sample_axis(ctx->ui.reflection_state.center_x, ctx->document.raster_width);
        center_y = clamp_sample_axis(ctx->ui.reflection_state.center_y, ctx->document.raster_height);
    }
    if (out_x) {
        *out_x = center_x;
    }
    if (out_y) {
        *out_y = center_y;
    }
    return 1;
}

void drawing_program_canvas_reflection_reset_active_center(DrawingProgramAppContext *ctx) {
    DrawingProgramReflectionState *state = 0;
    uint32_t center_x = 0u;
    uint32_t center_y = 0u;
    if (!ctx) {
        return;
    }
    drawing_program_canvas_reflection_default_center_for_document(&ctx->document, &center_x, &center_y);
    if (active_surface_mut(ctx)) {
        ensure_surface_reflection_state(active_surface_mut(ctx), &ctx->document);
        state = &active_surface_mut(ctx)->reflection_state;
    } else {
        ensure_ui_reflection_state(ctx);
        state = &ctx->ui.reflection_state;
    }
    drawing_program_reflection_state_set_center(state, center_x, center_y);
    reflection_state_merge_editor_crosshair(ctx, state);
    reflection_state_sync_all_from_state(ctx, state);
}

int drawing_program_canvas_reflection_set_active_center(DrawingProgramAppContext *ctx,
                                                        uint32_t sample_x,
                                                        uint32_t sample_y) {
    DrawingProgramReflectionState *state = 0;
    uint32_t center_x;
    uint32_t center_y;
    if (!ctx || ctx->document.raster_width == 0u || ctx->document.raster_height == 0u) {
        return 0;
    }
    center_x = clamp_sample_axis(sample_x, ctx->document.raster_width);
    center_y = clamp_sample_axis(sample_y, ctx->document.raster_height);
    if (active_surface_mut(ctx)) {
        ensure_surface_reflection_state(active_surface_mut(ctx), &ctx->document);
        state = &active_surface_mut(ctx)->reflection_state;
    } else {
        ensure_ui_reflection_state(ctx);
        state = &ctx->ui.reflection_state;
    }
    drawing_program_reflection_state_set_center(state, center_x, center_y);
    reflection_state_merge_editor_crosshair(ctx, state);
    reflection_state_sync_all_from_state(ctx, state);
    return 1;
}

const DrawingProgramReflectionState *drawing_program_canvas_reflection_active_state(
    const DrawingProgramAppContext *ctx) {
    return active_reflection_state(ctx);
}

int drawing_program_canvas_reflection_set_crosshair_enabled(DrawingProgramAppContext *ctx,
                                                            uint8_t horizontal_enabled,
                                                            uint8_t vertical_enabled) {
    DrawingProgramReflectionState *state = 0;
    if (!ctx) {
        return 0;
    }
    if (active_surface_mut(ctx)) {
        ensure_surface_reflection_state(active_surface_mut(ctx), &ctx->document);
        state = &active_surface_mut(ctx)->reflection_state;
    } else {
        ensure_ui_reflection_state(ctx);
        state = &ctx->ui.reflection_state;
    }
    drawing_program_reflection_state_set_crosshair_enabled(
        state, horizontal_enabled ? 1u : 0u, vertical_enabled ? 1u : 0u);
    reflection_state_sync_all_from_state(ctx, state);
    return 1;
}

int drawing_program_canvas_reflection_add_active_reflector(DrawingProgramAppContext *ctx,
                                                           int32_t direction_dx,
                                                           int32_t direction_dy) {
    DrawingProgramReflectionState *state = 0;
    if (!ctx) {
        return 0;
    }
    if (active_surface_mut(ctx)) {
        ensure_surface_reflection_state(active_surface_mut(ctx), &ctx->document);
        state = &active_surface_mut(ctx)->reflection_state;
    } else {
        ensure_ui_reflection_state(ctx);
        state = &ctx->ui.reflection_state;
    }
    if (!drawing_program_reflection_state_add_reflector(state, direction_dx, direction_dy)) {
        return 0;
    }
    reflection_state_sync_all_from_state(ctx, state);
    return 1;
}

int drawing_program_canvas_reflection_cycle_active_reflector(DrawingProgramAppContext *ctx, int delta) {
    DrawingProgramReflectionState *state = 0;
    if (!ctx) {
        return 0;
    }
    if (active_surface_mut(ctx)) {
        ensure_surface_reflection_state(active_surface_mut(ctx), &ctx->document);
        state = &active_surface_mut(ctx)->reflection_state;
    } else {
        ensure_ui_reflection_state(ctx);
        state = &ctx->ui.reflection_state;
    }
    if (!drawing_program_reflection_state_cycle_active_reflector(state, delta)) {
        return 0;
    }
    reflection_state_sync_all_from_state(ctx, state);
    return 1;
}

int drawing_program_canvas_reflection_toggle_active_reflector_enabled(DrawingProgramAppContext *ctx) {
    DrawingProgramReflectionState *state = 0;
    if (!ctx) {
        return 0;
    }
    if (active_surface_mut(ctx)) {
        ensure_surface_reflection_state(active_surface_mut(ctx), &ctx->document);
        state = &active_surface_mut(ctx)->reflection_state;
    } else {
        ensure_ui_reflection_state(ctx);
        state = &ctx->ui.reflection_state;
    }
    if (!drawing_program_reflection_state_toggle_active_reflector_enabled(state)) {
        return 0;
    }
    reflection_state_sync_all_from_state(ctx, state);
    return 1;
}

int drawing_program_canvas_reflection_delete_active_reflector(DrawingProgramAppContext *ctx) {
    DrawingProgramReflectionState *state = 0;
    if (!ctx) {
        return 0;
    }
    if (active_surface_mut(ctx)) {
        ensure_surface_reflection_state(active_surface_mut(ctx), &ctx->document);
        state = &active_surface_mut(ctx)->reflection_state;
    } else {
        ensure_ui_reflection_state(ctx);
        state = &ctx->ui.reflection_state;
    }
    if (!drawing_program_reflection_state_delete_active_reflector(state)) {
        return 0;
    }
    reflection_state_sync_all_from_state(ctx, state);
    return 1;
}

int drawing_program_canvas_reflection_set_active_reflector_direction(DrawingProgramAppContext *ctx,
                                                                     int32_t direction_dx,
                                                                     int32_t direction_dy) {
    DrawingProgramReflectionState *state = 0;
    if (!ctx) {
        return 0;
    }
    if (active_surface_mut(ctx)) {
        ensure_surface_reflection_state(active_surface_mut(ctx), &ctx->document);
        state = &active_surface_mut(ctx)->reflection_state;
    } else {
        ensure_ui_reflection_state(ctx);
        state = &ctx->ui.reflection_state;
    }
    if (!drawing_program_reflection_state_set_active_reflector_direction(state, direction_dx, direction_dy)) {
        return 0;
    }
    reflection_state_sync_all_from_state(ctx, state);
    return 1;
}

int drawing_program_canvas_reflection_enabled(const DrawingProgramAppContext *ctx) {
    const DrawingProgramReflectionState *state = 0;
    uint32_t i;
    if (!ctx) {
        return 0;
    }
    state = &ctx->editor.reflection_state;
    for (i = 0u; state && i < state->reflector_count && i < DRAWING_PROGRAM_REFLECTION_REFLECTOR_CAPACITY; ++i) {
        if (state->reflectors[i].enabled) {
            return 1;
        }
    }
    return (ctx->editor.symmetry_horizontal || ctx->editor.symmetry_vertical) ? 1 : 0;
}

uint32_t drawing_program_canvas_reflection_collect_points(
    const DrawingProgramAppContext *ctx,
    int32_t sample_x,
    int32_t sample_y,
    DrawingProgramCanvasReflectionPoint out_points[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY]) {
    int32_t xs[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY];
    int32_t ys[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY];
    uint32_t count;
    uint32_t i;
    if (!out_points || !ctx) {
        return 0u;
    }
    memset(out_points, 0, sizeof(*out_points) * DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY);
    count = drawing_program_reflection_state_collect_point_variants(&ctx->editor.reflection_state,
                                                                    sample_x,
                                                                    sample_y,
                                                                    xs,
                                                                    ys,
                                                                    DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY);
    for (i = 0u; i < count; ++i) {
        out_points[i].x = xs[i];
        out_points[i].y = ys[i];
    }
    return count;
}

uint32_t drawing_program_canvas_reflection_collect_segments(
    const DrawingProgramAppContext *ctx,
    int32_t start_x,
    int32_t start_y,
    int32_t end_x,
    int32_t end_y,
    DrawingProgramCanvasReflectionSegment out_segments[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY]) {
    int32_t start_xs[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY];
    int32_t start_ys[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY];
    int32_t end_xs[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY];
    int32_t end_ys[DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY];
    uint32_t count;
    uint32_t i;
    if (!out_segments || !ctx) {
        return 0u;
    }
    memset(out_segments, 0, sizeof(*out_segments) * DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY);
    count = drawing_program_reflection_state_collect_segment_variants(&ctx->editor.reflection_state,
                                                                      start_x,
                                                                      start_y,
                                                                      end_x,
                                                                      end_y,
                                                                      start_xs,
                                                                      start_ys,
                                                                      end_xs,
                                                                      end_ys,
                                                                      DRAWING_PROGRAM_CANVAS_REFLECTION_VARIANT_CAPACITY);
    for (i = 0u; i < count; ++i) {
        out_segments[i].start_x = start_xs[i];
        out_segments[i].start_y = start_ys[i];
        out_segments[i].end_x = end_xs[i];
        out_segments[i].end_y = end_ys[i];
    }
    return count;
}
