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

enum {
    DRAWING_PROGRAM_WORKSPACE_MAX_NODES = 32u,
    DRAWING_PROGRAM_WORKSPACE_PRESET_VERSION_V1 = 1u,
    DRAWING_PROGRAM_WORKSPACE_PRESET_VERSION_V2 = 2u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V1 = 1u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V2 = 2u,
    DRAWING_PROGRAM_UI_SETTINGS_VERSION_V3 = 3u
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
    DrawingProgramUiSettingsV3 ui_settings;
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
    ui_settings.version = DRAWING_PROGRAM_UI_SETTINGS_VERSION_V3;
    ui_settings.theme_preset_id = ctx->ui_theme_preset_id;
    ui_settings.font_preset_id = ctx->ui_font_preset_id;
    ui_settings.font_zoom_step = (int32_t)ctx->ui_font_zoom_step;
    ui_settings.left_panel_slot = ctx->ui_left_panel_slot;
    ui_settings.right_panel_slot = ctx->ui_right_panel_slot;
    ui_settings.active_color_index = ctx->ui_active_color_index;

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
    DrawingProgramSnapshotV1 payload;
    DrawingProgramUiSettingsV1 ui_settings_v1;
    DrawingProgramUiSettingsV2 ui_settings_v2;
    DrawingProgramUiSettingsV3 ui_settings_v3;
    CoreResult result;
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
    result = core_pack_reader_find_chunk(&reader, "DPUI", 0u, &ui_chunk);
    if (result.code == CORE_OK) {
        if (ui_chunk.size == (uint64_t)sizeof(ui_settings_v3)) {
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
    fprintf(f, "    \"logical_width\": %u,\n", ctx->document.logical_width);
    fprintf(f, "    \"logical_height\": %u,\n", ctx->document.logical_height);
    fprintf(f, "    \"sample_density\": %u,\n", ctx->document.sample_density);
    fprintf(f, "    \"layer_count\": %u\n", ctx->document.layer_count);
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
