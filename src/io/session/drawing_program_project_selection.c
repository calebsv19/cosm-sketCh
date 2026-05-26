#include "drawing_program_project_state_internal.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "drawing_program/drawing_program_texture_project_session.h"

typedef struct DrawingProgramRecentProjectEntry {
    char path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    uint64_t modified_time;
} DrawingProgramRecentProjectEntry;

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

static CoreResult drawing_program_project_state_texture_import_path(const struct DrawingProgramAppContext *ctx,
                                                                   const char *scene_id,
                                                                   const char *scene_path,
                                                                   const char *object_id,
                                                                   char *out_path,
                                                                   size_t out_cap) {
    char safe_scene[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
    char safe_object[DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY];
    char base_stem[(DRAWING_PROGRAM_TEXTURE_PROJECT_ID_CAPACITY * 2u) + 8u];
    uint32_t suffix = 0u;
    if (!ctx || !out_path || out_cap == 0u) {
        return drawing_program_project_invalid("invalid texture import project path request");
    }
    safe_scene[0] = '\0';
    safe_object[0] = '\0';
    base_stem[0] = '\0';
    if (scene_id && scene_id[0] != '\0') {
        drawing_program_project_sanitize_component(scene_id, safe_scene, sizeof(safe_scene));
    } else {
        drawing_program_project_scene_stem_from_path(scene_path, safe_scene, sizeof(safe_scene));
    }
    if (object_id && object_id[0] != '\0') {
        drawing_program_project_sanitize_component(object_id, safe_object, sizeof(safe_object));
    }
    if (safe_object[0] == '\0') {
        return drawing_program_project_state_nth_empty_path(ctx, 0u, out_path, out_cap);
    }
    if (safe_scene[0] != '\0') {
        (void)snprintf(base_stem, sizeof(base_stem), "%s__%s", safe_scene, safe_object);
    } else {
        (void)snprintf(base_stem, sizeof(base_stem), "%s", safe_object);
    }
    for (suffix = 0u; suffix < 1000u; ++suffix) {
        char candidate[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        if (suffix == 0u) {
            (void)snprintf(candidate,
                           sizeof(candidate),
                           "%s/%s.pack",
                           ctx->session.input_root_path,
                           base_stem);
        } else {
            (void)snprintf(candidate,
                           sizeof(candidate),
                           "%s/%s_%03u.pack",
                           ctx->session.input_root_path,
                           base_stem,
                           suffix);
        }
        if (access(candidate, F_OK) == 0) {
            continue;
        }
        (void)snprintf(out_path, out_cap, "%s", candidate);
        return core_result_ok();
    }
    return drawing_program_project_slot_path_missing("failed to allocate texture import project path");
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

CoreResult drawing_program_project_state_prepare_texture_import_path(struct DrawingProgramAppContext *ctx,
                                                                    const char *scene_id,
                                                                    const char *scene_path,
                                                                    const char *object_id) {
    CoreResult result;
    if (!ctx) {
        return drawing_program_project_invalid("invalid texture import project path preparation request");
    }
    result = drawing_program_project_mkdirs_if_needed(ctx->session.input_root_path);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_project_state_texture_import_path(ctx,
                                                               scene_id,
                                                               scene_path,
                                                               object_id,
                                                               ctx->session.project_path_buffer,
                                                               sizeof(ctx->session.project_path_buffer));
    if (result.code != CORE_OK) {
        return result;
    }
    ctx->session.project_path = ctx->session.project_path_buffer;
    ctx->session.project_has_saved_state = 0u;
    return core_result_ok();
}

CoreResult drawing_program_project_state_begin_new_blank(struct DrawingProgramAppContext *ctx) {
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
    result = drawing_program_texture_project_session_seed_blank(ctx,
                                                                logical_width,
                                                                logical_height,
                                                                DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD);
    if (result.code != CORE_OK) {
        return result;
    }
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
