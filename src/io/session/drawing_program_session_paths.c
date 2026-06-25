#include "drawing_program/drawing_program_session_paths.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_project_state.h"
#include "drawing_program/drawing_program_session_prefs.h"

static CoreResult drawing_program_session_paths_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static CoreResult drawing_program_session_paths_copy(char *out,
                                                     size_t out_cap,
                                                     const char *path,
                                                     const char *role) {
    int written;
    if (!out || out_cap == 0u || !path) {
        return drawing_program_session_paths_invalid("invalid session path copy request");
    }
    written = snprintf(out, out_cap, "%s", path);
    if (written < 0 || (size_t)written >= out_cap) {
        return drawing_program_session_paths_invalid(role ? role : "session path too long");
    }
    return core_result_ok();
}

static CoreResult drawing_program_session_paths_join_child(char *out,
                                                           size_t out_cap,
                                                           const char *root,
                                                           const char *child,
                                                           const char *role) {
    int written;
    if (!out || out_cap == 0u || !root || !child || root[0] == '\0' || child[0] == '\0') {
        return drawing_program_session_paths_invalid("invalid session child path request");
    }
    written = snprintf(out, out_cap, "%s/%s", root, child);
    if (written < 0 || (size_t)written >= out_cap) {
        return drawing_program_session_paths_invalid(role ? role : "session child path too long");
    }
    return core_result_ok();
}

static CoreResult drawing_program_session_paths_context_result(const char *role,
                                                               const char *path,
                                                               CoreResult cause) {
    static char message[768];
    (void)snprintf(message,
                   sizeof(message),
                   "session path failure role=%s path=%s detail=%s",
                   role ? role : "unknown",
                   path ? path : "(null)",
                   cause.message ? cause.message : "unknown");
    return (CoreResult){ cause.code, message };
}

static CoreResult drawing_program_session_paths_mkdir_one(const char *dir_path, const char *kind) {
    struct stat st;
    if (mkdir(dir_path, 0775) == 0 || errno == EEXIST) {
        if (stat(dir_path, &st) != 0) {
            return (CoreResult){ CORE_ERR_IO, "directory path exists but cannot be inspected" };
        }
        if (!S_ISDIR(st.st_mode)) {
            return (CoreResult){ CORE_ERR_IO,
                                 kind && strcmp(kind, "segment") == 0
                                     ? "directory segment exists but is not a directory"
                                     : "directory path exists but is not a directory" };
        }
        return core_result_ok();
    }
    return (CoreResult){ CORE_ERR_IO,
                         kind && strcmp(kind, "segment") == 0
                             ? "failed to create directory segment"
                             : "failed to create directory" };
}

static CoreResult drawing_program_session_paths_seed_data_roots(struct DrawingProgramAppContext *ctx) {
    const char *runtime_env;
    if (!ctx) {
        return drawing_program_session_paths_invalid("null app context");
    }
    runtime_env = getenv("DRAWING_PROGRAM_RUNTIME_DIR");
    if (runtime_env && runtime_env[0] != '\0') {
        if (!ctx->session.runtime_root_cli_override) {
            CoreResult result = drawing_program_session_paths_copy(ctx->session.runtime_root_path,
                                                                   sizeof(ctx->session.runtime_root_path),
                                                                   runtime_env,
                                                                   "runtime root path too long");
            if (result.code != CORE_OK) {
                return drawing_program_session_paths_context_result("runtime_root", runtime_env, result);
            }
        }
        if (!ctx->session.input_root_cli_override) {
            CoreResult result = drawing_program_session_paths_join_child(ctx->session.input_root_path,
                                                                         sizeof(ctx->session.input_root_path),
                                                                         ctx->session.runtime_root_path,
                                                                         "input",
                                                                         "input root path too long");
            if (result.code != CORE_OK) {
                return drawing_program_session_paths_context_result(
                    "input_root", ctx->session.runtime_root_path, result);
            }
        }
        if (!ctx->session.output_root_cli_override) {
            CoreResult result = drawing_program_session_paths_join_child(ctx->session.output_root_path,
                                                                         sizeof(ctx->session.output_root_path),
                                                                         ctx->session.runtime_root_path,
                                                                         "output",
                                                                         "output root path too long");
            if (result.code != CORE_OK) {
                return drawing_program_session_paths_context_result(
                    "output_root", ctx->session.runtime_root_path, result);
            }
        }
        if (!ctx->session.scene_authored_root_path[0]) {
            CoreResult result = drawing_program_session_paths_join_child(ctx->session.scene_authored_root_path,
                                                                         sizeof(ctx->session.scene_authored_root_path),
                                                                         ctx->session.runtime_root_path,
                                                                         "scene_authored",
                                                                         "scene authored root path too long");
            if (result.code != CORE_OK) {
                return drawing_program_session_paths_context_result(
                    "scene_authored_root", ctx->session.runtime_root_path, result);
            }
        }
    }
    return core_result_ok();
}

static CoreResult drawing_program_session_paths_mkdirs_if_needed(const char *dir_path) {
    char buffer[512];
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
            CoreResult segment_result = drawing_program_session_paths_mkdir_one(buffer, "segment");
            if (segment_result.code != CORE_OK) {
                return segment_result;
            }
        }
        buffer[i] = '/';
    }
    return drawing_program_session_paths_mkdir_one(buffer, "directory");
}

CoreResult drawing_program_session_paths_ensure_parent_dir(const char *file_path) {
    char buffer[512];
    char *slash;
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
    return drawing_program_session_paths_mkdirs_if_needed(buffer);
}

CoreResult drawing_program_session_paths_configure(struct DrawingProgramAppContext *ctx) {
    CoreResult result;
    if (!ctx) {
        return drawing_program_session_paths_invalid("null app context");
    }
    result = drawing_program_session_paths_seed_data_roots(ctx);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_session_paths_mkdirs_if_needed(ctx->session.runtime_root_path);
    if (result.code != CORE_OK) {
        return drawing_program_session_paths_context_result("runtime_root", ctx->session.runtime_root_path, result);
    }
    if (ctx->session.persist_enabled) {
        result = drawing_program_session_prefs_load(ctx);
        if (result.code != CORE_OK) {
            return drawing_program_session_paths_context_result("session_prefs", ctx->session.runtime_root_path, result);
        }
    }
    if (!ctx->session.scene_authored_root_path[0]) {
        result = drawing_program_session_paths_join_child(ctx->session.scene_authored_root_path,
                                                          sizeof(ctx->session.scene_authored_root_path),
                                                          ctx->session.runtime_root_path,
                                                          "scene_authored",
                                                          "scene authored root path too long");
        if (result.code != CORE_OK) {
            return drawing_program_session_paths_context_result(
                "scene_authored_root", ctx->session.runtime_root_path, result);
        }
    }
    result = drawing_program_session_paths_mkdirs_if_needed(ctx->session.input_root_path);
    if (result.code != CORE_OK) {
        return drawing_program_session_paths_context_result("input_root", ctx->session.input_root_path, result);
    }
    result = drawing_program_session_paths_mkdirs_if_needed(ctx->session.output_root_path);
    if (result.code != CORE_OK) {
        return drawing_program_session_paths_context_result("output_root", ctx->session.output_root_path, result);
    }
    result = drawing_program_session_paths_mkdirs_if_needed(ctx->session.scene_authored_root_path);
    if (result.code != CORE_OK) {
        return drawing_program_session_paths_context_result(
            "scene_authored_root", ctx->session.scene_authored_root_path, result);
    }
    if (!ctx->session.preset_path_cli_override) {
        result = drawing_program_session_paths_join_child(ctx->session.preset_path_buffer,
                                                          sizeof(ctx->session.preset_path_buffer),
                                                          ctx->session.runtime_root_path,
                                                          "last_session.pack",
                                                          "preset path too long");
        if (result.code != CORE_OK) {
            return drawing_program_session_paths_context_result("preset_path",
                                                               ctx->session.runtime_root_path,
                                                               result);
        }
        ctx->session.preset_path = ctx->session.preset_path_buffer;
    }
    result = drawing_program_project_state_configure_defaults(ctx);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_session_paths_ensure_parent_dir(ctx->session.preset_path);
    if (result.code != CORE_OK) {
        return drawing_program_session_paths_context_result("preset_path_parent", ctx->session.preset_path, result);
    }
    if (getenv("DRAWING_PROGRAM_TRACE_UI_STATE")) {
        fprintf(stderr,
                "drawing_program trace config_load runtime_root=%s input_root=%s output_root=%s preset_path=%s project_path=%s preset_ptr=%p project_ptr=%p\n",
                ctx->session.runtime_root_path,
                ctx->session.input_root_path,
                ctx->session.output_root_path,
                ctx->session.preset_path ? ctx->session.preset_path : "(null)",
                ctx->session.project_path ? ctx->session.project_path : "(null)",
                (const void *)ctx->session.preset_path,
                (const void *)ctx->session.project_path);
    }
    if (ctx->session.export_json_path) {
        result = drawing_program_session_paths_ensure_parent_dir(ctx->session.export_json_path);
        if (result.code != CORE_OK) {
            return drawing_program_session_paths_context_result(
                "export_json_parent", ctx->session.export_json_path, result);
        }
    }
    return core_result_ok();
}
