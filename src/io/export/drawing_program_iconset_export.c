#include "drawing_program/drawing_program_iconset_export.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_export_image.h"

typedef struct DrawingProgramIconsetEntry {
    const char *filename;
    uint32_t size;
} DrawingProgramIconsetEntry;

static const DrawingProgramIconsetEntry k_iconset_entries[] = {
    { "icon_16x16.png", 16u },
    { "icon_16x16@2x.png", 32u },
    { "icon_32x32.png", 32u },
    { "icon_32x32@2x.png", 64u },
    { "icon_128x128.png", 128u },
    { "icon_128x128@2x.png", 256u },
    { "icon_256x256.png", 256u },
    { "icon_256x256@2x.png", 512u },
    { "icon_512x512.png", 512u },
    { "icon_512x512@2x.png", 1024u }
};

static CoreResult iconset_export_invalid(const char *message) {
    return (CoreResult){ CORE_ERR_INVALID_ARG, message };
}

static CoreResult iconset_export_io_error(const char *message) {
    return (CoreResult){ CORE_ERR_IO, message };
}

static CoreResult iconset_export_mkdir_if_needed(const char *dir_path) {
    if (!dir_path || dir_path[0] == '\0') {
        return core_result_ok();
    }
    if (mkdir(dir_path, 0775) != 0 && errno != EEXIST) {
        return iconset_export_io_error("failed to create iconset directory");
    }
    return core_result_ok();
}

CoreResult drawing_program_iconset_export_default_path(const struct DrawingProgramAppContext *ctx,
                                                       char *out_path,
                                                       size_t out_cap) {
    char base_name[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx || !out_path || out_cap == 0u) {
        return iconset_export_invalid("invalid iconset export default path request");
    }
    result = drawing_program_export_image_default_basename(ctx, base_name, sizeof(base_name));
    if (result.code != CORE_OK) {
        return result;
    }
    (void)snprintf(out_path, out_cap, "%s/%s.iconset", ctx->session.output_root_path, base_name);
    return core_result_ok();
}

CoreResult drawing_program_iconset_export_current_document(const struct DrawingProgramAppContext *ctx,
                                                           const char *iconset_dir_path) {
    uint8_t *src_rgba = 0;
    uint32_t src_width = 0u;
    uint32_t src_height = 0u;
    CoreResult result;
    uint32_t i;
    if (!ctx || !iconset_dir_path || iconset_dir_path[0] == '\0') {
        return iconset_export_invalid("invalid iconset export request");
    }
    result = iconset_export_mkdir_if_needed(iconset_dir_path);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_export_image_compose_document_rgba(ctx, &src_rgba, &src_width, &src_height);
    if (result.code != CORE_OK) {
        return result;
    }
    for (i = 0u; i < (uint32_t)(sizeof(k_iconset_entries) / sizeof(k_iconset_entries[0])); ++i) {
        char output_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        uint8_t *scaled_rgba = 0;
        result = drawing_program_export_image_build_macos_icon_rgba(src_rgba,
                                                                    src_width,
                                                                    src_height,
                                                                    k_iconset_entries[i].size,
                                                                    &scaled_rgba);
        if (result.code != CORE_OK) {
            core_free(src_rgba);
            return result;
        }
        (void)snprintf(output_path,
                       sizeof(output_path),
                       "%s/%s",
                       iconset_dir_path,
                       k_iconset_entries[i].filename);
        result = drawing_program_export_image_write_png_rgba(output_path,
                                                             scaled_rgba,
                                                             k_iconset_entries[i].size,
                                                             k_iconset_entries[i].size);
        core_free(scaled_rgba);
        if (result.code != CORE_OK) {
            core_free(src_rgba);
            return result;
        }
    }
    core_free(src_rgba);
    return core_result_ok();
}
