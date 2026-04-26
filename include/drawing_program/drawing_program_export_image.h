#ifndef DRAWING_PROGRAM_EXPORT_IMAGE_H
#define DRAWING_PROGRAM_EXPORT_IMAGE_H

#include <stddef.h>
#include <stdint.h>

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

CoreResult drawing_program_export_image_default_basename(const struct DrawingProgramAppContext *ctx,
                                                         char *out_name,
                                                         size_t out_cap);
CoreResult drawing_program_export_image_compose_document_rgba(const struct DrawingProgramAppContext *ctx,
                                                              uint8_t **out_rgba,
                                                              uint32_t *out_width,
                                                              uint32_t *out_height);
CoreResult drawing_program_export_image_write_png_rgba(const char *path,
                                                       const uint8_t *rgba,
                                                       uint32_t width,
                                                       uint32_t height);
CoreResult drawing_program_export_image_scale_rgba_bilinear(const uint8_t *src_rgba,
                                                            uint32_t src_width,
                                                            uint32_t src_height,
                                                            uint32_t dst_width,
                                                            uint32_t dst_height,
                                                            uint8_t **out_rgba);
CoreResult drawing_program_export_image_build_macos_icon_rgba(const uint8_t *src_rgba,
                                                              uint32_t src_width,
                                                              uint32_t src_height,
                                                              uint32_t dst_size,
                                                              uint8_t **out_rgba);

#ifdef __cplusplus
}
#endif

#endif
