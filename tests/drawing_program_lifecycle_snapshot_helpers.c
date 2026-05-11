#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_pack.h"
#include "drawing_program_lifecycle_snapshot_helpers.h"

typedef struct DrawingProgramTextureProjectChunkHeaderV5Compat {
    uint32_t version;
    uint32_t primitive_kind;
    uint32_t net_layout_kind;
    uint32_t quality_preset;
    uint32_t export_binding_kind;
    uint32_t surface_count;
    uint32_t active_surface_index;
    uint32_t next_surface_id;
    char source_scene_id[64];
    char source_object_id[64];
    char source_scene_path[512];
} DrawingProgramTextureProjectChunkHeaderV5Compat;

typedef struct DrawingProgramTextureProjectChunkHeaderV6Compat {
    uint32_t version;
    uint32_t primitive_kind;
    uint32_t net_layout_kind;
    uint32_t quality_preset;
    uint32_t export_binding_kind;
    uint32_t export_intent_kind;
    uint32_t surface_count;
    uint32_t active_surface_index;
    uint32_t next_surface_id;
    char source_scene_id[64];
    char source_object_id[64];
    char source_scene_path[512];
} DrawingProgramTextureProjectChunkHeaderV6Compat;

typedef struct DrawingProgramTextureProjectChunkHeaderV7Compat {
    uint32_t version;
    uint32_t primitive_kind;
    uint32_t net_layout_kind;
    uint32_t quality_preset;
    uint32_t export_binding_kind;
    uint32_t export_intent_kind;
    uint32_t overlay_material_intent_kind;
    uint32_t surface_count;
    uint32_t active_surface_index;
    uint32_t next_surface_id;
    char source_scene_id[64];
    char source_object_id[64];
    char source_scene_path[512];
} DrawingProgramTextureProjectChunkHeaderV7Compat;

typedef DrawingProgramTextureProjectChunkHeaderV7Compat DrawingProgramTextureProjectChunkHeaderV8Compat;
typedef DrawingProgramTextureProjectChunkHeaderV7Compat DrawingProgramTextureProjectChunkHeaderV9Compat;
typedef DrawingProgramTextureProjectChunkHeaderV7Compat DrawingProgramTextureProjectChunkHeaderV10Compat;
typedef DrawingProgramTextureProjectChunkHeaderV7Compat DrawingProgramTextureProjectChunkHeaderV11Compat;

typedef struct DrawingProgramTextureProjectSurfaceRecordV4Compat {
    uint32_t surface_id;
    uint32_t face_role;
    uint32_t quality_preset;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    float layout_offset_x;
    float layout_offset_y;
    uint8_t is_blank;
    uint8_t resize_locked;
    uint8_t user_created;
    uint8_t reserved0;
    uint32_t net_layout_kind;
    uint32_t net_slot;
    uint32_t orientation;
    uint8_t corner_ids[4];
    uint8_t edge_ids[4];
    uint8_t adjacent_face_roles[4];
    uint8_t reserved1[4];
    char name[64];
} DrawingProgramTextureProjectSurfaceRecordV4Compat;

typedef struct DrawingProgramTextureProjectSurfaceRecordV5Compat {
    uint32_t surface_id;
    uint32_t face_role;
    uint32_t quality_preset;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    float layout_offset_x;
    float layout_offset_y;
    uint32_t reflection_center_x;
    uint32_t reflection_center_y;
    uint8_t is_blank;
    uint8_t resize_locked;
    uint8_t user_created;
    uint8_t reflection_horizontal;
    uint8_t reflection_vertical;
    uint8_t reserved0;
    uint8_t reserved1;
    uint8_t reserved2;
    uint32_t net_layout_kind;
    uint32_t net_slot;
    uint32_t orientation;
    uint8_t corner_ids[4];
    uint8_t edge_ids[4];
    uint8_t adjacent_face_roles[4];
    uint8_t reserved3[4];
    char name[64];
} DrawingProgramTextureProjectSurfaceRecordV5Compat;

typedef struct DrawingProgramTextureProjectSurfaceRecordV6Compat {
    uint32_t surface_id;
    uint32_t face_role;
    uint32_t quality_preset;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    float layout_offset_x;
    float layout_offset_y;
    uint32_t reflection_center_x;
    uint32_t reflection_center_y;
    uint8_t is_blank;
    uint8_t resize_locked;
    uint8_t user_created;
    uint8_t reflection_horizontal;
    uint8_t reflection_vertical;
    uint8_t layer_opacity_entry_count;
    uint8_t reserved0;
    uint32_t net_layout_kind;
    uint32_t net_slot;
    uint32_t orientation;
    uint8_t corner_ids[4];
    uint8_t edge_ids[4];
    uint8_t adjacent_face_roles[4];
    uint8_t reserved3[4];
    char name[64];
    uint8_t layer_opacity_values[16];
    uint32_t layer_opacity_layer_ids[16];
} DrawingProgramTextureProjectSurfaceRecordV6Compat;

typedef struct DrawingProgramTextureProjectSurfaceRecordV7Compat {
    uint32_t surface_id;
    uint32_t face_role;
    uint32_t quality_preset;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    float layout_offset_x;
    float layout_offset_y;
    uint32_t reflection_center_x;
    uint32_t reflection_center_y;
    uint8_t is_blank;
    uint8_t resize_locked;
    uint8_t user_created;
    uint8_t reflection_horizontal;
    uint8_t reflection_vertical;
    uint8_t layer_opacity_entry_count;
    uint8_t layer_role_entry_count;
    uint32_t net_layout_kind;
    uint32_t net_slot;
    uint32_t orientation;
    uint8_t corner_ids[4];
    uint8_t edge_ids[4];
    uint8_t adjacent_face_roles[4];
    uint8_t reserved3[4];
    char name[64];
    uint8_t layer_opacity_values[16];
    uint32_t layer_opacity_layer_ids[16];
    uint8_t layer_role_values[16];
    uint32_t layer_role_layer_ids[16];
} DrawingProgramTextureProjectSurfaceRecordV7Compat;

typedef struct DrawingProgramTextureProjectSurfaceRecordV8Compat {
    uint32_t surface_id;
    uint32_t face_role;
    uint32_t quality_preset;
    uint32_t logical_width;
    uint32_t logical_height;
    uint32_t sample_density;
    float layout_offset_x;
    float layout_offset_y;
    uint32_t reflection_center_x;
    uint32_t reflection_center_y;
    uint8_t is_blank;
    uint8_t resize_locked;
    uint8_t user_created;
    uint8_t reflection_horizontal;
    uint8_t reflection_vertical;
    uint8_t layer_opacity_entry_count;
    uint8_t layer_role_entry_count;
    uint8_t layer_material_intent_entry_count;
    uint32_t net_layout_kind;
    uint32_t net_slot;
    uint32_t orientation;
    uint8_t corner_ids[4];
    uint8_t edge_ids[4];
    uint8_t adjacent_face_roles[4];
    uint8_t reserved3[4];
    char name[64];
    uint8_t layer_opacity_values[16];
    uint32_t layer_opacity_layer_ids[16];
    uint8_t layer_role_values[16];
    uint32_t layer_role_layer_ids[16];
    uint8_t layer_material_intent_values[16];
    uint32_t layer_material_intent_layer_ids[16];
} DrawingProgramTextureProjectSurfaceRecordV8Compat;

static int lifecycle_snapshot_find_shell_chunk(CorePackReader *reader,
                                               CorePackChunkInfo *out_chunk,
                                               const char **out_chunk_id) {
    CoreResult result;
    if (!reader || !out_chunk || !out_chunk_id) {
        return 0;
    }
    memset(out_chunk, 0, sizeof(*out_chunk));
    *out_chunk_id = "DPS2";
    result = core_pack_reader_find_chunk(reader, *out_chunk_id, 0u, out_chunk);
    if (result.code == CORE_OK) {
        return 1;
    }
    *out_chunk_id = "DPS3";
    result = core_pack_reader_find_chunk(reader, *out_chunk_id, 0u, out_chunk);
    return result.code == CORE_OK;
}

int write_legacy_snapshot_without_layer_chunk(const char *source_pack_path,
                                                     const char *output_pack_path) {
    CorePackReader reader;
    CorePackWriter writer;
    CorePackChunkInfo shell_chunk;
    CorePackChunkInfo dpui_chunk;
    CoreResult result;
    uint8_t *shell_data = 0;
    uint8_t *dpui_data = 0;
    const char *shell_chunk_id = 0;
    int has_dpui = 0;
    memset(&shell_chunk, 0, sizeof(shell_chunk));
    memset(&dpui_chunk, 0, sizeof(dpui_chunk));
    result = core_pack_reader_open(source_pack_path, &reader);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed to open source snapshot %s (%s)\n",
                source_pack_path,
                result.message ? result.message : "unknown");
        return 0;
    }
    if (!lifecycle_snapshot_find_shell_chunk(&reader, &shell_chunk, &shell_chunk_id)) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing shell chunk\n");
        return 0;
    }
    shell_data = (uint8_t *)malloc((size_t)shell_chunk.size);
    if (!shell_data) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to allocate shell copy buffer\n");
        return 0;
    }
    result = core_pack_reader_read_chunk_data(&reader, &shell_chunk, shell_data, shell_chunk.size);
    if (result.code != CORE_OK) {
        free(shell_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to read shell chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPUI", 0u, &dpui_chunk);
    if (result.code == CORE_OK) {
        dpui_data = (uint8_t *)malloc((size_t)dpui_chunk.size);
        if (!dpui_data) {
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPUI copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dpui_chunk, dpui_data, dpui_chunk.size);
        if (result.code != CORE_OK) {
            free(dpui_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPUI chunk\n");
            return 0;
        }
        has_dpui = 1;
    }
    (void)core_pack_reader_close(&reader);

    result = core_pack_writer_open(output_pack_path, &writer);
    if (result.code != CORE_OK) {
        free(dpui_data);
        free(shell_data);
        fprintf(stderr, "lifecycle_test: failed to open legacy snapshot output %s\n", output_pack_path);
        return 0;
    }
    result = core_pack_writer_add_chunk(&writer, shell_chunk_id, shell_data, shell_chunk.size);
    if (result.code == CORE_OK && has_dpui) {
        result = core_pack_writer_add_chunk(&writer, "DPUI", dpui_data, dpui_chunk.size);
    }
    if (result.code == CORE_OK) {
        result = core_pack_writer_close(&writer);
    } else {
        (void)core_pack_writer_close(&writer);
    }
    free(dpui_data);
    free(shell_data);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed to write legacy snapshot output chunk set\n");
        return 0;
    }
    return 1;
}

int write_legacy_snapshot_without_object_chunk(const char *source_pack_path,
                                                      const char *output_pack_path) {
    CorePackReader reader;
    CorePackWriter writer;
    CorePackChunkInfo shell_chunk;
    CorePackChunkInfo dplr_chunk;
    CorePackChunkInfo dpui_chunk;
    CoreResult result;
    uint8_t *shell_data = 0;
    uint8_t *dplr_data = 0;
    uint8_t *dpui_data = 0;
    const char *shell_chunk_id = 0;
    int has_dplr = 0;
    int has_dpui = 0;
    memset(&shell_chunk, 0, sizeof(shell_chunk));
    memset(&dplr_chunk, 0, sizeof(dplr_chunk));
    memset(&dpui_chunk, 0, sizeof(dpui_chunk));
    result = core_pack_reader_open(source_pack_path, &reader);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed to open source snapshot %s (%s)\n",
                source_pack_path,
                result.message ? result.message : "unknown");
        return 0;
    }
    if (!lifecycle_snapshot_find_shell_chunk(&reader, &shell_chunk, &shell_chunk_id)) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing shell chunk\n");
        return 0;
    }
    shell_data = (uint8_t *)malloc((size_t)shell_chunk.size);
    if (!shell_data) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to allocate shell copy buffer\n");
        return 0;
    }
    result = core_pack_reader_read_chunk_data(&reader, &shell_chunk, shell_data, shell_chunk.size);
    if (result.code != CORE_OK) {
        free(shell_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to read shell chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPLR", 0u, &dplr_chunk);
    if (result.code == CORE_OK) {
        dplr_data = (uint8_t *)malloc((size_t)dplr_chunk.size);
        if (!dplr_data) {
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPLR copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dplr_chunk, dplr_data, dplr_chunk.size);
        if (result.code != CORE_OK) {
            free(dplr_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPLR chunk\n");
            return 0;
        }
        has_dplr = 1;
    }
    result = core_pack_reader_find_chunk(&reader, "DPUI", 0u, &dpui_chunk);
    if (result.code == CORE_OK) {
        dpui_data = (uint8_t *)malloc((size_t)dpui_chunk.size);
        if (!dpui_data) {
            free(dplr_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPUI copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dpui_chunk, dpui_data, dpui_chunk.size);
        if (result.code != CORE_OK) {
            free(dpui_data);
            free(dplr_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPUI chunk\n");
            return 0;
        }
        has_dpui = 1;
    }
    (void)core_pack_reader_close(&reader);

    result = core_pack_writer_open(output_pack_path, &writer);
    if (result.code != CORE_OK) {
        free(dpui_data);
        free(dplr_data);
        free(shell_data);
        fprintf(stderr, "lifecycle_test: failed to open legacy snapshot output %s\n", output_pack_path);
        return 0;
    }
    result = core_pack_writer_add_chunk(&writer, shell_chunk_id, shell_data, shell_chunk.size);
    if (result.code == CORE_OK && has_dplr) {
        result = core_pack_writer_add_chunk(&writer, "DPLR", dplr_data, dplr_chunk.size);
    }
    if (result.code == CORE_OK && has_dpui) {
        result = core_pack_writer_add_chunk(&writer, "DPUI", dpui_data, dpui_chunk.size);
    }
    if (result.code == CORE_OK) {
        result = core_pack_writer_close(&writer);
    } else {
        (void)core_pack_writer_close(&writer);
    }
    free(dpui_data);
    free(dplr_data);
    free(shell_data);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed to write legacy snapshot output without object chunk\n");
        return 0;
    }
    return 1;
}

int write_legacy_snapshot_without_texture_project_chunks(const char *source_pack_path,
                                                         const char *output_pack_path) {
    CorePackReader reader;
    CorePackWriter writer;
    CorePackChunkInfo shell_chunk;
    CorePackChunkInfo dps2_chunk;
    CorePackChunkInfo dplr_chunk;
    CorePackChunkInfo dpob_chunk;
    CorePackChunkInfo dpui_chunk;
    CoreResult result;
    uint8_t *shell_data = 0;
    uint8_t *dps2_data = 0;
    uint8_t *dplr_data = 0;
    uint8_t *dpob_data = 0;
    uint8_t *dpui_data = 0;
    const char *shell_chunk_id = 0;
    int has_dps2 = 0;
    int has_dplr = 0;
    int has_dpob = 0;
    int has_dpui = 0;
    memset(&shell_chunk, 0, sizeof(shell_chunk));
    memset(&dps2_chunk, 0, sizeof(dps2_chunk));
    memset(&dplr_chunk, 0, sizeof(dplr_chunk));
    memset(&dpob_chunk, 0, sizeof(dpob_chunk));
    memset(&dpui_chunk, 0, sizeof(dpui_chunk));
    result = core_pack_reader_open(source_pack_path, &reader);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed to open source snapshot %s (%s)\n",
                source_pack_path,
                result.message ? result.message : "unknown");
        return 0;
    }
    if (!lifecycle_snapshot_find_shell_chunk(&reader, &shell_chunk, &shell_chunk_id)) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing shell chunk\n");
        return 0;
    }
    shell_data = (uint8_t *)malloc((size_t)shell_chunk.size);
    if (!shell_data) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to allocate shell copy buffer\n");
        return 0;
    }
    result = core_pack_reader_read_chunk_data(&reader, &shell_chunk, shell_data, shell_chunk.size);
    if (result.code != CORE_OK) {
        free(shell_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to read shell chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPS2", 0u, &dps2_chunk);
    if (result.code == CORE_OK) {
        dps2_data = (uint8_t *)malloc((size_t)dps2_chunk.size);
        if (!dps2_data) {
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPS2 copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dps2_chunk, dps2_data, dps2_chunk.size);
        if (result.code != CORE_OK) {
            free(dps2_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPS2 chunk\n");
            return 0;
        }
        has_dps2 = 1;
    }
    result = core_pack_reader_find_chunk(&reader, "DPLR", 0u, &dplr_chunk);
    if (result.code == CORE_OK) {
        dplr_data = (uint8_t *)malloc((size_t)dplr_chunk.size);
        if (!dplr_data) {
            free(dps2_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPLR copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dplr_chunk, dplr_data, dplr_chunk.size);
        if (result.code != CORE_OK) {
            free(dplr_data);
            free(dps2_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPLR chunk\n");
            return 0;
        }
        has_dplr = 1;
    }
    result = core_pack_reader_find_chunk(&reader, "DPOB", 0u, &dpob_chunk);
    if (result.code == CORE_OK) {
        dpob_data = (uint8_t *)malloc((size_t)dpob_chunk.size);
        if (!dpob_data) {
            free(dplr_data);
            free(dps2_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPOB copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dpob_chunk, dpob_data, dpob_chunk.size);
        if (result.code != CORE_OK) {
            free(dpob_data);
            free(dplr_data);
            free(dps2_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPOB chunk\n");
            return 0;
        }
        has_dpob = 1;
    }
    result = core_pack_reader_find_chunk(&reader, "DPUI", 0u, &dpui_chunk);
    if (result.code == CORE_OK) {
        dpui_data = (uint8_t *)malloc((size_t)dpui_chunk.size);
        if (!dpui_data) {
            free(dpob_data);
            free(dplr_data);
            free(dps2_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPUI copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dpui_chunk, dpui_data, dpui_chunk.size);
        if (result.code != CORE_OK) {
            free(dpui_data);
            free(dpob_data);
            free(dplr_data);
            free(dps2_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPUI chunk\n");
            return 0;
        }
        has_dpui = 1;
    }
    (void)core_pack_reader_close(&reader);

    result = core_pack_writer_open(output_pack_path, &writer);
    if (result.code != CORE_OK) {
        free(dpui_data);
        free(dpob_data);
        free(dplr_data);
        free(dps2_data);
        free(shell_data);
        fprintf(stderr, "lifecycle_test: failed to open legacy texture-stripped snapshot output %s\n",
                output_pack_path);
        return 0;
    }
    result = core_pack_writer_add_chunk(&writer, shell_chunk_id, shell_data, shell_chunk.size);
    if (result.code == CORE_OK && has_dps2) {
        result = core_pack_writer_add_chunk(&writer, "DPS2", dps2_data, dps2_chunk.size);
    }
    if (result.code == CORE_OK && has_dplr) {
        result = core_pack_writer_add_chunk(&writer, "DPLR", dplr_data, dplr_chunk.size);
    }
    if (result.code == CORE_OK && has_dpob) {
        result = core_pack_writer_add_chunk(&writer, "DPOB", dpob_data, dpob_chunk.size);
    }
    if (result.code == CORE_OK && has_dpui) {
        result = core_pack_writer_add_chunk(&writer, "DPUI", dpui_data, dpui_chunk.size);
    }
    if (result.code == CORE_OK) {
        result = core_pack_writer_close(&writer);
    } else {
        (void)core_pack_writer_close(&writer);
    }
    free(dpui_data);
    free(dpob_data);
    free(dplr_data);
    free(dps2_data);
    free(shell_data);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed to write legacy snapshot output without texture chunks\n");
        return 0;
    }
    return 1;
}

int write_snapshot_with_truncated_dps2_shell(const char *source_pack_path,
                                             const char *output_pack_path) {
    CorePackReader reader;
    CorePackWriter writer;
    CorePackChunkInfo dps2_chunk;
    CorePackChunkInfo dplr_chunk;
    CorePackChunkInfo dpob_chunk;
    CorePackChunkInfo dpui_chunk;
    CoreResult result;
    uint8_t *dps2_data = 0;
    uint8_t *dplr_data = 0;
    uint8_t *dpob_data = 0;
    uint8_t *dpui_data = 0;
    uint64_t truncated_size = 0u;
    int has_dpob = 0;
    int has_dpui = 0;
    memset(&dps2_chunk, 0, sizeof(dps2_chunk));
    memset(&dplr_chunk, 0, sizeof(dplr_chunk));
    memset(&dpob_chunk, 0, sizeof(dpob_chunk));
    memset(&dpui_chunk, 0, sizeof(dpui_chunk));
    result = core_pack_reader_open(source_pack_path, &reader);
    if (result.code != CORE_OK) {
        fprintf(stderr,
                "lifecycle_test: failed to open source snapshot %s (%s)\n",
                source_pack_path,
                result.message ? result.message : "unknown");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPS2", 0u, &dps2_chunk);
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing DPS2 chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPLR", 0u, &dplr_chunk);
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing DPLR chunk\n");
        return 0;
    }
    dps2_data = (uint8_t *)malloc((size_t)dps2_chunk.size);
    dplr_data = (uint8_t *)malloc((size_t)dplr_chunk.size);
    if (!dps2_data || !dplr_data) {
        free(dps2_data);
        free(dplr_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to allocate truncated DPS2 buffers\n");
        return 0;
    }
    result = core_pack_reader_read_chunk_data(&reader, &dps2_chunk, dps2_data, dps2_chunk.size);
    if (result.code == CORE_OK) {
        result = core_pack_reader_read_chunk_data(&reader, &dplr_chunk, dplr_data, dplr_chunk.size);
    }
    if (result.code != CORE_OK) {
        free(dps2_data);
        free(dplr_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to read source shell or raster chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPOB", 0u, &dpob_chunk);
    if (result.code == CORE_OK) {
        dpob_data = (uint8_t *)malloc((size_t)dpob_chunk.size);
        if (!dpob_data) {
            free(dps2_data);
            free(dplr_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPOB buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dpob_chunk, dpob_data, dpob_chunk.size);
        if (result.code != CORE_OK) {
            free(dpob_data);
            free(dps2_data);
            free(dplr_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPOB chunk\n");
            return 0;
        }
        has_dpob = 1;
    }
    result = core_pack_reader_find_chunk(&reader, "DPUI", 0u, &dpui_chunk);
    if (result.code == CORE_OK) {
        dpui_data = (uint8_t *)malloc((size_t)dpui_chunk.size);
        if (!dpui_data) {
            free(dpob_data);
            free(dps2_data);
            free(dplr_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPUI buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dpui_chunk, dpui_data, dpui_chunk.size);
        if (result.code != CORE_OK) {
            free(dpui_data);
            free(dpob_data);
            free(dps2_data);
            free(dplr_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPUI chunk\n");
            return 0;
        }
        has_dpui = 1;
    }
    (void)core_pack_reader_close(&reader);

    truncated_size = dps2_chunk.size / 2u;
    if (truncated_size < 64u) {
        truncated_size = dps2_chunk.size;
    }
    if (truncated_size >= dps2_chunk.size) {
        truncated_size = dps2_chunk.size - 1u;
    }

    result = core_pack_writer_open(output_pack_path, &writer);
    if (result.code != CORE_OK) {
        free(dpui_data);
        free(dpob_data);
        free(dps2_data);
        free(dplr_data);
        fprintf(stderr, "lifecycle_test: failed to open truncated DPS2 output %s\n", output_pack_path);
        return 0;
    }
    result = core_pack_writer_add_chunk(&writer, "DPS2", dps2_data, truncated_size);
    if (result.code == CORE_OK) {
        result = core_pack_writer_add_chunk(&writer, "DPLR", dplr_data, dplr_chunk.size);
    }
    if (result.code == CORE_OK && has_dpob) {
        result = core_pack_writer_add_chunk(&writer, "DPOB", dpob_data, dpob_chunk.size);
    }
    if (result.code == CORE_OK && has_dpui) {
        result = core_pack_writer_add_chunk(&writer, "DPUI", dpui_data, dpui_chunk.size);
    }
    if (result.code == CORE_OK) {
        result = core_pack_writer_close(&writer);
    } else {
        (void)core_pack_writer_close(&writer);
    }
    free(dpui_data);
    free(dpob_data);
    free(dps2_data);
    free(dplr_data);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed to write truncated DPS2 snapshot\n");
        return 0;
    }
    return 1;
}

int write_snapshot_with_truncated_dps3_shell(const char *source_pack_path,
                                             const char *output_pack_path) {
    CorePackReader reader;
    CorePackWriter writer;
    CorePackChunkInfo dps3_chunk;
    CorePackChunkInfo dps2_chunk;
    CorePackChunkInfo dplr_chunk;
    CorePackChunkInfo dpob_chunk;
    CorePackChunkInfo dpui_chunk;
    CoreResult result;
    uint8_t *dps3_data = 0;
    uint8_t *dps2_data = 0;
    uint8_t *dplr_data = 0;
    uint8_t *dpob_data = 0;
    uint8_t *dpui_data = 0;
    uint64_t truncated_size = 0u;
    int has_dpob = 0;
    int has_dpui = 0;
    memset(&dps3_chunk, 0, sizeof(dps3_chunk));
    memset(&dps2_chunk, 0, sizeof(dps2_chunk));
    memset(&dplr_chunk, 0, sizeof(dplr_chunk));
    memset(&dpob_chunk, 0, sizeof(dpob_chunk));
    memset(&dpui_chunk, 0, sizeof(dpui_chunk));
    result = core_pack_reader_open(source_pack_path, &reader);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed to open source snapshot %s (%s)\n",
                source_pack_path,
                result.message ? result.message : "unknown");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPS3", 0u, &dps3_chunk);
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing DPS3 chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPS2", 0u, &dps2_chunk);
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing DPS2 chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPLR", 0u, &dplr_chunk);
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing DPLR chunk\n");
        return 0;
    }
    dps3_data = (uint8_t *)malloc((size_t)dps3_chunk.size);
    dps2_data = (uint8_t *)malloc((size_t)dps2_chunk.size);
    dplr_data = (uint8_t *)malloc((size_t)dplr_chunk.size);
    if (!dps3_data || !dps2_data || !dplr_data) {
        free(dplr_data);
        free(dps2_data);
        free(dps3_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to allocate truncated DPS3 buffers\n");
        return 0;
    }
    result = core_pack_reader_read_chunk_data(&reader, &dps3_chunk, dps3_data, dps3_chunk.size);
    if (result.code == CORE_OK) {
        result = core_pack_reader_read_chunk_data(&reader, &dps2_chunk, dps2_data, dps2_chunk.size);
    }
    if (result.code == CORE_OK) {
        result = core_pack_reader_read_chunk_data(&reader, &dplr_chunk, dplr_data, dplr_chunk.size);
    }
    if (result.code != CORE_OK) {
        free(dplr_data);
        free(dps2_data);
        free(dps3_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to read truncated DPS3 source chunks\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPOB", 0u, &dpob_chunk);
    if (result.code == CORE_OK) {
        dpob_data = (uint8_t *)malloc((size_t)dpob_chunk.size);
        if (!dpob_data) {
            free(dplr_data);
            free(dps2_data);
            free(dps3_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPOB copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dpob_chunk, dpob_data, dpob_chunk.size);
        if (result.code != CORE_OK) {
            free(dpob_data);
            free(dplr_data);
            free(dps2_data);
            free(dps3_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPOB chunk\n");
            return 0;
        }
        has_dpob = 1;
    }
    result = core_pack_reader_find_chunk(&reader, "DPUI", 0u, &dpui_chunk);
    if (result.code == CORE_OK) {
        dpui_data = (uint8_t *)malloc((size_t)dpui_chunk.size);
        if (!dpui_data) {
            free(dpob_data);
            free(dplr_data);
            free(dps2_data);
            free(dps3_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPUI copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dpui_chunk, dpui_data, dpui_chunk.size);
        if (result.code != CORE_OK) {
            free(dpui_data);
            free(dpob_data);
            free(dplr_data);
            free(dps2_data);
            free(dps3_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPUI chunk\n");
            return 0;
        }
        has_dpui = 1;
    }
    (void)core_pack_reader_close(&reader);

    truncated_size = dps3_chunk.size / 2u;
    if (truncated_size == 0u) {
        truncated_size = dps3_chunk.size;
    }

    result = core_pack_writer_open(output_pack_path, &writer);
    if (result.code != CORE_OK) {
        free(dpui_data);
        free(dpob_data);
        free(dplr_data);
        free(dps2_data);
        free(dps3_data);
        fprintf(stderr, "lifecycle_test: failed to open truncated DPS3 snapshot output %s\n", output_pack_path);
        return 0;
    }
    result = core_pack_writer_add_chunk(&writer, "DPS3", dps3_data, truncated_size);
    if (result.code == CORE_OK) {
        result = core_pack_writer_add_chunk(&writer, "DPS2", dps2_data, dps2_chunk.size);
    }
    if (result.code == CORE_OK) {
        result = core_pack_writer_add_chunk(&writer, "DPLR", dplr_data, dplr_chunk.size);
    }
    if (result.code == CORE_OK && has_dpob) {
        result = core_pack_writer_add_chunk(&writer, "DPOB", dpob_data, dpob_chunk.size);
    }
    if (result.code == CORE_OK && has_dpui) {
        result = core_pack_writer_add_chunk(&writer, "DPUI", dpui_data, dpui_chunk.size);
    }
    if (result.code == CORE_OK) {
        result = core_pack_writer_close(&writer);
    } else {
        (void)core_pack_writer_close(&writer);
    }
    free(dpui_data);
    free(dpob_data);
    free(dplr_data);
    free(dps2_data);
    free(dps3_data);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed to write truncated DPS3 snapshot output\n");
        return 0;
    }
    return 1;
}

int write_texture_project_snapshot_with_v5_root(const char *source_pack_path,
                                                const char *output_pack_path) {
    CorePackReader reader;
    CorePackWriter writer;
    CorePackChunkInfo shell_chunk;
    CorePackChunkInfo dplr_chunk;
    CorePackChunkInfo dpob_chunk;
    CorePackChunkInfo dpui_chunk;
    CorePackChunkInfo texture_root_chunk;
    CorePackChunkInfo surface_chunk;
    CorePackChunkInfo layer_chunk;
    CoreResult result;
    const char *shell_chunk_id = 0;
    uint8_t *shell_data = 0;
    uint8_t *dplr_data = 0;
    uint8_t *dpob_data = 0;
    uint8_t *dpui_data = 0;
    uint8_t *texture_root_data = 0;
    uint8_t *downgraded_texture_root_data = 0;
    uint8_t *surface_data = 0;
    uint8_t *layer_data = 0;
    int has_dplr = 0;
    int has_dpob = 0;
    int has_dpui = 0;
    uint32_t i = 0u;
    DrawingProgramTextureProjectChunkHeaderV6Compat header_v6;
    DrawingProgramTextureProjectChunkHeaderV7Compat header_v7;
    DrawingProgramTextureProjectChunkHeaderV8Compat header_v8;
    DrawingProgramTextureProjectChunkHeaderV9Compat header_v9;
    DrawingProgramTextureProjectChunkHeaderV10Compat header_v10;
    DrawingProgramTextureProjectChunkHeaderV11Compat header_v11;
    DrawingProgramTextureProjectChunkHeaderV5Compat header_v5;
    uint64_t source_header_size = 0u;
    uint32_t source_root_version = 0u;
    memset(&shell_chunk, 0, sizeof(shell_chunk));
    memset(&dplr_chunk, 0, sizeof(dplr_chunk));
    memset(&dpob_chunk, 0, sizeof(dpob_chunk));
    memset(&dpui_chunk, 0, sizeof(dpui_chunk));
    memset(&texture_root_chunk, 0, sizeof(texture_root_chunk));
    memset(&surface_chunk, 0, sizeof(surface_chunk));
    memset(&layer_chunk, 0, sizeof(layer_chunk));
    memset(&header_v6, 0, sizeof(header_v6));
    memset(&header_v7, 0, sizeof(header_v7));
    memset(&header_v8, 0, sizeof(header_v8));
    memset(&header_v9, 0, sizeof(header_v9));
    memset(&header_v10, 0, sizeof(header_v10));
    memset(&header_v11, 0, sizeof(header_v11));
    memset(&header_v5, 0, sizeof(header_v5));

    result = core_pack_reader_open(source_pack_path, &reader);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed to open source snapshot %s (%s)\n",
                source_pack_path,
                result.message ? result.message : "unknown");
        return 0;
    }
    if (!lifecycle_snapshot_find_shell_chunk(&reader, &shell_chunk, &shell_chunk_id)) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing shell chunk\n");
        return 0;
    }
    shell_data = (uint8_t *)malloc((size_t)shell_chunk.size);
    if (!shell_data) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to allocate shell copy buffer\n");
        return 0;
    }
    result = core_pack_reader_read_chunk_data(&reader, &shell_chunk, shell_data, shell_chunk.size);
    if (result.code != CORE_OK) {
        free(shell_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to read shell chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPLR", 0u, &dplr_chunk);
    if (result.code == CORE_OK) {
        dplr_data = (uint8_t *)malloc((size_t)dplr_chunk.size);
        if (!dplr_data) {
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPLR copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dplr_chunk, dplr_data, dplr_chunk.size);
        if (result.code != CORE_OK) {
            free(dplr_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPLR chunk\n");
            return 0;
        }
        has_dplr = 1;
    }
    result = core_pack_reader_find_chunk(&reader, "DPOB", 0u, &dpob_chunk);
    if (result.code == CORE_OK) {
        dpob_data = (uint8_t *)malloc((size_t)dpob_chunk.size);
        if (!dpob_data) {
            free(dplr_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPOB copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dpob_chunk, dpob_data, dpob_chunk.size);
        if (result.code != CORE_OK) {
            free(dpob_data);
            free(dplr_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPOB chunk\n");
            return 0;
        }
        has_dpob = 1;
    }
    result = core_pack_reader_find_chunk(&reader, "DPUI", 0u, &dpui_chunk);
    if (result.code == CORE_OK) {
        dpui_data = (uint8_t *)malloc((size_t)dpui_chunk.size);
        if (!dpui_data) {
            free(dpob_data);
            free(dplr_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPUI copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dpui_chunk, dpui_data, dpui_chunk.size);
        if (result.code != CORE_OK) {
            free(dpui_data);
            free(dpob_data);
            free(dplr_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPUI chunk\n");
            return 0;
        }
        has_dpui = 1;
    }
    result = core_pack_reader_find_chunk(&reader, "DPTP", 0u, &texture_root_chunk);
    if (result.code != CORE_OK) {
        free(dpui_data);
        free(dpob_data);
        free(dplr_data);
        free(shell_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing DPTP chunk\n");
        return 0;
    }
    texture_root_data = (uint8_t *)malloc((size_t)texture_root_chunk.size);
    if (!texture_root_data) {
        free(dpui_data);
        free(dpob_data);
        free(dplr_data);
        free(shell_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to allocate DPTP copy buffer\n");
        return 0;
    }
    result = core_pack_reader_read_chunk_data(&reader, &texture_root_chunk, texture_root_data, texture_root_chunk.size);
    if (result.code != CORE_OK || texture_root_chunk.size < (uint64_t)sizeof(header_v6)) {
        free(texture_root_data);
        free(dpui_data);
        free(dpob_data);
        free(dplr_data);
        free(shell_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to read DPTP chunk\n");
        return 0;
    }
    memcpy(&header_v6, texture_root_data, sizeof(header_v6));
    if (texture_root_chunk.size >= (uint64_t)sizeof(header_v7)) {
        memcpy(&header_v7, texture_root_data, sizeof(header_v7));
    }
    if (texture_root_chunk.size >= (uint64_t)sizeof(header_v8)) {
        memcpy(&header_v8, texture_root_data, sizeof(header_v8));
    }
    if (texture_root_chunk.size >= (uint64_t)sizeof(header_v11)) {
        memcpy(&header_v11, texture_root_data, sizeof(header_v11));
    }
    if (texture_root_chunk.size >= (uint64_t)sizeof(header_v10)) {
        memcpy(&header_v10, texture_root_data, sizeof(header_v10));
    }
    if (texture_root_chunk.size >= (uint64_t)sizeof(header_v9)) {
        memcpy(&header_v9, texture_root_data, sizeof(header_v9));
    }
    if (header_v11.version == 11u && header_v11.surface_count > 0u) {
        source_header_size = (uint64_t)sizeof(header_v11);
        source_root_version = 11u;
    } else if (header_v10.version == 10u && header_v10.surface_count > 0u) {
        source_header_size = (uint64_t)sizeof(header_v10);
        source_root_version = 10u;
    } else if (header_v9.version == 9u && header_v9.surface_count > 0u) {
        source_header_size = (uint64_t)sizeof(header_v9);
        source_root_version = 9u;
    } else if (header_v8.version == 8u && header_v8.surface_count > 0u) {
        source_header_size = (uint64_t)sizeof(header_v8);
        source_root_version = 8u;
    } else if (header_v7.version == 7u && header_v7.surface_count > 0u) {
        source_header_size = (uint64_t)sizeof(header_v7);
        source_root_version = 7u;
    } else if (header_v6.version == 6u && header_v6.surface_count > 0u) {
        source_header_size = (uint64_t)sizeof(header_v6);
        source_root_version = 6u;
    } else {
        free(texture_root_data);
        free(dpui_data);
        free(dpob_data);
        free(dplr_data);
        free(shell_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: expected V6-V11 DPTP root before downgrade\n");
        return 0;
    }
    header_v5.version = 5u;
    if (source_root_version == 11u) {
        header_v5.primitive_kind = header_v11.primitive_kind;
        header_v5.net_layout_kind = header_v11.net_layout_kind;
        header_v5.quality_preset = header_v11.quality_preset;
        header_v5.export_binding_kind = header_v11.export_binding_kind;
        header_v5.surface_count = header_v11.surface_count;
        header_v5.active_surface_index = header_v11.active_surface_index;
        header_v5.next_surface_id = header_v11.next_surface_id;
        memcpy(header_v5.source_scene_id, header_v11.source_scene_id, sizeof(header_v5.source_scene_id));
        memcpy(header_v5.source_object_id, header_v11.source_object_id, sizeof(header_v5.source_object_id));
        memcpy(header_v5.source_scene_path, header_v11.source_scene_path, sizeof(header_v5.source_scene_path));
    } else if (source_root_version == 10u) {
        header_v5.primitive_kind = header_v10.primitive_kind;
        header_v5.net_layout_kind = header_v10.net_layout_kind;
        header_v5.quality_preset = header_v10.quality_preset;
        header_v5.export_binding_kind = header_v10.export_binding_kind;
        header_v5.surface_count = header_v10.surface_count;
        header_v5.active_surface_index = header_v10.active_surface_index;
        header_v5.next_surface_id = header_v10.next_surface_id;
        memcpy(header_v5.source_scene_id, header_v10.source_scene_id, sizeof(header_v5.source_scene_id));
        memcpy(header_v5.source_object_id, header_v10.source_object_id, sizeof(header_v5.source_object_id));
        memcpy(header_v5.source_scene_path, header_v10.source_scene_path, sizeof(header_v5.source_scene_path));
    } else if (source_root_version == 9u) {
        header_v5.primitive_kind = header_v9.primitive_kind;
        header_v5.net_layout_kind = header_v9.net_layout_kind;
        header_v5.quality_preset = header_v9.quality_preset;
        header_v5.export_binding_kind = header_v9.export_binding_kind;
        header_v5.surface_count = header_v9.surface_count;
        header_v5.active_surface_index = header_v9.active_surface_index;
        header_v5.next_surface_id = header_v9.next_surface_id;
        memcpy(header_v5.source_scene_id, header_v9.source_scene_id, sizeof(header_v5.source_scene_id));
        memcpy(header_v5.source_object_id, header_v9.source_object_id, sizeof(header_v5.source_object_id));
        memcpy(header_v5.source_scene_path, header_v9.source_scene_path, sizeof(header_v5.source_scene_path));
    } else if (source_root_version == 8u) {
        header_v5.primitive_kind = header_v8.primitive_kind;
        header_v5.net_layout_kind = header_v8.net_layout_kind;
        header_v5.quality_preset = header_v8.quality_preset;
        header_v5.export_binding_kind = header_v8.export_binding_kind;
        header_v5.surface_count = header_v8.surface_count;
        header_v5.active_surface_index = header_v8.active_surface_index;
        header_v5.next_surface_id = header_v8.next_surface_id;
        memcpy(header_v5.source_scene_id, header_v8.source_scene_id, sizeof(header_v5.source_scene_id));
        memcpy(header_v5.source_object_id, header_v8.source_object_id, sizeof(header_v5.source_object_id));
        memcpy(header_v5.source_scene_path, header_v8.source_scene_path, sizeof(header_v5.source_scene_path));
    } else if (source_root_version == 7u) {
        header_v5.primitive_kind = header_v7.primitive_kind;
        header_v5.net_layout_kind = header_v7.net_layout_kind;
        header_v5.quality_preset = header_v7.quality_preset;
        header_v5.export_binding_kind = header_v7.export_binding_kind;
        header_v5.surface_count = header_v7.surface_count;
        header_v5.active_surface_index = header_v7.active_surface_index;
        header_v5.next_surface_id = header_v7.next_surface_id;
        memcpy(header_v5.source_scene_id, header_v7.source_scene_id, sizeof(header_v5.source_scene_id));
        memcpy(header_v5.source_object_id, header_v7.source_object_id, sizeof(header_v5.source_object_id));
        memcpy(header_v5.source_scene_path, header_v7.source_scene_path, sizeof(header_v5.source_scene_path));
    } else {
        header_v5.primitive_kind = header_v6.primitive_kind;
        header_v5.net_layout_kind = header_v6.net_layout_kind;
        header_v5.quality_preset = header_v6.quality_preset;
        header_v5.export_binding_kind = header_v6.export_binding_kind;
        header_v5.surface_count = header_v6.surface_count;
        header_v5.active_surface_index = header_v6.active_surface_index;
        header_v5.next_surface_id = header_v6.next_surface_id;
        memcpy(header_v5.source_scene_id, header_v6.source_scene_id, sizeof(header_v5.source_scene_id));
        memcpy(header_v5.source_object_id, header_v6.source_object_id, sizeof(header_v5.source_object_id));
        memcpy(header_v5.source_scene_path, header_v6.source_scene_path, sizeof(header_v5.source_scene_path));
    }
    {
        const uint64_t old_records_size = texture_root_chunk.size - source_header_size;
        uint64_t new_records_size = old_records_size;
        uint64_t new_root_size = 0u;
        if (source_root_version == 8u) {
            new_records_size =
                (uint64_t)header_v8.surface_count * (uint64_t)sizeof(DrawingProgramTextureProjectSurfaceRecordV4Compat);
        } else if (source_root_version == 9u || source_root_version == 10u || source_root_version == 11u) {
            new_records_size =
                (uint64_t)header_v5.surface_count * (uint64_t)sizeof(DrawingProgramTextureProjectSurfaceRecordV4Compat);
        }
        new_root_size = (uint64_t)sizeof(header_v5) + new_records_size;
        downgraded_texture_root_data = (uint8_t *)malloc((size_t)new_root_size);
        if (!downgraded_texture_root_data) {
            free(texture_root_data);
            free(dpui_data);
            free(dpob_data);
            free(dplr_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate downgraded DPTP payload\n");
            return 0;
        }
        memcpy(downgraded_texture_root_data, &header_v5, sizeof(header_v5));
        if (source_root_version == 8u) {
            const DrawingProgramTextureProjectSurfaceRecordV5Compat *src_records =
                (const DrawingProgramTextureProjectSurfaceRecordV5Compat *)(texture_root_data + source_header_size);
            DrawingProgramTextureProjectSurfaceRecordV4Compat *dst_records =
                (DrawingProgramTextureProjectSurfaceRecordV4Compat *)(downgraded_texture_root_data + sizeof(header_v5));
            for (i = 0u; i < header_v8.surface_count; ++i) {
                memset(&dst_records[i], 0, sizeof(dst_records[i]));
                dst_records[i].surface_id = src_records[i].surface_id;
                dst_records[i].face_role = src_records[i].face_role;
                dst_records[i].quality_preset = src_records[i].quality_preset;
                dst_records[i].logical_width = src_records[i].logical_width;
                dst_records[i].logical_height = src_records[i].logical_height;
                dst_records[i].sample_density = src_records[i].sample_density;
                dst_records[i].layout_offset_x = src_records[i].layout_offset_x;
                dst_records[i].layout_offset_y = src_records[i].layout_offset_y;
                dst_records[i].is_blank = src_records[i].is_blank;
                dst_records[i].resize_locked = src_records[i].resize_locked;
                dst_records[i].user_created = src_records[i].user_created;
                dst_records[i].net_layout_kind = src_records[i].net_layout_kind;
                dst_records[i].net_slot = src_records[i].net_slot;
                dst_records[i].orientation = src_records[i].orientation;
                memcpy(dst_records[i].corner_ids, src_records[i].corner_ids, sizeof(dst_records[i].corner_ids));
                memcpy(dst_records[i].edge_ids, src_records[i].edge_ids, sizeof(dst_records[i].edge_ids));
                memcpy(dst_records[i].adjacent_face_roles,
                       src_records[i].adjacent_face_roles,
                       sizeof(dst_records[i].adjacent_face_roles));
                memcpy(dst_records[i].name, src_records[i].name, sizeof(dst_records[i].name));
            }
        } else if (source_root_version == 11u) {
            const DrawingProgramTextureProjectSurfaceRecordV8Compat *src_records =
                (const DrawingProgramTextureProjectSurfaceRecordV8Compat *)(texture_root_data + source_header_size);
            DrawingProgramTextureProjectSurfaceRecordV4Compat *dst_records =
                (DrawingProgramTextureProjectSurfaceRecordV4Compat *)(downgraded_texture_root_data + sizeof(header_v5));
            for (i = 0u; i < header_v11.surface_count; ++i) {
                memset(&dst_records[i], 0, sizeof(dst_records[i]));
                dst_records[i].surface_id = src_records[i].surface_id;
                dst_records[i].face_role = src_records[i].face_role;
                dst_records[i].quality_preset = src_records[i].quality_preset;
                dst_records[i].logical_width = src_records[i].logical_width;
                dst_records[i].logical_height = src_records[i].logical_height;
                dst_records[i].sample_density = src_records[i].sample_density;
                dst_records[i].layout_offset_x = src_records[i].layout_offset_x;
                dst_records[i].layout_offset_y = src_records[i].layout_offset_y;
                dst_records[i].is_blank = src_records[i].is_blank;
                dst_records[i].resize_locked = src_records[i].resize_locked;
                dst_records[i].user_created = src_records[i].user_created;
                dst_records[i].net_layout_kind = src_records[i].net_layout_kind;
                dst_records[i].net_slot = src_records[i].net_slot;
                dst_records[i].orientation = src_records[i].orientation;
                memcpy(dst_records[i].corner_ids, src_records[i].corner_ids, sizeof(dst_records[i].corner_ids));
                memcpy(dst_records[i].edge_ids, src_records[i].edge_ids, sizeof(dst_records[i].edge_ids));
                memcpy(dst_records[i].adjacent_face_roles,
                       src_records[i].adjacent_face_roles,
                       sizeof(dst_records[i].adjacent_face_roles));
                memcpy(dst_records[i].name, src_records[i].name, sizeof(dst_records[i].name));
            }
        } else if (source_root_version == 10u) {
            const DrawingProgramTextureProjectSurfaceRecordV7Compat *src_records =
                (const DrawingProgramTextureProjectSurfaceRecordV7Compat *)(texture_root_data + source_header_size);
            DrawingProgramTextureProjectSurfaceRecordV4Compat *dst_records =
                (DrawingProgramTextureProjectSurfaceRecordV4Compat *)(downgraded_texture_root_data + sizeof(header_v5));
            for (i = 0u; i < header_v10.surface_count; ++i) {
                memset(&dst_records[i], 0, sizeof(dst_records[i]));
                dst_records[i].surface_id = src_records[i].surface_id;
                dst_records[i].face_role = src_records[i].face_role;
                dst_records[i].quality_preset = src_records[i].quality_preset;
                dst_records[i].logical_width = src_records[i].logical_width;
                dst_records[i].logical_height = src_records[i].logical_height;
                dst_records[i].sample_density = src_records[i].sample_density;
                dst_records[i].layout_offset_x = src_records[i].layout_offset_x;
                dst_records[i].layout_offset_y = src_records[i].layout_offset_y;
                dst_records[i].is_blank = src_records[i].is_blank;
                dst_records[i].resize_locked = src_records[i].resize_locked;
                dst_records[i].user_created = src_records[i].user_created;
                dst_records[i].net_layout_kind = src_records[i].net_layout_kind;
                dst_records[i].net_slot = src_records[i].net_slot;
                dst_records[i].orientation = src_records[i].orientation;
                memcpy(dst_records[i].corner_ids, src_records[i].corner_ids, sizeof(dst_records[i].corner_ids));
                memcpy(dst_records[i].edge_ids, src_records[i].edge_ids, sizeof(dst_records[i].edge_ids));
                memcpy(dst_records[i].adjacent_face_roles,
                       src_records[i].adjacent_face_roles,
                       sizeof(dst_records[i].adjacent_face_roles));
                memcpy(dst_records[i].name, src_records[i].name, sizeof(dst_records[i].name));
            }
        } else if (source_root_version == 9u) {
            const DrawingProgramTextureProjectSurfaceRecordV6Compat *src_records =
                (const DrawingProgramTextureProjectSurfaceRecordV6Compat *)(texture_root_data + source_header_size);
            DrawingProgramTextureProjectSurfaceRecordV4Compat *dst_records =
                (DrawingProgramTextureProjectSurfaceRecordV4Compat *)(downgraded_texture_root_data + sizeof(header_v5));
            for (i = 0u; i < header_v9.surface_count; ++i) {
                memset(&dst_records[i], 0, sizeof(dst_records[i]));
                dst_records[i].surface_id = src_records[i].surface_id;
                dst_records[i].face_role = src_records[i].face_role;
                dst_records[i].quality_preset = src_records[i].quality_preset;
                dst_records[i].logical_width = src_records[i].logical_width;
                dst_records[i].logical_height = src_records[i].logical_height;
                dst_records[i].sample_density = src_records[i].sample_density;
                dst_records[i].layout_offset_x = src_records[i].layout_offset_x;
                dst_records[i].layout_offset_y = src_records[i].layout_offset_y;
                dst_records[i].is_blank = src_records[i].is_blank;
                dst_records[i].resize_locked = src_records[i].resize_locked;
                dst_records[i].user_created = src_records[i].user_created;
                dst_records[i].net_layout_kind = src_records[i].net_layout_kind;
                dst_records[i].net_slot = src_records[i].net_slot;
                dst_records[i].orientation = src_records[i].orientation;
                memcpy(dst_records[i].corner_ids, src_records[i].corner_ids, sizeof(dst_records[i].corner_ids));
                memcpy(dst_records[i].edge_ids, src_records[i].edge_ids, sizeof(dst_records[i].edge_ids));
                memcpy(dst_records[i].adjacent_face_roles,
                       src_records[i].adjacent_face_roles,
                       sizeof(dst_records[i].adjacent_face_roles));
                memcpy(dst_records[i].name, src_records[i].name, sizeof(dst_records[i].name));
            }
        } else {
            memcpy(downgraded_texture_root_data + sizeof(header_v5),
                   texture_root_data + source_header_size,
                   (size_t)old_records_size);
        }
        free(texture_root_data);
        texture_root_data = 0;

        result = core_pack_writer_open(output_pack_path, &writer);
        if (result.code != CORE_OK) {
            free(downgraded_texture_root_data);
            free(dpui_data);
            free(dpob_data);
            free(dplr_data);
            free(shell_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to open downgraded snapshot output %s\n", output_pack_path);
            return 0;
        }
        result = core_pack_writer_add_chunk(&writer, shell_chunk_id, shell_data, shell_chunk.size);
        if (result.code == CORE_OK && has_dplr) {
            result = core_pack_writer_add_chunk(&writer, "DPLR", dplr_data, dplr_chunk.size);
        }
        if (result.code == CORE_OK && has_dpob) {
            result = core_pack_writer_add_chunk(&writer, "DPOB", dpob_data, dpob_chunk.size);
        }
        if (result.code == CORE_OK && has_dpui) {
            result = core_pack_writer_add_chunk(&writer, "DPUI", dpui_data, dpui_chunk.size);
        }
        if (result.code == CORE_OK) {
            result = core_pack_writer_add_chunk(&writer, "DPTP", downgraded_texture_root_data, new_root_size);
        }
        for (i = 0u; result.code == CORE_OK; ++i) {
            result = core_pack_reader_find_chunk(&reader, "DTSD", i, &surface_chunk);
            if (result.code == CORE_ERR_NOT_FOUND) {
                result = core_result_ok();
                break;
            }
            if (result.code != CORE_OK) {
                break;
            }
            surface_data = (uint8_t *)malloc((size_t)surface_chunk.size);
            if (!surface_data) {
                result = (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate DTSD copy buffer" };
                break;
            }
            result = core_pack_reader_read_chunk_data(&reader, &surface_chunk, surface_data, surface_chunk.size);
            if (result.code == CORE_OK) {
                result = core_pack_writer_add_chunk(&writer, "DTSD", surface_data, surface_chunk.size);
            }
            free(surface_data);
            surface_data = 0;
        }
        for (i = 0u; result.code == CORE_OK; ++i) {
            result = core_pack_reader_find_chunk(&reader, "DTSR", i, &layer_chunk);
            if (result.code == CORE_ERR_NOT_FOUND) {
                result = core_result_ok();
                break;
            }
            if (result.code != CORE_OK) {
                break;
            }
            layer_data = (uint8_t *)malloc((size_t)layer_chunk.size);
            if (!layer_data) {
                result = (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate DTSR copy buffer" };
                break;
            }
            result = core_pack_reader_read_chunk_data(&reader, &layer_chunk, layer_data, layer_chunk.size);
            if (result.code == CORE_OK) {
                result = core_pack_writer_add_chunk(&writer, "DTSR", layer_data, layer_chunk.size);
            }
            free(layer_data);
            layer_data = 0;
        }
        if (result.code == CORE_OK) {
            result = core_pack_writer_close(&writer);
        } else {
            (void)core_pack_writer_close(&writer);
        }
    }
    (void)core_pack_reader_close(&reader);
    free(layer_data);
    free(surface_data);
    free(downgraded_texture_root_data);
    free(texture_root_data);
    free(dpui_data);
    free(dpob_data);
    free(dplr_data);
    free(shell_data);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed to write downgraded texture-project snapshot\n");
        return 0;
    }
    return 1;
}
