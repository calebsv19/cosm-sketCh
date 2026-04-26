#include "drawing_program/drawing_program_layer_raster.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "drawing_program/drawing_program_color_model.h"

static CoreResult layer_raster_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static int layer_raster_document_shape_valid(const DrawingProgramDocument *document) {
    return document &&
           document->layer_count <= DRAWING_PROGRAM_MAX_LAYERS &&
           document->raster_width > 0u &&
           document->raster_height > 0u &&
           document->raster_sample_count > 0u &&
           document->raster_sample_count <= DRAWING_PROGRAM_MAX_RASTER_SAMPLES;
}

static DrawingProgramRasterSample *layer_raster_slot_ptr(DrawingProgramLayerRasterStore *store,
                                                         uint32_t slot_index) {
    if (!store || !store->slot_samples || slot_index >= store->slot_capacity) {
        return 0;
    }
    return store->slot_samples + ((size_t)slot_index * (size_t)store->sample_count);
}

static const DrawingProgramRasterSample *layer_raster_slot_ptr_const(const DrawingProgramLayerRasterStore *store,
                                                                     uint32_t slot_index) {
    if (!store || !store->slot_samples || slot_index >= store->slot_capacity) {
        return 0;
    }
    return store->slot_samples + ((size_t)slot_index * (size_t)store->sample_count);
}

static CoreResult layer_raster_find_slot_index(const DrawingProgramLayerRasterStore *store,
                                               uint32_t layer_id,
                                               uint32_t *out_slot_index) {
    uint32_t i;
    if (!store || !out_slot_index || layer_id == 0u) {
        return layer_raster_invalid("invalid slot index query");
    }
    for (i = 0u; i < store->slot_capacity; ++i) {
        if (store->slot_layer_ids[i] == layer_id) {
            *out_slot_index = i;
            return core_result_ok();
        }
    }
    return (CoreResult){ CORE_ERR_NOT_FOUND, "layer raster slot not found" };
}

static CoreResult layer_raster_ensure_capacity(DrawingProgramLayerRasterStore *store, uint32_t required_capacity) {
    DrawingProgramRasterSample *next_samples = 0;
    uint64_t next_bytes_u64;
    size_t next_bytes;
    DrawingProgramRasterSample erase_value;
    uint32_t i;
    if (!store || required_capacity == 0u || required_capacity > DRAWING_PROGRAM_MAX_LAYERS) {
        return layer_raster_invalid("invalid raster capacity request");
    }
    if (store->sample_count == 0u || store->sample_count > DRAWING_PROGRAM_MAX_RASTER_SAMPLES) {
        return layer_raster_invalid("invalid raster sample count for capacity");
    }
    if (required_capacity <= store->slot_capacity && store->slot_samples) {
        return core_result_ok();
    }
    next_bytes_u64 = (uint64_t)required_capacity * (uint64_t)store->sample_count *
                     (uint64_t)sizeof(DrawingProgramRasterSample);
    if (next_bytes_u64 == 0u || next_bytes_u64 > (uint64_t)SIZE_MAX) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "raster capacity byte size invalid" };
    }
    next_bytes = (size_t)next_bytes_u64;
    next_samples = (DrawingProgramRasterSample *)malloc(next_bytes);
    if (!next_samples) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate layer raster storage" };
    }
    erase_value = drawing_program_color_eraser_value();
    for (i = 0u; i < required_capacity * store->sample_count; ++i) {
        next_samples[i] = erase_value;
    }
    if (store->slot_samples && store->slot_capacity > 0u) {
        uint32_t copy_slots = store->slot_capacity;
        if (copy_slots > required_capacity) {
            copy_slots = required_capacity;
        }
        for (i = 0u; i < copy_slots; ++i) {
            const DrawingProgramRasterSample *src = layer_raster_slot_ptr_const(store, i);
            DrawingProgramRasterSample *dst = next_samples + ((size_t)i * (size_t)store->sample_count);
            if (src) {
                memcpy(dst, src, (size_t)store->sample_count * sizeof(*dst));
            }
        }
    }
    free(store->slot_samples);
    store->slot_samples = next_samples;
    if (required_capacity > store->slot_capacity) {
        for (i = store->slot_capacity; i < required_capacity; ++i) {
            store->slot_layer_ids[i] = 0u;
        }
    }
    store->slot_capacity = required_capacity;
    if (store->slot_count > store->slot_capacity) {
        store->slot_count = store->slot_capacity;
    }
    return core_result_ok();
}

static CoreResult layer_raster_claim_slot(DrawingProgramLayerRasterStore *store,
                                          uint32_t layer_id,
                                          uint32_t *out_slot_index) {
    CoreResult result;
    uint32_t i;
    if (!store || layer_id == 0u || !out_slot_index) {
        return layer_raster_invalid("invalid layer raster slot claim");
    }
    result = layer_raster_find_slot_index(store, layer_id, out_slot_index);
    if (result.code == CORE_OK) {
        return core_result_ok();
    }
    for (i = 0u; i < store->slot_capacity; ++i) {
        if (store->slot_layer_ids[i] == 0u) {
            store->slot_layer_ids[i] = layer_id;
            store->slot_count += 1u;
            *out_slot_index = i;
            return core_result_ok();
        }
    }
    return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "no free layer raster slots" };
}

CoreResult drawing_program_layer_raster_store_init_from_document(
    DrawingProgramLayerRasterStore *store,
    const DrawingProgramDocument *document) {
    CoreResult result;
    uint32_t capacity = 1u;
    if (!store || !layer_raster_document_shape_valid(document)) {
        return layer_raster_invalid("invalid layer raster init request");
    }
    drawing_program_layer_raster_store_dispose(store);
    store->raster_width = document->raster_width;
    store->raster_height = document->raster_height;
    store->sample_count = document->raster_sample_count;
    if (document->layer_count > capacity) {
        capacity = document->layer_count;
    }
    result = layer_raster_ensure_capacity(store, capacity);
    if (result.code != CORE_OK) {
        drawing_program_layer_raster_store_dispose(store);
        return result;
    }
    result = drawing_program_layer_raster_store_sync_document_layers(store, document);
    if (result.code != CORE_OK) {
        drawing_program_layer_raster_store_dispose(store);
        return result;
    }
    return drawing_program_layer_raster_store_seed_from_legacy_surface(store, document);
}

void drawing_program_layer_raster_store_dispose(DrawingProgramLayerRasterStore *store) {
    if (!store) {
        return;
    }
    free(store->slot_samples);
    memset(store, 0, sizeof(*store));
}

CoreResult drawing_program_layer_raster_store_sync_document_layers(
    DrawingProgramLayerRasterStore *store,
    const DrawingProgramDocument *document) {
    uint8_t keep_slots[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t i;
    CoreResult result;
    if (!store || !layer_raster_document_shape_valid(document)) {
        return layer_raster_invalid("invalid layer raster sync request");
    }
    if (store->raster_width != document->raster_width ||
        store->raster_height != document->raster_height ||
        store->sample_count != document->raster_sample_count) {
        return (CoreResult){ CORE_ERR_FORMAT, "layer raster shape does not match document shape" };
    }
    result = layer_raster_ensure_capacity(store, document->layer_count > 0u ? document->layer_count : 1u);
    if (result.code != CORE_OK) {
        return result;
    }
    memset(keep_slots, 0, sizeof(keep_slots));
    for (i = 0u; i < document->layer_count; ++i) {
        uint32_t slot_index = 0u;
        result = layer_raster_claim_slot(store, document->layers[i].layer_id, &slot_index);
        if (result.code != CORE_OK) {
            return result;
        }
        keep_slots[slot_index] = 1u;
    }
    for (i = 0u; i < store->slot_capacity; ++i) {
        if (store->slot_layer_ids[i] != 0u && !keep_slots[i]) {
            DrawingProgramRasterSample *slot_samples = layer_raster_slot_ptr(store, i);
            store->slot_layer_ids[i] = 0u;
            if (store->slot_count > 0u) {
                store->slot_count -= 1u;
            }
            if (slot_samples && store->sample_count > 0u) {
                uint32_t sample_i;
                DrawingProgramRasterSample empty_sample = drawing_program_color_eraser_value();
                for (sample_i = 0u; sample_i < store->sample_count; ++sample_i) {
                    slot_samples[sample_i] = empty_sample;
                }
            }
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_layer_raster_store_seed_from_legacy_surface(
    DrawingProgramLayerRasterStore *store,
    const DrawingProgramDocument *document) {
    uint32_t slot_index = 0u;
    DrawingProgramRasterSample *slot_samples = 0;
    size_t copy_count = 0u;
    CoreResult result;
    if (!store || !layer_raster_document_shape_valid(document)) {
        return layer_raster_invalid("invalid legacy seed request");
    }
    if (document->layer_count == 0u) {
        return core_result_ok();
    }
    result = layer_raster_find_slot_index(store, document->layers[0].layer_id, &slot_index);
    if (result.code != CORE_OK) {
        result = layer_raster_claim_slot(store, document->layers[0].layer_id, &slot_index);
        if (result.code != CORE_OK) {
            return result;
        }
    }
    slot_samples = layer_raster_slot_ptr(store, slot_index);
    if (!slot_samples) {
        return (CoreResult){ CORE_ERR_FORMAT, "failed to resolve base layer slot buffer" };
    }
    {
        uint32_t i;
        DrawingProgramRasterSample empty_sample = drawing_program_color_eraser_value();
        for (i = 0u; i < store->sample_count; ++i) {
            slot_samples[i] = empty_sample;
        }
    }
    copy_count = (size_t)document->raster_sample_count;
    if (copy_count > (size_t)store->sample_count) {
        copy_count = (size_t)store->sample_count;
    }
    memcpy(slot_samples, document->raster_samples, copy_count * sizeof(*slot_samples));
    return core_result_ok();
}

CoreResult drawing_program_layer_raster_store_sample_read(
    const DrawingProgramLayerRasterStore *store,
    uint32_t layer_id,
    uint32_t sample_x,
    uint32_t sample_y,
    uint8_t *out_value) {
    DrawingProgramRasterSample sample = drawing_program_color_eraser_value();
    CoreResult result = drawing_program_layer_raster_store_raster_sample_read(store,
                                                                              layer_id,
                                                                              sample_x,
                                                                              sample_y,
                                                                              &sample);
    if (result.code != CORE_OK) {
        return result;
    }
    *out_value = drawing_program_color_legacy_sample_from_sample(sample);
    return core_result_ok();
}

CoreResult drawing_program_layer_raster_store_raster_sample_read(
    const DrawingProgramLayerRasterStore *store,
    uint32_t layer_id,
    uint32_t sample_x,
    uint32_t sample_y,
    DrawingProgramRasterSample *out_value) {
    CoreResult result;
    uint32_t slot_index = 0u;
    uint32_t sample_index = 0u;
    const DrawingProgramRasterSample *slot_samples = 0;
    if (!store || !out_value) {
        return layer_raster_invalid("invalid layer sample read request");
    }
    if (sample_x >= store->raster_width || sample_y >= store->raster_height) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "layer sample coordinate out of bounds" };
    }
    sample_index = (sample_y * store->raster_width) + sample_x;
    if (sample_index >= store->sample_count) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "layer sample index out of bounds" };
    }
    result = layer_raster_find_slot_index(store, layer_id, &slot_index);
    if (result.code != CORE_OK) {
        return result;
    }
    slot_samples = layer_raster_slot_ptr_const(store, slot_index);
    if (!slot_samples) {
        return (CoreResult){ CORE_ERR_FORMAT, "failed to resolve layer slot samples" };
    }
    *out_value = slot_samples[sample_index];
    return core_result_ok();
}

CoreResult drawing_program_layer_raster_store_sample_write(
    DrawingProgramLayerRasterStore *store,
    uint32_t layer_id,
    uint32_t sample_x,
    uint32_t sample_y,
    DrawingProgramRasterSample value,
    DrawingProgramRasterSample *out_previous_value) {
    CoreResult result;
    uint32_t slot_index = 0u;
    uint32_t sample_index = 0u;
    DrawingProgramRasterSample *slot_samples = 0;
    if (!store) {
        return layer_raster_invalid("invalid layer sample write request");
    }
    if (sample_x >= store->raster_width || sample_y >= store->raster_height) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "layer sample coordinate out of bounds" };
    }
    sample_index = (sample_y * store->raster_width) + sample_x;
    if (sample_index >= store->sample_count) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "layer sample index out of bounds" };
    }
    result = layer_raster_find_slot_index(store, layer_id, &slot_index);
    if (result.code != CORE_OK) {
        return result;
    }
    slot_samples = layer_raster_slot_ptr(store, slot_index);
    if (!slot_samples) {
        return (CoreResult){ CORE_ERR_FORMAT, "failed to resolve layer slot samples" };
    }
    if (out_previous_value) {
        *out_previous_value = slot_samples[sample_index];
    }
    slot_samples[sample_index] = drawing_program_color_normalize_input_sample(value);
    return core_result_ok();
}

CoreResult drawing_program_layer_raster_store_export_layer(
    const DrawingProgramLayerRasterStore *store,
    uint32_t layer_id,
    const DrawingProgramRasterSample **out_samples,
    uint32_t *out_sample_count) {
    CoreResult result;
    uint32_t slot_index = 0u;
    const DrawingProgramRasterSample *slot_samples = 0;
    if (!store || !out_samples || !out_sample_count) {
        return layer_raster_invalid("invalid layer export request");
    }
    result = layer_raster_find_slot_index(store, layer_id, &slot_index);
    if (result.code != CORE_OK) {
        return result;
    }
    slot_samples = layer_raster_slot_ptr_const(store, slot_index);
    if (!slot_samples) {
        return (CoreResult){ CORE_ERR_FORMAT, "failed to resolve layer export slot" };
    }
    *out_samples = slot_samples;
    *out_sample_count = store->sample_count;
    return core_result_ok();
}

CoreResult drawing_program_layer_raster_store_export_layer_mutable(
    DrawingProgramLayerRasterStore *store,
    uint32_t layer_id,
    DrawingProgramRasterSample **out_samples,
    uint32_t *out_sample_count) {
    CoreResult result;
    uint32_t slot_index = 0u;
    DrawingProgramRasterSample *slot_samples = 0;
    if (!store || !out_samples || !out_sample_count) {
        return layer_raster_invalid("invalid mutable layer export request");
    }
    result = layer_raster_find_slot_index(store, layer_id, &slot_index);
    if (result.code != CORE_OK) {
        return result;
    }
    slot_samples = layer_raster_slot_ptr(store, slot_index);
    if (!slot_samples) {
        return (CoreResult){ CORE_ERR_FORMAT, "failed to resolve mutable layer export slot" };
    }
    *out_samples = slot_samples;
    *out_sample_count = store->sample_count;
    return core_result_ok();
}

CoreResult drawing_program_layer_raster_store_import_layer(
    DrawingProgramLayerRasterStore *store,
    uint32_t layer_id,
    const DrawingProgramRasterSample *samples,
    uint32_t sample_count) {
    CoreResult result;
    uint32_t slot_index = 0u;
    DrawingProgramRasterSample *slot_samples = 0;
    if (!store || !samples || layer_id == 0u) {
        return layer_raster_invalid("invalid layer import request");
    }
    if (sample_count != store->sample_count) {
        return (CoreResult){ CORE_ERR_FORMAT, "layer import sample count mismatch" };
    }
    result = layer_raster_find_slot_index(store, layer_id, &slot_index);
    if (result.code != CORE_OK) {
        result = layer_raster_claim_slot(store, layer_id, &slot_index);
        if (result.code != CORE_OK) {
            return result;
        }
    }
    slot_samples = layer_raster_slot_ptr(store, slot_index);
    if (!slot_samples) {
        return (CoreResult){ CORE_ERR_FORMAT, "failed to resolve layer import slot" };
    }
    {
        uint32_t i;
        for (i = 0u; i < store->sample_count; ++i) {
            slot_samples[i] = drawing_program_color_normalize_input_sample(samples[i]);
        }
    }
    return core_result_ok();
}
