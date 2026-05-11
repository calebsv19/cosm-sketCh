#include "drawing_program/drawing_program_texture_workspace.h"

#include <math.h>

#include "core_viewport2d.h"
#include "drawing_program/drawing_program_texture_project.h"
#include "drawing_program/drawing_program_viewport.h"

typedef struct DrawingProgramTextureWorkspaceRect {
    float x;
    float y;
    float width;
    float height;
} DrawingProgramTextureWorkspaceRect;

typedef struct DrawingProgramTextureWorkspaceLayout {
    uint32_t columns;
    uint32_t rows;
    float atlas_width;
    float atlas_height;
    float origin_offset_x;
    float origin_offset_y;
    float cell_width;
    float cell_height;
    float gutter;
    float padding;
    uint8_t semantic_layout_active;
} DrawingProgramTextureWorkspaceLayout;

typedef struct DrawingProgramTextureWorkspaceSemanticMap {
    int32_t plane_front_index;
    int32_t prism_front_index;
    int32_t prism_back_index;
    int32_t prism_left_index;
    int32_t prism_right_index;
    int32_t prism_top_index;
    int32_t prism_bottom_index;
    uint8_t valid;
} DrawingProgramTextureWorkspaceSemanticMap;

static int texture_workspace_frame_valid(SDL_Rect pane_rect) {
    return pane_rect.w > 0 && pane_rect.h > 0;
}

static int texture_workspace_surface_valid(const DrawingProgramTextureSurface *surface) {
    return surface && surface->storage && surface->storage->document.raster_width > 0u &&
           surface->storage->document.raster_height > 0u;
}

static float texture_workspace_content_width(const DrawingProgramDocument *document) {
    return (float)document->raster_width * drawing_program_viewport_base_pixel_size();
}

static float texture_workspace_content_height(const DrawingProgramDocument *document) {
    return (float)document->raster_height * drawing_program_viewport_base_pixel_size();
}

static float texture_workspace_surface_width(const DrawingProgramTextureSurface *surface) {
    return texture_workspace_content_width(&surface->storage->document);
}

static float texture_workspace_surface_height(const DrawingProgramTextureSurface *surface) {
    return texture_workspace_content_height(&surface->storage->document);
}

static float texture_workspace_frame_padding(SDL_Rect pane_rect) {
    float min_dim = (float)(pane_rect.w < pane_rect.h ? pane_rect.w : pane_rect.h);
    float padding = min_dim * 0.05f;
    if (!isfinite(padding) || padding < 0.0f) {
        padding = 0.0f;
    }
    if (padding < 16.0f) {
        padding = 16.0f;
    }
    if (padding > 48.0f) {
        padding = 48.0f;
    }
    if ((padding * 2.0f) >= (float)pane_rect.w || (padding * 2.0f) >= (float)pane_rect.h) {
        padding = 0.0f;
    }
    return padding;
}

static int texture_workspace_generic_index_valid(const DrawingProgramTextureProject *project,
                                                 const DrawingProgramTextureWorkspaceSemanticMap *semantic_map,
                                                 uint32_t surface_index) {
    if (!project || surface_index >= project->surface_count) {
        return 0;
    }
    if (!semantic_map || !semantic_map->valid) {
        return 1;
    }
    if ((int32_t)surface_index == semantic_map->plane_front_index ||
        (int32_t)surface_index == semantic_map->prism_front_index ||
        (int32_t)surface_index == semantic_map->prism_back_index ||
        (int32_t)surface_index == semantic_map->prism_left_index ||
        (int32_t)surface_index == semantic_map->prism_right_index ||
        (int32_t)surface_index == semantic_map->prism_top_index ||
        (int32_t)surface_index == semantic_map->prism_bottom_index) {
        return 0;
    }
    return 1;
}

static void texture_workspace_semantic_map_clear(DrawingProgramTextureWorkspaceSemanticMap *map) {
    if (!map) {
        return;
    }
    map->plane_front_index = -1;
    map->prism_front_index = -1;
    map->prism_back_index = -1;
    map->prism_left_index = -1;
    map->prism_right_index = -1;
    map->prism_top_index = -1;
    map->prism_bottom_index = -1;
    map->valid = 0u;
}

static int texture_workspace_surface_eligible_for_semantic_layout(const DrawingProgramTextureProject *project,
                                                                  const DrawingProgramTextureSurface *surface) {
    if (!project || !surface || !texture_workspace_surface_valid(surface)) {
        return 0;
    }
    if (surface->user_created) {
        return 0;
    }
    if (surface->semantic.layout_kind != project->net_layout_kind) {
        return 0;
    }
    if (surface->semantic.net_slot == DRAWING_PROGRAM_TEXTURE_NET_SLOT_UNSPECIFIED) {
        return 0;
    }
    return 1;
}

static int texture_workspace_try_build_semantic_map(const DrawingProgramTextureProject *project,
                                                    DrawingProgramTextureWorkspaceSemanticMap *out_map) {
    uint32_t i;
    DrawingProgramTextureWorkspaceSemanticMap map;
    if (!project || !out_map || project->surface_count == 0u) {
        return 0;
    }
    texture_workspace_semantic_map_clear(&map);
    switch (project->net_layout_kind) {
        case DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_PLANE_SINGLE:
            for (i = 0u; i < project->surface_count; ++i) {
                const DrawingProgramTextureSurface *surface = drawing_program_texture_project_surface_at(project, i);
                if (!texture_workspace_surface_eligible_for_semantic_layout(project, surface) ||
                    surface->semantic.net_slot != DRAWING_PROGRAM_TEXTURE_NET_SLOT_FRONT) {
                    continue;
                }
                if (map.plane_front_index >= 0) {
                    texture_workspace_semantic_map_clear(out_map);
                    return 0;
                }
                map.plane_front_index = (int32_t)i;
            }
            map.valid = (map.plane_front_index >= 0) ? 1u : 0u;
            break;
        case DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_RECT_PRISM_CROSS:
            for (i = 0u; i < project->surface_count; ++i) {
                const DrawingProgramTextureSurface *surface = drawing_program_texture_project_surface_at(project, i);
                if (!texture_workspace_surface_eligible_for_semantic_layout(project, surface)) {
                    continue;
                }
                switch (surface->semantic.net_slot) {
                    case DRAWING_PROGRAM_TEXTURE_NET_SLOT_FRONT:
                        if (map.prism_front_index >= 0) {
                            texture_workspace_semantic_map_clear(out_map);
                            return 0;
                        }
                        map.prism_front_index = (int32_t)i;
                        break;
                    case DRAWING_PROGRAM_TEXTURE_NET_SLOT_BACK:
                        if (map.prism_back_index >= 0) {
                            texture_workspace_semantic_map_clear(out_map);
                            return 0;
                        }
                        map.prism_back_index = (int32_t)i;
                        break;
                    case DRAWING_PROGRAM_TEXTURE_NET_SLOT_LEFT:
                        if (map.prism_left_index >= 0) {
                            texture_workspace_semantic_map_clear(out_map);
                            return 0;
                        }
                        map.prism_left_index = (int32_t)i;
                        break;
                    case DRAWING_PROGRAM_TEXTURE_NET_SLOT_RIGHT:
                        if (map.prism_right_index >= 0) {
                            texture_workspace_semantic_map_clear(out_map);
                            return 0;
                        }
                        map.prism_right_index = (int32_t)i;
                        break;
                    case DRAWING_PROGRAM_TEXTURE_NET_SLOT_TOP:
                        if (map.prism_top_index >= 0) {
                            texture_workspace_semantic_map_clear(out_map);
                            return 0;
                        }
                        map.prism_top_index = (int32_t)i;
                        break;
                    case DRAWING_PROGRAM_TEXTURE_NET_SLOT_BOTTOM:
                        if (map.prism_bottom_index >= 0) {
                            texture_workspace_semantic_map_clear(out_map);
                            return 0;
                        }
                        map.prism_bottom_index = (int32_t)i;
                        break;
                    default: break;
                }
            }
            map.valid = (map.prism_front_index >= 0 && map.prism_back_index >= 0 && map.prism_left_index >= 0 &&
                         map.prism_right_index >= 0 && map.prism_top_index >= 0 && map.prism_bottom_index >= 0)
                            ? 1u
                            : 0u;
            break;
        default: break;
    }
    *out_map = map;
    return map.valid != 0u;
}

static int texture_workspace_build_generic_layout_filtered(const DrawingProgramTextureProject *project,
                                                           const DrawingProgramTextureWorkspaceSemanticMap *semantic_map,
                                                           DrawingProgramTextureWorkspaceLayout *out_layout,
                                                           uint32_t *out_surface_count) {
    uint32_t i;
    uint32_t count = 0u;
    float max_width = 0.0f;
    float max_height = 0.0f;
    float base_pixel = drawing_program_viewport_base_pixel_size();
    if (!project || !out_layout || project->surface_count == 0u) {
        return 0;
    }
    for (i = 0u; i < project->surface_count; ++i) {
        const DrawingProgramTextureSurface *surface = drawing_program_texture_project_surface_at(project, i);
        float width;
        float height;
        if (!texture_workspace_generic_index_valid(project, semantic_map, i)) {
            continue;
        }
        if (!texture_workspace_surface_valid(surface)) {
            return 0;
        }
        width = texture_workspace_surface_width(surface);
        height = texture_workspace_surface_height(surface);
        if (width > max_width) {
            max_width = width;
        }
        if (height > max_height) {
            max_height = height;
        }
        count += 1u;
    }
    if (out_surface_count) {
        *out_surface_count = count;
    }
    if (count == 0u) {
        memset(out_layout, 0, sizeof(*out_layout));
        out_layout->gutter = base_pixel * 24.0f;
        if (out_layout->gutter < 18.0f) {
            out_layout->gutter = 18.0f;
        }
        out_layout->padding = out_layout->gutter;
        return 1;
    }
    if (max_width <= 0.0f || max_height <= 0.0f) {
        return 0;
    }
    out_layout->semantic_layout_active = 0u;
    out_layout->columns = (uint32_t)ceil(sqrt((double)count));
    if (out_layout->columns == 0u) {
        out_layout->columns = 1u;
    }
    out_layout->rows = (count + out_layout->columns - 1u) / out_layout->columns;
    out_layout->cell_width = max_width;
    out_layout->cell_height = max_height;
    out_layout->gutter = base_pixel * 24.0f;
    if (out_layout->gutter < 18.0f) {
        out_layout->gutter = 18.0f;
    }
    out_layout->padding = out_layout->gutter;
    out_layout->atlas_width =
        (out_layout->padding * 2.0f) +
        ((float)out_layout->columns * out_layout->cell_width) +
        ((float)(out_layout->columns - 1u) * out_layout->gutter);
    out_layout->atlas_height =
        (out_layout->padding * 2.0f) +
        ((float)out_layout->rows * out_layout->cell_height) +
        ((float)(out_layout->rows - 1u) * out_layout->gutter);
    return 1;
}

static int texture_workspace_generic_surface_rect_filtered(const DrawingProgramTextureProject *project,
                                                           const DrawingProgramTextureWorkspaceSemanticMap *semantic_map,
                                                           uint32_t surface_index,
                                                           float origin_x,
                                                           float origin_y,
                                                           DrawingProgramTextureWorkspaceRect *out_rect,
                                                           DrawingProgramTextureWorkspaceLayout *out_layout) {
    DrawingProgramTextureWorkspaceLayout layout;
    const DrawingProgramTextureSurface *surface;
    const DrawingProgramDocument *document;
    uint32_t filtered_count = 0u;
    uint32_t filtered_index = 0u;
    uint32_t i;
    uint32_t row;
    uint32_t column;
    float width;
    float height;
    float cell_x;
    float cell_y;
    if (!project || !out_rect || surface_index >= project->surface_count ||
        !texture_workspace_generic_index_valid(project, semantic_map, surface_index) ||
        !texture_workspace_build_generic_layout_filtered(project, semantic_map, &layout, &filtered_count) ||
        filtered_count == 0u) {
        return 0;
    }
    for (i = 0u; i < project->surface_count; ++i) {
        if (!texture_workspace_generic_index_valid(project, semantic_map, i)) {
            continue;
        }
        if (i == surface_index) {
            break;
        }
        filtered_index += 1u;
    }
    if (filtered_index >= filtered_count) {
        return 0;
    }
    surface = drawing_program_texture_project_surface_at(project, surface_index);
    if (!texture_workspace_surface_valid(surface)) {
        return 0;
    }
    document = &surface->storage->document;
    row = filtered_index / layout.columns;
    column = filtered_index % layout.columns;
    width = texture_workspace_content_width(document);
    height = texture_workspace_content_height(document);
    cell_x = layout.padding + ((float)column * (layout.cell_width + layout.gutter));
    cell_y = layout.padding + ((float)row * (layout.cell_height + layout.gutter));
    out_rect->x = origin_x + cell_x + ((layout.cell_width - width) * 0.5f);
    out_rect->y = origin_y + cell_y + ((layout.cell_height - height) * 0.5f);
    out_rect->width = width;
    out_rect->height = height;
    if (out_layout) {
        *out_layout = layout;
    }
    return 1;
}

static int texture_workspace_try_semantic_surface_rect(const DrawingProgramTextureProject *project,
                                                       uint32_t surface_index,
                                                       DrawingProgramTextureWorkspaceRect *out_rect,
                                                       DrawingProgramTextureWorkspaceLayout *out_layout) {
    DrawingProgramTextureWorkspaceSemanticMap semantic_map;
    DrawingProgramTextureWorkspaceLayout extras_layout;
    DrawingProgramTextureWorkspaceRect raw_front;
    DrawingProgramTextureWorkspaceRect raw_back;
    DrawingProgramTextureWorkspaceRect raw_left;
    DrawingProgramTextureWorkspaceRect raw_right;
    DrawingProgramTextureWorkspaceRect raw_top;
    DrawingProgramTextureWorkspaceRect raw_bottom;
    DrawingProgramTextureWorkspaceRect *target_rect = 0;
    float gutter;
    float padding;
    float min_x;
    float min_y;
    float max_x;
    float max_y;
    float semantic_width;
    float semantic_height;
    uint32_t extras_count = 0u;
    int32_t front_index;
    if (!project || !out_rect || !texture_workspace_try_build_semantic_map(project, &semantic_map)) {
        return 0;
    }
    gutter = drawing_program_viewport_base_pixel_size() * 24.0f;
    if (gutter < 18.0f) {
        gutter = 18.0f;
    }
    padding = gutter;
    memset(&extras_layout, 0, sizeof(extras_layout));
    switch (project->net_layout_kind) {
        case DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_PLANE_SINGLE: {
            const DrawingProgramTextureSurface *front_surface = drawing_program_texture_project_surface_at(
                project, (uint32_t)semantic_map.plane_front_index);
            if (!front_surface || !texture_workspace_surface_valid(front_surface)) {
                return 0;
            }
            semantic_width = texture_workspace_surface_width(front_surface) + (padding * 2.0f);
            semantic_height = texture_workspace_surface_height(front_surface) + (padding * 2.0f);
            if ((int32_t)surface_index == semantic_map.plane_front_index) {
                out_rect->x = padding;
                out_rect->y = padding;
                out_rect->width = texture_workspace_surface_width(front_surface);
                out_rect->height = texture_workspace_surface_height(front_surface);
            } else if (!texture_workspace_generic_surface_rect_filtered(project,
                                                                        &semantic_map,
                                                                        surface_index,
                                                                        0.0f,
                                                                        semantic_height + gutter,
                                                                        out_rect,
                                                                        &extras_layout)) {
                return 0;
            }
            (void)texture_workspace_build_generic_layout_filtered(project, &semantic_map, &extras_layout, &extras_count);
            if (out_layout) {
                memset(out_layout, 0, sizeof(*out_layout));
                out_layout->gutter = gutter;
                out_layout->padding = padding;
                out_layout->semantic_layout_active = 1u;
                out_layout->atlas_width = semantic_width;
                out_layout->atlas_height = semantic_height;
                if (extras_count > 0u) {
                    if (extras_layout.atlas_width > out_layout->atlas_width) {
                        out_layout->atlas_width = extras_layout.atlas_width;
                    }
                    out_layout->atlas_height = semantic_height + gutter + extras_layout.atlas_height;
                }
            }
            return 1;
        }
        case DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_RECT_PRISM_CROSS: break;
        default: return 0;
    }
    front_index = semantic_map.prism_front_index;
    raw_front = (DrawingProgramTextureWorkspaceRect){0.0f,
                                                     0.0f,
                                                     texture_workspace_surface_width(drawing_program_texture_project_surface_at(
                                                         project, (uint32_t)semantic_map.prism_front_index)),
                                                     texture_workspace_surface_height(drawing_program_texture_project_surface_at(
                                                         project, (uint32_t)semantic_map.prism_front_index))};
    raw_left = (DrawingProgramTextureWorkspaceRect){
        -gutter - texture_workspace_surface_width(
                      drawing_program_texture_project_surface_at(project, (uint32_t)semantic_map.prism_left_index)),
        0.0f,
        texture_workspace_surface_width(
            drawing_program_texture_project_surface_at(project, (uint32_t)semantic_map.prism_left_index)),
        texture_workspace_surface_height(
            drawing_program_texture_project_surface_at(project, (uint32_t)semantic_map.prism_left_index))};
    raw_right = (DrawingProgramTextureWorkspaceRect){
        raw_front.width + gutter,
        0.0f,
        texture_workspace_surface_width(
            drawing_program_texture_project_surface_at(project, (uint32_t)semantic_map.prism_right_index)),
        texture_workspace_surface_height(
            drawing_program_texture_project_surface_at(project, (uint32_t)semantic_map.prism_right_index))};
    raw_back = (DrawingProgramTextureWorkspaceRect){
        raw_right.x + raw_right.width + gutter,
        0.0f,
        texture_workspace_surface_width(
            drawing_program_texture_project_surface_at(project, (uint32_t)semantic_map.prism_back_index)),
        texture_workspace_surface_height(
            drawing_program_texture_project_surface_at(project, (uint32_t)semantic_map.prism_back_index))};
    raw_top = (DrawingProgramTextureWorkspaceRect){
        raw_front.x + ((raw_front.width -
                        texture_workspace_surface_width(
                            drawing_program_texture_project_surface_at(project, (uint32_t)semantic_map.prism_top_index))) *
                       0.5f),
        -gutter -
            texture_workspace_surface_height(
                drawing_program_texture_project_surface_at(project, (uint32_t)semantic_map.prism_top_index)),
        texture_workspace_surface_width(
            drawing_program_texture_project_surface_at(project, (uint32_t)semantic_map.prism_top_index)),
        texture_workspace_surface_height(
            drawing_program_texture_project_surface_at(project, (uint32_t)semantic_map.prism_top_index))};
    raw_bottom = (DrawingProgramTextureWorkspaceRect){
        raw_front.x + ((raw_front.width -
                        texture_workspace_surface_width(
                            drawing_program_texture_project_surface_at(project, (uint32_t)semantic_map.prism_bottom_index))) *
                       0.5f),
        raw_front.height + gutter,
        texture_workspace_surface_width(
            drawing_program_texture_project_surface_at(project, (uint32_t)semantic_map.prism_bottom_index)),
        texture_workspace_surface_height(
            drawing_program_texture_project_surface_at(project, (uint32_t)semantic_map.prism_bottom_index))};
    min_x = raw_left.x;
    min_y = raw_top.y;
    max_x = raw_back.x + raw_back.width;
    max_y = raw_bottom.y + raw_bottom.height;
    if ((raw_top.x + raw_top.width) > max_x) {
        max_x = raw_top.x + raw_top.width;
    }
    if (raw_top.x < min_x) {
        min_x = raw_top.x;
    }
    if ((raw_bottom.x + raw_bottom.width) > max_x) {
        max_x = raw_bottom.x + raw_bottom.width;
    }
    if (raw_bottom.x < min_x) {
        min_x = raw_bottom.x;
    }
    semantic_width = (max_x - min_x) + (padding * 2.0f);
    semantic_height = (max_y - min_y) + (padding * 2.0f);

    if ((int32_t)surface_index == semantic_map.prism_front_index) {
        target_rect = &raw_front;
    } else if ((int32_t)surface_index == semantic_map.prism_back_index) {
        target_rect = &raw_back;
    } else if ((int32_t)surface_index == semantic_map.prism_left_index) {
        target_rect = &raw_left;
    } else if ((int32_t)surface_index == semantic_map.prism_right_index) {
        target_rect = &raw_right;
    } else if ((int32_t)surface_index == semantic_map.prism_top_index) {
        target_rect = &raw_top;
    } else if ((int32_t)surface_index == semantic_map.prism_bottom_index) {
        target_rect = &raw_bottom;
    }
    if (target_rect) {
        out_rect->x = target_rect->x - min_x + padding;
        out_rect->y = target_rect->y - min_y + padding;
        out_rect->width = target_rect->width;
        out_rect->height = target_rect->height;
    } else if (!texture_workspace_generic_surface_rect_filtered(project,
                                                                &semantic_map,
                                                                surface_index,
                                                                0.0f,
                                                                semantic_height + gutter,
                                                                out_rect,
                                                                &extras_layout)) {
        return 0;
    }
    (void)texture_workspace_build_generic_layout_filtered(project, &semantic_map, &extras_layout, &extras_count);
    if (out_layout) {
        memset(out_layout, 0, sizeof(*out_layout));
        out_layout->gutter = gutter;
        out_layout->padding = padding;
        out_layout->semantic_layout_active = 1u;
        out_layout->atlas_width = semantic_width;
        out_layout->atlas_height = semantic_height;
        if (extras_count > 0u) {
            if (extras_layout.atlas_width > out_layout->atlas_width) {
                out_layout->atlas_width = extras_layout.atlas_width;
            }
            out_layout->atlas_height = semantic_height + gutter + extras_layout.atlas_height;
        }
        (void)front_index;
    }
    return 1;
}

static int texture_workspace_surface_base_rect(const DrawingProgramTextureProject *project,
                                               uint32_t surface_index,
                                               DrawingProgramTextureWorkspaceRect *out_rect,
                                               DrawingProgramTextureWorkspaceLayout *out_layout) {
    DrawingProgramTextureWorkspaceLayout layout;
    if (!project || !out_rect || surface_index >= project->surface_count) {
        return 0;
    }
    if (texture_workspace_try_semantic_surface_rect(project, surface_index, out_rect, &layout)) {
        if (out_layout) {
            *out_layout = layout;
        }
        return 1;
    }
    return texture_workspace_generic_surface_rect_filtered(project, 0, surface_index, 0.0f, 0.0f, out_rect, out_layout);
}

static void texture_workspace_adjust_layout_for_offsets(const DrawingProgramTextureProject *project,
                                                        DrawingProgramTextureWorkspaceLayout *layout) {
    uint32_t i;
    float min_x = 0.0f;
    float min_y = 0.0f;
    float max_x = 0.0f;
    float max_y = 0.0f;
    uint8_t have_bounds = 0u;
    if (!project || !layout) {
        return;
    }
    layout->origin_offset_x = 0.0f;
    layout->origin_offset_y = 0.0f;
    for (i = 0u; i < project->surface_count; ++i) {
        DrawingProgramTextureWorkspaceRect rect;
        const DrawingProgramTextureSurface *surface = drawing_program_texture_project_surface_at(project, i);
        if (!surface || !texture_workspace_surface_base_rect(project, i, &rect, 0)) {
            continue;
        }
        rect.x += surface->layout_offset_x;
        rect.y += surface->layout_offset_y;
        if (!have_bounds) {
            min_x = rect.x;
            min_y = rect.y;
            max_x = rect.x + rect.width;
            max_y = rect.y + rect.height;
            have_bounds = 1u;
            continue;
        }
        if (rect.x < min_x) {
            min_x = rect.x;
        }
        if (rect.y < min_y) {
            min_y = rect.y;
        }
        if ((rect.x + rect.width) > max_x) {
            max_x = rect.x + rect.width;
        }
        if ((rect.y + rect.height) > max_y) {
            max_y = rect.y + rect.height;
        }
    }
    if (!have_bounds) {
        return;
    }
    if (min_x < 0.0f) {
        layout->origin_offset_x = -min_x;
    }
    if (min_y < 0.0f) {
        layout->origin_offset_y = -min_y;
    }
    if ((max_x + layout->origin_offset_x) > layout->atlas_width) {
        layout->atlas_width = max_x + layout->origin_offset_x;
    }
    if ((max_y + layout->origin_offset_y) > layout->atlas_height) {
        layout->atlas_height = max_y + layout->origin_offset_y;
    }
}

static int texture_workspace_build_layout(const DrawingProgramTextureProject *project,
                                          DrawingProgramTextureWorkspaceLayout *out_layout) {
    DrawingProgramTextureWorkspaceRect ignored_rect;
    DrawingProgramTextureWorkspaceLayout layout;
    if (!project || !out_layout || project->surface_count == 0u) {
        return 0;
    }
    if (texture_workspace_try_semantic_surface_rect(project, project->active_surface_index, &ignored_rect, &layout) ||
        texture_workspace_build_generic_layout_filtered(project, 0, &layout, 0)) {
        texture_workspace_adjust_layout_for_offsets(project, &layout);
        *out_layout = layout;
        return 1;
    }
    return 0;
}

static int texture_workspace_surface_rect(const DrawingProgramTextureProject *project,
                                          uint32_t surface_index,
                                          DrawingProgramTextureWorkspaceRect *out_rect,
                                          DrawingProgramTextureWorkspaceLayout *out_layout) {
    DrawingProgramTextureWorkspaceLayout layout;
    const DrawingProgramTextureSurface *surface = 0;
    if (!project || !out_rect || surface_index >= project->surface_count ||
        !texture_workspace_build_layout(project, &layout) ||
        !texture_workspace_surface_base_rect(project, surface_index, out_rect, 0)) {
        return 0;
    }
    surface = drawing_program_texture_project_surface_at(project, surface_index);
    if (surface) {
        out_rect->x += surface->layout_offset_x + layout.origin_offset_x;
        out_rect->y += surface->layout_offset_y + layout.origin_offset_y;
    }
    if (out_layout) {
        *out_layout = layout;
    }
    return 1;
}

static int texture_workspace_core_from_state(const DrawingProgramViewportState *viewport,
                                             SDL_Rect pane_rect,
                                             float atlas_width,
                                             float atlas_height,
                                             CoreViewport2D *out_core) {
    float zoom;
    if (!viewport || !out_core || !texture_workspace_frame_valid(pane_rect) || atlas_width <= 0.0f ||
        atlas_height <= 0.0f) {
        return 0;
    }
    out_core->min_zoom = viewport->min_zoom;
    out_core->max_zoom = viewport->max_zoom;
    zoom = drawing_program_viewport_clamp_zoom(viewport->zoom);
    out_core->zoom = core_viewport2d_clamp_zoom(out_core, zoom);
    out_core->pan_x = ((float)pane_rect.x + ((float)pane_rect.w * 0.5f)) + viewport->pan_x -
                      ((atlas_width * out_core->zoom) * 0.5f);
    out_core->pan_y = ((float)pane_rect.y + ((float)pane_rect.h * 0.5f)) + viewport->pan_y -
                      ((atlas_height * out_core->zoom) * 0.5f);
    return core_viewport2d_validate(out_core).code == CORE_OK;
}

static int texture_workspace_state_from_core(DrawingProgramViewportState *viewport,
                                             SDL_Rect pane_rect,
                                             const CoreViewport2D *core,
                                             float atlas_width,
                                             float atlas_height) {
    if (!viewport || !core || !texture_workspace_frame_valid(pane_rect) || atlas_width <= 0.0f ||
        atlas_height <= 0.0f) {
        return 0;
    }
    viewport->pan_x = core->pan_x - ((float)pane_rect.x + ((float)pane_rect.w * 0.5f)) +
                      ((atlas_width * core->zoom) * 0.5f);
    viewport->pan_y = core->pan_y - ((float)pane_rect.y + ((float)pane_rect.h * 0.5f)) +
                      ((atlas_height * core->zoom) * 0.5f);
    viewport->zoom = drawing_program_viewport_clamp_zoom(core->zoom);
    viewport->min_zoom = core->min_zoom;
    viewport->max_zoom = core->max_zoom;
    return 1;
}

static int texture_workspace_content_point_for_screen(const DrawingProgramAppContext *ctx,
                                                      SDL_Rect pane_rect,
                                                      int sx,
                                                      int sy,
                                                      float *out_content_x,
                                                      float *out_content_y,
                                                      DrawingProgramTextureWorkspaceLayout *out_layout) {
    DrawingProgramTextureWorkspaceLayout layout;
    CoreViewport2D core;
    if (!ctx || !out_content_x || !out_content_y || !texture_workspace_build_layout(&ctx->texture_project, &layout) ||
        !texture_workspace_core_from_state(&ctx->editor.viewport,
                                           pane_rect,
                                           layout.atlas_width,
                                           layout.atlas_height,
                                           &core)) {
        return 0;
    }
    if (core_viewport2d_screen_to_content(&core, (float)sx, (float)sy, out_content_x, out_content_y).code != CORE_OK) {
        return 0;
    }
    if (out_layout) {
        *out_layout = layout;
    }
    return 1;
}

static int texture_workspace_sample_from_surface_content(const DrawingProgramDocument *document,
                                                         float local_content_x,
                                                         float local_content_y,
                                                         uint32_t *out_sample_x,
                                                         uint32_t *out_sample_y,
                                                         int clamp_to_surface) {
    float base_pixel = drawing_program_viewport_base_pixel_size();
    float max_x;
    float max_y;
    if (!document || !out_sample_x || !out_sample_y || document->raster_width == 0u || document->raster_height == 0u) {
        return 0;
    }
    max_x = ((float)document->raster_width * base_pixel) - 0.001f;
    max_y = ((float)document->raster_height * base_pixel) - 0.001f;
    if (!clamp_to_surface) {
        if (local_content_x < 0.0f || local_content_y < 0.0f || local_content_x > max_x || local_content_y > max_y) {
            return 0;
        }
    } else {
        if (local_content_x < 0.0f) {
            local_content_x = 0.0f;
        }
        if (local_content_y < 0.0f) {
            local_content_y = 0.0f;
        }
        if (local_content_x > max_x) {
            local_content_x = max_x;
        }
        if (local_content_y > max_y) {
            local_content_y = max_y;
        }
    }
    *out_sample_x = (uint32_t)floorf(local_content_x / base_pixel);
    *out_sample_y = (uint32_t)floorf(local_content_y / base_pixel);
    if (*out_sample_x >= document->raster_width) {
        *out_sample_x = document->raster_width - 1u;
    }
    if (*out_sample_y >= document->raster_height) {
        *out_sample_y = document->raster_height - 1u;
    }
    return 1;
}

int drawing_program_texture_workspace_surface_sheet_metrics(const DrawingProgramAppContext *ctx,
                                                            SDL_Rect pane_rect,
                                                            uint32_t surface_index,
                                                            VisualCanvasSheetMetrics *out_metrics) {
    DrawingProgramTextureWorkspaceRect surface_rect;
    DrawingProgramTextureWorkspaceLayout layout;
    CoreViewport2D core;
    float screen_x = 0.0f;
    float screen_y = 0.0f;
    if (!ctx || !out_metrics || !texture_workspace_surface_rect(&ctx->texture_project,
                                                                surface_index,
                                                                &surface_rect,
                                                                &layout) ||
        !texture_workspace_core_from_state(
            &ctx->editor.viewport, pane_rect, layout.atlas_width, layout.atlas_height, &core) ||
        core_viewport2d_content_to_screen(&core, surface_rect.x, surface_rect.y, &screen_x, &screen_y).code != CORE_OK) {
        out_metrics->sheet_rect = (SDL_Rect){0, 0, 0, 0};
        out_metrics->pixel_size = 0.0f;
        return 0;
    }
    out_metrics->pixel_size = drawing_program_viewport_base_pixel_size() * core.zoom;
    out_metrics->sheet_rect.x = (int)screen_x;
    out_metrics->sheet_rect.y = (int)screen_y;
    out_metrics->sheet_rect.w = (int)(surface_rect.width * core.zoom);
    out_metrics->sheet_rect.h = (int)(surface_rect.height * core.zoom);
    return 1;
}

int drawing_program_texture_workspace_active_sheet_metrics(const DrawingProgramAppContext *ctx,
                                                           SDL_Rect pane_rect,
                                                           VisualCanvasSheetMetrics *out_metrics) {
    if (!ctx) {
        return 0;
    }
    return drawing_program_texture_workspace_surface_sheet_metrics(
        ctx, pane_rect, ctx->texture_project.active_surface_index, out_metrics);
}

int drawing_program_texture_workspace_hit_test_surface(const DrawingProgramAppContext *ctx,
                                                       SDL_Rect pane_rect,
                                                       int sx,
                                                       int sy,
                                                       uint32_t *out_surface_index) {
    DrawingProgramTextureWorkspaceLayout layout;
    float content_x = 0.0f;
    float content_y = 0.0f;
    uint32_t i;
    if (!ctx || !out_surface_index ||
        !texture_workspace_content_point_for_screen(ctx, pane_rect, sx, sy, &content_x, &content_y, &layout)) {
        return 0;
    }
    for (i = 0u; i < ctx->texture_project.surface_count; ++i) {
        DrawingProgramTextureWorkspaceRect rect;
        if (!texture_workspace_surface_rect(&ctx->texture_project, i, &rect, 0)) {
            continue;
        }
        if (content_x >= rect.x && content_x < (rect.x + rect.width) && content_y >= rect.y &&
            content_y < (rect.y + rect.height)) {
            *out_surface_index = i;
            return 1;
        }
    }
    return 0;
}

int drawing_program_texture_workspace_screen_to_active_sample(const DrawingProgramAppContext *ctx,
                                                              SDL_Rect pane_rect,
                                                              int sx,
                                                              int sy,
                                                              uint32_t *out_sample_x,
                                                              uint32_t *out_sample_y) {
    DrawingProgramTextureWorkspaceRect rect;
    const DrawingProgramTextureSurface *surface;
    float content_x = 0.0f;
    float content_y = 0.0f;
    if (!ctx || !out_sample_x || !out_sample_y ||
        !texture_workspace_content_point_for_screen(ctx, pane_rect, sx, sy, &content_x, &content_y, 0) ||
        !texture_workspace_surface_rect(&ctx->texture_project, ctx->texture_project.active_surface_index, &rect, 0)) {
        return 0;
    }
    surface = drawing_program_texture_project_surface_at(&ctx->texture_project, ctx->texture_project.active_surface_index);
    if (!texture_workspace_surface_valid(surface)) {
        return 0;
    }
    return texture_workspace_sample_from_surface_content(&surface->storage->document,
                                                         content_x - rect.x,
                                                         content_y - rect.y,
                                                         out_sample_x,
                                                         out_sample_y,
                                                         0);
}

int drawing_program_texture_workspace_screen_to_active_sample_clamped(const DrawingProgramAppContext *ctx,
                                                                      SDL_Rect pane_rect,
                                                                      int sx,
                                                                      int sy,
                                                                      uint32_t *out_sample_x,
                                                                      uint32_t *out_sample_y) {
    DrawingProgramTextureWorkspaceRect rect;
    const DrawingProgramTextureSurface *surface;
    float content_x = 0.0f;
    float content_y = 0.0f;
    if (!ctx || !out_sample_x || !out_sample_y ||
        !texture_workspace_content_point_for_screen(ctx, pane_rect, sx, sy, &content_x, &content_y, 0) ||
        !texture_workspace_surface_rect(&ctx->texture_project, ctx->texture_project.active_surface_index, &rect, 0)) {
        return 0;
    }
    surface = drawing_program_texture_project_surface_at(&ctx->texture_project, ctx->texture_project.active_surface_index);
    if (!texture_workspace_surface_valid(surface)) {
        return 0;
    }
    return texture_workspace_sample_from_surface_content(&surface->storage->document,
                                                         content_x - rect.x,
                                                         content_y - rect.y,
                                                         out_sample_x,
                                                         out_sample_y,
                                                         1);
}

int drawing_program_texture_workspace_fit_all(DrawingProgramAppContext *ctx, SDL_Rect pane_rect) {
    DrawingProgramTextureWorkspaceLayout layout;
    CoreViewport2D core;
    float view_padding;
    float available_width;
    float available_height;
    float zoom_x;
    float zoom_y;
    if (!ctx || !texture_workspace_frame_valid(pane_rect) ||
        !texture_workspace_build_layout(&ctx->texture_project, &layout)) {
        return 0;
    }
    if (core_viewport2d_init(&core).code != CORE_OK) {
        return 0;
    }
    core.min_zoom = ctx->editor.viewport.min_zoom;
    core.max_zoom = ctx->editor.viewport.max_zoom;
    view_padding = texture_workspace_frame_padding(pane_rect);
    available_width = (float)pane_rect.w - (view_padding * 2.0f);
    available_height = (float)pane_rect.h - (view_padding * 2.0f);
    if (available_width <= 0.0f || available_height <= 0.0f) {
        available_width = (float)pane_rect.w;
        available_height = (float)pane_rect.h;
    }
    zoom_x = available_width / layout.atlas_width;
    zoom_y = available_height / layout.atlas_height;
    core.zoom = drawing_program_viewport_clamp_zoom(zoom_x < zoom_y ? zoom_x : zoom_y);
    core.zoom = core_viewport2d_clamp_zoom(&core, core.zoom);
    core.pan_x = ((float)pane_rect.x + ((float)pane_rect.w * 0.5f)) - ((layout.atlas_width * core.zoom) * 0.5f);
    core.pan_y = ((float)pane_rect.y + ((float)pane_rect.h * 0.5f)) - ((layout.atlas_height * core.zoom) * 0.5f);
    return texture_workspace_state_from_core(
        &ctx->editor.viewport, pane_rect, &core, layout.atlas_width, layout.atlas_height);
}

int drawing_program_texture_workspace_fit_surface(DrawingProgramAppContext *ctx,
                                                  SDL_Rect pane_rect,
                                                  uint32_t surface_index) {
    DrawingProgramTextureWorkspaceRect rect;
    DrawingProgramTextureWorkspaceLayout layout;
    CoreViewport2D core;
    float view_padding;
    float available_width;
    float available_height;
    float zoom_x;
    float zoom_y;
    if (!ctx || !texture_workspace_frame_valid(pane_rect) ||
        !texture_workspace_surface_rect(&ctx->texture_project, surface_index, &rect, &layout)) {
        return 0;
    }
    if (core_viewport2d_init(&core).code != CORE_OK) {
        return 0;
    }
    core.min_zoom = ctx->editor.viewport.min_zoom;
    core.max_zoom = ctx->editor.viewport.max_zoom;
    view_padding = texture_workspace_frame_padding(pane_rect);
    available_width = (float)pane_rect.w - (view_padding * 2.0f);
    available_height = (float)pane_rect.h - (view_padding * 2.0f);
    if (available_width <= 0.0f || available_height <= 0.0f) {
        available_width = (float)pane_rect.w;
        available_height = (float)pane_rect.h;
    }
    zoom_x = available_width / rect.width;
    zoom_y = available_height / rect.height;
    core.zoom = drawing_program_viewport_clamp_zoom(zoom_x < zoom_y ? zoom_x : zoom_y);
    core.zoom = core_viewport2d_clamp_zoom(&core, core.zoom);
    core.pan_x = ((float)pane_rect.x + ((float)pane_rect.w * 0.5f)) -
                 (((rect.x + (rect.width * 0.5f)) * core.zoom));
    core.pan_y = ((float)pane_rect.y + ((float)pane_rect.h * 0.5f)) -
                 (((rect.y + (rect.height * 0.5f)) * core.zoom));
    return texture_workspace_state_from_core(
        &ctx->editor.viewport, pane_rect, &core, layout.atlas_width, layout.atlas_height);
}
