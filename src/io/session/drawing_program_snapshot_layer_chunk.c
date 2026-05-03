#include "drawing_program_snapshot_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V1 = 1u,
    DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V2 = 2u
};

typedef struct DrawingProgramLegacyLayerChunkSeed {
    uint32_t raster_width;
    uint32_t raster_height;
    uint32_t sample_count;
    uint32_t layer_count;
    uint32_t layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t max_layer_id;
} DrawingProgramLegacyLayerChunkSeed;

static int drawing_program_snapshot_document_has_layer_id(
    const DrawingProgramDocument *document,
    uint32_t layer_id) {
    uint32_t i;
    if (!document || layer_id == 0u) {
        return 0;
    }
    for (i = 0u; i < document->layer_count; ++i) {
        if (document->layers[i].layer_id == layer_id) {
            return 1;
        }
    }
    return 0;
}

static CoreResult drawing_program_snapshot_inspect_layer_raster_chunk(
    const void *chunk_data,
    uint64_t chunk_size,
    DrawingProgramLegacyLayerChunkSeed *out_seed) {
    const uint8_t *cursor = (const uint8_t *)chunk_data;
    const uint8_t *end = cursor + chunk_size;
    uint32_t version = 0u;
    uint32_t i;
    if (!cursor || !out_seed || chunk_size < (uint64_t)(sizeof(uint32_t) * 5u)) {
        return drawing_program_snapshot_invalid("invalid legacy layer chunk inspect request");
    }
    memset(out_seed, 0, sizeof(*out_seed));
    memcpy(&version, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&out_seed->raster_width, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&out_seed->raster_height, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&out_seed->sample_count, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&out_seed->layer_count, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    if (version != DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V1 &&
        version != DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V2) {
        return (CoreResult){ CORE_ERR_FORMAT, "unsupported drawing layer raster chunk version" };
    }
    if (out_seed->raster_width == 0u ||
        out_seed->raster_height == 0u ||
        out_seed->sample_count == 0u ||
        out_seed->sample_count != (out_seed->raster_width * out_seed->raster_height) ||
        out_seed->layer_count == 0u ||
        out_seed->layer_count > DRAWING_PROGRAM_MAX_LAYERS) {
        return (CoreResult){ CORE_ERR_FORMAT, "invalid legacy layer chunk bounds" };
    }
    for (i = 0u; i < out_seed->layer_count; ++i) {
        uint32_t layer_id = 0u;
        uint32_t entry_sample_count = 0u;
        uint64_t sample_bytes = 0u;
        if ((uint64_t)(end - cursor) < (uint64_t)(sizeof(uint32_t) * 2u)) {
            return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster chunk truncated header" };
        }
        memcpy(&layer_id, cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        memcpy(&entry_sample_count, cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        if (layer_id == 0u || entry_sample_count != out_seed->sample_count) {
            return (CoreResult){ CORE_ERR_FORMAT, "invalid legacy layer chunk entry" };
        }
        out_seed->layer_ids[i] = layer_id;
        if (layer_id > out_seed->max_layer_id) {
            out_seed->max_layer_id = layer_id;
        }
        sample_bytes = (version == DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V2)
                           ? ((uint64_t)entry_sample_count * (uint64_t)sizeof(DrawingProgramRasterSample))
                           : (uint64_t)entry_sample_count;
        if ((uint64_t)(end - cursor) < sample_bytes) {
            return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster chunk truncated samples" };
        }
        cursor += sample_bytes;
    }
    if (cursor != end) {
        return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster chunk trailing bytes" };
    }
    return core_result_ok();
}

static CoreResult drawing_program_snapshot_seed_document_from_legacy_layer_chunk(
    struct DrawingProgramAppContext *ctx,
    const void *chunk_data,
    uint64_t chunk_size) {
    DrawingProgramLegacyLayerChunkSeed seed;
    CoreResult result;
    uint32_t i;
    if (!ctx || !chunk_data) {
        return drawing_program_snapshot_invalid("invalid legacy layer chunk bootstrap request");
    }
    result = drawing_program_snapshot_inspect_layer_raster_chunk(chunk_data, chunk_size, &seed);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_document_init_with_shape(&ctx->document, seed.raster_width, seed.raster_height, 1u);
    if (result.code != CORE_OK) {
        return result;
    }
    ctx->document.layer_count = seed.layer_count;
    ctx->document.next_layer_id = seed.max_layer_id + 1u;
    if (ctx->document.next_layer_id == 0u) {
        ctx->document.next_layer_id = 1u;
    }
    memset(ctx->document.layers, 0, sizeof(ctx->document.layers));
    for (i = 0u; i < seed.layer_count; ++i) {
        ctx->document.layers[i].layer_id = seed.layer_ids[i];
        ctx->document.layers[i].visible = 1u;
        ctx->document.layers[i].locked = 0u;
        if (i == 0u) {
            (void)snprintf(ctx->document.layers[i].name,
                           sizeof(ctx->document.layers[i].name),
                           "%s",
                           "Base Layer");
        } else {
            (void)snprintf(ctx->document.layers[i].name,
                           sizeof(ctx->document.layers[i].name),
                           "Layer %u",
                           (unsigned)ctx->document.layers[i].layer_id);
        }
    }
    drawing_program_editor_state_init(&ctx->editor, &ctx->document);
    drawing_program_history_clear(&ctx->history);
    drawing_program_selection_reset(&ctx->selection);
    drawing_program_object_selection_reset(&ctx->object_selection);
    return core_result_ok();
}

CoreResult drawing_program_snapshot_load_legacy_chunked_fallback(
    struct DrawingProgramAppContext *ctx,
    CorePackReader *reader) {
    CorePackChunkInfo layer_chunk;
    CoreResult result;
    uint8_t *layer_chunk_data = 0;
    if (!ctx || !reader) {
        return drawing_program_snapshot_invalid("invalid legacy snapshot fallback load request");
    }
    memset(&layer_chunk, 0, sizeof(layer_chunk));
    result = core_pack_reader_find_chunk(reader, "DPLR", 0u, &layer_chunk);
    if (result.code != CORE_OK) {
        return result;
    }
    layer_chunk_data = (uint8_t *)malloc((size_t)layer_chunk.size);
    if (!layer_chunk_data) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate legacy layer raster chunk buffer" };
    }
    result = core_pack_reader_read_chunk_data(reader, &layer_chunk, layer_chunk_data, layer_chunk.size);
    if (result.code == CORE_OK) {
        result = drawing_program_snapshot_seed_document_from_legacy_layer_chunk(ctx, layer_chunk_data, layer_chunk.size);
    }
    free(layer_chunk_data);
    return result;
}

CoreResult drawing_program_snapshot_upgrade_legacy_palette_index_samples(
    struct DrawingProgramAppContext *ctx,
    uint8_t *out_upgraded) {
    uint8_t upgraded = 0u;
    uint32_t i;
    if (!ctx) {
        return drawing_program_snapshot_invalid("invalid palette sample upgrade request");
    }
    if (ctx->document.schema_version >= DRAWING_PROGRAM_DOCUMENT_SCHEMA_VERSION_TRUE_COLOR) {
        if (out_upgraded) {
            *out_upgraded = 0u;
        }
        return core_result_ok();
    }
    for (i = 0u; i < ctx->document.raster_sample_count; ++i) {
        DrawingProgramRasterSample normalized = drawing_program_color_normalize_legacy_sample(
            (uint8_t)(ctx->document.raster_samples[i] & 0xffu));
        if (normalized != ctx->document.raster_samples[i]) {
            ctx->document.raster_samples[i] = normalized;
            upgraded = 1u;
        }
    }
    if (ctx->layer_rasters.slot_samples &&
        ctx->layer_rasters.sample_count == ctx->document.raster_sample_count) {
        uint32_t slot;
        for (slot = 0u; slot < ctx->layer_rasters.slot_capacity; ++slot) {
            DrawingProgramRasterSample *slot_samples;
            if (ctx->layer_rasters.slot_layer_ids[slot] == 0u) {
                continue;
            }
            slot_samples = ctx->layer_rasters.slot_samples +
                           ((size_t)slot * (size_t)ctx->layer_rasters.sample_count);
            for (i = 0u; i < ctx->layer_rasters.sample_count; ++i) {
                DrawingProgramRasterSample normalized = drawing_program_color_normalize_legacy_sample(
                    (uint8_t)(slot_samples[i] & 0xffu));
                if (normalized != slot_samples[i]) {
                    slot_samples[i] = normalized;
                    upgraded = 1u;
                }
            }
        }
    }
    if (ctx->document.schema_version != DRAWING_PROGRAM_DOCUMENT_SCHEMA_VERSION_TRUE_COLOR) {
        ctx->document.schema_version = DRAWING_PROGRAM_DOCUMENT_SCHEMA_VERSION_TRUE_COLOR;
        upgraded = 1u;
    }
    if (out_upgraded) {
        *out_upgraded = upgraded;
    }
    return core_result_ok();
}

CoreResult drawing_program_snapshot_write_layer_raster_chunk(
    CorePackWriter *writer,
    const struct DrawingProgramAppContext *ctx) {
    uint64_t header_bytes;
    uint64_t layer_bytes;
    uint64_t payload_size;
    uint8_t *payload = 0;
    uint8_t *cursor = 0;
    uint32_t i;
    const uint32_t sample_count = ctx->document.raster_sample_count;
    const uint64_t sample_bytes = (uint64_t)sample_count * (uint64_t)sizeof(DrawingProgramRasterSample);
    if (!writer || !ctx) {
        return drawing_program_snapshot_invalid("invalid layer raster chunk write request");
    }
    header_bytes = (uint64_t)(sizeof(uint32_t) * 5u);
    layer_bytes = (uint64_t)(sizeof(uint32_t) * 2u) + sample_bytes;
    if (sample_count == 0u || ctx->document.layer_count == 0u) {
        return core_result_ok();
    }
    payload_size = header_bytes + (layer_bytes * (uint64_t)ctx->document.layer_count);
    payload = (uint8_t *)malloc((size_t)payload_size);
    if (!payload) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate layer raster snapshot payload" };
    }
    cursor = payload;
    memcpy(cursor, &(uint32_t){ DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V2 }, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &ctx->document.raster_width, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &ctx->document.raster_height, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &ctx->document.raster_sample_count, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &ctx->document.layer_count, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    for (i = 0u; i < ctx->document.layer_count; ++i) {
        const uint32_t layer_id = ctx->document.layers[i].layer_id;
        const DrawingProgramRasterSample *samples = 0;
        uint32_t exported_sample_count = 0u;
        CoreResult export_result = { CORE_ERR_NOT_FOUND, "layer raster export unavailable" };
        memcpy(cursor, &layer_id, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        memcpy(cursor, &sample_count, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        if (i == 0u) {
            memcpy(cursor, ctx->document.raster_samples, (size_t)sample_bytes);
            cursor += sample_bytes;
            continue;
        }
        export_result = drawing_program_layer_raster_store_export_layer(&ctx->layer_rasters,
                                                                        layer_id,
                                                                        &samples,
                                                                        &exported_sample_count);
        if (export_result.code == CORE_OK &&
            samples &&
            exported_sample_count == sample_count) {
            memcpy(cursor, samples, (size_t)sample_bytes);
        } else {
            uint32_t sample_i;
            DrawingProgramRasterSample *write_samples = (DrawingProgramRasterSample *)cursor;
            DrawingProgramRasterSample empty_sample = drawing_program_color_eraser_value();
            for (sample_i = 0u; sample_i < sample_count; ++sample_i) {
                write_samples[sample_i] = empty_sample;
            }
        }
        cursor += sample_bytes;
    }
    {
        CoreResult result = core_pack_writer_add_chunk(writer, "DPLR", payload, payload_size);
        free(payload);
        return result;
    }
}

CoreResult drawing_program_snapshot_apply_layer_raster_chunk(
    struct DrawingProgramAppContext *ctx,
    const void *chunk_data,
    uint64_t chunk_size) {
    const uint8_t *cursor = (const uint8_t *)chunk_data;
    const uint8_t *end = cursor + chunk_size;
    uint32_t version = 0u;
    uint32_t raster_width = 0u;
    uint32_t raster_height = 0u;
    uint32_t sample_count = 0u;
    uint32_t layer_count = 0u;
    uint32_t i;
    if (!ctx || !cursor || chunk_size < (uint64_t)(sizeof(uint32_t) * 5u)) {
        return drawing_program_snapshot_invalid("invalid layer raster chunk payload");
    }
    memcpy(&version, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&raster_width, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&raster_height, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&sample_count, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(&layer_count, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    if (version != DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V1 &&
        version != DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V2) {
        return (CoreResult){ CORE_ERR_FORMAT, "unsupported drawing layer raster chunk version" };
    }
    if (raster_width != ctx->document.raster_width ||
        raster_height != ctx->document.raster_height ||
        sample_count != ctx->document.raster_sample_count ||
        layer_count > DRAWING_PROGRAM_MAX_LAYERS) {
        return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster chunk shape mismatch" };
    }
    for (i = 0u; i < layer_count; ++i) {
        uint32_t layer_id = 0u;
        uint32_t entry_sample_count = 0u;
        if ((uint64_t)(end - cursor) < (uint64_t)(sizeof(uint32_t) * 2u)) {
            return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster chunk truncated header" };
        }
        memcpy(&layer_id, cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        memcpy(&entry_sample_count, cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        if (entry_sample_count != sample_count) {
            return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster entry sample count mismatch" };
        }
        if ((uint64_t)(end - cursor) <
            ((version == DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V2)
                 ? ((uint64_t)entry_sample_count * (uint64_t)sizeof(DrawingProgramRasterSample))
                 : (uint64_t)entry_sample_count)) {
            return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster chunk truncated samples" };
        }
        if (drawing_program_snapshot_document_has_layer_id(&ctx->document, layer_id)) {
            CoreResult import_result;
            const uint32_t base_layer_id =
                (ctx->document.layer_count > 0u) ? ctx->document.layers[0].layer_id : 0u;
            if (version == DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V2) {
                import_result = drawing_program_layer_raster_store_import_layer(
                    &ctx->layer_rasters,
                    layer_id,
                    (const DrawingProgramRasterSample *)cursor,
                    entry_sample_count);
                if (import_result.code == CORE_OK && layer_id == base_layer_id) {
                    memcpy(ctx->document.raster_samples,
                           cursor,
                           (size_t)entry_sample_count * sizeof(ctx->document.raster_samples[0]));
                }
            } else {
                DrawingProgramRasterSample *legacy_samples = 0;
                uint32_t sample_i;
                legacy_samples =
                    (DrawingProgramRasterSample *)malloc((size_t)entry_sample_count * sizeof(*legacy_samples));
                if (!legacy_samples) {
                    return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate legacy raster upgrade buffer" };
                }
                for (sample_i = 0u; sample_i < entry_sample_count; ++sample_i) {
                    legacy_samples[sample_i] = drawing_program_color_normalize_legacy_sample(cursor[sample_i]);
                }
                import_result = drawing_program_layer_raster_store_import_layer(
                    &ctx->layer_rasters,
                    layer_id,
                    legacy_samples,
                    entry_sample_count);
                if (import_result.code == CORE_OK && layer_id == base_layer_id) {
                    memcpy(ctx->document.raster_samples,
                           legacy_samples,
                           (size_t)entry_sample_count * sizeof(ctx->document.raster_samples[0]));
                }
                free(legacy_samples);
            }
            if (import_result.code != CORE_OK) {
                return import_result;
            }
        }
        cursor += (version == DRAWING_PROGRAM_LAYER_RASTER_CHUNK_VERSION_V2)
                      ? ((uint64_t)entry_sample_count * (uint64_t)sizeof(DrawingProgramRasterSample))
                      : (uint64_t)entry_sample_count;
    }
    if (cursor != end) {
        return (CoreResult){ CORE_ERR_FORMAT, "drawing layer raster chunk trailing bytes" };
    }
    return core_result_ok();
}
