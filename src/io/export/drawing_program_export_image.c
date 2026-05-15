#include "drawing_program/drawing_program_export_image.h"

#include <errno.h>
#include <math.h>
#include <png.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_object_rasterize.h"
#include "drawing_program/drawing_program_render_domain.h"

static CoreResult export_image_invalid(const char *message) {
    return (CoreResult){ CORE_ERR_INVALID_ARG, message };
}

static CoreResult export_image_io_error(const char *message) {
    return (CoreResult){ CORE_ERR_IO, message };
}

static const double k_export_image_macos_icon_safe_inset_fraction = 84.0 / 1024.0;
static const double k_export_image_macos_icon_superellipse_exponent = 5.0;

static CoreResult export_image_mkdirs_if_needed(const char *dir_path) {
    char buffer[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    size_t i;
    size_t len;
    if (!dir_path || dir_path[0] == '\0') {
        return core_result_ok();
    }
    len = strlen(dir_path);
    if (len >= sizeof(buffer)) {
        return export_image_invalid("export directory path too long");
    }
    (void)snprintf(buffer, sizeof(buffer), "%s", dir_path);
    for (i = 1u; i < len; ++i) {
        if (buffer[i] != '/') {
            continue;
        }
        buffer[i] = '\0';
        if (buffer[0] != '\0') {
            if (mkdir(buffer, 0775) != 0 && errno != EEXIST) {
                return export_image_io_error("failed to create export directory segment");
            }
        }
        buffer[i] = '/';
    }
    if (mkdir(buffer, 0775) != 0 && errno != EEXIST) {
        return export_image_io_error("failed to create export directory");
    }
    return core_result_ok();
}

static CoreResult export_image_ensure_parent_dir(const char *file_path) {
    char buffer[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    char *slash = 0;
    if (!file_path || file_path[0] == '\0') {
        return core_result_ok();
    }
    if (strlen(file_path) >= sizeof(buffer)) {
        return export_image_invalid("export file path too long");
    }
    (void)snprintf(buffer, sizeof(buffer), "%s", file_path);
    slash = strrchr(buffer, '/');
    if (!slash) {
        return core_result_ok();
    }
    *slash = '\0';
    if (buffer[0] == '\0') {
        return core_result_ok();
    }
    return export_image_mkdirs_if_needed(buffer);
}

static void export_image_build_layer_opacity(const struct DrawingProgramAppContext *ctx,
                                             uint8_t out_opacity[DRAWING_PROGRAM_MAX_LAYERS]) {
    uint32_t i;
    if (!ctx || !out_opacity) {
        return;
    }
    for (i = 0u; i < ctx->document.layer_count && i < DRAWING_PROGRAM_MAX_LAYERS; ++i) {
        uint8_t opacity = 100u;
        uint32_t j;
        for (j = 0u; j < ctx->ui.layer_opacity_entry_count; ++j) {
            if (ctx->ui.layer_opacity_layer_ids[j] == ctx->document.layers[i].layer_id) {
                opacity = ctx->ui.layer_opacity_values[j];
                break;
            }
        }
        out_opacity[i] = opacity;
    }
}

static CoreResult export_image_prepare_scratch_layers(const struct DrawingProgramAppContext *ctx,
                                                      DrawingProgramDocument *scratch_document,
                                                      DrawingProgramLayerRasterStore *scratch_rasters) {
    uint32_t i;
    CoreResult result;
    if (!ctx || !scratch_document || !scratch_rasters) {
        return export_image_invalid("invalid scratch layer preparation request");
    }
    memset(scratch_document, 0, sizeof(*scratch_document));
    memset(scratch_rasters, 0, sizeof(*scratch_rasters));
    *scratch_document = ctx->document;
    result = drawing_program_layer_raster_store_init_from_document(scratch_rasters, scratch_document);
    if (result.code != CORE_OK) {
        drawing_program_layer_raster_store_dispose(scratch_rasters);
        return result;
    }
    if (scratch_rasters->slot_samples && scratch_rasters->sample_count > 0u && scratch_rasters->slot_capacity > 0u) {
        uint32_t i;
        DrawingProgramRasterSample empty_sample = drawing_program_color_eraser_value();
        for (i = 0u; i < scratch_rasters->sample_count * scratch_rasters->slot_capacity; ++i) {
            scratch_rasters->slot_samples[i] = empty_sample;
        }
    }
    for (i = 0u; i < scratch_document->layer_count; ++i) {
        const DrawingProgramRasterSample *layer_samples = 0;
        uint32_t layer_sample_count = 0u;
        result = drawing_program_layer_raster_store_export_layer(&ctx->layer_rasters,
                                                                 scratch_document->layers[i].layer_id,
                                                                 &layer_samples,
                                                                 &layer_sample_count);
        if (result.code != CORE_OK || !layer_samples || layer_sample_count != scratch_document->raster_sample_count) {
            continue;
        }
        result = drawing_program_layer_raster_store_import_layer(scratch_rasters,
                                                                 scratch_document->layers[i].layer_id,
                                                                 layer_samples,
                                                                 layer_sample_count);
        if (result.code != CORE_OK) {
            drawing_program_layer_raster_store_dispose(scratch_rasters);
            return result;
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_export_image_default_basename(const struct DrawingProgramAppContext *ctx,
                                                         char *out_name,
                                                         size_t out_cap) {
    const char *project_path = 0;
    const char *base_name = 0;
    size_t len = 0u;
    if (!out_name || out_cap == 0u) {
        return export_image_invalid("invalid export basename request");
    }
    project_path = (ctx && ctx->session.project_path) ? ctx->session.project_path : 0;
    if (!project_path || project_path[0] == '\0') {
        (void)snprintf(out_name, out_cap, "canvas_export");
        return core_result_ok();
    }
    base_name = strrchr(project_path, '/');
    base_name = base_name ? (base_name + 1) : project_path;
    (void)snprintf(out_name, out_cap, "%s", base_name);
    len = strlen(out_name);
    if (len > 5u && strcmp(out_name + len - 5u, ".pack") == 0) {
        out_name[len - 5u] = '\0';
    }
    return core_result_ok();
}

CoreResult drawing_program_export_image_compose_document_rgba(const struct DrawingProgramAppContext *ctx,
                                                              uint8_t **out_rgba,
                                                              uint32_t *out_width,
                                                              uint32_t *out_height) {
    DrawingProgramDocument *scratch_document = 0;
    DrawingProgramLayerRasterStore scratch_rasters;
    DrawingProgramHistory *scratch_history = 0;
    DrawingProgramRasterSample *samples = 0;
    uint8_t *rgba = 0;
    uint8_t layer_opacity[DRAWING_PROGRAM_MAX_LAYERS] = { 0 };
    uint64_t rgba_capacity = 0u;
    uint32_t ignored_rasterized_count = 0u;
    uint32_t i;
    CoreResult result;
    if (!ctx || !out_rgba || !out_width || !out_height) {
        return export_image_invalid("invalid RGBA compose request");
    }
    *out_rgba = 0;
    *out_width = 0u;
    *out_height = 0u;
    memset(&scratch_rasters, 0, sizeof(scratch_rasters));
    scratch_document = core_alloc(sizeof(*scratch_document));
    scratch_history = core_alloc(sizeof(*scratch_history));
    if (!scratch_document || !scratch_history) {
        core_free(scratch_history);
        core_free(scratch_document);
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate export scratch state" };
    }
    memset(scratch_document, 0, sizeof(*scratch_document));
    drawing_program_history_init(scratch_history);
    result = export_image_prepare_scratch_layers(ctx, scratch_document, &scratch_rasters);
    if (result.code != CORE_OK) {
        core_free(scratch_history);
        core_free(scratch_document);
        return result;
    }
    samples = core_alloc((size_t)ctx->document.raster_sample_count * sizeof(*samples));
    if (!samples) {
        drawing_program_layer_raster_store_dispose(&scratch_rasters);
        core_free(scratch_history);
        core_free(scratch_document);
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate export sample buffer" };
    }
    rgba_capacity = (uint64_t)ctx->document.raster_sample_count * 4u;
    if (rgba_capacity == 0u || rgba_capacity > SIZE_MAX) {
        drawing_program_layer_raster_store_dispose(&scratch_rasters);
        core_free(samples);
        core_free(scratch_history);
        core_free(scratch_document);
        return export_image_invalid("invalid export pixel capacity");
    }
    rgba = core_alloc((size_t)rgba_capacity);
    if (!rgba) {
        drawing_program_layer_raster_store_dispose(&scratch_rasters);
        core_free(samples);
        core_free(scratch_history);
        core_free(scratch_document);
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate export RGBA buffer" };
    }
    result = drawing_program_object_rasterize_visible_to_layers(scratch_document,
                                                                &scratch_rasters,
                                                                scratch_history,
                                                                &ctx->object_store,
                                                                &ignored_rasterized_count);
    if (result.code != CORE_OK) {
        core_free(rgba);
        core_free(samples);
        drawing_program_layer_raster_store_dispose(&scratch_rasters);
        core_free(scratch_history);
        core_free(scratch_document);
        return result;
    }
    export_image_build_layer_opacity(ctx, layer_opacity);
    result = drawing_program_render_compose_visible_samples_with_layer_opacity(scratch_document,
                                                                               &scratch_rasters,
                                                                               layer_opacity,
                                                                               scratch_document->layer_count,
                                                                               samples,
                                                                               scratch_document->raster_sample_count);
    if (result.code != CORE_OK) {
        core_free(rgba);
        core_free(samples);
        drawing_program_layer_raster_store_dispose(&scratch_rasters);
        core_free(scratch_history);
        core_free(scratch_document);
        return result;
    }
    for (i = 0u; i < scratch_document->raster_sample_count; ++i) {
        uint8_t r = 0u;
        uint8_t g = 0u;
        uint8_t b = 0u;
        uint8_t a = 255u;
        drawing_program_color_rgba_from_sample(samples[i], &r, &g, &b, &a);
        rgba[(size_t)i * 4u + 0u] = r;
        rgba[(size_t)i * 4u + 1u] = g;
        rgba[(size_t)i * 4u + 2u] = b;
        rgba[(size_t)i * 4u + 3u] = a;
    }
    core_free(samples);
    drawing_program_layer_raster_store_dispose(&scratch_rasters);
    *out_rgba = rgba;
    *out_width = scratch_document->raster_width;
    *out_height = scratch_document->raster_height;
    core_free(scratch_history);
    core_free(scratch_document);
    return core_result_ok();
}

CoreResult drawing_program_export_image_write_png_rgba(const char *path,
                                                       const uint8_t *rgba,
                                                       uint32_t width,
                                                       uint32_t height) {
    FILE *png_file = 0;
    png_structp png_ptr = 0;
    png_infop info_ptr = 0;
    uint32_t i;
    CoreResult result;
    if (!path || path[0] == '\0' || !rgba || width == 0u || height == 0u) {
        return export_image_invalid("invalid PNG write request");
    }
    result = export_image_ensure_parent_dir(path);
    if (result.code != CORE_OK) {
        return result;
    }
    png_file = fopen(path, "wb");
    if (!png_file) {
        return export_image_io_error("failed to open PNG output file");
    }
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!png_ptr) {
        (void)fclose(png_file);
        return export_image_io_error("failed to allocate PNG writer");
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, 0);
        (void)fclose(png_file);
        return export_image_io_error("failed to allocate PNG info");
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        (void)fclose(png_file);
        return export_image_io_error("PNG writer failure");
    }
    png_init_io(png_ptr, png_file);
    png_set_IHDR(png_ptr,
                 info_ptr,
                 width,
                 height,
                 8,
                 PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);
    for (i = 0u; i < height; ++i) {
        png_write_row(png_ptr, (png_const_bytep)(rgba + ((size_t)i * (size_t)width * 4u)));
    }
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    if (fclose(png_file) != 0) {
        return export_image_io_error("failed to close PNG output file");
    }
    return core_result_ok();
}

CoreResult drawing_program_export_image_scale_rgba_bilinear(const uint8_t *src_rgba,
                                                            uint32_t src_width,
                                                            uint32_t src_height,
                                                            uint32_t dst_width,
                                                            uint32_t dst_height,
                                                            uint8_t **out_rgba) {
    uint8_t *dst_rgba = 0;
    uint64_t dst_capacity = 0u;
    uint32_t y;
    if (!src_rgba || !out_rgba || src_width == 0u || src_height == 0u || dst_width == 0u || dst_height == 0u) {
        return export_image_invalid("invalid RGBA scale request");
    }
    *out_rgba = 0;
    dst_capacity = (uint64_t)dst_width * (uint64_t)dst_height * 4u;
    if (dst_capacity == 0u || dst_capacity > SIZE_MAX) {
        return export_image_invalid("scaled image capacity too large");
    }
    dst_rgba = core_alloc((size_t)dst_capacity);
    if (!dst_rgba) {
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate scaled image buffer" };
    }
    for (y = 0u; y < dst_height; ++y) {
        uint32_t x;
        double src_y = (((double)y + 0.5) * (double)src_height / (double)dst_height) - 0.5;
        uint32_t y0;
        uint32_t y1;
        double fy;
        if (src_y < 0.0) {
            src_y = 0.0;
        }
        if (src_y > (double)(src_height - 1u)) {
            src_y = (double)(src_height - 1u);
        }
        y0 = (uint32_t)src_y;
        y1 = (y0 + 1u < src_height) ? (y0 + 1u) : y0;
        fy = src_y - (double)y0;
        for (x = 0u; x < dst_width; ++x) {
            double src_x = (((double)x + 0.5) * (double)src_width / (double)dst_width) - 0.5;
            uint32_t x0;
            uint32_t x1;
            double fx;
            double w00;
            double w10;
            double w01;
            double w11;
            const uint8_t *p00;
            const uint8_t *p10;
            const uint8_t *p01;
            const uint8_t *p11;
            double a;
            double pr;
            double pg;
            double pb;
            uint8_t *dst_px;
            if (src_x < 0.0) {
                src_x = 0.0;
            }
            if (src_x > (double)(src_width - 1u)) {
                src_x = (double)(src_width - 1u);
            }
            x0 = (uint32_t)src_x;
            x1 = (x0 + 1u < src_width) ? (x0 + 1u) : x0;
            fx = src_x - (double)x0;
            w00 = (1.0 - fx) * (1.0 - fy);
            w10 = fx * (1.0 - fy);
            w01 = (1.0 - fx) * fy;
            w11 = fx * fy;
            p00 = src_rgba + (((size_t)y0 * (size_t)src_width + (size_t)x0) * 4u);
            p10 = src_rgba + (((size_t)y0 * (size_t)src_width + (size_t)x1) * 4u);
            p01 = src_rgba + (((size_t)y1 * (size_t)src_width + (size_t)x0) * 4u);
            p11 = src_rgba + (((size_t)y1 * (size_t)src_width + (size_t)x1) * 4u);
            a = ((double)p00[3] * w00) + ((double)p10[3] * w10) + ((double)p01[3] * w01) + ((double)p11[3] * w11);
            pr = ((double)p00[0] * ((double)p00[3] / 255.0) * w00) +
                 ((double)p10[0] * ((double)p10[3] / 255.0) * w10) +
                 ((double)p01[0] * ((double)p01[3] / 255.0) * w01) +
                 ((double)p11[0] * ((double)p11[3] / 255.0) * w11);
            pg = ((double)p00[1] * ((double)p00[3] / 255.0) * w00) +
                 ((double)p10[1] * ((double)p10[3] / 255.0) * w10) +
                 ((double)p01[1] * ((double)p01[3] / 255.0) * w01) +
                 ((double)p11[1] * ((double)p11[3] / 255.0) * w11);
            pb = ((double)p00[2] * ((double)p00[3] / 255.0) * w00) +
                 ((double)p10[2] * ((double)p10[3] / 255.0) * w10) +
                 ((double)p01[2] * ((double)p01[3] / 255.0) * w01) +
                 ((double)p11[2] * ((double)p11[3] / 255.0) * w11);
            dst_px = dst_rgba + (((size_t)y * (size_t)dst_width + (size_t)x) * 4u);
            if (a <= 0.0) {
                dst_px[0] = 0u;
                dst_px[1] = 0u;
                dst_px[2] = 0u;
                dst_px[3] = 0u;
            } else {
                double alpha_norm = a / 255.0;
                dst_px[0] = (uint8_t)lrint((pr / alpha_norm));
                dst_px[1] = (uint8_t)lrint((pg / alpha_norm));
                dst_px[2] = (uint8_t)lrint((pb / alpha_norm));
                dst_px[3] = (uint8_t)lrint(a);
            }
        }
    }
    *out_rgba = dst_rgba;
    return core_result_ok();
}

CoreResult drawing_program_export_image_build_macos_icon_rgba(const uint8_t *src_rgba,
                                                              uint32_t src_width,
                                                              uint32_t src_height,
                                                              uint32_t dst_size,
                                                              uint8_t **out_rgba) {
    uint8_t *content_rgba = 0;
    uint8_t *icon_rgba = 0;
    uint64_t icon_capacity = 0u;
    uint32_t inset = 0u;
    uint32_t content_size = 0u;
    uint32_t offset = 0u;
    uint32_t y;
    CoreResult result;
    if (!src_rgba || !out_rgba || src_width == 0u || src_height == 0u || dst_size == 0u) {
        return export_image_invalid("invalid macOS icon RGBA request");
    }
    *out_rgba = 0;
    inset = (uint32_t)lrint((double)dst_size * k_export_image_macos_icon_safe_inset_fraction);
    if (inset * 2u >= dst_size) {
        inset = dst_size / 8u;
    }
    if (inset * 2u >= dst_size) {
        inset = 0u;
    }
    content_size = dst_size - (inset * 2u);
    if (content_size == 0u) {
        return export_image_invalid("invalid macOS icon content size");
    }
    result = drawing_program_export_image_scale_rgba_bilinear(src_rgba,
                                                              src_width,
                                                              src_height,
                                                              content_size,
                                                              content_size,
                                                              &content_rgba);
    if (result.code != CORE_OK) {
        return result;
    }
    icon_capacity = (uint64_t)dst_size * (uint64_t)dst_size * 4u;
    if (icon_capacity == 0u || icon_capacity > SIZE_MAX) {
        core_free(content_rgba);
        return export_image_invalid("macOS icon capacity too large");
    }
    icon_rgba = core_alloc((size_t)icon_capacity);
    if (!icon_rgba) {
        core_free(content_rgba);
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate macOS icon buffer" };
    }
    memset(icon_rgba, 0, (size_t)icon_capacity);
    offset = (dst_size - content_size) / 2u;
    for (y = 0u; y < content_size; ++y) {
        uint32_t x;
        for (x = 0u; x < content_size; ++x) {
            size_t src_index = (((size_t)y * (size_t)content_size) + (size_t)x) * 4u;
            size_t dst_index = ((((size_t)(y + offset) * (size_t)dst_size) + (size_t)(x + offset)) * 4u);
            icon_rgba[dst_index + 0u] = content_rgba[src_index + 0u];
            icon_rgba[dst_index + 1u] = content_rgba[src_index + 1u];
            icon_rgba[dst_index + 2u] = content_rgba[src_index + 2u];
            icon_rgba[dst_index + 3u] = content_rgba[src_index + 3u];
        }
    }
    core_free(content_rgba);
    {
        const double center = ((double)dst_size - 1.0) * 0.5;
        const double half_extent = (double)content_size * 0.5;
        const double exponent = k_export_image_macos_icon_superellipse_exponent;
        static const double k_subsample_offsets[4] = { 0.125, 0.375, 0.625, 0.875 };
        for (y = 0u; y < dst_size; ++y) {
            uint32_t x;
            for (x = 0u; x < dst_size; ++x) {
                uint32_t inside_samples = 0u;
                size_t dst_index = ((((size_t)y * (size_t)dst_size) + (size_t)x) * 4u);
                uint8_t alpha = icon_rgba[dst_index + 3u];
                uint32_t sy;
                if (alpha == 0u) {
                    continue;
                }
                for (sy = 0u; sy < 4u; ++sy) {
                    uint32_t sx;
                    for (sx = 0u; sx < 4u; ++sx) {
                        double sample_x = ((double)x + k_subsample_offsets[sx]);
                        double sample_y = ((double)y + k_subsample_offsets[sy]);
                        double nx = fabs((sample_x - center) / half_extent);
                        double ny = fabs((sample_y - center) / half_extent);
                        double shape = pow(nx, exponent) + pow(ny, exponent);
                        if (shape <= 1.0) {
                            inside_samples += 1u;
                        }
                    }
                }
                if (inside_samples == 0u) {
                    icon_rgba[dst_index + 3u] = 0u;
                } else if (inside_samples < 16u) {
                    double coverage = (double)inside_samples / 16.0;
                    icon_rgba[dst_index + 3u] = (uint8_t)lrint((double)alpha * coverage);
                }
            }
        }
    }
    *out_rgba = icon_rgba;
    return core_result_ok();
}
