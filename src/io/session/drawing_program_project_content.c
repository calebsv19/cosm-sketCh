/* In-process saved-content comparisons. Undo storage, render caches, UI focus
 * and the storage addresses of dynamically allocated buffers have no content meaning. */
#include "drawing_program_project_state_internal.h"

#include "drawing_program/drawing_program_authoring_host.h"

#include <stddef.h>
#include <string.h>

static uint64_t append(uint64_t signature, const void *data, size_t bytes) {
    return (signature ^ core_hash64_fnv1a(data, bytes)) * UINT64_C(1099511628211);
}

static uint64_t document_metadata(uint64_t h, const DrawingProgramDocument *doc) {
    h = append(h, &doc->logical_width, sizeof(doc->logical_width));
    h = append(h, &doc->logical_height, sizeof(doc->logical_height));
    h = append(h, &doc->sample_density, sizeof(doc->sample_density));
    for (uint32_t i = 0; i < doc->layer_count; ++i) {
        const DrawingProgramLayer *layer = &doc->layers[i];
        h = append(h, &layer->layer_id, sizeof(layer->layer_id));
        h = append(h, layer->name, strnlen(layer->name, sizeof(layer->name)));
        h = append(h, &layer->visible, sizeof(layer->visible));
        h = append(h, &layer->locked, sizeof(layer->locked));
    }
    return h;
}

uint64_t drawing_program_project_authoring_signature(const DrawingProgramAppContext *ctx) {
    const DrawingProgramTextureProject *p = &ctx->texture_project;
    DrawingProgramPaneState pane = {0};
    uint64_t h = 0;
    if (drawing_program_authoring_host_export_accepted_pane_state(ctx, &pane.layout, pane.nodes, &pane.node_count,
            &pane.root_index, pane.bindings, &pane.binding_count).code == CORE_OK) {
        h = append(h, pane.nodes, pane.node_count * sizeof(pane.nodes[0]));
        h = append(h, pane.bindings, pane.binding_count * sizeof(pane.bindings[0]));
        h = append(h, &pane.root_index, sizeof(pane.root_index));
    }
    /* The leading scalar fields are the serialized project settings through overlay intent. */
    h = append(h, p, offsetof(DrawingProgramTextureProject, surface_count));
    h = append(h, p->source_scene_id, sizeof(p->source_scene_id));
    h = append(h, p->source_object_id, sizeof(p->source_object_id));
    h = append(h, p->source_scene_path, sizeof(p->source_scene_path));
    h = append(h, ctx->object_store.objects, ctx->object_store.object_count * sizeof(ctx->object_store.objects[0]));
    if (!p->surface_count) h = document_metadata(h, &ctx->document);
    for (uint32_t i = 0; i < p->surface_count; ++i) {
        const DrawingProgramTextureSurface *s = &p->surfaces[i];
        h = append(h, &s->surface_id, sizeof(s->surface_id));
        h = append(h, &s->user_created, sizeof(s->user_created));
        h = append(h, &s->face_role, sizeof(s->face_role));
        h = append(h, &s->quality_preset, sizeof(s->quality_preset));
        h = append(h, &s->layout_offset_x, sizeof(float));
        h = append(h, &s->layout_offset_y, sizeof(float));
        h = append(h, &s->reflection_horizontal, sizeof(s->reflection_horizontal));
        h = append(h, &s->reflection_vertical, sizeof(s->reflection_vertical));
        h = append(h, &s->reflection_center_x, sizeof(s->reflection_center_x));
        h = append(h, &s->reflection_center_y, sizeof(s->reflection_center_y));
        h = append(h, &s->reflection_state, sizeof(s->reflection_state));
        h = append(h, &s->semantic, sizeof(s->semantic));
        h = append(h, s->name, strnlen(s->name, sizeof(s->name)));
        h = append(h, s->layer_role_layer_ids, s->layer_role_entry_count * sizeof(uint32_t));
        h = append(h, s->layer_role_values, s->layer_role_entry_count);
        h = append(h, s->layer_material_intent_layer_ids, s->layer_material_intent_entry_count * sizeof(uint32_t));
        h = append(h, s->layer_material_intent_values, s->layer_material_intent_entry_count);
        if (i == p->active_surface_index) {
            h = document_metadata(h, &ctx->document);
            h = append(h, ctx->ui.layer_opacity_layer_ids, ctx->ui.layer_opacity_entry_count * sizeof(uint32_t));
            h = append(h, ctx->ui.layer_opacity_values, ctx->ui.layer_opacity_entry_count);
        } else if (s->storage) {
            h = document_metadata(h, &s->storage->document);
            h = append(h, s->layer_opacity_layer_ids, s->layer_opacity_entry_count * sizeof(uint32_t));
            h = append(h, s->layer_opacity_values, s->layer_opacity_entry_count);
        }
    }
    if (p->profile_kind == DRAWING_PROGRAM_TEXTURE_PROJECT_PROFILE_INDEXED_ATLAS_V1) {
        h = append(h, &p->indexed_profile, sizeof(p->indexed_profile));
        h = append(h, p->indexed_raster.indices, p->indexed_raster.index_count);
        h = append(h, p->indexed_cells.cells, p->indexed_cells.count * sizeof(p->indexed_cells.cells[0]));
        h = append(h, p->indexed_tile_canvases.canvases,
                   p->indexed_tile_canvases.count * sizeof(p->indexed_tile_canvases.canvases[0]));
    }
    return h;
}

static uint64_t raster_state(uint64_t h, const DrawingProgramDocument *doc,
                             const DrawingProgramLayerRasterStore *layers, int content) {
    if (!content) {
        h = append(h, &doc->content_revision, sizeof(doc->content_revision));
        return append(h, layers->slot_content_revisions, sizeof(layers->slot_content_revisions));
    }
    for (uint32_t i = 0; i < doc->layer_count; ++i) {
        const DrawingProgramRasterSample *samples = NULL;
        uint32_t count = 0;
        if (drawing_program_layer_raster_store_export_layer_or_legacy_base(layers, doc, doc->layers[i].layer_id,
                                                                          &samples, &count).code == CORE_OK)
            h = append(h, samples, (size_t)count * sizeof(*samples));
    }
    return h;
}

uint64_t drawing_program_project_raster_signature(const DrawingProgramAppContext *ctx, int content) {
    const DrawingProgramTextureProject *p = &ctx->texture_project;
    uint64_t h = content ? 0 : ctx->session.control_document_generation;
    if (!p->surface_count) return raster_state(h, &ctx->document, &ctx->layer_rasters, content);
    for (uint32_t i = 0; i < p->surface_count; ++i) {
        const DrawingProgramTextureSurface *s = &p->surfaces[i];
        if (!content) h = append(h, &s->content_revision, sizeof(s->content_revision));
        if (i == p->active_surface_index)
            h = raster_state(h, &ctx->document, &ctx->layer_rasters, content);
        else if (s->storage)
            h = raster_state(h, &s->storage->document, &s->storage->layer_rasters, content);
    }
    return h;
}
