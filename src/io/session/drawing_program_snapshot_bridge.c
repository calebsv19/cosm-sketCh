#include "drawing_program/drawing_program_snapshot.h"

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
    result = drawing_program_pane_host_rebind_default_modules(ctx);
    if (result.code != CORE_OK) {
        return result;
    }
    return drawing_program_pane_host_rebuild(ctx);
}
