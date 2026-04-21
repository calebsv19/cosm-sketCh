#include "drawing_program/drawing_program_object_geometry.h"

#include <float.h>
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
    if (drawing_program_object_path_point_is_bezier_active(&payload->points[0])) {
        int32_t in_x = payload->points[0].x + payload->points[0].handle_in_dx;
        int32_t in_y = payload->points[0].y + payload->points[0].handle_in_dy;
        int32_t out_x = payload->points[0].x + payload->points[0].handle_out_dx;
        int32_t out_y = payload->points[0].y + payload->points[0].handle_out_dy;
        if (in_x < min_x) {
            min_x = in_x;
        }
        if (in_x > max_x) {
            max_x = in_x;
        }
        if (in_y < min_y) {
            min_y = in_y;
        }
        if (in_y > max_y) {
            max_y = in_y;
        }
        if (out_x < min_x) {
            min_x = out_x;
        }
        if (out_x > max_x) {
            max_x = out_x;
        }
        if (out_y < min_y) {
            min_y = out_y;
        }
        if (out_y > max_y) {
            max_y = out_y;
        }
    }
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
        if (drawing_program_object_path_point_is_bezier_active(&payload->points[i])) {
            int32_t in_x = payload->points[i].x + payload->points[i].handle_in_dx;
            int32_t in_y = payload->points[i].y + payload->points[i].handle_in_dy;
            int32_t out_x = payload->points[i].x + payload->points[i].handle_out_dx;
            int32_t out_y = payload->points[i].y + payload->points[i].handle_out_dy;
            if (in_x < min_x) {
                min_x = in_x;
            }
            if (in_x > max_x) {
                max_x = in_x;
            }
            if (in_y < min_y) {
                min_y = in_y;
            }
            if (in_y > max_y) {
                max_y = in_y;
            }
            if (out_x < min_x) {
                min_x = out_x;
            }
            if (out_x > max_x) {
                max_x = out_x;
            }
            if (out_y < min_y) {
                min_y = out_y;
            }
            if (out_y > max_y) {
                max_y = out_y;
            }
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

int drawing_program_object_path_point_is_bezier_active(const DrawingProgramPathPoint *point) {
    if (!point || !point->bezier_enabled) {
        return 0;
    }
    return (point->handle_in_dx != 0 || point->handle_in_dy != 0 || point->handle_out_dx != 0 || point->handle_out_dy != 0)
               ? 1
               : 0;
}

static void object_path_segment_control_points(const DrawingProgramPathPoint *a,
                                               const DrawingProgramPathPoint *b,
                                               double *out_x0,
                                               double *out_y0,
                                               double *out_x1,
                                               double *out_y1,
                                               double *out_x2,
                                               double *out_y2,
                                               double *out_x3,
                                               double *out_y3) {
    double x0 = 0.0;
    double y0 = 0.0;
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;
    double x3 = 0.0;
    double y3 = 0.0;
    if (!a || !b) {
        return;
    }
    x0 = (double)a->x + 0.5;
    y0 = (double)a->y + 0.5;
    x3 = (double)b->x + 0.5;
    y3 = (double)b->y + 0.5;
    x1 = x0;
    y1 = y0;
    x2 = x3;
    y2 = y3;
    if (drawing_program_object_path_point_is_bezier_active(a)) {
        x1 += (double)a->handle_out_dx;
        y1 += (double)a->handle_out_dy;
    }
    if (drawing_program_object_path_point_is_bezier_active(b)) {
        x2 += (double)b->handle_in_dx;
        y2 += (double)b->handle_in_dy;
    }
    if (out_x0) {
        *out_x0 = x0;
    }
    if (out_y0) {
        *out_y0 = y0;
    }
    if (out_x1) {
        *out_x1 = x1;
    }
    if (out_y1) {
        *out_y1 = y1;
    }
    if (out_x2) {
        *out_x2 = x2;
    }
    if (out_y2) {
        *out_y2 = y2;
    }
    if (out_x3) {
        *out_x3 = x3;
    }
    if (out_y3) {
        *out_y3 = y3;
    }
}

uint32_t drawing_program_object_path_segment_curve_steps(const DrawingProgramPathPoint *a,
                                                         const DrawingProgramPathPoint *b) {
    double x0 = 0.0;
    double y0 = 0.0;
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;
    double x3 = 0.0;
    double y3 = 0.0;
    double chord = 0.0;
    double net = 0.0;
    double scale = 0.0;
    uint32_t steps = 1u;
    if (!a || !b) {
        return 1u;
    }
    if (!drawing_program_object_path_point_is_bezier_active(a) &&
        !drawing_program_object_path_point_is_bezier_active(b)) {
        return 1u;
    }
    object_path_segment_control_points(a, b, &x0, &y0, &x1, &y1, &x2, &y2, &x3, &y3);
    chord = hypot(x3 - x0, y3 - y0);
    net = hypot(x1 - x0, y1 - y0) + hypot(x2 - x1, y2 - y1) + hypot(x3 - x2, y3 - y2);
    scale = (net > chord) ? net : chord;
    steps = (uint32_t)ceil(scale / 6.0);
    if (steps < 8u) {
        steps = 8u;
    }
    if (steps > 24u) {
        steps = 24u;
    }
    return steps;
}

void drawing_program_object_path_segment_eval(const DrawingProgramPathPoint *a,
                                              const DrawingProgramPathPoint *b,
                                              double t,
                                              double *out_x,
                                              double *out_y) {
    double x0 = 0.0;
    double y0 = 0.0;
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;
    double x3 = 0.0;
    double y3 = 0.0;
    double omt;
    if (!a || !b) {
        return;
    }
    if (t < 0.0) {
        t = 0.0;
    } else if (t > 1.0) {
        t = 1.0;
    }
    object_path_segment_control_points(a, b, &x0, &y0, &x1, &y1, &x2, &y2, &x3, &y3);
    omt = 1.0 - t;
    if (out_x) {
        *out_x = (omt * omt * omt * x0) + (3.0 * omt * omt * t * x1) + (3.0 * omt * t * t * x2) + (t * t * t * x3);
    }
    if (out_y) {
        *out_y = (omt * omt * omt * y0) + (3.0 * omt * omt * t * y1) + (3.0 * omt * t * t * y2) + (t * t * t * y3);
    }
}

void drawing_program_object_path_project_on_segment(double px,
                                                    double py,
                                                    double x0,
                                                    double y0,
                                                    double x1,
                                                    double y1,
                                                    double *out_proj_x,
                                                    double *out_proj_y) {
    double dx = x1 - x0;
    double dy = y1 - y0;
    double len_sq = (dx * dx) + (dy * dy);
    double t;
    if (len_sq <= 0.0) {
        if (out_proj_x) {
            *out_proj_x = x0;
        }
        if (out_proj_y) {
            *out_proj_y = y0;
        }
        return;
    }
    t = (((px - x0) * dx) + ((py - y0) * dy)) / len_sq;
    if (t < 0.0) {
        t = 0.0;
    } else if (t > 1.0) {
        t = 1.0;
    }
    if (out_proj_x) {
        *out_proj_x = x0 + (t * dx);
    }
    if (out_proj_y) {
        *out_proj_y = y0 + (t * dy);
    }
}

double drawing_program_object_path_segment_distance_sq(double px,
                                                       double py,
                                                       double x0,
                                                       double y0,
                                                       double x1,
                                                       double y1) {
    double proj_x = 0.0;
    double proj_y = 0.0;
    double rx;
    double ry;
    drawing_program_object_path_project_on_segment(px, py, x0, y0, x1, y1, &proj_x, &proj_y);
    rx = px - proj_x;
    ry = py - proj_y;
    return (rx * rx) + (ry * ry);
}

double drawing_program_object_path_point_segment_distance_sq(const DrawingProgramPathPoint *a,
                                                             const DrawingProgramPathPoint *b,
                                                             double px,
                                                             double py) {
    uint32_t steps;
    uint32_t i;
    double best_sq = DBL_MAX;
    double x0 = 0.0;
    double y0 = 0.0;
    if (!a || !b) {
        return DBL_MAX;
    }
    steps = drawing_program_object_path_segment_curve_steps(a, b);
    if (steps <= 1u) {
        return drawing_program_object_path_segment_distance_sq(
            px, py, (double)a->x + 0.5, (double)a->y + 0.5, (double)b->x + 0.5, (double)b->y + 0.5);
    }
    drawing_program_object_path_segment_eval(a, b, 0.0, &x0, &y0);
    for (i = 1u; i <= steps; ++i) {
        double x_next = 0.0;
        double y_next = 0.0;
        double d_sq;
        drawing_program_object_path_segment_eval(a, b, (double)i / (double)steps, &x_next, &y_next);
        d_sq = drawing_program_object_path_segment_distance_sq(px, py, x0, y0, x_next, y_next);
        if (d_sq < best_sq) {
            best_sq = d_sq;
        }
        x0 = x_next;
        y0 = y_next;
    }
    return best_sq;
}

void drawing_program_object_path_project_on_point_segment(const DrawingProgramPathPoint *a,
                                                          const DrawingProgramPathPoint *b,
                                                          double px,
                                                          double py,
                                                          double *out_proj_x,
                                                          double *out_proj_y) {
    uint32_t steps;
    uint32_t i;
    double best_sq = DBL_MAX;
    double best_x = 0.0;
    double best_y = 0.0;
    double x0 = 0.0;
    double y0 = 0.0;
    if (!a || !b) {
        return;
    }
    steps = drawing_program_object_path_segment_curve_steps(a, b);
    if (steps <= 1u) {
        drawing_program_object_path_project_on_segment(
            px, py, (double)a->x + 0.5, (double)a->y + 0.5, (double)b->x + 0.5, (double)b->y + 0.5, out_proj_x, out_proj_y);
        return;
    }
    drawing_program_object_path_segment_eval(a, b, 0.0, &x0, &y0);
    for (i = 1u; i <= steps; ++i) {
        double x_next = 0.0;
        double y_next = 0.0;
        double proj_x = 0.0;
        double proj_y = 0.0;
        double d_sq;
        drawing_program_object_path_segment_eval(a, b, (double)i / (double)steps, &x_next, &y_next);
        drawing_program_object_path_project_on_segment(px, py, x0, y0, x_next, y_next, &proj_x, &proj_y);
        d_sq = drawing_program_object_path_segment_distance_sq(px, py, x0, y0, x_next, y_next);
        if (d_sq < best_sq) {
            best_sq = d_sq;
            best_x = proj_x;
            best_y = proj_y;
        }
        x0 = x_next;
        y0 = y_next;
    }
    if (out_proj_x) {
        *out_proj_x = best_x;
    }
    if (out_proj_y) {
        *out_proj_y = best_y;
    }
}

CoreResult drawing_program_object_path_flatten_points(const DrawingProgramObjectRecord *object,
                                                      int32_t delta_x,
                                                      int32_t delta_y,
                                                      double *out_xy,
                                                      uint32_t max_points,
                                                      uint32_t *out_point_count) {
    uint32_t point_count;
    uint32_t segment_count;
    uint32_t seg_i;
    uint32_t write_count = 0u;
    if (!object || !out_xy || !out_point_count) {
        return object_geometry_invalid("invalid path flatten request");
    }
    point_count = (uint32_t)object->path_point_count;
    if (object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
        point_count < 2u || point_count > DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS || max_points == 0u) {
        return object_geometry_invalid("invalid path flatten target");
    }
    segment_count = point_count - 1u;
    if (object->path_closed) {
        segment_count += 1u;
    }
    out_xy[0] = (double)(object->path_points[0].x + delta_x) + 0.5;
    out_xy[1] = (double)(object->path_points[0].y + delta_y) + 0.5;
    write_count = 1u;
    for (seg_i = 0u; seg_i < segment_count; ++seg_i) {
        uint32_t a = seg_i;
        uint32_t b = (seg_i + 1u < point_count) ? (seg_i + 1u) : 0u;
        DrawingProgramPathPoint pa = object->path_points[a];
        DrawingProgramPathPoint pb = object->path_points[b];
        uint32_t steps;
        uint32_t step_i;
        pa.x += delta_x;
        pa.y += delta_y;
        pb.x += delta_x;
        pb.y += delta_y;
        steps = drawing_program_object_path_segment_curve_steps(&pa, &pb);
        if (steps < 1u) {
            steps = 1u;
        }
        for (step_i = 1u; step_i <= steps; ++step_i) {
            double x = 0.0;
            double y = 0.0;
            if (object->path_closed && b == 0u && step_i == steps) {
                continue;
            }
            if (write_count >= max_points) {
                return object_geometry_invalid("flattened path exceeds point capacity");
            }
            if (steps == 1u) {
                x = (double)pb.x + 0.5;
                y = (double)pb.y + 0.5;
            } else {
                drawing_program_object_path_segment_eval(&pa, &pb, (double)step_i / (double)steps, &x, &y);
            }
            out_xy[write_count * 2u] = x;
            out_xy[(write_count * 2u) + 1u] = y;
            write_count += 1u;
        }
    }
    *out_point_count = write_count;
    return core_result_ok();
}

int drawing_program_object_path_flattened_contains_fill(const double *xy,
                                                        uint32_t point_count,
                                                        double px,
                                                        double py) {
    uint32_t i;
    uint32_t j;
    int inside = 0;
    if (!xy || point_count < 3u) {
        return 0;
    }
    for (i = 0u, j = point_count - 1u; i < point_count; j = i++) {
        const double xi = xy[i * 2u];
        const double yi = xy[(i * 2u) + 1u];
        const double xj = xy[j * 2u];
        const double yj = xy[(j * 2u) + 1u];
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

int drawing_program_object_path_flattened_contains_outline(const double *xy,
                                                           uint32_t point_count,
                                                           uint8_t closed,
                                                           double px,
                                                           double py,
                                                           double threshold_sq) {
    uint32_t i;
    if (!xy || point_count < 2u) {
        return 0;
    }
    for (i = 0u; i + 1u < point_count; ++i) {
        double d_sq = drawing_program_object_path_segment_distance_sq(
            px, py, xy[i * 2u], xy[(i * 2u) + 1u], xy[(i + 1u) * 2u], xy[((i + 1u) * 2u) + 1u]);
        if (d_sq <= threshold_sq) {
            return 1;
        }
    }
    if (closed && point_count > 2u) {
        double d_sq = drawing_program_object_path_segment_distance_sq(
            px, py, xy[(point_count - 1u) * 2u], xy[((point_count - 1u) * 2u) + 1u], xy[0], xy[1]);
        if (d_sq <= threshold_sq) {
            return 1;
        }
    }
    return 0;
}

static int object_path_contains_fill(const DrawingProgramObjectRecord *object, double px, double py) {
    double flat_xy[DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS * 24u * 2u];
    uint32_t flat_count = 0u;
    CoreResult result;
    if (!object || object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH || !object->path_closed) {
        return 0;
    }
    result = drawing_program_object_path_flatten_points(
        object, 0, 0, flat_xy, DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS * 24u, &flat_count);
    if (result.code != CORE_OK) {
        return 0;
    }
    return drawing_program_object_path_flattened_contains_fill(flat_xy, flat_count, px, py);
}

static int object_path_contains_outline(const DrawingProgramObjectRecord *object, double px, double py) {
    double flat_xy[DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS * 24u * 2u];
    uint32_t flat_count = 0u;
    double half_width;
    const double pick_radius = 1.35;
    double threshold_sq;
    CoreResult result;
    if (!object || object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH) {
        return 0;
    }
    result = drawing_program_object_path_flatten_points(
        object, 0, 0, flat_xy, DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS * 24u, &flat_count);
    if (result.code != CORE_OK) {
        return 0;
    }
    half_width = ((double)(object->stroke_width == 0u ? 1u : object->stroke_width)) * 0.5;
    if (half_width < 0.6) {
        half_width = 0.6;
    }
    half_width += pick_radius;
    threshold_sq = half_width * half_width;
    return drawing_program_object_path_flattened_contains_outline(
        flat_xy, flat_count, object->path_closed, px, py, threshold_sq);
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
