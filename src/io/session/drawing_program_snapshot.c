#include "drawing_program/drawing_program_snapshot.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "core_pack.h"
#include "drawing_program/drawing_program_app_post_load.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_snapshot_shell.h"
#include "drawing_program/drawing_program_snapshot_ui_settings.h"

typedef struct DrawingProgramSnapshotHeaderV1 {
    uint32_t version;
    uint32_t reserved0;
    uint32_t node_count;
    uint32_t binding_count;
    uint32_t history_count;
    uint32_t history_cursor;
} DrawingProgramSnapshotHeaderV1;

typedef struct DrawingProgramSnapshotV1 {
    DrawingProgramSnapshotHeaderV1 header;
    DrawingProgramDocument document;
    DrawingProgramEditorState editor;
    CoreLayoutState layout_state;
    CorePaneNode nodes[DRAWING_PROGRAM_PANE_NODE_CAPACITY];
    CorePaneModuleBinding bindings[DRAWING_PROGRAM_MODULE_BINDING_CAPACITY];
    DrawingProgramCommand history_entries[DRAWING_PROGRAM_HISTORY_CAPACITY];
} DrawingProgramSnapshotV1;

typedef struct DrawingProgramObjectChunkHeaderV1 {
    uint32_t version;
    uint32_t object_count;
    uint32_t next_object_id;
    uint32_t reserved0;
} DrawingProgramObjectChunkHeaderV1;

typedef struct DrawingProgramObjectChunkEntryV1 {
    uint32_t object_id;
    uint32_t layer_id;
    uint8_t type;
    uint8_t visible;
    uint8_t locked;
    uint8_t stroke_color_index;
    uint8_t fill_color_index;
    uint8_t stroke_width;
    uint8_t style_mode;
    uint8_t reserved0;
    uint8_t reserved1;
    int32_t origin_x;
    int32_t origin_y;
    uint32_t width;
    uint32_t height;
    char name[DRAWING_PROGRAM_OBJECT_NAME_CAPACITY];
} DrawingProgramObjectChunkEntryV1;

typedef struct DrawingProgramObjectChunkHeaderV2 {
    uint32_t version;
    uint32_t object_count;
    uint32_t next_object_id;
    uint32_t reserved0;
} DrawingProgramObjectChunkHeaderV2;

typedef struct DrawingProgramObjectChunkEntryV2 {
    uint32_t object_id;
    uint32_t layer_id;
    uint8_t type;
    uint8_t visible;
    uint8_t locked;
    uint8_t stroke_color_index;
    uint8_t fill_color_index;
    uint8_t stroke_width;
    uint8_t style_mode;
    uint8_t path_closed;
    uint16_t path_point_count;
    int32_t origin_x;
    int32_t origin_y;
    uint32_t width;
    uint32_t height;
    char name[DRAWING_PROGRAM_OBJECT_NAME_CAPACITY];
    struct {
        int32_t x;
        int32_t y;
    } path_points[DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS];
} DrawingProgramObjectChunkEntryV2;

typedef struct DrawingProgramObjectChunkHeaderV3 {
    uint32_t version;
    uint32_t object_count;
    uint32_t next_object_id;
    uint32_t reserved0;
} DrawingProgramObjectChunkHeaderV3;

typedef struct DrawingProgramObjectChunkEntryV3 {
    uint32_t object_id;
    uint32_t layer_id;
    uint8_t type;
    uint8_t visible;
    uint8_t locked;
    uint8_t stroke_color_index;
    uint8_t fill_color_index;
    uint8_t stroke_width;
    uint8_t style_mode;
    uint8_t path_closed;
    uint16_t path_point_count;
    int32_t origin_x;
    int32_t origin_y;
    uint32_t width;
    uint32_t height;
    char name[DRAWING_PROGRAM_OBJECT_NAME_CAPACITY];
    DrawingProgramPathPoint path_points[DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS];
} DrawingProgramObjectChunkEntryV3;

typedef struct DrawingProgramObjectChunkHeaderV4 {
    uint32_t version;
    uint32_t object_count;
    uint32_t next_object_id;
    uint32_t reserved0;
} DrawingProgramObjectChunkHeaderV4;

typedef struct DrawingProgramObjectChunkEntryV4 {
    uint32_t object_id;
    uint32_t layer_id;
    uint8_t type;
    uint8_t visible;
    uint8_t locked;
    uint8_t stroke_width;
    uint8_t style_mode;
    uint8_t path_closed;
    uint16_t path_point_count;
    uint16_t reserved0;
    DrawingProgramRasterSample stroke_color_value;
    DrawingProgramRasterSample fill_color_value;
    int32_t origin_x;
    int32_t origin_y;
    uint32_t width;
    uint32_t height;
    char name[DRAWING_PROGRAM_OBJECT_NAME_CAPACITY];
    DrawingProgramPathPoint path_points[DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS];
} DrawingProgramObjectChunkEntryV4;

enum {
    DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V1 = 1u,
    DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V2 = 2u,
    DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V1 = 1u,
    DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V2 = 2u,
    DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V3 = 3u,
    DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V4 = 4u
};

typedef struct DrawingProgramLegacyLayerChunkSeed {
    uint32_t raster_width;
    uint32_t raster_height;
    uint32_t sample_count;
    uint32_t layer_count;
    uint32_t layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t max_layer_id;
} DrawingProgramLegacyLayerChunkSeed;

static CoreResult snapshot_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static int snapshot_trace_enabled(void) {
    const char *value = getenv("DRAWING_PROGRAM_TRACE_UI_STATE");
    if (!value || value[0] == '\0' || value[0] == '0') {
        return 0;
    }
    return 1;
}

static int drawing_program_snapshot_document_has_layer_id(const DrawingProgramDocument *document, uint32_t layer_id) {
    uint32_t i;
    if (!document || layer_id == 0u) {
        return 0;
    }
    for (i = 0u; i < document->layer_count; ++i) {
        if (document->layers[i].layer_id == layer_id) {
            return 1;
        }
    }
    return 0;
}

static CoreResult drawing_program_snapshot_inspect_layer_raster_chunk(
    const void *chunk_data,
    uint64_t chunk_size,
    DrawingProgramLegacyLayerChunkSeed *out_seed) {
    const uint8_t *cursor = (const uint8_t *)chunk_data;
    const uint8_t *end = cursor + chunk_size;
    uint32_t version = 0u;
    uint32_t i;
    if (!cursor || !out_seed || chunk_size < (uint64_t)(sizeof(uint32_t) * 5u)) {
        return snapshot_invalid("invalid legacy layer chunk inspect request");
    }
    memset(out_seed, 0, sizeof(*out_seed));
    memcpy(&version, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&out_seed->raster_width, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&out_seed->raster_height, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&out_seed->sample_count, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&out_seed->layer_count, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    if (version != DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V1 &&
        version != DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V2) {
        return (CoreResult){ CORE_ERR_FORMAT, "unsupported drawing layer raster chunk version" };
    }
    if (out_seed->raster_width == 0u ||
        out_seed->raster_height == 0u ||
        out_seed->sample_count == 0u ||
        out_seed->sample_count != (out_seed->raster_width * out_seed->raster_height) ||
        out_seed->layer_count == 0u ||
        out_seed->layer_count > DRAWING_PROGRAM_MAX_LAYERS) {
        return (CoreResult){ CORE_ERR_FORMAT, "invalid legacy layer chunk bounds" };
    }
    for (i = 0u; i < out_seed->layer_count; ++i) {
        uint32_t layer_id = 0u;
        uint32_t entry_sample_count = 0u;
        uint64_t sample_bytes = 0u;
        if ((uint64_t)(end - cursor) < (uint64_t)(sizeof(uint32_t) * 2u)) {
            return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster chunk truncated header" };
        }
        memcpy(&layer_id, cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        memcpy(&entry_sample_count, cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        if (layer_id == 0u || entry_sample_count != out_seed->sample_count) {
            return (CoreResult){ CORE_ERR_FORMAT, "invalid legacy layer chunk entry" };
        }
        out_seed->layer_ids[i] = layer_id;
        if (layer_id > out_seed->max_layer_id) {
            out_seed->max_layer_id = layer_id;
        }
        sample_bytes = (version == DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V2)
                           ? ((uint64_t)entry_sample_count * (uint64_t)sizeof(DrawingProgramRasterSample))
                           : (uint64_t)entry_sample_count;
        if ((uint64_t)(end - cursor) < sample_bytes) {
            return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster chunk truncated samples" };
        }
        cursor += sample_bytes;
    }
    if (cursor != end) {
        return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster chunk trailing bytes" };
    }
    return core_result_ok();
}

static CoreResult drawing_program_snapshot_seed_document_from_legacy_layer_chunk(
    struct DrawingProgramAppContext *ctx,
    const void *chunk_data,
    uint64_t chunk_size) {
    DrawingProgramLegacyLayerChunkSeed seed;
    CoreResult result;
    uint32_t i;
    if (!ctx || !chunk_data) {
        return snapshot_invalid("invalid legacy layer chunk bootstrap request");
    }
    result = drawing_program_snapshot_inspect_layer_raster_chunk(chunk_data, chunk_size, &seed);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_document_init_with_shape(&ctx->document, seed.raster_width, seed.raster_height, 1u);
    if (result.code != CORE_OK) {
        return result;
    }
    ctx->document.layer_count = seed.layer_count;
    ctx->document.next_layer_id = seed.max_layer_id + 1u;
    if (ctx->document.next_layer_id == 0u) {
        ctx->document.next_layer_id = 1u;
    }
    memset(ctx->document.layers, 0, sizeof(ctx->document.layers));
    for (i = 0u; i < seed.layer_count; ++i) {
        ctx->document.layers[i].layer_id = seed.layer_ids[i];
        ctx->document.layers[i].visible = 1u;
        ctx->document.layers[i].locked = 0u;
        if (i == 0u) {
            (void)snprintf(ctx->document.layers[i].name,
                           sizeof(ctx->document.layers[i].name),
                           "%s",
                           "Base Layer");
        } else {
            (void)snprintf(ctx->document.layers[i].name,
                           sizeof(ctx->document.layers[i].name),
                           "Layer %u",
                           (unsigned)ctx->document.layers[i].layer_id);
        }
    }
    drawing_program_editor_state_init(&ctx->editor, &ctx->document);
    drawing_program_history_clear(&ctx->history);
    drawing_program_selection_reset(&ctx->selection);
    drawing_program_object_selection_reset(&ctx->object_selection);
    return core_result_ok();
}

static CoreResult drawing_program_snapshot_load_legacy_chunked_fallback(
    struct DrawingProgramAppContext *ctx,
    CorePackReader *reader) {
    CorePackChunkInfo layer_chunk;
    CoreResult result;
    uint8_t *layer_chunk_data = 0;
    if (!ctx || !reader) {
        return snapshot_invalid("invalid legacy snapshot fallback load request");
    }
    memset(&layer_chunk, 0, sizeof(layer_chunk));
    result = core_pack_reader_find_chunk(reader, "DPLR", 0u, &layer_chunk);
    if (result.code != CORE_OK) {
        return result;
    }
    layer_chunk_data = (uint8_t *)malloc((size_t)layer_chunk.size);
    if (!layer_chunk_data) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate legacy layer raster chunk buffer" };
    }
    result = core_pack_reader_read_chunk_data(reader, &layer_chunk, layer_chunk_data, layer_chunk.size);
    if (result.code == CORE_OK) {
        result = drawing_program_snapshot_seed_document_from_legacy_layer_chunk(ctx, layer_chunk_data, layer_chunk.size);
    }
    free(layer_chunk_data);
    return result;
}

static CoreResult drawing_program_snapshot_upgrade_legacy_palette_index_samples(
    struct DrawingProgramAppContext *ctx,
    uint8_t *out_upgraded) {
    uint8_t upgraded = 0u;
    uint32_t i;
    if (!ctx) {
        return snapshot_invalid("invalid palette sample upgrade request");
    }
    if (ctx->document.schema_version >= DRAWING_PROGRAM_DOCUMENT_SCHEMA_VERSION_TRUE_COLOR) {
        if (out_upgraded) {
            *out_upgraded = 0u;
        }
        return core_result_ok();
    }
    for (i = 0u; i < ctx->document.raster_sample_count; ++i) {
        DrawingProgramRasterSample normalized = drawing_program_color_normalize_legacy_sample(
            (uint8_t)(ctx->document.raster_samples[i] & 0xffu));
        if (normalized != ctx->document.raster_samples[i]) {
            ctx->document.raster_samples[i] = normalized;
            upgraded = 1u;
        }
    }
    if (ctx->layer_rasters.slot_samples &&
        ctx->layer_rasters.sample_count == ctx->document.raster_sample_count) {
        uint32_t slot;
        for (slot = 0u; slot < ctx->layer_rasters.slot_capacity; ++slot) {
            DrawingProgramRasterSample *slot_samples;
            if (ctx->layer_rasters.slot_layer_ids[slot] == 0u) {
                continue;
            }
            slot_samples = ctx->layer_rasters.slot_samples +
                           ((size_t)slot * (size_t)ctx->layer_rasters.sample_count);
            for (i = 0u; i < ctx->layer_rasters.sample_count; ++i) {
                DrawingProgramRasterSample normalized = drawing_program_color_normalize_legacy_sample(
                    (uint8_t)(slot_samples[i] & 0xffu));
                if (normalized != slot_samples[i]) {
                    slot_samples[i] = normalized;
                    upgraded = 1u;
                }
            }
        }
    }
    if (ctx->document.schema_version != DRAWING_PROGRAM_DOCUMENT_SCHEMA_VERSION_TRUE_COLOR) {
        ctx->document.schema_version = DRAWING_PROGRAM_DOCUMENT_SCHEMA_VERSION_TRUE_COLOR;
        upgraded = 1u;
    }
    if (out_upgraded) {
        *out_upgraded = upgraded;
    }
    return core_result_ok();
}

static CoreResult drawing_program_snapshot_write_layer_raster_chunk(
    CorePackWriter *writer,
    const struct DrawingProgramAppContext *ctx) {
    uint64_t header_bytes;
    uint64_t layer_bytes;
    uint64_t payload_size;
    uint8_t *payload = 0;
    uint8_t *cursor = 0;
    uint32_t i;
    const uint32_t sample_count = ctx->document.raster_sample_count;
    const uint64_t sample_bytes = (uint64_t)sample_count * (uint64_t)sizeof(DrawingProgramRasterSample);
    if (!writer || !ctx) {
        return snapshot_invalid("invalid layer raster chunk write request");
    }
    header_bytes = (uint64_t)(sizeof(uint32_t) * 5u);
    layer_bytes = (uint64_t)(sizeof(uint32_t) * 2u) + sample_bytes;
    if (sample_count == 0u || ctx->document.layer_count == 0u) {
        return core_result_ok();
    }
    payload_size = header_bytes + (layer_bytes * (uint64_t)ctx->document.layer_count);
    payload = (uint8_t *)malloc((size_t)payload_size);
    if (!payload) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate layer raster snapshot payload" };
    }
    cursor = payload;
    memcpy(cursor, &(uint32_t){ DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V2 }, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &ctx->document.raster_width, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &ctx->document.raster_height, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &ctx->document.raster_sample_count, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &ctx->document.layer_count, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    for (i = 0u; i < ctx->document.layer_count; ++i) {
        const uint32_t layer_id = ctx->document.layers[i].layer_id;
        const DrawingProgramRasterSample *samples = 0;
        uint32_t exported_sample_count = 0u;
        CoreResult export_result = { CORE_ERR_NOT_FOUND, "layer raster export unavailable" };
        memcpy(cursor, &layer_id, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        memcpy(cursor, &sample_count, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        if (i == 0u) {
            memcpy(cursor, ctx->document.raster_samples, (size_t)sample_bytes);
            cursor += sample_bytes;
            continue;
        }
        export_result = drawing_program_layer_raster_store_export_layer(&ctx->layer_rasters,
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
        CoreResult result = core_pack_writer_add_chunk(writer, "DPLR", payload, payload_size);
        free(payload);
        return result;
    }
}

static CoreResult drawing_program_snapshot_apply_layer_raster_chunk(
    struct DrawingProgramAppContext *ctx,
    const void *chunk_data,
    uint64_t chunk_size) {
    const uint8_t *cursor = (const uint8_t *)chunk_data;
    const uint8_t *end = cursor + chunk_size;
    uint32_t version = 0u;
    uint32_t raster_width = 0u;
    uint32_t raster_height = 0u;
    uint32_t sample_count = 0u;
    uint32_t layer_count = 0u;
    uint32_t i;
    if (!ctx || !cursor || chunk_size < (uint64_t)(sizeof(uint32_t) * 5u)) {
        return snapshot_invalid("invalid layer raster chunk payload");
    }
    memcpy(&version, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&raster_width, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&raster_height, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&sample_count, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&layer_count, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    if (version != DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V1 &&
        version != DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V2) {
        return (CoreResult){ CORE_ERR_FORMAT, "unsupported drawing layer raster chunk version" };
    }
    if (raster_width != ctx->document.raster_width ||
        raster_height != ctx->document.raster_height ||
        sample_count != ctx->document.raster_sample_count ||
        layer_count > DRAWING_PROGRAM_MAX_LAYERS) {
        return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster chunk shape mismatch" };
    }
    for (i = 0u; i < layer_count; ++i) {
        uint32_t layer_id = 0u;
        uint32_t entry_sample_count = 0u;
        if ((uint64_t)(end - cursor) < (uint64_t)(sizeof(uint32_t) * 2u)) {
            return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster chunk truncated header" };
        }
        memcpy(&layer_id, cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        memcpy(&entry_sample_count, cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        if (entry_sample_count != sample_count) {
            return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster entry sample count mismatch" };
        }
        if ((uint64_t)(end - cursor) <
            ((version == DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V2)
                 ? ((uint64_t)entry_sample_count * (uint64_t)sizeof(DrawingProgramRasterSample))
                 : (uint64_t)entry_sample_count)) {
            return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster chunk truncated samples" };
        }
        if (drawing_program_snapshot_document_has_layer_id(&ctx->document, layer_id)) {
            CoreResult import_result;
            const uint32_t base_layer_id =
                (ctx->document.layer_count > 0u) ? ctx->document.layers[0].layer_id : 0u;
            if (version == DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V2) {
                import_result = drawing_program_layer_raster_store_import_layer(
                    &ctx->layer_rasters,
                    layer_id,
                    (const DrawingProgramRasterSample *)cursor,
                    entry_sample_count);
                if (import_result.code == CORE_OK && layer_id == base_layer_id) {
                    memcpy(ctx->document.raster_samples,
                           cursor,
                           (size_t)entry_sample_count * sizeof(ctx->document.raster_samples[0]));
                }
            } else {
                DrawingProgramRasterSample *legacy_samples = 0;
                uint32_t sample_i;
                legacy_samples =
                    (DrawingProgramRasterSample *)malloc((size_t)entry_sample_count * sizeof(*legacy_samples));
                if (!legacy_samples) {
                    return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate legacy raster upgrade buffer" };
                }
                for (sample_i = 0u; sample_i < entry_sample_count; ++sample_i) {
                    legacy_samples[sample_i] = drawing_program_color_normalize_legacy_sample(cursor[sample_i]);
                }
                import_result = drawing_program_layer_raster_store_import_layer(
                    &ctx->layer_rasters,
                    layer_id,
                    legacy_samples,
                    entry_sample_count);
                if (import_result.code == CORE_OK && layer_id == base_layer_id) {
                    memcpy(ctx->document.raster_samples,
                           legacy_samples,
                           (size_t)entry_sample_count * sizeof(ctx->document.raster_samples[0]));
                }
                free(legacy_samples);
            }
            if (import_result.code != CORE_OK) {
                return import_result;
            }
        }
        cursor += (version == DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V2)
                      ? ((uint64_t)entry_sample_count * (uint64_t)sizeof(DrawingProgramRasterSample))
                      : (uint64_t)entry_sample_count;
    }
    if (cursor != end) {
        return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster chunk trailing bytes" };
    }
    return core_result_ok();
}

static CoreResult drawing_program_snapshot_write_object_chunk(
    CorePackWriter *writer,
    const struct DrawingProgramAppContext *ctx) {
    DrawingProgramObjectChunkHeaderV4 header;
    uint64_t payload_size;
    uint8_t *payload = 0;
    uint8_t *cursor = 0;
    uint32_t i;
    if (!writer || !ctx) {
        return snapshot_invalid("invalid object chunk write request");
    }
    if (ctx->object_store.object_count > DRAWING_PROGRAM_MAX_OBJECTS) {
        return (CoreResult){ CORE_ERR_FORMAT, "object store count exceeds max object capacity" };
    }
    memset(&header, 0, sizeof(header));
    header.version = DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V4;
    header.object_count = ctx->object_store.object_count;
    header.next_object_id = ctx->object_store.next_object_id;
    if (header.next_object_id == 0u) {
        header.next_object_id = 1u;
    }
    payload_size = (uint64_t)sizeof(header) +
                   ((uint64_t)header.object_count * (uint64_t)sizeof(DrawingProgramObjectChunkEntryV4));
    payload = (uint8_t *)malloc((size_t)payload_size);
    if (!payload) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate object snapshot payload" };
    }
    cursor = payload;
    memcpy(cursor, &header, sizeof(header));
    cursor += sizeof(header);
    for (i = 0u; i < header.object_count; ++i) {
        DrawingProgramObjectChunkEntryV4 entry;
        const DrawingProgramObjectRecord *object = &ctx->object_store.objects[i];
        memset(&entry, 0, sizeof(entry));
        entry.object_id = object->object_id;
        entry.layer_id = object->layer_id;
        entry.type = object->type;
        entry.visible = object->visible ? 1u : 0u;
        entry.locked = object->locked ? 1u : 0u;
        entry.stroke_width = object->stroke_width;
        entry.style_mode = object->style_mode;
        entry.stroke_color_value = drawing_program_color_normalize_input_sample(object->stroke_color_value);
        entry.fill_color_value = drawing_program_color_normalize_input_sample(object->fill_color_value);
        entry.origin_x = object->origin_x;
        entry.origin_y = object->origin_y;
        entry.width = object->width;
        entry.height = object->height;
        if (object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
            uint16_t point_count = object->path_point_count;
            if (point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
                point_count = DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS;
            }
            entry.path_closed = object->path_closed ? 1u : 0u;
            entry.path_point_count = point_count;
            memcpy(entry.path_points,
                   object->path_points,
                   (size_t)point_count * sizeof(entry.path_points[0]));
        }
        memcpy(entry.name, object->name, sizeof(entry.name));
        entry.name[sizeof(entry.name) - 1u] = '\0';
        memcpy(cursor, &entry, sizeof(entry));
        cursor += sizeof(entry);
    }
    {
        CoreResult result = core_pack_writer_add_chunk(writer, "DPOB", payload, payload_size);
        free(payload);
        return result;
    }
}

static CoreResult drawing_program_snapshot_apply_object_chunk(
    struct DrawingProgramAppContext *ctx,
    const void *chunk_data,
    uint64_t chunk_size) {
    DrawingProgramObjectChunkHeaderV1 header_v1;
    DrawingProgramObjectChunkHeaderV2 header_v2;
    DrawingProgramObjectChunkHeaderV3 header_v3;
    DrawingProgramObjectChunkHeaderV4 header_v4;
    DrawingProgramObjectStore *staged_store = 0;
    const uint8_t *cursor = (const uint8_t *)chunk_data;
    const uint8_t *end = cursor + chunk_size;
    uint32_t version = 0u;
    if (!ctx || !cursor || chunk_size < (uint64_t)sizeof(DrawingProgramObjectChunkHeaderV1)) {
        return snapshot_invalid("invalid object chunk payload");
    }
    memcpy(&version, cursor, sizeof(uint32_t));
    staged_store = (DrawingProgramObjectStore *)calloc(1u, sizeof(*staged_store));
    if (!staged_store) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate staged object store" };
    }
    drawing_program_object_store_reset(staged_store);
    if (version == DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V1) {
        DrawingProgramObjectRecord records[DRAWING_PROGRAM_MAX_OBJECTS];
        uint32_t i;
        uint64_t expected_size;
        memset(&header_v1, 0, sizeof(header_v1));
        memcpy(&header_v1, cursor, sizeof(header_v1));
        if (header_v1.object_count > DRAWING_PROGRAM_MAX_OBJECTS) {
            free(staged_store);
            return (CoreResult){ CORE_ERR_FORMAT, "object chunk count exceeds max object capacity" };
        }
        expected_size = (uint64_t)sizeof(header_v1) +
                        ((uint64_t)header_v1.object_count * (uint64_t)sizeof(DrawingProgramObjectChunkEntryV1));
        if (expected_size != chunk_size) {
            free(staged_store);
            return (CoreResult){ CORE_ERR_FORMAT, "drawing object chunk size mismatch" };
        }
        cursor += sizeof(header_v1);
        memset(records, 0, sizeof(records));
        for (i = 0u; i < header_v1.object_count; ++i) {
            DrawingProgramObjectChunkEntryV1 entry;
            DrawingProgramObjectRecord record;
            if ((uint64_t)(end - cursor) < (uint64_t)sizeof(entry)) {
                free(staged_store);
                return (CoreResult){ CORE_ERR_FORMAT, "drawing object chunk truncated entries" };
            }
            memcpy(&entry, cursor, sizeof(entry));
            cursor += sizeof(entry);
            memset(&record, 0, sizeof(record));
            record.object_id = entry.object_id;
            record.layer_id = entry.layer_id;
            record.type = entry.type;
            record.visible = entry.visible ? 1u : 0u;
            record.locked = entry.locked ? 1u : 0u;
            record.stroke_color_value =
                drawing_program_color_value_from_index(drawing_program_color_index_clamp(entry.stroke_color_index));
            record.fill_color_value =
                drawing_program_color_value_from_index(drawing_program_color_index_clamp(entry.fill_color_index));
            record.stroke_width = entry.stroke_width;
            record.style_mode = entry.style_mode;
            record.origin_x = entry.origin_x;
            record.origin_y = entry.origin_y;
            record.width = entry.width;
            record.height = entry.height;
            memcpy(record.name, entry.name, sizeof(record.name));
            record.name[sizeof(record.name) - 1u] = '\0';
            records[i] = record;
        }
        if (cursor != end) {
            free(staged_store);
            return (CoreResult){ CORE_ERR_FORMAT, "drawing object chunk trailing bytes" };
        }
        {
            CoreResult replace_result = drawing_program_object_store_replace_all(&ctx->object_store,
                                                                                 records,
                                                                                 header_v1.object_count,
                                                                                 header_v1.next_object_id);
            free(staged_store);
            return replace_result;
        }
    }
    if (version == DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V2) {
        uint32_t i;
        uint64_t expected_size;
        memset(&header_v2, 0, sizeof(header_v2));
        memcpy(&header_v2, cursor, sizeof(header_v2));
        if (header_v2.object_count > DRAWING_PROGRAM_MAX_OBJECTS) {
            free(staged_store);
            return (CoreResult){ CORE_ERR_FORMAT, "object chunk count exceeds max object capacity" };
        }
        expected_size = (uint64_t)sizeof(header_v2) +
                        ((uint64_t)header_v2.object_count * (uint64_t)sizeof(DrawingProgramObjectChunkEntryV2));
        if (expected_size != chunk_size) {
            free(staged_store);
            return (CoreResult){ CORE_ERR_FORMAT, "drawing object chunk size mismatch" };
        }
        cursor += sizeof(header_v2);
        for (i = 0u; i < header_v2.object_count; ++i) {
            DrawingProgramObjectChunkEntryV2 entry;
            DrawingProgramObjectRecord record;
            CoreResult insert_result;
            if ((uint64_t)(end - cursor) < (uint64_t)sizeof(entry)) {
                free(staged_store);
                return (CoreResult){ CORE_ERR_FORMAT, "drawing object chunk truncated entries" };
            }
            memcpy(&entry, cursor, sizeof(entry));
            cursor += sizeof(entry);
            memset(&record, 0, sizeof(record));
            record.object_id = entry.object_id;
            record.layer_id = entry.layer_id;
            record.type = entry.type;
            record.visible = entry.visible ? 1u : 0u;
            record.locked = entry.locked ? 1u : 0u;
            record.stroke_color_value =
                drawing_program_color_value_from_index(drawing_program_color_index_clamp(entry.stroke_color_index));
            record.fill_color_value =
                drawing_program_color_value_from_index(drawing_program_color_index_clamp(entry.fill_color_index));
            record.stroke_width = entry.stroke_width;
            record.style_mode = entry.style_mode;
            record.origin_x = entry.origin_x;
            record.origin_y = entry.origin_y;
            record.width = entry.width;
            record.height = entry.height;
            memcpy(record.name, entry.name, sizeof(record.name));
            record.name[sizeof(record.name) - 1u] = '\0';
            if (record.type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
                if (entry.path_point_count < 2u ||
                    entry.path_point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
                    continue;
                }
                record.path_point_count = entry.path_point_count;
                record.path_closed = entry.path_closed ? 1u : 0u;
                {
                    uint32_t point_i;
                    for (point_i = 0u; point_i < (uint32_t)entry.path_point_count; ++point_i) {
                        record.path_points[point_i].x = entry.path_points[point_i].x;
                        record.path_points[point_i].y = entry.path_points[point_i].y;
                        record.path_points[point_i].handle_in_dx = 0;
                        record.path_points[point_i].handle_in_dy = 0;
                        record.path_points[point_i].handle_out_dx = 0;
                        record.path_points[point_i].handle_out_dy = 0;
                        record.path_points[point_i].bezier_enabled = 0u;
                        record.path_points[point_i].handle_linked = 0u;
                        record.path_points[point_i].reserved0 = 0u;
                        record.path_points[point_i].reserved1 = 0u;
                    }
                }
            }
            insert_result = drawing_program_object_store_insert_with_id(staged_store, &record, record.object_id);
            if (insert_result.code != CORE_OK) {
                continue;
            }
        }
        if (cursor != end) {
            free(staged_store);
            return (CoreResult){ CORE_ERR_FORMAT, "drawing object chunk trailing bytes" };
        }
        if (header_v2.next_object_id > staged_store->next_object_id) {
            staged_store->next_object_id = header_v2.next_object_id;
            if (staged_store->next_object_id == 0u) {
                staged_store->next_object_id = 1u;
            }
        }
        ctx->object_store = *staged_store;
        free(staged_store);
        return core_result_ok();
    }
    if (version == DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V3) {
        uint32_t i;
        uint64_t expected_size;
        memset(&header_v3, 0, sizeof(header_v3));
        memcpy(&header_v3, cursor, sizeof(header_v3));
        if (header_v3.object_count > DRAWING_PROGRAM_MAX_OBJECTS) {
            free(staged_store);
            return (CoreResult){ CORE_ERR_FORMAT, "object chunk count exceeds max object capacity" };
        }
        expected_size = (uint64_t)sizeof(header_v3) +
                        ((uint64_t)header_v3.object_count * (uint64_t)sizeof(DrawingProgramObjectChunkEntryV3));
        if (expected_size != chunk_size) {
            free(staged_store);
            return (CoreResult){ CORE_ERR_FORMAT, "drawing object chunk size mismatch" };
        }
        cursor += sizeof(header_v3);
        for (i = 0u; i < header_v3.object_count; ++i) {
            DrawingProgramObjectChunkEntryV3 entry;
            DrawingProgramObjectRecord record;
            CoreResult insert_result;
            if ((uint64_t)(end - cursor) < (uint64_t)sizeof(entry)) {
                free(staged_store);
                return (CoreResult){ CORE_ERR_FORMAT, "drawing object chunk truncated entries" };
            }
            memcpy(&entry, cursor, sizeof(entry));
            cursor += sizeof(entry);
            memset(&record, 0, sizeof(record));
            record.object_id = entry.object_id;
            record.layer_id = entry.layer_id;
            record.type = entry.type;
            record.visible = entry.visible ? 1u : 0u;
            record.locked = entry.locked ? 1u : 0u;
            record.stroke_color_value =
                drawing_program_color_value_from_index(drawing_program_color_index_clamp(entry.stroke_color_index));
            record.fill_color_value =
                drawing_program_color_value_from_index(drawing_program_color_index_clamp(entry.fill_color_index));
            record.stroke_width = entry.stroke_width;
            record.style_mode = entry.style_mode;
            record.origin_x = entry.origin_x;
            record.origin_y = entry.origin_y;
            record.width = entry.width;
            record.height = entry.height;
            memcpy(record.name, entry.name, sizeof(record.name));
            record.name[sizeof(record.name) - 1u] = '\0';
            if (record.type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
                if (entry.path_point_count < 2u ||
                    entry.path_point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
                    continue;
                }
                record.path_point_count = entry.path_point_count;
                record.path_closed = entry.path_closed ? 1u : 0u;
                memcpy(record.path_points,
                       entry.path_points,
                       (size_t)entry.path_point_count * sizeof(record.path_points[0]));
            }
            insert_result = drawing_program_object_store_insert_with_id(staged_store, &record, record.object_id);
            if (insert_result.code != CORE_OK) {
                continue;
            }
        }
        if (cursor != end) {
            free(staged_store);
            return (CoreResult){ CORE_ERR_FORMAT, "drawing object chunk trailing bytes" };
        }
        if (header_v3.next_object_id > staged_store->next_object_id) {
            staged_store->next_object_id = header_v3.next_object_id;
            if (staged_store->next_object_id == 0u) {
                staged_store->next_object_id = 1u;
            }
        }
        ctx->object_store = *staged_store;
        free(staged_store);
        return core_result_ok();
    }
    if (version == DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V4) {
        uint32_t i;
        uint64_t expected_size;
        memset(&header_v4, 0, sizeof(header_v4));
        memcpy(&header_v4, cursor, sizeof(header_v4));
        if (header_v4.object_count > DRAWING_PROGRAM_MAX_OBJECTS) {
            free(staged_store);
            return (CoreResult){ CORE_ERR_FORMAT, "object chunk count exceeds max object capacity" };
        }
        expected_size = (uint64_t)sizeof(header_v4) +
                        ((uint64_t)header_v4.object_count * (uint64_t)sizeof(DrawingProgramObjectChunkEntryV4));
        if (expected_size != chunk_size) {
            free(staged_store);
            return (CoreResult){ CORE_ERR_FORMAT, "drawing object chunk size mismatch" };
        }
        cursor += sizeof(header_v4);
        for (i = 0u; i < header_v4.object_count; ++i) {
            DrawingProgramObjectChunkEntryV4 entry;
            DrawingProgramObjectRecord record;
            CoreResult insert_result;
            if ((uint64_t)(end - cursor) < (uint64_t)sizeof(entry)) {
                free(staged_store);
                return (CoreResult){ CORE_ERR_FORMAT, "drawing object chunk truncated entries" };
            }
            memcpy(&entry, cursor, sizeof(entry));
            cursor += sizeof(entry);
            memset(&record, 0, sizeof(record));
            record.object_id = entry.object_id;
            record.layer_id = entry.layer_id;
            record.type = entry.type;
            record.visible = entry.visible ? 1u : 0u;
            record.locked = entry.locked ? 1u : 0u;
            record.stroke_color_value = drawing_program_color_normalize_input_sample(entry.stroke_color_value);
            record.fill_color_value = drawing_program_color_normalize_input_sample(entry.fill_color_value);
            record.stroke_width = entry.stroke_width;
            record.style_mode = entry.style_mode;
            record.origin_x = entry.origin_x;
            record.origin_y = entry.origin_y;
            record.width = entry.width;
            record.height = entry.height;
            memcpy(record.name, entry.name, sizeof(record.name));
            record.name[sizeof(record.name) - 1u] = '\0';
            if (record.type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
                if (entry.path_point_count < 2u ||
                    entry.path_point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
                    continue;
                }
                record.path_point_count = entry.path_point_count;
                record.path_closed = entry.path_closed ? 1u : 0u;
                memcpy(record.path_points,
                       entry.path_points,
                       (size_t)entry.path_point_count * sizeof(record.path_points[0]));
            }
            insert_result = drawing_program_object_store_insert_with_id(staged_store, &record, record.object_id);
            if (insert_result.code != CORE_OK) {
                continue;
            }
        }
        if (cursor != end) {
            free(staged_store);
            return (CoreResult){ CORE_ERR_FORMAT, "drawing object chunk trailing bytes" };
        }
        if (header_v4.next_object_id > staged_store->next_object_id) {
            staged_store->next_object_id = header_v4.next_object_id;
            if (staged_store->next_object_id == 0u) {
                staged_store->next_object_id = 1u;
            }
        }
        ctx->object_store = *staged_store;
        free(staged_store);
        return core_result_ok();
    }
    free(staged_store);
    return (CoreResult){ CORE_ERR_FORMAT, "unsupported drawing object chunk version" };
}

CoreResult drawing_program_snapshot_save(const struct DrawingProgramAppContext *ctx, const char *path) {
    CorePackWriter writer;
    DrawingProgramSnapshotV1 *legacy_payload = 0;
    CoreResult result;
    if (!ctx || !path) {
        return snapshot_invalid("invalid snapshot save request");
    }
    legacy_payload = (DrawingProgramSnapshotV1 *)calloc(1u, sizeof(*legacy_payload));
    if (!legacy_payload) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate legacy snapshot payload" };
    }
    legacy_payload->header.version = 1u;
    legacy_payload->header.node_count = ctx->pane_host.node_count;
    legacy_payload->header.binding_count = ctx->pane_host.module_binding_count;
    legacy_payload->header.history_count = ctx->history.count;
    legacy_payload->header.history_cursor = ctx->history.cursor;
    legacy_payload->document = ctx->document;
    legacy_payload->editor = ctx->editor;
    legacy_payload->layout_state = ctx->pane_host.layout_state;
    memcpy(legacy_payload->nodes, ctx->pane_host.nodes, sizeof(legacy_payload->nodes));
    memcpy(legacy_payload->bindings, ctx->pane_host.module_bindings, sizeof(legacy_payload->bindings));
    memcpy(legacy_payload->history_entries, ctx->history.entries, sizeof(legacy_payload->history_entries));
    if (snapshot_trace_enabled()) {
        fprintf(stderr,
                "drawing_program trace snapshot_save begin path=%s tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u color=%u\n",
                path ? path : "(null)",
                (unsigned)ctx->editor.active_tool,
                (unsigned)ctx->ui.theme_preset_id,
                (unsigned)ctx->ui.font_preset_id,
                (int)ctx->ui.font_zoom_step,
                (unsigned)ctx->ui.left_panel_slot,
                (unsigned)ctx->ui.right_panel_slot,
                (unsigned)ctx->ui.active_color_index);
    }

    result = core_pack_writer_open(path, &writer);
    if (result.code != CORE_OK) {
        free(legacy_payload);
        if (snapshot_trace_enabled()) {
            fprintf(stderr,
                    "drawing_program trace snapshot_save fail path=%s code=%d message=%s\n",
                    path ? path : "(null)",
                    (int)result.code,
                    result.message ? result.message : "(null)");
        }
        return result;
    }
    result = drawing_program_snapshot_shell_write_current(&writer, ctx);
    if (result.code != CORE_OK) {
        free(legacy_payload);
        (void)core_pack_writer_close(&writer);
        return result;
    }
    result = core_pack_writer_add_chunk(&writer, "DPS2", legacy_payload, (uint64_t)sizeof(*legacy_payload));
    if (result.code != CORE_OK) {
        free(legacy_payload);
        (void)core_pack_writer_close(&writer);
        return result;
    }
    result = drawing_program_snapshot_write_layer_raster_chunk(&writer, ctx);
    if (result.code != CORE_OK) {
        free(legacy_payload);
        (void)core_pack_writer_close(&writer);
        return result;
    }
    result = drawing_program_snapshot_write_object_chunk(&writer, ctx);
    if (result.code != CORE_OK) {
        free(legacy_payload);
        (void)core_pack_writer_close(&writer);
        return result;
    }
    result = drawing_program_snapshot_write_ui_settings_chunk(&writer, ctx);
    if (result.code != CORE_OK) {
        free(legacy_payload);
        (void)core_pack_writer_close(&writer);
        return result;
    }
    result = core_pack_writer_close(&writer);
    free(legacy_payload);
    if (snapshot_trace_enabled()) {
        fprintf(stderr,
                "drawing_program trace snapshot_save end path=%s code=%d message=%s\n",
                path ? path : "(null)",
                (int)result.code,
                result.message ? result.message : "(null)");
    }
    return result;
}

CoreResult drawing_program_snapshot_load(struct DrawingProgramAppContext *ctx, const char *path) {
    CorePackReader reader;
    CorePackChunkInfo chunk;
    CorePackChunkInfo ui_chunk;
    CorePackChunkInfo layer_chunk;
    CorePackChunkInfo object_chunk;
    DrawingProgramSnapshotV1 *payload = 0;
    CoreResult result;
    uint8_t upgraded_legacy_palette_samples = 0u;
    uint8_t loaded_current_shell = 0u;
    uint8_t *layer_chunk_data = 0;
    uint8_t *object_chunk_data = 0;
    if (!ctx || !path) {
        return snapshot_invalid("invalid snapshot load request");
    }

    payload = (DrawingProgramSnapshotV1 *)calloc(1u, sizeof(*payload));
    if (!payload) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate snapshot load payload" };
    }
    if (snapshot_trace_enabled()) {
        fprintf(stderr, "drawing_program trace snapshot_load begin path=%s\n", path ? path : "(null)");
    }
    result = core_pack_reader_open(path, &reader);
    if (result.code != CORE_OK) {
        free(payload);
        if (snapshot_trace_enabled()) {
            fprintf(stderr,
                    "drawing_program trace snapshot_load fail_open path=%s code=%d message=%s\n",
                    path ? path : "(null)",
                    (int)result.code,
                    result.message ? result.message : "(null)");
        }
        return result;
    }
    result = drawing_program_snapshot_shell_load_current(ctx, &reader, &loaded_current_shell);
    if (result.code != CORE_OK) {
        free(payload);
        (void)core_pack_reader_close(&reader);
        return result;
    }
    if (!loaded_current_shell) {
        result = core_pack_reader_find_chunk(&reader, "DPS2", 0u, &chunk);
        if (result.code != CORE_OK) {
            free(payload);
            (void)core_pack_reader_close(&reader);
            return result;
        }
        if (chunk.size != (uint64_t)sizeof(*payload)) {
            result = drawing_program_snapshot_load_legacy_chunked_fallback(ctx, &reader);
            if (result.code != CORE_OK) {
                free(payload);
                (void)core_pack_reader_close(&reader);
                return result;
            }
        } else {
            result = core_pack_reader_read_chunk_data(&reader, &chunk, payload, (uint64_t)sizeof(*payload));
            if (result.code != CORE_OK) {
                free(payload);
                (void)core_pack_reader_close(&reader);
                return result;
            }
            if (payload->header.version != 1u) {
                free(payload);
                (void)core_pack_reader_close(&reader);
                return (CoreResult){ CORE_ERR_FORMAT, "unsupported drawing snapshot version" };
            }
            if (payload->header.node_count > DRAWING_PROGRAM_PANE_NODE_CAPACITY ||
                payload->header.binding_count > DRAWING_PROGRAM_MODULE_BINDING_CAPACITY ||
                payload->header.history_count > DRAWING_PROGRAM_HISTORY_CAPACITY ||
                payload->header.history_cursor > payload->header.history_count) {
                free(payload);
                (void)core_pack_reader_close(&reader);
                return (CoreResult){ CORE_ERR_FORMAT, "invalid drawing snapshot bounds" };
            }

            ctx->document = payload->document;
            ctx->editor = payload->editor;
            ctx->pane_host.layout_state = payload->layout_state;
            memcpy(ctx->pane_host.nodes, payload->nodes, sizeof(ctx->pane_host.nodes));
            memcpy(ctx->pane_host.module_bindings, payload->bindings, sizeof(ctx->pane_host.module_bindings));
            memcpy(ctx->history.entries, payload->history_entries, sizeof(ctx->history.entries));
            ctx->history.count = payload->header.history_count;
            ctx->history.cursor = payload->header.history_cursor;
            ctx->pane_host.node_count = payload->header.node_count;
            ctx->pane_host.root_index = 0u;
            ctx->pane_host.module_binding_count = payload->header.binding_count;
        }
    }
    drawing_program_object_store_reset(&ctx->object_store);
    result = drawing_program_layer_raster_store_init_from_document(&ctx->layer_rasters, &ctx->document);
    if (result.code != CORE_OK) {
        free(payload);
        (void)core_pack_reader_close(&reader);
        return result;
    }
    memset(&layer_chunk, 0, sizeof(layer_chunk));
    memset(&object_chunk, 0, sizeof(object_chunk));
    result = core_pack_reader_find_chunk(&reader, "DPLR", 0u, &layer_chunk);
    if (result.code == CORE_OK) {
        layer_chunk_data = (uint8_t *)malloc((size_t)layer_chunk.size);
        if (!layer_chunk_data) {
            free(payload);
            (void)core_pack_reader_close(&reader);
            return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate layer raster chunk buffer" };
        }
        result = core_pack_reader_read_chunk_data(&reader, &layer_chunk, layer_chunk_data, layer_chunk.size);
        if (result.code != CORE_OK) {
            free(layer_chunk_data);
            layer_chunk_data = 0;
            free(payload);
            (void)core_pack_reader_close(&reader);
            return result;
        }
        result = drawing_program_snapshot_apply_layer_raster_chunk(ctx, layer_chunk_data, layer_chunk.size);
        free(layer_chunk_data);
        layer_chunk_data = 0;
        if (result.code != CORE_OK) {
            free(payload);
            (void)core_pack_reader_close(&reader);
            return result;
        }
    }
    result = core_pack_reader_find_chunk(&reader, "DPOB", 0u, &object_chunk);
    if (result.code == CORE_OK) {
        object_chunk_data = (uint8_t *)malloc((size_t)object_chunk.size);
        if (!object_chunk_data) {
            free(payload);
            (void)core_pack_reader_close(&reader);
            return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate object chunk buffer" };
        }
        result = core_pack_reader_read_chunk_data(&reader, &object_chunk, object_chunk_data, object_chunk.size);
        if (result.code != CORE_OK) {
            free(object_chunk_data);
            object_chunk_data = 0;
            free(payload);
            (void)core_pack_reader_close(&reader);
            return result;
        }
        result = drawing_program_snapshot_apply_object_chunk(ctx, object_chunk_data, object_chunk.size);
        free(object_chunk_data);
        object_chunk_data = 0;
        if (result.code != CORE_OK) {
            free(payload);
            (void)core_pack_reader_close(&reader);
            return result;
        }
    }
    result = drawing_program_snapshot_upgrade_legacy_palette_index_samples(ctx, &upgraded_legacy_palette_samples);
    if (result.code != CORE_OK) {
        free(payload);
        (void)core_pack_reader_close(&reader);
        return result;
    }
    result = core_pack_reader_find_chunk(&reader, "DPUI", 0u, &ui_chunk);
    if (result.code == CORE_OK) {
        result = drawing_program_snapshot_apply_ui_settings_chunk(ctx, &reader, &ui_chunk);
    }
    (void)core_pack_reader_close(&reader);
    free(payload);
    result = drawing_program_pane_host_rebuild(ctx);
    if (result.code == CORE_OK) {
        drawing_program_app_rearm_after_document_swap(ctx);
    }
    if (snapshot_trace_enabled()) {
        fprintf(stderr,
                "drawing_program trace snapshot_load end path=%s code=%d tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u color=%u leafs=%u upgraded_palette=%u\n",
                path ? path : "(null)",
                (int)result.code,
                (unsigned)ctx->editor.active_tool,
                (unsigned)ctx->ui.theme_preset_id,
                (unsigned)ctx->ui.font_preset_id,
                (int)ctx->ui.font_zoom_step,
                (unsigned)ctx->ui.left_panel_slot,
                (unsigned)ctx->ui.right_panel_slot,
                (unsigned)ctx->ui.active_color_index,
                (unsigned)ctx->pane_host.leaf_count,
                (unsigned)upgraded_legacy_palette_samples);
    }
    return result;
}
