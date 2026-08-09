#include "drawing_program/drawing_program_indexed_png.h"

#include <png.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CoreResult indexed_png_invalid(const char *message) {
    return (CoreResult){ CORE_ERR_INVALID_ARG, message };
}

static CoreResult indexed_png_io(const char *message) {
    return (CoreResult){ CORE_ERR_IO, message };
}

CoreResult drawing_program_indexed_png_write(
    const char *path,
    const uint8_t *indices,
    uint32_t width,
    uint32_t height,
    const CoreAuthoredTextureRgba8 *palette,
    uint32_t palette_count) {
    FILE *file;
    png_structp png_ptr;
    png_infop info_ptr;
    png_color colors[256];
    png_byte alpha[256];
    uint32_t i;
    if (!path || !indices || width == 0u || height == 0u || !palette ||
        palette_count == 0u || palette_count > 256u) {
        return indexed_png_invalid("invalid indexed PNG write request");
    }
    file = fopen(path, "wb");
    if (!file) return indexed_png_io("failed to open indexed PNG output");
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!png_ptr) { fclose(file); return indexed_png_io("failed to allocate indexed PNG writer"); }
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, 0);
        fclose(file);
        return indexed_png_io("failed to allocate indexed PNG info");
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(file);
        return indexed_png_io("indexed PNG writer failure");
    }
    for (i = 0u; i < palette_count; ++i) {
        colors[i].red = palette[i].r;
        colors[i].green = palette[i].g;
        colors[i].blue = palette[i].b;
        alpha[i] = palette[i].a;
    }
    png_init_io(png_ptr, file);
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_PALETTE,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_PLTE(png_ptr, info_ptr, colors, (int)palette_count);
    png_set_tRNS(png_ptr, info_ptr, alpha, (int)palette_count, 0);
    png_write_info(png_ptr, info_ptr);
    for (i = 0u; i < height; ++i) {
        png_write_row(png_ptr, (png_const_bytep)(indices + (size_t)i * width));
    }
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    if (fclose(file) != 0) return indexed_png_io("failed to close indexed PNG output");
    return core_result_ok();
}

CoreResult drawing_program_indexed_png_read(
    const char *path,
    DrawingProgramIndexedPngImage *out_image) {
    FILE *file;
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width;
    png_uint_32 height;
    int bit_depth;
    int color_type;
    png_colorp colors = 0;
    int color_count = 0;
    png_bytep alpha = 0;
    int alpha_count = 0;
    png_color_16p transparent_color = 0;
    uint32_t y;
    if (!path || !out_image) return indexed_png_invalid("invalid indexed PNG read request");
    memset(out_image, 0, sizeof(*out_image));
    file = fopen(path, "rb");
    if (!file) return indexed_png_io("failed to open indexed PNG input");
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!png_ptr) { fclose(file); return indexed_png_io("failed to allocate indexed PNG reader"); }
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, 0, 0);
        fclose(file);
        return indexed_png_io("failed to allocate indexed PNG read info");
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        fclose(file);
        drawing_program_indexed_png_dispose(out_image);
        return indexed_png_io("indexed PNG reader failure");
    }
    png_init_io(png_ptr, file);
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, 0, 0, 0);
    if (width == 0u || height == 0u || bit_depth != 8 || color_type != PNG_COLOR_TYPE_PALETTE ||
        !png_get_PLTE(png_ptr, info_ptr, &colors, &color_count) || color_count <= 0 || color_count > 256 ||
        (uint64_t)width * (uint64_t)height > (uint64_t)SIZE_MAX) {
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        fclose(file);
        return indexed_png_invalid("PNG is not an 8-bit indexed atlas");
    }
    (void)png_get_tRNS(png_ptr, info_ptr, &alpha, &alpha_count, &transparent_color);
    out_image->indices = (uint8_t *)malloc((size_t)width * (size_t)height);
    if (!out_image->indices) {
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        fclose(file);
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate indexed PNG pixels" };
    }
    out_image->width = (uint32_t)width;
    out_image->height = (uint32_t)height;
    out_image->palette_count = (uint32_t)color_count;
    for (int i = 0; i < color_count; ++i) {
        out_image->palette[i].r = colors[i].red;
        out_image->palette[i].g = colors[i].green;
        out_image->palette[i].b = colors[i].blue;
        out_image->palette[i].a = i < alpha_count ? alpha[i] : 255u;
    }
    png_read_update_info(png_ptr, info_ptr);
    if (png_get_rowbytes(png_ptr, info_ptr) != width) {
        png_error(png_ptr, "unexpected indexed PNG row width");
    }
    for (y = 0u; y < (uint32_t)height; ++y) {
        png_read_row(png_ptr, out_image->indices + (size_t)y * width, 0);
    }
    png_read_end(png_ptr, info_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    fclose(file);
    return core_result_ok();
}

void drawing_program_indexed_png_dispose(DrawingProgramIndexedPngImage *image) {
    if (!image) return;
    free(image->indices);
    memset(image, 0, sizeof(*image));
}
