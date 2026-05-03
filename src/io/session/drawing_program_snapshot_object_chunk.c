#include "drawing_program_snapshot_internal.h"

#include <stdlib.h>
#include <string.h>

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
    DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V1 = 1u,
    DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V2 = 2u,
    DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V3 = 3u,
    DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V4 = 4u
};

CoreResult drawing_program_snapshot_write_object_chunk(
    CorePackWriter *writer,
    const struct DrawingProgramAppContext *ctx) {
    DrawingProgramObjectChunkHeaderV4 header;
    uint64_t payload_size;
    uint8_t *payload = 0;
    uint8_t *cursor = 0;
    uint32_t i;
    if (!writer || !ctx) {
        return drawing_program_snapshot_invalid("invalid object chunk write request");
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

CoreResult drawing_program_snapshot_apply_object_chunk(
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
        return drawing_program_snapshot_invalid("invalid object chunk payload");
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
