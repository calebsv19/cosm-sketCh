#include "drawing_program/drawing_program_snapshot.h"

#include <math.h>
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

enum {
    DRAWING_PROGRAM_WORKSPACE_MAX_NODES = 32u,
    DRAWING_PROGRAM_WORKSPACE_PRESET_VERSION_V1 = 1u,
    DRAWING_PROGRAM_WORKSPACE_PRESET_VERSION_V2 = 2u,
    DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V1 = 1u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V1 = 1u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V2 = 2u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V3 = 3u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V4 = 4u
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

static CoreResult drawing_program_rebind_imported_modules(struct DrawingProgramAppContext *ctx) {
    uint32_t menu_idx = 0u;
    uint32_t i;
    uint32_t candidate_indices[DRAWING_PROGRAM_PANE_LEAF_CAPACITY];
    uint32_t candidate_count = 0u;
    uint32_t left_idx;
    uint32_t right_idx;
    uint32_t center_idx = 0u;
    float min_x = 0.0f;
    float max_x = 0.0f;
    float target_mid = 0.0f;
    float best_distance = 0.0f;
    int center_set = 0;
    if (!ctx) {
        return snapshot_invalid("invalid module rebind request");
    }
    if (ctx->pane_host.leaf_count < 4u) {
        return (CoreResult){ CORE_ERR_FORMAT, "workspace import requires at least 4 pane leaves" };
    }

    menu_idx = 0u;
    for (i = 1u; i < ctx->pane_host.leaf_count; ++i) {
        const CorePaneLeafRect *candidate = &ctx->pane_host.leaves[i];
        const CorePaneLeafRect *best = &ctx->pane_host.leaves[menu_idx];
        if (candidate->rect.y < best->rect.y ||
            (candidate->rect.y == best->rect.y && candidate->rect.width > best->rect.width)) {
            menu_idx = i;
        }
    }

    for (i = 0u; i < ctx->pane_host.leaf_count; ++i) {
        if (i == menu_idx) {
            continue;
        }
        candidate_indices[candidate_count++] = i;
    }
    if (candidate_count < 3u) {
        return (CoreResult){ CORE_ERR_FORMAT, "workspace import missing side/canvas panes" };
    }

    left_idx = candidate_indices[0];
    right_idx = candidate_indices[0];
    min_x = ctx->pane_host.leaves[left_idx].rect.x;
    max_x = min_x;
    for (i = 1u; i < candidate_count; ++i) {
        uint32_t idx = candidate_indices[i];
        float x = ctx->pane_host.leaves[idx].rect.x;
        if (x < min_x) {
            min_x = x;
            left_idx = idx;
        }
        if (x > max_x) {
            max_x = x;
            right_idx = idx;
        }
    }

    target_mid = (min_x + max_x) * 0.5f;
    for (i = 0u; i < candidate_count; ++i) {
        uint32_t idx = candidate_indices[i];
        float center_x;
        float distance;
        if (idx == left_idx || idx == right_idx) {
            continue;
        }
        center_x = ctx->pane_host.leaves[idx].rect.x + (ctx->pane_host.leaves[idx].rect.width * 0.5f);
        distance = fabsf(center_x - target_mid);
        if (!center_set || distance < best_distance) {
            center_set = 1;
            best_distance = distance;
            center_idx = idx;
        }
    }
    if (!center_set) {
        for (i = 0u; i < candidate_count; ++i) {
            uint32_t idx = candidate_indices[i];
            if (idx != left_idx && idx != right_idx) {
                center_idx = idx;
                center_set = 1;
                break;
            }
        }
    }
    if (!center_set) {
        return (CoreResult){ CORE_ERR_FORMAT, "workspace import failed to resolve center pane" };
    }
    if (left_idx == right_idx || center_idx == left_idx || center_idx == right_idx) {
        return (CoreResult){ CORE_ERR_FORMAT, "workspace import pane role resolution collision" };
    }

    ctx->pane_host.module_binding_count = 4u;
    ctx->pane_host.module_bindings[0] = (CorePaneModuleBinding){
        .instance_id = 1u,
        .pane_node_id = (uint32_t)ctx->pane_host.leaves[menu_idx].id,
        .module_type_id = 3u,
        .config_variant = 0u,
        .runtime_flags = 0u
    };
    ctx->pane_host.module_bindings[1] = (CorePaneModuleBinding){
        .instance_id = 2u,
        .pane_node_id = (uint32_t)ctx->pane_host.leaves[left_idx].id,
        .module_type_id = 2u,
        .config_variant = 0u,
        .runtime_flags = 0u
    };
    ctx->pane_host.module_bindings[2] = (CorePaneModuleBinding){
        .instance_id = 3u,
        .pane_node_id = (uint32_t)ctx->pane_host.leaves[center_idx].id,
        .module_type_id = 1u,
        .config_variant = 0u,
        .runtime_flags = 0u
    };
    ctx->pane_host.module_bindings[3] = (CorePaneModuleBinding){
        .instance_id = 4u,
        .pane_node_id = (uint32_t)ctx->pane_host.leaves[right_idx].id,
        .module_type_id = 4u,
        .config_variant = 0u,
        .runtime_flags = 0u
    };

    return core_result_ok();
}

CoreResult drawing_program_snapshot_save(const struct DrawingProgramAppContext *ctx, const char *path) {
    CorePackWriter writer;
    DrawingProgramSnapshotV1 payload;
    DrawingProgramUiSettingsV4 ui_settings;
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
    ui_settings.version = DRAWING_PROGRAM_UI_SETTINGS_VERSION_V4;
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
    DrawingProgramSnapshotV1 payload;
    DrawingProgramUiSettingsV1 ui_settings_v1;
    DrawingProgramUiSettingsV2 ui_settings_v2;
    DrawingProgramUiSettingsV3 ui_settings_v3;
    DrawingProgramUiSettingsV4 ui_settings_v4;
    CoreResult result;
    uint8_t *layer_chunk_data = 0;
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
    memset(&layer_chunk, 0, sizeof(layer_chunk));
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
    result = core_pack_reader_find_chunk(&reader, "DPUI", 0u, &ui_chunk);
    if (result.code == CORE_OK) {
        if (ui_chunk.size == (uint64_t)sizeof(ui_settings_v4)) {
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

CoreResult drawing_program_snapshot_bridge_check_workspace_preset(const char *workspace_preset_path) {
    CorePackReader reader;
    CorePackChunkInfo chunk;
    CoreResult result;
    if (!workspace_preset_path) {
        return snapshot_invalid("invalid workspace preset bridge request");
    }
    result = core_pack_reader_open(workspace_preset_path, &reader);
    if (result.code != CORE_OK) {
        return result;
    }
    result = core_pack_reader_find_chunk(&reader, "WSPS", 0u, &chunk);
    (void)core_pack_reader_close(&reader);
    if (result.code != CORE_OK) {
        return (CoreResult){ CORE_ERR_FORMAT, "workspace preset missing WSPS chunk" };
    }
    return core_result_ok();
}

CoreResult drawing_program_snapshot_bridge_import_workspace_preset(struct DrawingProgramAppContext *ctx,
                                                                   const char *workspace_preset_path) {
    CorePackReader reader;
    CorePackChunkInfo chunk;
    CoreResult result;
    uint8_t *payload = 0;
    uint32_t version = 0u;
    uint32_t node_count = 0u;
    uint32_t root_index = 0u;
    uint64_t offset = 0u;
    uint64_t nodes_bytes = sizeof(CorePaneNode) * DRAWING_PROGRAM_WORKSPACE_MAX_NODES;
    uint64_t required = 0u;
    if (!ctx || !workspace_preset_path) {
        return snapshot_invalid("invalid workspace preset import request");
    }
    result = core_pack_reader_open(workspace_preset_path, &reader);
    if (result.code != CORE_OK) {
        return result;
    }
    result = core_pack_reader_find_chunk(&reader, "WSPS", 0u, &chunk);
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        return (CoreResult){ CORE_ERR_FORMAT, "workspace preset missing WSPS chunk" };
    }

    payload = (uint8_t *)malloc((size_t)chunk.size);
    if (!payload) {
        (void)core_pack_reader_close(&reader);
        return (CoreResult){ CORE_ERR_IO, "workspace import allocation failed" };
    }
    result = core_pack_reader_read_chunk_data(&reader, &chunk, payload, chunk.size);
    (void)core_pack_reader_close(&reader);
    if (result.code != CORE_OK) {
        free(payload);
        return result;
    }

    if (chunk.size < sizeof(uint32_t)) {
        free(payload);
        return (CoreResult){ CORE_ERR_FORMAT, "workspace preset payload too small" };
    }
    memcpy(&version, payload, sizeof(uint32_t));
    if (version == DRAWING_PROGRAM_WORKSPACE_PRESET_VERSION_V1 ||
        version == DRAWING_PROGRAM_WORKSPACE_PRESET_VERSION_V2) {
        offset = sizeof(uint32_t);
    } else {
        free(payload);
        return (CoreResult){ CORE_ERR_FORMAT, "unsupported workspace preset version" };
    }
    required = offset + nodes_bytes + sizeof(uint32_t) + sizeof(uint32_t);
    if (chunk.size < required) {
        free(payload);
        return (CoreResult){ CORE_ERR_FORMAT, "workspace preset payload too small for layout" };
    }

    memcpy(ctx->pane_host.nodes, payload + offset, sizeof(ctx->pane_host.nodes));
    memcpy(&node_count, payload + offset + nodes_bytes, sizeof(uint32_t));
    memcpy(&root_index, payload + offset + nodes_bytes + sizeof(uint32_t), sizeof(uint32_t));
    free(payload);
    payload = 0;

    if (node_count == 0u ||
        node_count > DRAWING_PROGRAM_PANE_NODE_CAPACITY ||
        root_index >= node_count) {
        return (CoreResult){ CORE_ERR_FORMAT, "workspace import layout exceeds drawing capacity" };
    }
    ctx->pane_host.node_count = node_count;
    ctx->pane_host.root_index = root_index;
    ctx->pane_host.module_binding_count = 0u;
    result = drawing_program_pane_host_rebuild(ctx);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_rebind_imported_modules(ctx);
    if (result.code != CORE_OK) {
        return result;
    }
    return drawing_program_pane_host_rebuild(ctx);
}
