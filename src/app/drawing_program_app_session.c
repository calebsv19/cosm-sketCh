#include "drawing_program/drawing_program_app_main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_font.h"
#include "core_theme.h"
#include "drawing_program/drawing_program_project_state.h"
#include "drawing_program/drawing_program_session_paths.h"
#include "drawing_program/drawing_program_session_prefs.h"
#include "drawing_program/drawing_program_ui_color_state.h"

static CoreResult drawing_program_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static int drawing_program_parse_u32_strict(const char *text, uint32_t *out_value) {
    char *end = 0;
    unsigned long parsed = 0ul;
    if (!text || !text[0] || !out_value) {
        return 0;
    }
    parsed = strtoul(text, &end, 10);
    if (!end || *end != '\0') {
        return 0;
    }
    if (parsed == 0ul || parsed > 65535ul) {
        return 0;
    }
    *out_value = (uint32_t)parsed;
    return 1;
}

static int drawing_program_parse_canvas_size(const char *text,
                                             uint32_t *out_width,
                                             uint32_t *out_height) {
    const char *sep = 0;
    char width_buf[16];
    char height_buf[16];
    size_t width_len;
    size_t height_len;
    uint32_t width = 0u;
    uint32_t height = 0u;
    if (!text || !out_width || !out_height) {
        return 0;
    }
    sep = strchr(text, 'x');
    if (!sep) {
        sep = strchr(text, 'X');
    }
    if (!sep || sep == text || sep[1] == '\0') {
        return 0;
    }
    width_len = (size_t)(sep - text);
    height_len = strlen(sep + 1);
    if (width_len == 0u || height_len == 0u ||
        width_len >= sizeof(width_buf) || height_len >= sizeof(height_buf)) {
        return 0;
    }
    memcpy(width_buf, text, width_len);
    width_buf[width_len] = '\0';
    memcpy(height_buf, sep + 1, height_len);
    height_buf[height_len] = '\0';
    if (!drawing_program_parse_u32_strict(width_buf, &width) ||
        !drawing_program_parse_u32_strict(height_buf, &height)) {
        return 0;
    }
    *out_width = width;
    *out_height = height;
    return 1;
}

static int drawing_program_parse_texture_quality_preset(const char *text, uint32_t *out_quality_preset) {
    if (!text || !out_quality_preset) {
        return 0;
    }
    if (strcmp(text, "standard") == 0) {
        *out_quality_preset = DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD;
        return 1;
    }
    if (strcmp(text, "high") == 0) {
        *out_quality_preset = DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_HIGH;
        return 1;
    }
    if (strcmp(text, "ultra") == 0) {
        *out_quality_preset = DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_ULTRA;
        return 1;
    }
    if (strcmp(text, "custom") == 0) {
        *out_quality_preset = DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_CUSTOM;
        return 1;
    }
    return 0;
}

CoreResult drawing_program_app_bootstrap(DrawingProgramAppContext *ctx, int argc, char **argv) {
    int i;
    if (!ctx) {
        return drawing_program_invalid("null app context");
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->session.smoke_frames = 1u;
    ctx->session.persist_enabled = 1u;
    ctx->session.seed_canvas_logical_width = DRAWING_PROGRAM_DEFAULT_LOGICAL_WIDTH;
    ctx->session.seed_canvas_logical_height = DRAWING_PROGRAM_DEFAULT_LOGICAL_HEIGHT;
    ctx->session.preset_path = 0;
    ctx->session.project_path = 0;
    ctx->session.export_json_path = 0;
    ctx->session.bridge_workspace_preset_path = "workspace_sandbox/data/presets/sketch_layout_v1.pack";
    ctx->session.texture_scene_import_quality_preset = DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_HIGH;
    ctx->pane_host_bounds_width = 1200.0f;
    ctx->pane_host_bounds_height = 800.0f;
    ctx->ui.theme_preset_id = (uint32_t)CORE_THEME_PRESET_DARK_DEFAULT;
    ctx->ui.font_preset_id = (uint32_t)CORE_FONT_PRESET_IDE;
    ctx->ui.left_panel_slot = 0u;
    ctx->ui.right_panel_slot = 0u;
    ctx->ui.tool_brush_size = 2u;
    ctx->ui.tool_brush_opacity = 100u;
    ctx->ui.tool_brush_spacing = 2u;
    ctx->ui.tool_brush_hardness = 100u;
    ctx->ui.tool_eraser_size = 4u;
    ctx->ui.tool_shape_stroke_width = 1u;
    ctx->ui.tool_shape_mode = 0u;
    ctx->ui.tool_shape_target_mode = (uint8_t)DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL;
    ctx->ui.tool_fill_tolerance = 0u;
    ctx->ui.tool_select_mode = (uint8_t)DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE;
    ctx->ui.canvas_guide_mode = (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_OFF;
    ctx->ui.font_zoom_step = 0;
    drawing_program_ui_color_seed_defaults(ctx);
    (void)snprintf(
        ctx->session.file_action_status_message, sizeof(ctx->session.file_action_status_message), "%s", "READY");
    (void)snprintf(ctx->session.runtime_root_path, sizeof(ctx->session.runtime_root_path), "data/runtime");
    (void)snprintf(ctx->session.input_root_path, sizeof(ctx->session.input_root_path), "data/input");
    (void)snprintf(ctx->session.output_root_path, sizeof(ctx->session.output_root_path), "data/output");
    ctx->session.scene_authored_root_path[0] = '\0';

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--headless") == 0) {
            ctx->session.headless = 1u;
            continue;
        }
        if (strcmp(argv[i], "--print-lifecycle") == 0) {
            ctx->session.print_lifecycle = 1u;
            continue;
        }
        if (strcmp(argv[i], "--smoke-frames") == 0 && i + 1 < argc) {
            unsigned long parsed = strtoul(argv[++i], 0, 10);
            ctx->session.smoke_frames = (parsed > 0ul) ? (uint32_t)parsed : 1u;
            continue;
        }
        if (strcmp(argv[i], "--canvas-size") == 0 && i + 1 < argc) {
            uint32_t parsed_w = 0u;
            uint32_t parsed_h = 0u;
            if (!drawing_program_parse_canvas_size(argv[++i], &parsed_w, &parsed_h)) {
                return drawing_program_invalid("invalid --canvas-size (expected WxH)");
            }
            ctx->session.seed_canvas_logical_width = parsed_w;
            ctx->session.seed_canvas_logical_height = parsed_h;
            ctx->session.canvas_size_cli_override = 1u;
            continue;
        }
        if (strcmp(argv[i], "--canvas-width") == 0 && i + 1 < argc) {
            uint32_t parsed_w = 0u;
            if (!drawing_program_parse_u32_strict(argv[++i], &parsed_w)) {
                return drawing_program_invalid("invalid --canvas-width");
            }
            ctx->session.seed_canvas_logical_width = parsed_w;
            ctx->session.canvas_size_cli_override = 1u;
            continue;
        }
        if (strcmp(argv[i], "--canvas-height") == 0 && i + 1 < argc) {
            uint32_t parsed_h = 0u;
            if (!drawing_program_parse_u32_strict(argv[++i], &parsed_h)) {
                return drawing_program_invalid("invalid --canvas-height");
            }
            ctx->session.seed_canvas_logical_height = parsed_h;
            ctx->session.canvas_size_cli_override = 1u;
            continue;
        }
        if (strcmp(argv[i], "--preset") == 0 && i + 1 < argc) {
            ctx->session.preset_path = argv[++i];
            ctx->session.preset_path_cli_override = 1u;
            continue;
        }
        if (strcmp(argv[i], "--no-persist") == 0) {
            ctx->session.persist_enabled = 0u;
            continue;
        }
        if (strcmp(argv[i], "--export-json") == 0 && i + 1 < argc) {
            ctx->session.export_json_path = argv[++i];
            ctx->session.export_json_requested = 1u;
            continue;
        }
        if (strcmp(argv[i], "--bridge-workspace-preset") == 0 && i + 1 < argc) {
            ctx->session.bridge_workspace_preset_path = argv[++i];
            ctx->session.bridge_workspace_check_requested = 1u;
            continue;
        }
        if (strcmp(argv[i], "--bridge-workspace-import") == 0) {
            ctx->session.bridge_workspace_import_requested = 1u;
            continue;
        }
        if (strcmp(argv[i], "--texture-scene-import") == 0 && i + 1 < argc) {
            (void)snprintf(ctx->session.texture_scene_import_path,
                           sizeof(ctx->session.texture_scene_import_path),
                           "%s",
                           argv[++i]);
            ctx->session.texture_scene_import_requested = 1u;
            continue;
        }
        if (strcmp(argv[i], "--texture-scene-object") == 0 && i + 1 < argc) {
            (void)snprintf(ctx->session.texture_scene_import_object_id,
                           sizeof(ctx->session.texture_scene_import_object_id),
                           "%s",
                           argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "--texture-scene-quality") == 0 && i + 1 < argc) {
            uint32_t quality_preset = 0u;
            if (!drawing_program_parse_texture_quality_preset(argv[++i], &quality_preset)) {
                return drawing_program_invalid("invalid --texture-scene-quality");
            }
            ctx->session.texture_scene_import_quality_preset = quality_preset;
            continue;
        }
        if (strcmp(argv[i], "--texture-export") == 0) {
            ctx->session.texture_export_requested = 1u;
            continue;
        }
        if (strcmp(argv[i], "--texture-export-dir") == 0 && i + 1 < argc) {
            (void)snprintf(ctx->session.texture_export_dir_path,
                           sizeof(ctx->session.texture_export_dir_path),
                           "%s",
                           argv[++i]);
            ctx->session.texture_export_requested = 1u;
            continue;
        }
        if (strcmp(argv[i], "--runtime-root") == 0 && i + 1 < argc) {
            (void)snprintf(ctx->session.runtime_root_path, sizeof(ctx->session.runtime_root_path), "%s", argv[++i]);
            ctx->session.runtime_root_cli_override = 1u;
            continue;
        }
        if (strcmp(argv[i], "--input-root") == 0 && i + 1 < argc) {
            (void)snprintf(ctx->session.input_root_path, sizeof(ctx->session.input_root_path), "%s", argv[++i]);
            ctx->session.input_root_cli_override = 1u;
            continue;
        }
        if (strcmp(argv[i], "--output-root") == 0 && i + 1 < argc) {
            (void)snprintf(ctx->session.output_root_path, sizeof(ctx->session.output_root_path), "%s", argv[++i]);
            ctx->session.output_root_cli_override = 1u;
            continue;
        }
    }

    if (ctx->session.texture_scene_import_requested &&
        ctx->session.texture_scene_import_object_id[0] == '\0') {
        return drawing_program_invalid("texture scene import requires --texture-scene-object");
    }

    return core_result_ok();
}

CoreResult drawing_program_app_config_load(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return drawing_program_invalid("null app context");
    }
    return drawing_program_session_paths_configure(ctx);
}
