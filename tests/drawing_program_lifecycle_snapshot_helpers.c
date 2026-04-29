#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_pack.h"
#include "drawing_program_lifecycle_snapshot_helpers.h"

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
