#ifndef DRAWING_PROGRAM_OBJECT_GEOMETRY_H
#define DRAWING_PROGRAM_OBJECT_GEOMETRY_H

#include "core_base.h"
#include "drawing_program/drawing_program_object_store.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawing_program_object_path_payload_clear(DrawingProgramObjectRecord *object);
int drawing_program_object_path_payload_valid(const DrawingProgramObjectRecord *object);
CoreResult drawing_program_object_path_payload_bounds(const DrawingProgramPathPayload *payload,
                                                      int32_t *out_origin_x,
                                                      int32_t *out_origin_y,
                                                      uint32_t *out_width,
                                                      uint32_t *out_height);
int drawing_program_object_bounds_contains(const DrawingProgramObjectRecord *object,
                                           uint32_t sample_x,
                                           uint32_t sample_y);
int drawing_program_object_layer_allows_interaction(const DrawingProgramDocument *document,
                                                    uint32_t layer_id);
int drawing_program_object_ellipse_contains(const DrawingProgramObjectRecord *object,
                                            uint32_t sample_x,
                                            uint32_t sample_y);
int drawing_program_object_path_point_is_bezier_active(const DrawingProgramPathPoint *point);
uint32_t drawing_program_object_path_segment_curve_steps(const DrawingProgramPathPoint *a,
                                                         const DrawingProgramPathPoint *b);
void drawing_program_object_path_segment_eval(const DrawingProgramPathPoint *a,
                                              const DrawingProgramPathPoint *b,
                                              double t,
                                              double *out_x,
                                              double *out_y);
double drawing_program_object_path_segment_distance_sq(double px,
                                                       double py,
                                                       double x0,
                                                       double y0,
                                                       double x1,
                                                       double y1);
double drawing_program_object_path_point_segment_distance_sq(const DrawingProgramPathPoint *a,
                                                             const DrawingProgramPathPoint *b,
                                                             double px,
                                                             double py);
void drawing_program_object_path_project_on_segment(double px,
                                                    double py,
                                                    double x0,
                                                    double y0,
                                                    double x1,
                                                    double y1,
                                                    double *out_proj_x,
                                                    double *out_proj_y);
void drawing_program_object_path_project_on_point_segment(const DrawingProgramPathPoint *a,
                                                          const DrawingProgramPathPoint *b,
                                                          double px,
                                                          double py,
                                                          double *out_proj_x,
                                                          double *out_proj_y);
CoreResult drawing_program_object_path_flatten_points(const DrawingProgramObjectRecord *object,
                                                      int32_t delta_x,
                                                      int32_t delta_y,
                                                      double *out_xy,
                                                      uint32_t max_points,
                                                      uint32_t *out_point_count);
int drawing_program_object_path_flattened_contains_fill(const double *xy,
                                                        uint32_t point_count,
                                                        double px,
                                                        double py);
int drawing_program_object_path_flattened_contains_outline(const double *xy,
                                                           uint32_t point_count,
                                                           uint8_t closed,
                                                           double px,
                                                           double py,
                                                           double threshold_sq);
int drawing_program_object_path_contains(const DrawingProgramObjectRecord *object,
                                         uint32_t sample_x,
                                         uint32_t sample_y);

#ifdef __cplusplus
}
#endif

#endif
