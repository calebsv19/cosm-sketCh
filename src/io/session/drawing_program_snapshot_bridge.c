#include "drawing_program/drawing_program_snapshot.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "core_pack.h"
#include "drawing_program/drawing_program_app_main.h"

enum {
    DRAWING_PROGRAM_WORKSPACE_PRESET_VERSION_V1 = 1u,
    DRAWING_PROGRAM_WORKSPACE_PRESET_VERSION_V2 = 2u
};

static CoreResult snapshot_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
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
    uint64_t nodes_bytes = sizeof(CorePaneNode) * DRAWING_PROGRAM_PANE_NODE_CAPACITY;
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
