#include "drawing_program/drawing_program_project_state.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "drawing_program/drawing_program_app_post_load.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_snapshot.h"

typedef struct DrawingProgramRecentProjectEntry {
    char path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    uint64_t modified_time;
} DrawingProgramRecentProjectEntry;

static void drawing_program_project_state_copy_current_path(struct DrawingProgramAppContext *ctx,
                                                            const char *path);

static CoreResult drawing_program_project_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static CoreResult drawing_program_project_slot_path_missing(const char *message) {
    CoreResult r = { CORE_ERR_NOT_FOUND, message };
    return r;
}

static void drawing_program_project_trim_dir_path(char *path) {
    size_t len = 0u;
    if (!path) {
        return;
    }
    len = strlen(path);
    while (len > 1u && path[len - 1u] == '/') {
        path[--len] = '\0';
    }
}

static int drawing_program_project_path_has_pack_extension(const char *path) {
    const char *dot = 0;
    if (!path || path[0] == '\0') {
        return 0;
    }
    dot = strrchr(path, '.');
    return (dot && strcmp(dot, ".pack") == 0) ? 1 : 0;
}

static CoreResult drawing_program_project_mkdirs_if_needed(const char *dir_path) {
    char buffer[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    size_t i;
    size_t len;
    if (!dir_path || dir_path[0] == '\0') {
        return core_result_ok();
    }
    len = strlen(dir_path);
    if (len >= sizeof(buffer)) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "directory path too long" };
    }
    (void)snprintf(buffer, sizeof(buffer), "%s", dir_path);
    for (i = 1u; i < len; ++i) {
        if (buffer[i] != '/') {
            continue;
        }
        buffer[i] = '\0';
        if (buffer[0] != '\0') {
            if (mkdir(buffer, 0775) != 0 && errno != EEXIST) {
                return (CoreResult){ CORE_ERR_IO, "failed to create project directory segment" };
            }
        }
        buffer[i] = '/';
    }
    if (mkdir(buffer, 0775) != 0 && errno != EEXIST) {
        return (CoreResult){ CORE_ERR_IO, "failed to create project directory" };
    }
    return core_result_ok();
}

static CoreResult drawing_program_project_ensure_parent_dir(const char *file_path) {
    char buffer[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    char *slash = 0;
    if (!file_path || file_path[0] == '\0') {
        return core_result_ok();
    }
    if (strlen(file_path) >= sizeof(buffer)) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "file path too long" };
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
    return drawing_program_project_mkdirs_if_needed(buffer);
}

static CoreResult drawing_program_project_copy_parent_dir(const char *file_path, char *out_dir, size_t out_cap) {
    const char *slash = 0;
    size_t len = 0u;
    if (!file_path || !out_dir || out_cap == 0u) {
        return drawing_program_project_invalid("invalid project parent dir request");
    }
    slash = strrchr(file_path, '/');
    if (!slash) {
        if (out_cap < 2u) {
            return drawing_program_project_invalid("project parent dir buffer too small");
        }
        out_dir[0] = '.';
        out_dir[1] = '\0';
        return core_result_ok();
    }
    len = (size_t)(slash - file_path);
    if (len == 0u) {
        len = 1u;
    }
    if (len + 1u > out_cap) {
        return drawing_program_project_invalid("project parent dir path too long");
    }
    memcpy(out_dir, file_path, len);
    out_dir[len] = '\0';
    return core_result_ok();
}

static CoreResult drawing_program_project_normalize_path(const char *path,
                                                         uint8_t append_pack_extension,
                                                         char *out_path,
                                                         size_t out_cap) {
    size_t len = 0u;
    if (!path || !out_path || out_cap == 0u) {
        return drawing_program_project_invalid("invalid project path normalization request");
    }
    len = strlen(path);
    if (len == 0u) {
        return drawing_program_project_invalid("empty project path");
    }
    if (append_pack_extension && !drawing_program_project_path_has_pack_extension(path)) {
        if (len + 5u >= out_cap) {
            return drawing_program_project_invalid("project path too long");
        }
        (void)snprintf(out_path, out_cap, "%s.pack", path);
        return core_result_ok();
    }
    if (len >= out_cap) {
        return drawing_program_project_invalid("project path too long");
    }
    (void)snprintf(out_path, out_cap, "%s", path);
    return core_result_ok();
}

static CoreResult drawing_program_project_state_apply_current_path(struct DrawingProgramAppContext *ctx,
                                                                   const char *path) {
    char normalized_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    char parent_dir[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx || !path || path[0] == '\0') {
        return drawing_program_project_invalid("invalid current project path request");
    }
    result = drawing_program_project_normalize_path(path, 0u, normalized_path, sizeof(normalized_path));
    if (result.code != CORE_OK) {
        return result;
    }
    if (!drawing_program_project_path_has_pack_extension(normalized_path)) {
        return drawing_program_project_invalid("project path must use .pack extension");
    }
    result = drawing_program_project_copy_parent_dir(normalized_path, parent_dir, sizeof(parent_dir));
    if (result.code != CORE_OK) {
        return result;
    }
    if (strlen(parent_dir) >= sizeof(ctx->session.input_root_path)) {
        return drawing_program_project_invalid("project input root path too long");
    }
    (void)snprintf(ctx->session.input_root_path, sizeof(ctx->session.input_root_path), "%s", parent_dir);
    drawing_program_project_trim_dir_path(ctx->session.input_root_path);
    result = drawing_program_project_mkdirs_if_needed(ctx->session.input_root_path);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_project_state_refresh_recent(ctx);
    if (result.code != CORE_OK) {
        return result;
    }
    drawing_program_project_state_copy_current_path(ctx, normalized_path);
    return core_result_ok();
}

CoreResult drawing_program_project_state_current_exists(const struct DrawingProgramAppContext *ctx, uint8_t *out_exists) {
    uint8_t exists = 0u;
    if (!ctx || !out_exists) {
        return drawing_program_project_invalid("invalid current project exists request");
    }
    if (ctx->session.project_path && ctx->session.project_path[0] != '\0' &&
        access(ctx->session.project_path, F_OK) == 0) {
        exists = 1u;
    }
    *out_exists = exists;
    return core_result_ok();
}

static void drawing_program_project_state_copy_current_path(struct DrawingProgramAppContext *ctx,
                                                            const char *path) {
    if (!ctx) {
        return;
    }
    if (!path) {
        ctx->session.project_path_buffer[0] = '\0';
        ctx->session.project_path = 0;
        ctx->session.project_has_saved_state = 0u;
        return;
    }
    (void)snprintf(ctx->session.project_path_buffer, sizeof(ctx->session.project_path_buffer), "%s", path);
    ctx->session.project_path = ctx->session.project_path_buffer;
    ctx->session.project_has_saved_state = 0u;
}

static CoreResult drawing_program_project_state_nth_empty_path(const struct DrawingProgramAppContext *ctx,
                                                               uint32_t empty_offset,
                                                               char *out_path,
                                                               size_t out_cap) {
    uint32_t i;
    uint32_t missing_seen = 0u;
    if (!ctx || !out_path || out_cap == 0u) {
        return drawing_program_project_invalid("invalid project slot path request");
    }
    for (i = 1u; i <= 999u; ++i) {
        char candidate[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        (void)snprintf(candidate, sizeof(candidate), "%s/icon_project_%03u.pack", ctx->session.input_root_path, i);
        if (access(candidate, F_OK) == 0) {
            continue;
        }
        if (missing_seen == empty_offset) {
            (void)snprintf(out_path, out_cap, "%s", candidate);
            return core_result_ok();
        }
        missing_seen += 1u;
    }
    return drawing_program_project_slot_path_missing("failed to allocate project slot path");
}

static void drawing_program_project_state_insert_recent(DrawingProgramRecentProjectEntry *entries,
                                                        uint32_t *count,
                                                        const char *path,
                                                        uint64_t modified_time) {
    uint32_t entry_count = 0u;
    uint32_t insert_at = 0u;
    if (!entries || !count || !path || path[0] == '\0') {
        return;
    }
    entry_count = *count;
    while (insert_at < entry_count && entries[insert_at].modified_time >= modified_time) {
        ++insert_at;
    }
    if (insert_at >= DRAWING_PROGRAM_RECENT_PROJECT_CAPACITY) {
        return;
    }
    if (entry_count < DRAWING_PROGRAM_RECENT_PROJECT_CAPACITY) {
        ++entry_count;
    }
    while (entry_count > 0u && (entry_count - 1u) > insert_at) {
        entries[entry_count - 1u] = entries[entry_count - 2u];
        --entry_count;
    }
    (void)snprintf(entries[insert_at].path, sizeof(entries[insert_at].path), "%s", path);
    entries[insert_at].modified_time = modified_time;
    if (*count < DRAWING_PROGRAM_RECENT_PROJECT_CAPACITY) {
        *count += 1u;
    }
}

CoreResult drawing_program_project_state_refresh_recent(struct DrawingProgramAppContext *ctx) {
    DIR *dir = 0;
    struct dirent *entry = 0;
    DrawingProgramRecentProjectEntry recent[DRAWING_PROGRAM_RECENT_PROJECT_CAPACITY];
    uint32_t recent_count = 0u;
    uint32_t i;
    if (!ctx) {
        return drawing_program_project_invalid("invalid recent project refresh request");
    }
    memset(recent, 0, sizeof(recent));
    ctx->session.recent_project_count = 0u;
    for (i = 0u; i < DRAWING_PROGRAM_RECENT_PROJECT_CAPACITY; ++i) {
        ctx->session.recent_project_paths[i][0] = '\0';
    }
    if (!ctx->session.input_root_path[0]) {
        return core_result_ok();
    }
    dir = opendir(ctx->session.input_root_path);
    if (!dir) {
        if (errno == ENOENT) {
            return core_result_ok();
        }
        return (CoreResult){ CORE_ERR_IO, "failed to read input root for recent projects" };
    }
    while ((entry = readdir(dir)) != 0) {
        char full_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        struct stat st;
        if (entry->d_name[0] == '.') {
            continue;
        }
        if (!drawing_program_project_path_has_pack_extension(entry->d_name)) {
            continue;
        }
        (void)snprintf(full_path, sizeof(full_path), "%s/%s", ctx->session.input_root_path, entry->d_name);
        if (stat(full_path, &st) != 0 || !S_ISREG(st.st_mode)) {
            continue;
        }
        drawing_program_project_state_insert_recent(recent, &recent_count, full_path, (uint64_t)st.st_mtime);
    }
    (void)closedir(dir);
    for (i = 0u; i < recent_count; ++i) {
        (void)snprintf(ctx->session.recent_project_paths[i],
                       sizeof(ctx->session.recent_project_paths[i]),
                       "%s",
                       recent[i].path);
    }
    ctx->session.recent_project_count = (uint16_t)recent_count;
    return core_result_ok();
}

CoreResult drawing_program_project_state_prepare_new_path(struct DrawingProgramAppContext *ctx) {
    CoreResult result;
    if (!ctx) {
        return drawing_program_project_invalid("invalid new project request");
    }
    result = drawing_program_project_mkdirs_if_needed(ctx->session.input_root_path);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_project_state_nth_empty_path(ctx, 0u, ctx->session.project_path_buffer,
                                                          sizeof(ctx->session.project_path_buffer));
    if (result.code != CORE_OK) {
        return result;
    }
    ctx->session.project_path = ctx->session.project_path_buffer;
    ctx->session.project_has_saved_state = 0u;
    return core_result_ok();
}

CoreResult drawing_program_project_state_begin_new_blank(struct DrawingProgramAppContext *ctx) {
    DrawingProgramDocument next_document;
    DrawingProgramLayerRasterStore next_layer_rasters;
    uint32_t logical_width = DRAWING_PROGRAM_DEFAULT_LOGICAL_WIDTH;
    uint32_t logical_height = DRAWING_PROGRAM_DEFAULT_LOGICAL_HEIGHT;
    CoreResult result;
    if (!ctx) {
        return drawing_program_project_invalid("invalid new blank project request");
    }
    result = drawing_program_project_state_prepare_new_path(ctx);
    if (result.code != CORE_OK) {
        return result;
    }
    if (ctx->session.seed_canvas_logical_width > 0u) {
        logical_width = ctx->session.seed_canvas_logical_width;
    } else if (ctx->document.logical_width > 0u) {
        logical_width = ctx->document.logical_width;
    }
    if (ctx->session.seed_canvas_logical_height > 0u) {
        logical_height = ctx->session.seed_canvas_logical_height;
    } else if (ctx->document.logical_height > 0u) {
        logical_height = ctx->document.logical_height;
    }
    memset(&next_document, 0, sizeof(next_document));
    memset(&next_layer_rasters, 0, sizeof(next_layer_rasters));
    result = drawing_program_document_init_with_shape(&next_document, logical_width, logical_height, 1u);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_layer_raster_store_init_from_document(&next_layer_rasters, &next_document);
    if (result.code != CORE_OK) {
        drawing_program_layer_raster_store_dispose(&next_layer_rasters);
        return result;
    }
    drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
    ctx->document = next_document;
    ctx->layer_rasters = next_layer_rasters;
    drawing_program_object_store_reset(&ctx->object_store);
    drawing_program_object_selection_reset(&ctx->object_selection);
    drawing_program_editor_state_init(&ctx->editor, &ctx->document);
    drawing_program_history_init(&ctx->history);
    drawing_program_selection_reset(&ctx->selection);
    drawing_program_clipboard_reset(&ctx->clipboard);
    drawing_program_app_rearm_after_document_swap(ctx);
    ctx->session.project_saved_history_count = 0u;
    ctx->session.project_saved_history_cursor = 0u;
    ctx->session.project_has_saved_state = 0u;
    return core_result_ok();
}

CoreResult drawing_program_project_state_configure_defaults(struct DrawingProgramAppContext *ctx) {
    CoreResult result;
    if (!ctx) {
        return drawing_program_project_invalid("invalid project default configuration request");
    }
    result = drawing_program_project_state_refresh_recent(ctx);
    if (result.code != CORE_OK) {
        return result;
    }
    if (ctx->session.project_path && ctx->session.project_path[0] != '\0') {
        return core_result_ok();
    }
    if (ctx->session.recent_project_count > 0u) {
        drawing_program_project_state_copy_current_path(ctx, ctx->session.recent_project_paths[0]);
        return core_result_ok();
    }
    return drawing_program_project_state_prepare_new_path(ctx);
}

CoreResult drawing_program_project_state_select_recent(struct DrawingProgramAppContext *ctx, uint32_t recent_index) {
    return drawing_program_project_state_select_slot(ctx, recent_index);
}

CoreResult drawing_program_project_state_slot_path(const struct DrawingProgramAppContext *ctx,
                                                   uint32_t slot_index,
                                                   char *out_path,
                                                   size_t out_cap,
                                                   uint8_t *out_existing) {
    if (!ctx || !out_path || out_cap == 0u || slot_index >= DRAWING_PROGRAM_RECENT_PROJECT_CAPACITY) {
        return drawing_program_project_invalid("invalid project slot lookup request");
    }
    if (slot_index < ctx->session.recent_project_count) {
        (void)snprintf(out_path, out_cap, "%s", ctx->session.recent_project_paths[slot_index]);
        if (out_existing) {
            *out_existing = 1u;
        }
        return core_result_ok();
    }
    if (out_existing) {
        *out_existing = 0u;
    }
    return drawing_program_project_state_nth_empty_path(ctx,
                                                        slot_index - (uint32_t)ctx->session.recent_project_count,
                                                        out_path,
                                                        out_cap);
}

CoreResult drawing_program_project_state_select_slot(struct DrawingProgramAppContext *ctx, uint32_t slot_index) {
    char slot_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx || slot_index >= DRAWING_PROGRAM_RECENT_PROJECT_CAPACITY) {
        return drawing_program_project_invalid("invalid project slot selection request");
    }
    result = drawing_program_project_state_slot_path(ctx, slot_index, slot_path, sizeof(slot_path), 0);
    if (result.code != CORE_OK) {
        return result;
    }
    drawing_program_project_state_copy_current_path(ctx, slot_path);
    return core_result_ok();
}

CoreResult drawing_program_project_state_set_current_path(struct DrawingProgramAppContext *ctx, const char *path) {
    return drawing_program_project_state_apply_current_path(ctx, path);
}

CoreResult drawing_program_project_state_set_save_as_path(struct DrawingProgramAppContext *ctx, const char *path) {
    char normalized_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx || !path || path[0] == '\0') {
        return drawing_program_project_invalid("invalid project save as path request");
    }
    result = drawing_program_project_normalize_path(path, 1u, normalized_path, sizeof(normalized_path));
    if (result.code != CORE_OK) {
        return result;
    }
    return drawing_program_project_state_apply_current_path(ctx, normalized_path);
}

CoreResult drawing_program_project_state_set_input_root(struct DrawingProgramAppContext *ctx, const char *path) {
    CoreResult result;
    if (!ctx || !path || path[0] == '\0') {
        return drawing_program_project_invalid("invalid input root update request");
    }
    if (strlen(path) >= sizeof(ctx->session.input_root_path)) {
        return drawing_program_project_invalid("input root path too long");
    }
    (void)snprintf(ctx->session.input_root_path, sizeof(ctx->session.input_root_path), "%s", path);
    drawing_program_project_trim_dir_path(ctx->session.input_root_path);
    result = drawing_program_project_mkdirs_if_needed(ctx->session.input_root_path);
    if (result.code != CORE_OK) {
        return result;
    }
    ctx->session.project_path = 0;
    ctx->session.project_path_buffer[0] = '\0';
    ctx->session.project_has_saved_state = 0u;
    ctx->session.recent_project_count = 0u;
    return drawing_program_project_state_configure_defaults(ctx);
}

CoreResult drawing_program_project_state_set_output_root(struct DrawingProgramAppContext *ctx, const char *path) {
    CoreResult result;
    if (!ctx || !path || path[0] == '\0') {
        return drawing_program_project_invalid("invalid output root update request");
    }
    if (strlen(path) >= sizeof(ctx->session.output_root_path)) {
        return drawing_program_project_invalid("output root path too long");
    }
    (void)snprintf(ctx->session.output_root_path, sizeof(ctx->session.output_root_path), "%s", path);
    drawing_program_project_trim_dir_path(ctx->session.output_root_path);
    result = drawing_program_project_mkdirs_if_needed(ctx->session.output_root_path);
    if (result.code != CORE_OK) {
        return result;
    }
    return core_result_ok();
}

CoreResult drawing_program_project_state_save_current(struct DrawingProgramAppContext *ctx) {
    CoreResult result;
    if (!ctx || !ctx->session.project_path || ctx->session.project_path[0] == '\0') {
        return drawing_program_project_invalid("invalid project save request");
    }
    result = drawing_program_project_ensure_parent_dir(ctx->session.project_path);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_snapshot_save(ctx, ctx->session.project_path);
    if (result.code != CORE_OK) {
        return result;
    }
    drawing_program_project_state_mark_clean(ctx);
    return drawing_program_project_state_refresh_recent(ctx);
}

CoreResult drawing_program_project_state_load_current(struct DrawingProgramAppContext *ctx) {
    CoreResult result;
    uint8_t exists = 0u;
    if (!ctx || !ctx->session.project_path || ctx->session.project_path[0] == '\0') {
        return drawing_program_project_invalid("invalid project load request");
    }
    result = drawing_program_project_state_current_exists(ctx, &exists);
    if (result.code != CORE_OK) {
        return result;
    }
    if (!exists) {
        return drawing_program_project_slot_path_missing("selected project target does not exist");
    }
    result = drawing_program_snapshot_load(ctx, ctx->session.project_path);
    if (result.code != CORE_OK) {
        return result;
    }
    drawing_program_project_state_mark_clean(ctx);
    return drawing_program_project_state_refresh_recent(ctx);
}

void drawing_program_project_state_mark_clean(struct DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    ctx->session.project_saved_history_count = ctx->history.count;
    ctx->session.project_saved_history_cursor = ctx->history.cursor;
    ctx->session.project_has_saved_state = 1u;
}

uint8_t drawing_program_project_state_current_is_dirty(const struct DrawingProgramAppContext *ctx) {
    if (!ctx || !ctx->session.project_path || ctx->session.project_path[0] == '\0') {
        return 1u;
    }
    if (!ctx->session.project_has_saved_state) {
        return 1u;
    }
    if (ctx->session.project_saved_history_count != ctx->history.count) {
        return 1u;
    }
    if (ctx->session.project_saved_history_cursor != ctx->history.cursor) {
        return 1u;
    }
    return 0u;
}
