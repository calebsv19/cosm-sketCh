#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "core_font.h"
#include "core_theme.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_render_backend.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"

static int has_flag(int argc, char **argv, const char *flag) {
    int i;
    if (!argv || !flag) {
        return 0;
    }
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], flag) == 0) {
            return 1;
        }
    }
    return 0;
}

static int visual_trace_ui_state_enabled(void) {
    const char *value = getenv("DRAWING_PROGRAM_TRACE_UI_STATE");
    if (!value || value[0] == '\0' || value[0] == '0') {
        return 0;
    }
    return 1;
}

static uint32_t module_type_for_pane(const DrawingProgramAppContext *ctx, uint32_t pane_node_id) {
    uint32_t i;
    if (!ctx) {
        return 0u;
    }
    for (i = 0u; i < ctx->pane_host.module_binding_count; ++i) {
        if (ctx->pane_host.module_bindings[i].pane_node_id == pane_node_id) {
            return ctx->pane_host.module_bindings[i].module_type_id;
        }
    }
    return 0u;
}

static int set_module_type_for_pane(DrawingProgramAppContext *ctx, uint32_t pane_node_id, uint32_t module_type_id) {
    uint32_t i;
    if (!ctx) {
        return 0;
    }
    for (i = 0u; i < ctx->pane_host.module_binding_count; ++i) {
        if (ctx->pane_host.module_bindings[i].pane_node_id == pane_node_id) {
            ctx->pane_host.module_bindings[i].module_type_id = module_type_id;
            return 1;
        }
    }
    return 0;
}

static int pane_rect_for_module_type(const DrawingProgramAppContext *ctx,
                                     uint32_t module_type_id,
                                     SDL_Rect *out_rect) {
    uint32_t i;
    if (!ctx || !out_rect) {
        return 0;
    }
    for (i = 0u; i < ctx->pane_host.leaf_count; ++i) {
        const CorePaneLeafRect *leaf = &ctx->pane_host.leaves[i];
        if (module_type_for_pane(ctx, (uint32_t)leaf->id) == module_type_id) {
            out_rect->x = (int)leaf->rect.x;
            out_rect->y = (int)leaf->rect.y;
            out_rect->w = (int)leaf->rect.width;
            out_rect->h = (int)leaf->rect.height;
            if (out_rect->w < 1) {
                out_rect->w = 1;
            }
            if (out_rect->h < 1) {
                out_rect->h = 1;
            }
            return 1;
        }
    }
    return 0;
}

static int point_in_rect(SDL_Rect r, int x, int y) {
    return (x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h) ? 1 : 0;
}

static void map_input_to_render_coords(SDL_Window *window,
                                       SDL_Renderer *renderer,
                                       int input_x,
                                       int input_y,
                                       int *out_x,
                                       int *out_y) {
    int window_w = 0;
    int window_h = 0;
    int output_w = 0;
    int output_h = 0;
    if (!window || !renderer || !out_x || !out_y) {
        return;
    }
    SDL_GetWindowSize(window, &window_w, &window_h);
    if (SDL_GetRendererOutputSize(renderer, &output_w, &output_h) != 0 ||
        window_w <= 0 || window_h <= 0 || output_w <= 0 || output_h <= 0) {
        *out_x = input_x;
        *out_y = input_y;
        return;
    }
    *out_x = (input_x * output_w) / window_w;
    *out_y = (input_y * output_h) / window_h;
}

static uint8_t sample_value_for_tool(const DrawingProgramAppContext *ctx, DrawingProgramToolKind tool) {
    uint8_t color_index = drawing_program_color_default_index();
    if (ctx) {
        color_index = drawing_program_color_index_clamp(ctx->ui_active_color_index);
    }
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_ERASER:
            return drawing_program_color_eraser_value();
        case DRAWING_PROGRAM_TOOL_BRUSH:
        case DRAWING_PROGRAM_TOOL_FILL:
        case DRAWING_PROGRAM_TOOL_LINE:
        case DRAWING_PROGRAM_TOOL_RECT:
        case DRAWING_PROGRAM_TOOL_CIRCLE:
        case DRAWING_PROGRAM_TOOL_SELECT:
        case DRAWING_PROGRAM_TOOL_MOVE:
        case DRAWING_PROGRAM_TOOL_PICKER:
        default:
            return drawing_program_color_value_from_index(color_index);
    }
}

static uint32_t tool_brush_radius_samples(DrawingProgramToolKind tool) {
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_BRUSH:
            return 2u;
        case DRAWING_PROGRAM_TOOL_ERASER:
            return 4u;
        default:
            return 0u;
    }
}

static uint32_t tool_brush_spacing_samples(DrawingProgramToolKind tool, uint32_t radius) {
    uint32_t spacing = 1u;
    (void)tool;
    if (radius > 0u) {
        spacing = (radius / 2u) + 1u;
    }
    if (spacing < 1u) {
        spacing = 1u;
    }
    return spacing;
}

static int tool_uses_direct_sample_stroke(DrawingProgramToolKind tool) {
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_BRUSH:
        case DRAWING_PROGRAM_TOOL_ERASER:
            return 1;
        default:
            return 0;
    }
}

static int tool_uses_shape_commit(DrawingProgramToolKind tool) {
    return (tool == DRAWING_PROGRAM_TOOL_LINE ||
            tool == DRAWING_PROGRAM_TOOL_RECT ||
            tool == DRAWING_PROGRAM_TOOL_CIRCLE)
               ? 1
               : 0;
}

static uint8_t color_index_for_sample(uint8_t sample) {
    uint8_t best_index = drawing_program_color_default_index();
    int best_dist = 256;
    uint8_t i;
    for (i = 0u; i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++i) {
        int palette_value = (int)drawing_program_color_value_from_index(i);
        int d = palette_value - (int)sample;
        if (d < 0) {
            d = -d;
        }
        if (d < best_dist) {
            best_dist = d;
            best_index = i;
        }
    }
    return best_index;
}

static uint8_t seeded_background_sample_for_coord(const DrawingProgramDocument *document, uint32_t x, uint32_t y) {
    (void)document;
    (void)x;
    (void)y;
    return drawing_program_color_eraser_value();
}

static const DrawingProgramToolKind k_visual_tools[] = {
    DRAWING_PROGRAM_TOOL_BRUSH,
    DRAWING_PROGRAM_TOOL_ERASER,
    DRAWING_PROGRAM_TOOL_FILL,
    DRAWING_PROGRAM_TOOL_LINE,
    DRAWING_PROGRAM_TOOL_RECT,
    DRAWING_PROGRAM_TOOL_CIRCLE,
    DRAWING_PROGRAM_TOOL_SELECT,
    DRAWING_PROGRAM_TOOL_MOVE,
    DRAWING_PROGRAM_TOOL_PICKER
};

static DrawingProgramWorkflowControl workflow_control_for_tool(DrawingProgramToolKind tool) {
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_BRUSH: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_BRUSH;
        case DRAWING_PROGRAM_TOOL_ERASER: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_ERASER;
        case DRAWING_PROGRAM_TOOL_FILL: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_FILL;
        case DRAWING_PROGRAM_TOOL_LINE: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_LINE;
        case DRAWING_PROGRAM_TOOL_RECT: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_RECT;
        case DRAWING_PROGRAM_TOOL_CIRCLE: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_CIRCLE;
        case DRAWING_PROGRAM_TOOL_SELECT: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_SELECT;
        case DRAWING_PROGRAM_TOOL_MOVE: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_MOVE;
        case DRAWING_PROGRAM_TOOL_PICKER: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_PICKER;
        default: return DRAWING_PROGRAM_WORKFLOW_CONTROL_NONE;
    }
}

static SDL_Color sdl_color_from_theme(CoreThemeColor c) {
    SDL_Color out = { c.r, c.g, c.b, c.a };
    return out;
}

static SDL_Color sdl_color_adjust(SDL_Color c, int dr, int dg, int db) {
    int r = (int)c.r + dr;
    int g = (int)c.g + dg;
    int b = (int)c.b + db;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    c.r = (uint8_t)r;
    c.g = (uint8_t)g;
    c.b = (uint8_t)b;
    return c;
}

static int sdl_color_luma(SDL_Color c) {
    return ((int)c.r * 2126) + ((int)c.g * 7152) + ((int)c.b * 722);
}

static int sdl_color_contrast_distance(SDL_Color a, SDL_Color b) {
    int la = sdl_color_luma(a);
    int lb = sdl_color_luma(b);
    return (la > lb) ? (la - lb) : (lb - la);
}

static SDL_Color sdl_color_ensure_contrast(SDL_Color preferred, SDL_Color background) {
    SDL_Color white = { 244u, 244u, 248u, 255u };
    SDL_Color black = { 18u, 20u, 24u, 255u };
    if (sdl_color_contrast_distance(preferred, background) >= 2400) {
        return preferred;
    }
    if (sdl_color_contrast_distance(white, background) >= sdl_color_contrast_distance(black, background)) {
        return white;
    }
    return black;
}

static SDL_Color sdl_color_shift_by_luma(SDL_Color c, int amount) {
    if (sdl_color_luma(c) < 1280000) {
        return sdl_color_adjust(c, amount, amount, amount);
    }
    return sdl_color_adjust(c, -amount, -amount, -amount);
}

static CoreResult resolve_theme_color(const CoreThemePreset *theme,
                                      CoreThemeColorToken token,
                                      SDL_Color *out_color) {
    CoreThemeColor color;
    CoreResult result;
    if (!theme || !out_color) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid theme resolve request" };
    }
    result = core_theme_get_color(theme, token, &color);
    if (result.code != CORE_OK) {
        return result;
    }
    *out_color = sdl_color_from_theme(color);
    return core_result_ok();
}

typedef struct VisualThemePalette {
    SDL_Color app_background;
    SDL_Color pane_background;
    SDL_Color pane_background_alt;
    SDL_Color pane_border;
    SDL_Color button_fill;
    SDL_Color button_fill_hover;
    SDL_Color button_fill_active;
    SDL_Color button_border;
    SDL_Color text_primary;
    SDL_Color text_muted;
    SDL_Color accent_primary;
    SDL_Color status_ok;
    SDL_Color status_warn;
    SDL_Color status_error;
} VisualThemePalette;

static void resolve_visual_theme_palette(const CoreThemePreset *theme, VisualThemePalette *out) {
    SDL_Color surface0 = { 18u, 20u, 28u, 255u };
    SDL_Color surface1 = { 24u, 26u, 34u, 255u };
    SDL_Color surface2 = { 34u, 38u, 50u, 255u };
    SDL_Color text_primary = { 230u, 232u, 238u, 255u };
    SDL_Color text_muted = { 164u, 170u, 182u, 255u };
    SDL_Color accent = { 120u, 160u, 220u, 255u };
    SDL_Color ok = { 130u, 170u, 100u, 255u };
    SDL_Color warn = { 255u, 190u, 140u, 255u };
    SDL_Color err = { 180u, 60u, 60u, 255u };
    if (!out) {
        return;
    }
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_SURFACE_0, &surface0);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_SURFACE_1, &surface1);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_SURFACE_2, &surface2);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_TEXT_PRIMARY, &text_primary);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_TEXT_MUTED, &text_muted);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_ACCENT_PRIMARY, &accent);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_STATUS_OK, &ok);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_STATUS_WARN, &warn);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_STATUS_ERROR, &err);

    out->app_background = surface0;
    out->pane_background = surface1;
    out->pane_background_alt = surface2;
    out->pane_border = sdl_color_shift_by_luma(surface2, 10);
    out->button_fill = surface2;
    out->button_fill_hover = sdl_color_shift_by_luma(surface2, 8);
    out->button_fill_active = sdl_color_shift_by_luma(surface2, 16);
    out->button_border = sdl_color_shift_by_luma(surface2, 22);
    out->text_primary = text_primary;
    out->text_muted = text_muted;
    out->accent_primary = accent;
    out->status_ok = ok;
    out->status_warn = warn;
    out->status_error = err;
}

typedef struct VisualTextRendererState {
    uint8_t ttf_ready;
    uint8_t ttf_failed;
    uint8_t ttf_owner;
    CoreFontPresetId preset_id;
    char font_path[PATH_MAX];
    int base_point;
    TTF_Font *fonts[8];
    int points[8];
    uint8_t warned;
} VisualTextRendererState;

static VisualTextRendererState g_visual_text_renderer = {0};
typedef struct VisualCanvasTextureState {
    SDL_Texture *texture;
    SDL_PixelFormat *pixel_format;
    uint32_t width;
    uint32_t height;
} VisualCanvasTextureState;
static VisualCanvasTextureState g_visual_canvas_texture = {0};
static const DrawingProgramAppContext *g_visual_draw_ctx = 0;

static int visual_file_exists(const char *path) {
    FILE *f;
    if (!path || path[0] == '\0') {
        return 0;
    }
    f = fopen(path, "rb");
    if (!f) {
        return 0;
    }
    fclose(f);
    return 1;
}

static int visual_resolve_font_path_candidate(const char *candidate, char *out_path, size_t out_cap) {
    char path[PATH_MAX];
    char *base = 0;
    if (!candidate || candidate[0] == '\0') {
        return 0;
    }
    if ((candidate[0] == '/' || candidate[0] == '\\') && visual_file_exists(candidate)) {
        if (out_path && out_cap > 0u) {
            snprintf(out_path, out_cap, "%s", candidate);
        }
        return 1;
    }
    if (visual_file_exists(candidate)) {
        if (out_path && out_cap > 0u) {
            snprintf(out_path, out_cap, "%s", candidate);
        }
        return 1;
    }
    snprintf(path, sizeof(path), "./%s", candidate);
    if (visual_file_exists(path)) {
        if (out_path && out_cap > 0u) {
            snprintf(out_path, out_cap, "%s", path);
        }
        return 1;
    }
    snprintf(path, sizeof(path), "../%s", candidate);
    if (visual_file_exists(path)) {
        if (out_path && out_cap > 0u) {
            snprintf(out_path, out_cap, "%s", path);
        }
        return 1;
    }
    snprintf(path, sizeof(path), "../../%s", candidate);
    if (visual_file_exists(path)) {
        if (out_path && out_cap > 0u) {
            snprintf(out_path, out_cap, "%s", path);
        }
        return 1;
    }
    base = SDL_GetBasePath();
    if (base) {
        snprintf(path, sizeof(path), "%s%s", base, candidate);
        if (visual_file_exists(path)) {
            if (out_path && out_cap > 0u) {
                snprintf(out_path, out_cap, "%s", path);
            }
            SDL_free(base);
            return 1;
        }
        snprintf(path, sizeof(path), "%s../Resources/%s", base, candidate);
        if (visual_file_exists(path)) {
            if (out_path && out_cap > 0u) {
                snprintf(out_path, out_cap, "%s", path);
            }
            SDL_free(base);
            return 1;
        }
        snprintf(path, sizeof(path), "%sResources/%s", base, candidate);
        if (visual_file_exists(path)) {
            if (out_path && out_cap > 0u) {
                snprintf(out_path, out_cap, "%s", path);
            }
            SDL_free(base);
            return 1;
        }
        SDL_free(base);
    }
    return 0;
}

static const char *visual_basename(const char *path) {
    const char *slash = 0;
    const char *backslash = 0;
    if (!path) {
        return 0;
    }
    slash = strrchr(path, '/');
    backslash = strrchr(path, '\\');
    if (slash && backslash) {
        return (slash > backslash) ? (slash + 1) : (backslash + 1);
    }
    if (slash) {
        return slash + 1;
    }
    if (backslash) {
        return backslash + 1;
    }
    return path;
}

static int visual_resolve_font_role_path(const char *raw, char *out_path, size_t out_cap) {
    const char *base = 0;
    const char *runtime_dir = 0;
    char adjusted[PATH_MAX];
    char shared_path[PATH_MAX];
    char third_party_path[PATH_MAX];
    char app_resource_path[PATH_MAX];
    char parent_resource_path[PATH_MAX];
    char runtime_shared_path[PATH_MAX];
    if (!raw || raw[0] == '\0') {
        return 0;
    }
    if (visual_resolve_font_path_candidate(raw, out_path, out_cap)) {
        return 1;
    }

    base = visual_basename(raw);
    if (base && base[0]) {
        if (strncmp(raw, "include/fonts/", 14) == 0) {
            snprintf(adjusted, sizeof(adjusted), "include/fonts/%s", base);
            if (visual_resolve_font_path_candidate(adjusted, out_path, out_cap)) {
                return 1;
            }
        }
        snprintf(shared_path, sizeof(shared_path), "shared/assets/fonts/%s", base);
        if (visual_resolve_font_path_candidate(shared_path, out_path, out_cap)) {
            return 1;
        }
        snprintf(third_party_path, sizeof(third_party_path), "third_party/codework_shared/assets/fonts/%s", base);
        if (visual_resolve_font_path_candidate(third_party_path, out_path, out_cap)) {
            return 1;
        }
        snprintf(app_resource_path, sizeof(app_resource_path), "shared/assets/fonts/%s", base);
        if (visual_resolve_font_path_candidate(app_resource_path, out_path, out_cap)) {
            return 1;
        }
        snprintf(parent_resource_path, sizeof(parent_resource_path), "../shared/assets/fonts/%s", base);
        if (visual_resolve_font_path_candidate(parent_resource_path, out_path, out_cap)) {
            return 1;
        }
        runtime_dir = getenv("DRAWING_PROGRAM_RUNTIME_DIR");
        if (runtime_dir && runtime_dir[0]) {
            snprintf(runtime_shared_path, sizeof(runtime_shared_path), "%s/shared/assets/fonts/%s", runtime_dir, base);
            if (visual_resolve_font_path_candidate(runtime_shared_path, out_path, out_cap)) {
                return 1;
            }
        }
    }
    return 0;
}

static int visual_select_font_path_for_preset(CoreFontPresetId preset_id,
                                              char *out_path,
                                              size_t out_cap,
                                              int *out_base_point) {
    CoreFontPreset preset;
    CoreFontRoleSpec role;
    if (!out_path || out_cap == 0u) {
        return 0;
    }
    out_path[0] = '\0';
    if (core_font_get_preset(preset_id, &preset).code != CORE_OK) {
        return 0;
    }
    if (core_font_resolve_role(&preset, CORE_FONT_ROLE_UI_REGULAR, &role).code != CORE_OK) {
        return 0;
    }
    if (!visual_resolve_font_role_path(role.primary_path, out_path, out_cap) &&
        !visual_resolve_font_role_path(role.fallback_path, out_path, out_cap)) {
        return 0;
    }
    if (out_base_point) {
        *out_base_point = role.point_size;
    }
    return 1;
}

static void visual_text_renderer_shutdown(void) {
    size_t i;
    for (i = 0u; i < (sizeof(g_visual_text_renderer.fonts) / sizeof(g_visual_text_renderer.fonts[0])); ++i) {
        if (g_visual_text_renderer.fonts[i]) {
            TTF_CloseFont(g_visual_text_renderer.fonts[i]);
            g_visual_text_renderer.fonts[i] = 0;
            g_visual_text_renderer.points[i] = 0;
        }
    }
    if (g_visual_text_renderer.ttf_owner && TTF_WasInit()) {
        TTF_Quit();
    }
    memset(&g_visual_text_renderer, 0, sizeof(g_visual_text_renderer));
}

static void visual_canvas_texture_shutdown(void) {
    if (g_visual_canvas_texture.texture) {
        SDL_DestroyTexture(g_visual_canvas_texture.texture);
        g_visual_canvas_texture.texture = 0;
    }
    if (g_visual_canvas_texture.pixel_format) {
        SDL_FreeFormat(g_visual_canvas_texture.pixel_format);
        g_visual_canvas_texture.pixel_format = 0;
    }
    g_visual_canvas_texture.width = 0u;
    g_visual_canvas_texture.height = 0u;
}

static int visual_canvas_texture_sync(SDL_Renderer *renderer, const DrawingProgramAppContext *ctx) {
    void *pixels = 0;
    int pitch = 0;
    uint32_t x;
    uint32_t y;
    if (!renderer || !ctx) {
        return 0;
    }
    if (ctx->document.raster_width == 0u ||
        ctx->document.raster_height == 0u ||
        ctx->document.raster_sample_count == 0u) {
        return 0;
    }
    if (!g_visual_canvas_texture.pixel_format) {
        g_visual_canvas_texture.pixel_format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
        if (!g_visual_canvas_texture.pixel_format) {
            return 0;
        }
    }
    if (!g_visual_canvas_texture.texture ||
        g_visual_canvas_texture.width != ctx->document.raster_width ||
        g_visual_canvas_texture.height != ctx->document.raster_height) {
        visual_canvas_texture_shutdown();
        g_visual_canvas_texture.texture = SDL_CreateTexture(renderer,
                                                            SDL_PIXELFORMAT_RGBA8888,
                                                            SDL_TEXTUREACCESS_STREAMING,
                                                            (int)ctx->document.raster_width,
                                                            (int)ctx->document.raster_height);
        if (!g_visual_canvas_texture.texture) {
            return 0;
        }
        g_visual_canvas_texture.pixel_format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
        if (!g_visual_canvas_texture.pixel_format) {
            visual_canvas_texture_shutdown();
            return 0;
        }
        g_visual_canvas_texture.width = ctx->document.raster_width;
        g_visual_canvas_texture.height = ctx->document.raster_height;
        (void)SDL_SetTextureBlendMode(g_visual_canvas_texture.texture, SDL_BLENDMODE_NONE);
    }
    if (SDL_LockTexture(g_visual_canvas_texture.texture, 0, &pixels, &pitch) != 0) {
        return 0;
    }
    for (y = 0u; y < ctx->document.raster_height; ++y) {
        uint32_t *row = (uint32_t *)((uint8_t *)pixels + ((size_t)y * (size_t)pitch));
        size_t row_offset = (size_t)y * (size_t)ctx->document.raster_width;
        for (x = 0u; x < ctx->document.raster_width; ++x) {
            uint8_t sample = ctx->document.raster_samples[row_offset + x];
            uint8_t r = 0u;
            uint8_t g = 0u;
            uint8_t b = 0u;
            drawing_program_color_rgb_from_sample(sample, &r, &g, &b);
            row[x] = SDL_MapRGBA(g_visual_canvas_texture.pixel_format, r, g, b, 255u);
        }
    }
    SDL_UnlockTexture(g_visual_canvas_texture.texture);
    return 1;
}

static TTF_Font *visual_get_ttf_font(const DrawingProgramAppContext *ctx, int scale) {
    size_t i;
    int point_size;
    CoreFontPresetId preset_id;
    if (!ctx || scale < 1 || scale > 8 || g_visual_text_renderer.ttf_failed) {
        return 0;
    }
    preset_id = (ctx->ui_font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT)
                    ? (CoreFontPresetId)ctx->ui_font_preset_id
                    : CORE_FONT_PRESET_IDE;
    if (!g_visual_text_renderer.ttf_ready || g_visual_text_renderer.preset_id != preset_id) {
        visual_text_renderer_shutdown();
        if (TTF_WasInit() == 0) {
            if (TTF_Init() != 0) {
                g_visual_text_renderer.ttf_failed = 1u;
                if (!g_visual_text_renderer.warned) {
                    fprintf(stderr, "drawing_program: TTF_Init failed: %s\n", TTF_GetError());
                    g_visual_text_renderer.warned = 1u;
                }
                return 0;
            }
            g_visual_text_renderer.ttf_owner = 1u;
        }
        g_visual_text_renderer.ttf_ready = 1u;
        g_visual_text_renderer.preset_id = preset_id;
        if (!visual_select_font_path_for_preset(preset_id,
                                                g_visual_text_renderer.font_path,
                                                sizeof(g_visual_text_renderer.font_path),
                                                &g_visual_text_renderer.base_point)) {
            g_visual_text_renderer.ttf_failed = 1u;
            if (!g_visual_text_renderer.warned) {
                fprintf(stderr, "drawing_program: no resolvable TTF font path for preset id=%u\n", (unsigned)preset_id);
                g_visual_text_renderer.warned = 1u;
            }
            return 0;
        }
    }
    /* Keep point sizing close to legacy bitmap scale progression. */
    point_size = g_visual_text_renderer.base_point + (scale - 1) * 4;
    if (point_size < 8) {
        point_size = 8;
    }
    if (point_size > 48) {
        point_size = 48;
    }
    for (i = 0u; i < (sizeof(g_visual_text_renderer.fonts) / sizeof(g_visual_text_renderer.fonts[0])); ++i) {
        if (g_visual_text_renderer.fonts[i] && g_visual_text_renderer.points[i] == point_size) {
            return g_visual_text_renderer.fonts[i];
        }
    }
    for (i = 0u; i < (sizeof(g_visual_text_renderer.fonts) / sizeof(g_visual_text_renderer.fonts[0])); ++i) {
        if (!g_visual_text_renderer.fonts[i]) {
            g_visual_text_renderer.fonts[i] = TTF_OpenFont(g_visual_text_renderer.font_path, point_size);
            if (!g_visual_text_renderer.fonts[i]) {
                g_visual_text_renderer.ttf_failed = 1u;
                if (!g_visual_text_renderer.warned) {
                    fprintf(stderr,
                            "drawing_program: TTF_OpenFont failed path=%s size=%d err=%s\n",
                            g_visual_text_renderer.font_path,
                            point_size,
                            TTF_GetError());
                    g_visual_text_renderer.warned = 1u;
                }
                return 0;
            }
            TTF_SetFontHinting(g_visual_text_renderer.fonts[i], TTF_HINTING_LIGHT);
            TTF_SetFontKerning(g_visual_text_renderer.fonts[i], 1);
            g_visual_text_renderer.points[i] = point_size;
            return g_visual_text_renderer.fonts[i];
        }
    }
    return g_visual_text_renderer.fonts[0];
}

static void module_color(uint32_t module_type_id,
                         const CoreThemePreset *theme,
                         SDL_Color *out_fill,
                         SDL_Color *out_border) {
    VisualThemePalette p;
    if (!theme || !out_fill || !out_border) {
        return;
    }
    resolve_visual_theme_palette(theme, &p);
    if (module_type_id == 3u) { /* menu */
        *out_fill = p.pane_background;
        *out_border = p.pane_border;
        return;
    }
    if (module_type_id == 2u || module_type_id == 4u) { /* side panels */
        *out_fill = p.pane_background;
        *out_border = p.pane_border;
        return;
    }
    if (module_type_id == 1u) { /* canvas pane */
        *out_fill = p.pane_background_alt;
        *out_border = p.pane_border;
        return;
    }
    *out_fill = p.app_background;
    *out_border = p.pane_border;
}

typedef struct VisualCanvasSheetMetrics {
    SDL_Rect sheet_rect;
    float pixel_size;
} VisualCanvasSheetMetrics;

typedef struct VisualCanvasInteractionState {
    uint8_t drawing_active;
    uint8_t panning_active;
    uint8_t shape_active;
    uint8_t shape_tool;
    uint8_t has_last_sample;
    uint32_t last_sample_x;
    uint32_t last_sample_y;
    uint32_t shape_start_sample_x;
    uint32_t shape_start_sample_y;
    int last_mouse_x;
    int last_mouse_y;
} VisualCanvasInteractionState;

#define VISUAL_SELECTION_MAX_WIDTH 128u
#define VISUAL_SELECTION_MAX_HEIGHT 128u
#define VISUAL_SELECTION_MAX_AREA (VISUAL_SELECTION_MAX_WIDTH * VISUAL_SELECTION_MAX_HEIGHT)
#define VISUAL_SELECTION_MAX_AFFECTED (VISUAL_SELECTION_MAX_AREA * 2u)

typedef struct VisualSelectionState {
    uint8_t has_payload;
    uint8_t selecting;
    uint8_t moving;
    uint32_t origin_x;
    uint32_t origin_y;
    uint32_t width;
    uint32_t height;
    int32_t offset_x;
    int32_t offset_y;
    uint32_t marquee_start_x;
    uint32_t marquee_start_y;
    uint32_t marquee_end_x;
    uint32_t marquee_end_y;
    uint32_t move_anchor_sample_x;
    uint32_t move_anchor_sample_y;
    int32_t move_anchor_offset_x;
    int32_t move_anchor_offset_y;
    uint32_t payload_count;
    uint8_t payload_mask[VISUAL_SELECTION_MAX_AREA];
    uint8_t payload_value[VISUAL_SELECTION_MAX_AREA];
} VisualSelectionState;

typedef struct VisualAffectedSample {
    uint8_t used;
    uint32_t x;
    uint32_t y;
    uint8_t previous;
    uint8_t next;
} VisualAffectedSample;

typedef enum VisualLeftPanelSlot {
    VISUAL_LEFT_PANEL_SLOT_TOOLS = 0,
    VISUAL_LEFT_PANEL_SLOT_VIEW = 1
} VisualLeftPanelSlot;

typedef enum VisualRightPanelSlot {
    VISUAL_RIGHT_PANEL_SLOT_CANVAS = 0,
    VISUAL_RIGHT_PANEL_SLOT_LAYER = 1
} VisualRightPanelSlot;

typedef struct VisualPanelUiState {
    uint8_t left_slot;
    uint8_t right_slot;
    uint8_t mouse_known;
    int mouse_x;
    int mouse_y;
} VisualPanelUiState;

static void visual_selection_reset(VisualSelectionState *selection) {
    if (!selection) {
        return;
    }
    memset(selection, 0, sizeof(*selection));
}

static int32_t clamp_i32(int32_t value, int32_t min_value, int32_t max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static uint8_t visual_selection_mask_at(const VisualSelectionState *selection, uint32_t local_x, uint32_t local_y) {
    uint32_t index;
    if (!selection || selection->width == 0u || selection->height == 0u) {
        return 0u;
    }
    if (local_x >= selection->width || local_y >= selection->height) {
        return 0u;
    }
    index = local_y * selection->width + local_x;
    if (index >= VISUAL_SELECTION_MAX_AREA) {
        return 0u;
    }
    return selection->payload_mask[index];
}

static uint8_t visual_selection_value_at(const VisualSelectionState *selection, uint32_t local_x, uint32_t local_y) {
    uint32_t index;
    if (!selection || selection->width == 0u || selection->height == 0u) {
        return 0u;
    }
    if (local_x >= selection->width || local_y >= selection->height) {
        return 0u;
    }
    index = local_y * selection->width + local_x;
    if (index >= VISUAL_SELECTION_MAX_AREA) {
        return 0u;
    }
    return selection->payload_value[index];
}

static int visual_selection_capture_from_rect(DrawingProgramAppContext *ctx,
                                              VisualSelectionState *selection,
                                              int32_t x0,
                                              int32_t y0,
                                              uint32_t width,
                                              uint32_t height) {
    int32_t min_x;
    int32_t min_y;
    int32_t max_x;
    int32_t max_y;
    uint32_t clipped_w;
    uint32_t clipped_h;
    uint32_t x;
    uint32_t y;
    if (!ctx || !selection || width == 0u || height == 0u) {
        visual_selection_reset(selection);
        return 0;
    }
    min_x = x0;
    min_y = y0;
    max_x = x0 + (int32_t)width - 1;
    max_y = y0 + (int32_t)height - 1;
    if (max_x < 0 || max_y < 0 ||
        min_x >= (int32_t)ctx->document.raster_width ||
        min_y >= (int32_t)ctx->document.raster_height) {
        visual_selection_reset(selection);
        return 0;
    }
    min_x = clamp_i32(min_x, 0, (int32_t)ctx->document.raster_width - 1);
    min_y = clamp_i32(min_y, 0, (int32_t)ctx->document.raster_height - 1);
    max_x = clamp_i32(max_x, 0, (int32_t)ctx->document.raster_width - 1);
    max_y = clamp_i32(max_y, 0, (int32_t)ctx->document.raster_height - 1);
    if (max_x < min_x || max_y < min_y) {
        visual_selection_reset(selection);
        return 0;
    }
    clipped_w = (uint32_t)(max_x - min_x + 1);
    clipped_h = (uint32_t)(max_y - min_y + 1);
    if (clipped_w > VISUAL_SELECTION_MAX_WIDTH) {
        clipped_w = VISUAL_SELECTION_MAX_WIDTH;
    }
    if (clipped_h > VISUAL_SELECTION_MAX_HEIGHT) {
        clipped_h = VISUAL_SELECTION_MAX_HEIGHT;
    }
    memset(selection->payload_mask, 0, sizeof(selection->payload_mask));
    memset(selection->payload_value, 0, sizeof(selection->payload_value));
    selection->has_payload = 1u;
    selection->selecting = 0u;
    selection->moving = 0u;
    selection->origin_x = (uint32_t)min_x;
    selection->origin_y = (uint32_t)min_y;
    selection->width = clipped_w;
    selection->height = clipped_h;
    selection->offset_x = 0;
    selection->offset_y = 0;
    selection->payload_count = 0u;
    for (y = 0u; y < clipped_h; ++y) {
        for (x = 0u; x < clipped_w; ++x) {
            uint32_t index = y * clipped_w + x;
            uint8_t sample = 0u;
            uint32_t sx = (uint32_t)min_x + x;
            uint32_t sy = (uint32_t)min_y + y;
            uint8_t seed_sample;
            if (index >= VISUAL_SELECTION_MAX_AREA) {
                continue;
            }
            if (drawing_program_document_sample_read(&ctx->document,
                                                     sx,
                                                     sy,
                                                     &sample).code != CORE_OK) {
                continue;
            }
            seed_sample = seeded_background_sample_for_coord(&ctx->document, sx, sy);
            if (sample == seed_sample) {
                continue;
            }
            selection->payload_mask[index] = 1u;
            selection->payload_value[index] = sample;
            selection->payload_count += 1u;
        }
    }
    if (selection->payload_count == 0u) {
        visual_selection_reset(selection);
        return 0;
    }
    return 1;
}

static int visual_selection_capture_from_marquee(DrawingProgramAppContext *ctx,
                                                 VisualSelectionState *selection) {
    uint32_t min_x;
    uint32_t min_y;
    uint32_t max_x;
    uint32_t max_y;
    uint32_t width;
    uint32_t height;
    if (!ctx || !selection || !selection->selecting) {
        return 0;
    }
    min_x = (selection->marquee_start_x < selection->marquee_end_x)
                ? selection->marquee_start_x
                : selection->marquee_end_x;
    min_y = (selection->marquee_start_y < selection->marquee_end_y)
                ? selection->marquee_start_y
                : selection->marquee_end_y;
    max_x = (selection->marquee_start_x > selection->marquee_end_x)
                ? selection->marquee_start_x
                : selection->marquee_end_x;
    max_y = (selection->marquee_start_y > selection->marquee_end_y)
                ? selection->marquee_start_y
                : selection->marquee_end_y;
    width = max_x - min_x + 1u;
    height = max_y - min_y + 1u;
    return visual_selection_capture_from_rect(ctx, selection, (int32_t)min_x, (int32_t)min_y, width, height);
}

static int visual_selection_contains_sample(const VisualSelectionState *selection, uint32_t sample_x, uint32_t sample_y) {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
    if (!selection || !selection->has_payload || selection->width == 0u || selection->height == 0u) {
        return 0;
    }
    left = (int32_t)selection->origin_x + selection->offset_x;
    top = (int32_t)selection->origin_y + selection->offset_y;
    right = left + (int32_t)selection->width;
    bottom = top + (int32_t)selection->height;
    return ((int32_t)sample_x >= left &&
            (int32_t)sample_y >= top &&
            (int32_t)sample_x < right &&
            (int32_t)sample_y < bottom)
               ? 1
               : 0;
}

static int visual_selection_begin_move(const VisualSelectionState *selection, uint32_t sample_x, uint32_t sample_y) {
    return visual_selection_contains_sample(selection, sample_x, sample_y);
}

static int visual_selection_track_affected(VisualAffectedSample *affected,
                                           uint32_t *io_count,
                                           const DrawingProgramDocument *document,
                                           uint32_t x,
                                           uint32_t y) {
    uint32_t i;
    uint8_t previous = 0u;
    if (!affected || !io_count || !document) {
        return -1;
    }
    for (i = 0u; i < *io_count; ++i) {
        if (affected[i].used && affected[i].x == x && affected[i].y == y) {
            return (int)i;
        }
    }
    if (*io_count >= VISUAL_SELECTION_MAX_AFFECTED) {
        return -1;
    }
    if (drawing_program_document_sample_read(document, x, y, &previous).code != CORE_OK) {
        return -1;
    }
    affected[*io_count].used = 1u;
    affected[*io_count].x = x;
    affected[*io_count].y = y;
    affected[*io_count].previous = previous;
    affected[*io_count].next = previous;
    *io_count += 1u;
    return (int)(*io_count - 1u);
}

static CoreResult visual_selection_commit_move(DrawingProgramAppContext *ctx, VisualSelectionState *selection) {
    VisualAffectedSample affected[VISUAL_SELECTION_MAX_AFFECTED];
    uint32_t affected_count = 0u;
    uint32_t x;
    uint32_t y;
    int32_t target_origin_x;
    int32_t target_origin_y;
    memset(affected, 0, sizeof(affected));
    if (!ctx || !selection || !selection->has_payload) {
        return core_result_ok();
    }
    if (selection->offset_x == 0 && selection->offset_y == 0) {
        selection->moving = 0u;
        return core_result_ok();
    }
    for (y = 0u; y < selection->height; ++y) {
        for (x = 0u; x < selection->width; ++x) {
            uint32_t sx = selection->origin_x + x;
            uint32_t sy = selection->origin_y + y;
            int idx;
            if (!visual_selection_mask_at(selection, x, y)) {
                continue;
            }
            if (sx >= ctx->document.raster_width || sy >= ctx->document.raster_height) {
                continue;
            }
            idx = visual_selection_track_affected(affected, &affected_count, &ctx->document, sx, sy);
            if (idx < 0) {
                return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "selection move exceeded seed affected sample budget" };
            }
            affected[idx].next = seeded_background_sample_for_coord(&ctx->document, sx, sy);
        }
    }
    for (y = 0u; y < selection->height; ++y) {
        for (x = 0u; x < selection->width; ++x) {
            int32_t dx;
            int32_t dy;
            int idx;
            uint8_t value;
            if (!visual_selection_mask_at(selection, x, y)) {
                continue;
            }
            dx = (int32_t)selection->origin_x + selection->offset_x + (int32_t)x;
            dy = (int32_t)selection->origin_y + selection->offset_y + (int32_t)y;
            if (dx < 0 || dy < 0 ||
                dx >= (int32_t)ctx->document.raster_width ||
                dy >= (int32_t)ctx->document.raster_height) {
                continue;
            }
            value = visual_selection_value_at(selection, x, y);
            idx = visual_selection_track_affected(affected,
                                                  &affected_count,
                                                  &ctx->document,
                                                  (uint32_t)dx,
                                                  (uint32_t)dy);
            if (idx < 0) {
                return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "selection move exceeded seed affected sample budget" };
            }
            affected[idx].next = value;
        }
    }
    for (x = 0u; x < affected_count; ++x) {
        if (affected[x].next == affected[x].previous) {
            continue;
        }
        {
            CoreResult result = drawing_program_history_apply_set_sample_value(&ctx->history,
                                                                                &ctx->document,
                                                                                affected[x].x,
                                                                                affected[x].y,
                                                                                affected[x].next);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    }
    target_origin_x = (int32_t)selection->origin_x + selection->offset_x;
    target_origin_y = (int32_t)selection->origin_y + selection->offset_y;
    selection->moving = 0u;
    selection->offset_x = 0;
    selection->offset_y = 0;
    if (!visual_selection_capture_from_rect(ctx,
                                            selection,
                                            target_origin_x,
                                            target_origin_y,
                                            selection->width,
                                            selection->height)) {
        visual_selection_reset(selection);
    }
    return core_result_ok();
}

static const CoreThemePresetId k_visual_theme_cycle_order[] = {
    CORE_THEME_PRESET_DAW_DEFAULT,
    CORE_THEME_PRESET_MAP_FORGE_DEFAULT,
    CORE_THEME_PRESET_DARK_DEFAULT,
    CORE_THEME_PRESET_LIGHT_DEFAULT,
    CORE_THEME_PRESET_IDE_GRAY,
    CORE_THEME_PRESET_GREYSCALE
};

static uint8_t clamp_left_slot(uint8_t slot) {
    return (slot <= (uint8_t)VISUAL_LEFT_PANEL_SLOT_VIEW) ? slot : (uint8_t)VISUAL_LEFT_PANEL_SLOT_TOOLS;
}

static uint8_t clamp_right_slot(uint8_t slot) {
    return (slot <= (uint8_t)VISUAL_RIGHT_PANEL_SLOT_LAYER) ? slot : (uint8_t)VISUAL_RIGHT_PANEL_SLOT_CANVAS;
}

static CoreThemePresetId clamp_theme_preset_id(uint32_t raw) {
    if (raw < (uint32_t)CORE_THEME_PRESET_COUNT) {
        return (CoreThemePresetId)raw;
    }
    return CORE_THEME_PRESET_DARK_DEFAULT;
}

static void sync_panel_ui_from_app(const DrawingProgramAppContext *ctx, VisualPanelUiState *ui) {
    if (!ctx || !ui) {
        return;
    }
    ui->left_slot = clamp_left_slot(ctx->ui_left_panel_slot);
    ui->right_slot = clamp_right_slot(ctx->ui_right_panel_slot);
}

static int clamp_font_zoom_step(int step) {
    if (step < -2) {
        return -2;
    }
    if (step > 4) {
        return 4;
    }
    return step;
}

static int ui_scale_for_text(const DrawingProgramAppContext *ctx, int base_scale) {
    int scale = base_scale;
    int step = 0;
    if (ctx) {
        step = clamp_font_zoom_step((int)ctx->ui_font_zoom_step);
    }
    scale += step;
    if (scale < 1) {
        scale = 1;
    }
    if (scale > 8) {
        scale = 8;
    }
    return scale;
}

typedef struct VisualPaneLayoutMetrics {
    int pad_x;
    int pad_y;
    int section_gap;
    int line_h;
    int row_h;
    int tab_h;
    int tab_gap;
    int title_scale;
    int body_scale;
    int title_glyph_h;
    int body_glyph_h;
    int row_text_y;
    int tab_text_y;
} VisualPaneLayoutMetrics;

static VisualPaneLayoutMetrics make_pane_layout_metrics(const DrawingProgramAppContext *ctx) {
    VisualPaneLayoutMetrics m;
    int body_scale = ui_scale_for_text(ctx, 1);
    int title_scale = ui_scale_for_text(ctx, 2);
    int body_glyph_h = 7 * body_scale;
    int title_glyph_h = 7 * title_scale;
    m.pad_x = 8 + (body_scale * 2);
    m.pad_y = 8 + body_scale;
    m.section_gap = 4 + body_scale;
    m.line_h = body_glyph_h + 3 + body_scale;
    m.row_h = body_glyph_h + 6 + body_scale;
    if (m.row_h < 18) {
        m.row_h = 18;
    }
    m.tab_h = body_glyph_h + 5 + body_scale;
    if (m.tab_h < 18) {
        m.tab_h = 18;
    }
    m.tab_gap = 4 + (body_scale / 2);
    m.title_scale = title_scale;
    m.body_scale = body_scale;
    m.title_glyph_h = title_glyph_h;
    m.body_glyph_h = body_glyph_h;
    m.row_text_y = (m.row_h - m.body_glyph_h) / 2;
    if (m.row_text_y < 2) {
        m.row_text_y = 2;
    }
    m.tab_text_y = (m.tab_h - m.body_glyph_h) / 2;
    if (m.tab_text_y < 2) {
        m.tab_text_y = 2;
    }
    return m;
}

static int right_canvas_palette_header_y(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    int content_y = rect.y + m.pad_y + m.title_glyph_h + m.section_gap;
    content_y += m.tab_h + m.section_gap;
    content_y += (m.line_h * 8) + m.section_gap;
    return content_y;
}

static SDL_Rect right_canvas_palette_swatch_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, uint8_t color_index) {
    const int cols = 4;
    int swatch_w = (rect.w - (2 * m.pad_x) - ((cols - 1) * m.tab_gap)) / cols;
    int row = (int)(color_index / (uint8_t)cols);
    int col = (int)(color_index % (uint8_t)cols);
    int swatch_y = right_canvas_palette_header_y(rect, m) + m.line_h;
    if (swatch_w < 24) {
        swatch_w = 24;
    }
    return (SDL_Rect){
        rect.x + m.pad_x + col * (swatch_w + m.tab_gap),
        swatch_y + row * (m.row_h + m.section_gap),
        swatch_w,
        m.row_h
    };
}

static SDL_Rect right_canvas_reset_view_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    int palette_rows = ((int)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT + 3) / 4;
    int y = right_canvas_palette_header_y(rect, m) + m.line_h;
    y += palette_rows * m.row_h;
    if (palette_rows > 1) {
        y += (palette_rows - 1) * m.section_gap;
    }
    y += m.section_gap;
    y += (m.line_h * 4) + m.section_gap;
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
}

static SDL_Rect right_canvas_clear_history_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect reset = right_canvas_reset_view_button_rect(rect, m);
    return (SDL_Rect){ reset.x, reset.y + reset.h + m.section_gap, reset.w, reset.h };
}

static int cycle_theme_preset(CoreThemePresetId current, int direction, CoreThemePresetId *out_next) {
    uint32_t i;
    uint32_t count = (uint32_t)(sizeof(k_visual_theme_cycle_order) / sizeof(k_visual_theme_cycle_order[0]));
    if (!out_next || count == 0u) {
        return 0;
    }
    for (i = 0u; i < count; ++i) {
        if (k_visual_theme_cycle_order[i] == current) {
            if (direction >= 0) {
                *out_next = k_visual_theme_cycle_order[(i + 1u) % count];
            } else {
                *out_next = k_visual_theme_cycle_order[(i + count - 1u) % count];
            }
            return 1;
        }
    }
    *out_next = k_visual_theme_cycle_order[0];
    return 1;
}

static void compute_canvas_sheet_metrics(const DrawingProgramAppContext *ctx,
                                         SDL_Rect pane_rect,
                                         VisualCanvasSheetMetrics *out_metrics) {
    float zoom;
    float base_pixel = 0.75f;
    float pixel_size;
    int sheet_w;
    int sheet_h;
    int cx;
    int cy;
    if (!ctx || !out_metrics) {
        return;
    }
    zoom = ctx->editor.viewport.zoom;
    if (zoom < 0.25f) {
        zoom = 0.25f;
    }
    if (zoom > 8.0f) {
        zoom = 8.0f;
    }
    pixel_size = base_pixel * zoom;
    if (pixel_size < 0.25f) {
        pixel_size = 0.25f;
    }
    if (pixel_size > 16.0f) {
        pixel_size = 16.0f;
    }
    sheet_w = (int)((float)ctx->document.raster_width * pixel_size);
    sheet_h = (int)((float)ctx->document.raster_height * pixel_size);
    cx = pane_rect.x + pane_rect.w / 2 + (int)ctx->editor.viewport.pan_x;
    cy = pane_rect.y + pane_rect.h / 2 + (int)ctx->editor.viewport.pan_y;
    out_metrics->pixel_size = pixel_size;
    out_metrics->sheet_rect.x = cx - sheet_w / 2;
    out_metrics->sheet_rect.y = cy - sheet_h / 2;
    out_metrics->sheet_rect.w = sheet_w;
    out_metrics->sheet_rect.h = sheet_h;
}

static int screen_to_canvas_sample(const DrawingProgramAppContext *ctx,
                                   SDL_Rect pane_rect,
                                   int sx,
                                   int sy,
                                   uint32_t *out_sample_x,
                                   uint32_t *out_sample_y) {
    VisualCanvasSheetMetrics metrics;
    int local_x;
    int local_y;
    if (!ctx || !out_sample_x || !out_sample_y) {
        return 0;
    }
    compute_canvas_sheet_metrics(ctx, pane_rect, &metrics);
    if (!point_in_rect(metrics.sheet_rect, sx, sy)) {
        return 0;
    }
    local_x = sx - metrics.sheet_rect.x;
    local_y = sy - metrics.sheet_rect.y;
    if (metrics.pixel_size <= 0.0f) {
        return 0;
    }
    *out_sample_x = (uint32_t)((float)local_x / metrics.pixel_size);
    *out_sample_y = (uint32_t)((float)local_y / metrics.pixel_size);
    if (*out_sample_x >= ctx->document.raster_width || *out_sample_y >= ctx->document.raster_height) {
        return 0;
    }
    return 1;
}

static CoreResult apply_sample_if_changed(DrawingProgramAppContext *ctx,
                                          uint32_t sample_x,
                                          uint32_t sample_y,
                                          uint8_t value) {
    uint8_t current = 0u;
    CoreResult result;
    if (!ctx) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "null app context for sample apply" };
    }
    result = drawing_program_document_sample_read(&ctx->document, sample_x, sample_y, &current);
    if (result.code != CORE_OK) {
        return result;
    }
    if (current == value) {
        return core_result_ok();
    }
    return drawing_program_history_apply_set_sample_value(&ctx->history, &ctx->document, sample_x, sample_y, value);
}

static CoreResult apply_canvas_picker_at_screen(DrawingProgramAppContext *ctx,
                                                SDL_Rect pane_rect,
                                                int sx,
                                                int sy) {
    uint32_t sample_x = 0u;
    uint32_t sample_y = 0u;
    uint8_t sample = 0u;
    CoreResult result;
    if (!ctx) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "null app context for picker" };
    }
    if (!screen_to_canvas_sample(ctx, pane_rect, sx, sy, &sample_x, &sample_y)) {
        return core_result_ok();
    }
    result = drawing_program_document_sample_read(&ctx->document, sample_x, sample_y, &sample);
    if (result.code != CORE_OK) {
        return result;
    }
    ctx->ui_active_color_index = color_index_for_sample(sample);
    return core_result_ok();
}

static CoreResult apply_canvas_fill_at_screen(DrawingProgramAppContext *ctx,
                                              SDL_Rect pane_rect,
                                              int sx,
                                              int sy) {
    static uint32_t queue[DRAWING_PROGRAM_MAX_RASTER_SAMPLES];
    static uint8_t visited[DRAWING_PROGRAM_MAX_RASTER_SAMPLES];
    uint32_t start_x = 0u;
    uint32_t start_y = 0u;
    uint32_t width;
    uint32_t height;
    uint32_t start_index;
    uint32_t head = 0u;
    uint32_t tail = 0u;
    uint8_t target = 0u;
    uint8_t replacement = 0u;
    if (!ctx) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "null app context for fill" };
    }
    if (!screen_to_canvas_sample(ctx, pane_rect, sx, sy, &start_x, &start_y)) {
        return core_result_ok();
    }
    width = ctx->document.raster_width;
    height = ctx->document.raster_height;
    if (width == 0u || height == 0u || ctx->document.raster_sample_count == 0u) {
        return core_result_ok();
    }
    replacement = sample_value_for_tool(ctx, ctx->editor.active_tool);
    if (drawing_program_document_sample_read(&ctx->document, start_x, start_y, &target).code != CORE_OK) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "fill start sample read failed" };
    }
    if (target == replacement) {
        return core_result_ok();
    }
    memset(visited, 0, ctx->document.raster_sample_count * sizeof(visited[0]));
    start_index = (start_y * width) + start_x;
    queue[tail++] = start_index;
    visited[start_index] = 1u;
    while (head < tail) {
        uint32_t idx = queue[head++];
        uint32_t x = idx % width;
        uint32_t y = idx / width;
        uint8_t current = ctx->document.raster_samples[idx];
        if (current != target) {
            continue;
        }
        {
            CoreResult write_result = apply_sample_if_changed(ctx, x, y, replacement);
            if (write_result.code != CORE_OK) {
                return write_result;
            }
        }
        if (x > 0u) {
            uint32_t n = idx - 1u;
            if (!visited[n] && ctx->document.raster_samples[n] == target) {
                if (tail >= ctx->document.raster_sample_count) {
                    return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "fill queue exhausted" };
                }
                visited[n] = 1u;
                queue[tail++] = n;
            }
        }
        if (x + 1u < width) {
            uint32_t n = idx + 1u;
            if (!visited[n] && ctx->document.raster_samples[n] == target) {
                if (tail >= ctx->document.raster_sample_count) {
                    return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "fill queue exhausted" };
                }
                visited[n] = 1u;
                queue[tail++] = n;
            }
        }
        if (y > 0u) {
            uint32_t n = idx - width;
            if (!visited[n] && ctx->document.raster_samples[n] == target) {
                if (tail >= ctx->document.raster_sample_count) {
                    return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "fill queue exhausted" };
                }
                visited[n] = 1u;
                queue[tail++] = n;
            }
        }
        if (y + 1u < height) {
            uint32_t n = idx + width;
            if (!visited[n] && ctx->document.raster_samples[n] == target) {
                if (tail >= ctx->document.raster_sample_count) {
                    return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "fill queue exhausted" };
                }
                visited[n] = 1u;
                queue[tail++] = n;
            }
        }
    }
    return core_result_ok();
}

static CoreResult apply_canvas_line_between_samples(DrawingProgramAppContext *ctx,
                                                    uint32_t x0,
                                                    uint32_t y0,
                                                    uint32_t x1,
                                                    uint32_t y1,
                                                    uint8_t value) {
    int32_t x = (int32_t)x0;
    int32_t y = (int32_t)y0;
    int32_t tx = (int32_t)x1;
    int32_t ty = (int32_t)y1;
    int32_t dx = (tx > x) ? (tx - x) : (x - tx);
    int32_t sx = (x < tx) ? 1 : -1;
    int32_t dy = -((ty > y) ? (ty - y) : (y - ty));
    int32_t sy = (y < ty) ? 1 : -1;
    int32_t err = dx + dy;
    while (1) {
        CoreResult r;
        if (x >= 0 && y >= 0 &&
            (uint32_t)x < ctx->document.raster_width &&
            (uint32_t)y < ctx->document.raster_height) {
            r = apply_sample_if_changed(ctx, (uint32_t)x, (uint32_t)y, value);
            if (r.code != CORE_OK) {
                return r;
            }
        }
        if (x == tx && y == ty) {
            break;
        }
        {
            int32_t e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y += sy;
            }
        }
    }
    return core_result_ok();
}

static CoreResult apply_canvas_rect_between_samples(DrawingProgramAppContext *ctx,
                                                    uint32_t x0,
                                                    uint32_t y0,
                                                    uint32_t x1,
                                                    uint32_t y1,
                                                    uint8_t value) {
    uint32_t min_x = (x0 < x1) ? x0 : x1;
    uint32_t min_y = (y0 < y1) ? y0 : y1;
    uint32_t max_x = (x0 > x1) ? x0 : x1;
    uint32_t max_y = (y0 > y1) ? y0 : y1;
    CoreResult r;
    r = apply_canvas_line_between_samples(ctx, min_x, min_y, max_x, min_y, value);
    if (r.code != CORE_OK) return r;
    r = apply_canvas_line_between_samples(ctx, min_x, max_y, max_x, max_y, value);
    if (r.code != CORE_OK) return r;
    r = apply_canvas_line_between_samples(ctx, min_x, min_y, min_x, max_y, value);
    if (r.code != CORE_OK) return r;
    return apply_canvas_line_between_samples(ctx, max_x, min_y, max_x, max_y, value);
}

static CoreResult apply_canvas_circle_between_samples(DrawingProgramAppContext *ctx,
                                                      uint32_t center_x,
                                                      uint32_t center_y,
                                                      uint32_t edge_x,
                                                      uint32_t edge_y,
                                                      uint8_t value) {
    int32_t cx = (int32_t)center_x;
    int32_t cy = (int32_t)center_y;
    int32_t rx = (int32_t)edge_x - cx;
    int32_t ry = (int32_t)edge_y - cy;
    int32_t r = (rx < 0 ? -rx : rx);
    int32_t ay = (ry < 0 ? -ry : ry);
    int32_t x = 0;
    int32_t y;
    int32_t d;
    if (ay > r) {
        r = ay;
    }
    if (r <= 0) {
        return apply_sample_if_changed(ctx, center_x, center_y, value);
    }
    y = r;
    d = 1 - r;
    while (x <= y) {
        CoreResult rr;
#define APPLY_CIRCLE_POINT(px, py)                                                       \
    do {                                                                                 \
        if ((px) >= 0 && (py) >= 0 &&                                                    \
            (uint32_t)(px) < ctx->document.raster_width &&                              \
            (uint32_t)(py) < ctx->document.raster_height) {                             \
            rr = apply_sample_if_changed(ctx, (uint32_t)(px), (uint32_t)(py), value);   \
            if (rr.code != CORE_OK) {                                                    \
                return rr;                                                               \
            }                                                                            \
        }                                                                                \
    } while (0)
        APPLY_CIRCLE_POINT(cx + x, cy + y);
        APPLY_CIRCLE_POINT(cx - x, cy + y);
        APPLY_CIRCLE_POINT(cx + x, cy - y);
        APPLY_CIRCLE_POINT(cx - x, cy - y);
        APPLY_CIRCLE_POINT(cx + y, cy + x);
        APPLY_CIRCLE_POINT(cx - y, cy + x);
        APPLY_CIRCLE_POINT(cx + y, cy - x);
        APPLY_CIRCLE_POINT(cx - y, cy - x);
#undef APPLY_CIRCLE_POINT
        if (d < 0) {
            d += (2 * x) + 3;
        } else {
            d += (2 * (x - y)) + 5;
            y -= 1;
        }
        x += 1;
    }
    return core_result_ok();
}

static CoreResult apply_canvas_shape_commit(DrawingProgramAppContext *ctx,
                                            DrawingProgramToolKind tool,
                                            uint32_t start_x,
                                            uint32_t start_y,
                                            uint32_t end_x,
                                            uint32_t end_y) {
    uint8_t value;
    if (!ctx) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "null app context for shape commit" };
    }
    value = sample_value_for_tool(ctx, tool);
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_LINE:
            return apply_canvas_line_between_samples(ctx, start_x, start_y, end_x, end_y, value);
        case DRAWING_PROGRAM_TOOL_RECT:
            return apply_canvas_rect_between_samples(ctx, start_x, start_y, end_x, end_y, value);
        case DRAWING_PROGRAM_TOOL_CIRCLE:
            return apply_canvas_circle_between_samples(ctx, start_x, start_y, end_x, end_y, value);
        default:
            return core_result_ok();
    }
}

static int selection_sample_rect_to_screen_rect(const VisualCanvasSheetMetrics *metrics,
                                                int32_t sample_x,
                                                int32_t sample_y,
                                                uint32_t width,
                                                uint32_t height,
                                                SDL_Rect *out_rect) {
    int x;
    int y;
    int w;
    int h;
    if (!metrics || !out_rect || width == 0u || height == 0u) {
        return 0;
    }
    x = metrics->sheet_rect.x + (int)((float)sample_x * metrics->pixel_size);
    y = metrics->sheet_rect.y + (int)((float)sample_y * metrics->pixel_size);
    w = (int)((float)width * metrics->pixel_size);
    h = (int)((float)height * metrics->pixel_size);
    if (w < 1) {
        w = 1;
    }
    if (h < 1) {
        h = 1;
    }
    out_rect->x = x;
    out_rect->y = y;
    out_rect->w = w;
    out_rect->h = h;
    return 1;
}

static void draw_selection_overlay(SDL_Renderer *renderer,
                                   SDL_Rect pane_rect,
                                   const DrawingProgramAppContext *ctx,
                                   const CoreThemePreset *theme,
                                   const VisualCanvasSheetMetrics *metrics,
                                   const VisualSelectionState *selection) {
    SDL_Color accent = { 120u, 160u, 220u, 255u };
    SDL_Color accent_alt = { 220u, 180u, 120u, 255u };
    SDL_Rect rect = { 0, 0, 0, 0 };
    if (!renderer || !ctx || !metrics || !selection) {
        return;
    }
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_ACCENT_PRIMARY, &accent);
    accent_alt = sdl_color_shift_by_luma(accent, 40);
    (void)SDL_RenderSetClipRect(renderer, &pane_rect);
    if (selection->selecting) {
        int32_t min_x = (selection->marquee_start_x < selection->marquee_end_x)
                            ? (int32_t)selection->marquee_start_x
                            : (int32_t)selection->marquee_end_x;
        int32_t min_y = (selection->marquee_start_y < selection->marquee_end_y)
                            ? (int32_t)selection->marquee_start_y
                            : (int32_t)selection->marquee_end_y;
        int32_t max_x = (selection->marquee_start_x > selection->marquee_end_x)
                            ? (int32_t)selection->marquee_start_x
                            : (int32_t)selection->marquee_end_x;
        int32_t max_y = (selection->marquee_start_y > selection->marquee_end_y)
                            ? (int32_t)selection->marquee_start_y
                            : (int32_t)selection->marquee_end_y;
        uint32_t w = (uint32_t)(max_x - min_x + 1);
        uint32_t h = (uint32_t)(max_y - min_y + 1);
        if (selection_sample_rect_to_screen_rect(metrics, min_x, min_y, w, h, &rect)) {
            SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 32u);
            (void)SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 240u);
            (void)SDL_RenderDrawRect(renderer, &rect);
        }
    }
    if (selection->has_payload && selection->width > 0u && selection->height > 0u) {
        int32_t base_x = (int32_t)selection->origin_x;
        int32_t base_y = (int32_t)selection->origin_y;
        int32_t moved_x = base_x + selection->offset_x;
        int32_t moved_y = base_y + selection->offset_y;
        if (selection->moving &&
            (selection->offset_x != 0 || selection->offset_y != 0) &&
            selection_sample_rect_to_screen_rect(metrics,
                                                 base_x,
                                                 base_y,
                                                 selection->width,
                                                 selection->height,
                                                 &rect)) {
            SDL_SetRenderDrawColor(renderer, accent_alt.r, accent_alt.g, accent_alt.b, 30u);
            (void)SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, accent_alt.r, accent_alt.g, accent_alt.b, 170u);
            (void)SDL_RenderDrawRect(renderer, &rect);
        }
        if (selection_sample_rect_to_screen_rect(metrics,
                                                 moved_x,
                                                 moved_y,
                                                 selection->width,
                                                 selection->height,
                                                 &rect)) {
            SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 44u);
            (void)SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 255u);
            (void)SDL_RenderDrawRect(renderer, &rect);
        }
    }
    (void)SDL_RenderSetClipRect(renderer, 0);
}

static int sample_center_to_screen(const VisualCanvasSheetMetrics *metrics,
                                   const DrawingProgramAppContext *ctx,
                                   uint32_t sample_x,
                                   uint32_t sample_y,
                                   int *out_x,
                                   int *out_y) {
    float fx;
    float fy;
    if (!metrics || !ctx || !out_x || !out_y) {
        return 0;
    }
    if (sample_x >= ctx->document.raster_width || sample_y >= ctx->document.raster_height) {
        return 0;
    }
    fx = (float)metrics->sheet_rect.x + (((float)sample_x + 0.5f) * metrics->pixel_size);
    fy = (float)metrics->sheet_rect.y + (((float)sample_y + 0.5f) * metrics->pixel_size);
    *out_x = (int)fx;
    *out_y = (int)fy;
    return 1;
}

static void draw_shape_preview_overlay(SDL_Renderer *renderer,
                                       SDL_Rect pane_rect,
                                       const DrawingProgramAppContext *ctx,
                                       const CoreThemePreset *theme,
                                       const VisualCanvasSheetMetrics *metrics,
                                       const VisualCanvasInteractionState *interaction,
                                       const VisualPanelUiState *ui) {
    DrawingProgramToolKind tool;
    uint32_t end_x;
    uint32_t end_y;
    SDL_Color accent = { 120u, 160u, 220u, 255u };
    if (!renderer || !ctx || !metrics || !interaction) {
        return;
    }
    if (!interaction->shape_active) {
        return;
    }
    tool = (DrawingProgramToolKind)interaction->shape_tool;
    if (!tool_uses_shape_commit(tool)) {
        return;
    }
    end_x = interaction->shape_start_sample_x;
    end_y = interaction->shape_start_sample_y;
    if (ui && ui->mouse_known) {
        (void)screen_to_canvas_sample(ctx, pane_rect, ui->mouse_x, ui->mouse_y, &end_x, &end_y);
    }
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_ACCENT_PRIMARY, &accent);
    (void)SDL_RenderSetClipRect(renderer, &pane_rect);
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_LINE: {
            int x0 = 0;
            int y0 = 0;
            int x1 = 0;
            int y1 = 0;
            if (sample_center_to_screen(metrics,
                                        ctx,
                                        interaction->shape_start_sample_x,
                                        interaction->shape_start_sample_y,
                                        &x0,
                                        &y0) &&
                sample_center_to_screen(metrics, ctx, end_x, end_y, &x1, &y1)) {
                SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 230u);
                (void)SDL_RenderDrawLine(renderer, x0, y0, x1, y1);
            }
            break;
        }
        case DRAWING_PROGRAM_TOOL_RECT: {
            uint32_t min_x = (interaction->shape_start_sample_x < end_x) ? interaction->shape_start_sample_x : end_x;
            uint32_t min_y = (interaction->shape_start_sample_y < end_y) ? interaction->shape_start_sample_y : end_y;
            uint32_t max_x = (interaction->shape_start_sample_x > end_x) ? interaction->shape_start_sample_x : end_x;
            uint32_t max_y = (interaction->shape_start_sample_y > end_y) ? interaction->shape_start_sample_y : end_y;
            SDL_Rect rect = { 0, 0, 0, 0 };
            uint32_t w = (max_x - min_x) + 1u;
            uint32_t h = (max_y - min_y) + 1u;
            if (selection_sample_rect_to_screen_rect(metrics, (int32_t)min_x, (int32_t)min_y, w, h, &rect)) {
                SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 36u);
                (void)SDL_RenderFillRect(renderer, &rect);
                SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 230u);
                (void)SDL_RenderDrawRect(renderer, &rect);
            }
            break;
        }
        case DRAWING_PROGRAM_TOOL_CIRCLE: {
            int cx = 0;
            int cy = 0;
            int32_t dx = (int32_t)end_x - (int32_t)interaction->shape_start_sample_x;
            int32_t dy = (int32_t)end_y - (int32_t)interaction->shape_start_sample_y;
            int32_t r_samples = (dx < 0 ? -dx : dx);
            int32_t ay = (dy < 0 ? -dy : dy);
            float radius_px;
            int i;
            const int segments = 64;
            const float two_pi = 6.28318530718f;
            if (ay > r_samples) {
                r_samples = ay;
            }
            if (!sample_center_to_screen(metrics,
                                         ctx,
                                         interaction->shape_start_sample_x,
                                         interaction->shape_start_sample_y,
                                         &cx,
                                         &cy)) {
                break;
            }
            radius_px = ((float)r_samples + 0.5f) * metrics->pixel_size;
            if (radius_px < 1.0f) {
                radius_px = 1.0f;
            }
            SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 230u);
            for (i = 0; i < segments; ++i) {
                float t0 = ((float)i / (float)segments) * two_pi;
                float t1 = ((float)(i + 1) / (float)segments) * two_pi;
                int x0 = cx + (int)(cosf(t0) * radius_px);
                int y0 = cy + (int)(sinf(t0) * radius_px);
                int x1 = cx + (int)(cosf(t1) * radius_px);
                int y1 = cy + (int)(sinf(t1) * radius_px);
                (void)SDL_RenderDrawLine(renderer, x0, y0, x1, y1);
            }
            break;
        }
        default:
            break;
    }
    (void)SDL_RenderSetClipRect(renderer, 0);
}

static CoreResult apply_canvas_draw_at_screen(DrawingProgramAppContext *ctx,
                                              SDL_Rect pane_rect,
                                              int sx,
                                              int sy,
                                              VisualCanvasInteractionState *state) {
    uint32_t sample_x;
    uint32_t sample_y;
    uint32_t radius;
    uint32_t spacing;
    uint8_t value;
    CoreResult result;
    if (!ctx || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid canvas draw request" };
    }
    if (!screen_to_canvas_sample(ctx, pane_rect, sx, sy, &sample_x, &sample_y)) {
        return core_result_ok();
    }

    value = sample_value_for_tool(ctx, ctx->editor.active_tool);
    radius = tool_brush_radius_samples(ctx->editor.active_tool);
    spacing = tool_brush_spacing_samples(ctx->editor.active_tool, radius);

    if (!state->has_last_sample) {
        int32_t x;
        int32_t y;
        int32_t r = (int32_t)radius;
        for (y = -r; y <= r; ++y) {
            for (x = -r; x <= r; ++x) {
                int32_t tx = (int32_t)sample_x + x;
                int32_t ty = (int32_t)sample_y + y;
                uint8_t current = 0u;
                uint8_t write_value = value;
                if ((x * x) + (y * y) > (r * r)) {
                    continue;
                }
                if (tx < 0 || ty < 0 ||
                    (uint32_t)tx >= ctx->document.raster_width ||
                    (uint32_t)ty >= ctx->document.raster_height) {
                    continue;
                }
                if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_ERASER) {
                    write_value = seeded_background_sample_for_coord(&ctx->document, (uint32_t)tx, (uint32_t)ty);
                }
                result = drawing_program_document_sample_read(&ctx->document, (uint32_t)tx, (uint32_t)ty, &current);
                if (result.code != CORE_OK) {
                    return result;
                }
                if (current == write_value) {
                    continue;
                }
                result = drawing_program_history_apply_set_sample_value(&ctx->history,
                                                                        &ctx->document,
                                                                        (uint32_t)tx,
                                                                        (uint32_t)ty,
                                                                        write_value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
        state->has_last_sample = 1u;
        state->last_sample_x = sample_x;
        state->last_sample_y = sample_y;
        return core_result_ok();
    }

    if (state->last_sample_x == sample_x &&
        state->last_sample_y == sample_y) {
        return core_result_ok();
    }

    {
        int32_t x0 = (int32_t)state->last_sample_x;
        int32_t y0 = (int32_t)state->last_sample_y;
        int32_t x1 = (int32_t)sample_x;
        int32_t y1 = (int32_t)sample_y;
        int32_t dx = x1 - x0;
        int32_t dy = y1 - y0;
        int32_t adx = (dx < 0) ? -dx : dx;
        int32_t ady = (dy < 0) ? -dy : dy;
        int32_t steps = (adx > ady) ? adx : ady;
        uint32_t stamp_every = (spacing < 1u) ? 1u : spacing;
        int32_t i = 0;
        for (i = 1; i <= steps; ++i) {
            int32_t ix = x0 + (dx * i) / ((steps > 0) ? steps : 1);
            int32_t iy = y0 + (dy * i) / ((steps > 0) ? steps : 1);
            int32_t sxr;
            int32_t syr;
            int32_t rr = (int32_t)radius;
            if (((uint32_t)i % stamp_every) != 0u && i != steps) {
                continue;
            }
            for (syr = -rr; syr <= rr; ++syr) {
                for (sxr = -rr; sxr <= rr; ++sxr) {
                    int32_t tx = ix + sxr;
                    int32_t ty = iy + syr;
                    uint8_t current = 0u;
                    uint8_t write_value = value;
                    if ((sxr * sxr) + (syr * syr) > (rr * rr)) {
                        continue;
                    }
                    if (tx < 0 || ty < 0 ||
                        (uint32_t)tx >= ctx->document.raster_width ||
                        (uint32_t)ty >= ctx->document.raster_height) {
                        continue;
                    }
                    if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_ERASER) {
                        write_value = seeded_background_sample_for_coord(&ctx->document, (uint32_t)tx, (uint32_t)ty);
                    }
                    result = drawing_program_document_sample_read(&ctx->document, (uint32_t)tx, (uint32_t)ty, &current);
                    if (result.code != CORE_OK) {
                        return result;
                    }
                    if (current == write_value) {
                        continue;
                    }
                    result = drawing_program_history_apply_set_sample_value(&ctx->history,
                                                                            &ctx->document,
                                                                            (uint32_t)tx,
                                                                            (uint32_t)ty,
                                                                            write_value);
                    if (result.code != CORE_OK) {
                        return result;
                    }
                }
            }
        }
    }

    state->has_last_sample = 1u;
    state->last_sample_x = sample_x;
    state->last_sample_y = sample_y;
    return core_result_ok();
}

/* Keep SDL debug lane dependency-light: use tiny bitmap text until kit_render lane is active. */
static void bitmap_glyph_rows(unsigned char c, uint8_t rows[7]) {
    size_t i;
    for (i = 0; i < 7; ++i) {
        rows[i] = 0u;
    }
    if (c >= 'a' && c <= 'z') {
        c = (unsigned char)(c - 'a' + 'A');
    }
    switch (c) {
        case 'A': rows[0]=14; rows[1]=17; rows[2]=17; rows[3]=31; rows[4]=17; rows[5]=17; rows[6]=17; break;
        case 'B': rows[0]=30; rows[1]=17; rows[2]=17; rows[3]=30; rows[4]=17; rows[5]=17; rows[6]=30; break;
        case 'C': rows[0]=14; rows[1]=17; rows[2]=16; rows[3]=16; rows[4]=16; rows[5]=17; rows[6]=14; break;
        case 'D': rows[0]=30; rows[1]=17; rows[2]=17; rows[3]=17; rows[4]=17; rows[5]=17; rows[6]=30; break;
        case 'E': rows[0]=31; rows[1]=16; rows[2]=16; rows[3]=30; rows[4]=16; rows[5]=16; rows[6]=31; break;
        case 'F': rows[0]=31; rows[1]=16; rows[2]=16; rows[3]=30; rows[4]=16; rows[5]=16; rows[6]=16; break;
        case 'G': rows[0]=14; rows[1]=17; rows[2]=16; rows[3]=23; rows[4]=17; rows[5]=17; rows[6]=14; break;
        case 'H': rows[0]=17; rows[1]=17; rows[2]=17; rows[3]=31; rows[4]=17; rows[5]=17; rows[6]=17; break;
        case 'I': rows[0]=31; rows[1]=4; rows[2]=4; rows[3]=4; rows[4]=4; rows[5]=4; rows[6]=31; break;
        case 'J': rows[0]=1; rows[1]=1; rows[2]=1; rows[3]=1; rows[4]=17; rows[5]=17; rows[6]=14; break;
        case 'K': rows[0]=17; rows[1]=18; rows[2]=20; rows[3]=24; rows[4]=20; rows[5]=18; rows[6]=17; break;
        case 'L': rows[0]=16; rows[1]=16; rows[2]=16; rows[3]=16; rows[4]=16; rows[5]=16; rows[6]=31; break;
        case 'M': rows[0]=17; rows[1]=27; rows[2]=21; rows[3]=21; rows[4]=17; rows[5]=17; rows[6]=17; break;
        case 'N': rows[0]=17; rows[1]=25; rows[2]=21; rows[3]=19; rows[4]=17; rows[5]=17; rows[6]=17; break;
        case 'O': rows[0]=14; rows[1]=17; rows[2]=17; rows[3]=17; rows[4]=17; rows[5]=17; rows[6]=14; break;
        case 'P': rows[0]=30; rows[1]=17; rows[2]=17; rows[3]=30; rows[4]=16; rows[5]=16; rows[6]=16; break;
        case 'Q': rows[0]=14; rows[1]=17; rows[2]=17; rows[3]=17; rows[4]=21; rows[5]=18; rows[6]=13; break;
        case 'R': rows[0]=30; rows[1]=17; rows[2]=17; rows[3]=30; rows[4]=20; rows[5]=18; rows[6]=17; break;
        case 'S': rows[0]=15; rows[1]=16; rows[2]=16; rows[3]=14; rows[4]=1; rows[5]=1; rows[6]=30; break;
        case 'T': rows[0]=31; rows[1]=4; rows[2]=4; rows[3]=4; rows[4]=4; rows[5]=4; rows[6]=4; break;
        case 'U': rows[0]=17; rows[1]=17; rows[2]=17; rows[3]=17; rows[4]=17; rows[5]=17; rows[6]=14; break;
        case 'V': rows[0]=17; rows[1]=17; rows[2]=17; rows[3]=17; rows[4]=17; rows[5]=10; rows[6]=4; break;
        case 'W': rows[0]=17; rows[1]=17; rows[2]=17; rows[3]=21; rows[4]=21; rows[5]=21; rows[6]=10; break;
        case 'X': rows[0]=17; rows[1]=17; rows[2]=10; rows[3]=4; rows[4]=10; rows[5]=17; rows[6]=17; break;
        case 'Y': rows[0]=17; rows[1]=17; rows[2]=10; rows[3]=4; rows[4]=4; rows[5]=4; rows[6]=4; break;
        case 'Z': rows[0]=31; rows[1]=1; rows[2]=2; rows[3]=4; rows[4]=8; rows[5]=16; rows[6]=31; break;
        case '0': rows[0]=14; rows[1]=17; rows[2]=19; rows[3]=21; rows[4]=25; rows[5]=17; rows[6]=14; break;
        case '1': rows[0]=4; rows[1]=12; rows[2]=4; rows[3]=4; rows[4]=4; rows[5]=4; rows[6]=14; break;
        case '2': rows[0]=14; rows[1]=17; rows[2]=1; rows[3]=2; rows[4]=4; rows[5]=8; rows[6]=31; break;
        case '3': rows[0]=30; rows[1]=1; rows[2]=1; rows[3]=14; rows[4]=1; rows[5]=1; rows[6]=30; break;
        case '4': rows[0]=2; rows[1]=6; rows[2]=10; rows[3]=18; rows[4]=31; rows[5]=2; rows[6]=2; break;
        case '5': rows[0]=31; rows[1]=16; rows[2]=16; rows[3]=30; rows[4]=1; rows[5]=1; rows[6]=30; break;
        case '6': rows[0]=14; rows[1]=16; rows[2]=16; rows[3]=30; rows[4]=17; rows[5]=17; rows[6]=14; break;
        case '7': rows[0]=31; rows[1]=1; rows[2]=2; rows[3]=4; rows[4]=8; rows[5]=8; rows[6]=8; break;
        case '8': rows[0]=14; rows[1]=17; rows[2]=17; rows[3]=14; rows[4]=17; rows[5]=17; rows[6]=14; break;
        case '9': rows[0]=14; rows[1]=17; rows[2]=17; rows[3]=15; rows[4]=1; rows[5]=1; rows[6]=14; break;
        case ':': rows[0]=0; rows[1]=12; rows[2]=12; rows[3]=0; rows[4]=12; rows[5]=12; rows[6]=0; break;
        case '-': rows[0]=0; rows[1]=0; rows[2]=0; rows[3]=14; rows[4]=0; rows[5]=0; rows[6]=0; break;
        case ' ': break;
        default: rows[0]=14; rows[1]=17; rows[2]=1; rows[3]=2; rows[4]=4; rows[5]=0; rows[6]=4; break;
    }
}

static int draw_bitmap_text(SDL_Renderer *renderer,
                            SDL_Rect clip_rect,
                            int x,
                            int y,
                            const char *text,
                            SDL_Color color,
                            int scale) {
    TTF_Font *font = 0;
    SDL_Surface *surface = 0;
    SDL_Texture *texture = 0;
    int cursor_x = x;
    size_t i;
    if (!renderer || !text || scale < 1) {
        return 0;
    }
    if (g_visual_draw_ctx) {
        font = visual_get_ttf_font(g_visual_draw_ctx, scale);
    }
    if (font) {
        surface = TTF_RenderUTF8_Blended(font, text, color);
        if (surface) {
            texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                SDL_Rect dst = { x, y, surface->w, surface->h };
                (void)SDL_RenderSetClipRect(renderer, &clip_rect);
                (void)SDL_RenderCopy(renderer, texture, 0, &dst);
                (void)SDL_RenderSetClipRect(renderer, 0);
                cursor_x = x + surface->w;
                SDL_DestroyTexture(texture);
                SDL_FreeSurface(surface);
                return cursor_x - x;
            }
            SDL_FreeSurface(surface);
        }
    }
    /* Fallback bitmap path */
    (void)SDL_RenderSetClipRect(renderer, &clip_rect);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (i = 0; text[i] != '\0'; ++i) {
        uint8_t rows[7];
        int gy;
        int gx;
        bitmap_glyph_rows((unsigned char)text[i], rows);
        for (gy = 0; gy < 7; ++gy) {
            for (gx = 0; gx < 5; ++gx) {
                if ((rows[gy] >> (4 - gx)) & 1u) {
                    SDL_Rect px = { cursor_x + gx * scale, y + gy * scale, scale, scale };
                    (void)SDL_RenderFillRect(renderer, &px);
                }
            }
        }
        cursor_x += (5 * scale) + scale;
        if (cursor_x > clip_rect.x + clip_rect.w - 8) {
            break;
        }
    }
    (void)SDL_RenderSetClipRect(renderer, 0);
    return cursor_x - x;
}

static const char *tool_name(DrawingProgramToolKind tool) {
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_BRUSH: return "BRUSH";
        case DRAWING_PROGRAM_TOOL_ERASER: return "ERASER";
        case DRAWING_PROGRAM_TOOL_FILL: return "FILL";
        case DRAWING_PROGRAM_TOOL_LINE: return "LINE";
        case DRAWING_PROGRAM_TOOL_RECT: return "RECT";
        case DRAWING_PROGRAM_TOOL_CIRCLE: return "CIRCLE";
        case DRAWING_PROGRAM_TOOL_SELECT: return "SELECT";
        case DRAWING_PROGRAM_TOOL_MOVE: return "MOVE";
        case DRAWING_PROGRAM_TOOL_PICKER: return "PICKER";
        default: return "UNKNOWN";
    }
}

static int ui_hovered(const VisualPanelUiState *ui, SDL_Rect rect) {
    if (!ui || !ui->mouse_known) {
        return 0;
    }
    return point_in_rect(rect, ui->mouse_x, ui->mouse_y);
}

static void draw_tab_button(SDL_Renderer *renderer,
                            SDL_Rect clip_rect,
                            SDL_Rect rect,
                            const char *label,
                            SDL_Color fill,
                            SDL_Color fill_hover,
                            SDL_Color fill_active,
                            SDL_Color border,
                            SDL_Color text,
                            int text_scale,
                            int selected,
                            int hovered) {
    SDL_Color bg = selected ? fill_active : (hovered ? fill_hover : fill);
    SDL_Color text_final = sdl_color_ensure_contrast(text, bg);
    int glyph_h = 7 * text_scale;
    int text_y = rect.y + ((rect.h - glyph_h) / 2);
    if (text_y < rect.y + 2) {
        text_y = rect.y + 2;
    }
    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
    (void)SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
    (void)SDL_RenderDrawRect(renderer, &rect);
    draw_bitmap_text(renderer, clip_rect, rect.x + 6, text_y, label, text_final, text_scale);
}

static void draw_menu_bar_chrome(SDL_Renderer *renderer,
                                 SDL_Rect rect,
                                 const DrawingProgramAppContext *ctx,
                                 const CoreThemePreset *theme) {
    VisualPaneLayoutMetrics m;
    VisualThemePalette p;
    SDL_Rect chip;
    int chip_w;
    int chip_x;
    int header_y;
    int label_w;
    int tool_x;
    if (!renderer || !ctx) {
        return;
    }
    resolve_visual_theme_palette(theme, &p);
    m = make_pane_layout_metrics(ctx);
    header_y = rect.y + m.pad_y;
    draw_bitmap_text(renderer, rect, rect.x + m.pad_x, header_y, "MENU  FILE  EDIT  VIEW  HELP", p.text_primary, m.title_scale);
    chip_w = (int)(rect.w * 0.30f);
    if (chip_w < 170) {
        chip_w = 170;
    }
    if (chip_w > 300) {
        chip_w = 300;
    }
    chip_x = rect.x + rect.w - m.pad_x - chip_w;
    if (chip_x < rect.x + m.pad_x + 120) {
        chip_x = rect.x + m.pad_x + 120;
    }
    chip.x = chip_x;
    chip.y = header_y - 1;
    chip.w = chip_w;
    chip.h = m.tab_h;
    SDL_SetRenderDrawColor(renderer, p.button_fill.r, p.button_fill.g, p.button_fill.b, p.button_fill.a);
    (void)SDL_RenderFillRect(renderer, &chip);
    SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
    (void)SDL_RenderDrawRect(renderer, &chip);
    label_w = draw_bitmap_text(renderer, chip, chip.x + 6, chip.y + m.tab_text_y, "ACTIVE TOOL:", p.text_muted, m.body_scale);
    tool_x = chip.x + 6 + label_w + 6;
    draw_bitmap_text(renderer, chip, tool_x, chip.y + m.tab_text_y, tool_name(ctx->editor.active_tool), p.text_primary, m.body_scale);
}

static void draw_left_panel_chrome(SDL_Renderer *renderer,
                                   SDL_Rect rect,
                                   const DrawingProgramAppContext *ctx,
                                   const CoreThemePreset *theme,
                                   const VisualPanelUiState *ui) {
    uint32_t i;
    uint8_t left_slot;
    VisualPaneLayoutMetrics m;
    VisualThemePalette p;
    SDL_Rect tab_tools;
    SDL_Rect tab_view;
    int y;
    if (!renderer || !ctx) {
        return;
    }
    left_slot = clamp_left_slot(ctx->ui_left_panel_slot);
    m = make_pane_layout_metrics(ctx);
    resolve_visual_theme_palette(theme, &p);

    y = rect.y + m.pad_y;
    draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "LEFT PANEL", p.text_primary, m.title_scale);
    y += m.title_glyph_h + m.section_gap;
    tab_tools = (SDL_Rect){ rect.x + m.pad_x, y, (rect.w - (2 * m.pad_x) - m.tab_gap) / 2, m.tab_h };
    tab_view = (SDL_Rect){ tab_tools.x + tab_tools.w + m.tab_gap, y, (rect.w - (2 * m.pad_x) - m.tab_gap) / 2, m.tab_h };
    draw_tab_button(renderer,
                    rect,
                    tab_tools,
                    "TOOLS",
                    p.button_fill,
                    p.button_fill_hover,
                    p.button_fill_active,
                    p.button_border,
                    (left_slot == VISUAL_LEFT_PANEL_SLOT_TOOLS) ? p.text_primary : p.text_muted,
                    m.body_scale,
                    left_slot == VISUAL_LEFT_PANEL_SLOT_TOOLS,
                    ui_hovered(ui, tab_tools));
    draw_tab_button(renderer,
                    rect,
                    tab_view,
                    "VIEW",
                    p.button_fill,
                    p.button_fill_hover,
                    p.button_fill_active,
                    p.button_border,
                    (left_slot == VISUAL_LEFT_PANEL_SLOT_VIEW) ? p.text_primary : p.text_muted,
                    m.body_scale,
                    left_slot == VISUAL_LEFT_PANEL_SLOT_VIEW,
                    ui_hovered(ui, tab_view));
    y += m.tab_h + m.section_gap;

    if (left_slot == VISUAL_LEFT_PANEL_SLOT_TOOLS) {
        for (i = 0u; i < sizeof(k_visual_tools) / sizeof(k_visual_tools[0]); ++i) {
            SDL_Rect row = { rect.x + m.pad_x, y + (int)i * m.row_h, rect.w - (2 * m.pad_x), m.row_h };
            int active = (ctx->editor.active_tool == k_visual_tools[i]) ? 1 : 0;
            int hovered = ui_hovered(ui, row);
            SDL_Color row_color = active ? p.button_fill_active : (hovered ? p.button_fill_hover : p.button_fill);
            SDL_Color label_color = active ? p.text_primary : sdl_color_ensure_contrast(p.text_muted, row_color);
            SDL_SetRenderDrawColor(renderer, row_color.r, row_color.g, row_color.b, row_color.a);
            (void)SDL_RenderFillRect(renderer, &row);
            SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
            (void)SDL_RenderDrawRect(renderer, &row);
            draw_bitmap_text(renderer, rect, row.x + 6, row.y + m.row_text_y, tool_name(k_visual_tools[i]), label_color, m.body_scale);
        }
    } else {
        char line[96];
        SDL_Rect row;
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "WORLD NAV", p.text_primary, m.body_scale);
        y += m.line_h;
        (void)snprintf(line, sizeof(line), "PAN %d,%d", (int)ctx->editor.viewport.pan_x, (int)ctx->editor.viewport.pan_y);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line, sizeof(line), "ZOOM %.2fx", (double)ctx->editor.viewport.zoom);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h + m.section_gap;
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "RIGHT-DRAG: PAN", p.text_muted, m.body_scale);
        y += m.line_h;
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "WHEEL: ZOOM", p.text_muted, m.body_scale);
        y += m.line_h;
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "LEFT-DRAG: DRAW", p.text_muted, m.body_scale);
        y += m.line_h + m.section_gap;
        row = (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
        SDL_Color action_fill = ui_hovered(ui, row) ? p.button_fill_hover : p.button_fill;
        SDL_SetRenderDrawColor(renderer, action_fill.r, action_fill.g, action_fill.b, action_fill.a);
        (void)SDL_RenderFillRect(renderer, &row);
        SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
        (void)SDL_RenderDrawRect(renderer, &row);
        draw_bitmap_text(renderer, rect, row.x + 6, row.y + m.row_text_y, "ZOOM +", p.text_primary, m.body_scale);
    }
}

static const char *left_action_label_for_state(DrawingProgramToolKind tool,
                                               const VisualSelectionState *selection,
                                               const VisualCanvasInteractionState *interaction) {
    if (interaction && interaction->shape_active && tool_uses_shape_commit((DrawingProgramToolKind)interaction->shape_tool)) {
        DrawingProgramToolKind shape_tool = (DrawingProgramToolKind)interaction->shape_tool;
        if (shape_tool == DRAWING_PROGRAM_TOOL_LINE) {
            return "LEFT-DRAG: PREVIEW LINE";
        }
        if (shape_tool == DRAWING_PROGRAM_TOOL_RECT) {
            return "LEFT-DRAG: PREVIEW RECT";
        }
        if (shape_tool == DRAWING_PROGRAM_TOOL_CIRCLE) {
            return "LEFT-DRAG: PREVIEW CIRCLE";
        }
    }
    if (selection && selection->selecting) {
        return "LEFT-DRAG: MARQUEE SELECT";
    }
    if (selection && selection->moving) {
        return "LEFT-DRAG: MOVE SELECTION";
    }
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_SELECT: return "LEFT: MARQUEE SELECT";
        case DRAWING_PROGRAM_TOOL_MOVE: return "LEFT: MOVE SELECTION";
        case DRAWING_PROGRAM_TOOL_PICKER: return "LEFT: PICK COLOR";
        case DRAWING_PROGRAM_TOOL_FILL: return "LEFT: FILL REGION";
        case DRAWING_PROGRAM_TOOL_LINE: return "LEFT-DRAG: LINE";
        case DRAWING_PROGRAM_TOOL_RECT: return "LEFT-DRAG: RECT";
        case DRAWING_PROGRAM_TOOL_CIRCLE: return "LEFT-DRAG: CIRCLE";
        case DRAWING_PROGRAM_TOOL_ERASER: return "LEFT-DRAG: ERASE";
        case DRAWING_PROGRAM_TOOL_BRUSH:
        default:
            return "LEFT-DRAG: DRAW";
    }
}

static void draw_right_panel_chrome(SDL_Renderer *renderer,
                                    SDL_Rect rect,
                                    const DrawingProgramAppContext *ctx,
                                    const CoreThemePreset *theme,
                                    const VisualPanelUiState *ui,
                                    const VisualSelectionState *selection,
                                    const VisualCanvasInteractionState *interaction) {
    char line[96];
    uint8_t right_slot;
    VisualPaneLayoutMetrics m;
    VisualThemePalette p;
    SDL_Rect tab_canvas;
    SDL_Rect tab_layer;
    SDL_Rect row;
    int y;
    uint8_t active_visible = 0u;
    uint32_t i;
    if (!renderer || !ctx) {
        return;
    }
    right_slot = clamp_right_slot(ctx->ui_right_panel_slot);
    m = make_pane_layout_metrics(ctx);
    resolve_visual_theme_palette(theme, &p);

    for (i = 0u; i < ctx->document.layer_count; ++i) {
        if (ctx->document.layers[i].layer_id == ctx->editor.active_layer_id) {
            active_visible = ctx->document.layers[i].visible;
            break;
        }
    }

    y = rect.y + m.pad_y;
    draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "RIGHT PANEL", p.text_primary, m.title_scale);
    y += m.title_glyph_h + m.section_gap;
    tab_canvas = (SDL_Rect){ rect.x + m.pad_x, y, (rect.w - (2 * m.pad_x) - m.tab_gap) / 2, m.tab_h };
    tab_layer = (SDL_Rect){ tab_canvas.x + tab_canvas.w + m.tab_gap, y, (rect.w - (2 * m.pad_x) - m.tab_gap) / 2, m.tab_h };
    draw_tab_button(renderer,
                    rect,
                    tab_canvas,
                    "CANVAS",
                    p.button_fill,
                    p.button_fill_hover,
                    p.button_fill_active,
                    p.button_border,
                    (right_slot == VISUAL_RIGHT_PANEL_SLOT_CANVAS) ? p.text_primary : p.text_muted,
                    m.body_scale,
                    right_slot == VISUAL_RIGHT_PANEL_SLOT_CANVAS,
                    ui_hovered(ui, tab_canvas));
    draw_tab_button(renderer,
                    rect,
                    tab_layer,
                    "LAYER",
                    p.button_fill,
                    p.button_fill_hover,
                    p.button_fill_active,
                    p.button_border,
                    (right_slot == VISUAL_RIGHT_PANEL_SLOT_LAYER) ? p.text_primary : p.text_muted,
                    m.body_scale,
                    right_slot == VISUAL_RIGHT_PANEL_SLOT_LAYER,
                    ui_hovered(ui, tab_layer));
    y += m.tab_h + m.section_gap;

    if (right_slot == VISUAL_RIGHT_PANEL_SLOT_CANVAS) {
        const char *font_name = "unknown";
        const char *left_action = left_action_label_for_state(ctx->editor.active_tool, selection, interaction);
        const char *right_action = (interaction && interaction->panning_active) ? "RIGHT: PANNING VIEW" : "RIGHT: PAN VIEW";
        const char *pointer_state = "POINTER: UI/PANEL";
        uint32_t history_cursor_units = 0u;
        uint32_t history_count_units = 0u;
        int font_zoom_percent = 100 + (int)ctx->ui_font_zoom_step * 10;
        uint8_t active_color_index = drawing_program_color_index_clamp(ctx->ui_active_color_index);
        uint8_t active_color_value = drawing_program_color_value_from_index(active_color_index);
        uint8_t swatch_r = 0u;
        uint8_t swatch_g = 0u;
        uint8_t swatch_b = 0u;
        SDL_Rect reset_view_button;
        SDL_Rect clear_history_button;
        uint8_t palette_i;
        uint32_t brush_radius = tool_brush_radius_samples(ctx->editor.active_tool);
        uint32_t brush_spacing = tool_brush_spacing_samples(ctx->editor.active_tool, brush_radius);
        uint32_t selection_w = (selection && selection->has_payload) ? selection->width : 0u;
        uint32_t selection_h = (selection && selection->has_payload) ? selection->height : 0u;
        uint32_t selection_payload = (selection && selection->has_payload) ? selection->payload_count : 0u;
        if (ui && ui->mouse_known) {
            SDL_Rect canvas_rect = { 0, 0, 0, 0 };
            if (pane_rect_for_module_type(ctx, 1u, &canvas_rect) &&
                point_in_rect(canvas_rect, ui->mouse_x, ui->mouse_y)) {
                pointer_state = "POINTER: CANVAS";
            }
        }
        drawing_program_history_query_units(&ctx->history, &history_cursor_units, &history_count_units);
        (void)snprintf(line, sizeof(line), "HISTORY %u/%u", history_cursor_units, history_count_units);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line,
                       sizeof(line),
                       "RASTER %ux%u",
                       ctx->document.raster_width,
                       ctx->document.raster_height);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line,
                       sizeof(line),
                       "PAN %d,%d",
                       (int)ctx->editor.viewport.pan_x,
                       (int)ctx->editor.viewport.pan_y);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line, sizeof(line), "ZOOM %.2fx", (double)ctx->editor.viewport.zoom);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line, sizeof(line), "TOOL %s", tool_name(ctx->editor.active_tool));
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line, sizeof(line), "BRUSH R%u S%u", brush_radius, brush_spacing);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line, sizeof(line), "SELECTION %ux%u P%u", selection_w, selection_h, selection_payload);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        if (ctx->ui_font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
            font_name = core_font_preset_name((CoreFontPresetId)ctx->ui_font_preset_id);
        }
        (void)snprintf(line, sizeof(line), "FONT %s %d%%", font_name ? font_name : "unknown", font_zoom_percent);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h + m.section_gap;
        (void)snprintf(line, sizeof(line), "ACTIVE COLOR %u (%u)", (unsigned)active_color_index + 1u, (unsigned)active_color_value);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "PALETTE", p.text_primary, m.body_scale);
        for (palette_i = 0u; palette_i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++palette_i) {
            SDL_Rect swatch = right_canvas_palette_swatch_rect(rect, m, palette_i);
            int selected = (palette_i == active_color_index) ? 1 : 0;
            drawing_program_color_rgb_from_index(palette_i, &swatch_r, &swatch_g, &swatch_b);
            SDL_SetRenderDrawColor(renderer, swatch_r, swatch_g, swatch_b, 255u);
            (void)SDL_RenderFillRect(renderer, &swatch);
            if (selected) {
                SDL_SetRenderDrawColor(renderer, p.accent_primary.r, p.accent_primary.g, p.accent_primary.b, 255u);
            } else {
                SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
            }
            (void)SDL_RenderDrawRect(renderer, &swatch);
        }
        y = right_canvas_reset_view_button_rect(rect, m).y - (m.line_h * 4) - m.section_gap;
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, pointer_state, p.text_primary, m.body_scale);
        y += m.line_h;
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, left_action, p.text_muted, m.body_scale);
        y += m.line_h;
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, right_action, p.text_muted, m.body_scale);
        y += m.line_h;
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "WHEEL: ZOOM", p.text_muted, m.body_scale);
        reset_view_button = right_canvas_reset_view_button_rect(rect, m);
        row = reset_view_button;
        {
            SDL_Color action_fill = ui_hovered(ui, row) ? p.button_fill_hover : p.button_fill;
            SDL_SetRenderDrawColor(renderer, action_fill.r, action_fill.g, action_fill.b, action_fill.a);
        }
        (void)SDL_RenderFillRect(renderer, &row);
        SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
        (void)SDL_RenderDrawRect(renderer, &row);
        draw_bitmap_text(renderer, rect, row.x + 6, row.y + m.row_text_y, "RESET VIEW", p.text_primary, m.body_scale);
        clear_history_button = right_canvas_clear_history_button_rect(rect, m);
        row = clear_history_button;
        {
            SDL_Color action_fill = ui_hovered(ui, row) ? p.button_fill_hover : p.button_fill;
            SDL_SetRenderDrawColor(renderer, action_fill.r, action_fill.g, action_fill.b, action_fill.a);
        }
        (void)SDL_RenderFillRect(renderer, &row);
        SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
        (void)SDL_RenderDrawRect(renderer, &row);
        draw_bitmap_text(renderer, rect, row.x + 6, row.y + m.row_text_y, "CLEAR HISTORY", p.text_primary, m.body_scale);
    } else {
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "ACTIVE LAYER", p.text_primary, m.body_scale);
        y += m.line_h;
        (void)snprintf(line, sizeof(line), "LAYER %u", ctx->editor.active_layer_id);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        row = (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
        SDL_SetRenderDrawColor(renderer,
                               active_visible ? p.status_ok.r : p.status_warn.r,
                               active_visible ? p.status_ok.g : p.status_warn.g,
                               active_visible ? p.status_ok.b : p.status_warn.b,
                               ui_hovered(ui, row) ? 64u : 48u);
        (void)SDL_RenderFillRect(renderer, &row);
        SDL_SetRenderDrawColor(renderer,
                               active_visible ? p.status_ok.r : p.status_warn.r,
                               active_visible ? p.status_ok.g : p.status_warn.g,
                               active_visible ? p.status_ok.b : p.status_warn.b,
                               ui_hovered(ui, row) ? 220u : 180u);
        (void)SDL_RenderDrawRect(renderer, &row);
        draw_bitmap_text(renderer,
                         rect,
                         row.x + 6,
                         row.y + m.row_text_y,
                         active_visible ? "VISIBLE: ON (CLICK)" : "VISIBLE: OFF (CLICK)",
                         p.text_primary,
                         m.body_scale);
        y += m.row_h + m.section_gap;
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "HOTKEYS", p.text_primary, m.body_scale);
        y += m.line_h;
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "V TOGGLE LAYER", p.text_muted, m.body_scale);
        y += m.line_h;
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "Z/SHIFT+Z UNDO/REDO", p.text_muted, m.body_scale);
        y += m.line_h;
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "SPACE STAMP", p.text_muted, m.body_scale);
        y += m.line_h;
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "CMD/CTRL+K CLEAR HISTORY", p.text_muted, m.body_scale);
    }
}

static void apply_workflow_control_if_valid(DrawingProgramAppContext *ctx,
                                            DrawingProgramWorkflowControl control) {
    CoreResult control_result;
    if (!ctx || control == DRAWING_PROGRAM_WORKFLOW_CONTROL_NONE) {
        return;
    }
    control_result = drawing_program_runtime_orchestration_apply_workflow_control(ctx, control);
    if (control_result.code != CORE_OK && control_result.code != CORE_ERR_NOT_FOUND) {
        fprintf(stderr, "drawing_program: workflow control failed: %s\n", control_result.message);
    }
}

static void handle_left_panel_click(DrawingProgramAppContext *ctx,
                                    SDL_Rect rect,
                                    int x,
                                    int y,
                                    VisualPanelUiState *ui) {
    VisualPaneLayoutMetrics m;
    int content_y;
    SDL_Rect tab_tools;
    SDL_Rect tab_view;
    uint32_t i;
    if (!ctx || !ui) {
        return;
    }
    m = make_pane_layout_metrics(ctx);
    content_y = rect.y + m.pad_y + m.title_glyph_h + m.section_gap;
    tab_tools = (SDL_Rect){ rect.x + m.pad_x, content_y, (rect.w - (2 * m.pad_x) - m.tab_gap) / 2, m.tab_h };
    tab_view = (SDL_Rect){ tab_tools.x + tab_tools.w + m.tab_gap, content_y, (rect.w - (2 * m.pad_x) - m.tab_gap) / 2, m.tab_h };
    content_y += m.tab_h + m.section_gap;
    if (point_in_rect(tab_tools, x, y)) {
        ctx->ui_left_panel_slot = VISUAL_LEFT_PANEL_SLOT_TOOLS;
        sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (point_in_rect(tab_view, x, y)) {
        ctx->ui_left_panel_slot = VISUAL_LEFT_PANEL_SLOT_VIEW;
        sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (clamp_left_slot(ctx->ui_left_panel_slot) == VISUAL_LEFT_PANEL_SLOT_TOOLS) {
        for (i = 0u; i < sizeof(k_visual_tools) / sizeof(k_visual_tools[0]); ++i) {
            SDL_Rect row = { rect.x + m.pad_x, content_y + (int)i * m.row_h, rect.w - (2 * m.pad_x), m.row_h };
            if (point_in_rect(row, x, y)) {
                apply_workflow_control_if_valid(ctx, workflow_control_for_tool(k_visual_tools[i]));
                return;
            }
        }
    } else {
        int y_line = content_y + (m.line_h * 6) + (m.section_gap * 2);
        SDL_Rect zoom_in = { rect.x + m.pad_x, y_line, rect.w - (2 * m.pad_x), m.row_h };
        if (point_in_rect(zoom_in, x, y)) {
            float next_zoom = ctx->editor.viewport.zoom + 0.1f;
            if (next_zoom > 8.0f) {
                next_zoom = 8.0f;
            }
            ctx->editor.viewport.zoom = next_zoom;
        }
    }
}

static void handle_right_panel_click(DrawingProgramAppContext *ctx,
                                     SDL_Rect rect,
                                     int x,
                                     int y,
                                     VisualPanelUiState *ui) {
    VisualPaneLayoutMetrics m;
    int content_y;
    SDL_Rect tab_canvas;
    SDL_Rect tab_layer;
    if (!ctx || !ui) {
        return;
    }
    m = make_pane_layout_metrics(ctx);
    content_y = rect.y + m.pad_y + m.title_glyph_h + m.section_gap;
    tab_canvas = (SDL_Rect){ rect.x + m.pad_x, content_y, (rect.w - (2 * m.pad_x) - m.tab_gap) / 2, m.tab_h };
    tab_layer = (SDL_Rect){ tab_canvas.x + tab_canvas.w + m.tab_gap, content_y, (rect.w - (2 * m.pad_x) - m.tab_gap) / 2, m.tab_h };
    content_y += m.tab_h + m.section_gap;
    if (point_in_rect(tab_canvas, x, y)) {
        ctx->ui_right_panel_slot = VISUAL_RIGHT_PANEL_SLOT_CANVAS;
        sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (point_in_rect(tab_layer, x, y)) {
        ctx->ui_right_panel_slot = VISUAL_RIGHT_PANEL_SLOT_LAYER;
        sync_panel_ui_from_app(ctx, ui);
        return;
    }
    if (clamp_right_slot(ctx->ui_right_panel_slot) == VISUAL_RIGHT_PANEL_SLOT_LAYER) {
        int y_button = content_y + (m.line_h * 2);
        SDL_Rect visibility_button = { rect.x + m.pad_x, y_button, rect.w - (2 * m.pad_x), m.row_h };
        if (point_in_rect(visibility_button, x, y)) {
            apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY);
        }
    } else {
        uint8_t palette_i;
        SDL_Rect reset_view_button;
        SDL_Rect clear_history_button;
        for (palette_i = 0u; palette_i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++palette_i) {
            SDL_Rect swatch = right_canvas_palette_swatch_rect(rect, m, palette_i);
            if (point_in_rect(swatch, x, y)) {
                ctx->ui_active_color_index = palette_i;
                return;
            }
        }
        reset_view_button = right_canvas_reset_view_button_rect(rect, m);
        if (point_in_rect(reset_view_button, x, y)) {
            ctx->editor.viewport.pan_x = 0.0f;
            ctx->editor.viewport.pan_y = 0.0f;
            ctx->editor.viewport.zoom = 1.0f;
            return;
        }
        clear_history_button = right_canvas_clear_history_button_rect(rect, m);
        if (point_in_rect(clear_history_button, x, y)) {
            apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_HISTORY);
            return;
        }
    }
}

static void draw_canvas_viewport_chrome(SDL_Renderer *renderer,
                                        SDL_Rect rect,
                                        const DrawingProgramAppContext *ctx,
                                        const CoreThemePreset *theme) {
    VisualPaneLayoutMetrics m;
    char line[96];
    VisualThemePalette p;
    int y;
    if (!renderer || !ctx) {
        return;
    }
    m = make_pane_layout_metrics(ctx);
    resolve_visual_theme_palette(theme, &p);
    y = rect.y + m.pad_y;
    draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "VIEWPORT", p.text_primary, m.title_scale);
    y += m.title_glyph_h + m.section_gap;
    (void)snprintf(line, sizeof(line), "WORLD VIEW  ZOOM: %.2fx", (double)ctx->editor.viewport.zoom);
    draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
}

static void update_window_title(SDL_Window *window,
                                const DrawingProgramAppContext *ctx,
                                const VisualSelectionState *selection,
                                uint64_t present_count) {
    char title[256];
    uint32_t center_module_type_id;
    const char *font_name = "unknown";
    const char *text_backend = "bitmap";
    uint32_t selection_w = 0u;
    uint32_t selection_h = 0u;
    int32_t selection_dx = 0;
    int32_t selection_dy = 0;
    if (!window || !ctx) {
        return;
    }
    if (selection && selection->has_payload) {
        selection_w = selection->width;
        selection_h = selection->height;
        selection_dx = selection->offset_x;
        selection_dy = selection->offset_y;
    }
    center_module_type_id = module_type_for_pane(ctx, 6u);
    if (ctx->ui_font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
        font_name = core_font_preset_name((CoreFontPresetId)ctx->ui_font_preset_id);
    }
    if (g_visual_text_renderer.ttf_ready && !g_visual_text_renderer.ttf_failed) {
        text_backend = "ttf";
    }
    (void)snprintf(title,
                   sizeof(title),
                   "sketCh | backend=sdl-debug text_backend=%s frame=%llu present=%llu panes=%u center_module=%u active_tool=%u color=%u visible_layers=%u active_layer=%u sel=%ux%u d=%d,%d theme=%u font=%s zoomstep=%d",
                   text_backend,
                   (unsigned long long)ctx->frame_counter,
                   (unsigned long long)present_count,
                   ctx->pane_host.leaf_count,
                   center_module_type_id,
                   (unsigned)ctx->editor.active_tool,
                   (unsigned)drawing_program_color_index_clamp(ctx->ui_active_color_index),
                   ctx->render_projection.visible_layer_count,
                   ctx->render_projection.active_layer_id,
                   selection_w,
                   selection_h,
                   (int)selection_dx,
                   (int)selection_dy,
                   ctx->ui_theme_preset_id,
                   font_name ? font_name : "unknown",
                   (int)ctx->ui_font_zoom_step);
    SDL_SetWindowTitle(window, title);
}

static void draw_canvas_world_view(SDL_Renderer *renderer,
                                   SDL_Rect pane_rect,
                                   const DrawingProgramAppContext *ctx,
                                   const CoreThemePreset *theme,
                                   const VisualSelectionState *selection,
                                   const VisualPanelUiState *ui,
                                   const VisualCanvasInteractionState *interaction) {
    VisualCanvasSheetMetrics metrics;
    SDL_Color world_grid_minor = { 42u, 48u, 66u, 255u };
    SDL_Color world_grid_major = { 52u, 58u, 78u, 255u };
    SDL_Color sheet_border = { 210u, 210u, 220u, 255u };
    SDL_Color sheet_fill = { 244u, 244u, 248u, 255u };
    SDL_Rect draw_sheet;
    SDL_Rect clip_sheet;
    int px;
    int py;
    if (!renderer || !ctx) {
        return;
    }
    if (ctx->document.raster_width == 0u ||
        ctx->document.raster_height == 0u ||
        ctx->document.raster_sample_count == 0u) {
        return;
    }
    compute_canvas_sheet_metrics(ctx, pane_rect, &metrics);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_SURFACE_1, &world_grid_minor);
    (void)resolve_theme_color(theme, CORE_THEME_COLOR_SURFACE_2, &world_grid_major);
    world_grid_minor = sdl_color_shift_by_luma(world_grid_minor, 8);
    world_grid_major = sdl_color_shift_by_luma(world_grid_major, 16);

    (void)SDL_RenderSetClipRect(renderer, &pane_rect);
    SDL_SetRenderDrawColor(renderer, world_grid_minor.r, world_grid_minor.g, world_grid_minor.b, 255u);
    for (px = pane_rect.x; px < pane_rect.x + pane_rect.w; px += 24) {
        (void)SDL_RenderDrawLine(renderer, px, pane_rect.y, px, pane_rect.y + pane_rect.h);
    }
    for (py = pane_rect.y; py < pane_rect.y + pane_rect.h; py += 24) {
        (void)SDL_RenderDrawLine(renderer, pane_rect.x, py, pane_rect.x + pane_rect.w, py);
    }
    SDL_SetRenderDrawColor(renderer, world_grid_major.r, world_grid_major.g, world_grid_major.b, 255u);
    for (px = pane_rect.x; px < pane_rect.x + pane_rect.w; px += 96) {
        (void)SDL_RenderDrawLine(renderer, px, pane_rect.y, px, pane_rect.y + pane_rect.h);
    }
    for (py = pane_rect.y; py < pane_rect.y + pane_rect.h; py += 96) {
        (void)SDL_RenderDrawLine(renderer, pane_rect.x, py, pane_rect.x + pane_rect.w, py);
    }

    draw_sheet = metrics.sheet_rect;
    clip_sheet = draw_sheet;
    if (clip_sheet.x < pane_rect.x) {
        clip_sheet.w -= (pane_rect.x - clip_sheet.x);
        clip_sheet.x = pane_rect.x;
    }
    if (clip_sheet.y < pane_rect.y) {
        clip_sheet.h -= (pane_rect.y - clip_sheet.y);
        clip_sheet.y = pane_rect.y;
    }
    if (clip_sheet.x + clip_sheet.w > pane_rect.x + pane_rect.w) {
        clip_sheet.w = pane_rect.x + pane_rect.w - clip_sheet.x;
    }
    if (clip_sheet.y + clip_sheet.h > pane_rect.y + pane_rect.h) {
        clip_sheet.h = pane_rect.y + pane_rect.h - clip_sheet.y;
    }
    if (clip_sheet.w <= 0 || clip_sheet.h <= 0) {
        (void)SDL_RenderSetClipRect(renderer, 0);
        return;
    }

    SDL_SetRenderDrawColor(renderer, sheet_fill.r, sheet_fill.g, sheet_fill.b, sheet_fill.a);
    (void)SDL_RenderFillRect(renderer, &clip_sheet);

    if (visual_canvas_texture_sync(renderer, ctx) && g_visual_canvas_texture.texture) {
        (void)SDL_RenderSetClipRect(renderer, &clip_sheet);
        (void)SDL_RenderCopy(renderer, g_visual_canvas_texture.texture, 0, &draw_sheet);
    }
    (void)SDL_RenderSetClipRect(renderer, &pane_rect);
    SDL_SetRenderDrawColor(renderer, sheet_border.r, sheet_border.g, sheet_border.b, sheet_border.a);
    (void)SDL_RenderDrawRect(renderer, &draw_sheet);
    draw_selection_overlay(renderer, pane_rect, ctx, theme, &metrics, selection);
    draw_shape_preview_overlay(renderer, pane_rect, ctx, theme, &metrics, interaction, ui);
    (void)SDL_RenderSetClipRect(renderer, 0);
}

static int draw_visual_debug_frame(SDL_Window *window,
                                   SDL_Renderer *renderer,
                                   const DrawingProgramAppContext *ctx,
                                   const CoreThemePreset *theme,
                                   const VisualPanelUiState *ui,
                                   const VisualSelectionState *selection,
                                   const VisualCanvasInteractionState *interaction) {
    int width = 0;
    int height = 0;
    uint32_t i;
    SDL_Color background;
    if (!window || !renderer || !ctx) {
        return 0;
    }
    if (ctx->overlay_adapter.lifecycle_state != DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE ||
        ctx->overlay_adapter.runtime_paused) {
        return 0;
    }
    if (ctx->pane_host.leaf_count == 0u) {
        return 0;
    }

    (void)window;
    if (SDL_GetRendererOutputSize(renderer, &width, &height) != 0) {
        return 0;
    }
    if (width <= 0 || height <= 0) {
        return 0;
    }

    if (!theme || resolve_theme_color(theme, CORE_THEME_COLOR_SURFACE_0, &background).code != CORE_OK) {
        background = (SDL_Color){ 18u, 20u, 28u, 255u };
    }
    SDL_SetRenderDrawColor(renderer, background.r, background.g, background.b, background.a);
    if (SDL_RenderClear(renderer) != 0) {
        return 0;
    }

    for (i = 0u; i < ctx->pane_host.leaf_count; ++i) {
        const CorePaneLeafRect *leaf = &ctx->pane_host.leaves[i];
        uint32_t module_type_id = module_type_for_pane(ctx, (uint32_t)leaf->id);
        SDL_Color fill;
        SDL_Color border;
        SDL_Rect rect;
        module_color(module_type_id, theme, &fill, &border);
        rect.x = (int)(leaf->rect.x);
        rect.y = (int)(leaf->rect.y);
        rect.w = (int)(leaf->rect.width);
        rect.h = (int)(leaf->rect.height);
        if (rect.w < 2) {
            rect.w = 2;
        }
        if (rect.h < 2) {
            rect.h = 2;
        }
        SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
        (void)SDL_RenderFillRect(renderer, &rect);
        if (module_type_id == 3u) {
            draw_menu_bar_chrome(renderer, rect, ctx, theme);
        } else if (module_type_id == 2u) {
            draw_left_panel_chrome(renderer, rect, ctx, theme, ui);
        } else if (module_type_id == 4u) {
            draw_right_panel_chrome(renderer, rect, ctx, theme, ui, selection, interaction);
        } else if (module_type_id == 1u) {
            draw_canvas_world_view(renderer, rect, ctx, theme, selection, ui, interaction);
            draw_canvas_viewport_chrome(renderer, rect, ctx, theme);
        }
        SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
        (void)SDL_RenderDrawRect(renderer, &rect);
    }

    return 1;
}

static void cancel_canvas_draw_and_shape(VisualCanvasInteractionState *interaction) {
    if (!interaction) {
        return;
    }
    interaction->drawing_active = 0u;
    interaction->shape_active = 0u;
    interaction->has_last_sample = 0u;
}

static void cancel_selection_transient(VisualSelectionState *selection) {
    if (!selection) {
        return;
    }
    selection->selecting = 0u;
    selection->moving = 0u;
    selection->offset_x = 0;
    selection->offset_y = 0;
}

static void begin_canvas_history_group(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    (void)drawing_program_history_begin_group(&ctx->history);
}

static void end_canvas_history_group(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    (void)drawing_program_history_end_group(&ctx->history);
}

static void cancel_all_transient_interactions(DrawingProgramAppContext *ctx,
                                              VisualCanvasInteractionState *interaction,
                                              VisualSelectionState *selection,
                                              int clear_pan_state) {
    end_canvas_history_group(ctx);
    cancel_canvas_draw_and_shape(interaction);
    cancel_selection_transient(selection);
    if (clear_pan_state && interaction) {
        interaction->panning_active = 0u;
    }
}

static int run_visual_mode(int argc, char **argv) {
    CoreResult result;
    DrawingProgramRenderBackendKind backend_kind = DRAWING_PROGRAM_RENDER_BACKEND_SDL_DEBUG;
    CoreThemePreset theme_preset;
    CoreResult theme_env_result;
    CoreThemePresetId selected_theme = CORE_THEME_PRESET_DARK_DEFAULT;
    CoreFontPresetId selected_font = CORE_FONT_PRESET_IDE;
    DrawingProgramAppContext app;
    SDL_Window *window = 0;
    SDL_Renderer *renderer = 0;
    int quit = 0;
    uint64_t present_count = 0u;
    VisualCanvasInteractionState canvas_interaction;
    VisualPanelUiState panel_ui;
    VisualSelectionState selection_state;

    result = drawing_program_render_backend_parse_flag(argc, argv, &backend_kind);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: %s\n", result.message);
        return 1;
    }
    if (!drawing_program_render_backend_is_supported_now(backend_kind)) {
        fprintf(stderr,
                "drawing_program: render backend '%s' is defined but not implemented in P4-S1 (use --render-backend sdl-debug)\n",
                drawing_program_render_backend_kind_string(backend_kind));
        return 2;
    }
    theme_env_result = core_theme_apply_env_override("CORE_THEME_PRESET", &selected_theme);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "drawing_program: SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("sketCh",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              1280,
                              800,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        fprintf(stderr, "drawing_program: SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        fprintf(stderr, "drawing_program: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    result = drawing_program_app_bootstrap(&app, argc, argv);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: bootstrap failed: %s\n", result.message);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    result = drawing_program_app_config_load(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: config_load failed: %s\n", result.message);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    result = drawing_program_app_state_seed(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: state_seed failed: %s\n", result.message);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    result = drawing_program_app_subsystems_init(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: subsystems_init failed: %s\n", result.message);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    result = drawing_program_runtime_start(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: runtime_start failed: %s\n", result.message);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    if (visual_trace_ui_state_enabled()) {
        fprintf(stderr,
                "drawing_program trace visual after_runtime_start tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u\n",
                (unsigned)app.editor.active_tool,
                (unsigned)app.ui_theme_preset_id,
                (unsigned)app.ui_font_preset_id,
                (int)app.ui_font_zoom_step,
                (unsigned)app.ui_left_panel_slot,
                (unsigned)app.ui_right_panel_slot);
    }
    if (theme_env_result.code == CORE_OK && !app.snapshot_loaded_from_preset) {
        app.ui_theme_preset_id = (uint32_t)selected_theme;
    }
    selected_theme = clamp_theme_preset_id(app.ui_theme_preset_id);
    app.ui_theme_preset_id = (uint32_t)selected_theme;
    if (core_theme_get_preset(selected_theme, &theme_preset).code != CORE_OK) {
        selected_theme = CORE_THEME_PRESET_DARK_DEFAULT;
        app.ui_theme_preset_id = (uint32_t)selected_theme;
        if (core_theme_get_preset(selected_theme, &theme_preset).code != CORE_OK) {
            fprintf(stderr, "drawing_program: failed to resolve core theme preset\n");
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
    }
    if (app.ui_font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
        selected_font = (CoreFontPresetId)app.ui_font_preset_id;
    } else {
        app.ui_font_preset_id = (uint32_t)selected_font;
    }
    if (!core_font_preset_name(selected_font)) {
        selected_font = CORE_FONT_PRESET_IDE;
        app.ui_font_preset_id = (uint32_t)selected_font;
    }
    app.ui_font_zoom_step = (int8_t)clamp_font_zoom_step((int)app.ui_font_zoom_step);
    memset(&canvas_interaction, 0, sizeof(canvas_interaction));
    memset(&panel_ui, 0, sizeof(panel_ui));
    visual_selection_reset(&selection_state);
    sync_panel_ui_from_app(&app, &panel_ui);
    if (visual_trace_ui_state_enabled()) {
        fprintf(stderr,
                "drawing_program trace visual after_ui_resolve tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u env_override=%d\n",
                (unsigned)app.editor.active_tool,
                (unsigned)app.ui_theme_preset_id,
                (unsigned)app.ui_font_preset_id,
                (int)app.ui_font_zoom_step,
                (unsigned)app.ui_left_panel_slot,
                (unsigned)app.ui_right_panel_slot,
                theme_env_result.code == CORE_OK ? 1 : 0);
    }
    {
        int initial_w = 0;
        int initial_h = 0;
        if (SDL_GetRendererOutputSize(renderer, &initial_w, &initial_h) == 0 &&
            initial_w > 0 && initial_h > 0) {
            result = drawing_program_app_set_pane_host_bounds(&app, (float)initial_w, (float)initial_h);
            if (result.code != CORE_OK) {
                fprintf(stderr, "drawing_program: set pane host bounds failed: %s\n", result.message);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 1;
            }
        }
    }

    while (!quit) {
        SDL_Event event;
        int current_w = 0;
        int current_h = 0;
        while (SDL_PollEvent(&event)) {
            int event_x = 0;
            int event_y = 0;
            int event_has_position = 0;
            SDL_Rect canvas_pane = { 0, 0, 0, 0 };
            SDL_Rect left_pane = { 0, 0, 0, 0 };
            SDL_Rect right_pane = { 0, 0, 0, 0 };
            int has_canvas_pane = pane_rect_for_module_type(&app, 1u, &canvas_pane);
            int has_left_pane = pane_rect_for_module_type(&app, 2u, &left_pane);
            int has_right_pane = pane_rect_for_module_type(&app, 4u, &right_pane);
            if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
                map_input_to_render_coords(window, renderer, event.button.x, event.button.y, &event_x, &event_y);
                event_has_position = 1;
            } else if (event.type == SDL_MOUSEMOTION) {
                map_input_to_render_coords(window, renderer, event.motion.x, event.motion.y, &event_x, &event_y);
                event_has_position = 1;
            }
            if (event_has_position) {
                panel_ui.mouse_known = 1u;
                panel_ui.mouse_x = event_x;
                panel_ui.mouse_y = event_y;
            }
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                quit = 1;
            }
            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_LEAVE) {
                    panel_ui.mouse_known = 0u;
                } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                    panel_ui.mouse_known = 0u;
                    cancel_all_transient_interactions(&app, &canvas_interaction, &selection_state, 1);
                }
            }
            if (event.type == SDL_MOUSEWHEEL && has_canvas_pane) {
                int mx = 0;
                int my = 0;
                SDL_GetMouseState(&mx, &my);
                map_input_to_render_coords(window, renderer, mx, my, &mx, &my);
                if (point_in_rect(canvas_pane, mx, my)) {
                    float next_zoom = app.editor.viewport.zoom + ((float)event.wheel.y * 0.1f);
                    if (next_zoom < 0.25f) {
                        next_zoom = 0.25f;
                    }
                    if (next_zoom > 8.0f) {
                        next_zoom = 8.0f;
                    }
                    app.editor.viewport.zoom = next_zoom;
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int click_x = event_has_position ? event_x : event.button.x;
                int click_y = event_has_position ? event_y : event.button.y;
                uint32_t sample_x = 0u;
                uint32_t sample_y = 0u;
                int click_on_left = has_left_pane && point_in_rect(left_pane, click_x, click_y);
                int click_on_right = has_right_pane && point_in_rect(right_pane, click_x, click_y);
                int click_on_canvas = has_canvas_pane && point_in_rect(canvas_pane, click_x, click_y);
                if (event.button.button == SDL_BUTTON_RIGHT && click_on_canvas) {
                    /* Pan takes precedence over all left-tool interaction modes. */
                    cancel_all_transient_interactions(&app, &canvas_interaction, &selection_state, 0);
                    canvas_interaction.panning_active = 1u;
                    canvas_interaction.last_mouse_x = click_x;
                    canvas_interaction.last_mouse_y = click_y;
                    continue;
                }
                if (event.button.button == SDL_BUTTON_LEFT && click_on_left) {
                    cancel_all_transient_interactions(&app, &canvas_interaction, &selection_state, 0);
                    handle_left_panel_click(&app, left_pane, click_x, click_y, &panel_ui);
                    continue;
                }
                if (event.button.button == SDL_BUTTON_LEFT && click_on_right) {
                    cancel_all_transient_interactions(&app, &canvas_interaction, &selection_state, 0);
                    handle_right_panel_click(&app, right_pane, click_x, click_y, &panel_ui);
                    continue;
                }
                if (event.button.button == SDL_BUTTON_LEFT &&
                    !click_on_left &&
                    !click_on_right &&
                    !click_on_canvas) {
                    /* Clicking outside known panes clears stale transient modes. */
                    cancel_all_transient_interactions(&app, &canvas_interaction, &selection_state, 0);
                    continue;
                }
                if (event.button.button == SDL_BUTTON_LEFT && click_on_canvas && !canvas_interaction.panning_active) {
                    if (app.editor.active_tool == DRAWING_PROGRAM_TOOL_SELECT &&
                        screen_to_canvas_sample(&app, canvas_pane, click_x, click_y, &sample_x, &sample_y)) {
                        selection_state.selecting = 1u;
                        selection_state.moving = 0u;
                        selection_state.marquee_start_x = sample_x;
                        selection_state.marquee_start_y = sample_y;
                        selection_state.marquee_end_x = sample_x;
                        selection_state.marquee_end_y = sample_y;
                        cancel_canvas_draw_and_shape(&canvas_interaction);
                    } else if (app.editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE &&
                               selection_state.has_payload &&
                               screen_to_canvas_sample(&app, canvas_pane, click_x, click_y, &sample_x, &sample_y) &&
                               visual_selection_begin_move(&selection_state, sample_x, sample_y)) {
                        selection_state.moving = 1u;
                        selection_state.selecting = 0u;
                        selection_state.move_anchor_sample_x = sample_x;
                        selection_state.move_anchor_sample_y = sample_y;
                        selection_state.move_anchor_offset_x = selection_state.offset_x;
                        selection_state.move_anchor_offset_y = selection_state.offset_y;
                        cancel_canvas_draw_and_shape(&canvas_interaction);
                    } else if (app.editor.active_tool == DRAWING_PROGRAM_TOOL_PICKER) {
                        cancel_all_transient_interactions(&app, &canvas_interaction, &selection_state, 0);
                        (void)apply_canvas_picker_at_screen(&app, canvas_pane, click_x, click_y);
                    } else if (app.editor.active_tool == DRAWING_PROGRAM_TOOL_FILL) {
                        begin_canvas_history_group(&app);
                        (void)apply_canvas_fill_at_screen(&app, canvas_pane, click_x, click_y);
                        cancel_all_transient_interactions(&app, &canvas_interaction, &selection_state, 0);
                    } else if (tool_uses_shape_commit(app.editor.active_tool) &&
                               screen_to_canvas_sample(&app, canvas_pane, click_x, click_y, &sample_x, &sample_y)) {
                        begin_canvas_history_group(&app);
                        cancel_selection_transient(&selection_state);
                        canvas_interaction.shape_active = 1u;
                        canvas_interaction.shape_tool = (uint8_t)app.editor.active_tool;
                        canvas_interaction.shape_start_sample_x = sample_x;
                        canvas_interaction.shape_start_sample_y = sample_y;
                        canvas_interaction.drawing_active = 0u;
                        canvas_interaction.has_last_sample = 0u;
                    } else if (tool_uses_direct_sample_stroke(app.editor.active_tool)) {
                        canvas_interaction.shape_active = 0u;
                        canvas_interaction.drawing_active = 1u;
                        canvas_interaction.has_last_sample = 0u;
                        begin_canvas_history_group(&app);
                        (void)apply_canvas_draw_at_screen(&app,
                                                          canvas_pane,
                                                          click_x,
                                                          click_y,
                                                          &canvas_interaction);
                    }
                }
            }
            if (event.type == SDL_MOUSEBUTTONUP) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int release_x = event_has_position ? event_x : event.button.x;
                    int release_y = event_has_position ? event_y : event.button.y;
                    uint32_t sample_x = 0u;
                    uint32_t sample_y = 0u;
                    if (canvas_interaction.panning_active) {
                        cancel_all_transient_interactions(&app, &canvas_interaction, &selection_state, 1);
                        continue;
                    }
                    if (has_canvas_pane &&
                        screen_to_canvas_sample(&app, canvas_pane, release_x, release_y, &sample_x, &sample_y)) {
                        if (selection_state.selecting) {
                            selection_state.marquee_end_x = sample_x;
                            selection_state.marquee_end_y = sample_y;
                        }
                        if (selection_state.moving) {
                            int32_t dx = (int32_t)sample_x - (int32_t)selection_state.move_anchor_sample_x;
                            int32_t dy = (int32_t)sample_y - (int32_t)selection_state.move_anchor_sample_y;
                            selection_state.offset_x = selection_state.move_anchor_offset_x + dx;
                            selection_state.offset_y = selection_state.move_anchor_offset_y + dy;
                        }
                    }
                    if (selection_state.selecting) {
                        (void)visual_selection_capture_from_marquee(&app, &selection_state);
                    }
                    if (selection_state.moving) {
                        begin_canvas_history_group(&app);
                        CoreResult move_commit = visual_selection_commit_move(&app, &selection_state);
                        end_canvas_history_group(&app);
                        if (move_commit.code != CORE_OK) {
                            fprintf(stderr, "drawing_program: selection move commit failed: %s\n", move_commit.message);
                            selection_state.moving = 0u;
                            selection_state.offset_x = 0;
                            selection_state.offset_y = 0;
                        }
                    }
                    if (canvas_interaction.shape_active &&
                        tool_uses_shape_commit((DrawingProgramToolKind)canvas_interaction.shape_tool) &&
                        has_canvas_pane &&
                        screen_to_canvas_sample(&app, canvas_pane, release_x, release_y, &sample_x, &sample_y)) {
                        (void)apply_canvas_shape_commit(&app,
                                                        (DrawingProgramToolKind)canvas_interaction.shape_tool,
                                                        canvas_interaction.shape_start_sample_x,
                                                        canvas_interaction.shape_start_sample_y,
                                                        sample_x,
                                                        sample_y);
                    }
                    cancel_all_transient_interactions(&app, &canvas_interaction, &selection_state, 0);
                }
                if (event.button.button == SDL_BUTTON_RIGHT) {
                    cancel_all_transient_interactions(&app, &canvas_interaction, &selection_state, 1);
                }
            }
            if (event.type == SDL_MOUSEMOTION && has_canvas_pane) {
                panel_ui.mouse_known = 1u;
                panel_ui.mouse_x = event_has_position ? event_x : event.motion.x;
                panel_ui.mouse_y = event_has_position ? event_y : event.motion.y;
                if (canvas_interaction.panning_active) {
                    int dx = panel_ui.mouse_x - canvas_interaction.last_mouse_x;
                    int dy = panel_ui.mouse_y - canvas_interaction.last_mouse_y;
                    app.editor.viewport.pan_x += (float)dx;
                    app.editor.viewport.pan_y += (float)dy;
                    canvas_interaction.last_mouse_x = panel_ui.mouse_x;
                    canvas_interaction.last_mouse_y = panel_ui.mouse_y;
                    continue;
                }
                if (selection_state.selecting && app.editor.active_tool == DRAWING_PROGRAM_TOOL_SELECT) {
                    uint32_t sample_x = 0u;
                    uint32_t sample_y = 0u;
                    if (screen_to_canvas_sample(&app,
                                                canvas_pane,
                                                panel_ui.mouse_x,
                                                panel_ui.mouse_y,
                                                &sample_x,
                                                &sample_y)) {
                        selection_state.marquee_end_x = sample_x;
                        selection_state.marquee_end_y = sample_y;
                    }
                }
                if (selection_state.moving && app.editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE) {
                    uint32_t sample_x = 0u;
                    uint32_t sample_y = 0u;
                    if (screen_to_canvas_sample(&app,
                                                canvas_pane,
                                                panel_ui.mouse_x,
                                                panel_ui.mouse_y,
                                                &sample_x,
                                                &sample_y)) {
                        int32_t dx = (int32_t)sample_x - (int32_t)selection_state.move_anchor_sample_x;
                        int32_t dy = (int32_t)sample_y - (int32_t)selection_state.move_anchor_sample_y;
                        selection_state.offset_x = selection_state.move_anchor_offset_x + dx;
                        selection_state.offset_y = selection_state.move_anchor_offset_y + dy;
                    }
                }
                if (canvas_interaction.drawing_active) {
                    (void)apply_canvas_draw_at_screen(&app,
                                                      canvas_pane,
                                                      panel_ui.mouse_x,
                                                      panel_ui.mouse_y,
                                                      &canvas_interaction);
                }
            }
            if (event.type == SDL_KEYDOWN) {
                DrawingProgramWorkflowControl control = DRAWING_PROGRAM_WORKFLOW_CONTROL_NONE;
                int ctrl_or_cmd = (event.key.keysym.mod & (KMOD_CTRL | KMOD_GUI)) != 0;
                int shift = (event.key.keysym.mod & KMOD_SHIFT) != 0;
                if (ctrl_or_cmd && shift &&
                    event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_4) {
                    uint32_t module_type_id = (uint32_t)(event.key.keysym.sym - SDLK_1) + 1u;
                    (void)set_module_type_for_pane(&app, 6u, module_type_id);
                    continue;
                }
                if (ctrl_or_cmd &&
                    (event.key.keysym.sym == SDLK_EQUALS ||
                     event.key.keysym.sym == SDLK_PLUS ||
                     event.key.keysym.sym == SDLK_KP_PLUS ||
                     event.key.keysym.sym == SDLK_MINUS ||
                     event.key.keysym.sym == SDLK_KP_MINUS ||
                     event.key.keysym.sym == SDLK_0 ||
                     event.key.keysym.sym == SDLK_KP_0)) {
                    int step = (int)app.ui_font_zoom_step;
                    if (event.key.keysym.sym == SDLK_EQUALS ||
                        event.key.keysym.sym == SDLK_PLUS ||
                        event.key.keysym.sym == SDLK_KP_PLUS) {
                        step += 1;
                    } else if (event.key.keysym.sym == SDLK_MINUS ||
                               event.key.keysym.sym == SDLK_KP_MINUS) {
                        step -= 1;
                    } else {
                        step = 0;
                    }
                    app.ui_font_zoom_step = (int8_t)clamp_font_zoom_step(step);
                    continue;
                }
                if (ctrl_or_cmd && event.key.keysym.sym == SDLK_k) {
                    apply_workflow_control_if_valid(&app, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_HISTORY);
                    continue;
                }
                if (ctrl_or_cmd && shift &&
                    (event.key.keysym.sym == SDLK_t || event.key.keysym.sym == SDLK_y)) {
                    CoreThemePresetId next_theme = selected_theme;
                    int direction = (event.key.keysym.sym == SDLK_t) ? 1 : -1;
                    selected_theme = clamp_theme_preset_id(app.ui_theme_preset_id);
                    if (cycle_theme_preset(selected_theme, direction, &next_theme)) {
                        selected_theme = next_theme;
                        app.ui_theme_preset_id = (uint32_t)selected_theme;
                        if (core_theme_get_preset(selected_theme, &theme_preset).code != CORE_OK) {
                            selected_theme = CORE_THEME_PRESET_DARK_DEFAULT;
                            app.ui_theme_preset_id = (uint32_t)selected_theme;
                            (void)core_theme_get_preset(selected_theme, &theme_preset);
                        }
                    }
                    continue;
                }
                if (event.key.keysym.sym == SDLK_LEFTBRACKET || event.key.keysym.sym == SDLK_RIGHTBRACKET) {
                    int color = (int)drawing_program_color_index_clamp(app.ui_active_color_index);
                    if (event.key.keysym.sym == SDLK_LEFTBRACKET) {
                        color -= 1;
                    } else {
                        color += 1;
                    }
                    if (color < 0) {
                        color = (int)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT - 1;
                    }
                    if (color >= (int)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT) {
                        color = 0;
                    }
                    app.ui_active_color_index = (uint8_t)color;
                    continue;
                }
                if (event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_8) {
                    uint8_t selected = (uint8_t)(event.key.keysym.sym - SDLK_1);
                    if (selected < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT) {
                        app.ui_active_color_index = selected;
                        continue;
                    }
                }
                switch (event.key.keysym.sym) {
                    case SDLK_b:
                        control = DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_BRUSH;
                        break;
                    case SDLK_e:
                        control = DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_ERASER;
                        break;
                    case SDLK_f:
                        control = DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_FILL;
                        break;
                    case SDLK_l:
                        control = DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_LINE;
                        break;
                    case SDLK_r:
                        control = DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_RECT;
                        break;
                    case SDLK_c:
                        control = DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_CIRCLE;
                        break;
                    case SDLK_s:
                        control = DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_SELECT;
                        break;
                    case SDLK_m:
                        control = DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_MOVE;
                        break;
                    case SDLK_i:
                        control = DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_PICKER;
                        break;
                    case SDLK_v:
                        control = DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY;
                        break;
                    case SDLK_SPACE:
                        control = DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE;
                        break;
                    case SDLK_z:
                        control = (event.key.keysym.mod & KMOD_SHIFT)
                                      ? DRAWING_PROGRAM_WORKFLOW_CONTROL_REDO
                                      : DRAWING_PROGRAM_WORKFLOW_CONTROL_UNDO;
                        break;
                    case SDLK_y:
                        control = DRAWING_PROGRAM_WORKFLOW_CONTROL_REDO;
                        break;
                    default:
                        break;
                }
                apply_workflow_control_if_valid(&app, control);
                if (control != DRAWING_PROGRAM_WORKFLOW_CONTROL_NONE) {
                    cancel_all_transient_interactions(&app, &canvas_interaction, &selection_state, 0);
                }
            }
        }
        if (SDL_GetRendererOutputSize(renderer, &current_w, &current_h) == 0 &&
            current_w > 0 && current_h > 0) {
            result = drawing_program_app_set_pane_host_bounds(&app, (float)current_w, (float)current_h);
            if (result.code != CORE_OK) {
                fprintf(stderr, "drawing_program: set pane host bounds failed: %s\n", result.message);
                break;
            }
        }

        result = drawing_program_app_run_loop(&app);
        if (result.code != CORE_OK) {
            fprintf(stderr, "drawing_program: run_loop failed: %s\n", result.message);
            break;
        }
        g_visual_draw_ctx = &app;
        if (!draw_visual_debug_frame(window,
                                     renderer,
                                     &app,
                                     &theme_preset,
                                     &panel_ui,
                                     &selection_state,
                                     &canvas_interaction)) {
            g_visual_draw_ctx = 0;
            fprintf(stderr, "drawing_program: visual debug frame draw failed\n");
            result = (CoreResult){ CORE_ERR_IO, "visual debug frame draw failed" };
            break;
        }
        g_visual_draw_ctx = 0;
        SDL_RenderPresent(renderer);
        present_count += 1u;
        update_window_title(window, &app, &selection_state, present_count);
        SDL_Delay(16);
    }

    result = drawing_program_app_shutdown(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: shutdown failed: %s\n", result.message);
    }

    visual_text_renderer_shutdown();
    visual_canvas_texture_shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return (result.code == CORE_OK) ? 0 : 1;
}

int drawing_program_app_visual_main(int argc, char **argv) {
    if (has_flag(argc, argv, "--headless")) {
        return drawing_program_app_main(argc, argv);
    }
    return run_visual_mode(argc, argv);
}
