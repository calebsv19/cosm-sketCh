#include "drawing_program/drawing_program_visual_resources.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_layer_raster.h"
#include "drawing_program/drawing_program_render_domain.h"

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

typedef struct VisualCanvasTextureState {
    SDL_Texture *texture;
    SDL_PixelFormat *pixel_format;
    uint32_t width;
    uint32_t height;
    uint8_t *composited_samples;
    uint32_t composited_capacity;
    uint8_t has_sync_signature;
    uint32_t last_raster_hash32;
    uint32_t last_raster_nonzero_count;
    uint32_t last_layer_opacity_hash32;
} VisualCanvasTextureState;

static VisualTextRendererState g_visual_text_renderer = {0};
static VisualCanvasTextureState g_visual_canvas_texture = {0};

static uint32_t visual_layer_opacity_hash32(const uint8_t *layer_opacity_percent,
                                            uint32_t layer_opacity_count) {
    uint32_t hash32 = 2166136261u;
    uint32_t i;
    if (!layer_opacity_percent || layer_opacity_count == 0u) {
        return 0u;
    }
    for (i = 0u; i < layer_opacity_count; ++i) {
        hash32 ^= (uint32_t)layer_opacity_percent[i];
        hash32 *= 16777619u;
    }
    return hash32;
}

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

void drawing_program_visual_font_cache_shutdown(void) {
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

void drawing_program_visual_canvas_texture_shutdown(void) {
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
    free(g_visual_canvas_texture.composited_samples);
    g_visual_canvas_texture.composited_samples = 0;
    g_visual_canvas_texture.composited_capacity = 0u;
    g_visual_canvas_texture.has_sync_signature = 0u;
    g_visual_canvas_texture.last_raster_hash32 = 0u;
    g_visual_canvas_texture.last_raster_nonzero_count = 0u;
    g_visual_canvas_texture.last_layer_opacity_hash32 = 0u;
}

int drawing_program_visual_canvas_texture_sync_with_signature(
    SDL_Renderer *renderer,
    const struct DrawingProgramDocument *document,
    const struct DrawingProgramLayerRasterStore *layer_rasters,
    const uint8_t *layer_opacity_percent,
    uint32_t layer_opacity_count,
    uint32_t raster_hash32,
    uint32_t raster_nonzero_count) {
    void *pixels = 0;
    int pitch = 0;
    uint32_t x;
    uint32_t y;
    uint32_t layer_opacity_hash32 = 0u;
    const uint8_t *source_samples = 0;
    CoreResult compose_result;
    int signature_unchanged = 0;
    if (!renderer || !document || !layer_rasters || !layer_opacity_percent || layer_opacity_count == 0u) {
        return 0;
    }
    if (document->raster_width == 0u ||
        document->raster_height == 0u ||
        document->raster_sample_count == 0u) {
        return 0;
    }
    layer_opacity_hash32 = visual_layer_opacity_hash32(layer_opacity_percent, layer_opacity_count);
    if (!g_visual_canvas_texture.pixel_format) {
        g_visual_canvas_texture.pixel_format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
        if (!g_visual_canvas_texture.pixel_format) {
            return 0;
        }
    }
    if (!g_visual_canvas_texture.texture ||
        g_visual_canvas_texture.width != document->raster_width ||
        g_visual_canvas_texture.height != document->raster_height) {
        drawing_program_visual_canvas_texture_shutdown();
        g_visual_canvas_texture.texture = SDL_CreateTexture(renderer,
                                                            SDL_PIXELFORMAT_RGBA8888,
                                                            SDL_TEXTUREACCESS_STREAMING,
                                                            (int)document->raster_width,
                                                            (int)document->raster_height);
        if (!g_visual_canvas_texture.texture) {
            return 0;
        }
        g_visual_canvas_texture.pixel_format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
        if (!g_visual_canvas_texture.pixel_format) {
            drawing_program_visual_canvas_texture_shutdown();
            return 0;
        }
        g_visual_canvas_texture.width = document->raster_width;
        g_visual_canvas_texture.height = document->raster_height;
        g_visual_canvas_texture.has_sync_signature = 0u;
        (void)SDL_SetTextureBlendMode(g_visual_canvas_texture.texture, SDL_BLENDMODE_NONE);
    }
    signature_unchanged = (g_visual_canvas_texture.has_sync_signature &&
                           g_visual_canvas_texture.last_raster_hash32 == raster_hash32 &&
                           g_visual_canvas_texture.last_raster_nonzero_count == raster_nonzero_count &&
                           g_visual_canvas_texture.last_layer_opacity_hash32 == layer_opacity_hash32);
    if (signature_unchanged) {
        return 1;
    }
    if (g_visual_canvas_texture.composited_capacity < document->raster_sample_count) {
        uint8_t *next_samples = (uint8_t *)realloc(g_visual_canvas_texture.composited_samples,
                                                   (size_t)document->raster_sample_count);
        if (!next_samples) {
            return 0;
        }
        g_visual_canvas_texture.composited_samples = next_samples;
        g_visual_canvas_texture.composited_capacity = document->raster_sample_count;
    }
    source_samples = document->raster_samples;
    compose_result = drawing_program_render_compose_visible_samples_with_layer_opacity(document,
                                                                                        layer_rasters,
                                                                                        layer_opacity_percent,
                                                                                        layer_opacity_count,
                                                                                        g_visual_canvas_texture.composited_samples,
                                                                                        g_visual_canvas_texture.composited_capacity);
    if (compose_result.code == CORE_OK) {
        source_samples = g_visual_canvas_texture.composited_samples;
    }
    if (SDL_LockTexture(g_visual_canvas_texture.texture, 0, &pixels, &pitch) != 0) {
        return 0;
    }
    for (y = 0u; y < document->raster_height; ++y) {
        uint32_t *row = (uint32_t *)((uint8_t *)pixels + ((size_t)y * (size_t)pitch));
        size_t row_offset = (size_t)y * (size_t)document->raster_width;
        for (x = 0u; x < document->raster_width; ++x) {
            uint8_t sample = source_samples[row_offset + x];
            uint8_t r = 0u;
            uint8_t g = 0u;
            uint8_t b = 0u;
            drawing_program_color_rgb_from_sample(sample, &r, &g, &b);
            row[x] = SDL_MapRGBA(g_visual_canvas_texture.pixel_format, r, g, b, 255u);
        }
    }
    SDL_UnlockTexture(g_visual_canvas_texture.texture);
    g_visual_canvas_texture.has_sync_signature = 1u;
    g_visual_canvas_texture.last_raster_hash32 = raster_hash32;
    g_visual_canvas_texture.last_raster_nonzero_count = raster_nonzero_count;
    g_visual_canvas_texture.last_layer_opacity_hash32 = layer_opacity_hash32;
    return 1;
}

int drawing_program_visual_canvas_texture_sync(SDL_Renderer *renderer,
                                               const struct DrawingProgramDocument *document,
                                               const struct DrawingProgramLayerRasterStore *layer_rasters,
                                               const uint8_t *layer_opacity_percent,
                                               uint32_t layer_opacity_count) {
    g_visual_canvas_texture.has_sync_signature = 0u;
    return drawing_program_visual_canvas_texture_sync_with_signature(renderer,
                                                                     document,
                                                                     layer_rasters,
                                                                     layer_opacity_percent,
                                                                     layer_opacity_count,
                                                                     0u,
                                                                     0u);
}

SDL_Texture *drawing_program_visual_canvas_texture_get(void) {
    return g_visual_canvas_texture.texture;
}

TTF_Font *drawing_program_visual_get_ttf_font_for_preset(uint32_t ui_font_preset_id, int scale) {
    size_t i;
    int point_size;
    CoreFontPresetId preset_id;
    if (scale < 1 || scale > 8 || g_visual_text_renderer.ttf_failed) {
        return 0;
    }
    preset_id = (ui_font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT)
                    ? (CoreFontPresetId)ui_font_preset_id
                    : CORE_FONT_PRESET_IDE;
    if (!g_visual_text_renderer.ttf_ready || g_visual_text_renderer.preset_id != preset_id) {
        drawing_program_visual_font_cache_shutdown();
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

int drawing_program_visual_font_backend_is_ready(void) {
    return (g_visual_text_renderer.ttf_ready && !g_visual_text_renderer.ttf_failed) ? 1 : 0;
}
