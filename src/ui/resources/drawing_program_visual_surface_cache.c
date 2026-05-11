#include "drawing_program/drawing_program_visual_surface_cache.h"

#include <stdlib.h>
#include <string.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_layer_raster.h"
#include "drawing_program/drawing_program_render_domain.h"
#include "drawing_program/drawing_program_texture_project.h"

typedef struct DrawingProgramVisualSurfaceCacheEntry {
    uint64_t project_epoch;
    uint64_t content_revision;
    uint64_t layer_opacity_revision;
    uint64_t pending_content_revision;
    uint64_t pending_layer_opacity_revision;
    uint32_t surface_id;
    uint32_t zoom_bucket_percent;
    uint32_t pending_zoom_bucket_percent;
    uint32_t width;
    uint32_t height;
    uint8_t has_sync_signature;
    uint8_t pending_sync;
    uint8_t pending_active_surface;
    SDL_Renderer *renderer_owner;
    SDL_Texture *texture;
    SDL_PixelFormat *pixel_format;
    const DrawingProgramDocument *pending_document;
    const DrawingProgramLayerRasterStore *pending_layer_rasters;
    uint8_t pending_layer_opacity[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t pending_layer_opacity_count;
    DrawingProgramRasterSample *composited_samples;
    uint32_t composited_capacity;
} DrawingProgramVisualSurfaceCacheEntry;

typedef struct DrawingProgramVisualSurfaceCacheState {
    DrawingProgramVisualSurfaceCacheEntry *entries;
    uint32_t count;
    uint32_t capacity;
} DrawingProgramVisualSurfaceCacheState;

static DrawingProgramVisualSurfaceCacheState g_surface_cache = {0};

static uint64_t surface_cache_perf_counter_frequency(void) {
    static uint64_t s_frequency = 0u;
    if (s_frequency == 0u) {
        s_frequency = (uint64_t)SDL_GetPerformanceFrequency();
        if (s_frequency == 0u) {
            s_frequency = 1u;
        }
    }
    return s_frequency;
}

static uint32_t surface_cache_elapsed_us(Uint64 begin_counter, Uint64 end_counter) {
    uint64_t frequency = surface_cache_perf_counter_frequency();
    uint64_t delta = 0u;
    if (end_counter <= begin_counter) {
        return 0u;
    }
    delta = (uint64_t)(end_counter - begin_counter);
    return (uint32_t)((delta * 1000000u) / frequency);
}

static uint32_t surface_cache_target_dimension(uint32_t source_dimension, uint32_t zoom_bucket_percent) {
    uint64_t scaled = 0u;
    if (source_dimension == 0u) {
        return 0u;
    }
    if (zoom_bucket_percent == 0u || zoom_bucket_percent >= 100u) {
        return source_dimension;
    }
    scaled = ((uint64_t)source_dimension * (uint64_t)zoom_bucket_percent + 50u) / 100u;
    if (scaled == 0u) {
        scaled = 1u;
    }
    if (scaled > source_dimension) {
        scaled = source_dimension;
    }
    return (uint32_t)scaled;
}

static void surface_cache_entry_reset(DrawingProgramVisualSurfaceCacheEntry *entry) {
    if (!entry) {
        return;
    }
    if (entry->texture) {
        SDL_DestroyTexture(entry->texture);
        entry->texture = 0;
    }
    if (entry->pixel_format) {
        SDL_FreeFormat(entry->pixel_format);
        entry->pixel_format = 0;
    }
    free(entry->composited_samples);
    memset(entry, 0, sizeof(*entry));
}

static int surface_cache_ensure_capacity(uint32_t required_capacity) {
    DrawingProgramVisualSurfaceCacheEntry *next_entries = 0;
    uint32_t next_capacity = 0u;
    if (required_capacity <= g_surface_cache.capacity) {
        return 1;
    }
    next_capacity = g_surface_cache.capacity > 0u ? g_surface_cache.capacity : 4u;
    while (next_capacity < required_capacity) {
        next_capacity *= 2u;
    }
    next_entries =
        (DrawingProgramVisualSurfaceCacheEntry *)realloc(g_surface_cache.entries,
                                                         (size_t)next_capacity * sizeof(*next_entries));
    if (!next_entries) {
        return 0;
    }
    memset(next_entries + g_surface_cache.capacity,
           0,
           (size_t)(next_capacity - g_surface_cache.capacity) * sizeof(*next_entries));
    g_surface_cache.entries = next_entries;
    g_surface_cache.capacity = next_capacity;
    return 1;
}

static DrawingProgramVisualSurfaceCacheEntry *surface_cache_find_entry(uint64_t project_epoch, uint32_t surface_id) {
    uint32_t i;
    for (i = 0u; i < g_surface_cache.count; ++i) {
        DrawingProgramVisualSurfaceCacheEntry *entry = &g_surface_cache.entries[i];
        if (entry->project_epoch == project_epoch && entry->surface_id == surface_id) {
            return entry;
        }
    }
    return 0;
}

static DrawingProgramVisualSurfaceCacheEntry *surface_cache_ensure_entry(uint64_t project_epoch, uint32_t surface_id) {
    DrawingProgramVisualSurfaceCacheEntry *entry = surface_cache_find_entry(project_epoch, surface_id);
    if (entry) {
        return entry;
    }
    if (!surface_cache_ensure_capacity(g_surface_cache.count + 1u)) {
        return 0;
    }
    entry = &g_surface_cache.entries[g_surface_cache.count++];
    memset(entry, 0, sizeof(*entry));
    entry->project_epoch = project_epoch;
    entry->surface_id = surface_id;
    return entry;
}

static int surface_cache_has_live_texture(const DrawingProgramVisualSurfaceCacheEntry *entry) {
    return entry && entry->texture && entry->pixel_format && entry->width > 0u && entry->height > 0u;
}

static int surface_cache_signature_matches(const DrawingProgramVisualSurfaceCacheEntry *entry,
                                           const DrawingProgramVisualSurfaceCacheRequest *request) {
    if (!entry || !request || !entry->has_sync_signature) {
        return 0;
    }
    if (entry->zoom_bucket_percent != request->zoom_bucket_percent) {
        return 0;
    }
    if (entry->layer_opacity_revision != request->layer_opacity_revision) {
        return 0;
    }
    return entry->content_revision == request->content_revision;
}

static int surface_cache_pending_signature_matches(const DrawingProgramVisualSurfaceCacheEntry *entry,
                                                   const DrawingProgramVisualSurfaceCacheRequest *request) {
    if (!entry || !request || !entry->pending_sync) {
        return 0;
    }
    if (entry->pending_zoom_bucket_percent != request->zoom_bucket_percent) {
        return 0;
    }
    if (entry->pending_layer_opacity_revision != request->layer_opacity_revision) {
        return 0;
    }
    if (entry->pending_active_surface != (request->active_surface ? 1u : 0u)) {
        return 0;
    }
    return entry->pending_content_revision == request->content_revision;
}

static void surface_cache_queue_request(DrawingProgramVisualSurfaceCacheEntry *entry,
                                        SDL_Renderer *renderer,
                                        const DrawingProgramVisualSurfaceCacheRequest *request,
                                        const DrawingProgramDocument *document,
                                        const DrawingProgramLayerRasterStore *layer_rasters,
                                        const uint8_t *layer_opacity_percent,
                                        uint32_t layer_opacity_count) {
    uint32_t copy_count = 0u;
    if (!entry || !renderer || !request || !document || !layer_rasters || !layer_opacity_percent) {
        return;
    }
    entry->renderer_owner = renderer;
    entry->pending_document = document;
    entry->pending_layer_rasters = layer_rasters;
    entry->pending_content_revision = request->content_revision;
    entry->pending_layer_opacity_revision = request->layer_opacity_revision;
    entry->pending_zoom_bucket_percent = request->zoom_bucket_percent;
    entry->pending_active_surface = request->active_surface ? 1u : 0u;
    copy_count = layer_opacity_count;
    if (copy_count > DRAWING_PROGRAM_MAX_LAYERS) {
        copy_count = DRAWING_PROGRAM_MAX_LAYERS;
    }
    if (copy_count > 0u) {
        memcpy(entry->pending_layer_opacity, layer_opacity_percent, (size_t)copy_count * sizeof(uint8_t));
    }
    entry->pending_layer_opacity_count = copy_count;
    entry->pending_sync = 1u;
}

static int surface_cache_sync_pixels(DrawingProgramVisualSurfaceCacheEntry *entry,
                                     SDL_Texture *target_texture,
                                     SDL_PixelFormat *target_pixel_format,
                                     uint32_t target_width,
                                     uint32_t target_height,
                                     const DrawingProgramDocument *document,
                                     const DrawingProgramLayerRasterStore *layer_rasters,
                                     const uint8_t *layer_opacity_percent,
                                     uint32_t layer_opacity_count,
                                     uint32_t *out_compose_us,
                                     uint32_t *out_upload_us) {
    void *pixels = 0;
    int pitch = 0;
    uint32_t x;
    uint32_t y;
    CoreResult compose_result;
    const DrawingProgramRasterSample *source_samples = 0;
    Uint64 compose_begin = 0u;
    Uint64 compose_end = 0u;
    Uint64 upload_begin = 0u;
    Uint64 upload_end = 0u;
    if (!entry || !target_texture || !target_pixel_format || !document || !layer_rasters || !layer_opacity_percent ||
        layer_opacity_count == 0u || target_width == 0u || target_height == 0u) {
        return 0;
    }
    if (entry->composited_capacity < document->raster_sample_count) {
        DrawingProgramRasterSample *next_samples =
            (DrawingProgramRasterSample *)realloc(entry->composited_samples,
                                                  (size_t)document->raster_sample_count * sizeof(*next_samples));
        if (!next_samples) {
            return 0;
        }
        entry->composited_samples = next_samples;
        entry->composited_capacity = document->raster_sample_count;
    }
    source_samples = document->raster_samples;
    compose_begin = SDL_GetPerformanceCounter();
    compose_result = drawing_program_render_compose_visible_samples_with_layer_opacity(document,
                                                                                        layer_rasters,
                                                                                        layer_opacity_percent,
                                                                                        layer_opacity_count,
                                                                                        entry->composited_samples,
                                                                                        entry->composited_capacity);
    compose_end = SDL_GetPerformanceCounter();
    if (compose_result.code == CORE_OK) {
        source_samples = entry->composited_samples;
    }
    upload_begin = SDL_GetPerformanceCounter();
    if (SDL_LockTexture(target_texture, 0, &pixels, &pitch) != 0) {
        return 0;
    }
    for (y = 0u; y < target_height; ++y) {
        uint32_t *row = (uint32_t *)((uint8_t *)pixels + ((size_t)y * (size_t)pitch));
        uint32_t source_y = (uint32_t)(((uint64_t)y * (uint64_t)document->raster_height) / (uint64_t)target_height);
        size_t row_offset;
        if (source_y >= document->raster_height) {
            source_y = document->raster_height - 1u;
        }
        row_offset = (size_t)source_y * (size_t)document->raster_width;
        for (x = 0u; x < target_width; ++x) {
            uint32_t source_x = (uint32_t)(((uint64_t)x * (uint64_t)document->raster_width) / (uint64_t)target_width);
            DrawingProgramRasterSample sample;
            uint8_t r = 0u;
            uint8_t g = 0u;
            uint8_t b = 0u;
            uint8_t a = 0u;
            if (source_x >= document->raster_width) {
                source_x = document->raster_width - 1u;
            }
            sample = source_samples[row_offset + source_x];
            drawing_program_color_rgba_from_sample(sample, &r, &g, &b, &a);
            row[x] = SDL_MapRGBA(target_pixel_format, r, g, b, a);
        }
    }
    SDL_UnlockTexture(target_texture);
    upload_end = SDL_GetPerformanceCounter();
    if (out_compose_us) {
        *out_compose_us = surface_cache_elapsed_us(compose_begin, compose_end);
    }
    if (out_upload_us) {
        *out_upload_us = surface_cache_elapsed_us(upload_begin, upload_end);
    }
    return 1;
}

static int surface_cache_rebuild_entry(DrawingProgramVisualSurfaceCacheEntry *entry,
                                       SDL_Renderer *renderer,
                                       const DrawingProgramVisualSurfaceCacheRequest *request,
                                       const DrawingProgramDocument *document,
                                       const DrawingProgramLayerRasterStore *layer_rasters,
                                       const uint8_t *layer_opacity_percent,
                                       uint32_t layer_opacity_count,
                                       DrawingProgramVisualSurfaceCacheTelemetry *out_telemetry) {
    SDL_Texture *target_texture = 0;
    SDL_PixelFormat *target_pixel_format = 0;
    uint32_t target_width = 0u;
    uint32_t target_height = 0u;
    uint32_t compose_us = 0u;
    uint32_t upload_us = 0u;
    int reuse_live_texture = 0;
    Uint64 rebuild_begin = 0u;
    Uint64 rebuild_end = 0u;
    if (!entry || !renderer || !request || !document || !layer_rasters || !layer_opacity_percent) {
        return 0;
    }
    rebuild_begin = SDL_GetPerformanceCounter();
    target_width = surface_cache_target_dimension(document->raster_width, request->zoom_bucket_percent);
    target_height = surface_cache_target_dimension(document->raster_height, request->zoom_bucket_percent);
    reuse_live_texture = (entry->renderer_owner == renderer &&
                          entry->width == target_width &&
                          entry->height == target_height &&
                          surface_cache_has_live_texture(entry));
    if (reuse_live_texture) {
        target_texture = entry->texture;
        target_pixel_format = entry->pixel_format;
    } else {
        target_texture = SDL_CreateTexture(renderer,
                                           SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           (int)target_width,
                                           (int)target_height);
        if (!target_texture) {
            return 0;
        }
        target_pixel_format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
        if (!target_pixel_format) {
            SDL_DestroyTexture(target_texture);
            return 0;
        }
    }
    (void)SDL_SetTextureBlendMode(target_texture, SDL_BLENDMODE_NONE);
    if (!surface_cache_sync_pixels(entry,
                                   target_texture,
                                   target_pixel_format,
                                   target_width,
                                   target_height,
                                   document,
                                   layer_rasters,
                                   layer_opacity_percent,
                                   layer_opacity_count,
                                   &compose_us,
                                   &upload_us)) {
        if (!reuse_live_texture) {
            SDL_FreeFormat(target_pixel_format);
            SDL_DestroyTexture(target_texture);
        }
        return 0;
    }
    if (!reuse_live_texture) {
        if (entry->texture) {
            SDL_DestroyTexture(entry->texture);
        }
        if (entry->pixel_format) {
            SDL_FreeFormat(entry->pixel_format);
        }
        entry->texture = target_texture;
        entry->pixel_format = target_pixel_format;
        entry->renderer_owner = renderer;
        entry->width = target_width;
        entry->height = target_height;
    }
    entry->project_epoch = request->project_epoch;
    entry->surface_id = request->surface_id;
    entry->content_revision = request->content_revision;
    entry->layer_opacity_revision = request->layer_opacity_revision;
    entry->zoom_bucket_percent = request->zoom_bucket_percent;
    entry->has_sync_signature = 1u;
    entry->pending_sync = 0u;
    entry->pending_active_surface = 0u;
    entry->pending_document = 0;
    entry->pending_layer_rasters = 0;
    entry->pending_layer_opacity_count = 0u;
    rebuild_end = SDL_GetPerformanceCounter();
    if (out_telemetry) {
        out_telemetry->cache_rebuilt = 1u;
        out_telemetry->cache_compose_us = compose_us;
        out_telemetry->cache_upload_us = upload_us;
        out_telemetry->cache_rebuild_us = surface_cache_elapsed_us(rebuild_begin, rebuild_end);
    }
    return 1;
}

SDL_Texture *drawing_program_visual_surface_cache_sync(
    SDL_Renderer *renderer,
    const DrawingProgramVisualSurfaceCacheRequest *request,
    const DrawingProgramDocument *document,
    const DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    DrawingProgramVisualSurfaceCacheTelemetry *out_telemetry) {
    DrawingProgramVisualSurfaceCacheEntry *entry = 0;
    if (out_telemetry) {
        memset(out_telemetry, 0, sizeof(*out_telemetry));
    }
    if (!renderer || !request || !document || !layer_rasters || !layer_opacity_percent || layer_opacity_count == 0u ||
        request->surface_id == 0u || request->project_epoch == 0u || document->raster_sample_count == 0u ||
        request->zoom_bucket_percent == 0u) {
        return 0;
    }
    entry = surface_cache_ensure_entry(request->project_epoch, request->surface_id);
    if (!entry) {
        return 0;
    }
    if (surface_cache_signature_matches(entry, request)) {
        if (out_telemetry) {
            out_telemetry->cache_hit = 1u;
            out_telemetry->cache_copy_ready = 1u;
        }
        return entry->texture;
    }
    if (out_telemetry) {
        out_telemetry->cache_miss = 1u;
    }
    if (!entry->has_sync_signature) {
        if (!surface_cache_rebuild_entry(entry,
                                         renderer,
                                         request,
                                         document,
                                         layer_rasters,
                                         layer_opacity_percent,
                                         layer_opacity_count,
                                         out_telemetry)) {
            if (out_telemetry) {
                out_telemetry->cache_unavailable = 1u;
            }
            return 0;
        }
        if (out_telemetry) {
            out_telemetry->cache_copy_ready = 1u;
        }
        return entry->texture;
    }
    if (entry->pending_sync) {
        if (!surface_cache_pending_signature_matches(entry, request)) {
            surface_cache_queue_request(entry,
                                        renderer,
                                        request,
                                        document,
                                        layer_rasters,
                                        layer_opacity_percent,
                                        layer_opacity_count);
        }
        if (out_telemetry) {
            out_telemetry->cache_deferred = 1u;
            if (surface_cache_has_live_texture(entry)) {
                out_telemetry->cache_copy_ready = 1u;
            } else {
                out_telemetry->cache_unavailable = 1u;
            }
        }
        return surface_cache_has_live_texture(entry) ? entry->texture : 0;
    }
    surface_cache_queue_request(entry,
                                renderer,
                                request,
                                document,
                                layer_rasters,
                                layer_opacity_percent,
                                layer_opacity_count);
    if (out_telemetry) {
        out_telemetry->cache_deferred = 1u;
        if (surface_cache_has_live_texture(entry)) {
            out_telemetry->cache_copy_ready = 1u;
        } else {
            out_telemetry->cache_unavailable = 1u;
        }
    }
    return surface_cache_has_live_texture(entry) ? entry->texture : 0;
}

void drawing_program_visual_surface_cache_prune_for_project(const DrawingProgramTextureProject *project) {
    uint32_t read_index = 0u;
    uint32_t write_index = 0u;
    if (!project) {
        return;
    }
    for (read_index = 0u; read_index < g_surface_cache.count; ++read_index) {
        DrawingProgramVisualSurfaceCacheEntry *entry = &g_surface_cache.entries[read_index];
        uint32_t surface_index = 0u;
        int keep = 0;
        if (entry->project_epoch == project->runtime_cache_epoch) {
            for (surface_index = 0u; surface_index < project->surface_count; ++surface_index) {
                const DrawingProgramTextureSurface *surface =
                    drawing_program_texture_project_surface_at(project, surface_index);
                if (surface && surface->surface_id == entry->surface_id) {
                    keep = 1;
                    break;
                }
            }
        }
        if (!keep) {
            surface_cache_entry_reset(entry);
            continue;
        }
        if (write_index != read_index) {
            g_surface_cache.entries[write_index] = *entry;
            memset(entry, 0, sizeof(*entry));
        }
        write_index += 1u;
    }
    g_surface_cache.count = write_index;
}

uint32_t drawing_program_visual_surface_cache_entry_count(void) {
    return g_surface_cache.count;
}

uint32_t drawing_program_visual_surface_cache_pending_count(void) {
    uint32_t i;
    uint32_t pending_count = 0u;
    for (i = 0u; i < g_surface_cache.count; ++i) {
        if (g_surface_cache.entries[i].pending_sync) {
            pending_count += 1u;
        }
    }
    return pending_count;
}

uint32_t drawing_program_visual_surface_cache_process_pending_step(uint8_t active_only,
                                                                  DrawingProgramVisualSurfaceCacheTelemetry *out_telemetry) {
    uint32_t i;
    uint32_t fallback_index = UINT32_MAX;
    uint32_t target_index = UINT32_MAX;
    if (out_telemetry) {
        memset(out_telemetry, 0, sizeof(*out_telemetry));
    }
    for (i = 0u; i < g_surface_cache.count; ++i) {
        DrawingProgramVisualSurfaceCacheEntry *entry = &g_surface_cache.entries[i];
        if (!entry->pending_sync || !entry->pending_document || !entry->pending_layer_rasters || !entry->renderer_owner) {
            continue;
        }
        if (entry->pending_active_surface) {
            target_index = i;
            break;
        }
        if (!active_only && fallback_index == UINT32_MAX) {
            fallback_index = i;
        }
    }
    if (target_index == UINT32_MAX) {
        target_index = fallback_index;
    }
    if (target_index == UINT32_MAX) {
        return 0u;
    }
    {
        DrawingProgramVisualSurfaceCacheEntry *entry = &g_surface_cache.entries[target_index];
        DrawingProgramVisualSurfaceCacheRequest request;
        memset(&request, 0, sizeof(request));
        request.project_epoch = entry->project_epoch;
        request.content_revision = entry->pending_content_revision;
        request.layer_opacity_revision = entry->pending_layer_opacity_revision;
        request.surface_id = entry->surface_id;
        request.zoom_bucket_percent = entry->pending_zoom_bucket_percent;
        request.active_surface = entry->pending_active_surface;
        if (!surface_cache_rebuild_entry(entry,
                                         entry->renderer_owner,
                                         &request,
                                         entry->pending_document,
                                         entry->pending_layer_rasters,
                                         entry->pending_layer_opacity,
                                         entry->pending_layer_opacity_count,
                                         out_telemetry)) {
            entry->pending_sync = 0u;
            return 0u;
        }
    }
    return 1u;
}

void drawing_program_visual_surface_cache_shutdown(void) {
    uint32_t i;
    for (i = 0u; i < g_surface_cache.count; ++i) {
        surface_cache_entry_reset(&g_surface_cache.entries[i]);
    }
    free(g_surface_cache.entries);
    memset(&g_surface_cache, 0, sizeof(g_surface_cache));
}
