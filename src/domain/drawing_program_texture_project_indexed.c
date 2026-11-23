#include "drawing_program/drawing_program_texture_project.h"

#include <string.h>

static bool texture_project_indexed_surface_matches(
    const DrawingProgramTextureProject *project,
    const DrawingProgramIndexedTilesetProfile *profile) {
    const DrawingProgramTextureSurface *surface =
        drawing_program_texture_project_surface_at(project, 0u);
    return surface && surface->storage &&
           surface->storage->document.sample_density == 1u &&
           surface->storage->document.logical_width == profile->atlas_width &&
           surface->storage->document.logical_height == profile->atlas_height;
}

CoreResult drawing_program_texture_project_validate_indexed_atlas(
    const DrawingProgramTextureProject *project) {
    CoreResult result;
    if (!project || project->profile_kind != DRAWING_PROGRAM_TEXTURE_PROJECT_PROFILE_INDEXED_ATLAS_V1 ||
        project->surface_count != 1u) {
        return (CoreResult){CORE_ERR_INVALID_ARG, "invalid indexed texture project profile"};
    }
    if (!texture_project_indexed_surface_matches(project, &project->indexed_profile)) {
        return (CoreResult){CORE_ERR_INVALID_ARG, "indexed texture project surface mismatch"};
    }
    result = drawing_program_indexed_tileset_profile_validate(&project->indexed_profile);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_indexed_layer_raster_validate(&project->indexed_raster);
    if (result.code != CORE_OK) {
        return result;
    }
    if (project->indexed_raster.width != project->indexed_profile.atlas_width ||
        project->indexed_raster.height != project->indexed_profile.atlas_height ||
        project->indexed_raster.slot_count != project->indexed_profile.slot_count) {
        return (CoreResult){CORE_ERR_INVALID_ARG, "indexed texture project raster/profile mismatch"};
    }
    if (project->indexed_cells.count > 0u) {
        result = drawing_program_indexed_cell_table_validate(
            &project->indexed_cells,
            project->indexed_profile.atlas_width,
            project->indexed_profile.atlas_height,
            project->indexed_profile.logical_cell_width,
            project->indexed_profile.logical_cell_height);
        if (result.code != CORE_OK) return result;
        if (project->indexed_tile_canvases.count > 0u) {
            DrawingProgramIndexedLayerRaster composed;
            result = drawing_program_indexed_tile_canvas_table_validate(
                &project->indexed_tile_canvases, &project->indexed_cells, &project->indexed_profile);
            if (result.code != CORE_OK) return result;
            memset(&composed, 0, sizeof(composed));
            result = drawing_program_indexed_tile_canvas_table_compose_atlas(
                &project->indexed_tile_canvases, &project->indexed_cells,
                &project->indexed_profile, &composed);
            if (result.code != CORE_OK || composed.index_count != project->indexed_raster.index_count ||
                memcmp(composed.indices, project->indexed_raster.indices, composed.index_count) != 0) {
                drawing_program_indexed_layer_raster_dispose(&composed);
                return (CoreResult){CORE_ERR_INVALID_ARG, "indexed tile canvases and composed atlas diverged"};
            }
            drawing_program_indexed_layer_raster_dispose(&composed);
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_texture_project_enable_indexed_atlas(
    DrawingProgramTextureProject *project,
    const DrawingProgramIndexedTilesetProfile *profile,
    uint8_t fill_index) {
    DrawingProgramIndexedLayerRaster next_raster;
    CoreResult result;
    if (!project || !profile || project->surface_count != 1u) {
        return (CoreResult){CORE_ERR_INVALID_ARG, "invalid indexed texture project enable request"};
    }
    result = drawing_program_indexed_tileset_profile_validate(profile);
    if (result.code != CORE_OK) {
        return result;
    }
    if (!texture_project_indexed_surface_matches(project, profile)) {
        return (CoreResult){CORE_ERR_INVALID_ARG, "indexed texture project requires one matching density-one surface"};
    }
    memset(&next_raster, 0, sizeof(next_raster));
    result = drawing_program_indexed_layer_raster_init(&next_raster,
                                                       profile->atlas_width,
                                                       profile->atlas_height,
                                                       profile->slot_count,
                                                       fill_index);
    if (result.code != CORE_OK) {
        return result;
    }
    drawing_program_indexed_history_clear(&project->indexed_history);
    drawing_program_indexed_cell_history_clear(&project->indexed_cell_history);
    drawing_program_indexed_cell_table_clear(&project->indexed_cells);
    drawing_program_indexed_tile_canvas_table_clear(&project->indexed_tile_canvases);
    drawing_program_indexed_layer_raster_dispose(&project->indexed_raster);
    project->indexed_profile = *profile;
    project->indexed_raster = next_raster;
    project->profile_kind = DRAWING_PROGRAM_TEXTURE_PROJECT_PROFILE_INDEXED_ATLAS_V1;
    return drawing_program_texture_project_validate_indexed_atlas(project);
}
