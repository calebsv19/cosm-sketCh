#include "drawing_program/drawing_program_session_prefs.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_project_state.h"

enum {
    DRAWING_PROGRAM_SESSION_PREFS_VERSION = 1u
};

static CoreResult drawing_program_session_prefs_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static void drawing_program_session_prefs_trim_newline(char *text) {
    size_t len = 0u;
    if (!text) {
        return;
    }
    len = strlen(text);
    while (len > 0u && (text[len - 1u] == '\n' || text[len - 1u] == '\r')) {
        text[--len] = '\0';
    }
}

static CoreResult drawing_program_session_prefs_path(const struct DrawingProgramAppContext *ctx,
                                                     char *out_path,
                                                     size_t out_cap) {
    if (!ctx || !out_path || out_cap == 0u) {
        return drawing_program_session_prefs_invalid("invalid session prefs path request");
    }
    if (!ctx->session.runtime_root_path[0]) {
        return drawing_program_session_prefs_invalid("runtime root missing for session prefs");
    }
    if (snprintf(out_path, out_cap, "%s/session_prefs_v1.txt", ctx->session.runtime_root_path) >= (int)out_cap) {
        return drawing_program_session_prefs_invalid("session prefs path too long");
    }
    return core_result_ok();
}

CoreResult drawing_program_session_prefs_load(struct DrawingProgramAppContext *ctx) {
    char prefs_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    char input_root[sizeof(ctx->session.input_root_path)];
    char output_root[sizeof(ctx->session.output_root_path)];
    char project_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    FILE *prefs = 0;
    char line[1024];
    CoreResult result;
    input_root[0] = '\0';
    output_root[0] = '\0';
    project_path[0] = '\0';
    if (!ctx) {
        return drawing_program_session_prefs_invalid("invalid session prefs load request");
    }
    result = drawing_program_session_prefs_path(ctx, prefs_path, sizeof(prefs_path));
    if (result.code != CORE_OK) {
        return result;
    }
    prefs = fopen(prefs_path, "rb");
    if (!prefs) {
        if (errno == ENOENT) {
            return core_result_ok();
        }
        return (CoreResult){ CORE_ERR_IO, "failed to open session prefs file" };
    }
    while (fgets(line, (int)sizeof(line), prefs)) {
        drawing_program_session_prefs_trim_newline(line);
        if (strncmp(line, "input_root=", 11u) == 0u) {
            (void)snprintf(input_root, sizeof(input_root), "%s", line + 11);
        } else if (strncmp(line, "output_root=", 12u) == 0u) {
            (void)snprintf(output_root, sizeof(output_root), "%s", line + 12);
        } else if (strncmp(line, "project_path=", 13u) == 0u) {
            (void)snprintf(project_path, sizeof(project_path), "%s", line + 13);
        }
    }
    (void)fclose(prefs);
    if (!ctx->session.input_root_cli_override && input_root[0] != '\0') {
        (void)snprintf(ctx->session.input_root_path, sizeof(ctx->session.input_root_path), "%s", input_root);
    }
    if (!ctx->session.output_root_cli_override && output_root[0] != '\0') {
        (void)snprintf(ctx->session.output_root_path, sizeof(ctx->session.output_root_path), "%s", output_root);
    }
    if (!ctx->session.input_root_cli_override && project_path[0] != '\0') {
        result = drawing_program_project_state_set_current_path(ctx, project_path);
        if (result.code != CORE_OK) {
            return result;
        }
    }
    return core_result_ok();
}

CoreResult drawing_program_session_prefs_save(const struct DrawingProgramAppContext *ctx) {
    char prefs_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    FILE *prefs = 0;
    CoreResult result;
    if (!ctx) {
        return drawing_program_session_prefs_invalid("invalid session prefs save request");
    }
    result = drawing_program_session_prefs_path(ctx, prefs_path, sizeof(prefs_path));
    if (result.code != CORE_OK) {
        return result;
    }
    prefs = fopen(prefs_path, "wb");
    if (!prefs) {
        return (CoreResult){ CORE_ERR_IO, "failed to write session prefs file" };
    }
    (void)fprintf(prefs, "version=%u\n", (unsigned)DRAWING_PROGRAM_SESSION_PREFS_VERSION);
    (void)fprintf(prefs, "input_root=%s\n", ctx->session.input_root_path);
    (void)fprintf(prefs, "output_root=%s\n", ctx->session.output_root_path);
    (void)fprintf(prefs, "project_path=%s\n", ctx->session.project_path ? ctx->session.project_path : "");
    if (fclose(prefs) != 0) {
        return (CoreResult){ CORE_ERR_IO, "failed to finalize session prefs file" };
    }
    return core_result_ok();
}
