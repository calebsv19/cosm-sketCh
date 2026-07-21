#include "drawing_program/drawing_program_indexed_layer_raster.h"

#include <stdlib.h>
#include <string.h>

static CoreResult indexed_raster_invalid(const char *message) {
    CoreResult result = { CORE_ERR_INVALID_ARG, message };
    return result;
}

static CoreResult indexed_raster_shape(uint32_t width, uint32_t height, uint32_t *out_count) {
    uint64_t count = (uint64_t)width * (uint64_t)height;
    if (!out_count || width == 0u || height == 0u || count > UINT32_MAX) {
        return indexed_raster_invalid("invalid indexed raster dimensions");
    }
    *out_count = (uint32_t)count;
    return core_result_ok();
}

void drawing_program_indexed_layer_raster_dispose(
    DrawingProgramIndexedLayerRaster *raster) {
    if (!raster) {
        return;
    }
    free(raster->indices);
    memset(raster, 0, sizeof(*raster));
}

CoreResult drawing_program_indexed_layer_raster_init(
    DrawingProgramIndexedLayerRaster *raster,
    uint32_t width,
    uint32_t height,
    uint32_t slot_count,
    uint8_t fill_index) {
    uint32_t index_count = 0u;
    uint8_t *indices = 0;
    CoreResult result;
    if (!raster || slot_count == 0u || slot_count > 256u || fill_index >= slot_count) {
        return indexed_raster_invalid("invalid indexed raster init request");
    }
    result = indexed_raster_shape(width, height, &index_count);
    if (result.code != CORE_OK) {
        return result;
    }
    indices = (uint8_t *)malloc((size_t)index_count);
    if (!indices) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate indexed raster" };
    }
    memset(indices, fill_index, (size_t)index_count);
    drawing_program_indexed_layer_raster_dispose(raster);
    raster->width = width;
    raster->height = height;
    raster->index_count = index_count;
    raster->slot_count = slot_count;
    raster->indices = indices;
    return core_result_ok();
}

CoreResult drawing_program_indexed_layer_raster_import(
    DrawingProgramIndexedLayerRaster *raster,
    uint32_t width,
    uint32_t height,
    uint32_t slot_count,
    const uint8_t *indices,
    uint32_t index_count) {
    DrawingProgramIndexedLayerRaster next;
    uint32_t expected_count = 0u;
    uint32_t i;
    CoreResult result;
    if (!raster || !indices || slot_count == 0u || slot_count > 256u) {
        return indexed_raster_invalid("invalid indexed raster import request");
    }
    result = indexed_raster_shape(width, height, &expected_count);
    if (result.code != CORE_OK || expected_count != index_count) {
        return indexed_raster_invalid("indexed raster import shape mismatch");
    }
    for (i = 0u; i < index_count; ++i) {
        if ((uint32_t)indices[i] >= slot_count) {
            return indexed_raster_invalid("indexed raster contains out-of-range slot");
        }
    }
    memset(&next, 0, sizeof(next));
    result = drawing_program_indexed_layer_raster_init(&next, width, height, slot_count, 0u);
    if (result.code != CORE_OK) {
        return result;
    }
    memcpy(next.indices, indices, (size_t)index_count);
    drawing_program_indexed_layer_raster_dispose(raster);
    *raster = next;
    return core_result_ok();
}

CoreResult drawing_program_indexed_layer_raster_clone(
    DrawingProgramIndexedLayerRaster *out_raster,
    const DrawingProgramIndexedLayerRaster *source) {
    CoreResult result = drawing_program_indexed_layer_raster_validate(source);
    if (!out_raster || result.code != CORE_OK) {
        return result.code == CORE_OK ? indexed_raster_invalid("invalid indexed raster clone target") : result;
    }
    return drawing_program_indexed_layer_raster_import(out_raster,
                                                       source->width,
                                                       source->height,
                                                       source->slot_count,
                                                       source->indices,
                                                       source->index_count);
}

CoreResult drawing_program_indexed_layer_raster_validate(
    const DrawingProgramIndexedLayerRaster *raster) {
    uint32_t expected_count = 0u;
    uint32_t i;
    if (!raster || !raster->indices || raster->slot_count == 0u || raster->slot_count > 256u ||
        indexed_raster_shape(raster->width, raster->height, &expected_count).code != CORE_OK ||
        expected_count != raster->index_count) {
        return indexed_raster_invalid("invalid indexed raster state");
    }
    for (i = 0u; i < raster->index_count; ++i) {
        if ((uint32_t)raster->indices[i] >= raster->slot_count) {
            return indexed_raster_invalid("indexed raster contains out-of-range slot");
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_indexed_layer_raster_read(
    const DrawingProgramIndexedLayerRaster *raster,
    uint32_t x,
    uint32_t y,
    uint8_t *out_index) {
    if (!out_index || drawing_program_indexed_layer_raster_validate(raster).code != CORE_OK ||
        x >= raster->width || y >= raster->height) {
        return indexed_raster_invalid("invalid indexed raster read request");
    }
    *out_index = raster->indices[(y * raster->width) + x];
    return core_result_ok();
}

CoreResult drawing_program_indexed_layer_raster_write(
    DrawingProgramIndexedLayerRaster *raster,
    uint32_t x,
    uint32_t y,
    uint8_t index) {
    if (!raster || !raster->indices || x >= raster->width || y >= raster->height ||
        (uint32_t)index >= raster->slot_count) {
        return indexed_raster_invalid("invalid indexed raster write request");
    }
    raster->indices[(y * raster->width) + x] = index;
    return core_result_ok();
}
