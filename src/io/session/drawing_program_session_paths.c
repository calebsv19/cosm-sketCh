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

static void drawing_program_session_paths_seed_data_roots(struct DrawingProgramAppContext *ctx) {
    const char *runtime_env;
    if (!ctx) {
        return;
    }
    runtime_env = getenv("DRAWING_PROGRAM_RUNTIME_DIR");
    if (runtime_env && runtime_env[0] != '\0') {
        if (!ctx->session.runtime_root_cli_override) {
            (void)snprintf(ctx->session.runtime_root_path, sizeof(ctx->session.runtime_root_path), "%s", runtime_env);
        }
        if (!ctx->session.input_root_cli_override) {
            (void)snprintf(
                ctx->session.input_root_path, sizeof(ctx->session.input_root_path), "%s/input", ctx->session.runtime_root_path);
        }
        if (!ctx->session.output_root_cli_override) {
            (void)snprintf(
                ctx->session.output_root_path, sizeof(ctx->session.output_root_path), "%s/output", ctx->session.runtime_root_path);
        }
        if (!ctx->session.scene_authored_root_path[0]) {
            (void)snprintf(ctx->session.scene_authored_root_path,
                           sizeof(ctx->session.scene_authored_root_path),
                           "%s/scene_authored",
                           ctx->session.runtime_root_path);
        }
    }
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
            if (mkdir(buffer, 0775) != 0 && errno != EEXIST) {
                return (CoreResult){ CORE_ERR_IO, "failed to create runtime directory segment" };
            }
        }
        buffer[i] = '/';
    }
    if (mkdir(buffer, 0775) != 0 && errno != EEXIST) {
        return (CoreResult){ CORE_ERR_IO, "failed to create runtime directory" };
    }
    return core_result_ok();
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
    drawing_program_session_paths_seed_data_roots(ctx);
    result = drawing_program_session_paths_mkdirs_if_needed(ctx->session.runtime_root_path);
    if (result.code != CORE_OK) {
        return result;
    }
    if (ctx->session.persist_enabled) {
        result = drawing_program_session_prefs_load(ctx);
        if (result.code != CORE_OK) {
            return result;
        }
    }
    if (!ctx->session.scene_authored_root_path[0]) {
        (void)snprintf(ctx->session.scene_authored_root_path,
                       sizeof(ctx->session.scene_authored_root_path),
                       "%s/scene_authored",
                       ctx->session.runtime_root_path);
    }
    result = drawing_program_session_paths_mkdirs_if_needed(ctx->session.input_root_path);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_session_paths_mkdirs_if_needed(ctx->session.output_root_path);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_session_paths_mkdirs_if_needed(ctx->session.scene_authored_root_path);
    if (result.code != CORE_OK) {
        return result;
    }
    if (!ctx->session.preset_path_cli_override) {
        (void)snprintf(
            ctx->session.preset_path_buffer, sizeof(ctx->session.preset_path_buffer), "%s/last_session.pack", ctx->session.runtime_root_path);
        ctx->session.preset_path = ctx->session.preset_path_buffer;
    }
    result = drawing_program_project_state_configure_defaults(ctx);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_session_paths_ensure_parent_dir(ctx->session.preset_path);
    if (result.code != CORE_OK) {
        return result;
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
            return result;
        }
    }
    return core_result_ok();
}
