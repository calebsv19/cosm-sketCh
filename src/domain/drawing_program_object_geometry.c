#include "drawing_program/drawing_program_object_geometry.h"

#include <limits.h>
#include <math.h>
#include <string.h>

static CoreResult object_geometry_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

void drawing_program_object_path_payload_clear(DrawingProgramObjectRecord *object) {
    if (!object) {
        return;
    }
    object->path_point_count = 0u;
    object->path_closed = 0u;
    object->path_reserved0 = 0u;
    memset(object->path_points, 0, sizeof(object->path_points));
}

int drawing_program_object_path_payload_valid(const DrawingProgramObjectRecord *object) {
    if (!object) {
        return 0;
    }
    if (object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
        return 1;
    }
    if (object->path_point_count < 2u ||
        object->path_point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
        return 0;
    }
    return 1;
}

CoreResult drawing_program_object_path_payload_bounds(const DrawingProgramPathPayload *payload,
                                                      int32_t *out_origin_x,
                                                      int32_t *out_origin_y,
                                                      uint32_t *out_width,
                                                      uint32_t *out_height) {
    uint32_t i;
    int32_t min_x;
    int32_t min_y;
    int32_t max_x;
    int32_t max_y;
    int64_t span_x;
    int64_t span_y;
    if (!payload || !out_origin_x || !out_origin_y || !out_width || !out_height) {
        return object_geometry_invalid("invalid path bounds request");
    }
    if (payload->point_count < 2u ||
        payload->point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
        return object_geometry_invalid("invalid path point count");
    }
    min_x = payload->points[0].x;
    min_y = payload->points[0].y;
    max_x = payload->points[0].x;
    max_y = payload->points[0].y;
    for (i = 1u; i < payload->point_count; ++i) {
        const int32_t px = payload->points[i].x;
        const int32_t py = payload->points[i].y;
        if (px < min_x) {
            min_x = px;
        }
        if (px > max_x) {
            max_x = px;
        }
        if (py < min_y) {
            min_y = py;
        }
        if (py > max_y) {
            max_y = py;
        }
    }
    span_x = ((int64_t)max_x - (int64_t)min_x) + 1ll;
    span_y = ((int64_t)max_y - (int64_t)min_y) + 1ll;
    if (span_x <= 0ll || span_y <= 0ll ||
        span_x > (int64_t)UINT32_MAX || span_y > (int64_t)UINT32_MAX) {
        return object_geometry_invalid("invalid path bounds span");
    }
    *out_origin_x = min_x;
    *out_origin_y = min_y;
    *out_width = (uint32_t)span_x;
    *out_height = (uint32_t)span_y;
    return core_result_ok();
}

int drawing_program_object_bounds_contains(const DrawingProgramObjectRecord *object,
                                           uint32_t sample_x,
                                           uint32_t sample_y) {
    int64_t left;
    int64_t top;
    int64_t right;
    int64_t bottom;
    int64_t sx;
    int64_t sy;
    if (!object) {
        return 0;
    }
    left = (int64_t)object->origin_x;
    top = (int64_t)object->origin_y;
    right = left + (int64_t)object->width;
    bottom = top + (int64_t)object->height;
    sx = (int64_t)sample_x;
    sy = (int64_t)sample_y;
    if (sx < left || sy < top) {
        return 0;
    }
    if (sx >= right || sy >= bottom) {
        return 0;
    }
    return 1;
}

int drawing_program_object_layer_allows_interaction(const DrawingProgramDocument *document, uint32_t layer_id) {
    uint32_t layer_index = 0u;
    if (!document) {
        return 1;
    }
    if (drawing_program_document_layer_index_for_id(document, layer_id, &layer_index).code != CORE_OK) {
        return 0;
    }
    if (!document->layers[layer_index].visible || document->layers[layer_index].locked) {
        return 0;
    }
    return 1;
}

int drawing_program_object_ellipse_contains(const DrawingProgramObjectRecord *object,
                                            uint32_t sample_x,
                                            uint32_t sample_y) {
    double rx;
    double ry;
    double cx;
    double cy;
    double dx;
    double dy;
    if (!object || object->width == 0u || object->height == 0u) {
        return 0;
    }
    if (!drawing_program_object_bounds_contains(object, sample_x, sample_y)) {
        return 0;
    }
    rx = ((double)object->width) * 0.5;
    ry = ((double)object->height) * 0.5;
    if (rx <= 0.0 || ry <= 0.0) {
        return 0;
    }
    cx = (double)object->origin_x + rx;
    cy = (double)object->origin_y + ry;
    dx = (((double)sample_x) + 0.5) - cx;
    dy = (((double)sample_y) + 0.5) - cy;
    return (((dx * dx) / (rx * rx)) + ((dy * dy) / (ry * ry)) <= 1.0) ? 1 : 0;
}

static int object_style_includes_fill(uint8_t style_mode) {
    return (style_mode == 1u || style_mode == 2u) ? 1 : 0;
}

static int object_style_includes_outline(uint8_t style_mode) {
    return (style_mode == 1u) ? 0 : 1;
}

static double path_segment_distance_sq(double px,
                                       double py,
                                       double x0,
                                       double y0,
                                       double x1,
                                       double y1) {
    double dx = x1 - x0;
    double dy = y1 - y0;
    double len_sq = (dx * dx) + (dy * dy);
    double t;
    double rx;
    double ry;
    if (len_sq <= 0.0) {
        rx = px - x0;
        ry = py - y0;
        return (rx * rx) + (ry * ry);
    }
    t = (((px - x0) * dx) + ((py - y0) * dy)) / len_sq;
    if (t < 0.0) {
        t = 0.0;
    } else if (t > 1.0) {
        t = 1.0;
    }
    rx = px - (x0 + (t * dx));
    ry = py - (y0 + (t * dy));
    return (rx * rx) + (ry * ry);
}

static int object_path_contains_fill(const DrawingProgramObjectRecord *object, double px, double py) {
    uint32_t i;
    uint32_t j;
    int inside = 0;
    uint32_t point_count;
    if (!object || object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH || !object->path_closed) {
        return 0;
    }
    point_count = (uint32_t)object->path_point_count;
    if (point_count < 3u || point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
        return 0;
    }
    for (i = 0u, j = point_count - 1u; i < point_count; j = i++) {
        const double xi = (double)object->path_points[i].x + 0.5;
        const double yi = (double)object->path_points[i].y + 0.5;
        const double xj = (double)object->path_points[j].x + 0.5;
        const double yj = (double)object->path_points[j].y + 0.5;
        const int y_cross = ((yi > py) != (yj > py));
        if (!y_cross) {
            continue;
        }
        if (fabs(yj - yi) < 1e-6) {
            continue;
        }
        if (px < (((xj - xi) * (py - yi)) / (yj - yi)) + xi) {
            inside = !inside;
        }
    }
    return inside;
}

static int object_path_contains_outline(const DrawingProgramObjectRecord *object, double px, double py) {
    uint32_t segment_count = 0u;
    uint32_t point_count;
    uint32_t i;
    double half_width;
    const double pick_radius = 1.35;
    double threshold_sq;
    if (!object || object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
        return 0;
    }
    point_count = (uint32_t)object->path_point_count;
    if (point_count < 2u || point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS) {
        return 0;
    }
    segment_count = point_count - 1u;
    if (object->path_closed) {
        segment_count += 1u;
    }
    if (segment_count == 0u) {
        return 0;
    }
    half_width = ((double)(object->stroke_width == 0u ? 1u : object->stroke_width)) * 0.5;
    if (half_width < 0.6) {
        half_width = 0.6;
    }
    half_width += pick_radius;
    threshold_sq = half_width * half_width;
    for (i = 0u; i < segment_count; ++i) {
        uint32_t a = i;
        uint32_t b = (i + 1u < point_count) ? (i + 1u) : 0u;
        double d_sq = path_segment_distance_sq(px,
                                               py,
                                               (double)object->path_points[a].x + 0.5,
                                               (double)object->path_points[a].y + 0.5,
                                               (double)object->path_points[b].x + 0.5,
                                               (double)object->path_points[b].y + 0.5);
        if (d_sq <= threshold_sq) {
            return 1;
        }
    }
    return 0;
}

int drawing_program_object_path_contains(const DrawingProgramObjectRecord *object,
                                         uint32_t sample_x,
                                         uint32_t sample_y) {
    double px;
    double py;
    int fill_hit;
    int outline_hit;
    if (!object) {
        return 0;
    }
    if (!drawing_program_object_bounds_contains(object, sample_x, sample_y)) {
        return 0;
    }
    px = (double)sample_x + 0.5;
    py = (double)sample_y + 0.5;
    fill_hit = object_style_includes_fill(object->style_mode) ? object_path_contains_fill(object, px, py) : 0;
    outline_hit = object_style_includes_outline(object->style_mode) ? object_path_contains_outline(object, px, py) : 0;
    return (fill_hit || outline_hit) ? 1 : 0;
}
