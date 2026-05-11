#ifndef DRAWING_PROGRAM_VISUAL_SURFACE_CACHE_H
#define DRAWING_PROGRAM_VISUAL_SURFACE_CACHE_H

#include <stdint.h>

#include <SDL2/SDL.h>

struct DrawingProgramDocument;
struct DrawingProgramLayerRasterStore;
struct DrawingProgramTextureProject;

typedef struct DrawingProgramVisualSurfaceCacheRequest {
    uint64_t project_epoch;
    uint64_t content_revision;
    uint64_t layer_opacity_revision;
    uint32_t surface_id;
    uint32_t zoom_bucket_percent;
    uint8_t active_surface;
} DrawingProgramVisualSurfaceCacheRequest;

typedef struct DrawingProgramVisualSurfaceCacheTelemetry {
    uint8_t cache_hit;
    uint8_t cache_miss;
    uint8_t cache_deferred;
    uint8_t cache_rebuilt;
    uint8_t cache_copy_ready;
    uint8_t cache_unavailable;
    uint32_t cache_compose_us;
    uint32_t cache_upload_us;
    uint32_t cache_rebuild_us;
} DrawingProgramVisualSurfaceCacheTelemetry;

SDL_Texture *drawing_program_visual_surface_cache_sync(
    SDL_Renderer *renderer,
    const DrawingProgramVisualSurfaceCacheRequest *request,
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    DrawingProgramVisualSurfaceCacheTelemetry *out_telemetry);
void drawing_program_visual_surface_cache_prune_for_project(
    const struct DrawingProgramTextureProject *project);
uint32_t drawing_program_visual_surface_cache_entry_count(void);
uint32_t drawing_program_visual_surface_cache_pending_count(void);
uint32_t drawing_program_visual_surface_cache_process_pending_step(uint8_t active_only,
                                                                  DrawingProgramVisualSurfaceCacheTelemetry *out_telemetry);
void drawing_program_visual_surface_cache_shutdown(void);

#endif
