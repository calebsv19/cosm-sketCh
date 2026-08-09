#include "drawing_program/drawing_program_indexed_tileset_export.h"

#include <dirent.h>
#include <errno.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "drawing_program/drawing_program_export_image.h"
#include "drawing_program/drawing_program_indexed_png.h"

static CoreResult indexed_export_invalid(const char *message) {
    return (CoreResult){ CORE_ERR_INVALID_ARG, message };
}

static CoreResult indexed_export_io(const char *message) {
    return (CoreResult){ CORE_ERR_IO, message };
}

static int indexed_export_path(char *out, size_t capacity, const char *parent, const char *child) {
    int written;
    if (!out || !parent || !child) return 0;
    written = snprintf(out, capacity, "%s/%s", parent, child);
    return written > 0 && (size_t)written < capacity;
}

static int indexed_export_dir_exists(const char *path) {
    struct stat info;
    return path && stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

static int indexed_export_remove_tree(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char child[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
    struct stat info;
    if (!path || lstat(path, &info) != 0) return errno == ENOENT;
    if (!S_ISDIR(info.st_mode)) return unlink(path) == 0;
    dir = opendir(path);
    if (!dir) return 0;
    while ((entry = readdir(dir)) != 0) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (!indexed_export_path(child, sizeof(child), path, entry->d_name) ||
            !indexed_export_remove_tree(child)) {
            closedir(dir);
            return 0;
        }
    }
    closedir(dir);
    return rmdir(path) == 0;
}

static uint64_t indexed_export_hash_bytes(uint64_t hash, const void *data, size_t size) {
    const uint8_t *bytes = (const uint8_t *)data;
    size_t i;
    for (i = 0u; i < size; ++i) {
        hash ^= bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static uint64_t indexed_export_hash_u32(uint64_t hash, uint32_t value) {
    uint8_t bytes[4] = {
        (uint8_t)(value & 0xffu), (uint8_t)((value >> 8u) & 0xffu),
        (uint8_t)((value >> 16u) & 0xffu), (uint8_t)((value >> 24u) & 0xffu)
    };
    return indexed_export_hash_bytes(hash, bytes, sizeof(bytes));
}

static void indexed_export_digest(char out[17], uint64_t value) {
    (void)snprintf(out, 17u, "%016llx", (unsigned long long)value);
}

static void indexed_export_compute_digests(
    const DrawingProgramTextureProject *project,
    DrawingProgramIndexedTilesetExportReport *report) {
    uint64_t index_hash = UINT64_C(1469598103934665603);
    uint64_t palette_hash = UINT64_C(1469598103934665603);
    uint64_t cell_hash = UINT64_C(1469598103934665603);
    uint32_t i;
    index_hash = indexed_export_hash_bytes(index_hash, project->indexed_raster.indices,
                                           project->indexed_raster.index_count);
    for (i = 0u; i < project->indexed_profile.slot_count; ++i) {
        const DrawingProgramIndexedPaletteSlot *slot = &project->indexed_profile.slots[i];
        palette_hash = indexed_export_hash_bytes(palette_hash, slot->id, strlen(slot->id) + 1u);
        palette_hash = indexed_export_hash_bytes(palette_hash, &slot->source_rgba, sizeof(slot->source_rgba));
        palette_hash = indexed_export_hash_bytes(palette_hash, &slot->preview_rgba, sizeof(slot->preview_rgba));
    }
    for (i = 0u; i < project->indexed_cells.count; ++i) {
        const DrawingProgramIndexedCell *cell = &project->indexed_cells.cells[i];
        cell_hash = indexed_export_hash_bytes(cell_hash, cell->id, strlen(cell->id) + 1u);
        cell_hash = indexed_export_hash_u32(cell_hash, cell->x);
        cell_hash = indexed_export_hash_u32(cell_hash, cell->y);
        cell_hash = indexed_export_hash_u32(cell_hash, cell->width);
        cell_hash = indexed_export_hash_u32(cell_hash, cell->height);
    }
    indexed_export_digest(report->index_digest, index_hash);
    indexed_export_digest(report->palette_digest, palette_hash);
    indexed_export_digest(report->cell_digest, cell_hash);
}

static json_object *indexed_export_rgba(CoreAuthoredTextureRgba8 rgba) {
    json_object *array = json_object_new_array_ext(4);
    json_object_array_add(array, json_object_new_int(rgba.r));
    json_object_array_add(array, json_object_new_int(rgba.g));
    json_object_array_add(array, json_object_new_int(rgba.b));
    json_object_array_add(array, json_object_new_int(rgba.a));
    return array;
}

static CoreResult indexed_export_write_json(const char *path, json_object *root) {
    FILE *file;
    const char *text;
    size_t length;
    if (!path || !root) return indexed_export_invalid("invalid indexed JSON write request");
    text = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_NOSLASHESCAPE);
    length = strlen(text);
    file = fopen(path, "wb");
    if (!file) return indexed_export_io("failed to open indexed JSON output");
    if (fwrite(text, 1u, length, file) != length || fwrite("\n", 1u, 1u, file) != 1u ||
        fclose(file) != 0) {
        return indexed_export_io("failed to write indexed JSON output");
    }
    return core_result_ok();
}

static CoreResult indexed_export_write_manifest(
    const DrawingProgramTextureProject *project,
    const char *path,
    const char *image_name,
    const char *palette_id) {
    json_object *root = json_object_new_object();
    json_object *logical = json_object_new_object();
    json_object *atlas = json_object_new_object();
    json_object *slots = json_object_new_array_ext((int)project->indexed_profile.slot_count);
    json_object *cells = json_object_new_array_ext((int)project->indexed_cells.count);
    CoreResult result;
    uint32_t i;
    if (!root || !logical || !atlas || !slots || !cells) {
        json_object_put(root); json_object_put(logical); json_object_put(atlas);
        json_object_put(slots); json_object_put(cells);
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate indexed manifest" };
    }
    json_object_object_add(root, "contract_revision", json_object_new_int((int)project->indexed_profile.contract_revision));
    json_object_object_add(root, "tileset", json_object_new_string(project->indexed_profile.tileset_id));
    json_object_object_add(logical, "width", json_object_new_int((int)project->indexed_profile.logical_cell_width));
    json_object_object_add(logical, "height", json_object_new_int((int)project->indexed_profile.logical_cell_height));
    json_object_object_add(root, "logical_tile", logical);
    json_object_object_add(atlas, "width", json_object_new_int((int)project->indexed_profile.atlas_width));
    json_object_object_add(atlas, "height", json_object_new_int((int)project->indexed_profile.atlas_height));
    json_object_object_add(atlas, "output_kind", json_object_new_string("INDEX_ATLAS"));
    json_object_object_add(atlas, "image", json_object_new_string(image_name));
    json_object_object_add(root, "atlas", atlas);
    for (i = 0u; i < project->indexed_profile.slot_count; ++i) {
        json_object *slot = json_object_new_object();
        json_object_object_add(slot, "id", json_object_new_string(project->indexed_profile.slots[i].id));
        json_object_object_add(slot, "source_rgba", indexed_export_rgba(project->indexed_profile.slots[i].source_rgba));
        json_object_array_add(slots, slot);
    }
    json_object_object_add(root, "palette_slots", slots);
    for (i = 0u; i < project->indexed_cells.count; ++i) {
        const DrawingProgramIndexedCell *source = &project->indexed_cells.cells[i];
        json_object *cell = json_object_new_object();
        json_object_object_add(cell, "key", json_object_new_string(source->id));
        json_object_object_add(cell, "x", json_object_new_int((int)source->x));
        json_object_object_add(cell, "y", json_object_new_int((int)source->y));
        json_object_object_add(cell, "width", json_object_new_int((int)source->width));
        json_object_object_add(cell, "height", json_object_new_int((int)source->height));
        json_object_array_add(cells, cell);
    }
    json_object_object_add(root, "cells", cells);
    json_object_object_add(root, "preview_palette", json_object_new_string(palette_id));
    result = indexed_export_write_json(path, root);
    json_object_put(root);
    return result;
}

static CoreResult indexed_export_write_palette_json(
    const DrawingProgramTextureProject *project,
    const char *path,
    const char *palette_id) {
    json_object *root = json_object_new_object();
    json_object *colors = json_object_new_object();
    CoreResult result;
    uint32_t i;
    if (!root || !colors) {
        json_object_put(root); json_object_put(colors);
        return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate indexed palette JSON" };
    }
    json_object_object_add(root, "contract_revision", json_object_new_int((int)project->indexed_profile.contract_revision));
    json_object_object_add(root, "palette", json_object_new_string(palette_id));
    for (i = 0u; i < project->indexed_profile.slot_count; ++i) {
        json_object_object_add(colors, project->indexed_profile.slots[i].id,
                               indexed_export_rgba(project->indexed_profile.slots[i].preview_rgba));
    }
    json_object_object_add(root, "colors", colors);
    result = indexed_export_write_json(path, root);
    json_object_put(root);
    return result;
}

static CoreResult indexed_export_write_validation(
    const DrawingProgramTextureProject *project,
    const char *path,
    const DrawingProgramIndexedTilesetExportReport *report) {
    json_object *root = json_object_new_object();
    CoreResult result;
    if (!root) return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate export validation JSON" };
    json_object_object_add(root, "contract_revision", json_object_new_int((int)project->indexed_profile.contract_revision));
    json_object_object_add(root, "tileset", json_object_new_string(project->indexed_profile.tileset_id));
    json_object_object_add(root, "index_digest", json_object_new_string(report->index_digest));
    json_object_object_add(root, "palette_digest", json_object_new_string(report->palette_digest));
    json_object_object_add(root, "cell_digest", json_object_new_string(report->cell_digest));
    json_object_object_add(root, "status", json_object_new_string("validated"));
    result = indexed_export_write_json(path, root);
    json_object_put(root);
    return result;
}

static CoreResult indexed_export_validate_reopen(
    const DrawingProgramTextureProject *project,
    const char *manifest_path,
    const char *png_path) {
    DrawingProgramIndexedPngImage image;
    json_object *manifest;
    json_object *cells;
    json_object *slots;
    uint32_t i;
    CoreResult result = drawing_program_indexed_png_read(png_path, &image);
    if (result.code != CORE_OK) return result;
    if (image.width != project->indexed_raster.width || image.height != project->indexed_raster.height ||
        image.palette_count != project->indexed_profile.slot_count ||
        memcmp(image.indices, project->indexed_raster.indices, project->indexed_raster.index_count) != 0) {
        drawing_program_indexed_png_dispose(&image);
        return indexed_export_invalid("reopened indexed PNG differs from source indices");
    }
    for (i = 0u; i < image.palette_count; ++i) {
        if (!core_authored_texture_rgba8_equal(image.palette[i], project->indexed_profile.slots[i].preview_rgba)) {
            drawing_program_indexed_png_dispose(&image);
            return indexed_export_invalid("reopened indexed PNG palette order differs from source");
        }
    }
    drawing_program_indexed_png_dispose(&image);
    manifest = json_object_from_file(manifest_path);
    if (!manifest || !json_object_object_get_ex(manifest, "cells", &cells) ||
        !json_object_is_type(cells, json_type_array) ||
        json_object_array_length(cells) != project->indexed_cells.count ||
        !json_object_object_get_ex(manifest, "palette_slots", &slots) ||
        !json_object_is_type(slots, json_type_array) ||
        json_object_array_length(slots) != project->indexed_profile.slot_count) {
        json_object_put(manifest);
        return indexed_export_invalid("reopened indexed manifest shape differs from source");
    }
    for (i = 0u; i < project->indexed_cells.count; ++i) {
        json_object *cell = json_object_array_get_idx(cells, i);
        json_object *key;
        json_object *x;
        json_object *y;
        json_object *width;
        json_object *height;
        const DrawingProgramIndexedCell *source = &project->indexed_cells.cells[i];
        if (!json_object_object_get_ex(cell, "key", &key) || strcmp(json_object_get_string(key), source->id) != 0 ||
            !json_object_object_get_ex(cell, "x", &x) || json_object_get_int64(x) != source->x ||
            !json_object_object_get_ex(cell, "y", &y) || json_object_get_int64(y) != source->y ||
            !json_object_object_get_ex(cell, "width", &width) || json_object_get_int64(width) != source->width ||
            !json_object_object_get_ex(cell, "height", &height) || json_object_get_int64(height) != source->height) {
            json_object_put(manifest);
            return indexed_export_invalid("reopened indexed manifest cells differ from source");
        }
    }
    for (i = 0u; i < project->indexed_profile.slot_count; ++i) {
        json_object *slot = json_object_array_get_idx(slots, i);
        json_object *id;
        if (!json_object_object_get_ex(slot, "id", &id) ||
            strcmp(json_object_get_string(id), project->indexed_profile.slots[i].id) != 0) {
            json_object_put(manifest);
            return indexed_export_invalid("reopened indexed manifest slot order differs from source");
        }
    }
    json_object_put(manifest);
    return core_result_ok();
}

CoreResult drawing_program_indexed_tileset_export(
    const DrawingProgramTextureProject *project,
    const char *destination,
    const DrawingProgramIndexedTilesetExportOptions *options,
    DrawingProgramIndexedTilesetExportReport *out_report) {
    DrawingProgramIndexedTilesetExportReport report;
    CoreAuthoredTextureRgba8 palette[256];
    uint8_t *rgba = 0;
    char parent[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
    char stage[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
    char backup[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
    char palettes_dir[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
    char png_name[DRAWING_PROGRAM_INDEXED_TILESET_ID_CAPACITY + 20u];
    char palette_png_name[DRAWING_PROGRAM_INDEXED_TILESET_ID_CAPACITY + 5u];
    char palette_json_name[DRAWING_PROGRAM_INDEXED_TILESET_ID_CAPACITY + 14u];
    char png_path[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
    char palette_png_path[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
    char palette_json_path[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
    char manifest_path[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
    char validation_path[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
    const char *palette_id = options && options->palette_id ? options->palette_id : "default";
    const char *slash;
    uint32_t i;
    CoreResult result;
    int had_destination;
    if (!project || !destination || destination[0] == '\0' || !out_report ||
        !core_authored_texture_identifier_validate(palette_id) ||
        drawing_program_texture_project_validate_indexed_atlas(project).code != CORE_OK ||
        drawing_program_indexed_cell_table_validate(&project->indexed_cells,
            project->indexed_profile.atlas_width, project->indexed_profile.atlas_height,
            project->indexed_profile.logical_cell_width,
            project->indexed_profile.logical_cell_height).code != CORE_OK) {
        return indexed_export_invalid("invalid indexed tileset export request");
    }
    memset(&report, 0, sizeof(report));
    indexed_export_compute_digests(project, &report);
    slash = strrchr(destination, '/');
    if (!slash || slash == destination || (size_t)(slash - destination) >= sizeof(parent)) {
        return indexed_export_invalid("indexed tileset destination requires an absolute parent path");
    }
    memcpy(parent, destination, (size_t)(slash - destination));
    parent[slash - destination] = '\0';
    if (!indexed_export_dir_exists(parent) ||
        strlen(destination) + 40u >= sizeof(stage) ||
        strlen(destination) + 40u >= sizeof(backup) ||
        snprintf(stage, sizeof(stage), "%s.dpt-stage-%ld", destination, (long)getpid()) <= 0 ||
        snprintf(backup, sizeof(backup), "%s.dpt-backup-%ld", destination, (long)getpid()) <= 0) {
        return indexed_export_io("indexed tileset destination parent is unavailable");
    }
    (void)indexed_export_remove_tree(stage);
    (void)indexed_export_remove_tree(backup);
    if (mkdir(stage, 0755) != 0 || !indexed_export_path(palettes_dir, sizeof(palettes_dir), stage, "palettes") ||
        mkdir(palettes_dir, 0755) != 0) {
        (void)indexed_export_remove_tree(stage);
        return indexed_export_io("failed to create indexed export staging directory");
    }
    (void)snprintf(png_name, sizeof(png_name), "%s_indices.png", project->indexed_profile.tileset_id);
    (void)snprintf(palette_png_name, sizeof(palette_png_name), "%s.png", palette_id);
    (void)snprintf(palette_json_name, sizeof(palette_json_name), "%s.palette.json", palette_id);
    if (!indexed_export_path(png_path, sizeof(png_path), stage, png_name) ||
        !indexed_export_path(palette_png_path, sizeof(palette_png_path), palettes_dir, palette_png_name) ||
        !indexed_export_path(palette_json_path, sizeof(palette_json_path), palettes_dir, palette_json_name) ||
        !indexed_export_path(manifest_path, sizeof(manifest_path), stage, "tileset_manifest.json") ||
        !indexed_export_path(validation_path, sizeof(validation_path), stage, "export_validation.json")) {
        result = indexed_export_invalid("indexed export artifact path too long");
        goto fail;
    }
    rgba = (uint8_t *)malloc((size_t)project->indexed_raster.index_count * 4u);
    if (!rgba) { result = (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "failed to allocate palette preview" }; goto fail; }
    for (i = 0u; i < project->indexed_profile.slot_count; ++i) palette[i] = project->indexed_profile.slots[i].preview_rgba;
    for (i = 0u; i < project->indexed_raster.index_count; ++i) {
        CoreAuthoredTextureRgba8 color = palette[project->indexed_raster.indices[i]];
        rgba[i * 4u + 0u] = color.r; rgba[i * 4u + 1u] = color.g;
        rgba[i * 4u + 2u] = color.b; rgba[i * 4u + 3u] = color.a;
    }
    result = drawing_program_indexed_png_write(png_path, project->indexed_raster.indices,
        project->indexed_raster.width, project->indexed_raster.height, palette,
        project->indexed_profile.slot_count);
    if (result.code != CORE_OK) goto fail;
    result = drawing_program_export_image_write_png_rgba(palette_png_path, rgba,
        project->indexed_raster.width, project->indexed_raster.height);
    if (result.code != CORE_OK) goto fail;
    free(rgba); rgba = 0;
    result = indexed_export_write_palette_json(project, palette_json_path, palette_id);
    if (result.code != CORE_OK) goto fail;
    result = indexed_export_write_manifest(project, manifest_path, png_name, palette_id);
    if (result.code != CORE_OK) goto fail;
    result = indexed_export_write_validation(project, validation_path, &report);
    if (result.code != CORE_OK) goto fail;
    result = indexed_export_validate_reopen(project, manifest_path, png_path);
    if (result.code != CORE_OK) goto fail;
    if (options && options->force_failure_before_commit) {
        result = indexed_export_io("forced indexed export failure before commit");
        goto fail;
    }
    had_destination = indexed_export_dir_exists(destination);
    if (had_destination && rename(destination, backup) != 0) {
        result = indexed_export_io("failed to preserve prior indexed export destination");
        goto fail;
    }
    if (rename(stage, destination) != 0) {
        if (had_destination) (void)rename(backup, destination);
        result = indexed_export_io("failed to commit indexed export destination");
        goto fail;
    }
    if (had_destination && !indexed_export_remove_tree(backup)) {
        return indexed_export_io("indexed export committed but prior destination cleanup failed");
    }
    (void)indexed_export_path(report.index_png_path, sizeof(report.index_png_path), destination, png_name);
    (void)indexed_export_path(report.manifest_path, sizeof(report.manifest_path), destination, "tileset_manifest.json");
    (void)indexed_export_path(report.validation_path, sizeof(report.validation_path), destination, "export_validation.json");
    *out_report = report;
    return core_result_ok();
fail:
    free(rgba);
    (void)indexed_export_remove_tree(stage);
    return result;
}
