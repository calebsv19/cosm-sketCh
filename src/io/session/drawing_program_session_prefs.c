#include "drawing_program/drawing_program_session_prefs.h"

#include <stdlib.h>
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

static int drawing_program_session_prefs_parse_u32(const char *text, uint32_t *out_value) {
    char *end = 0;
    unsigned long parsed = 0ul;
    if (!text || !text[0] || !out_value) {
        return 0;
    }
    parsed = strtoul(text, &end, 10);
    if (!end || *end != '\0' || parsed > 0xfffffffful) {
        return 0;
    }
    *out_value = (uint32_t)parsed;
    return 1;
}

static int drawing_program_session_prefs_parse_i32(const char *text, int32_t *out_value) {
    char *end = 0;
    long parsed = 0l;
    if (!text || !text[0] || !out_value) {
        return 0;
    }
    parsed = strtol(text, &end, 10);
    if (!end || *end != '\0') {
        return 0;
    }
    if (parsed < -2147483647l - 1l || parsed > 2147483647l) {
        return 0;
    }
    *out_value = (int32_t)parsed;
    return 1;
}

CoreResult drawing_program_session_prefs_load(struct DrawingProgramAppContext *ctx) {
    char prefs_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    char input_root[sizeof(ctx->session.input_root_path)];
    char output_root[sizeof(ctx->session.output_root_path)];
    char scene_root[sizeof(ctx->session.scene_authored_root_path)];
    char project_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    uint32_t ui_theme_preset_id = 0u;
    uint32_t ui_font_preset_id = 0u;
    int32_t ui_font_zoom_step = 0;
    uint8_t ui_theme_loaded = 0u;
    uint8_t ui_font_loaded = 0u;
    uint8_t ui_zoom_loaded = 0u;
    FILE *prefs = 0;
    char line[1024];
    CoreResult result;
    input_root[0] = '\0';
    output_root[0] = '\0';
    scene_root[0] = '\0';
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
        } else if (strncmp(line, "scene_authored_root=", 20u) == 0u) {
            (void)snprintf(scene_root, sizeof(scene_root), "%s", line + 20);
        } else if (strncmp(line, "project_path=", 13u) == 0u) {
            (void)snprintf(project_path, sizeof(project_path), "%s", line + 13);
        } else if (strncmp(line, "ui_theme_preset_id=", 19u) == 0u) {
            ui_theme_loaded =
                drawing_program_session_prefs_parse_u32(line + 19, &ui_theme_preset_id) ? 1u : 0u;
        } else if (strncmp(line, "ui_font_preset_id=", 18u) == 0u) {
            ui_font_loaded =
                drawing_program_session_prefs_parse_u32(line + 18, &ui_font_preset_id) ? 1u : 0u;
        } else if (strncmp(line, "ui_font_zoom_step=", 18u) == 0u) {
            ui_zoom_loaded =
                drawing_program_session_prefs_parse_i32(line + 18, &ui_font_zoom_step) ? 1u : 0u;
        }
    }
    (void)fclose(prefs);
    if (!ctx->session.input_root_cli_override && input_root[0] != '\0') {
        (void)snprintf(ctx->session.input_root_path, sizeof(ctx->session.input_root_path), "%s", input_root);
    }
    if (!ctx->session.output_root_cli_override && output_root[0] != '\0') {
        (void)snprintf(ctx->session.output_root_path, sizeof(ctx->session.output_root_path), "%s", output_root);
    }
    if (scene_root[0] != '\0') {
        (void)snprintf(ctx->session.scene_authored_root_path,
                       sizeof(ctx->session.scene_authored_root_path),
                       "%s",
                       scene_root);
    }
    if (!ctx->session.input_root_cli_override && project_path[0] != '\0') {
        result = drawing_program_project_state_set_current_path(ctx, project_path);
        if (result.code != CORE_OK) {
            return result;
        }
    }
    if (ui_theme_loaded) {
        ctx->session.ui_theme_preset_id = ui_theme_preset_id;
    }
    if (ui_font_loaded) {
        ctx->session.ui_font_preset_id = ui_font_preset_id;
    }
    if (ui_zoom_loaded) {
        ctx->session.ui_font_zoom_step = (int8_t)ui_font_zoom_step;
    }
    ctx->session.ui_prefs_loaded = (ui_theme_loaded || ui_font_loaded || ui_zoom_loaded) ? 1u : 0u;
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
    (void)fprintf(prefs, "scene_authored_root=%s\n", ctx->session.scene_authored_root_path);
    (void)fprintf(prefs, "project_path=%s\n", ctx->session.project_path ? ctx->session.project_path : "");
    (void)fprintf(prefs, "ui_theme_preset_id=%u\n", (unsigned)ctx->ui.theme_preset_id);
    (void)fprintf(prefs, "ui_font_preset_id=%u\n", (unsigned)ctx->ui.font_preset_id);
    (void)fprintf(prefs, "ui_font_zoom_step=%d\n", (int)ctx->ui.font_zoom_step);
    if (fclose(prefs) != 0) {
        return (CoreResult){ CORE_ERR_IO, "failed to finalize session prefs file" };
    }
    return core_result_ok();
}
