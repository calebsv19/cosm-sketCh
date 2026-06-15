#include "drawing_program_texture_project_snapshot_internal.h"

#include <stdlib.h>
#include <string.h>

CoreResult texture_project_snapshot_write_surface_layer_chunk(
    CorePackWriter *writer,
    const DrawingProgramTextureSurfaceStorage *storage) {
    uint64_t header_bytes;
    uint64_t layer_bytes;
    uint64_t payload_size;
    uint8_t *payload = 0;
    uint8_t *cursor = 0;
    uint32_t i;
    const DrawingProgramDocument *document = 0;
    const DrawingProgramLayerRasterStore *layer_rasters = 0;
    uint32_t sample_count = 0u;
    uint64_t sample_bytes = 0u;
    if (!writer || !storage) {
        return texture_project_snapshot_invalid("invalid texture surface layer chunk write request");
    }
    document = &storage->document;
    layer_rasters = &storage->layer_rasters;
    if (!texture_project_document_bounds_valid(document)) {
        return texture_project_snapshot_invalid("invalid texture surface document while writing layers");
    }
    sample_count = document->raster_sample_count;
    sample_bytes = (uint64_t)sample_count * (uint64_t)sizeof(DrawingProgramRasterSample);
    header_bytes = (uint64_t)(sizeof(uint32_t) * 5u);
    layer_bytes = (uint64_t)(sizeof(uint32_t) * 2u) + sample_bytes;
    payload_size = header_bytes + (layer_bytes * (uint64_t)document->layer_count);
    payload = (uint8_t *)malloc((size_t)payload_size);
    if (!payload) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate texture surface layer payload" };
    }
    cursor = payload;
    memcpy(cursor, &(uint32_t){ DRAWING_PROGRAM_TEXTURE_SURFACE_LAYER_CHUNK_VERSION_V2 }, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &document->raster_width, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &document->raster_height, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &document->raster_sample_count, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &document->layer_count, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    for (i = 0u; i < document->layer_count; ++i) {
        const uint32_t layer_id = document->layers[i].layer_id;
        const DrawingProgramRasterSample *samples = 0;
        uint32_t exported_sample_count = 0u;
        CoreResult export_result = { CORE_ERR_NOT_FOUND, "texture layer raster export unavailable" };
        memcpy(cursor, &layer_id, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        memcpy(cursor, &sample_count, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        export_result = drawing_program_layer_raster_store_export_layer_or_legacy_base(layer_rasters,
                                                                                       document,
                                                                                       layer_id,
                                                                                       &samples,
                                                                                       &exported_sample_count);
        if (export_result.code == CORE_OK &&
            samples &&
            exported_sample_count == sample_count) {
            memcpy(cursor, samples, (size_t)sample_bytes);
        } else {
            uint32_t sample_i;
            DrawingProgramRasterSample *write_samples = (DrawingProgramRasterSample *)cursor;
            DrawingProgramRasterSample empty_sample = drawing_program_color_eraser_value();
            for (sample_i = 0u; sample_i < sample_count; ++sample_i) {
                write_samples[sample_i] = empty_sample;
            }
        }
        cursor += sample_bytes;
    }
    {
        CoreResult result = core_pack_writer_add_chunk(writer, "DTSR", payload, payload_size);
        free(payload);
        return result;
    }
}

CoreResult drawing_program_texture_project_snapshot_write(
    CorePackWriter *writer,
    const DrawingProgramTextureProject *project) {
    DrawingProgramTextureProjectChunkHeaderV12 header;
    DrawingProgramTextureProjectSurfaceRecordV9 *records = 0;
    CoreResult result;
    uint32_t i;
    if (!writer || !project || project->surface_count == 0u) {
        return texture_project_snapshot_invalid("invalid texture project snapshot write request");
    }
    memset(&header, 0, sizeof(header));
    header.version = DRAWING_PROGRAM_TEXTURE_PROJECT_CHUNK_VERSION_V12;
    header.primitive_kind = project->primitive_kind;
    header.net_layout_kind = project->net_layout_kind;
    header.quality_preset = project->quality_preset;
    header.export_binding_kind = project->export_binding_kind;
    header.export_intent_kind = project->export_intent_kind;
    header.overlay_material_intent_kind = project->overlay_material_intent_kind;
    header.surface_count = project->surface_count;
    header.active_surface_index = project->active_surface_index;
    header.next_surface_id = project->next_surface_id;
    memcpy(header.source_scene_id, project->source_scene_id, sizeof(header.source_scene_id));
    memcpy(header.source_object_id, project->source_object_id, sizeof(header.source_object_id));
    memcpy(header.source_scene_path, project->source_scene_path, sizeof(header.source_scene_path));
    records = (DrawingProgramTextureProjectSurfaceRecordV9 *)calloc(project->surface_count, sizeof(*records));
    if (!records) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate texture project surface records" };
    }
    for (i = 0u; i < project->surface_count; ++i) {
        const DrawingProgramTextureSurface *surface = drawing_program_texture_project_surface_at(project, i);
        const DrawingProgramDocument *document = 0;
        DrawingProgramTextureSurfaceDocumentChunkV1 document_chunk;
        if (!surface || !surface->storage) {
            free(records);
            return (CoreResult){ CORE_ERR_FORMAT, "texture project surface missing storage" };
        }
        document = &surface->storage->document;
        if (!texture_project_document_bounds_valid(document)) {
            free(records);
            return (CoreResult){ CORE_ERR_FORMAT, "texture project surface document invalid" };
        }
        records[i].surface_id = surface->surface_id;
        records[i].face_role = surface->face_role;
        records[i].quality_preset = surface->quality_preset;
        records[i].logical_width = document->logical_width;
        records[i].logical_height = document->logical_height;
        records[i].sample_density = document->sample_density;
        records[i].layout_offset_x = surface->layout_offset_x;
        records[i].layout_offset_y = surface->layout_offset_y;
        records[i].reflection_state = surface->reflection_state;
        records[i].reflection_center_x = surface->reflection_center_x;
        records[i].reflection_center_y = surface->reflection_center_y;
        records[i].is_blank = surface->is_blank;
        records[i].resize_locked = surface->resize_locked;
        records[i].user_created = surface->user_created;
        records[i].reflection_horizontal = surface->reflection_horizontal;
        records[i].reflection_vertical = surface->reflection_vertical;
        records[i].layer_opacity_entry_count = surface->layer_opacity_entry_count;
        records[i].layer_role_entry_count = surface->layer_role_entry_count;
        records[i].layer_material_intent_entry_count = surface->layer_material_intent_entry_count;
        records[i].net_layout_kind = surface->semantic.layout_kind;
        records[i].net_slot = surface->semantic.net_slot;
        records[i].orientation = surface->semantic.orientation;
        memcpy(records[i].corner_ids, surface->semantic.corner_ids, sizeof(records[i].corner_ids));
        memcpy(records[i].edge_ids, surface->semantic.edge_ids, sizeof(records[i].edge_ids));
        memcpy(records[i].adjacent_face_roles,
               surface->semantic.adjacent_face_roles,
               sizeof(records[i].adjacent_face_roles));
        memcpy(records[i].name, surface->name, sizeof(records[i].name));
        memcpy(records[i].layer_opacity_values,
               surface->layer_opacity_values,
               sizeof(records[i].layer_opacity_values));
        memcpy(records[i].layer_opacity_layer_ids,
               surface->layer_opacity_layer_ids,
               sizeof(records[i].layer_opacity_layer_ids));
        memcpy(records[i].layer_role_values,
               surface->layer_role_values,
               sizeof(records[i].layer_role_values));
        memcpy(records[i].layer_role_layer_ids,
               surface->layer_role_layer_ids,
               sizeof(records[i].layer_role_layer_ids));
        memcpy(records[i].layer_material_intent_values,
               surface->layer_material_intent_values,
               sizeof(records[i].layer_material_intent_values));
        memcpy(records[i].layer_material_intent_layer_ids,
               surface->layer_material_intent_layer_ids,
               sizeof(records[i].layer_material_intent_layer_ids));
        memset(&document_chunk, 0, sizeof(document_chunk));
        document_chunk.version = DRAWING_PROGRAM_TEXTURE_SURFACE_DOCUMENT_CHUNK_VERSION_V1;
        document_chunk.document = *document;
        result = core_pack_writer_add_chunk(writer, "DTSD", &document_chunk, (uint64_t)sizeof(document_chunk));
        if (result.code != CORE_OK) {
            free(records);
            return result;
        }
        result = texture_project_snapshot_write_surface_layer_chunk(writer, surface->storage);
        if (result.code != CORE_OK) {
            free(records);
            return result;
        }
    }
    {
        const uint64_t payload_size =
            (uint64_t)sizeof(header) + ((uint64_t)project->surface_count * (uint64_t)sizeof(*records));
        uint8_t *payload = (uint8_t *)malloc((size_t)payload_size);
        if (!payload) {
            free(records);
            return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate texture project root payload" };
        }
        memcpy(payload, &header, sizeof(header));
        memcpy(payload + sizeof(header), records, (size_t)project->surface_count * sizeof(*records));
        result = core_pack_writer_add_chunk(writer, "DPTP", payload, payload_size);
        free(payload);
    }
    free(records);
    return result;
}
