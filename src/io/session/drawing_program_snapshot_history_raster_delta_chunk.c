#include "drawing_program_snapshot_internal.h"

#include <stdlib.h>
#include <string.h>

typedef struct DrawingProgramHistoryRasterDeltaChunkHeaderV1 {
    uint32_t version;
    uint32_t delta_count;
} DrawingProgramHistoryRasterDeltaChunkHeaderV1;

enum {
    DRAWING_PROGRAM_HISTORY_RASTER_DELTA_CHUNK_VERSION_V1 = 1u
};

CoreResult drawing_program_snapshot_write_history_raster_delta_chunk(
    CorePackWriter *writer,
    const struct DrawingProgramAppContext *ctx) {
    DrawingProgramHistoryRasterDeltaChunkHeaderV1 header;
    uint64_t payload_size;
    uint8_t *payload = 0;
    uint8_t *cursor = 0;
    if (!writer || !ctx) {
        return drawing_program_snapshot_invalid("invalid history raster-delta chunk write request");
    }
    memset(&header, 0, sizeof(header));
    header.version = DRAWING_PROGRAM_HISTORY_RASTER_DELTA_CHUNK_VERSION_V1;
    header.delta_count = ctx->history.raster_delta_count;
    payload_size = (uint64_t)sizeof(header) +
                   ((uint64_t)header.delta_count * (uint64_t)sizeof(ctx->history.raster_delta_entries[0]));
    payload = (uint8_t *)malloc((size_t)payload_size);
    if (!payload) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate history raster-delta snapshot payload" };
    }
    cursor = payload;
    memcpy(cursor, &header, sizeof(header));
    cursor += sizeof(header);
    if (header.delta_count > 0u) {
        memcpy(cursor,
               ctx->history.raster_delta_entries,
               (size_t)header.delta_count * sizeof(ctx->history.raster_delta_entries[0]));
    }
    {
        CoreResult result = core_pack_writer_add_chunk(writer, "DPHD", payload, payload_size);
        free(payload);
        return result;
    }
}

CoreResult drawing_program_snapshot_apply_history_raster_delta_chunk(
    struct DrawingProgramAppContext *ctx,
    CorePackReader *reader) {
    CorePackChunkInfo chunk;
    DrawingProgramHistoryRasterDeltaChunkHeaderV1 header;
    uint8_t *payload = 0;
    CoreResult result;
    if (!ctx || !reader) {
        return drawing_program_snapshot_invalid("invalid history raster-delta chunk load request");
    }
    ctx->history.raster_delta_count = 0u;
    memset(&chunk, 0, sizeof(chunk));
    result = core_pack_reader_find_chunk(reader, "DPHD", 0u, &chunk);
    if (result.code == CORE_ERR_NOT_FOUND) {
        return drawing_program_history_validate_raster_delta_storage(&ctx->history);
    }
    if (result.code != CORE_OK) {
        return result;
    }
    if (chunk.size < (uint64_t)sizeof(header)) {
        return (CoreResult){ CORE_ERR_FORMAT, "history raster-delta chunk truncated header" };
    }
    payload = (uint8_t *)malloc((size_t)chunk.size);
    if (!payload) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate history raster-delta chunk buffer" };
    }
    result = core_pack_reader_read_chunk_data(reader, &chunk, payload, chunk.size);
    if (result.code != CORE_OK) {
        free(payload);
        return result;
    }
    memcpy(&header, payload, sizeof(header));
    if (header.version != DRAWING_PROGRAM_HISTORY_RASTER_DELTA_CHUNK_VERSION_V1) {
        free(payload);
        return (CoreResult){ CORE_ERR_FORMAT, "unsupported history raster-delta chunk version" };
    }
    if (header.delta_count > DRAWING_PROGRAM_HISTORY_RASTER_DELTA_CAPACITY ||
        chunk.size != (uint64_t)sizeof(header) +
                         ((uint64_t)header.delta_count * (uint64_t)sizeof(ctx->history.raster_delta_entries[0]))) {
        free(payload);
        return (CoreResult){ CORE_ERR_FORMAT, "invalid history raster-delta chunk bounds" };
    }
    if (header.delta_count > 0u) {
        memcpy(ctx->history.raster_delta_entries,
               payload + sizeof(header),
               (size_t)header.delta_count * sizeof(ctx->history.raster_delta_entries[0]));
    }
    ctx->history.raster_delta_count = header.delta_count;
    free(payload);
    return drawing_program_history_validate_raster_delta_storage(&ctx->history);
}
