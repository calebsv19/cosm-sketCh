#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_pack.h"
#include "drawing_program_lifecycle_snapshot_helpers.h"

int write_legacy_snapshot_without_layer_chunk(const char *source_pack_path,
                                                     const char *output_pack_path) {
    CorePackReader reader;
    CorePackWriter writer;
    CorePackChunkInfo dps2_chunk;
    CorePackChunkInfo dpui_chunk;
    CoreResult result;
    uint8_t *dps2_data = 0;
    uint8_t *dpui_data = 0;
    int has_dpui = 0;
    memset(&dps2_chunk, 0, sizeof(dps2_chunk));
    memset(&dpui_chunk, 0, sizeof(dpui_chunk));
    result = core_pack_reader_open(source_pack_path, &reader);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed to open source snapshot %s (%s)\n",
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
    dps2_data = (uint8_t *)malloc((size_t)dps2_chunk.size);
    if (!dps2_data) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to allocate DPS2 copy buffer\n");
        return 0;
    }
    result = core_pack_reader_read_chunk_data(&reader, &dps2_chunk, dps2_data, dps2_chunk.size);
    if (result.code != CORE_OK) {
        free(dps2_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to read DPS2 chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPUI", 0u, &dpui_chunk);
    if (result.code == CORE_OK) {
        dpui_data = (uint8_t *)malloc((size_t)dpui_chunk.size);
        if (!dpui_data) {
            free(dps2_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPUI copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dpui_chunk, dpui_data, dpui_chunk.size);
        if (result.code != CORE_OK) {
            free(dpui_data);
            free(dps2_data);
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
        free(dps2_data);
        fprintf(stderr, "lifecycle_test: failed to open legacy snapshot output %s\n", output_pack_path);
        return 0;
    }
    result = core_pack_writer_add_chunk(&writer, "DPS2", dps2_data, dps2_chunk.size);
    if (result.code == CORE_OK && has_dpui) {
        result = core_pack_writer_add_chunk(&writer, "DPUI", dpui_data, dpui_chunk.size);
    }
    if (result.code == CORE_OK) {
        result = core_pack_writer_close(&writer);
    } else {
        (void)core_pack_writer_close(&writer);
    }
    free(dpui_data);
    free(dps2_data);
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
    CorePackChunkInfo dps2_chunk;
    CorePackChunkInfo dplr_chunk;
    CorePackChunkInfo dpui_chunk;
    CoreResult result;
    uint8_t *dps2_data = 0;
    uint8_t *dplr_data = 0;
    uint8_t *dpui_data = 0;
    int has_dplr = 0;
    int has_dpui = 0;
    memset(&dps2_chunk, 0, sizeof(dps2_chunk));
    memset(&dplr_chunk, 0, sizeof(dplr_chunk));
    memset(&dpui_chunk, 0, sizeof(dpui_chunk));
    result = core_pack_reader_open(source_pack_path, &reader);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed to open source snapshot %s (%s)\n",
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
    dps2_data = (uint8_t *)malloc((size_t)dps2_chunk.size);
    if (!dps2_data) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to allocate DPS2 copy buffer\n");
        return 0;
    }
    result = core_pack_reader_read_chunk_data(&reader, &dps2_chunk, dps2_data, dps2_chunk.size);
    if (result.code != CORE_OK) {
        free(dps2_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to read DPS2 chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPLR", 0u, &dplr_chunk);
    if (result.code == CORE_OK) {
        dplr_data = (uint8_t *)malloc((size_t)dplr_chunk.size);
        if (!dplr_data) {
            free(dps2_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPLR copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dplr_chunk, dplr_data, dplr_chunk.size);
        if (result.code != CORE_OK) {
            free(dplr_data);
            free(dps2_data);
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
            free(dps2_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPUI copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dpui_chunk, dpui_data, dpui_chunk.size);
        if (result.code != CORE_OK) {
            free(dpui_data);
            free(dplr_data);
            free(dps2_data);
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
        free(dps2_data);
        fprintf(stderr, "lifecycle_test: failed to open legacy snapshot output %s\n", output_pack_path);
        return 0;
    }
    result = core_pack_writer_add_chunk(&writer, "DPS2", dps2_data, dps2_chunk.size);
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
    free(dps2_data);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed to write legacy snapshot output without object chunk\n");
        return 0;
    }
    return 1;
}
