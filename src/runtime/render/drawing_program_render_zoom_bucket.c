#include "drawing_program/drawing_program_render_zoom_bucket.h"

static const float g_render_zoom_bucket_anchors[] = { 0.25f, 0.50f, 1.00f, 2.00f, 4.00f };

float drawing_program_render_zoom_bucket_anchor(float zoom) {
    float best_anchor = g_render_zoom_bucket_anchors[0];
    float best_distance;
    uint32_t i;
    if (zoom <= 0.0f) {
        return best_anchor;
    }
    best_distance = (zoom > best_anchor) ? (zoom - best_anchor) : (best_anchor - zoom);
    for (i = 1u; i < (uint32_t)(sizeof(g_render_zoom_bucket_anchors) / sizeof(g_render_zoom_bucket_anchors[0])); ++i) {
        float anchor = g_render_zoom_bucket_anchors[i];
        float distance = (zoom > anchor) ? (zoom - anchor) : (anchor - zoom);
        if (distance < best_distance) {
            best_anchor = anchor;
            best_distance = distance;
        }
    }
    return best_anchor;
}

uint32_t drawing_program_render_zoom_bucket_percent(float zoom) {
    float anchor = drawing_program_render_zoom_bucket_anchor(zoom);
    return (uint32_t)(anchor * 100.0f + 0.5f);
}
