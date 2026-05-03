#ifndef DRAWING_PROGRAM_SNAPSHOT_INTERNAL_H
#define DRAWING_PROGRAM_SNAPSHOT_INTERNAL_H

#include "core_pack.h"
#include "drawing_program/drawing_program_app_main.h"

CoreResult drawing_program_snapshot_invalid(const char *message);

CoreResult drawing_program_snapshot_load_legacy_chunked_fallback(
    struct DrawingProgramAppContext *ctx,
    CorePackReader *reader);
CoreResult drawing_program_snapshot_upgrade_legacy_palette_index_samples(
    struct DrawingProgramAppContext *ctx,
    uint8_t *out_upgraded);
CoreResult drawing_program_snapshot_write_layer_raster_chunk(
    CorePackWriter *writer,
    const struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_snapshot_apply_layer_raster_chunk(
    struct DrawingProgramAppContext *ctx,
    const void *chunk_data,
    uint64_t chunk_size);
CoreResult drawing_program_snapshot_write_object_chunk(
    CorePackWriter *writer,
    const struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_snapshot_apply_object_chunk(
    struct DrawingProgramAppContext *ctx,
    const void *chunk_data,
    uint64_t chunk_size);

#endif
