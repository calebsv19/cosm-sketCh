#ifndef DRAWING_PROGRAM_RENDER_ZOOM_BUCKET_H
#define DRAWING_PROGRAM_RENDER_ZOOM_BUCKET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

float drawing_program_render_zoom_bucket_anchor(float zoom);
uint32_t drawing_program_render_zoom_bucket_percent(float zoom);

#ifdef __cplusplus
}
#endif

#endif
