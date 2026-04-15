#include "drawing_program/drawing_program_snapshot.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "core_pack.h"
#include "core_font.h"
#include "core_theme.h"
#include "drawing_program/drawing_program_app_main.h"

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

typedef struct DrawingProgramUiSettingsV1 {
    uint32_t version;
    uint32_t theme_preset_id;
    uint8_t left_panel_slot;
    uint8_t right_panel_slot;
    uint8_t reserved0;
    uint8_t reserved1;
} DrawingProgramUiSettingsV1;

typedef struct DrawingProgramUiSettingsV2 {
    uint32_t version;
    uint32_t theme_preset_id;
    uint32_t font_preset_id;
    int32_t font_zoom_step;
    uint8_t left_panel_slot;
    uint8_t right_panel_slot;
    uint8_t reserved0;
    uint8_t reserved1;
} DrawingProgramUiSettingsV2;

typedef struct DrawingProgramUiSettingsV3 {
    uint32_t version;
    uint32_t theme_preset_id;
    uint32_t font_preset_id;
    int32_t font_zoom_step;
    uint8_t left_panel_slot;
    uint8_t right_panel_slot;
    uint8_t active_color_index;
    uint8_t reserved0;
} DrawingProgramUiSettingsV3;

typedef struct DrawingProgramUiSettingsV4 {
    uint32_t version;
    uint32_t theme_preset_id;
    uint32_t font_preset_id;
    int32_t font_zoom_step;
    uint8_t left_panel_slot;
    uint8_t right_panel_slot;
    uint8_t active_color_index;
    uint8_t selection_has_payload;
    uint32_t selection_origin_x;
    uint32_t selection_origin_y;
    uint32_t selection_width;
    uint32_t selection_height;
} DrawingProgramUiSettingsV4;

typedef struct DrawingProgramUiSettingsV5 {
    uint32_t version;
    uint32_t theme_preset_id;
    uint32_t font_preset_id;
    int32_t font_zoom_step;
    uint8_t left_panel_slot;
    uint8_t right_panel_slot;
    uint8_t active_color_index;
    uint8_t selection_has_payload;
    uint32_t selection_origin_x;
    uint32_t selection_origin_y;
    uint32_t selection_width;
    uint32_t selection_height;
    uint8_t tool_brush_size;
    uint8_t tool_brush_opacity;
    uint8_t tool_eraser_size;
    uint8_t tool_shape_stroke_width;
    uint8_t tool_shape_mode;
    uint8_t tool_fill_tolerance;
    uint8_t reserved0;
    uint8_t reserved1;
} DrawingProgramUiSettingsV5;

typedef struct DrawingProgramUiSettingsV6 {
    uint32_t version;
    uint32_t theme_preset_id;
    uint32_t font_preset_id;
    int32_t font_zoom_step;
    uint8_t left_panel_slot;
    uint8_t right_panel_slot;
    uint8_t active_color_index;
    uint8_t selection_has_payload;
    uint32_t selection_origin_x;
    uint32_t selection_origin_y;
    uint32_t selection_width;
    uint32_t selection_height;
    uint8_t tool_brush_size;
    uint8_t tool_brush_opacity;
    uint8_t tool_brush_spacing;
    uint8_t tool_brush_hardness;
    uint8_t tool_eraser_size;
    uint8_t tool_shape_stroke_width;
    uint8_t tool_shape_mode;
    uint8_t tool_fill_tolerance;
    uint8_t layer_opacity_entry_count;
    uint8_t reserved0;
    uint8_t reserved1;
    uint8_t reserved2;
    uint32_t layer_opacity_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t layer_opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
} DrawingProgramUiSettingsV6;

typedef struct DrawingProgramUiSettingsV7 {
    uint32_t version;
    uint32_t theme_preset_id;
    uint32_t font_preset_id;
    int32_t font_zoom_step;
    uint8_t left_panel_slot;
    uint8_t right_panel_slot;
    uint8_t active_color_index;
    uint8_t selection_has_payload;
    uint32_t selection_origin_x;
    uint32_t selection_origin_y;
    uint32_t selection_width;
    uint32_t selection_height;
    uint8_t tool_brush_size;
    uint8_t tool_brush_opacity;
    uint8_t tool_brush_spacing;
    uint8_t tool_brush_hardness;
    uint8_t tool_eraser_size;
    uint8_t tool_shape_stroke_width;
    uint8_t tool_shape_mode;
    uint8_t tool_shape_target_mode;
    uint8_t tool_fill_tolerance;
    uint8_t tool_select_mode;
    uint8_t layer_opacity_entry_count;
    uint8_t reserved0;
    uint8_t reserved1;
    uint8_t reserved2;
    uint32_t layer_opacity_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t layer_opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
} DrawingProgramUiSettingsV7;

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
    DrawingProgramPathPoint path_points[DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS];
} DrawingProgramObjectChunkEntryV2;

enum {
    DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V1 = 1u,
    DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V1 = 1u,
    DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V2 = 2u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V1 = 1u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V2 = 2u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V3 = 3u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V4 = 4u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V5 = 5u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V6 = 6u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V7 = 7u
};

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
    const uint8_t eraser_value = drawing_program_color_eraser_value();
    if (!writer || !ctx) {
        return snapshot_invalid("invalid layer raster chunk write request");
    }
    header_bytes = (uint64_t)(sizeof(uint32_t) * 5u);
    layer_bytes = (uint64_t)(sizeof(uint32_t) * 2u) + (uint64_t)sample_count;
    if (sample_count == 0u || ctx->document.layer_count == 0u) {
        return core_result_ok();
    }
    payload_size = header_bytes + (layer_bytes * (uint64_t)ctx->document.layer_count);
    payload = (uint8_t *)malloc((size_t)payload_size);
    if (!payload) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate layer raster snapshot payload" };
    }
    cursor = payload;
    memcpy(cursor, &(uint32_t){ DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V1 }, sizeof(uint32_t));
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
        const uint8_t *samples = 0;
        uint32_t exported_sample_count = 0u;
        CoreResult export_result = { CORE_ERR_NOT_FOUND, "layer raster export unavailable" };
        memcpy(cursor, &layer_id, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        memcpy(cursor, &sample_count, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        if (i == 0u) {
            memcpy(cursor, ctx->document.raster_samples, (size_t)sample_count);
            cursor += sample_count;
            continue;
        }
        export_result = drawing_program_layer_raster_store_export_layer(&ctx->layer_rasters,
                                                                        layer_id,
                                                                        &samples,
                                                                        &exported_sample_count);
        if (export_result.code == CORE_OK &&
            samples &&
            exported_sample_count == sample_count) {
            memcpy(cursor, samples, (size_t)sample_count);
        } else {
            memset(cursor, (int)eraser_value, (size_t)sample_count);
        }
        cursor += sample_count;
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
    if (version != DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V1) {
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
        if ((uint64_t)(end - cursor) < (uint64_t)entry_sample_count) {
            return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster chunk truncated samples" };
        }
        if (drawing_program_snapshot_document_has_layer_id(&ctx->document, layer_id)) {
            CoreResult import_result = drawing_program_layer_raster_store_import_layer(
                &ctx->layer_rasters,
                layer_id,
                cursor,
                entry_sample_count);
            if (import_result.code != CORE_OK) {
                return import_result;
            }
        }
        cursor += entry_sample_count;
    }
    if (cursor != end) {
        return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster chunk trailing bytes" };
    }
    return core_result_ok();
}

static CoreResult drawing_program_snapshot_write_object_chunk(
    CorePackWriter *writer,
    const struct DrawingProgramAppContext *ctx) {
    DrawingProgramObjectChunkHeaderV2 header;
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
    header.version = DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V2;
    header.object_count = ctx->object_store.object_count;
    header.next_object_id = ctx->object_store.next_object_id;
    if (header.next_object_id == 0u) {
        header.next_object_id = 1u;
    }
    payload_size = (uint64_t)sizeof(header) +
                   ((uint64_t)header.object_count * (uint64_t)sizeof(DrawingProgramObjectChunkEntryV2));
    payload = (uint8_t *)malloc((size_t)payload_size);
    if (!payload) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate object snapshot payload" };
    }
    cursor = payload;
    memcpy(cursor, &header, sizeof(header));
    cursor += sizeof(header);
    for (i = 0u; i < header.object_count; ++i) {
        DrawingProgramObjectChunkEntryV2 entry;
        const DrawingProgramObjectRecord *object = &ctx->object_store.objects[i];
        memset(&entry, 0, sizeof(entry));
        entry.object_id = object->object_id;
        entry.layer_id = object->layer_id;
        entry.type = object->type;
        entry.visible = object->visible ? 1u : 0u;
        entry.locked = object->locked ? 1u : 0u;
        entry.stroke_color_index = object->stroke_color_index;
        entry.fill_color_index = object->fill_color_index;
        entry.stroke_width = object->stroke_width;
        entry.style_mode = object->style_mode;
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
    DrawingProgramObjectStore staged_store;
    const uint8_t *cursor = (const uint8_t *)chunk_data;
    const uint8_t *end = cursor + chunk_size;
    uint32_t version = 0u;
    if (!ctx || !cursor || chunk_size < (uint64_t)sizeof(DrawingProgramObjectChunkHeaderV1)) {
        return snapshot_invalid("invalid object chunk payload");
    }
    memcpy(&version, cursor, sizeof(uint32_t));
    drawing_program_object_store_reset(&staged_store);
    if (version == DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V1) {
        DrawingProgramObjectRecord records[DRAWING_PROGRAM_MAX_OBJECTS];
        uint32_t i;
        uint64_t expected_size;
        memset(&header_v1, 0, sizeof(header_v1));
        memcpy(&header_v1, cursor, sizeof(header_v1));
        if (header_v1.object_count > DRAWING_PROGRAM_MAX_OBJECTS) {
            return (CoreResult){ CORE_ERR_FORMAT, "object chunk count exceeds max object capacity" };
        }
        expected_size = (uint64_t)sizeof(header_v1) +
                        ((uint64_t)header_v1.object_count * (uint64_t)sizeof(DrawingProgramObjectChunkEntryV1));
        if (expected_size != chunk_size) {
            return (CoreResult){ CORE_ERR_FORMAT, "drawing object chunk size mismatch" };
        }
        cursor += sizeof(header_v1);
        memset(records, 0, sizeof(records));
        for (i = 0u; i < header_v1.object_count; ++i) {
            DrawingProgramObjectChunkEntryV1 entry;
            DrawingProgramObjectRecord record;
            if ((uint64_t)(end - cursor) < (uint64_t)sizeof(entry)) {
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
            record.stroke_color_index = entry.stroke_color_index;
            record.fill_color_index = entry.fill_color_index;
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
            return (CoreResult){ CORE_ERR_FORMAT, "drawing object chunk trailing bytes" };
        }
        return drawing_program_object_store_replace_all(&ctx->object_store,
                                                        records,
                                                        header_v1.object_count,
                                                        header_v1.next_object_id);
    }
    if (version == DRAWING_PROGRAM_OBJECT_CHUNK_VERSION_V2) {
        uint32_t i;
        uint64_t expected_size;
        memset(&header_v2, 0, sizeof(header_v2));
        memcpy(&header_v2, cursor, sizeof(header_v2));
        if (header_v2.object_count > DRAWING_PROGRAM_MAX_OBJECTS) {
            return (CoreResult){ CORE_ERR_FORMAT, "object chunk count exceeds max object capacity" };
        }
        expected_size = (uint64_t)sizeof(header_v2) +
                        ((uint64_t)header_v2.object_count * (uint64_t)sizeof(DrawingProgramObjectChunkEntryV2));
        if (expected_size != chunk_size) {
            return (CoreResult){ CORE_ERR_FORMAT, "drawing object chunk size mismatch" };
        }
        cursor += sizeof(header_v2);
        for (i = 0u; i < header_v2.object_count; ++i) {
            DrawingProgramObjectChunkEntryV2 entry;
            DrawingProgramObjectRecord record;
            CoreResult insert_result;
            if ((uint64_t)(end - cursor) < (uint64_t)sizeof(entry)) {
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
            record.stroke_color_index = entry.stroke_color_index;
            record.fill_color_index = entry.fill_color_index;
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
            insert_result = drawing_program_object_store_insert_with_id(&staged_store, &record, record.object_id);
            if (insert_result.code != CORE_OK) {
                continue;
            }
        }
        if (cursor != end) {
            return (CoreResult){ CORE_ERR_FORMAT, "drawing object chunk trailing bytes" };
        }
        if (header_v2.next_object_id > staged_store.next_object_id) {
            staged_store.next_object_id = header_v2.next_object_id;
            if (staged_store.next_object_id == 0u) {
                staged_store.next_object_id = 1u;
            }
        }
        ctx->object_store = staged_store;
        return core_result_ok();
    }
    return (CoreResult){ CORE_ERR_FORMAT, "unsupported drawing object chunk version" };
}

CoreResult drawing_program_snapshot_save(const struct DrawingProgramAppContext *ctx, const char *path) {
    CorePackWriter writer;
    DrawingProgramSnapshotV1 payload;
    DrawingProgramUiSettingsV7 ui_settings;
    CoreResult result;
    if (!ctx || !path) {
        return snapshot_invalid("invalid snapshot save request");
    }

    memset(&payload, 0, sizeof(payload));
    payload.header.version = 1u;
    payload.header.node_count = ctx->pane_host.node_count;
    payload.header.binding_count = ctx->pane_host.module_binding_count;
    payload.header.history_count = ctx->history.count;
    payload.header.history_cursor = ctx->history.cursor;
    payload.document = ctx->document;
    payload.editor = ctx->editor;
    payload.layout_state = ctx->pane_host.layout_state;
    memcpy(payload.nodes, ctx->pane_host.nodes, sizeof(payload.nodes));
    memcpy(payload.bindings, ctx->pane_host.module_bindings, sizeof(payload.bindings));
    memcpy(payload.history_entries, ctx->history.entries, sizeof(payload.history_entries));
    memset(&ui_settings, 0, sizeof(ui_settings));
    ui_settings.version = DRAWING_PROGRAM_UI_SETTINGS_VERSION_V7;
    ui_settings.theme_preset_id = ctx->ui_theme_preset_id;
    ui_settings.font_preset_id = ctx->ui_font_preset_id;
    ui_settings.font_zoom_step = (int32_t)ctx->ui_font_zoom_step;
    ui_settings.left_panel_slot = ctx->ui_left_panel_slot;
    ui_settings.right_panel_slot = ctx->ui_right_panel_slot;
    ui_settings.active_color_index = ctx->ui_active_color_index;
    ui_settings.selection_has_payload = (ctx->selection.has_payload && ctx->selection.width > 0u && ctx->selection.height > 0u) ? 1u : 0u;
    ui_settings.selection_origin_x = ctx->selection.origin_x;
    ui_settings.selection_origin_y = ctx->selection.origin_y;
    ui_settings.selection_width = ctx->selection.width;
    ui_settings.selection_height = ctx->selection.height;
    ui_settings.tool_brush_size = ctx->ui_tool_brush_size;
    ui_settings.tool_brush_opacity = ctx->ui_tool_brush_opacity;
    ui_settings.tool_brush_spacing = ctx->ui_tool_brush_spacing;
    ui_settings.tool_brush_hardness = ctx->ui_tool_brush_hardness;
    ui_settings.tool_eraser_size = ctx->ui_tool_eraser_size;
    ui_settings.tool_shape_stroke_width = ctx->ui_tool_shape_stroke_width;
    ui_settings.tool_shape_mode = ctx->ui_tool_shape_mode;
    ui_settings.tool_shape_target_mode = ctx->ui_tool_shape_target_mode;
    ui_settings.tool_fill_tolerance = ctx->ui_tool_fill_tolerance;
    ui_settings.tool_select_mode = ctx->ui_tool_select_mode;
    ui_settings.layer_opacity_entry_count = ctx->ui_layer_opacity_entry_count;
    if (ui_settings.layer_opacity_entry_count > DRAWING_PROGRAM_MAX_LAYERS) {
        ui_settings.layer_opacity_entry_count = DRAWING_PROGRAM_MAX_LAYERS;
    }
    memcpy(ui_settings.layer_opacity_layer_ids,
           ctx->ui_layer_opacity_layer_ids,
           sizeof(ui_settings.layer_opacity_layer_ids));
    memcpy(ui_settings.layer_opacity_values,
           ctx->ui_layer_opacity_values,
           sizeof(ui_settings.layer_opacity_values));

    if (snapshot_trace_enabled()) {
        fprintf(stderr,
                "drawing_program trace snapshot_save begin path=%s tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u color=%u\n",
                path ? path : "(null)",
                (unsigned)ctx->editor.active_tool,
                (unsigned)ctx->ui_theme_preset_id,
                (unsigned)ctx->ui_font_preset_id,
                (int)ctx->ui_font_zoom_step,
                (unsigned)ctx->ui_left_panel_slot,
                (unsigned)ctx->ui_right_panel_slot,
                (unsigned)ctx->ui_active_color_index);
    }

    result = core_pack_writer_open(path, &writer);
    if (result.code != CORE_OK) {
        if (snapshot_trace_enabled()) {
            fprintf(stderr,
                    "drawing_program trace snapshot_save fail path=%s code=%d message=%s\n",
                    path ? path : "(null)",
                    (int)result.code,
                    result.message ? result.message : "(null)");
        }
        return result;
    }
    result = core_pack_writer_add_chunk(&writer, "DPS2", &payload, (uint64_t)sizeof(payload));
    if (result.code != CORE_OK) {
        (void)core_pack_writer_close(&writer);
        return result;
    }
    result = drawing_program_snapshot_write_layer_raster_chunk(&writer, ctx);
    if (result.code != CORE_OK) {
        (void)core_pack_writer_close(&writer);
        return result;
    }
    result = drawing_program_snapshot_write_object_chunk(&writer, ctx);
    if (result.code != CORE_OK) {
        (void)core_pack_writer_close(&writer);
        return result;
    }
    result = core_pack_writer_add_chunk(&writer, "DPUI", &ui_settings, (uint64_t)sizeof(ui_settings));
    if (result.code != CORE_OK) {
        (void)core_pack_writer_close(&writer);
        return result;
    }
    result = core_pack_writer_close(&writer);
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
    DrawingProgramSnapshotV1 payload;
    DrawingProgramUiSettingsV1 ui_settings_v1;
    DrawingProgramUiSettingsV2 ui_settings_v2;
    DrawingProgramUiSettingsV3 ui_settings_v3;
    DrawingProgramUiSettingsV4 ui_settings_v4;
    DrawingProgramUiSettingsV5 ui_settings_v5;
    DrawingProgramUiSettingsV6 ui_settings_v6;
    DrawingProgramUiSettingsV7 ui_settings_v7;
    CoreResult result;
    uint8_t *layer_chunk_data = 0;
    uint8_t *object_chunk_data = 0;
    if (!ctx || !path) {
        return snapshot_invalid("invalid snapshot load request");
    }

    memset(&payload, 0, sizeof(payload));
    if (snapshot_trace_enabled()) {
        fprintf(stderr, "drawing_program trace snapshot_load begin path=%s\n", path ? path : "(null)");
    }
    result = core_pack_reader_open(path, &reader);
    if (result.code != CORE_OK) {
        if (snapshot_trace_enabled()) {
            fprintf(stderr,
                    "drawing_program trace snapshot_load fail_open path=%s code=%d message=%s\n",
                    path ? path : "(null)",
                    (int)result.code,
                    result.message ? result.message : "(null)");
        }
        return result;
    }
    result = core_pack_reader_find_chunk(&reader, "DPS2", 0u, &chunk);
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        return result;
    }
    if (chunk.size != (uint64_t)sizeof(payload)) {
        (void)core_pack_reader_close(&reader);
        return (CoreResult){ CORE_ERR_FORMAT, "unexpected drawing snapshot payload size" };
    }
    result = core_pack_reader_read_chunk_data(&reader, &chunk, &payload, (uint64_t)sizeof(payload));
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        return result;
    }
    if (payload.header.version != 1u) {
        return (CoreResult){ CORE_ERR_FORMAT, "unsupported drawing snapshot version" };
    }
    if (payload.header.node_count > DRAWING_PROGRAM_PANE_NODE_CAPACITY ||
        payload.header.binding_count > DRAWING_PROGRAM_MODULE_BINDING_CAPACITY ||
        payload.header.history_count > DRAWING_PROGRAM_HISTORY_CAPACITY ||
        payload.header.history_cursor > payload.header.history_count) {
        return (CoreResult){ CORE_ERR_FORMAT, "invalid drawing snapshot bounds" };
    }

    ctx->document = payload.document;
    ctx->editor = payload.editor;
    drawing_program_object_store_reset(&ctx->object_store);
    result = drawing_program_layer_raster_store_init_from_document(&ctx->layer_rasters, &ctx->document);
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        return result;
    }
    ctx->pane_host.layout_state = payload.layout_state;
    memcpy(ctx->pane_host.nodes, payload.nodes, sizeof(ctx->pane_host.nodes));
    memcpy(ctx->pane_host.module_bindings, payload.bindings, sizeof(ctx->pane_host.module_bindings));
    memcpy(ctx->history.entries, payload.history_entries, sizeof(ctx->history.entries));
    ctx->history.count = payload.header.history_count;
    ctx->history.cursor = payload.header.history_cursor;
    ctx->pane_host.node_count = payload.header.node_count;
    ctx->pane_host.root_index = 0u;
    ctx->pane_host.module_binding_count = payload.header.binding_count;
    memset(&ui_settings_v1, 0, sizeof(ui_settings_v1));
    memset(&ui_settings_v2, 0, sizeof(ui_settings_v2));
    memset(&ui_settings_v3, 0, sizeof(ui_settings_v3));
    memset(&ui_settings_v4, 0, sizeof(ui_settings_v4));
    memset(&ui_settings_v5, 0, sizeof(ui_settings_v5));
    memset(&ui_settings_v6, 0, sizeof(ui_settings_v6));
    memset(&ui_settings_v7, 0, sizeof(ui_settings_v7));
    memset(&layer_chunk, 0, sizeof(layer_chunk));
    memset(&object_chunk, 0, sizeof(object_chunk));
    result = core_pack_reader_find_chunk(&reader, "DPLR", 0u, &layer_chunk);
    if (result.code == CORE_OK) {
        layer_chunk_data = (uint8_t *)malloc((size_t)layer_chunk.size);
        if (!layer_chunk_data) {
            (void)core_pack_reader_close(&reader);
            return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate layer raster chunk buffer" };
        }
        result = core_pack_reader_read_chunk_data(&reader, &layer_chunk, layer_chunk_data, layer_chunk.size);
        if (result.code != CORE_OK) {
            free(layer_chunk_data);
            layer_chunk_data = 0;
            (void)core_pack_reader_close(&reader);
            return result;
        }
        result = drawing_program_snapshot_apply_layer_raster_chunk(ctx, layer_chunk_data, layer_chunk.size);
        free(layer_chunk_data);
        layer_chunk_data = 0;
        if (result.code != CORE_OK) {
            (void)core_pack_reader_close(&reader);
            return result;
        }
    }
    result = core_pack_reader_find_chunk(&reader, "DPOB", 0u, &object_chunk);
    if (result.code == CORE_OK) {
        object_chunk_data = (uint8_t *)malloc((size_t)object_chunk.size);
        if (!object_chunk_data) {
            (void)core_pack_reader_close(&reader);
            return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate object chunk buffer" };
        }
        result = core_pack_reader_read_chunk_data(&reader, &object_chunk, object_chunk_data, object_chunk.size);
        if (result.code != CORE_OK) {
            free(object_chunk_data);
            object_chunk_data = 0;
            (void)core_pack_reader_close(&reader);
            return result;
        }
        result = drawing_program_snapshot_apply_object_chunk(ctx, object_chunk_data, object_chunk.size);
        free(object_chunk_data);
        object_chunk_data = 0;
        if (result.code != CORE_OK) {
            (void)core_pack_reader_close(&reader);
            return result;
        }
    }
    result = core_pack_reader_find_chunk(&reader, "DPUI", 0u, &ui_chunk);
    if (result.code == CORE_OK) {
        if (ui_chunk.size == (uint64_t)sizeof(ui_settings_v7)) {
            result = core_pack_reader_read_chunk_data(&reader, &ui_chunk, &ui_settings_v7, (uint64_t)sizeof(ui_settings_v7));
            if (result.code == CORE_OK &&
                ui_settings_v7.version == DRAWING_PROGRAM_UI_SETTINGS_VERSION_V7) {
                uint8_t entry_count = ui_settings_v7.layer_opacity_entry_count;
                if (ui_settings_v7.theme_preset_id < (uint32_t)CORE_THEME_PRESET_COUNT) {
                    ctx->ui_theme_preset_id = ui_settings_v7.theme_preset_id;
                }
                if (ui_settings_v7.font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
                    ctx->ui_font_preset_id = ui_settings_v7.font_preset_id;
                }
                ctx->ui_font_zoom_step = (int8_t)ui_settings_v7.font_zoom_step;
                ctx->ui_left_panel_slot = ui_settings_v7.left_panel_slot;
                ctx->ui_right_panel_slot = ui_settings_v7.right_panel_slot;
                ctx->ui_active_color_index = ui_settings_v7.active_color_index;
                ctx->ui_tool_brush_size = ui_settings_v7.tool_brush_size;
                ctx->ui_tool_brush_opacity = ui_settings_v7.tool_brush_opacity;
                ctx->ui_tool_brush_spacing = ui_settings_v7.tool_brush_spacing;
                ctx->ui_tool_brush_hardness = ui_settings_v7.tool_brush_hardness;
                ctx->ui_tool_eraser_size = ui_settings_v7.tool_eraser_size;
                ctx->ui_tool_shape_stroke_width = ui_settings_v7.tool_shape_stroke_width;
                ctx->ui_tool_shape_mode = ui_settings_v7.tool_shape_mode;
                ctx->ui_tool_shape_target_mode = ui_settings_v7.tool_shape_target_mode;
                ctx->ui_tool_fill_tolerance = ui_settings_v7.tool_fill_tolerance;
                ctx->ui_tool_select_mode = ui_settings_v7.tool_select_mode;
                if (entry_count > DRAWING_PROGRAM_MAX_LAYERS) {
                    entry_count = DRAWING_PROGRAM_MAX_LAYERS;
                }
                ctx->ui_layer_opacity_entry_count = entry_count;
                memcpy(ctx->ui_layer_opacity_layer_ids,
                       ui_settings_v7.layer_opacity_layer_ids,
                       sizeof(ctx->ui_layer_opacity_layer_ids));
                memcpy(ctx->ui_layer_opacity_values,
                       ui_settings_v7.layer_opacity_values,
                       sizeof(ctx->ui_layer_opacity_values));
                if (ui_settings_v7.selection_has_payload) {
                    (void)drawing_program_selection_capture_from_rect(&ctx->document,
                                                                      &ctx->layer_rasters,
                                                                      ctx->editor.active_layer_id,
                                                                      &ctx->selection,
                                                                      (int32_t)ui_settings_v7.selection_origin_x,
                                                                      (int32_t)ui_settings_v7.selection_origin_y,
                                                                      ui_settings_v7.selection_width,
                                                                      ui_settings_v7.selection_height);
                } else {
                    drawing_program_selection_reset(&ctx->selection);
                }
            }
        } else if (ui_chunk.size == (uint64_t)sizeof(ui_settings_v6)) {
            result = core_pack_reader_read_chunk_data(&reader, &ui_chunk, &ui_settings_v6, (uint64_t)sizeof(ui_settings_v6));
            if (result.code == CORE_OK &&
                ui_settings_v6.version == DRAWING_PROGRAM_UI_SETTINGS_VERSION_V6) {
                uint8_t entry_count = ui_settings_v6.layer_opacity_entry_count;
                if (ui_settings_v6.theme_preset_id < (uint32_t)CORE_THEME_PRESET_COUNT) {
                    ctx->ui_theme_preset_id = ui_settings_v6.theme_preset_id;
                }
                if (ui_settings_v6.font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
                    ctx->ui_font_preset_id = ui_settings_v6.font_preset_id;
                }
                ctx->ui_font_zoom_step = (int8_t)ui_settings_v6.font_zoom_step;
                ctx->ui_left_panel_slot = ui_settings_v6.left_panel_slot;
                ctx->ui_right_panel_slot = ui_settings_v6.right_panel_slot;
                ctx->ui_active_color_index = ui_settings_v6.active_color_index;
                ctx->ui_tool_brush_size = ui_settings_v6.tool_brush_size;
                ctx->ui_tool_brush_opacity = ui_settings_v6.tool_brush_opacity;
                ctx->ui_tool_brush_spacing = ui_settings_v6.tool_brush_spacing;
                ctx->ui_tool_brush_hardness = ui_settings_v6.tool_brush_hardness;
                ctx->ui_tool_eraser_size = ui_settings_v6.tool_eraser_size;
                ctx->ui_tool_shape_stroke_width = ui_settings_v6.tool_shape_stroke_width;
                ctx->ui_tool_shape_mode = ui_settings_v6.tool_shape_mode;
                ctx->ui_tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
                ctx->ui_tool_fill_tolerance = ui_settings_v6.tool_fill_tolerance;
                ctx->ui_tool_select_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
                if (entry_count > DRAWING_PROGRAM_MAX_LAYERS) {
                    entry_count = DRAWING_PROGRAM_MAX_LAYERS;
                }
                ctx->ui_layer_opacity_entry_count = entry_count;
                memcpy(ctx->ui_layer_opacity_layer_ids,
                       ui_settings_v6.layer_opacity_layer_ids,
                       sizeof(ctx->ui_layer_opacity_layer_ids));
                memcpy(ctx->ui_layer_opacity_values,
                       ui_settings_v6.layer_opacity_values,
                       sizeof(ctx->ui_layer_opacity_values));
                if (ui_settings_v6.selection_has_payload) {
                    (void)drawing_program_selection_capture_from_rect(&ctx->document,
                                                                      &ctx->layer_rasters,
                                                                      ctx->editor.active_layer_id,
                                                                      &ctx->selection,
                                                                      (int32_t)ui_settings_v6.selection_origin_x,
                                                                      (int32_t)ui_settings_v6.selection_origin_y,
                                                                      ui_settings_v6.selection_width,
                                                                      ui_settings_v6.selection_height);
                } else {
                    drawing_program_selection_reset(&ctx->selection);
                }
            }
        } else if (ui_chunk.size == (uint64_t)sizeof(ui_settings_v5)) {
            result = core_pack_reader_read_chunk_data(&reader, &ui_chunk, &ui_settings_v5, (uint64_t)sizeof(ui_settings_v5));
            if (result.code == CORE_OK &&
                ui_settings_v5.version == DRAWING_PROGRAM_UI_SETTINGS_VERSION_V5) {
                if (ui_settings_v5.theme_preset_id < (uint32_t)CORE_THEME_PRESET_COUNT) {
                    ctx->ui_theme_preset_id = ui_settings_v5.theme_preset_id;
                }
                if (ui_settings_v5.font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
                    ctx->ui_font_preset_id = ui_settings_v5.font_preset_id;
                }
                ctx->ui_font_zoom_step = (int8_t)ui_settings_v5.font_zoom_step;
                ctx->ui_left_panel_slot = ui_settings_v5.left_panel_slot;
                ctx->ui_right_panel_slot = ui_settings_v5.right_panel_slot;
                ctx->ui_active_color_index = ui_settings_v5.active_color_index;
                ctx->ui_tool_brush_size = ui_settings_v5.tool_brush_size;
                ctx->ui_tool_brush_opacity = ui_settings_v5.tool_brush_opacity;
                ctx->ui_tool_brush_spacing = 2u;
                ctx->ui_tool_brush_hardness = 100u;
                ctx->ui_tool_eraser_size = ui_settings_v5.tool_eraser_size;
                ctx->ui_tool_shape_stroke_width = ui_settings_v5.tool_shape_stroke_width;
                ctx->ui_tool_shape_mode = ui_settings_v5.tool_shape_mode;
                ctx->ui_tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
                ctx->ui_tool_fill_tolerance = ui_settings_v5.tool_fill_tolerance;
                ctx->ui_tool_select_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
                if (ui_settings_v5.selection_has_payload) {
                    (void)drawing_program_selection_capture_from_rect(&ctx->document,
                                                                      &ctx->layer_rasters,
                                                                      ctx->editor.active_layer_id,
                                                                      &ctx->selection,
                                                                      (int32_t)ui_settings_v5.selection_origin_x,
                                                                      (int32_t)ui_settings_v5.selection_origin_y,
                                                                      ui_settings_v5.selection_width,
                                                                      ui_settings_v5.selection_height);
                } else {
                    drawing_program_selection_reset(&ctx->selection);
                }
            }
        } else if (ui_chunk.size == (uint64_t)sizeof(ui_settings_v4)) {
            result = core_pack_reader_read_chunk_data(&reader, &ui_chunk, &ui_settings_v4, (uint64_t)sizeof(ui_settings_v4));
            if (result.code == CORE_OK &&
                ui_settings_v4.version == DRAWING_PROGRAM_UI_SETTINGS_VERSION_V4) {
                if (ui_settings_v4.theme_preset_id < (uint32_t)CORE_THEME_PRESET_COUNT) {
                    ctx->ui_theme_preset_id = ui_settings_v4.theme_preset_id;
                }
                if (ui_settings_v4.font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
                    ctx->ui_font_preset_id = ui_settings_v4.font_preset_id;
                }
                ctx->ui_font_zoom_step = (int8_t)ui_settings_v4.font_zoom_step;
                ctx->ui_left_panel_slot = ui_settings_v4.left_panel_slot;
                ctx->ui_right_panel_slot = ui_settings_v4.right_panel_slot;
                ctx->ui_active_color_index = ui_settings_v4.active_color_index;
                ctx->ui_tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
                ctx->ui_tool_select_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
                if (ui_settings_v4.selection_has_payload) {
                    (void)drawing_program_selection_capture_from_rect(&ctx->document,
                                                                      &ctx->layer_rasters,
                                                                      ctx->editor.active_layer_id,
                                                                      &ctx->selection,
                                                                      (int32_t)ui_settings_v4.selection_origin_x,
                                                                      (int32_t)ui_settings_v4.selection_origin_y,
                                                                      ui_settings_v4.selection_width,
                                                                      ui_settings_v4.selection_height);
                } else {
                    drawing_program_selection_reset(&ctx->selection);
                }
            }
        } else if (ui_chunk.size == (uint64_t)sizeof(ui_settings_v3)) {
            result = core_pack_reader_read_chunk_data(&reader, &ui_chunk, &ui_settings_v3, (uint64_t)sizeof(ui_settings_v3));
            if (result.code == CORE_OK &&
                ui_settings_v3.version == DRAWING_PROGRAM_UI_SETTINGS_VERSION_V3) {
                if (ui_settings_v3.theme_preset_id < (uint32_t)CORE_THEME_PRESET_COUNT) {
                    ctx->ui_theme_preset_id = ui_settings_v3.theme_preset_id;
                }
                if (ui_settings_v3.font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
                    ctx->ui_font_preset_id = ui_settings_v3.font_preset_id;
                }
                ctx->ui_font_zoom_step = (int8_t)ui_settings_v3.font_zoom_step;
                ctx->ui_left_panel_slot = ui_settings_v3.left_panel_slot;
                ctx->ui_right_panel_slot = ui_settings_v3.right_panel_slot;
                ctx->ui_active_color_index = ui_settings_v3.active_color_index;
                ctx->ui_tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
                ctx->ui_tool_select_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
                drawing_program_selection_reset(&ctx->selection);
            }
        } else if (ui_chunk.size == (uint64_t)sizeof(ui_settings_v2)) {
            result = core_pack_reader_read_chunk_data(&reader, &ui_chunk, &ui_settings_v2, (uint64_t)sizeof(ui_settings_v2));
            if (result.code == CORE_OK &&
                ui_settings_v2.version == DRAWING_PROGRAM_UI_SETTINGS_VERSION_V2) {
                if (ui_settings_v2.theme_preset_id < (uint32_t)CORE_THEME_PRESET_COUNT) {
                    ctx->ui_theme_preset_id = ui_settings_v2.theme_preset_id;
                }
                if (ui_settings_v2.font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
                    ctx->ui_font_preset_id = ui_settings_v2.font_preset_id;
                }
                ctx->ui_font_zoom_step = (int8_t)ui_settings_v2.font_zoom_step;
                ctx->ui_left_panel_slot = ui_settings_v2.left_panel_slot;
                ctx->ui_right_panel_slot = ui_settings_v2.right_panel_slot;
                ctx->ui_active_color_index = drawing_program_color_default_index();
                ctx->ui_tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
                ctx->ui_tool_select_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
                drawing_program_selection_reset(&ctx->selection);
            }
        } else if (ui_chunk.size == (uint64_t)sizeof(ui_settings_v1)) {
            result = core_pack_reader_read_chunk_data(&reader, &ui_chunk, &ui_settings_v1, (uint64_t)sizeof(ui_settings_v1));
            if (result.code == CORE_OK &&
                ui_settings_v1.version == DRAWING_PROGRAM_UI_SETTINGS_VERSION_V1) {
                if (ui_settings_v1.theme_preset_id < (uint32_t)CORE_THEME_PRESET_COUNT) {
                    ctx->ui_theme_preset_id = ui_settings_v1.theme_preset_id;
                }
                ctx->ui_left_panel_slot = ui_settings_v1.left_panel_slot;
                ctx->ui_right_panel_slot = ui_settings_v1.right_panel_slot;
                ctx->ui_active_color_index = drawing_program_color_default_index();
                ctx->ui_tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
                ctx->ui_tool_select_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
                drawing_program_selection_reset(&ctx->selection);
            }
        }
    }
    (void)core_pack_reader_close(&reader);
    result = drawing_program_pane_host_rebuild(ctx);
    if (snapshot_trace_enabled()) {
        fprintf(stderr,
                "drawing_program trace snapshot_load end path=%s code=%d tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u color=%u leafs=%u\n",
                path ? path : "(null)",
                (int)result.code,
                (unsigned)ctx->editor.active_tool,
                (unsigned)ctx->ui_theme_preset_id,
                (unsigned)ctx->ui_font_preset_id,
                (int)ctx->ui_font_zoom_step,
                (unsigned)ctx->ui_left_panel_slot,
                (unsigned)ctx->ui_right_panel_slot,
                (unsigned)ctx->ui_active_color_index,
                (unsigned)ctx->pane_host.leaf_count);
    }
    return result;
}

CoreResult drawing_program_snapshot_export_debug_json(const struct DrawingProgramAppContext *ctx,
                                                      const char *path) {
    FILE *f;
    uint32_t i;
    if (!ctx || !path) {
        return snapshot_invalid("invalid snapshot export request");
    }
    f = fopen(path, "wb");
    if (!f) {
        return (CoreResult){ CORE_ERR_IO, "failed to open snapshot json output" };
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"schema\": 1,\n");
    fprintf(f, "  \"document\": {\n");
    fprintf(f, "    \"schema_version\": %u,\n", ctx->document.schema_version);
    fprintf(f, "    \"logical_width\": %u,\n", ctx->document.logical_width);
    fprintf(f, "    \"logical_height\": %u,\n", ctx->document.logical_height);
    fprintf(f, "    \"sample_density\": %u,\n", ctx->document.sample_density);
    fprintf(f, "    \"layer_count\": %u,\n", ctx->document.layer_count);
    fprintf(f, "    \"raster_sample_count\": %u\n", ctx->document.raster_sample_count);
    fprintf(f, "  },\n");
    fprintf(f, "  \"layer_raster\": {\n");
    fprintf(f, "    \"slot_capacity\": %u,\n", ctx->layer_rasters.slot_capacity);
    fprintf(f, "    \"slot_count\": %u,\n", ctx->layer_rasters.slot_count);
    fprintf(f, "    \"sample_count\": %u\n", ctx->layer_rasters.sample_count);
    fprintf(f, "  },\n");
    fprintf(f, "  \"objects\": {\n");
    fprintf(f, "    \"schema_version\": %u,\n", (unsigned)ctx->object_store.schema_version);
    fprintf(f, "    \"count\": %u,\n", (unsigned)ctx->object_store.object_count);
    fprintf(f, "    \"next_object_id\": %u,\n", (unsigned)ctx->object_store.next_object_id);
    fprintf(f, "    \"items\": [\n");
    for (i = 0u; i < ctx->object_store.object_count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        const DrawingProgramObjectRecord *object = &ctx->object_store.objects[i];
        fprintf(f,
                "      {\"id\": %u, \"layer_id\": %u, \"type\": %u, \"visible\": %u, \"locked\": %u, \"x\": %d, \"y\": %d, \"w\": %u, \"h\": %u, \"path_point_count\": %u, \"path_closed\": %u}%s\n",
                (unsigned)object->object_id,
                (unsigned)object->layer_id,
                (unsigned)object->type,
                (unsigned)object->visible,
                (unsigned)object->locked,
                (int)object->origin_x,
                (int)object->origin_y,
                (unsigned)object->width,
                (unsigned)object->height,
                (unsigned)object->path_point_count,
                (unsigned)object->path_closed,
                ((i + 1u) < ctx->object_store.object_count && (i + 1u) < DRAWING_PROGRAM_MAX_OBJECTS) ? "," : "");
    }
    fprintf(f, "    ]\n");
    fprintf(f, "  },\n");
    fprintf(f, "  \"editor\": {\n");
    fprintf(f, "    \"active_tool\": %u,\n", (unsigned)ctx->editor.active_tool);
    fprintf(f, "    \"active_layer_id\": %u,\n", ctx->editor.active_layer_id);
    fprintf(f, "    \"zoom\": %.3f\n", (double)ctx->editor.viewport.zoom);
    fprintf(f, "  },\n");
    fprintf(f, "  \"ui\": {\n");
    fprintf(f, "    \"theme_preset_id\": %u,\n", ctx->ui_theme_preset_id);
    fprintf(f, "    \"font_preset_id\": %u,\n", ctx->ui_font_preset_id);
    fprintf(f, "    \"font_zoom_step\": %d,\n", (int)ctx->ui_font_zoom_step);
    fprintf(f, "    \"left_panel_slot\": %u,\n", (unsigned)ctx->ui_left_panel_slot);
    fprintf(f, "    \"right_panel_slot\": %u,\n", (unsigned)ctx->ui_right_panel_slot);
    fprintf(f, "    \"active_color_index\": %u,\n", (unsigned)ctx->ui_active_color_index);
    fprintf(f, "    \"active_color_value\": %u\n",
            (unsigned)drawing_program_color_value_from_index(ctx->ui_active_color_index));
    fprintf(f, "  },\n");
    fprintf(f, "  \"tool_settings\": {\n");
    fprintf(f, "    \"brush_size\": %u,\n", (unsigned)ctx->ui_tool_brush_size);
    fprintf(f, "    \"brush_opacity\": %u,\n", (unsigned)ctx->ui_tool_brush_opacity);
    fprintf(f, "    \"brush_spacing\": %u,\n", (unsigned)ctx->ui_tool_brush_spacing);
    fprintf(f, "    \"brush_hardness\": %u,\n", (unsigned)ctx->ui_tool_brush_hardness);
    fprintf(f, "    \"eraser_size\": %u,\n", (unsigned)ctx->ui_tool_eraser_size);
    fprintf(f, "    \"shape_stroke_width\": %u,\n", (unsigned)ctx->ui_tool_shape_stroke_width);
    fprintf(f, "    \"shape_mode\": %u,\n", (unsigned)ctx->ui_tool_shape_mode);
    fprintf(f, "    \"shape_target_mode\": %u,\n", (unsigned)ctx->ui_tool_shape_target_mode);
    fprintf(f, "    \"fill_tolerance\": %u,\n", (unsigned)ctx->ui_tool_fill_tolerance);
    fprintf(f, "    \"select_mode\": %u\n", (unsigned)ctx->ui_tool_select_mode);
    fprintf(f, "  },\n");
    fprintf(f, "  \"layer_ui\": {\n");
    fprintf(f, "    \"opacity_entry_count\": %u,\n", (unsigned)ctx->ui_layer_opacity_entry_count);
    fprintf(f, "    \"opacity_entries\": [\n");
    for (i = 0u; i < ctx->ui_layer_opacity_entry_count && i < DRAWING_PROGRAM_MAX_LAYERS; ++i) {
        fprintf(f,
                "      {\"layer_id\": %u, \"opacity\": %u}%s\n",
                (unsigned)ctx->ui_layer_opacity_layer_ids[i],
                (unsigned)ctx->ui_layer_opacity_values[i],
                ((i + 1u) < ctx->ui_layer_opacity_entry_count && (i + 1u) < DRAWING_PROGRAM_MAX_LAYERS) ? "," : "");
    }
    fprintf(f, "    ]\n");
    fprintf(f, "  },\n");
    fprintf(f, "  \"selection\": {\n");
    fprintf(f, "    \"has_payload\": %u,\n", (unsigned)ctx->selection.has_payload);
    fprintf(f, "    \"selecting\": %u,\n", (unsigned)ctx->selection.selecting);
    fprintf(f, "    \"moving\": %u,\n", (unsigned)ctx->selection.moving);
    fprintf(f, "    \"origin_x\": %u,\n", (unsigned)ctx->selection.origin_x);
    fprintf(f, "    \"origin_y\": %u,\n", (unsigned)ctx->selection.origin_y);
    fprintf(f, "    \"width\": %u,\n", (unsigned)ctx->selection.width);
    fprintf(f, "    \"height\": %u,\n", (unsigned)ctx->selection.height);
    fprintf(f, "    \"offset_x\": %d,\n", (int)ctx->selection.offset_x);
    fprintf(f, "    \"offset_y\": %d,\n", (int)ctx->selection.offset_y);
    fprintf(f, "    \"payload_count\": %u\n", (unsigned)ctx->selection.payload_count);
    fprintf(f, "  },\n");
    fprintf(f, "  \"pane\": {\n");
    fprintf(f, "    \"node_count\": %u,\n", ctx->pane_host.node_count);
    fprintf(f, "    \"leaf_count\": %u,\n", ctx->pane_host.leaf_count);
    fprintf(f, "    \"module_binding_count\": %u,\n", ctx->pane_host.module_binding_count);
    fprintf(f, "    \"leaves\": [\n");
    for (i = 0u; i < ctx->pane_host.leaf_count; ++i) {
        const CorePaneLeafRect *leaf = &ctx->pane_host.leaves[i];
        fprintf(f,
                "      {\"id\": %u, \"x\": %.1f, \"y\": %.1f, \"w\": %.1f, \"h\": %.1f}%s\n",
                (unsigned)leaf->id,
                (double)leaf->rect.x,
                (double)leaf->rect.y,
                (double)leaf->rect.width,
                (double)leaf->rect.height,
                (i + 1u < ctx->pane_host.leaf_count) ? "," : "");
    }
    fprintf(f, "    ]\n");
    fprintf(f, "  }\n");
    fprintf(f, "}\n");

    if (fclose(f) != 0) {
        return (CoreResult){ CORE_ERR_IO, "failed to close snapshot json output" };
    }
    return core_result_ok();
}
