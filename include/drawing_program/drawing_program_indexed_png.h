#ifndef DRAWING_PROGRAM_INDEXED_PNG_H
#define DRAWING_PROGRAM_INDEXED_PNG_H

#include <stdint.h>

#include "core_authored_texture.h"
#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DrawingProgramIndexedPngImage {
    uint32_t width;
    uint32_t height;
    uint32_t palette_count;
    uint8_t *indices;
    CoreAuthoredTextureRgba8 palette[256];
} DrawingProgramIndexedPngImage;

CoreResult drawing_program_indexed_png_write(
    const char *path,
    const uint8_t *indices,
    uint32_t width,
    uint32_t height,
    const CoreAuthoredTextureRgba8 *palette,
    uint32_t palette_count);
CoreResult drawing_program_indexed_png_read(
    const char *path,
    DrawingProgramIndexedPngImage *out_image);
void drawing_program_indexed_png_dispose(DrawingProgramIndexedPngImage *image);

#ifdef __cplusplus
}
#endif

#endif
