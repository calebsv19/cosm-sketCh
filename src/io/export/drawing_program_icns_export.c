#include "drawing_program/drawing_program_icns_export.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_export_image.h"
#include "drawing_program/drawing_program_iconset_export.h"

typedef struct DrawingProgramIcnsEntry {
    const char *filename;
    const char *chunk_type;
} DrawingProgramIcnsEntry;

static const DrawingProgramIcnsEntry k_icns_entries[] = {
    { "icon_16x16.png", "icp4" },
    { "icon_32x32.png", "icp5" },
    { "icon_32x32@2x.png", "icp6" },
    { "icon_128x128.png", "ic07" },
    { "icon_256x256.png", "ic08" },
    { "icon_512x512.png", "ic09" },
    { "icon_512x512@2x.png", "ic10" }
};

static CoreResult icns_export_invalid(const char *message) {
    return (CoreResult){ CORE_ERR_INVALID_ARG, message };
}

static CoreResult icns_export_io_error(const char *message) {
    return (CoreResult){ CORE_ERR_IO, message };
}

static CoreResult icns_export_mkdirs_if_needed(const char *dir_path) {
    char buffer[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    size_t i;
    size_t len;
    if (!dir_path || dir_path[0] == '\0') {
        return core_result_ok();
    }
    len = strlen(dir_path);
    if (len >= sizeof(buffer)) {
        return icns_export_invalid("icns directory path too long");
    }
    (void)snprintf(buffer, sizeof(buffer), "%s", dir_path);
    for (i = 1u; i < len; ++i) {
        if (buffer[i] != '/') {
            continue;
        }
        buffer[i] = '\0';
        if (buffer[0] != '\0') {
            if (mkdir(buffer, 0775) != 0 && errno != EEXIST) {
                return icns_export_io_error("failed to create icns directory segment");
            }
        }
        buffer[i] = '/';
    }
    if (mkdir(buffer, 0775) != 0 && errno != EEXIST) {
        return icns_export_io_error("failed to create icns directory");
    }
    return core_result_ok();
}

static CoreResult icns_export_ensure_parent_dir(const char *file_path) {
    char dir_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    char *slash = 0;
    if (!file_path || file_path[0] == '\0') {
        return core_result_ok();
    }
    if (strlen(file_path) >= sizeof(dir_path)) {
        return icns_export_invalid("icns export path too long");
    }
    (void)snprintf(dir_path, sizeof(dir_path), "%s", file_path);
    slash = strrchr(dir_path, '/');
    if (!slash) {
        return core_result_ok();
    }
    *slash = '\0';
    if (dir_path[0] == '\0') {
        return core_result_ok();
    }
    return icns_export_mkdirs_if_needed(dir_path);
}

static CoreResult icns_export_iconset_path_for_icns(const char *icns_path, char *out_path, size_t out_cap) {
    size_t len = 0u;
    if (!icns_path || !out_path || out_cap == 0u) {
        return icns_export_invalid("invalid icns/iconset path request");
    }
    len = strlen(icns_path);
    if (len == 0u || len >= out_cap) {
        return icns_export_invalid("icns path too long");
    }
    if (len > 5u && strcmp(icns_path + len - 5u, ".icns") == 0) {
        if (len + 4u >= out_cap) {
            return icns_export_invalid("iconset path too long");
        }
        memcpy(out_path, icns_path, len - 5u);
        out_path[len - 5u] = '\0';
        (void)snprintf(out_path + (len - 5u), out_cap - (len - 5u), ".iconset");
        return core_result_ok();
    }
    if (len + 8u >= out_cap) {
        return icns_export_invalid("iconset path too long");
    }
    (void)snprintf(out_path, out_cap, "%s.iconset", icns_path);
    return core_result_ok();
}

static CoreResult icns_export_read_file_bytes(const char *path, uint8_t **out_bytes, uint32_t *out_size) {
    FILE *file = 0;
    long raw_size = 0;
    uint8_t *bytes = 0;
    size_t read_count = 0u;
    if (!path || !out_bytes || !out_size) {
        return icns_export_invalid("invalid icns file read request");
    }
    *out_bytes = 0;
    *out_size = 0u;
    file = fopen(path, "rb");
    if (!file) {
        return icns_export_io_error("failed to open iconset PNG for icns export");
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        (void)fclose(file);
        return icns_export_io_error("failed to seek iconset PNG");
    }
    raw_size = ftell(file);
    if (raw_size <= 0 || raw_size > 0x7fffffffL) {
        (void)fclose(file);
        return icns_export_io_error("invalid iconset PNG size");
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        (void)fclose(file);
        return icns_export_io_error("failed to rewind iconset PNG");
    }
    bytes = (uint8_t *)core_alloc((size_t)raw_size);
    if (!bytes) {
        (void)fclose(file);
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate icns PNG buffer" };
    }
    read_count = fread(bytes, 1u, (size_t)raw_size, file);
    (void)fclose(file);
    if (read_count != (size_t)raw_size) {
        core_free(bytes);
        return icns_export_io_error("failed to read iconset PNG bytes");
    }
    *out_bytes = bytes;
    *out_size = (uint32_t)raw_size;
    return core_result_ok();
}

static CoreResult icns_export_write_be32(FILE *file, uint32_t value) {
    unsigned char bytes[4];
    if (!file) {
        return icns_export_invalid("invalid icns write request");
    }
    bytes[0] = (unsigned char)((value >> 24) & 0xffu);
    bytes[1] = (unsigned char)((value >> 16) & 0xffu);
    bytes[2] = (unsigned char)((value >> 8) & 0xffu);
    bytes[3] = (unsigned char)(value & 0xffu);
    if (fwrite(bytes, 1u, sizeof(bytes), file) != sizeof(bytes)) {
        return icns_export_io_error("failed to write icns length");
    }
    return core_result_ok();
}

CoreResult drawing_program_icns_export_default_path(const struct DrawingProgramAppContext *ctx,
                                                    char *out_path,
                                                    size_t out_cap) {
    char base_name[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx || !out_path || out_cap == 0u) {
        return icns_export_invalid("invalid icns export default path request");
    }
    result = drawing_program_export_image_default_basename(ctx, base_name, sizeof(base_name));
    if (result.code != CORE_OK) {
        return result;
    }
    (void)snprintf(out_path, out_cap, "%s/%s.icns", ctx->session.output_root_path, base_name);
    return core_result_ok();
}

CoreResult drawing_program_icns_export_current_document(const struct DrawingProgramAppContext *ctx,
                                                        const char *icns_path) {
    char iconset_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    uint8_t *chunk_bytes[sizeof(k_icns_entries) / sizeof(k_icns_entries[0])] = { 0 };
    uint32_t chunk_sizes[sizeof(k_icns_entries) / sizeof(k_icns_entries[0])] = { 0 };
    uint32_t total_size = 8u;
    FILE *icns_file = 0;
    CoreResult result;
    uint32_t i;
    if (!ctx || !icns_path || icns_path[0] == '\0') {
        return icns_export_invalid("invalid icns export request");
    }
    result = icns_export_ensure_parent_dir(icns_path);
    if (result.code != CORE_OK) {
        return result;
    }
    result = icns_export_iconset_path_for_icns(icns_path, iconset_path, sizeof(iconset_path));
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_iconset_export_current_document(ctx, iconset_path);
    if (result.code != CORE_OK) {
        return result;
    }
    for (i = 0u; i < (uint32_t)(sizeof(k_icns_entries) / sizeof(k_icns_entries[0])); ++i) {
        char png_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        (void)snprintf(png_path, sizeof(png_path), "%s/%s", iconset_path, k_icns_entries[i].filename);
        result = icns_export_read_file_bytes(png_path, &chunk_bytes[i], &chunk_sizes[i]);
        if (result.code != CORE_OK) {
            uint32_t j;
            for (j = 0u; j <= i; ++j) {
                core_free(chunk_bytes[j]);
            }
            return result;
        }
        total_size += 8u + chunk_sizes[i];
    }
    icns_file = fopen(icns_path, "wb");
    if (!icns_file) {
        for (i = 0u; i < (uint32_t)(sizeof(k_icns_entries) / sizeof(k_icns_entries[0])); ++i) {
            core_free(chunk_bytes[i]);
        }
        return icns_export_io_error("failed to open icns output path");
    }
    if (fwrite("icns", 1u, 4u, icns_file) != 4u) {
        (void)fclose(icns_file);
        for (i = 0u; i < (uint32_t)(sizeof(k_icns_entries) / sizeof(k_icns_entries[0])); ++i) {
            core_free(chunk_bytes[i]);
        }
        return icns_export_io_error("failed to write icns header");
    }
    result = icns_export_write_be32(icns_file, total_size);
    if (result.code != CORE_OK) {
        (void)fclose(icns_file);
        for (i = 0u; i < (uint32_t)(sizeof(k_icns_entries) / sizeof(k_icns_entries[0])); ++i) {
            core_free(chunk_bytes[i]);
        }
        return result;
    }
    for (i = 0u; i < (uint32_t)(sizeof(k_icns_entries) / sizeof(k_icns_entries[0])); ++i) {
        if (fwrite(k_icns_entries[i].chunk_type, 1u, 4u, icns_file) != 4u) {
            result = icns_export_io_error("failed to write icns chunk type");
            break;
        }
        result = icns_export_write_be32(icns_file, chunk_sizes[i] + 8u);
        if (result.code != CORE_OK) {
            break;
        }
        if (fwrite(chunk_bytes[i], 1u, chunk_sizes[i], icns_file) != chunk_sizes[i]) {
            result = icns_export_io_error("failed to write icns chunk bytes");
            break;
        }
    }
    (void)fclose(icns_file);
    for (i = 0u; i < (uint32_t)(sizeof(k_icns_entries) / sizeof(k_icns_entries[0])); ++i) {
        core_free(chunk_bytes[i]);
    }
    return result.code == CORE_OK ? core_result_ok() : result;
}
