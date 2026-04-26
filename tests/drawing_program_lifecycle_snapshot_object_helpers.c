#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_pack.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_color_model.h"
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

typedef struct LifecycleTestObjectChunkHeaderV1 {
    uint32_t version;
    uint32_t object_count;
    uint32_t next_object_id;
    uint32_t reserved0;
} LifecycleTestObjectChunkHeaderV1;

typedef struct LifecycleTestObjectChunkEntryV1 {
    uint32_t object_id;
    uint32_t layer_id;
    uint8_t type;
    uint8_t visible;
    uint8_t locked;
    uint8_t stroke_color_index;
    uint8_t fill_color_index;
    uint8_t stroke_width;
    uint8_t style_mode;
    uint8_t reserved0;
    uint8_t reserved1;
    int32_t origin_x;
    int32_t origin_y;
    uint32_t width;
    uint32_t height;
    char name[DRAWING_PROGRAM_OBJECT_NAME_CAPACITY];
} LifecycleTestObjectChunkEntryV1;

typedef struct LifecycleTestObjectChunkHeaderV2 {
    uint32_t version;
    uint32_t object_count;
    uint32_t next_object_id;
    uint32_t reserved0;
} LifecycleTestObjectChunkHeaderV2;

typedef struct LifecycleTestObjectChunkEntryV2 {
    uint32_t object_id;
    uint32_t layer_id;
    uint8_t type;
    uint8_t visible;
    uint8_t locked;
    uint8_t stroke_color_index;
    uint8_t fill_color_index;
    uint8_t stroke_width;
    uint8_t style_mode;
    uint8_t path_closed;
    uint16_t path_point_count;
    int32_t origin_x;
    int32_t origin_y;
    uint32_t width;
    uint32_t height;
    char name[DRAWING_PROGRAM_OBJECT_NAME_CAPACITY];
    struct {
        int32_t x;
        int32_t y;
    } path_points[DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS];
} LifecycleTestObjectChunkEntryV2;

typedef struct LifecycleTestObjectChunkHeaderV3 {
    uint32_t version;
    uint32_t object_count;
    uint32_t next_object_id;
    uint32_t reserved0;
} LifecycleTestObjectChunkHeaderV3;

typedef struct LifecycleTestObjectChunkEntryV3 {
    uint32_t object_id;
    uint32_t layer_id;
    uint8_t type;
    uint8_t visible;
    uint8_t locked;
    uint8_t stroke_color_index;
    uint8_t fill_color_index;
    uint8_t stroke_width;
    uint8_t style_mode;
    uint8_t path_closed;
    uint16_t path_point_count;
    int32_t origin_x;
    int32_t origin_y;
    uint32_t width;
    uint32_t height;
    char name[DRAWING_PROGRAM_OBJECT_NAME_CAPACITY];
    DrawingProgramPathPoint path_points[DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS];
} LifecycleTestObjectChunkEntryV3;

typedef struct LifecycleTestObjectChunkHeaderV4 {
    uint32_t version;
    uint32_t object_count;
    uint32_t next_object_id;
    uint32_t reserved0;
} LifecycleTestObjectChunkHeaderV4;

typedef struct LifecycleTestObjectChunkEntryV4 {
    uint32_t object_id;
    uint32_t layer_id;
    uint8_t type;
    uint8_t visible;
    uint8_t locked;
    uint8_t stroke_width;
    uint8_t style_mode;
    uint8_t path_closed;
    uint16_t path_point_count;
    uint16_t reserved0;
    DrawingProgramRasterSample stroke_color_value;
    DrawingProgramRasterSample fill_color_value;
    int32_t origin_x;
    int32_t origin_y;
    uint32_t width;
    uint32_t height;
    char name[DRAWING_PROGRAM_OBJECT_NAME_CAPACITY];
    DrawingProgramPathPoint path_points[DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS];
} LifecycleTestObjectChunkEntryV4;

int write_legacy_snapshot_with_v1_object_chunk(const char *source_pack_path,
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
    uint8_t *dpob_v1_data = 0;
    uint64_t dpob_v1_size = 0u;
    const char *shell_chunk_id = 0;
    int has_dplr = 0;
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
    if (!lifecycle_snapshot_find_shell_chunk(&reader, &dps2_chunk, &shell_chunk_id)) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing shell chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPOB", 0u, &dpob_chunk);
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing DPOB chunk\n");
        return 0;
    }
    dps2_data = (uint8_t *)malloc((size_t)dps2_chunk.size);
    dpob_data = (uint8_t *)malloc((size_t)dpob_chunk.size);
    if (!dps2_data || !dpob_data) {
        free(dpob_data);
        free(dps2_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to allocate required snapshot buffers\n");
        return 0;
    }
    result = core_pack_reader_read_chunk_data(&reader, &dps2_chunk, dps2_data, dps2_chunk.size);
    if (result.code != CORE_OK) {
        free(dpob_data);
        free(dps2_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to read shell chunk\n");
        return 0;
    }
    result = core_pack_reader_read_chunk_data(&reader, &dpob_chunk, dpob_data, dpob_chunk.size);
    if (result.code != CORE_OK) {
        free(dpob_data);
        free(dps2_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to read DPOB chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPLR", 0u, &dplr_chunk);
    if (result.code == CORE_OK) {
        dplr_data = (uint8_t *)malloc((size_t)dplr_chunk.size);
        if (!dplr_data) {
            free(dpob_data);
            free(dps2_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPLR copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dplr_chunk, dplr_data, dplr_chunk.size);
        if (result.code != CORE_OK) {
            free(dplr_data);
            free(dpob_data);
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
            free(dpob_data);
            free(dps2_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPUI copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dpui_chunk, dpui_data, dpui_chunk.size);
        if (result.code != CORE_OK) {
            free(dpui_data);
            free(dplr_data);
            free(dpob_data);
            free(dps2_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPUI chunk\n");
            return 0;
        }
        has_dpui = 1;
    }
    (void)core_pack_reader_close(&reader);

    if (dpob_chunk.size < sizeof(uint32_t)) {
        free(dpui_data);
        free(dplr_data);
        free(dpob_data);
        free(dps2_data);
        fprintf(stderr, "lifecycle_test: DPOB chunk too small for version field\n");
        return 0;
    }
    {
        uint32_t version = 0u;
        memcpy(&version, dpob_data, sizeof(uint32_t));
        if (version == 1u) {
            dpob_v1_size = dpob_chunk.size;
            dpob_v1_data = (uint8_t *)malloc((size_t)dpob_v1_size);
            if (!dpob_v1_data) {
                free(dpui_data);
                free(dplr_data);
                free(dpob_data);
                free(dps2_data);
                fprintf(stderr, "lifecycle_test: failed to allocate DPOB v1 copy buffer\n");
                return 0;
            }
            memcpy(dpob_v1_data, dpob_data, (size_t)dpob_v1_size);
        } else if (version == 2u || version == 3u || version == 4u) {
            LifecycleTestObjectChunkHeaderV2 header_v2;
            LifecycleTestObjectChunkHeaderV1 header_v1;
            const uint8_t *cursor = dpob_data;
            uint8_t *out_cursor = 0;
            uint32_t i;
            uint64_t expected_size = 0u;

            memset(&header_v2, 0, sizeof(header_v2));
            memcpy(&header_v2, cursor, sizeof(header_v2));
            expected_size = (uint64_t)sizeof(header_v2) +
                            ((uint64_t)header_v2.object_count *
                             (uint64_t)(version == 2u ? sizeof(LifecycleTestObjectChunkEntryV2)
                                                      : (version == 3u ? sizeof(LifecycleTestObjectChunkEntryV3)
                                                                       : sizeof(LifecycleTestObjectChunkEntryV4))));
            if (expected_size != dpob_chunk.size) {
                free(dpui_data);
                free(dplr_data);
                free(dpob_data);
                free(dps2_data);
                fprintf(stderr, "lifecycle_test: invalid DPOB v2 chunk size\n");
                return 0;
            }
            memset(&header_v1, 0, sizeof(header_v1));
            header_v1.version = 1u;
            header_v1.object_count = header_v2.object_count;
            header_v1.next_object_id = header_v2.next_object_id;
            dpob_v1_size = (uint64_t)sizeof(header_v1) +
                           ((uint64_t)header_v1.object_count *
                            (uint64_t)sizeof(LifecycleTestObjectChunkEntryV1));
            dpob_v1_data = (uint8_t *)malloc((size_t)dpob_v1_size);
            if (!dpob_v1_data) {
                free(dpui_data);
                free(dplr_data);
                free(dpob_data);
                free(dps2_data);
                fprintf(stderr, "lifecycle_test: failed to allocate DPOB v1 output buffer\n");
                return 0;
            }
            out_cursor = dpob_v1_data;
            memcpy(out_cursor, &header_v1, sizeof(header_v1));
            out_cursor += sizeof(header_v1);
            cursor += sizeof(header_v2);
            for (i = 0u; i < header_v2.object_count; ++i) {
                LifecycleTestObjectChunkEntryV1 entry_v1;

                memset(&entry_v1, 0, sizeof(entry_v1));
                if (version == 2u) {
                    LifecycleTestObjectChunkEntryV2 entry_v2;

                    memcpy(&entry_v2, cursor, sizeof(entry_v2));
                    cursor += sizeof(entry_v2);
                    entry_v1.object_id = entry_v2.object_id;
                    entry_v1.layer_id = entry_v2.layer_id;
                    entry_v1.type = entry_v2.type;
                    entry_v1.visible = entry_v2.visible;
                    entry_v1.locked = entry_v2.locked;
                    entry_v1.stroke_color_index = entry_v2.stroke_color_index;
                    entry_v1.fill_color_index = entry_v2.fill_color_index;
                    entry_v1.stroke_width = entry_v2.stroke_width;
                    entry_v1.style_mode = entry_v2.style_mode;
                    entry_v1.origin_x = entry_v2.origin_x;
                    entry_v1.origin_y = entry_v2.origin_y;
                    entry_v1.width = entry_v2.width;
                    entry_v1.height = entry_v2.height;
                    memcpy(entry_v1.name, entry_v2.name, sizeof(entry_v1.name));
                } else if (version == 3u) {
                    LifecycleTestObjectChunkEntryV3 entry_v3;

                    memcpy(&entry_v3, cursor, sizeof(entry_v3));
                    cursor += sizeof(entry_v3);
                    entry_v1.object_id = entry_v3.object_id;
                    entry_v1.layer_id = entry_v3.layer_id;
                    entry_v1.type = entry_v3.type;
                    entry_v1.visible = entry_v3.visible;
                    entry_v1.locked = entry_v3.locked;
                    entry_v1.stroke_color_index = entry_v3.stroke_color_index;
                    entry_v1.fill_color_index = entry_v3.fill_color_index;
                    entry_v1.stroke_width = entry_v3.stroke_width;
                    entry_v1.style_mode = entry_v3.style_mode;
                    entry_v1.origin_x = entry_v3.origin_x;
                    entry_v1.origin_y = entry_v3.origin_y;
                    entry_v1.width = entry_v3.width;
                    entry_v1.height = entry_v3.height;
                    memcpy(entry_v1.name, entry_v3.name, sizeof(entry_v1.name));
                } else {
                    LifecycleTestObjectChunkEntryV4 entry_v4;

                    memcpy(&entry_v4, cursor, sizeof(entry_v4));
                    cursor += sizeof(entry_v4);
                    entry_v1.object_id = entry_v4.object_id;
                    entry_v1.layer_id = entry_v4.layer_id;
                    entry_v1.type = entry_v4.type;
                    entry_v1.visible = entry_v4.visible;
                    entry_v1.locked = entry_v4.locked;
                    entry_v1.stroke_color_index =
                        drawing_program_color_index_from_sample(entry_v4.stroke_color_value);
                    entry_v1.fill_color_index =
                        drawing_program_color_index_from_sample(entry_v4.fill_color_value);
                    entry_v1.stroke_width = entry_v4.stroke_width;
                    entry_v1.style_mode = entry_v4.style_mode;
                    entry_v1.origin_x = entry_v4.origin_x;
                    entry_v1.origin_y = entry_v4.origin_y;
                    entry_v1.width = entry_v4.width;
                    entry_v1.height = entry_v4.height;
                    memcpy(entry_v1.name, entry_v4.name, sizeof(entry_v1.name));
                }
                entry_v1.name[sizeof(entry_v1.name) - 1u] = '\0';
                memcpy(out_cursor, &entry_v1, sizeof(entry_v1));
                out_cursor += sizeof(entry_v1);
            }
        } else {
            free(dpui_data);
            free(dplr_data);
            free(dpob_data);
            free(dps2_data);
            fprintf(stderr, "lifecycle_test: unsupported DPOB version for legacy conversion\n");
            return 0;
        }
    }

    result = core_pack_writer_open(output_pack_path, &writer);
    if (result.code != CORE_OK) {
        free(dpob_v1_data);
        free(dpui_data);
        free(dplr_data);
        free(dpob_data);
        free(dps2_data);
        fprintf(stderr, "lifecycle_test: failed to open legacy-v1 output %s\n", output_pack_path);
        return 0;
    }
    result = core_pack_writer_add_chunk(&writer, shell_chunk_id, dps2_data, dps2_chunk.size);
    if (result.code == CORE_OK && has_dplr) {
        result = core_pack_writer_add_chunk(&writer, "DPLR", dplr_data, dplr_chunk.size);
    }
    if (result.code == CORE_OK) {
        result = core_pack_writer_add_chunk(&writer, "DPOB", dpob_v1_data, dpob_v1_size);
    }
    if (result.code == CORE_OK && has_dpui) {
        result = core_pack_writer_add_chunk(&writer, "DPUI", dpui_data, dpui_chunk.size);
    }
    if (result.code == CORE_OK) {
        result = core_pack_writer_close(&writer);
    } else {
        (void)core_pack_writer_close(&writer);
    }
    free(dpob_v1_data);
    free(dpui_data);
    free(dplr_data);
    free(dpob_data);
    free(dps2_data);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed writing legacy-v1 snapshot output\n");
        return 0;
    }
    return 1;
}

int write_legacy_snapshot_with_v2_object_chunk(const char *source_pack_path,
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
    uint8_t *dpob_v2_data = 0;
    uint64_t dpob_v2_size = 0u;
    const char *shell_chunk_id = 0;
    int has_dplr = 0;
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
    if (!lifecycle_snapshot_find_shell_chunk(&reader, &dps2_chunk, &shell_chunk_id)) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing shell chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPOB", 0u, &dpob_chunk);
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing DPOB chunk\n");
        return 0;
    }
    dps2_data = (uint8_t *)malloc((size_t)dps2_chunk.size);
    dpob_data = (uint8_t *)malloc((size_t)dpob_chunk.size);
    if (!dps2_data || !dpob_data) {
        free(dpob_data);
        free(dps2_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to allocate required snapshot buffers\n");
        return 0;
    }
    result = core_pack_reader_read_chunk_data(&reader, &dps2_chunk, dps2_data, dps2_chunk.size);
    if (result.code != CORE_OK) {
        free(dpob_data);
        free(dps2_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to read shell chunk\n");
        return 0;
    }
    result = core_pack_reader_read_chunk_data(&reader, &dpob_chunk, dpob_data, dpob_chunk.size);
    if (result.code != CORE_OK) {
        free(dpob_data);
        free(dps2_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to read DPOB chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPLR", 0u, &dplr_chunk);
    if (result.code == CORE_OK) {
        dplr_data = (uint8_t *)malloc((size_t)dplr_chunk.size);
        if (!dplr_data) {
            free(dpob_data);
            free(dps2_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPLR copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dplr_chunk, dplr_data, dplr_chunk.size);
        if (result.code != CORE_OK) {
            free(dplr_data);
            free(dpob_data);
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
            free(dpob_data);
            free(dps2_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPUI copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dpui_chunk, dpui_data, dpui_chunk.size);
        if (result.code != CORE_OK) {
            free(dpui_data);
            free(dplr_data);
            free(dpob_data);
            free(dps2_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPUI chunk\n");
            return 0;
        }
        has_dpui = 1;
    }
    (void)core_pack_reader_close(&reader);

    if (dpob_chunk.size < sizeof(uint32_t)) {
        free(dpui_data);
        free(dplr_data);
        free(dpob_data);
        free(dps2_data);
        fprintf(stderr, "lifecycle_test: DPOB chunk too small for version field\n");
        return 0;
    }
    {
        uint32_t version = 0u;
        memcpy(&version, dpob_data, sizeof(uint32_t));
        if (version == 2u) {
            dpob_v2_size = dpob_chunk.size;
            dpob_v2_data = (uint8_t *)malloc((size_t)dpob_v2_size);
            if (!dpob_v2_data) {
                free(dpui_data);
                free(dplr_data);
                free(dpob_data);
                free(dps2_data);
                fprintf(stderr, "lifecycle_test: failed to allocate DPOB v2 copy buffer\n");
                return 0;
            }
            memcpy(dpob_v2_data, dpob_data, (size_t)dpob_v2_size);
        } else if (version == 3u || version == 4u) {
            LifecycleTestObjectChunkHeaderV3 header_v3;
            LifecycleTestObjectChunkHeaderV2 header_v2;
            const uint8_t *cursor = dpob_data;
            uint8_t *out_cursor = 0;
            uint32_t i;
            uint64_t expected_size = 0u;

            memcpy(&header_v3, cursor, sizeof(header_v3));
            expected_size = (uint64_t)sizeof(header_v3) +
                            ((uint64_t)header_v3.object_count *
                             (uint64_t)(version == 3u ? sizeof(LifecycleTestObjectChunkEntryV3)
                                                      : sizeof(LifecycleTestObjectChunkEntryV4)));
            if (expected_size != dpob_chunk.size) {
                free(dpui_data);
                free(dplr_data);
                free(dpob_data);
                free(dps2_data);
                fprintf(stderr, "lifecycle_test: invalid DPOB v3 chunk size\n");
                return 0;
            }
            memset(&header_v2, 0, sizeof(header_v2));
            header_v2.version = 2u;
            header_v2.object_count = header_v3.object_count;
            header_v2.next_object_id = header_v3.next_object_id;
            dpob_v2_size = (uint64_t)sizeof(header_v2) +
                           ((uint64_t)header_v2.object_count *
                            (uint64_t)sizeof(LifecycleTestObjectChunkEntryV2));
            dpob_v2_data = (uint8_t *)malloc((size_t)dpob_v2_size);
            if (!dpob_v2_data) {
                free(dpui_data);
                free(dplr_data);
                free(dpob_data);
                free(dps2_data);
                fprintf(stderr, "lifecycle_test: failed to allocate DPOB v2 output buffer\n");
                return 0;
            }
            out_cursor = dpob_v2_data;
            memcpy(out_cursor, &header_v2, sizeof(header_v2));
            out_cursor += sizeof(header_v2);
            cursor += sizeof(header_v3);
            for (i = 0u; i < header_v3.object_count; ++i) {
                LifecycleTestObjectChunkEntryV2 entry_v2;
                uint32_t point_i;

                memset(&entry_v2, 0, sizeof(entry_v2));
                if (version == 3u) {
                    LifecycleTestObjectChunkEntryV3 entry_v3;

                    memcpy(&entry_v3, cursor, sizeof(entry_v3));
                    cursor += sizeof(entry_v3);
                    entry_v2.object_id = entry_v3.object_id;
                    entry_v2.layer_id = entry_v3.layer_id;
                    entry_v2.type = entry_v3.type;
                    entry_v2.visible = entry_v3.visible;
                    entry_v2.locked = entry_v3.locked;
                    entry_v2.stroke_color_index = entry_v3.stroke_color_index;
                    entry_v2.fill_color_index = entry_v3.fill_color_index;
                    entry_v2.stroke_width = entry_v3.stroke_width;
                    entry_v2.style_mode = entry_v3.style_mode;
                    entry_v2.path_closed = entry_v3.path_closed;
                    entry_v2.path_point_count = entry_v3.path_point_count;
                    entry_v2.origin_x = entry_v3.origin_x;
                    entry_v2.origin_y = entry_v3.origin_y;
                    entry_v2.width = entry_v3.width;
                    entry_v2.height = entry_v3.height;
                    memcpy(entry_v2.name, entry_v3.name, sizeof(entry_v2.name));
                    for (point_i = 0u;
                         point_i < (uint32_t)entry_v3.path_point_count &&
                         point_i < DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS;
                         ++point_i) {
                        entry_v2.path_points[point_i].x = entry_v3.path_points[point_i].x;
                        entry_v2.path_points[point_i].y = entry_v3.path_points[point_i].y;
                    }
                } else {
                    LifecycleTestObjectChunkEntryV4 entry_v4;

                    memcpy(&entry_v4, cursor, sizeof(entry_v4));
                    cursor += sizeof(entry_v4);
                    entry_v2.object_id = entry_v4.object_id;
                    entry_v2.layer_id = entry_v4.layer_id;
                    entry_v2.type = entry_v4.type;
                    entry_v2.visible = entry_v4.visible;
                    entry_v2.locked = entry_v4.locked;
                    entry_v2.stroke_color_index =
                        drawing_program_color_index_from_sample(entry_v4.stroke_color_value);
                    entry_v2.fill_color_index =
                        drawing_program_color_index_from_sample(entry_v4.fill_color_value);
                    entry_v2.stroke_width = entry_v4.stroke_width;
                    entry_v2.style_mode = entry_v4.style_mode;
                    entry_v2.path_closed = entry_v4.path_closed;
                    entry_v2.path_point_count = entry_v4.path_point_count;
                    entry_v2.origin_x = entry_v4.origin_x;
                    entry_v2.origin_y = entry_v4.origin_y;
                    entry_v2.width = entry_v4.width;
                    entry_v2.height = entry_v4.height;
                    memcpy(entry_v2.name, entry_v4.name, sizeof(entry_v2.name));
                    for (point_i = 0u;
                         point_i < (uint32_t)entry_v4.path_point_count &&
                         point_i < DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS;
                         ++point_i) {
                        entry_v2.path_points[point_i].x = entry_v4.path_points[point_i].x;
                        entry_v2.path_points[point_i].y = entry_v4.path_points[point_i].y;
                    }
                }
                entry_v2.name[sizeof(entry_v2.name) - 1u] = '\0';
                memcpy(out_cursor, &entry_v2, sizeof(entry_v2));
                out_cursor += sizeof(entry_v2);
            }
        } else {
            free(dpui_data);
            free(dplr_data);
            free(dpob_data);
            free(dps2_data);
            fprintf(stderr, "lifecycle_test: unsupported DPOB version for v2 conversion\n");
            return 0;
        }
    }

    result = core_pack_writer_open(output_pack_path, &writer);
    if (result.code != CORE_OK) {
        free(dpob_v2_data);
        free(dpui_data);
        free(dplr_data);
        free(dpob_data);
        free(dps2_data);
        fprintf(stderr, "lifecycle_test: failed to open legacy-v2 output %s\n", output_pack_path);
        return 0;
    }
    result = core_pack_writer_add_chunk(&writer, shell_chunk_id, dps2_data, dps2_chunk.size);
    if (result.code == CORE_OK && has_dplr) {
        result = core_pack_writer_add_chunk(&writer, "DPLR", dplr_data, dplr_chunk.size);
    }
    if (result.code == CORE_OK) {
        result = core_pack_writer_add_chunk(&writer, "DPOB", dpob_v2_data, dpob_v2_size);
    }
    if (result.code == CORE_OK && has_dpui) {
        result = core_pack_writer_add_chunk(&writer, "DPUI", dpui_data, dpui_chunk.size);
    }
    if (result.code == CORE_OK) {
        result = core_pack_writer_close(&writer);
    } else {
        (void)core_pack_writer_close(&writer);
    }
    free(dpob_v2_data);
    free(dpui_data);
    free(dplr_data);
    free(dpob_data);
    free(dps2_data);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed writing legacy-v2 snapshot output\n");
        return 0;
    }
    return 1;
}

int write_snapshot_with_invalid_path_payload(const char *source_pack_path,
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
    const char *shell_chunk_id = 0;
    int has_dplr = 0;
    int has_dpui = 0;
    int mutated = 0;

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
    if (!lifecycle_snapshot_find_shell_chunk(&reader, &dps2_chunk, &shell_chunk_id)) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing shell chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPOB", 0u, &dpob_chunk);
    if (result.code != CORE_OK) {
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: source snapshot missing DPOB chunk\n");
        return 0;
    }
    dps2_data = (uint8_t *)malloc((size_t)dps2_chunk.size);
    dpob_data = (uint8_t *)malloc((size_t)dpob_chunk.size);
    if (!dps2_data || !dpob_data) {
        free(dpob_data);
        free(dps2_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to allocate required snapshot buffers\n");
        return 0;
    }
    result = core_pack_reader_read_chunk_data(&reader, &dps2_chunk, dps2_data, dps2_chunk.size);
    if (result.code != CORE_OK) {
        free(dpob_data);
        free(dps2_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to read shell chunk\n");
        return 0;
    }
    result = core_pack_reader_read_chunk_data(&reader, &dpob_chunk, dpob_data, dpob_chunk.size);
    if (result.code != CORE_OK) {
        free(dpob_data);
        free(dps2_data);
        (void)core_pack_reader_close(&reader);
        fprintf(stderr, "lifecycle_test: failed to read DPOB chunk\n");
        return 0;
    }
    result = core_pack_reader_find_chunk(&reader, "DPLR", 0u, &dplr_chunk);
    if (result.code == CORE_OK) {
        dplr_data = (uint8_t *)malloc((size_t)dplr_chunk.size);
        if (!dplr_data) {
            free(dpob_data);
            free(dps2_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPLR copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dplr_chunk, dplr_data, dplr_chunk.size);
        if (result.code != CORE_OK) {
            free(dplr_data);
            free(dpob_data);
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
            free(dpob_data);
            free(dps2_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to allocate DPUI copy buffer\n");
            return 0;
        }
        result = core_pack_reader_read_chunk_data(&reader, &dpui_chunk, dpui_data, dpui_chunk.size);
        if (result.code != CORE_OK) {
            free(dpui_data);
            free(dplr_data);
            free(dpob_data);
            free(dps2_data);
            (void)core_pack_reader_close(&reader);
            fprintf(stderr, "lifecycle_test: failed to read DPUI chunk\n");
            return 0;
        }
        has_dpui = 1;
    }
    (void)core_pack_reader_close(&reader);
    if (dpob_chunk.size < sizeof(LifecycleTestObjectChunkHeaderV2)) {
        free(dpui_data);
        free(dplr_data);
        free(dpob_data);
        free(dps2_data);
        fprintf(stderr, "lifecycle_test: DPOB too small for v2 header\n");
        return 0;
    }
    {
        LifecycleTestObjectChunkHeaderV2 header_v2;
        uint8_t *cursor = dpob_data;
        uint32_t i;
        uint64_t expected_size = 0u;

        memcpy(&header_v2, cursor, sizeof(header_v2));
        if (header_v2.version != 2u && header_v2.version != 3u && header_v2.version != 4u) {
            free(dpui_data);
            free(dplr_data);
            free(dpob_data);
            free(dps2_data);
            fprintf(stderr, "lifecycle_test: invalid-path helper requires DPOB v2, v3, or v4\n");
            return 0;
        }
        expected_size = (uint64_t)sizeof(header_v2) +
                        ((uint64_t)header_v2.object_count *
                         (uint64_t)(header_v2.version == 2u ? sizeof(LifecycleTestObjectChunkEntryV2)
                                                            : (header_v2.version == 3u
                                                                   ? sizeof(LifecycleTestObjectChunkEntryV3)
                                                                   : sizeof(LifecycleTestObjectChunkEntryV4))));
        if (expected_size != dpob_chunk.size) {
            free(dpui_data);
            free(dplr_data);
            free(dpob_data);
            free(dps2_data);
            fprintf(stderr, "lifecycle_test: invalid-path helper saw malformed DPOB v2 size\n");
            return 0;
        }
        cursor += sizeof(header_v2);
        for (i = 0u; i < header_v2.object_count; ++i) {
            if (header_v2.version == 2u) {
                LifecycleTestObjectChunkEntryV2 entry;

                memcpy(&entry, cursor, sizeof(entry));
                if (entry.type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
                    entry.path_point_count = (uint16_t)(DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS + 1u);
                    memcpy(cursor, &entry, sizeof(entry));
                    mutated = 1;
                    break;
                }
                cursor += sizeof(LifecycleTestObjectChunkEntryV2);
            } else if (header_v2.version == 3u) {
                LifecycleTestObjectChunkEntryV3 entry;

                memcpy(&entry, cursor, sizeof(entry));
                if (entry.type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
                    entry.path_point_count = (uint16_t)(DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS + 1u);
                    memcpy(cursor, &entry, sizeof(entry));
                    mutated = 1;
                    break;
                }
                cursor += sizeof(LifecycleTestObjectChunkEntryV3);
            } else {
                LifecycleTestObjectChunkEntryV4 entry;

                memcpy(&entry, cursor, sizeof(entry));
                if (entry.type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
                    entry.path_point_count = (uint16_t)(DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS + 1u);
                    memcpy(cursor, &entry, sizeof(entry));
                    mutated = 1;
                    break;
                }
                cursor += sizeof(LifecycleTestObjectChunkEntryV4);
            }
        }
    }
    if (!mutated) {
        free(dpui_data);
        free(dplr_data);
        free(dpob_data);
        free(dps2_data);
        fprintf(stderr, "lifecycle_test: invalid-path helper did not find a PATH object to mutate\n");
        return 0;
    }
    result = core_pack_writer_open(output_pack_path, &writer);
    if (result.code != CORE_OK) {
        free(dpui_data);
        free(dplr_data);
        free(dpob_data);
        free(dps2_data);
        fprintf(stderr, "lifecycle_test: failed to open invalid-path output %s\n", output_pack_path);
        return 0;
    }
    result = core_pack_writer_add_chunk(&writer, shell_chunk_id, dps2_data, dps2_chunk.size);
    if (result.code == CORE_OK && has_dplr) {
        result = core_pack_writer_add_chunk(&writer, "DPLR", dplr_data, dplr_chunk.size);
    }
    if (result.code == CORE_OK) {
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
    free(dplr_data);
    free(dpob_data);
    free(dps2_data);
    if (result.code != CORE_OK) {
        fprintf(stderr, "lifecycle_test: failed writing invalid-path snapshot output\n");
        return 0;
    }
    return 1;
}
