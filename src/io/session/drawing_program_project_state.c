#include "drawing_program/drawing_program_project_state.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "drawing_program/drawing_program_app_post_load.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_snapshot.h"
#include "drawing_program_project_state_internal.h"

CoreResult drawing_program_project_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

CoreResult drawing_program_project_slot_path_missing(const char *message) {
    CoreResult r = { CORE_ERR_NOT_FOUND, message };
    return r;
}

void drawing_program_project_trim_dir_path(char *path) {
    size_t len = 0u;
    if (!path) {
        return;
    }
    len = strlen(path);
    while (len > 1u && path[len - 1u] == '/') {
        path[--len] = '\0';
    }
}

int drawing_program_project_path_has_pack_extension(const char *path) {
    const char *dot = 0;
    if (!path || path[0] == '\0') {
        return 0;
    }
    dot = strrchr(path, '.');
    return (dot && strcmp(dot, ".pack") == 0) ? 1 : 0;
}

void drawing_program_project_sanitize_component(const char *input, char *out, size_t out_cap) {
    size_t in_i = 0u;
    size_t out_i = 0u;
    int last_was_separator = 0;
    if (!out || out_cap == 0u) {
        return;
    }
    out[0] = '\0';
    if (!input || input[0] == '\0') {
        return;
    }
    for (in_i = 0u; input[in_i] != '\0' && out_i + 1u < out_cap; ++in_i) {
        unsigned char c = (unsigned char)input[in_i];
        int is_safe =
            ((c >= (unsigned char)'a' && c <= (unsigned char)'z') ||
             (c >= (unsigned char)'A' && c <= (unsigned char)'Z') ||
             (c >= (unsigned char)'0' && c <= (unsigned char)'9'));
        if (is_safe || c == (unsigned char)'_' || c == (unsigned char)'-') {
            out[out_i++] = (char)c;
            last_was_separator = 0;
            continue;
        }
        if (!last_was_separator) {
            out[out_i++] = '_';
            last_was_separator = 1;
        }
    }
    while (out_i > 0u && (out[out_i - 1u] == '_' || out[out_i - 1u] == '-')) {
        out_i -= 1u;
    }
    out[out_i] = '\0';
}

void drawing_program_project_scene_stem_from_path(const char *path, char *out_stem, size_t out_cap) {
    const char *base = 0;
    const char *dot = 0;
    size_t len = 0u;
    char raw_stem[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
    if (!out_stem || out_cap == 0u) {
        return;
    }
    out_stem[0] = '\0';
    if (!path || path[0] == '\0') {
        return;
    }
    base = strrchr(path, '/');
    base = base ? (base + 1) : path;
    dot = strrchr(base, '.');
    len = dot && dot > base ? (size_t)(dot - base) : strlen(base);
    if (len == 0u) {
        return;
    }
    if (len >= sizeof(raw_stem)) {
        len = sizeof(raw_stem) - 1u;
    }
    memcpy(raw_stem, base, len);
    raw_stem[len] = '\0';
    drawing_program_project_sanitize_component(raw_stem, out_stem, out_cap);
}

CoreResult drawing_program_project_mkdirs_if_needed(const char *dir_path) {
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

CoreResult drawing_program_project_ensure_parent_dir(const char *file_path) {
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

CoreResult drawing_program_project_copy_parent_dir(const char *file_path, char *out_dir, size_t out_cap) {
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

CoreResult drawing_program_project_normalize_path(const char *path,
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

void drawing_program_project_state_copy_current_path(struct DrawingProgramAppContext *ctx, const char *path) {
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

CoreResult drawing_program_project_state_set_scene_authored_root(struct DrawingProgramAppContext *ctx, const char *path) {
    CoreResult result;
    if (!ctx || !path || path[0] == '\0') {
        return drawing_program_project_invalid("invalid scene authored root update request");
    }
    if (strlen(path) >= sizeof(ctx->session.scene_authored_root_path)) {
        return drawing_program_project_invalid("scene authored root path too long");
    }
    (void)snprintf(ctx->session.scene_authored_root_path, sizeof(ctx->session.scene_authored_root_path), "%s", path);
    drawing_program_project_trim_dir_path(ctx->session.scene_authored_root_path);
    result = drawing_program_project_mkdirs_if_needed(ctx->session.scene_authored_root_path);
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
