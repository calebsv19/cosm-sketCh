#include "drawing_program/drawing_program_render_revision.h"

#include "drawing_program/drawing_program_texture_project.h"

static uint64_t render_revision_mix_u64(uint64_t state, uint64_t value) {
    state ^= value + 0x9e3779b97f4a7c15ull + (state << 6) + (state >> 2);
    if (state == 0u) {
        state = 1u;
    }
    return state;
}

uint64_t drawing_program_render_revision_compose_active_surface_content(
    const DrawingProgramAppContext *ctx) {
    const DrawingProgramTextureSurface *active_surface = 0;
    uint64_t revision = 1469598103934665603ull;
    if (!ctx) {
        return 0u;
    }
    active_surface =
        drawing_program_texture_project_surface_at(&ctx->texture_project, ctx->texture_project.active_surface_index);
    revision = render_revision_mix_u64(revision, active_surface ? active_surface->content_revision : 0u);
    revision = render_revision_mix_u64(revision, ctx->document.content_revision);
    revision = render_revision_mix_u64(revision, ctx->document.logical_width);
    revision = render_revision_mix_u64(revision, ctx->document.logical_height);
    revision = render_revision_mix_u64(revision, ctx->document.sample_density);
    revision = render_revision_mix_u64(revision, ctx->document.layer_count);
    revision = render_revision_mix_u64(revision, ctx->document.raster_width);
    revision = render_revision_mix_u64(revision, ctx->document.raster_height);
    return revision;
}

void drawing_program_render_revision_refresh(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    ctx->runtime.render_active_surface_content_revision =
        drawing_program_render_revision_compose_active_surface_content(ctx);
}

void drawing_program_render_revision_mark_layer_opacity_changed(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    ctx->runtime.render_layer_opacity_revision += 1u;
    if (ctx->runtime.render_layer_opacity_revision == 0u) {
        ctx->runtime.render_layer_opacity_revision = 1u;
    }
}
