#include "drawing_program/drawing_program_object_rasterize.h"

#include <math.h>
#include <string.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_object_geometry.h"

static CoreResult object_rasterize_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static int object_style_includes_fill(uint8_t style_mode) {
    return (style_mode == 1u || style_mode == 2u) ? 1 : 0;
}

static int object_style_includes_outline(uint8_t style_mode) {
    return (style_mode == 1u) ? 0 : 1;
}

static uint8_t object_sample_value_from_color_index(uint8_t color_index) {
    return drawing_program_color_value_from_index(drawing_program_color_index_clamp(color_index));
}

static CoreResult object_rasterize_write_sample(DrawingProgramDocument *document,
                                                DrawingProgramLayerRasterStore *layer_rasters,
                                                DrawingProgramHistory *history,
                                                uint32_t target_layer_id,
                                                int32_t sample_x,
                                                int32_t sample_y,
                                                uint8_t value) {
    if (!document || !layer_rasters || !history || target_layer_id == 0u) {
        return object_rasterize_invalid("invalid object sample write request");
    }
    if (sample_x < 0 || sample_y < 0 ||
        sample_x >= (int32_t)document->raster_width ||
        sample_y >= (int32_t)document->raster_height) {
        return core_result_ok();
    }
    return drawing_program_history_apply_set_sample_value(history,
                                                          document,
                                                          layer_rasters,
                                                          target_layer_id,
                                                          (uint32_t)sample_x,
                                                          (uint32_t)sample_y,
                                                          value);
}

static CoreResult object_rasterize_rect_fill(DrawingProgramDocument *document,
                                             DrawingProgramLayerRasterStore *layer_rasters,
                                             DrawingProgramHistory *history,
                                             uint32_t target_layer_id,
                                             const DrawingProgramObjectRecord *object,
                                             uint8_t fill_value) {
    uint32_t x;
    uint32_t y;
    if (!object || object->width == 0u || object->height == 0u) {
        return core_result_ok();
    }
    for (y = 0u; y < object->height; ++y) {
        for (x = 0u; x < object->width; ++x) {
            CoreResult result = object_rasterize_write_sample(document,
                                                              layer_rasters,
                                                              history,
                                                              target_layer_id,
                                                              object->origin_x + (int32_t)x,
                                                              object->origin_y + (int32_t)y,
                                                              fill_value);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    }
    return core_result_ok();
}

static CoreResult object_rasterize_rect_outline(DrawingProgramDocument *document,
                                                DrawingProgramLayerRasterStore *layer_rasters,
                                                DrawingProgramHistory *history,
                                                uint32_t target_layer_id,
                                                const DrawingProgramObjectRecord *object,
                                                uint8_t stroke_value,
                                                uint32_t stroke_width) {
    uint32_t pass;
    int32_t left;
    int32_t right;
    int32_t top;
    int32_t bottom;
    if (!object || object->width == 0u || object->height == 0u) {
        return core_result_ok();
    }
    if (stroke_width == 0u) {
        stroke_width = 1u;
    }
    for (pass = 0u; pass < stroke_width; ++pass) {
        int32_t x;
        int32_t y;
        CoreResult result;
        left = object->origin_x + (int32_t)pass;
        top = object->origin_y + (int32_t)pass;
        right = object->origin_x + (int32_t)object->width - 1 - (int32_t)pass;
        bottom = object->origin_y + (int32_t)object->height - 1 - (int32_t)pass;
        if (left > right || top > bottom) {
            break;
        }
        for (x = left; x <= right; ++x) {
            result = object_rasterize_write_sample(
                document, layer_rasters, history, target_layer_id, x, top, stroke_value);
            if (result.code != CORE_OK) {
                return result;
            }
            if (bottom != top) {
                result = object_rasterize_write_sample(
                    document, layer_rasters, history, target_layer_id, x, bottom, stroke_value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
        for (y = top + 1; y < bottom; ++y) {
            result = object_rasterize_write_sample(
                document, layer_rasters, history, target_layer_id, left, y, stroke_value);
            if (result.code != CORE_OK) {
                return result;
            }
            if (right != left) {
                result = object_rasterize_write_sample(
                    document, layer_rasters, history, target_layer_id, right, y, stroke_value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
    }
    return core_result_ok();
}

static CoreResult object_rasterize_ellipse_fill(DrawingProgramDocument *document,
                                                DrawingProgramLayerRasterStore *layer_rasters,
                                                DrawingProgramHistory *history,
                                                uint32_t target_layer_id,
                                                const DrawingProgramObjectRecord *object,
                                                uint8_t fill_value) {
    int32_t x;
    int32_t y;
    double rx;
    double ry;
    double cx;
    double cy;
    if (!object || object->width == 0u || object->height == 0u) {
        return core_result_ok();
    }
    rx = ((double)object->width) * 0.5;
    ry = ((double)object->height) * 0.5;
    if (rx <= 0.0 || ry <= 0.0) {
        return core_result_ok();
    }
    cx = (double)object->origin_x + rx;
    cy = (double)object->origin_y + ry;
    for (y = object->origin_y; y < (int32_t)(object->origin_y + (int32_t)object->height); ++y) {
        for (x = object->origin_x; x < (int32_t)(object->origin_x + (int32_t)object->width); ++x) {
            double nx = ((double)x + 0.5 - cx) / rx;
            double ny = ((double)y + 0.5 - cy) / ry;
            double d = (nx * nx) + (ny * ny);
            if (d <= 1.0) {
                CoreResult result = object_rasterize_write_sample(
                    document, layer_rasters, history, target_layer_id, x, y, fill_value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
    }
    return core_result_ok();
}

static CoreResult object_rasterize_ellipse_outline(DrawingProgramDocument *document,
                                                   DrawingProgramLayerRasterStore *layer_rasters,
                                                   DrawingProgramHistory *history,
                                                   uint32_t target_layer_id,
                                                   const DrawingProgramObjectRecord *object,
                                                   uint8_t stroke_value,
                                                   uint32_t stroke_width) {
    int32_t x;
    int32_t y;
    double rx;
    double ry;
    double cx;
    double cy;
    double min_radius;
    double thickness_norm;
    double inner_threshold;
    if (!object || object->width == 0u || object->height == 0u) {
        return core_result_ok();
    }
    rx = ((double)object->width) * 0.5;
    ry = ((double)object->height) * 0.5;
    if (rx <= 0.0 || ry <= 0.0) {
        return core_result_ok();
    }
    if (stroke_width == 0u) {
        stroke_width = 1u;
    }
    min_radius = (rx < ry) ? rx : ry;
    thickness_norm = (double)stroke_width / (min_radius > 1.0 ? min_radius : 1.0);
    if (thickness_norm > 1.0) {
        thickness_norm = 1.0;
    }
    inner_threshold = 1.0 - thickness_norm;
    if (inner_threshold < 0.0) {
        inner_threshold = 0.0;
    }
    cx = (double)object->origin_x + rx;
    cy = (double)object->origin_y + ry;
    for (y = object->origin_y; y < (int32_t)(object->origin_y + (int32_t)object->height); ++y) {
        for (x = object->origin_x; x < (int32_t)(object->origin_x + (int32_t)object->width); ++x) {
            double nx = ((double)x + 0.5 - cx) / rx;
            double ny = ((double)y + 0.5 - cy) / ry;
            double d = (nx * nx) + (ny * ny);
            if (d <= 1.0 && d >= inner_threshold) {
                CoreResult result = object_rasterize_write_sample(
                    document, layer_rasters, history, target_layer_id, x, y, stroke_value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
    }
    return core_result_ok();
}

static int object_rasterize_path_contains_fill(const DrawingProgramObjectRecord *object,
                                               const double *flat_xy,
                                               uint32_t flat_count,
                                               double px,
                                               double py) {
    if (!object || !object->path_closed || !flat_xy || flat_count < 3u) {
        return 0;
    }
    return drawing_program_object_path_flattened_contains_fill(flat_xy, flat_count, px, py);
}

static int object_rasterize_path_contains_outline(const DrawingProgramObjectRecord *object,
                                                  const double *flat_xy,
                                                  uint32_t flat_count,
                                                  double px,
                                                  double py,
                                                  uint32_t stroke_width) {
    double half_width;
    double threshold_sq;
    if (!object || !flat_xy || flat_count < 2u) {
        return 0;
    }
    if (stroke_width == 0u) {
        stroke_width = 1u;
    }
    half_width = ((double)stroke_width) * 0.5;
    if (half_width < 0.6) {
        half_width = 0.6;
    }
    threshold_sq = half_width * half_width;
    return drawing_program_object_path_flattened_contains_outline(
        flat_xy, flat_count, object->path_closed, px, py, threshold_sq);
}

static CoreResult object_rasterize_path(DrawingProgramDocument *document,
                                        DrawingProgramLayerRasterStore *layer_rasters,
                                        DrawingProgramHistory *history,
                                        uint32_t target_layer_id,
                                        const DrawingProgramObjectRecord *object,
                                        uint8_t style_mode,
                                        uint8_t stroke_value,
                                        uint8_t fill_value,
                                        uint32_t stroke_width) {
    int32_t x;
    int32_t y;
    int has_fill;
    int has_outline;
    double flat_xy[DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS * 24u * 2u];
    uint32_t flat_count = 0u;
    CoreResult flat_result;
    if (!document || !layer_rasters || !history || !object || object->path_point_count < 2u) {
        return core_result_ok();
    }
    has_fill = object_style_includes_fill(style_mode) ? 1 : 0;
    has_outline = object_style_includes_outline(style_mode) ? 1 : 0;
    if (!has_fill && !has_outline) {
        return core_result_ok();
    }
    flat_result = drawing_program_object_path_flatten_points(
        object, 0, 0, flat_xy, DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS * 24u, &flat_count);
    if (flat_result.code != CORE_OK) {
        return flat_result;
    }
    for (y = object->origin_y; y < (object->origin_y + (int32_t)object->height); ++y) {
        for (x = object->origin_x; x < (object->origin_x + (int32_t)object->width); ++x) {
            double px;
            double py;
            int on_outline = 0;
            int in_fill = 0;
            if (x < 0 || y < 0 || x >= (int32_t)document->raster_width || y >= (int32_t)document->raster_height) {
                continue;
            }
            px = (double)x + 0.5;
            py = (double)y + 0.5;
            if (has_outline) {
                on_outline = object_rasterize_path_contains_outline(object, flat_xy, flat_count, px, py, stroke_width);
            }
            if (has_fill) {
                in_fill = object_rasterize_path_contains_fill(object, flat_xy, flat_count, px, py);
            }
            if (on_outline) {
                CoreResult result = object_rasterize_write_sample(
                    document, layer_rasters, history, target_layer_id, x, y, stroke_value);
                if (result.code != CORE_OK) {
                    return result;
                }
            } else if (in_fill) {
                CoreResult result = object_rasterize_write_sample(
                    document, layer_rasters, history, target_layer_id, x, y, fill_value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
    }
    return core_result_ok();
}

static CoreResult object_rasterize_single(DrawingProgramDocument *document,
                                          DrawingProgramLayerRasterStore *layer_rasters,
                                          DrawingProgramHistory *history,
                                          uint32_t target_layer_id,
                                          const DrawingProgramObjectRecord *object) {
    uint8_t style_mode;
    uint8_t stroke_value;
    uint8_t fill_value;
    uint32_t stroke_width;
    CoreResult result = core_result_ok();
    if (!document || !layer_rasters || !history || !object || target_layer_id == 0u) {
        return object_rasterize_invalid("invalid object rasterize request");
    }
    style_mode = object->style_mode;
    if (style_mode > 2u) {
        style_mode = 2u;
    }
    stroke_value = object_sample_value_from_color_index(object->stroke_color_index);
    fill_value = object_sample_value_from_color_index(object->fill_color_index);
    stroke_width = object->stroke_width == 0u ? 1u : (uint32_t)object->stroke_width;

    if (object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
        return object_rasterize_path(document,
                                     layer_rasters,
                                     history,
                                     target_layer_id,
                                     object,
                                     style_mode,
                                     stroke_value,
                                     fill_value,
                                     stroke_width);
    }

    if (object->type == (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_ELLIPSE) {
        if (object_style_includes_fill(style_mode)) {
            result = object_rasterize_ellipse_fill(
                document, layer_rasters, history, target_layer_id, object, fill_value);
            if (result.code != CORE_OK) {
                return result;
            }
        }
        if (object_style_includes_outline(style_mode)) {
            result = object_rasterize_ellipse_outline(
                document, layer_rasters, history, target_layer_id, object, stroke_value, stroke_width);
        }
        return result;
    }

    if (object_style_includes_fill(style_mode)) {
        result = object_rasterize_rect_fill(
            document, layer_rasters, history, target_layer_id, object, fill_value);
        if (result.code != CORE_OK) {
            return result;
        }
    }
    if (object_style_includes_outline(style_mode)) {
        result = object_rasterize_rect_outline(
            document, layer_rasters, history, target_layer_id, object, stroke_value, stroke_width);
    }
    return result;
}

CoreResult drawing_program_object_rasterize_selection_to_layer(
    DrawingProgramDocument *document,
    DrawingProgramLayerRasterStore *layer_rasters,
    DrawingProgramHistory *history,
    DrawingProgramObjectStore *object_store,
    DrawingProgramObjectSelectionState *selection,
    uint32_t target_layer_id,
    uint32_t *out_rasterized_count) {
    CoreResult result;
    uint32_t i;
    uint32_t rasterized_count = 0u;
    uint32_t rasterized_ids[DRAWING_PROGRAM_MAX_OBJECTS];
    uint32_t layer_index = 0u;
    if (!document || !layer_rasters || !history || !object_store || !selection || target_layer_id == 0u) {
        return object_rasterize_invalid("invalid rasterize-selection request");
    }
    if (drawing_program_document_layer_index_for_id(document, target_layer_id, &layer_index).code != CORE_OK) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "target layer for rasterize not found" };
    }
    if (selection->count == 0u) {
        if (out_rasterized_count) {
            *out_rasterized_count = 0u;
        }
        return core_result_ok();
    }

    memset(rasterized_ids, 0, sizeof(rasterized_ids));
    result = drawing_program_history_begin_group(history);
    if (result.code != CORE_OK) {
        return result;
    }
    for (i = 0u; i < selection->count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        uint32_t object_id = selection->object_ids[i];
        const DrawingProgramObjectRecord *object = 0;
        if (object_id == 0u) {
            continue;
        }
        object = drawing_program_object_store_get_by_id(object_store, object_id);
        if (!object || !object->visible || object->locked) {
            continue;
        }
        result = object_rasterize_single(document, layer_rasters, history, target_layer_id, object);
        if (result.code != CORE_OK) {
            (void)drawing_program_history_end_group(history);
            return result;
        }
        rasterized_ids[rasterized_count] = object_id;
        rasterized_count += 1u;
    }
    result = drawing_program_history_end_group(history);
    if (result.code != CORE_OK) {
        return result;
    }

    for (i = 0u; i < rasterized_count; ++i) {
        (void)drawing_program_object_store_remove_by_id(object_store, rasterized_ids[i], 0);
    }
    drawing_program_object_selection_reset(selection);
    if (out_rasterized_count) {
        *out_rasterized_count = rasterized_count;
    }
    return core_result_ok();
}
