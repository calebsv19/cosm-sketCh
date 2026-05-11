#include "drawing_program/drawing_program_snapshot_shell.h"

#include <string.h>
#include <stdlib.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_authoring_host.h"

typedef struct DrawingProgramSnapshotShellHeaderV2 {
    uint32_t version;
    uint32_t reserved0;
    uint32_t node_count;
    uint32_t binding_count;
    uint32_t history_count;
    uint32_t history_cursor;
    uint32_t schema_version;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    uint32_t layer_count;
    uint32_t next_layer_id;
    uint32_t raster_width;
    uint32_t raster_height;
    uint32_t raster_sample_count;
} DrawingProgramSnapshotShellHeaderV2;

typedef struct DrawingProgramSnapshotShellV2 {
    DrawingProgramSnapshotShellHeaderV2 header;
    DrawingProgramLayer layers[DRAWING_PROGRAM_MAX_LAYERS];
    DrawingProgramEditorState editor;
    CoreLayoutState layout_state;
    CorePaneNode nodes[DRAWING_PROGRAM_PANE_NODE_CAPACITY];
    CorePaneModuleBinding bindings[DRAWING_PROGRAM_MODULE_BINDING_CAPACITY];
    DrawingProgramCommand history_entries[DRAWING_PROGRAM_HISTORY_CAPACITY];
} DrawingProgramSnapshotShellV2;

enum {
    DRAWING_PROGRAM_SNAPSHOT_SHELL_VERSION_V2 = 2u
};

static CoreResult drawing_program_snapshot_shell_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

const char *drawing_program_snapshot_shell_chunk_id_current(void) {
    return "DPS3";
}

CoreResult drawing_program_snapshot_shell_write_current(
    CorePackWriter *writer,
    const struct DrawingProgramAppContext *ctx) {
    DrawingProgramSnapshotShellV2 *payload = 0;
    CoreResult result;
    uint32_t accepted_root_index = 0u;
    if (!writer || !ctx) {
        return drawing_program_snapshot_shell_invalid("invalid snapshot shell write request");
    }
    payload = (DrawingProgramSnapshotShellV2 *)calloc(1u, sizeof(*payload));
    if (!payload) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate snapshot shell payload" };
    }
    payload->header.version = DRAWING_PROGRAM_SNAPSHOT_SHELL_VERSION_V2;
    payload->header.history_count = ctx->history.count;
    payload->header.history_cursor = ctx->history.cursor;
    payload->header.schema_version = ctx->document.schema_version;
    payload->header.logical_width = ctx->document.logical_width;
    payload->header.logical_height = ctx->document.logical_height;
    payload->header.sample_density = ctx->document.sample_density;
    payload->header.layer_count = ctx->document.layer_count;
    payload->header.next_layer_id = ctx->document.next_layer_id;
    payload->header.raster_width = ctx->document.raster_width;
    payload->header.raster_height = ctx->document.raster_height;
    payload->header.raster_sample_count = ctx->document.raster_sample_count;
    memcpy(payload->layers, ctx->document.layers, sizeof(payload->layers));
    payload->editor = ctx->editor;
    result = drawing_program_authoring_host_export_accepted_pane_state(ctx,
                                                                       &payload->layout_state,
                                                                       payload->nodes,
                                                                       &payload->header.node_count,
                                                                       &accepted_root_index,
                                                                       payload->bindings,
                                                                       &payload->header.binding_count);
    if (result.code != CORE_OK) {
        free(payload);
        return result;
    }
    (void)accepted_root_index;
    memcpy(payload->history_entries, ctx->history.entries, sizeof(payload->history_entries));
    result = core_pack_writer_add_chunk(writer,
                                        drawing_program_snapshot_shell_chunk_id_current(),
                                        payload,
                                        (uint64_t)sizeof(*payload));
    free(payload);
    return result;
}

CoreResult drawing_program_snapshot_shell_load_current(
    struct DrawingProgramAppContext *ctx,
    CorePackReader *reader,
    uint8_t *out_found) {
    CorePackChunkInfo chunk;
    DrawingProgramSnapshotShellV2 *payload = 0;
    CoreResult result;
    uint32_t i;
    if (out_found) {
        *out_found = 0u;
    }
    if (!ctx || !reader) {
        return drawing_program_snapshot_shell_invalid("invalid snapshot shell load request");
    }
    memset(&chunk, 0, sizeof(chunk));
    result = core_pack_reader_find_chunk(reader, drawing_program_snapshot_shell_chunk_id_current(), 0u, &chunk);
    if (result.code == CORE_ERR_NOT_FOUND) {
        return core_result_ok();
    }
    if (result.code != CORE_OK) {
        return result;
    }
    if (chunk.size != (uint64_t)sizeof(DrawingProgramSnapshotShellV2)) {
        return (CoreResult){ CORE_ERR_FORMAT, "unexpected drawing snapshot shell payload size" };
    }
    payload = (DrawingProgramSnapshotShellV2 *)calloc(1u, sizeof(*payload));
    if (!payload) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate snapshot shell load payload" };
    }
    result = core_pack_reader_read_chunk_data(reader, &chunk, payload, (uint64_t)sizeof(*payload));
    if (result.code != CORE_OK) {
        free(payload);
        return result;
    }
    if (payload->header.version != DRAWING_PROGRAM_SNAPSHOT_SHELL_VERSION_V2) {
        free(payload);
        return (CoreResult){ CORE_ERR_FORMAT, "unsupported drawing snapshot shell version" };
    }
    if (payload->header.node_count > DRAWING_PROGRAM_PANE_NODE_CAPACITY ||
        payload->header.binding_count > DRAWING_PROGRAM_MODULE_BINDING_CAPACITY ||
        payload->header.history_count > DRAWING_PROGRAM_HISTORY_CAPACITY ||
        payload->header.history_cursor > payload->header.history_count ||
        payload->header.layer_count == 0u ||
        payload->header.layer_count > DRAWING_PROGRAM_MAX_LAYERS ||
        payload->header.logical_width == 0u ||
        payload->header.logical_height == 0u ||
        payload->header.sample_density == 0u) {
        free(payload);
        return (CoreResult){ CORE_ERR_FORMAT, "invalid drawing snapshot shell bounds" };
    }
    result = drawing_program_document_init_with_shape(&ctx->document,
                                                      payload->header.logical_width,
                                                      payload->header.logical_height,
                                                      payload->header.sample_density);
    if (result.code != CORE_OK) {
        free(payload);
        return result;
    }
    if (ctx->document.raster_width != payload->header.raster_width ||
        ctx->document.raster_height != payload->header.raster_height ||
        ctx->document.raster_sample_count != payload->header.raster_sample_count) {
        free(payload);
        return (CoreResult){ CORE_ERR_FORMAT, "drawing snapshot shell raster shape mismatch" };
    }
    ctx->document.schema_version = payload->header.schema_version;
    ctx->document.layer_count = payload->header.layer_count;
    ctx->document.next_layer_id = payload->header.next_layer_id;
    if (ctx->document.next_layer_id == 0u) {
        ctx->document.next_layer_id = 1u;
    }
    memset(ctx->document.layers, 0, sizeof(ctx->document.layers));
    memcpy(ctx->document.layers, payload->layers, sizeof(payload->layers));
    for (i = 0u; i < ctx->document.layer_count; ++i) {
        if (ctx->document.layers[i].layer_id == 0u) {
            free(payload);
            return (CoreResult){ CORE_ERR_FORMAT, "drawing snapshot shell contains invalid layer id" };
        }
        ctx->document.layers[i].name[DRAWING_PROGRAM_LAYER_NAME_CAPACITY - 1u] = '\0';
    }
    ctx->editor = payload->editor;
    ctx->pane_host.layout_state = payload->layout_state;
    memcpy(ctx->pane_host.nodes, payload->nodes, sizeof(ctx->pane_host.nodes));
    memcpy(ctx->pane_host.module_bindings, payload->bindings, sizeof(ctx->pane_host.module_bindings));
    memcpy(ctx->history.entries, payload->history_entries, sizeof(ctx->history.entries));
    ctx->history.count = payload->header.history_count;
    ctx->history.cursor = payload->header.history_cursor;
    ctx->history.raster_delta_count = 0u;
    ctx->pane_host.node_count = payload->header.node_count;
    ctx->pane_host.root_index = 0u;
    ctx->pane_host.module_binding_count = payload->header.binding_count;
    free(payload);
    if (out_found) {
        *out_found = 1u;
    }
    return core_result_ok();
}
