#include "drawing_program/drawing_program_snapshot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drawing_program_snapshot_internal.h"
#include "drawing_program/drawing_program_app_post_load.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_snapshot_shell.h"
#include "drawing_program/drawing_program_texture_project_session.h"
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

CoreResult drawing_program_snapshot_invalid(const char *message) {
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

CoreResult drawing_program_snapshot_save(struct DrawingProgramAppContext *ctx, const char *path) {
    CorePackWriter writer;
    DrawingProgramSnapshotV1 *legacy_payload = 0;
    CoreResult result;
    uint32_t accepted_root_index = 0u;
    if (!ctx || !path) {
        return drawing_program_snapshot_invalid("invalid snapshot save request");
    }
    result = drawing_program_texture_project_session_commit_active_surface(ctx);
    if (result.code != CORE_OK) {
        return result;
    }
    legacy_payload = (DrawingProgramSnapshotV1 *)calloc(1u, sizeof(*legacy_payload));
    if (!legacy_payload) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate legacy snapshot payload" };
    }
    legacy_payload->header.version = 1u;
    legacy_payload->header.history_count = ctx->history.count;
    legacy_payload->header.history_cursor = ctx->history.cursor;
    legacy_payload->document = ctx->document;
    legacy_payload->editor = ctx->editor;
    result = drawing_program_authoring_host_export_accepted_pane_state(ctx,
                                                                       &legacy_payload->layout_state,
                                                                       legacy_payload->nodes,
                                                                       &legacy_payload->header.node_count,
                                                                       &accepted_root_index,
                                                                       legacy_payload->bindings,
                                                                       &legacy_payload->header.binding_count);
    if (result.code != CORE_OK) {
        free(legacy_payload);
        return result;
    }
    (void)accepted_root_index;
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
    result = drawing_program_snapshot_write_history_raster_delta_chunk(&writer, ctx);
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
    result = drawing_program_texture_project_snapshot_write(&writer, &ctx->texture_project);
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
    CoreResult shell_result;
    CoreResult texture_project_result;
    DrawingProgramTextureProject loaded_texture_project;
    uint8_t upgraded_legacy_palette_samples = 0u;
    uint8_t loaded_current_shell = 0u;
    uint8_t current_shell_format_incompatible = 0u;
    uint8_t loaded_texture_project_found = 0u;
    uint8_t *layer_chunk_data = 0;
    uint8_t *object_chunk_data = 0;
    if (!ctx || !path) {
        return drawing_program_snapshot_invalid("invalid snapshot load request");
    }

    memset(&loaded_texture_project, 0, sizeof(loaded_texture_project));
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
    texture_project_result = drawing_program_texture_project_snapshot_load(&loaded_texture_project,
                                                                          &reader,
                                                                          &loaded_texture_project_found);
    if (texture_project_result.code != CORE_OK) {
        free(payload);
        (void)core_pack_reader_close(&reader);
        drawing_program_texture_project_dispose(&loaded_texture_project);
        return texture_project_result;
    }
    shell_result = drawing_program_snapshot_shell_load_current(ctx, &reader, &loaded_current_shell);
    if (snapshot_trace_enabled()) {
        fprintf(stderr,
                "drawing_program trace snapshot_load shell_result code=%d message=%s found=%u\n",
                (int)shell_result.code,
                shell_result.message ? shell_result.message : "(null)",
                (unsigned)loaded_current_shell);
    }
    if (shell_result.code == CORE_ERR_FORMAT) {
        current_shell_format_incompatible = 1u;
        loaded_current_shell = 0u;
    } else if (shell_result.code != CORE_OK) {
        free(payload);
        (void)core_pack_reader_close(&reader);
        drawing_program_texture_project_dispose(&loaded_texture_project);
        return shell_result;
    }
    if (!loaded_current_shell) {
        result = core_pack_reader_find_chunk(&reader, "DPS2", 0u, &chunk);
        if (result.code != CORE_OK) {
            if (current_shell_format_incompatible) {
                free(payload);
                (void)core_pack_reader_close(&reader);
                drawing_program_texture_project_dispose(&loaded_texture_project);
                if (snapshot_trace_enabled()) {
                    fprintf(stderr,
                            "drawing_program trace snapshot_load fail path=%s code=%d message=%s\n",
                            path ? path : "(null)",
                            (int)shell_result.code,
                            shell_result.message ? shell_result.message : "(null)");
                }
                return shell_result;
            }
            free(payload);
            (void)core_pack_reader_close(&reader);
            drawing_program_texture_project_dispose(&loaded_texture_project);
            if (snapshot_trace_enabled()) {
                fprintf(stderr,
                        "drawing_program trace snapshot_load fail path=%s code=%d message=%s\n",
                        path ? path : "(null)",
                        (int)result.code,
                        result.message ? result.message : "(null)");
            }
            return result;
        }
        if (chunk.size != (uint64_t)sizeof(*payload)) {
            result = drawing_program_snapshot_load_legacy_chunked_fallback(ctx, &reader);
            if (result.code != CORE_OK) {
                free(payload);
                (void)core_pack_reader_close(&reader);
                drawing_program_texture_project_dispose(&loaded_texture_project);
                return result;
            }
        } else {
            result = core_pack_reader_read_chunk_data(&reader, &chunk, payload, (uint64_t)sizeof(*payload));
            if (result.code != CORE_OK) {
                free(payload);
                (void)core_pack_reader_close(&reader);
                drawing_program_texture_project_dispose(&loaded_texture_project);
                return result;
            }
            if (payload->header.version != 1u) {
                free(payload);
                (void)core_pack_reader_close(&reader);
                drawing_program_texture_project_dispose(&loaded_texture_project);
                return (CoreResult){ CORE_ERR_FORMAT, "unsupported drawing snapshot version" };
            }
            if (payload->header.node_count > DRAWING_PROGRAM_PANE_NODE_CAPACITY ||
                payload->header.binding_count > DRAWING_PROGRAM_MODULE_BINDING_CAPACITY ||
                payload->header.history_count > DRAWING_PROGRAM_HISTORY_CAPACITY ||
                payload->header.history_cursor > payload->header.history_count) {
                free(payload);
                (void)core_pack_reader_close(&reader);
                drawing_program_texture_project_dispose(&loaded_texture_project);
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
            ctx->history.raster_delta_count = 0u;
            ctx->pane_host.node_count = payload->header.node_count;
            ctx->pane_host.root_index = 0u;
            ctx->pane_host.module_binding_count = payload->header.binding_count;
        }
    }
    result = drawing_program_snapshot_apply_history_raster_delta_chunk(ctx, &reader);
    if (result.code != CORE_OK) {
        free(payload);
        (void)core_pack_reader_close(&reader);
        drawing_program_texture_project_dispose(&loaded_texture_project);
        return result;
    }
    drawing_program_object_store_reset(&ctx->object_store);
    if (snapshot_trace_enabled()) {
        fprintf(stderr,
                "drawing_program trace snapshot_load pre_raster_init logical=%ux%u density=%u layer_count=%u raster=%ux%u samples=%u next_layer=%u schema=%u\n",
                (unsigned)ctx->document.logical_width,
                (unsigned)ctx->document.logical_height,
                (unsigned)ctx->document.sample_density,
                (unsigned)ctx->document.layer_count,
                (unsigned)ctx->document.raster_width,
                (unsigned)ctx->document.raster_height,
                (unsigned)ctx->document.raster_sample_count,
                (unsigned)ctx->document.next_layer_id,
                (unsigned)ctx->document.schema_version);
    }
    result = drawing_program_layer_raster_store_init_from_document(&ctx->layer_rasters, &ctx->document);
    if (result.code != CORE_OK) {
        free(payload);
        (void)core_pack_reader_close(&reader);
        drawing_program_texture_project_dispose(&loaded_texture_project);
        if (snapshot_trace_enabled()) {
            fprintf(stderr,
                    "drawing_program trace snapshot_load layer_raster_init_fail code=%d message=%s logical=%ux%u density=%u layer_count=%u raster=%ux%u samples=%u\n",
                    (int)result.code,
                    result.message ? result.message : "(null)",
                    (unsigned)ctx->document.logical_width,
                    (unsigned)ctx->document.logical_height,
                    (unsigned)ctx->document.sample_density,
                    (unsigned)ctx->document.layer_count,
                    (unsigned)ctx->document.raster_width,
                    (unsigned)ctx->document.raster_height,
                    (unsigned)ctx->document.raster_sample_count);
        }
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
            drawing_program_texture_project_dispose(&loaded_texture_project);
            return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate layer raster chunk buffer" };
        }
        result = core_pack_reader_read_chunk_data(&reader, &layer_chunk, layer_chunk_data, layer_chunk.size);
        if (result.code != CORE_OK) {
            free(layer_chunk_data);
            layer_chunk_data = 0;
            free(payload);
            (void)core_pack_reader_close(&reader);
            drawing_program_texture_project_dispose(&loaded_texture_project);
            return result;
        }
        result = drawing_program_snapshot_apply_layer_raster_chunk(ctx, layer_chunk_data, layer_chunk.size);
        free(layer_chunk_data);
        layer_chunk_data = 0;
        if (result.code != CORE_OK) {
            free(payload);
            (void)core_pack_reader_close(&reader);
            drawing_program_texture_project_dispose(&loaded_texture_project);
            return result;
        }
    }
    result = core_pack_reader_find_chunk(&reader, "DPOB", 0u, &object_chunk);
    if (result.code == CORE_OK) {
        object_chunk_data = (uint8_t *)malloc((size_t)object_chunk.size);
        if (!object_chunk_data) {
            free(payload);
            (void)core_pack_reader_close(&reader);
            drawing_program_texture_project_dispose(&loaded_texture_project);
            return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate object chunk buffer" };
        }
        result = core_pack_reader_read_chunk_data(&reader, &object_chunk, object_chunk_data, object_chunk.size);
        if (result.code != CORE_OK) {
            free(object_chunk_data);
            object_chunk_data = 0;
            free(payload);
            (void)core_pack_reader_close(&reader);
            drawing_program_texture_project_dispose(&loaded_texture_project);
            return result;
        }
        result = drawing_program_snapshot_apply_object_chunk(ctx, object_chunk_data, object_chunk.size);
        free(object_chunk_data);
        object_chunk_data = 0;
        if (result.code != CORE_OK) {
            free(payload);
            (void)core_pack_reader_close(&reader);
            drawing_program_texture_project_dispose(&loaded_texture_project);
            return result;
        }
    }
    result = drawing_program_snapshot_upgrade_legacy_palette_index_samples(ctx, &upgraded_legacy_palette_samples);
    if (result.code != CORE_OK) {
        free(payload);
        (void)core_pack_reader_close(&reader);
        drawing_program_texture_project_dispose(&loaded_texture_project);
        return result;
    }
    result = core_pack_reader_find_chunk(&reader, "DPUI", 0u, &ui_chunk);
    if (result.code == CORE_OK) {
        result = drawing_program_snapshot_apply_ui_settings_chunk(ctx, &reader, &ui_chunk);
    }
    (void)core_pack_reader_close(&reader);
    free(payload);
    result = drawing_program_pane_host_rebuild(ctx);
    if (result.code == CORE_OK && !drawing_program_pane_host_default_modules_ready(ctx)) {
        result = drawing_program_pane_host_rebind_default_modules(ctx);
        if (result.code == CORE_OK) {
            result = drawing_program_pane_host_rebuild(ctx);
        }
    }
    if (result.code == CORE_OK) {
        if (loaded_texture_project_found) {
            drawing_program_texture_project_dispose(&ctx->texture_project);
            ctx->texture_project = loaded_texture_project;
            result = drawing_program_texture_project_session_commit_active_surface(ctx);
        } else {
            result = drawing_program_texture_project_session_init_from_current_document(ctx);
        }
        if (result.code == CORE_OK) {
            drawing_program_texture_project_session_sync_scene_selection_from_project(ctx);
        }
    } else {
        drawing_program_texture_project_dispose(&loaded_texture_project);
    }
    if (result.code == CORE_OK) {
        drawing_program_app_rearm_after_document_swap(ctx);
    }
    if (snapshot_trace_enabled() && result.code != CORE_OK) {
        fprintf(stderr,
                "drawing_program trace snapshot_load fail path=%s code=%d message=%s\n",
                path ? path : "(null)",
                (int)result.code,
                result.message ? result.message : "(null)");
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
