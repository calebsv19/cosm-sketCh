#include "drawing_program/drawing_program_snapshot.h"

#include <stdio.h>
#include <string.h>

#include "core_pack.h"
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

static CoreResult snapshot_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

CoreResult drawing_program_snapshot_save(const struct DrawingProgramAppContext *ctx, const char *path) {
    CorePackWriter writer;
    DrawingProgramSnapshotV1 payload;
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

    result = core_pack_writer_open(path, &writer);
    if (result.code != CORE_OK) {
        return result;
    }
    result = core_pack_writer_add_chunk(&writer, "DPS2", &payload, (uint64_t)sizeof(payload));
    if (result.code != CORE_OK) {
        (void)core_pack_writer_close(&writer);
        return result;
    }
    return core_pack_writer_close(&writer);
}

CoreResult drawing_program_snapshot_load(struct DrawingProgramAppContext *ctx, const char *path) {
    CorePackReader reader;
    CorePackChunkInfo chunk;
    DrawingProgramSnapshotV1 payload;
    CoreResult result;
    if (!ctx || !path) {
        return snapshot_invalid("invalid snapshot load request");
    }

    memset(&payload, 0, sizeof(payload));
    result = core_pack_reader_open(path, &reader);
    if (result.code != CORE_OK) {
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
    (void)core_pack_reader_close(&reader);
    if (result.code != CORE_OK) {
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
    return drawing_program_pane_host_rebuild(ctx);
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
