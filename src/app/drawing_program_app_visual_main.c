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

static int active_layer_query(const DrawingProgramAppContext *ctx,
                              uint32_t *out_layer_id,
                              uint32_t *out_index,
                              uint8_t *out_visible,
                              uint8_t *out_locked) {
    CoreResult result;
    result = drawing_program_runtime_orchestration_resolve_active_layer(ctx,
                                                                        out_layer_id,
                                                                        out_index,
                                                                        out_visible,
                                                                        out_locked);
    if (result.code != CORE_OK) {
        return 0;
    }
    return 1;
}

static int active_layer_allows_edits_visual(const DrawingProgramAppContext *ctx) {
    uint8_t visible = 0u;
    uint8_t locked = 0u;
    if (!active_layer_query(ctx, 0, 0, &visible, &locked)) {
        return 0;
    }
    return (visible && !locked) ? 1 : 0;
}

static CoreResult active_layer_sample_read_visual(const DrawingProgramAppContext *ctx,
                                                  uint32_t sample_x,
                                                  uint32_t sample_y,
                                                  uint8_t *out_value) {
    CoreResult result;
    if (!ctx || !out_value) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid active-layer sample read request" };
    }
    result = drawing_program_layer_raster_store_sample_read(&ctx->layer_rasters,
                                                            ctx->editor.active_layer_id,
                                                            sample_x,
                                                            sample_y,
                                                            out_value);
    if (result.code == CORE_OK) {
        return core_result_ok();
    }
    return drawing_program_document_sample_read(&ctx->document, sample_x, sample_y, out_value);
}

static uint8_t clamp_percent_u8(uint8_t value) {
    return (value > 100u) ? 100u : value;
}

static void visual_layer_opacity_compact(DrawingProgramAppContext *ctx) {
    uint8_t compact_count = 0u;
    uint8_t i;
    if (!ctx) {
        return;
    }
    for (i = 0u; i < ctx->ui_layer_opacity_entry_count; ++i) {
        uint32_t layer_id = ctx->ui_layer_opacity_layer_ids[i];
        uint32_t ignored_index = 0u;
        if (layer_id != 0u &&
            drawing_program_document_layer_index_for_id(&ctx->document, layer_id, &ignored_index).code == CORE_OK) {
            ctx->ui_layer_opacity_layer_ids[compact_count] = layer_id;
            ctx->ui_layer_opacity_values[compact_count] =
                clamp_percent_u8(ctx->ui_layer_opacity_values[i]);
            compact_count += 1u;
        }
    }
    ctx->ui_layer_opacity_entry_count = compact_count;
}

static uint8_t visual_layer_opacity_get(const DrawingProgramAppContext *ctx, uint32_t layer_id) {
    uint8_t i;
    if (!ctx || layer_id == 0u) {
        return 100u;
    }
    for (i = 0u; i < ctx->ui_layer_opacity_entry_count; ++i) {
        if (ctx->ui_layer_opacity_layer_ids[i] == layer_id) {
            return clamp_percent_u8(ctx->ui_layer_opacity_values[i]);
        }
    }
    return 100u;
}

static void visual_layer_opacity_set(DrawingProgramAppContext *ctx,
                                     uint32_t layer_id,
                                     uint8_t opacity_percent) {
    uint8_t i;
    uint8_t opacity = clamp_percent_u8(opacity_percent);
    if (!ctx || layer_id == 0u) {
        return;
    }
    for (i = 0u; i < ctx->ui_layer_opacity_entry_count; ++i) {
        if (ctx->ui_layer_opacity_layer_ids[i] == layer_id) {
            ctx->ui_layer_opacity_values[i] = opacity;
            return;
        }
    }
    if (ctx->ui_layer_opacity_entry_count >= DRAWING_PROGRAM_MAX_LAYERS) {
        return;
    }
    ctx->ui_layer_opacity_layer_ids[ctx->ui_layer_opacity_entry_count] = layer_id;
    ctx->ui_layer_opacity_values[ctx->ui_layer_opacity_entry_count] = opacity;
    ctx->ui_layer_opacity_entry_count += 1u;
}

static void visual_layer_opacity_sync_document(DrawingProgramAppContext *ctx) {
    uint32_t i;
    if (!ctx) {
        return;
    }
    visual_layer_opacity_compact(ctx);
    for (i = 0u; i < ctx->document.layer_count; ++i) {
        visual_layer_opacity_set(ctx, ctx->document.layers[i].layer_id, 100u);
    }
}

static CoreResult layer_sample_read_visual(const DrawingProgramAppContext *ctx,
                                           uint32_t layer_id,
                                           uint32_t sample_x,
                                           uint32_t sample_y,
                                           uint8_t *out_value) {
    CoreResult result;
    if (!ctx || !out_value || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid layer sample read request" };
    }
    result = drawing_program_layer_raster_store_sample_read(&ctx->layer_rasters,
                                                            layer_id,
                                                            sample_x,
                                                            sample_y,
                                                            out_value);
    if (result.code == CORE_OK) {
        return core_result_ok();
    }
    return drawing_program_document_sample_read(&ctx->document, sample_x, sample_y, out_value);
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

static uint8_t clamp_setting_u8(uint8_t value, uint8_t min_v, uint8_t max_v) {
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

static uint32_t tool_brush_radius_samples(const DrawingProgramAppContext *ctx, DrawingProgramToolKind tool) {
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_BRUSH:
            return (uint32_t)clamp_setting_u8(ctx ? ctx->ui_tool_brush_size : 2u, 1u, 16u);
        case DRAWING_PROGRAM_TOOL_ERASER:
            return (uint32_t)clamp_setting_u8(ctx ? ctx->ui_tool_eraser_size : 4u, 1u, 16u);
        default:
            return 0u;
    }
}

static uint32_t tool_brush_spacing_samples(const DrawingProgramAppContext *ctx,
                                           DrawingProgramToolKind tool,
                                           uint32_t radius) {
    uint32_t spacing = 1u;
    if (tool == DRAWING_PROGRAM_TOOL_BRUSH && ctx) {
        spacing = (uint32_t)clamp_setting_u8(ctx->ui_tool_brush_spacing, 1u, 16u);
    } else if (radius > 0u) {
        spacing = (radius / 2u) + 1u;
    }
    if (spacing < 1u) {
        spacing = 1u;
    }
    return spacing;
}

static uint8_t tool_brush_hardness_percent(const DrawingProgramAppContext *ctx,
                                           DrawingProgramToolKind tool) {
    if (tool == DRAWING_PROGRAM_TOOL_BRUSH && ctx) {
        return clamp_setting_u8(ctx->ui_tool_brush_hardness, 1u, 100u);
    }
    return 100u;
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

static uint8_t tool_shape_mode(const DrawingProgramAppContext *ctx) {
    return clamp_setting_u8(ctx ? ctx->ui_tool_shape_mode : 0u, 0u, 2u);
}

static uint32_t tool_shape_stroke_width(const DrawingProgramAppContext *ctx) {
    return (uint32_t)clamp_setting_u8(ctx ? ctx->ui_tool_shape_stroke_width : 1u, 1u, 16u);
}

static const char *shape_mode_name(uint8_t mode) {
    switch (clamp_setting_u8(mode, 0u, 2u)) {
        case 0u: return "OUTLINE";
        case 1u: return "FILL";
        case 2u: return "FILL+OUTLINE";
        default: return "OUTLINE";
    }
}

static int shape_mode_includes_fill(DrawingProgramToolKind tool, uint8_t mode) {
    if (tool == DRAWING_PROGRAM_TOOL_LINE) {
        return 0;
    }
    return (clamp_setting_u8(mode, 0u, 2u) == 1u || clamp_setting_u8(mode, 0u, 2u) == 2u) ? 1 : 0;
}

static int shape_mode_includes_outline(DrawingProgramToolKind tool, uint8_t mode) {
    if (tool == DRAWING_PROGRAM_TOOL_LINE) {
        return 1;
    }
    return (clamp_setting_u8(mode, 0u, 2u) == 1u) ? 0 : 1;
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

typedef enum VisualToolOptionKind {
    VISUAL_TOOL_OPTION_BRUSH_SIZE = 0,
    VISUAL_TOOL_OPTION_BRUSH_OPACITY,
    VISUAL_TOOL_OPTION_BRUSH_SPACING,
    VISUAL_TOOL_OPTION_BRUSH_HARDNESS,
    VISUAL_TOOL_OPTION_ERASER_SIZE,
    VISUAL_TOOL_OPTION_SHAPE_STROKE_WIDTH,
    VISUAL_TOOL_OPTION_SHAPE_MODE,
    VISUAL_TOOL_OPTION_FILL_TOLERANCE,
    VISUAL_TOOL_OPTION_SELECT_DELETE
} VisualToolOptionKind;

static uint32_t visual_tool_option_count(const DrawingProgramAppContext *ctx, DrawingProgramToolKind tool) {
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_BRUSH:
            return 4u;
        case DRAWING_PROGRAM_TOOL_ERASER:
            return 1u;
        case DRAWING_PROGRAM_TOOL_LINE:
        case DRAWING_PROGRAM_TOOL_RECT:
        case DRAWING_PROGRAM_TOOL_CIRCLE:
            return 2u;
        case DRAWING_PROGRAM_TOOL_FILL:
            return 1u;
        case DRAWING_PROGRAM_TOOL_SELECT:
            return (ctx && ctx->selection.has_payload) ? 1u : 0u;
        default:
            return 0u;
    }
}

static VisualToolOptionKind visual_tool_option_kind_for_index(const DrawingProgramAppContext *ctx,
                                                              DrawingProgramToolKind tool,
                                                              uint32_t index) {
    (void)ctx;
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_BRUSH:
            if (index == 0u) return VISUAL_TOOL_OPTION_BRUSH_SIZE;
            if (index == 1u) return VISUAL_TOOL_OPTION_BRUSH_OPACITY;
            if (index == 2u) return VISUAL_TOOL_OPTION_BRUSH_SPACING;
            return VISUAL_TOOL_OPTION_BRUSH_HARDNESS;
        case DRAWING_PROGRAM_TOOL_ERASER:
            return VISUAL_TOOL_OPTION_ERASER_SIZE;
        case DRAWING_PROGRAM_TOOL_LINE:
        case DRAWING_PROGRAM_TOOL_RECT:
        case DRAWING_PROGRAM_TOOL_CIRCLE:
            return (index == 0u) ? VISUAL_TOOL_OPTION_SHAPE_STROKE_WIDTH : VISUAL_TOOL_OPTION_SHAPE_MODE;
        case DRAWING_PROGRAM_TOOL_FILL:
            return VISUAL_TOOL_OPTION_FILL_TOLERANCE;
        case DRAWING_PROGRAM_TOOL_SELECT:
            return VISUAL_TOOL_OPTION_SELECT_DELETE;
        default:
            return VISUAL_TOOL_OPTION_BRUSH_SIZE;
    }
}

static const char *visual_tool_option_label(VisualToolOptionKind option) {
    switch (option) {
        case VISUAL_TOOL_OPTION_BRUSH_SIZE: return "SIZE";
        case VISUAL_TOOL_OPTION_BRUSH_OPACITY: return "OPACITY";
        case VISUAL_TOOL_OPTION_BRUSH_SPACING: return "SPACING";
        case VISUAL_TOOL_OPTION_BRUSH_HARDNESS: return "HARDNESS";
        case VISUAL_TOOL_OPTION_ERASER_SIZE: return "SIZE";
        case VISUAL_TOOL_OPTION_SHAPE_STROKE_WIDTH: return "STROKE";
        case VISUAL_TOOL_OPTION_SHAPE_MODE: return "MODE";
        case VISUAL_TOOL_OPTION_FILL_TOLERANCE: return "TOLERANCE";
        case VISUAL_TOOL_OPTION_SELECT_DELETE: return "SELECTION";
        default: return "VALUE";
    }
}

static void visual_tool_option_value_text(const DrawingProgramAppContext *ctx,
                                          VisualToolOptionKind option,
                                          char *out_text,
                                          size_t out_cap) {
    if (!out_text || out_cap == 0u) {
        return;
    }
    switch (option) {
        case VISUAL_TOOL_OPTION_BRUSH_SIZE:
            (void)snprintf(out_text, out_cap, "%u", (unsigned)clamp_setting_u8(ctx ? ctx->ui_tool_brush_size : 2u, 1u, 16u));
            break;
        case VISUAL_TOOL_OPTION_BRUSH_OPACITY:
            (void)snprintf(out_text, out_cap, "%u%%", (unsigned)clamp_setting_u8(ctx ? ctx->ui_tool_brush_opacity : 100u, 1u, 100u));
            break;
        case VISUAL_TOOL_OPTION_BRUSH_SPACING:
            (void)snprintf(out_text, out_cap, "%u", (unsigned)clamp_setting_u8(ctx ? ctx->ui_tool_brush_spacing : 2u, 1u, 16u));
            break;
        case VISUAL_TOOL_OPTION_BRUSH_HARDNESS:
            (void)snprintf(out_text, out_cap, "%u%%", (unsigned)clamp_setting_u8(ctx ? ctx->ui_tool_brush_hardness : 100u, 1u, 100u));
            break;
        case VISUAL_TOOL_OPTION_ERASER_SIZE:
            (void)snprintf(out_text, out_cap, "%u", (unsigned)clamp_setting_u8(ctx ? ctx->ui_tool_eraser_size : 4u, 1u, 16u));
            break;
        case VISUAL_TOOL_OPTION_SHAPE_STROKE_WIDTH:
            (void)snprintf(out_text, out_cap, "%u", (unsigned)clamp_setting_u8(ctx ? ctx->ui_tool_shape_stroke_width : 1u, 1u, 16u));
            break;
        case VISUAL_TOOL_OPTION_SHAPE_MODE:
            (void)snprintf(out_text, out_cap, "%s", shape_mode_name(tool_shape_mode(ctx)));
            break;
        case VISUAL_TOOL_OPTION_FILL_TOLERANCE:
            (void)snprintf(out_text, out_cap, "%u", (unsigned)clamp_setting_u8(ctx ? ctx->ui_tool_fill_tolerance : 0u, 0u, 16u));
            break;
        case VISUAL_TOOL_OPTION_SELECT_DELETE:
            (void)snprintf(out_text, out_cap, "DELETE");
            break;
        default:
            (void)snprintf(out_text, out_cap, "-");
            break;
    }
}

static void visual_tool_option_adjust(DrawingProgramAppContext *ctx, VisualToolOptionKind option, int delta) {
    if (!ctx || delta == 0) {
        return;
    }
    switch (option) {
        case VISUAL_TOOL_OPTION_BRUSH_SIZE: {
            int v = (int)ctx->ui_tool_brush_size + delta;
            if (v < 1) v = 1;
            if (v > 16) v = 16;
            ctx->ui_tool_brush_size = (uint8_t)v;
            break;
        }
        case VISUAL_TOOL_OPTION_BRUSH_OPACITY: {
            int v = (int)ctx->ui_tool_brush_opacity + (delta * 5);
            if (v < 1) v = 1;
            if (v > 100) v = 100;
            ctx->ui_tool_brush_opacity = (uint8_t)v;
            break;
        }
        case VISUAL_TOOL_OPTION_BRUSH_SPACING: {
            int v = (int)ctx->ui_tool_brush_spacing + delta;
            if (v < 1) v = 1;
            if (v > 16) v = 16;
            ctx->ui_tool_brush_spacing = (uint8_t)v;
            break;
        }
        case VISUAL_TOOL_OPTION_BRUSH_HARDNESS: {
            int v = (int)ctx->ui_tool_brush_hardness + (delta * 5);
            if (v < 1) v = 1;
            if (v > 100) v = 100;
            ctx->ui_tool_brush_hardness = (uint8_t)v;
            break;
        }
        case VISUAL_TOOL_OPTION_ERASER_SIZE: {
            int v = (int)ctx->ui_tool_eraser_size + delta;
            if (v < 1) v = 1;
            if (v > 16) v = 16;
            ctx->ui_tool_eraser_size = (uint8_t)v;
            break;
        }
        case VISUAL_TOOL_OPTION_SHAPE_STROKE_WIDTH: {
            int v = (int)ctx->ui_tool_shape_stroke_width + delta;
            if (v < 1) v = 1;
            if (v > 16) v = 16;
            ctx->ui_tool_shape_stroke_width = (uint8_t)v;
            break;
        }
        case VISUAL_TOOL_OPTION_SHAPE_MODE: {
            int v = (int)ctx->ui_tool_shape_mode + delta;
            while (v < 0) {
                v += 3;
            }
            while (v > 2) {
                v -= 3;
            }
            ctx->ui_tool_shape_mode = (uint8_t)v;
            break;
        }
        case VISUAL_TOOL_OPTION_FILL_TOLERANCE: {
            int v = (int)ctx->ui_tool_fill_tolerance + delta;
            if (v < 0) v = 0;
            if (v > 16) v = 16;
            ctx->ui_tool_fill_tolerance = (uint8_t)v;
            break;
        }
        case VISUAL_TOOL_OPTION_SELECT_DELETE:
            break;
        default:
            break;
    }
}

static int visual_tool_option_is_action_button(VisualToolOptionKind option) {
    return (option == VISUAL_TOOL_OPTION_SELECT_DELETE) ? 1 : 0;
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
    uint8_t *composited_samples;
    uint32_t composited_capacity;
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
    free(g_visual_canvas_texture.composited_samples);
    g_visual_canvas_texture.composited_samples = 0;
    g_visual_canvas_texture.composited_capacity = 0u;
}

static void visual_collect_layer_opacity_by_index(const DrawingProgramAppContext *ctx,
                                                  uint8_t *out_opacity,
                                                  uint32_t out_count) {
    uint32_t i;
    if (!ctx || !out_opacity) {
        return;
    }
    for (i = 0u; i < out_count; ++i) {
        uint8_t opacity = 100u;
        if (i < ctx->document.layer_count) {
            opacity = visual_layer_opacity_get(ctx, ctx->document.layers[i].layer_id);
        }
        out_opacity[i] = opacity;
    }
}

static int visual_canvas_texture_sync(SDL_Renderer *renderer, const DrawingProgramAppContext *ctx) {
    void *pixels = 0;
    int pitch = 0;
    uint32_t x;
    uint32_t y;
    const uint8_t *source_samples = 0;
    uint8_t layer_opacity[DRAWING_PROGRAM_MAX_LAYERS];
    CoreResult compose_result;
    if (!renderer || !ctx) {
        return 0;
    }
    if (ctx->document.raster_width == 0u ||
        ctx->document.raster_height == 0u ||
        ctx->document.raster_sample_count == 0u) {
        return 0;
    }
    if (g_visual_canvas_texture.composited_capacity < ctx->document.raster_sample_count) {
        uint8_t *next_samples = (uint8_t *)realloc(g_visual_canvas_texture.composited_samples,
                                                   (size_t)ctx->document.raster_sample_count);
        if (!next_samples) {
            return 0;
        }
        g_visual_canvas_texture.composited_samples = next_samples;
        g_visual_canvas_texture.composited_capacity = ctx->document.raster_sample_count;
    }
    visual_collect_layer_opacity_by_index(ctx, layer_opacity, DRAWING_PROGRAM_MAX_LAYERS);
    compose_result = drawing_program_render_compose_visible_samples_with_layer_opacity(&ctx->document,
                                                                                        &ctx->layer_rasters,
                                                                                        layer_opacity,
                                                                                        DRAWING_PROGRAM_MAX_LAYERS,
                                                                                        g_visual_canvas_texture.composited_samples,
                                                                                        g_visual_canvas_texture.composited_capacity);
    if (compose_result.code == CORE_OK) {
        source_samples = g_visual_canvas_texture.composited_samples;
    } else {
        source_samples = ctx->document.raster_samples;
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
            uint8_t sample = source_samples[row_offset + x];
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
    uint8_t transform_active;
    uint8_t transform_kind;
    uint8_t move_axis_lock;
    uint8_t marquee_commit_mode;
    uint8_t has_last_sample;
    uint32_t last_sample_x;
    uint32_t last_sample_y;
    uint32_t shape_start_sample_x;
    uint32_t shape_start_sample_y;
    int last_mouse_x;
    int last_mouse_y;
} VisualCanvasInteractionState;

typedef DrawingProgramSelectionState VisualSelectionState;

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

typedef enum VisualMarqueeCommitMode {
    VISUAL_MARQUEE_COMMIT_REPLACE = 0,
    VISUAL_MARQUEE_COMMIT_ADD = 1,
    VISUAL_MARQUEE_COMMIT_SUBTRACT = 2
} VisualMarqueeCommitMode;

typedef enum VisualTransformSessionKind {
    VISUAL_TRANSFORM_SESSION_NONE = 0,
    VISUAL_TRANSFORM_SESSION_MOVE = 1
} VisualTransformSessionKind;

static VisualMarqueeCommitMode visual_marquee_commit_mode_from_mods(SDL_Keymod mods) {
    if ((mods & KMOD_ALT) != 0) {
        return VISUAL_MARQUEE_COMMIT_SUBTRACT;
    }
    if ((mods & KMOD_SHIFT) != 0) {
        return VISUAL_MARQUEE_COMMIT_ADD;
    }
    return VISUAL_MARQUEE_COMMIT_REPLACE;
}

static VisualMarqueeCommitMode visual_marquee_commit_mode_clamp(uint8_t raw) {
    if (raw > (uint8_t)VISUAL_MARQUEE_COMMIT_SUBTRACT) {
        return VISUAL_MARQUEE_COMMIT_REPLACE;
    }
    return (VisualMarqueeCommitMode)raw;
}

static int visual_selection_capture_from_marquee(DrawingProgramAppContext *ctx,
                                                 VisualSelectionState *selection,
                                                 VisualMarqueeCommitMode mode) {
    if (!ctx) {
        return 0;
    }
    if (mode == VISUAL_MARQUEE_COMMIT_ADD) {
        return drawing_program_selection_add_from_marquee(
            &ctx->document, &ctx->layer_rasters, ctx->editor.active_layer_id, selection);
    }
    if (mode == VISUAL_MARQUEE_COMMIT_SUBTRACT) {
        return drawing_program_selection_subtract_from_marquee(
            &ctx->document, &ctx->layer_rasters, ctx->editor.active_layer_id, selection);
    }
    return drawing_program_selection_capture_from_marquee(&ctx->document,
                                                          &ctx->layer_rasters,
                                                          ctx->editor.active_layer_id,
                                                          selection);
}

static int visual_selection_begin_move(const VisualSelectionState *selection, uint32_t sample_x, uint32_t sample_y) {
    return drawing_program_selection_begin_move(selection, sample_x, sample_y);
}

static CoreResult visual_selection_commit_move(DrawingProgramAppContext *ctx, VisualSelectionState *selection) {
    if (!ctx) {
        return core_result_ok();
    }
    return drawing_program_selection_commit_move(&ctx->document,
                                                 &ctx->layer_rasters,
                                                 ctx->editor.active_layer_id,
                                                 &ctx->history,
                                                 selection);
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

static int right_canvas_content_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    int y = rect.y + m.pad_y + m.title_glyph_h + m.section_gap;
    y += m.tab_h + m.section_gap;
    return y;
}

static int right_canvas_palette_header_y(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    int y = right_canvas_content_start_y(rect, m);
    y += m.line_h + m.section_gap;
    return y;
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
    int y = rect.y + rect.h - m.pad_y - (4 * m.row_h) - (3 * m.section_gap);
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
}

static SDL_Rect right_canvas_clear_canvas_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect reset = right_canvas_reset_view_button_rect(rect, m);
    return (SDL_Rect){ reset.x, reset.y + reset.h + m.section_gap, reset.w, reset.h };
}

static SDL_Rect right_canvas_delete_selection_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect clear_canvas = right_canvas_clear_canvas_button_rect(rect, m);
    return (SDL_Rect){ clear_canvas.x, clear_canvas.y + clear_canvas.h + m.section_gap, clear_canvas.w, clear_canvas.h };
}

static SDL_Rect right_canvas_clear_history_button_rect(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    SDL_Rect delete_selection = right_canvas_delete_selection_button_rect(rect, m);
    return (SDL_Rect){ delete_selection.x, delete_selection.y + delete_selection.h + m.section_gap, delete_selection.w, delete_selection.h };
}

static int right_canvas_metrics_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    int palette_rows = ((int)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT + 3) / 4;
    int y = right_canvas_palette_header_y(rect, m) + m.line_h;
    y += palette_rows * m.row_h;
    if (palette_rows > 1) {
        y += (palette_rows - 1) * m.section_gap;
    }
    y += m.section_gap;
    return y;
}

typedef enum VisualLayerActionButton {
    VISUAL_LAYER_ACTION_ADD = 0,
    VISUAL_LAYER_ACTION_DUPLICATE,
    VISUAL_LAYER_ACTION_RENAME,
    VISUAL_LAYER_ACTION_DELETE,
    VISUAL_LAYER_ACTION_ACTIVE_PREV,
    VISUAL_LAYER_ACTION_ACTIVE_NEXT,
    VISUAL_LAYER_ACTION_MOVE_UP,
    VISUAL_LAYER_ACTION_MOVE_DOWN,
    VISUAL_LAYER_ACTION_TOGGLE_VISIBLE,
    VISUAL_LAYER_ACTION_TOGGLE_LOCK
} VisualLayerActionButton;

static int right_layer_content_start_y(SDL_Rect rect, VisualPaneLayoutMetrics m) {
    int y = rect.y + m.pad_y + m.title_glyph_h + m.section_gap;
    y += m.tab_h + m.section_gap;
    return y;
}

static SDL_Rect right_layer_row_rect(SDL_Rect rect,
                                     VisualPaneLayoutMetrics m,
                                     uint32_t display_row_index) {
    int y = right_layer_content_start_y(rect, m);
    y += m.line_h;
    y += (int)display_row_index * (m.row_h + m.section_gap);
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
}

static int right_layer_after_rows_y(SDL_Rect rect,
                                    VisualPaneLayoutMetrics m,
                                    uint32_t layer_count) {
    int y = right_layer_content_start_y(rect, m);
    y += m.line_h;
    y += (int)layer_count * m.row_h;
    if (layer_count > 1u) {
        y += (int)(layer_count - 1u) * m.section_gap;
    }
    return y;
}

static SDL_Rect right_layer_opacity_row_rect(SDL_Rect rect,
                                             VisualPaneLayoutMetrics m,
                                             uint32_t layer_count) {
    int y = right_layer_after_rows_y(rect, m, layer_count);
    y += m.section_gap;
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
}

static SDL_Rect right_layer_opacity_track_rect(SDL_Rect opacity_row, VisualPaneLayoutMetrics m) {
    SDL_Rect track = opacity_row;
    int inner_pad = m.tab_gap + 4;
    if (inner_pad < 6) {
        inner_pad = 6;
    }
    track.x += inner_pad;
    track.w -= (inner_pad * 2);
    if (track.w < 24) {
        track.w = 24;
    }
    track.y += (track.h / 2);
    track.h = (m.row_h > 10) ? (m.row_h / 3) : 3;
    return track;
}

static int right_layer_actions_label_y(SDL_Rect rect,
                                       VisualPaneLayoutMetrics m,
                                       uint32_t layer_count) {
    SDL_Rect opacity_row = right_layer_opacity_row_rect(rect, m, layer_count);
    return opacity_row.y + opacity_row.h + m.section_gap;
}

static SDL_Rect right_layer_action_button_rect(SDL_Rect rect,
                                               VisualPaneLayoutMetrics m,
                                               uint32_t layer_count,
                                               VisualLayerActionButton button) {
    int y = right_layer_actions_label_y(rect, m, layer_count);
    y += m.line_h + m.section_gap;
    y += (int)button * (m.row_h + m.section_gap);
    return (SDL_Rect){ rect.x + m.pad_x, y, rect.w - (2 * m.pad_x), m.row_h };
}

static SDL_Rect left_tool_option_row_rect(SDL_Rect rect, VisualPaneLayoutMetrics m, int y) {
    return (SDL_Rect){
        rect.x + m.pad_x + m.tab_gap,
        y,
        rect.w - (2 * m.pad_x) - m.tab_gap,
        m.row_h
    };
}

static SDL_Rect left_tool_option_minus_rect(SDL_Rect row, VisualPaneLayoutMetrics m) {
    int button_w = m.tab_h;
    int value_w = (int)((float)row.w * 0.34f);
    int value_x;
    if (value_w < 74) {
        value_w = 74;
    }
    if (value_w > 126) {
        value_w = 126;
    }
    value_x = row.x + row.w - button_w - m.tab_gap - value_w;
    return (SDL_Rect){ value_x - m.tab_gap - button_w, row.y, button_w, row.h };
}

static SDL_Rect left_tool_option_plus_rect(SDL_Rect row, VisualPaneLayoutMetrics m) {
    int button_w = m.tab_h;
    return (SDL_Rect){ row.x + row.w - button_w, row.y, button_w, row.h };
}

static SDL_Rect left_tool_option_value_rect(SDL_Rect row, VisualPaneLayoutMetrics m) {
    SDL_Rect plus = left_tool_option_plus_rect(row, m);
    int value_w = (int)((float)row.w * 0.34f);
    if (value_w < 74) {
        value_w = 74;
    }
    if (value_w > 126) {
        value_w = 126;
    }
    return (SDL_Rect){ plus.x - m.tab_gap - value_w, row.y, value_w, row.h };
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

static int screen_to_canvas_sample_clamped(const DrawingProgramAppContext *ctx,
                                           SDL_Rect pane_rect,
                                           int sx,
                                           int sy,
                                           uint32_t *out_sample_x,
                                           uint32_t *out_sample_y) {
    VisualCanvasSheetMetrics metrics;
    int local_x;
    int local_y;
    int32_t sample_x;
    int32_t sample_y;
    if (!ctx || !out_sample_x || !out_sample_y || ctx->document.raster_width == 0u || ctx->document.raster_height == 0u) {
        return 0;
    }
    compute_canvas_sheet_metrics(ctx, pane_rect, &metrics);
    if (metrics.pixel_size <= 0.0f) {
        return 0;
    }
    local_x = sx - metrics.sheet_rect.x;
    local_y = sy - metrics.sheet_rect.y;
    sample_x = (int32_t)((float)local_x / metrics.pixel_size);
    sample_y = (int32_t)((float)local_y / metrics.pixel_size);
    if (sample_x < 0) {
        sample_x = 0;
    }
    if (sample_y < 0) {
        sample_y = 0;
    }
    if ((uint32_t)sample_x >= ctx->document.raster_width) {
        sample_x = (int32_t)ctx->document.raster_width - 1;
    }
    if ((uint32_t)sample_y >= ctx->document.raster_height) {
        sample_y = (int32_t)ctx->document.raster_height - 1;
    }
    *out_sample_x = (uint32_t)sample_x;
    *out_sample_y = (uint32_t)sample_y;
    return 1;
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
    static uint8_t region_mask[DRAWING_PROGRAM_MAX_RASTER_SAMPLES];
    const uint8_t *active_layer_samples = 0;
    uint32_t start_x = 0u;
    uint32_t start_y = 0u;
    uint32_t width;
    uint32_t height;
    uint32_t start_index;
    uint32_t active_layer_sample_count = 0u;
    uint32_t active_layer_id = 0u;
    uint32_t head = 0u;
    uint32_t tail = 0u;
    uint32_t fill_region_count = 0u;
    uint8_t target = 0u;
    uint8_t replacement = 0u;
    CoreResult result;
    if (!ctx) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "null app context for fill" };
    }
    if (!active_layer_allows_edits_visual(ctx)) {
        return core_result_ok();
    }
    if (!active_layer_query(ctx, &active_layer_id, 0, 0, 0)) {
        return core_result_ok();
    }
    if (active_layer_id == 0u) {
        return core_result_ok();
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
    result = drawing_program_layer_raster_store_export_layer(&ctx->layer_rasters,
                                                             active_layer_id,
                                                             &active_layer_samples,
                                                             &active_layer_sample_count);
    if (result.code != CORE_OK ||
        !active_layer_samples ||
        active_layer_sample_count != ctx->document.raster_sample_count) {
        active_layer_samples = ctx->document.raster_samples;
    }
    result = active_layer_sample_read_visual(ctx, start_x, start_y, &target);
    if (result.code != CORE_OK) {
        return (CoreResult){ CORE_ERR_NOT_FOUND, "fill start sample read failed" };
    }
    if (target == replacement) {
        return core_result_ok();
    }
    memset(region_mask, 0, ctx->document.raster_sample_count * sizeof(region_mask[0]));
    start_index = (start_y * width) + start_x;
    queue[tail++] = start_index;
    region_mask[start_index] = 1u;
    fill_region_count = 1u;
    while (head < tail) {
        uint32_t idx = queue[head++];
        uint32_t x = idx % width;
        uint32_t y = idx / width;
        uint8_t current = active_layer_samples[idx];
        if (current != target) {
            continue;
        }
        if (x > 0u) {
            uint32_t n = idx - 1u;
            if (!region_mask[n] && active_layer_samples[n] == target) {
                if (tail >= ctx->document.raster_sample_count) {
                    return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "fill queue exhausted" };
                }
                region_mask[n] = 1u;
                queue[tail++] = n;
                fill_region_count += 1u;
            }
        }
        if (x + 1u < width) {
            uint32_t n = idx + 1u;
            if (!region_mask[n] && active_layer_samples[n] == target) {
                if (tail >= ctx->document.raster_sample_count) {
                    return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "fill queue exhausted" };
                }
                region_mask[n] = 1u;
                queue[tail++] = n;
                fill_region_count += 1u;
            }
        }
        if (y > 0u) {
            uint32_t n = idx - width;
            if (!region_mask[n] && active_layer_samples[n] == target) {
                if (tail >= ctx->document.raster_sample_count) {
                    return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "fill queue exhausted" };
                }
                region_mask[n] = 1u;
                queue[tail++] = n;
                fill_region_count += 1u;
            }
        }
        if (y + 1u < height) {
            uint32_t n = idx + width;
            if (!region_mask[n] && active_layer_samples[n] == target) {
                if (tail >= ctx->document.raster_sample_count) {
                    return (CoreResult){ CORE_ERR_OUT_OF_MEMORY, "fill queue exhausted" };
                }
                region_mask[n] = 1u;
                queue[tail++] = n;
                fill_region_count += 1u;
            }
        }
    }
    if (fill_region_count == 0u) {
        return core_result_ok();
    }
    {
        uint32_t y;
        for (y = 0u; y < height; ++y) {
            uint32_t x = 0u;
            uint32_t row_start = y * width;
            while (x < width) {
                uint32_t span_start_x = x;
                uint32_t span_len = 0u;
                while (x < width && !region_mask[row_start + x]) {
                    x += 1u;
                }
                if (x >= width) {
                    break;
                }
                span_start_x = x;
                while (x < width && region_mask[row_start + x]) {
                    span_len += 1u;
                    x += 1u;
                }
                if (span_len > 0u) {
                    CoreResult span_result = drawing_program_history_apply_set_sample_span_value(&ctx->history,
                                                                                                  &ctx->document,
                                                                                                  &ctx->layer_rasters,
                                                                                                  active_layer_id,
                                                                                                  row_start + span_start_x,
                                                                                                  span_len,
                                                                                                  replacement);
                    if (span_result.code != CORE_OK) {
                        return span_result;
                    }
                }
            }
        }
    }
    return core_result_ok();
}

static CoreResult apply_sample_if_changed_on_layer(DrawingProgramAppContext *ctx,
                                                   uint32_t layer_id,
                                                   uint32_t sample_x,
                                                   uint32_t sample_y,
                                                   uint8_t value) {
    return drawing_program_history_apply_set_sample_value(&ctx->history,
                                                          &ctx->document,
                                                          &ctx->layer_rasters,
                                                          layer_id,
                                                          sample_x,
                                                          sample_y,
                                                          value);
}

static uint8_t blend_sample_value_u8(uint8_t dst, uint8_t src, uint8_t opacity_percent) {
    uint32_t alpha = (uint32_t)clamp_percent_u8(opacity_percent);
    return (uint8_t)(((uint32_t)src * alpha + (uint32_t)dst * (100u - alpha) + 50u) / 100u);
}

static CoreResult apply_canvas_stamp_square_on_layer(DrawingProgramAppContext *ctx,
                                                     uint32_t layer_id,
                                                     int32_t sample_x,
                                                     int32_t sample_y,
                                                     uint8_t value,
                                                     uint32_t stroke_width,
                                                     uint8_t hardness_percent) {
    int32_t start_offset;
    int32_t end_offset;
    int32_t y;
    int32_t x;
    float radius_limit;
    float inner_radius;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid stamp request" };
    }
    if (stroke_width < 1u) {
        stroke_width = 1u;
    }
    hardness_percent = clamp_setting_u8(hardness_percent, 1u, 100u);
    start_offset = -((int32_t)stroke_width / 2);
    end_offset = start_offset + (int32_t)stroke_width - 1;
    if (hardness_percent >= 100u) {
        for (y = start_offset; y <= end_offset; ++y) {
            for (x = start_offset; x <= end_offset; ++x) {
                int32_t tx = sample_x + x;
                int32_t ty = sample_y + y;
                CoreResult result;
                if (tx < 0 || ty < 0 ||
                    (uint32_t)tx >= ctx->document.raster_width ||
                    (uint32_t)ty >= ctx->document.raster_height) {
                    continue;
                }
                result = apply_sample_if_changed_on_layer(ctx,
                                                          layer_id,
                                                          (uint32_t)tx,
                                                          (uint32_t)ty,
                                                          value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
        return core_result_ok();
    }
    radius_limit = ((float)stroke_width * 0.5f);
    if (radius_limit < 0.5f) {
        radius_limit = 0.5f;
    }
    inner_radius = radius_limit * ((float)hardness_percent / 100.0f);
    for (y = start_offset; y <= end_offset; ++y) {
        for (x = start_offset; x <= end_offset; ++x) {
            int32_t tx = sample_x + x;
            int32_t ty = sample_y + y;
            float dx = (float)x + 0.5f;
            float dy = (float)y + 0.5f;
            float d = sqrtf((dx * dx) + (dy * dy));
            uint8_t opacity = 100u;
            if (tx < 0 || ty < 0 ||
                (uint32_t)tx >= ctx->document.raster_width ||
                (uint32_t)ty >= ctx->document.raster_height) {
                continue;
            }
            if (d > radius_limit) {
                continue;
            }
            if (hardness_percent < 100u && d > inner_radius && radius_limit > inner_radius) {
                float edge_t = (radius_limit - d) / (radius_limit - inner_radius);
                int edge_alpha = (int)(edge_t * 100.0f);
                if (edge_alpha < 1) {
                    edge_alpha = 1;
                }
                if (edge_alpha > 100) {
                    edge_alpha = 100;
                }
                opacity = (uint8_t)edge_alpha;
            }
            {
                uint8_t out_value = value;
                if (opacity < 100u) {
                    uint8_t current = drawing_program_color_eraser_value();
                    (void)layer_sample_read_visual(ctx, layer_id, (uint32_t)tx, (uint32_t)ty, &current);
                    out_value = blend_sample_value_u8(current, value, opacity);
                }
                {
                    CoreResult result = apply_sample_if_changed_on_layer(ctx,
                                                                         layer_id,
                                                                         (uint32_t)tx,
                                                                         (uint32_t)ty,
                                                                         out_value);
                if (result.code != CORE_OK) {
                    return result;
                }
                }
            }
        }
    }
    return core_result_ok();
}

static CoreResult apply_canvas_line_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                             uint32_t layer_id,
                                                             uint32_t x0,
                                                             uint32_t y0,
                                                             uint32_t x1,
                                                             uint32_t y1,
                                                             uint8_t value,
                                                             uint32_t stroke_width,
                                                             uint8_t hardness_percent) {
    int32_t x = (int32_t)x0;
    int32_t y = (int32_t)y0;
    int32_t tx = (int32_t)x1;
    int32_t ty = (int32_t)y1;
    int32_t dx = (tx > x) ? (tx - x) : (x - tx);
    int32_t sx = (x < tx) ? 1 : -1;
    int32_t dy = -((ty > y) ? (ty - y) : (y - ty));
    int32_t sy = (y < ty) ? 1 : -1;
    int32_t err = dx + dy;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid line request" };
    }
    if (stroke_width < 1u) {
        stroke_width = 1u;
    }
    while (1) {
        if (x >= 0 && y >= 0 &&
            (uint32_t)x < ctx->document.raster_width &&
            (uint32_t)y < ctx->document.raster_height) {
            CoreResult stamp_result = apply_canvas_stamp_square_on_layer(ctx,
                                                                         layer_id,
                                                                         x,
                                                                         y,
                                                                         value,
                                                                         stroke_width,
                                                                         hardness_percent);
            if (stamp_result.code != CORE_OK) {
                return stamp_result;
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

static CoreResult apply_canvas_rect_fill_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                  uint32_t layer_id,
                                                                  uint32_t x0,
                                                                  uint32_t y0,
                                                                  uint32_t x1,
                                                                  uint32_t y1,
                                                                  uint8_t value) {
    uint32_t min_x = (x0 < x1) ? x0 : x1;
    uint32_t min_y = (y0 < y1) ? y0 : y1;
    uint32_t max_x = (x0 > x1) ? x0 : x1;
    uint32_t max_y = (y0 > y1) ? y0 : y1;
    uint32_t y;
    uint32_t x;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid rect-fill request" };
    }
    for (y = min_y; y <= max_y; ++y) {
        for (x = min_x; x <= max_x; ++x) {
            CoreResult result = apply_sample_if_changed_on_layer(ctx, layer_id, x, y, value);
            if (result.code != CORE_OK) {
                return result;
            }
        }
    }
    return core_result_ok();
}

static CoreResult apply_canvas_rect_outline_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                     uint32_t layer_id,
                                                                     uint32_t x0,
                                                                     uint32_t y0,
                                                                     uint32_t x1,
                                                                     uint32_t y1,
                                                                     uint8_t value,
                                                                     uint32_t stroke_width) {
    uint32_t min_x = (x0 < x1) ? x0 : x1;
    uint32_t min_y = (y0 < y1) ? y0 : y1;
    uint32_t max_x = (x0 > x1) ? x0 : x1;
    uint32_t max_y = (y0 > y1) ? y0 : y1;
    uint32_t y;
    uint32_t x;
    uint32_t rect_width;
    uint32_t rect_height;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid rect-outline request" };
    }
    if (stroke_width < 1u) {
        stroke_width = 1u;
    }
    rect_width = (max_x - min_x) + 1u;
    rect_height = (max_y - min_y) + 1u;
    if (stroke_width >= rect_width || stroke_width >= rect_height) {
        return apply_canvas_rect_fill_between_samples_on_layer(ctx, layer_id, min_x, min_y, max_x, max_y, value);
    }
    for (y = min_y; y <= max_y; ++y) {
        for (x = min_x; x <= max_x; ++x) {
            uint32_t left_dist = x - min_x;
            uint32_t right_dist = max_x - x;
            uint32_t top_dist = y - min_y;
            uint32_t bottom_dist = max_y - y;
            if (left_dist < stroke_width ||
                right_dist < stroke_width ||
                top_dist < stroke_width ||
                bottom_dist < stroke_width) {
                CoreResult result = apply_sample_if_changed_on_layer(ctx, layer_id, x, y, value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
    }
    return core_result_ok();
}

static CoreResult apply_canvas_circle_fill_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                    uint32_t layer_id,
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
    int32_t min_x;
    int32_t max_x;
    int32_t min_y;
    int32_t max_y;
    int32_t y;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid circle-fill request" };
    }
    if (ay > r) {
        r = ay;
    }
    if (r <= 0) {
        return apply_sample_if_changed_on_layer(ctx, layer_id, center_x, center_y, value);
    }
    min_x = cx - r;
    max_x = cx + r;
    min_y = cy - r;
    max_y = cy + r;
    for (y = min_y; y <= max_y; ++y) {
        int32_t x;
        if (y < 0 || (uint32_t)y >= ctx->document.raster_height) {
            continue;
        }
        for (x = min_x; x <= max_x; ++x) {
            int32_t dx;
            int32_t dy;
            if (x < 0 || (uint32_t)x >= ctx->document.raster_width) {
                continue;
            }
            dx = x - cx;
            dy = y - cy;
            if ((dx * dx) + (dy * dy) <= (r * r)) {
                CoreResult result = apply_sample_if_changed_on_layer(ctx, layer_id, (uint32_t)x, (uint32_t)y, value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
    }
    return core_result_ok();
}

static CoreResult apply_canvas_circle_outline_between_samples_on_layer(DrawingProgramAppContext *ctx,
                                                                       uint32_t layer_id,
                                                                       uint32_t center_x,
                                                                       uint32_t center_y,
                                                                       uint32_t edge_x,
                                                                       uint32_t edge_y,
                                                                       uint8_t value,
                                                                       uint32_t stroke_width) {
    int32_t cx = (int32_t)center_x;
    int32_t cy = (int32_t)center_y;
    int32_t rx = (int32_t)edge_x - cx;
    int32_t ry = (int32_t)edge_y - cy;
    int32_t outer_r = (rx < 0 ? -rx : rx);
    int32_t ay = (ry < 0 ? -ry : ry);
    int32_t inner_r;
    int32_t min_x;
    int32_t max_x;
    int32_t min_y;
    int32_t max_y;
    int32_t y;
    int32_t outer_r2;
    int32_t inner_r2;
    if (!ctx || layer_id == 0u) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid circle-outline request" };
    }
    if (stroke_width < 1u) {
        stroke_width = 1u;
    }
    if (ay > outer_r) {
        outer_r = ay;
    }
    if (outer_r <= 0) {
        return apply_canvas_stamp_square_on_layer(ctx, layer_id, cx, cy, value, stroke_width, 100u);
    }
    inner_r = outer_r - (int32_t)stroke_width + 1;
    if (inner_r < 0) {
        inner_r = 0;
    }
    outer_r2 = outer_r * outer_r;
    inner_r2 = inner_r * inner_r;
    min_x = cx - outer_r;
    max_x = cx + outer_r;
    min_y = cy - outer_r;
    max_y = cy + outer_r;
    for (y = min_y; y <= max_y; ++y) {
        int32_t x;
        if (y < 0 || (uint32_t)y >= ctx->document.raster_height) {
            continue;
        }
        for (x = min_x; x <= max_x; ++x) {
            int32_t dx;
            int32_t dy;
            int32_t d2;
            if (x < 0 || (uint32_t)x >= ctx->document.raster_width) {
                continue;
            }
            dx = x - cx;
            dy = y - cy;
            d2 = (dx * dx) + (dy * dy);
            if (d2 <= outer_r2 && d2 >= inner_r2) {
                CoreResult result = apply_sample_if_changed_on_layer(ctx, layer_id, (uint32_t)x, (uint32_t)y, value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
        }
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
    uint8_t mode;
    uint32_t stroke_width;
    uint32_t active_layer_id = 0u;
    if (!ctx) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "null app context for shape commit" };
    }
    if (!active_layer_allows_edits_visual(ctx)) {
        return core_result_ok();
    }
    if (!active_layer_query(ctx, &active_layer_id, 0, 0, 0) || active_layer_id == 0u) {
        return core_result_ok();
    }
    value = sample_value_for_tool(ctx, tool);
    mode = tool_shape_mode(ctx);
    stroke_width = tool_shape_stroke_width(ctx);
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_LINE:
            return apply_canvas_line_between_samples_on_layer(ctx,
                                                              active_layer_id,
                                                              start_x,
                                                              start_y,
                                                              end_x,
                                                              end_y,
                                                              value,
                                                              stroke_width,
                                                              100u);
        case DRAWING_PROGRAM_TOOL_RECT: {
            CoreResult result = core_result_ok();
            if (shape_mode_includes_fill(tool, mode)) {
                result = apply_canvas_rect_fill_between_samples_on_layer(ctx,
                                                                         active_layer_id,
                                                                         start_x,
                                                                         start_y,
                                                                         end_x,
                                                                         end_y,
                                                                         value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
            if (shape_mode_includes_outline(tool, mode)) {
                result = apply_canvas_rect_outline_between_samples_on_layer(ctx,
                                                                            active_layer_id,
                                                                            start_x,
                                                                            start_y,
                                                                            end_x,
                                                                            end_y,
                                                                            value,
                                                                            stroke_width);
            }
            return result;
        }
        case DRAWING_PROGRAM_TOOL_CIRCLE: {
            CoreResult result = core_result_ok();
            if (shape_mode_includes_fill(tool, mode)) {
                result = apply_canvas_circle_fill_between_samples_on_layer(ctx,
                                                                           active_layer_id,
                                                                           start_x,
                                                                           start_y,
                                                                           end_x,
                                                                           end_y,
                                                                           value);
                if (result.code != CORE_OK) {
                    return result;
                }
            }
            if (shape_mode_includes_outline(tool, mode)) {
                result = apply_canvas_circle_outline_between_samples_on_layer(ctx,
                                                                              active_layer_id,
                                                                              start_x,
                                                                              start_y,
                                                                              end_x,
                                                                              end_y,
                                                                              value,
                                                                              stroke_width);
            }
            return result;
        }
        default:
            return core_result_ok();
    }
}

CoreResult drawing_program_app_shape_commit_samples(DrawingProgramAppContext *ctx,
                                                    DrawingProgramToolKind tool,
                                                    uint32_t start_x,
                                                    uint32_t start_y,
                                                    uint32_t end_x,
                                                    uint32_t end_y) {
    return apply_canvas_shape_commit(ctx, tool, start_x, start_y, end_x, end_y);
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

#define VISUAL_SELECTION_HANDLE_COUNT 8
#define VISUAL_SELECTION_MAX_COMPONENT_BOUNDS 128u

typedef struct VisualSelectionComponentBounds {
    uint32_t min_x;
    uint32_t min_y;
    uint32_t max_x;
    uint32_t max_y;
} VisualSelectionComponentBounds;

static int selection_handle_size_for_metrics(const VisualCanvasSheetMetrics *metrics) {
    int handle_size = 8;
    if (metrics) {
        handle_size = (int)(metrics->pixel_size * 1.75f);
    }
    if (handle_size < 6) {
        handle_size = 6;
    }
    if (handle_size > 16) {
        handle_size = 16;
    }
    return handle_size;
}

static void selection_populate_handle_rects(const SDL_Rect *selection_rect,
                                            int handle_size,
                                            SDL_Rect out_handles[VISUAL_SELECTION_HANDLE_COUNT]) {
    int x;
    int y;
    int w;
    int h;
    int cx;
    int cy;
    if (!selection_rect || !out_handles) {
        return;
    }
    x = selection_rect->x;
    y = selection_rect->y;
    w = selection_rect->w;
    h = selection_rect->h;
    cx = x + (w / 2);
    cy = y + (h / 2);
    out_handles[0] = (SDL_Rect){ x - (handle_size / 2), y - (handle_size / 2), handle_size, handle_size };
    out_handles[1] = (SDL_Rect){ cx - (handle_size / 2), y - (handle_size / 2), handle_size, handle_size };
    out_handles[2] = (SDL_Rect){ x + w - (handle_size / 2), y - (handle_size / 2), handle_size, handle_size };
    out_handles[3] = (SDL_Rect){ x + w - (handle_size / 2), cy - (handle_size / 2), handle_size, handle_size };
    out_handles[4] = (SDL_Rect){ x + w - (handle_size / 2), y + h - (handle_size / 2), handle_size, handle_size };
    out_handles[5] = (SDL_Rect){ cx - (handle_size / 2), y + h - (handle_size / 2), handle_size, handle_size };
    out_handles[6] = (SDL_Rect){ x - (handle_size / 2), y + h - (handle_size / 2), handle_size, handle_size };
    out_handles[7] = (SDL_Rect){ x - (handle_size / 2), cy - (handle_size / 2), handle_size, handle_size };
}

static int selection_current_screen_rect(const DrawingProgramAppContext *ctx,
                                         SDL_Rect pane_rect,
                                         const VisualSelectionState *selection,
                                         SDL_Rect *out_rect,
                                         VisualCanvasSheetMetrics *out_metrics) {
    int32_t moved_x;
    int32_t moved_y;
    VisualCanvasSheetMetrics metrics;
    if (!ctx || !selection || !out_rect || !selection->has_payload || selection->width == 0u || selection->height == 0u) {
        return 0;
    }
    compute_canvas_sheet_metrics(ctx, pane_rect, &metrics);
    moved_x = (int32_t)selection->origin_x + selection->offset_x;
    moved_y = (int32_t)selection->origin_y + selection->offset_y;
    if (!selection_sample_rect_to_screen_rect(&metrics, moved_x, moved_y, selection->width, selection->height, out_rect)) {
        return 0;
    }
    if (out_metrics) {
        *out_metrics = metrics;
    }
    return 1;
}

static int selection_move_handle_hit(const DrawingProgramAppContext *ctx,
                                     SDL_Rect pane_rect,
                                     const VisualSelectionState *selection,
                                     int x,
                                     int y) {
    SDL_Rect selection_rect;
    VisualCanvasSheetMetrics metrics;
    SDL_Rect handles[VISUAL_SELECTION_HANDLE_COUNT];
    int handle_size;
    int i;
    if (!selection_current_screen_rect(ctx, pane_rect, selection, &selection_rect, &metrics)) {
        return 0;
    }
    handle_size = selection_handle_size_for_metrics(&metrics);
    selection_populate_handle_rects(&selection_rect, handle_size, handles);
    for (i = 0; i < VISUAL_SELECTION_HANDLE_COUNT; ++i) {
        if (point_in_rect(handles[i], x, y)) {
            return 1;
        }
    }
    return 0;
}

static void draw_selection_payload_preview(SDL_Renderer *renderer,
                                           const DrawingProgramAppContext *ctx,
                                           const VisualCanvasSheetMetrics *metrics,
                                           const VisualSelectionState *selection,
                                           int32_t origin_x,
                                           int32_t origin_y,
                                           uint8_t alpha) {
    uint32_t x;
    uint32_t y;
    if (!renderer || !ctx || !metrics || !selection || !selection->has_payload) {
        return;
    }
    if (selection->width == 0u || selection->height == 0u) {
        return;
    }
    for (y = 0u; y < selection->height; ++y) {
        for (x = 0u; x < selection->width; ++x) {
            int32_t sample_x;
            int32_t sample_y;
            SDL_Rect sample_rect;
            uint8_t value;
            uint8_t r = 0u;
            uint8_t g = 0u;
            uint8_t b = 0u;
            if (!drawing_program_selection_mask_at(selection, x, y)) {
                continue;
            }
            sample_x = origin_x + (int32_t)x;
            sample_y = origin_y + (int32_t)y;
            if (sample_x < 0 || sample_y < 0 ||
                sample_x >= (int32_t)ctx->document.raster_width ||
                sample_y >= (int32_t)ctx->document.raster_height) {
                continue;
            }
            if (!selection_sample_rect_to_screen_rect(metrics, sample_x, sample_y, 1u, 1u, &sample_rect)) {
                continue;
            }
            value = drawing_program_selection_value_at(selection, x, y);
            drawing_program_color_rgb_from_sample(value, &r, &g, &b);
            SDL_SetRenderDrawColor(renderer, r, g, b, alpha);
            (void)SDL_RenderFillRect(renderer, &sample_rect);
        }
    }
}

static uint32_t collect_selection_component_bounds(const VisualSelectionState *selection,
                                                   VisualSelectionComponentBounds *out_bounds,
                                                   uint32_t max_bounds) {
    uint32_t width;
    uint32_t height;
    uint32_t area;
    uint8_t *visited = 0;
    uint32_t *queue = 0;
    uint32_t idx;
    uint32_t component_count = 0u;
    if (!selection || !selection->has_payload || !out_bounds || max_bounds == 0u) {
        return 0u;
    }
    width = selection->width;
    height = selection->height;
    if (width == 0u || height == 0u) {
        return 0u;
    }
    area = width * height;
    if (area == 0u || area > DRAWING_PROGRAM_SELECTION_MAX_AREA) {
        return 0u;
    }
    visited = (uint8_t *)calloc(area, sizeof(uint8_t));
    queue = (uint32_t *)malloc(area * sizeof(uint32_t));
    if (!visited || !queue) {
        free(visited);
        free(queue);
        return 0u;
    }
    for (idx = 0u; idx < area; ++idx) {
        uint32_t head;
        uint32_t tail;
        uint32_t seed_x;
        uint32_t seed_y;
        uint32_t min_x;
        uint32_t min_y;
        uint32_t max_x;
        uint32_t max_y;
        if (!selection->payload_mask[idx] || visited[idx]) {
            continue;
        }
        seed_x = idx % width;
        seed_y = idx / width;
        min_x = seed_x;
        min_y = seed_y;
        max_x = seed_x;
        max_y = seed_y;
        head = 0u;
        tail = 0u;
        queue[tail++] = idx;
        visited[idx] = 1u;
        while (head < tail) {
            uint32_t current = queue[head++];
            uint32_t x = current % width;
            uint32_t y = current / width;
            if (x > 0u) {
                uint32_t neighbor = current - 1u;
                if (selection->payload_mask[neighbor] && !visited[neighbor]) {
                    visited[neighbor] = 1u;
                    queue[tail++] = neighbor;
                    if (x - 1u < min_x) {
                        min_x = x - 1u;
                    }
                }
            }
            if ((x + 1u) < width) {
                uint32_t neighbor = current + 1u;
                if (selection->payload_mask[neighbor] && !visited[neighbor]) {
                    visited[neighbor] = 1u;
                    queue[tail++] = neighbor;
                    if (x + 1u > max_x) {
                        max_x = x + 1u;
                    }
                }
            }
            if (y > 0u) {
                uint32_t neighbor = current - width;
                if (selection->payload_mask[neighbor] && !visited[neighbor]) {
                    visited[neighbor] = 1u;
                    queue[tail++] = neighbor;
                    if (y - 1u < min_y) {
                        min_y = y - 1u;
                    }
                }
            }
            if ((y + 1u) < height) {
                uint32_t neighbor = current + width;
                if (selection->payload_mask[neighbor] && !visited[neighbor]) {
                    visited[neighbor] = 1u;
                    queue[tail++] = neighbor;
                    if (y + 1u > max_y) {
                        max_y = y + 1u;
                    }
                }
            }
        }
        if (component_count < max_bounds) {
            out_bounds[component_count].min_x = min_x;
            out_bounds[component_count].min_y = min_y;
            out_bounds[component_count].max_x = max_x;
            out_bounds[component_count].max_y = max_y;
            component_count += 1u;
        }
    }
    free(visited);
    free(queue);
    return component_count;
}

static uint32_t draw_selection_component_outlines(SDL_Renderer *renderer,
                                                  const VisualCanvasSheetMetrics *metrics,
                                                  const VisualSelectionState *selection,
                                                  int32_t delta_x,
                                                  int32_t delta_y,
                                                  SDL_Color color,
                                                  uint8_t alpha) {
    VisualSelectionComponentBounds components[VISUAL_SELECTION_MAX_COMPONENT_BOUNDS];
    uint32_t component_count;
    uint32_t i;
    if (!renderer || !metrics || !selection || !selection->has_payload) {
        return 0u;
    }
    component_count = collect_selection_component_bounds(selection,
                                                         components,
                                                         VISUAL_SELECTION_MAX_COMPONENT_BOUNDS);
    if (component_count == 0u) {
        return 0u;
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
    for (i = 0u; i < component_count; ++i) {
        int32_t sample_x = (int32_t)selection->origin_x + (int32_t)components[i].min_x + delta_x;
        int32_t sample_y = (int32_t)selection->origin_y + (int32_t)components[i].min_y + delta_y;
        uint32_t region_w = components[i].max_x - components[i].min_x + 1u;
        uint32_t region_h = components[i].max_y - components[i].min_y + 1u;
        SDL_Rect rect;
        if (selection_sample_rect_to_screen_rect(metrics, sample_x, sample_y, region_w, region_h, &rect)) {
            (void)SDL_RenderDrawRect(renderer, &rect);
        }
    }
    return component_count;
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
            SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 240u);
            (void)SDL_RenderDrawRect(renderer, &rect);
            if (rect.w > 2 && rect.h > 2) {
                SDL_Rect inner = { rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2 };
                SDL_SetRenderDrawColor(renderer, accent_alt.r, accent_alt.g, accent_alt.b, 220u);
                (void)SDL_RenderDrawRect(renderer, &inner);
            }
        }
    }
    if (selection->has_payload && selection->width > 0u && selection->height > 0u) {
        int32_t base_x = (int32_t)selection->origin_x;
        int32_t base_y = (int32_t)selection->origin_y;
        int32_t moved_x = base_x + selection->offset_x;
        int32_t moved_y = base_y + selection->offset_y;
        uint32_t moved_component_count = 0u;
        if (selection->moving &&
            (selection->offset_x != 0 || selection->offset_y != 0)) {
            uint32_t base_component_count =
                draw_selection_component_outlines(renderer, metrics, selection, 0, 0, accent_alt, 170u);
            if (base_component_count == 0u &&
                selection_sample_rect_to_screen_rect(metrics,
                                                     base_x,
                                                     base_y,
                                                     selection->width,
                                                     selection->height,
                                                     &rect)) {
                SDL_SetRenderDrawColor(renderer, accent_alt.r, accent_alt.g, accent_alt.b, 170u);
                (void)SDL_RenderDrawRect(renderer, &rect);
            }
        }
        moved_component_count =
            draw_selection_component_outlines(renderer,
                                              metrics,
                                              selection,
                                              selection->offset_x,
                                              selection->offset_y,
                                              accent,
                                              255u);
        if (moved_component_count == 0u &&
            selection_sample_rect_to_screen_rect(metrics,
                                                 moved_x,
                                                 moved_y,
                                                 selection->width,
                                                 selection->height,
                                                 &rect)) {
            SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 255u);
            (void)SDL_RenderDrawRect(renderer, &rect);
        }
        if (selection_sample_rect_to_screen_rect(metrics,
                                                 moved_x,
                                                 moved_y,
                                                 selection->width,
                                                 selection->height,
                                                 &rect)) {
            SDL_Rect handles[VISUAL_SELECTION_HANDLE_COUNT];
            int handle_size = selection_handle_size_for_metrics(metrics);
            int i;
            if (selection->moving && (selection->offset_x != 0 || selection->offset_y != 0)) {
                draw_selection_payload_preview(renderer, ctx, metrics, selection, moved_x, moved_y, 255u);
            }
            if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE || selection->moving) {
                selection_populate_handle_rects(&rect, handle_size, handles);
                for (i = 0; i < VISUAL_SELECTION_HANDLE_COUNT; ++i) {
                    SDL_SetRenderDrawColor(renderer, accent_alt.r, accent_alt.g, accent_alt.b, 255u);
                    (void)SDL_RenderFillRect(renderer, &handles[i]);
                    SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 255u);
                    (void)SDL_RenderDrawRect(renderer, &handles[i]);
                }
            }
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

static int preview_stroke_width_pixels(const VisualCanvasSheetMetrics *metrics, uint32_t stroke_width_samples) {
    int stroke_px;
    if (stroke_width_samples < 1u) {
        stroke_width_samples = 1u;
    }
    if (!metrics || metrics->pixel_size <= 0.0f) {
        return (int)stroke_width_samples;
    }
    stroke_px = (int)lroundf((float)stroke_width_samples * metrics->pixel_size);
    if (stroke_px < 1) {
        stroke_px = 1;
    }
    if (stroke_px > 128) {
        stroke_px = 128;
    }
    return stroke_px;
}

static void draw_preview_thick_line(SDL_Renderer *renderer,
                                    int x0,
                                    int y0,
                                    int x1,
                                    int y1,
                                    int stroke_px,
                                    SDL_Color color,
                                    uint8_t alpha) {
    float dx;
    float dy;
    float len;
    float nx;
    float ny;
    int i;
    int start;
    if (!renderer) {
        return;
    }
    if (stroke_px < 1) {
        stroke_px = 1;
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
    if (stroke_px == 1) {
        (void)SDL_RenderDrawLine(renderer, x0, y0, x1, y1);
        return;
    }
    dx = (float)(x1 - x0);
    dy = (float)(y1 - y0);
    len = sqrtf((dx * dx) + (dy * dy));
    if (len <= 0.0001f) {
        SDL_Rect dot = { x0 - (stroke_px / 2), y0 - (stroke_px / 2), stroke_px, stroke_px };
        (void)SDL_RenderFillRect(renderer, &dot);
        return;
    }
    nx = -dy / len;
    ny = dx / len;
    start = -(stroke_px / 2);
    for (i = 0; i < stroke_px; ++i) {
        float off = (float)(start + i);
        int ox = (int)lroundf(nx * off);
        int oy = (int)lroundf(ny * off);
        (void)SDL_RenderDrawLine(renderer, x0 + ox, y0 + oy, x1 + ox, y1 + oy);
    }
}

static void draw_preview_thick_rect_outline(SDL_Renderer *renderer,
                                            SDL_Rect rect,
                                            int stroke_px,
                                            SDL_Color color,
                                            uint8_t alpha) {
    int i;
    if (!renderer) {
        return;
    }
    if (stroke_px < 1) {
        stroke_px = 1;
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
    for (i = 0; i < stroke_px; ++i) {
        SDL_Rect ring = { rect.x + i, rect.y + i, rect.w - (2 * i), rect.h - (2 * i) };
        if (ring.w <= 0 || ring.h <= 0) {
            break;
        }
        (void)SDL_RenderDrawRect(renderer, &ring);
    }
}

static void draw_preview_circle_fill(SDL_Renderer *renderer,
                                     int center_x,
                                     int center_y,
                                     int radius_px,
                                     SDL_Color color,
                                     uint8_t alpha) {
    int y;
    if (!renderer) {
        return;
    }
    if (radius_px < 1) {
        radius_px = 1;
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
    for (y = -radius_px; y <= radius_px; ++y) {
        float row = (float)((radius_px * radius_px) - (y * y));
        int span = (row > 0.0f) ? (int)floorf(sqrtf(row)) : 0;
        (void)SDL_RenderDrawLine(renderer, center_x - span, center_y + y, center_x + span, center_y + y);
    }
}

static void draw_preview_circle_outline(SDL_Renderer *renderer,
                                        int center_x,
                                        int center_y,
                                        int radius_px,
                                        int stroke_px,
                                        SDL_Color color,
                                        uint8_t alpha) {
    int s;
    const int segments = 64;
    const float two_pi = 6.28318530718f;
    if (!renderer) {
        return;
    }
    if (stroke_px < 1) {
        stroke_px = 1;
    }
    if (radius_px < 1) {
        radius_px = 1;
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
    for (s = 0; s < stroke_px; ++s) {
        float radius = (float)radius_px - (float)(stroke_px / 2) + (float)s;
        int i;
        if (radius < 1.0f) {
            continue;
        }
        for (i = 0; i < segments; ++i) {
            float t0 = ((float)i / (float)segments) * two_pi;
            float t1 = ((float)(i + 1) / (float)segments) * two_pi;
            int x0 = center_x + (int)(cosf(t0) * radius);
            int y0 = center_y + (int)(sinf(t0) * radius);
            int x1 = center_x + (int)(cosf(t1) * radius);
            int y1 = center_y + (int)(sinf(t1) * radius);
            (void)SDL_RenderDrawLine(renderer, x0, y0, x1, y1);
        }
    }
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
            int stroke_px = preview_stroke_width_pixels(metrics, tool_shape_stroke_width(ctx));
            if (sample_center_to_screen(metrics,
                                        ctx,
                                        interaction->shape_start_sample_x,
                                        interaction->shape_start_sample_y,
                                        &x0,
                                        &y0) &&
                sample_center_to_screen(metrics, ctx, end_x, end_y, &x1, &y1)) {
                draw_preview_thick_line(renderer, x0, y0, x1, y1, stroke_px, accent, 230u);
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
            uint8_t mode = tool_shape_mode(ctx);
            int stroke_px = preview_stroke_width_pixels(metrics, tool_shape_stroke_width(ctx));
            if (selection_sample_rect_to_screen_rect(metrics, (int32_t)min_x, (int32_t)min_y, w, h, &rect)) {
                if (shape_mode_includes_fill(tool, mode)) {
                    SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, 36u);
                    (void)SDL_RenderFillRect(renderer, &rect);
                }
                if (shape_mode_includes_outline(tool, mode)) {
                    draw_preview_thick_rect_outline(renderer, rect, stroke_px, accent, 230u);
                }
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
            int radius_px;
            int stroke_px = preview_stroke_width_pixels(metrics, tool_shape_stroke_width(ctx));
            uint8_t mode = tool_shape_mode(ctx);
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
            radius_px = (int)lroundf((((float)r_samples) + 0.5f) * metrics->pixel_size);
            if (radius_px < 1) {
                radius_px = 1;
            }
            if (shape_mode_includes_fill(tool, mode)) {
                draw_preview_circle_fill(renderer, cx, cy, radius_px, accent, 36u);
            }
            if (shape_mode_includes_outline(tool, mode)) {
                draw_preview_circle_outline(renderer, cx, cy, radius_px, stroke_px, accent, 230u);
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
    uint32_t active_layer_id = 0u;
    uint32_t radius;
    uint32_t spacing;
    uint32_t stamp_width;
    uint8_t hardness;
    uint8_t value;
    CoreResult result;
    if (!ctx || !state) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "invalid canvas draw request" };
    }
    if (!screen_to_canvas_sample(ctx, pane_rect, sx, sy, &sample_x, &sample_y)) {
        return core_result_ok();
    }
    if (!active_layer_allows_edits_visual(ctx)) {
        return core_result_ok();
    }
    if (!active_layer_query(ctx, &active_layer_id, 0, 0, 0) || active_layer_id == 0u) {
        return core_result_ok();
    }

    value = sample_value_for_tool(ctx, ctx->editor.active_tool);
    radius = tool_brush_radius_samples(ctx, ctx->editor.active_tool);
    spacing = tool_brush_spacing_samples(ctx, ctx->editor.active_tool, radius);
    hardness = tool_brush_hardness_percent(ctx, ctx->editor.active_tool);
    stamp_width = (radius * 2u) + 1u;

    if (!state->has_last_sample) {
        uint8_t write_value = value;
        if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_ERASER) {
            write_value = seeded_background_sample_for_coord(&ctx->document, sample_x, sample_y);
            hardness = 100u;
        }
        result = apply_canvas_stamp_square_on_layer(ctx,
                                                    active_layer_id,
                                                    (int32_t)sample_x,
                                                    (int32_t)sample_y,
                                                    write_value,
                                                    stamp_width,
                                                    hardness);
        if (result.code != CORE_OK) {
            return result;
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
            uint8_t write_value = value;
            if (((uint32_t)i % stamp_every) != 0u && i != steps) {
                continue;
            }
            if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_ERASER) {
                write_value = seeded_background_sample_for_coord(&ctx->document, (uint32_t)ix, (uint32_t)iy);
                hardness = 100u;
            }
            result = apply_canvas_stamp_square_on_layer(ctx,
                                                        active_layer_id,
                                                        ix,
                                                        iy,
                                                        write_value,
                                                        stamp_width,
                                                        hardness);
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

static int measure_bitmap_text_width(const char *text, int scale) {
    TTF_Font *font = 0;
    int width = 0;
    int height = 0;
    size_t i;
    if (!text || scale < 1) {
        return 0;
    }
    if (g_visual_draw_ctx) {
        font = visual_get_ttf_font(g_visual_draw_ctx, scale);
    }
    if (font && TTF_SizeUTF8(font, text, &width, &height) == 0 && width > 0) {
        return width;
    }
    for (i = 0; text[i] != '\0'; ++i) {
        width += (5 * scale) + scale;
    }
    return width;
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
    const char *menu_label = "MENU  FILE  EDIT  VIEW  HELP";
    const char *tool_label = "ACTIVE TOOL:";
    const char *tool_value = 0;
    int chip_w;
    int chip_x;
    int chip_right;
    int min_chip_x;
    int min_chip_w;
    int available_w;
    int desired_w;
    int menu_w;
    int label_w;
    int tool_w;
    int header_y;
    int tool_x = 0;
    if (!renderer || !ctx) {
        return;
    }
    resolve_visual_theme_palette(theme, &p);
    m = make_pane_layout_metrics(ctx);
    header_y = rect.y + m.pad_y;
    tool_value = tool_name(ctx->editor.active_tool);
    draw_bitmap_text(renderer, rect, rect.x + m.pad_x, header_y, menu_label, p.text_primary, m.title_scale);

    menu_w = measure_bitmap_text_width(menu_label, m.title_scale);
    label_w = measure_bitmap_text_width(tool_label, m.body_scale);
    tool_w = measure_bitmap_text_width(tool_value, m.body_scale);
    desired_w = 6 + label_w + 6 + tool_w + 6;
    min_chip_w = 120;
    if (desired_w < min_chip_w) {
        desired_w = min_chip_w;
    }

    min_chip_x = rect.x + m.pad_x + menu_w + 12;
    chip_right = rect.x + rect.w - m.pad_x;
    available_w = chip_right - min_chip_x;
    if (available_w < min_chip_w) {
        available_w = min_chip_w;
        min_chip_x = chip_right - available_w;
    }
    chip_w = desired_w;
    if (chip_w > available_w) {
        chip_w = available_w;
    }
    chip_x = chip_right - chip_w;
    if (chip_x < min_chip_x) {
        chip_x = min_chip_x;
    }

    chip.x = chip_x;
    chip.y = header_y - 1;
    chip.w = chip_w;
    chip.h = m.tab_h;
    SDL_SetRenderDrawColor(renderer, p.button_fill.r, p.button_fill.g, p.button_fill.b, p.button_fill.a);
    (void)SDL_RenderFillRect(renderer, &chip);
    SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
    (void)SDL_RenderDrawRect(renderer, &chip);
    label_w = draw_bitmap_text(renderer, chip, chip.x + 6, chip.y + m.tab_text_y, tool_label, p.text_muted, m.body_scale);
    tool_x = chip.x + 6 + label_w + 6;
    draw_bitmap_text(renderer, chip, tool_x, chip.y + m.tab_text_y, tool_value, p.text_primary, m.body_scale);
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
        int y_cursor = y;
        uint32_t active_option_count = visual_tool_option_count(ctx, ctx->editor.active_tool);
        for (i = 0u; i < sizeof(k_visual_tools) / sizeof(k_visual_tools[0]); ++i) {
            SDL_Rect row = { rect.x + m.pad_x, y_cursor, rect.w - (2 * m.pad_x), m.row_h };
            int active = (ctx->editor.active_tool == k_visual_tools[i]) ? 1 : 0;
            int hovered = ui_hovered(ui, row);
            SDL_Color row_color = active ? p.button_fill_active : (hovered ? p.button_fill_hover : p.button_fill);
            SDL_Color label_color = active ? p.text_primary : sdl_color_ensure_contrast(p.text_muted, row_color);
            SDL_SetRenderDrawColor(renderer, row_color.r, row_color.g, row_color.b, row_color.a);
            (void)SDL_RenderFillRect(renderer, &row);
            SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
            (void)SDL_RenderDrawRect(renderer, &row);
            draw_bitmap_text(renderer, rect, row.x + 6, row.y + m.row_text_y, tool_name(k_visual_tools[i]), label_color, m.body_scale);
            y_cursor += m.row_h;
            if (active && active_option_count > 0u) {
                uint32_t option_i;
                for (option_i = 0u; option_i < active_option_count; ++option_i) {
                    VisualToolOptionKind option_kind = visual_tool_option_kind_for_index(ctx, ctx->editor.active_tool, option_i);
                    SDL_Rect option_row = left_tool_option_row_rect(rect, m, y_cursor);
                    char value_text[48];
                    visual_tool_option_value_text(ctx, option_kind, value_text, sizeof(value_text));
                    if (visual_tool_option_is_action_button(option_kind)) {
                        draw_tab_button(renderer,
                                        rect,
                                        option_row,
                                        "DELETE SELECTION",
                                        p.button_fill,
                                        p.button_fill_hover,
                                        p.button_fill_active,
                                        p.button_border,
                                        p.text_primary,
                                        m.body_scale,
                                        0,
                                        ui_hovered(ui, option_row));
                    } else {
                        SDL_Rect minus_rect = left_tool_option_minus_rect(option_row, m);
                        SDL_Rect value_rect = left_tool_option_value_rect(option_row, m);
                        SDL_Rect plus_rect = left_tool_option_plus_rect(option_row, m);
                        SDL_SetRenderDrawColor(renderer, p.button_fill.r, p.button_fill.g, p.button_fill.b, p.button_fill.a);
                        (void)SDL_RenderFillRect(renderer, &option_row);
                        SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
                        (void)SDL_RenderDrawRect(renderer, &option_row);
                        draw_bitmap_text(renderer,
                                         rect,
                                         option_row.x + 6,
                                         option_row.y + m.row_text_y,
                                         visual_tool_option_label(option_kind),
                                         p.text_muted,
                                         m.body_scale);
                        draw_tab_button(renderer,
                                        rect,
                                        minus_rect,
                                        "-",
                                        p.button_fill,
                                        p.button_fill_hover,
                                        p.button_fill_active,
                                        p.button_border,
                                        p.text_primary,
                                        m.body_scale,
                                        0,
                                        ui_hovered(ui, minus_rect));
                        draw_tab_button(renderer,
                                        rect,
                                        value_rect,
                                        value_text,
                                        p.button_fill,
                                        p.button_fill,
                                        p.button_fill,
                                        p.button_border,
                                        p.text_primary,
                                        m.body_scale,
                                        0,
                                        0);
                        draw_tab_button(renderer,
                                        rect,
                                        plus_rect,
                                        "+",
                                        p.button_fill,
                                        p.button_fill_hover,
                                        p.button_fill_active,
                                        p.button_border,
                                        p.text_primary,
                                        m.body_scale,
                                        0,
                                        ui_hovered(ui, plus_rect));
                    }
                    y_cursor += m.row_h;
                }
            }
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
    uint8_t active_locked = 0u;
    uint8_t active_opacity = 100u;
    uint32_t i;
    (void)interaction;
    if (!renderer || !ctx) {
        return;
    }
    right_slot = clamp_right_slot(ctx->ui_right_panel_slot);
    m = make_pane_layout_metrics(ctx);
    resolve_visual_theme_palette(theme, &p);

    for (i = 0u; i < ctx->document.layer_count; ++i) {
        if (ctx->document.layers[i].layer_id == ctx->editor.active_layer_id) {
            active_visible = ctx->document.layers[i].visible;
            active_locked = ctx->document.layers[i].locked;
            active_opacity = visual_layer_opacity_get(ctx, ctx->editor.active_layer_id);
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
        const char *pointer_state = "POINTER: PANEL";
        uint32_t history_cursor_units = 0u;
        uint32_t history_count_units = 0u;
        uint8_t active_color_index = drawing_program_color_index_clamp(ctx->ui_active_color_index);
        uint8_t active_color_value = drawing_program_color_value_from_index(active_color_index);
        uint8_t swatch_r = 0u;
        uint8_t swatch_g = 0u;
        uint8_t swatch_b = 0u;
        SDL_Rect reset_view_button;
        SDL_Rect clear_canvas_button;
        SDL_Rect delete_selection_button;
        SDL_Rect clear_history_button;
        uint8_t palette_i;
        uint32_t brush_radius = tool_brush_radius_samples(ctx, ctx->editor.active_tool);
        uint32_t brush_spacing = tool_brush_spacing_samples(ctx, ctx->editor.active_tool, brush_radius);
        uint32_t selection_w = (selection && selection->has_payload) ? selection->width : 0u;
        uint32_t selection_h = (selection && selection->has_payload) ? selection->height : 0u;
        uint32_t selection_payload = (selection && selection->has_payload) ? selection->payload_count : 0u;
        uint32_t selection_layer_id = (selection && selection->has_payload) ? selection->layer_id : 0u;
        int delete_selection_enabled = (selection &&
                                        selection->has_payload &&
                                        active_layer_allows_edits_visual(ctx))
                                           ? 1
                                           : 0;
        uint32_t clipboard_w = ctx->clipboard.has_payload ? ctx->clipboard.width : 0u;
        uint32_t clipboard_h = ctx->clipboard.has_payload ? ctx->clipboard.height : 0u;
        uint32_t clipboard_payload = ctx->clipboard.has_payload ? ctx->clipboard.payload_count : 0u;
        uint32_t clipboard_source_layer_id = ctx->clipboard.has_payload ? ctx->clipboard.source_layer_id : 0u;
        if (ui && ui->mouse_known) {
            SDL_Rect canvas_rect = { 0, 0, 0, 0 };
            if (pane_rect_for_module_type(ctx, 1u, &canvas_rect) &&
                point_in_rect(canvas_rect, ui->mouse_x, ui->mouse_y)) {
                pointer_state = "POINTER: CANVAS";
            }
        }
        drawing_program_history_query_units(&ctx->history, &history_cursor_units, &history_count_units);
        (void)snprintf(line, sizeof(line), "ACTIVE COLOR %u (%u)", (unsigned)active_color_index + 1u, (unsigned)active_color_value);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_primary, m.body_scale);
        y = right_canvas_palette_header_y(rect, m);
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

        y = right_canvas_metrics_start_y(rect, m);
        (void)snprintf(line, sizeof(line), "HISTORY %u/%u", history_cursor_units, history_count_units);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_BRUSH) {
            (void)snprintf(line,
                           sizeof(line),
                           "TOOL %s  R%u S%u O%u%% H%u%%",
                           tool_name(ctx->editor.active_tool),
                           brush_radius,
                           brush_spacing,
                           (unsigned)clamp_setting_u8(ctx->ui_tool_brush_opacity, 1u, 100u),
                           (unsigned)clamp_setting_u8(ctx->ui_tool_brush_hardness, 1u, 100u));
        } else if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_ERASER) {
            (void)snprintf(line,
                           sizeof(line),
                           "TOOL %s  R%u S%u",
                           tool_name(ctx->editor.active_tool),
                           brush_radius,
                           brush_spacing);
        } else if (tool_uses_shape_commit(ctx->editor.active_tool)) {
            (void)snprintf(line,
                           sizeof(line),
                           "TOOL %s  W%u MODE %s",
                           tool_name(ctx->editor.active_tool),
                           (unsigned)clamp_setting_u8(ctx->ui_tool_shape_stroke_width, 1u, 16u),
                           shape_mode_name(tool_shape_mode(ctx)));
        } else if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_FILL) {
            (void)snprintf(line,
                           sizeof(line),
                           "TOOL %s  TOL %u",
                           tool_name(ctx->editor.active_tool),
                           (unsigned)clamp_setting_u8(ctx->ui_tool_fill_tolerance, 0u, 16u));
        } else {
            (void)snprintf(line, sizeof(line), "TOOL %s", tool_name(ctx->editor.active_tool));
        }
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line,
                       sizeof(line),
                       "LAYER %u O%u%% V%s K%s",
                       (unsigned)ctx->editor.active_layer_id,
                       (unsigned)active_opacity,
                       active_visible ? "ON" : "OFF",
                       active_locked ? "ON" : "OFF");
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line, sizeof(line), "VIEW Z%.2fx PAN %d,%d", (double)ctx->editor.viewport.zoom, (int)ctx->editor.viewport.pan_x, (int)ctx->editor.viewport.pan_y);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line, sizeof(line), "RASTER %ux%u", ctx->document.raster_width, ctx->document.raster_height);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line, sizeof(line), "SELECTION %ux%u P%u L%u", selection_w, selection_h, selection_payload, selection_layer_id);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line, sizeof(line), "CLIPBOARD %ux%u P%u L%u", clipboard_w, clipboard_h, clipboard_payload, clipboard_source_layer_id);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line, sizeof(line), "SELSCOPE %s", drawing_program_selection_scope_policy_label());
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, pointer_state, p.text_muted, m.body_scale);

        reset_view_button = right_canvas_reset_view_button_rect(rect, m);
        clear_canvas_button = right_canvas_clear_canvas_button_rect(rect, m);
        delete_selection_button = right_canvas_delete_selection_button_rect(rect, m);
        clear_history_button = right_canvas_clear_history_button_rect(rect, m);

        draw_tab_button(renderer, rect, reset_view_button, "RESET VIEW",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, reset_view_button));
        draw_tab_button(renderer, rect, clear_canvas_button, "CLEAR CANVAS",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, clear_canvas_button));
        draw_tab_button(renderer, rect, delete_selection_button, "DELETE SELECTION",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        delete_selection_enabled ? p.text_primary : p.text_muted,
                        m.body_scale, 0, ui_hovered(ui, delete_selection_button));
        draw_tab_button(renderer, rect, clear_history_button, "CLEAR HISTORY",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, clear_history_button));
    } else {
        uint32_t display_i;
        SDL_Rect button_rect;
        SDL_Rect opacity_row_rect;
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "LAYER STACK", p.text_primary, m.body_scale);
        y += m.line_h;
        if (ctx->document.layer_count == 0u) {
            draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "NO LAYERS", p.text_muted, m.body_scale);
            y += m.line_h;
        }
        for (display_i = 0u; display_i < ctx->document.layer_count; ++display_i) {
            uint32_t model_i = (ctx->document.layer_count - 1u) - display_i;
            const DrawingProgramLayer *layer = &ctx->document.layers[model_i];
            int is_active = (layer->layer_id == ctx->editor.active_layer_id) ? 1 : 0;
            uint8_t layer_opacity = visual_layer_opacity_get(ctx, layer->layer_id);
            SDL_Color fill = is_active ? p.button_fill_active : p.button_fill;
            row = right_layer_row_rect(rect, m, display_i);
            if (ui_hovered(ui, row)) {
                fill = is_active ? p.button_fill_active : p.button_fill_hover;
            }
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            (void)SDL_RenderFillRect(renderer, &row);
            if (is_active) {
                SDL_SetRenderDrawColor(renderer, p.accent_primary.r, p.accent_primary.g, p.accent_primary.b, 255u);
            } else {
                SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
            }
            (void)SDL_RenderDrawRect(renderer, &row);
            (void)snprintf(line,
                           sizeof(line),
                           "%c L%u O%u%% V%s K%s %s",
                           is_active ? '*' : ' ',
                           (unsigned)layer->layer_id,
                           (unsigned)layer_opacity,
                           layer->visible ? "ON" : "OFF",
                           layer->locked ? "ON" : "OFF",
                           layer->name);
            draw_bitmap_text(renderer, rect, row.x + 6, row.y + m.row_text_y, line, p.text_primary, m.body_scale);
            y = row.y + row.h + m.section_gap;
        }

        opacity_row_rect = right_layer_opacity_row_rect(rect, m, ctx->document.layer_count);
        draw_tab_button(renderer,
                        rect,
                        opacity_row_rect,
                        "OPACITY",
                        p.button_fill,
                        p.button_fill_hover,
                        p.button_fill_active,
                        p.button_border,
                        p.text_muted,
                        m.body_scale,
                        0,
                        ui_hovered(ui, opacity_row_rect));
        {
            SDL_Rect opacity_track = right_layer_opacity_track_rect(opacity_row_rect, m);
            SDL_SetRenderDrawColor(renderer, p.pane_background_alt.r, p.pane_background_alt.g, p.pane_background_alt.b, p.pane_background_alt.a);
            (void)SDL_RenderFillRect(renderer, &opacity_track);
            {
                SDL_Rect fill_track = opacity_track;
                int fill_w = (int)(((uint32_t)opacity_track.w * (uint32_t)active_opacity) / 100u);
                if (fill_w < 1) {
                    fill_w = 1;
                }
                fill_track.w = fill_w;
                SDL_SetRenderDrawColor(renderer, p.accent_primary.r, p.accent_primary.g, p.accent_primary.b, p.accent_primary.a);
                (void)SDL_RenderFillRect(renderer, &fill_track);
            }
            SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
            (void)SDL_RenderDrawRect(renderer, &opacity_track);
            (void)snprintf(line, sizeof(line), "OPACITY %u%%", (unsigned)active_opacity);
            draw_bitmap_text(renderer, rect, opacity_row_rect.x + 6, opacity_row_rect.y + m.row_text_y, line, p.text_primary, m.body_scale);
        }

        y = right_layer_actions_label_y(rect, m, ctx->document.layer_count);
        draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "ACTIONS", p.text_primary, m.body_scale);

        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ADD);
        draw_tab_button(renderer, rect, button_rect, "ADD LAYER",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, button_rect));
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_DUPLICATE);
        draw_tab_button(renderer, rect, button_rect, "DUPLICATE SELECTED",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, button_rect));
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_RENAME);
        draw_tab_button(renderer, rect, button_rect, "RENAME SELECTED",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, button_rect));
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_DELETE);
        draw_tab_button(renderer, rect, button_rect, "DELETE SELECTED",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, button_rect));
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ACTIVE_PREV);
        draw_tab_button(renderer, rect, button_rect, "ACTIVE PREV",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, button_rect));
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ACTIVE_NEXT);
        draw_tab_button(renderer, rect, button_rect, "ACTIVE NEXT",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, button_rect));
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_MOVE_UP);
        draw_tab_button(renderer, rect, button_rect, "MOVE UP",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, button_rect));
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_MOVE_DOWN);
        draw_tab_button(renderer, rect, button_rect, "MOVE DOWN",
                        p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                        p.text_primary, m.body_scale, 0, ui_hovered(ui, button_rect));
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_TOGGLE_VISIBLE);
        draw_tab_button(renderer,
                        rect,
                        button_rect,
                        active_visible ? "VISIBLE: ON" : "VISIBLE: OFF",
                        p.button_fill,
                        p.button_fill_hover,
                        p.button_fill_active,
                        p.button_border,
                        p.text_primary,
                        m.body_scale,
                        0,
                        ui_hovered(ui, button_rect));
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_TOGGLE_LOCK);
        draw_tab_button(renderer,
                        rect,
                        button_rect,
                        active_locked ? "LOCK: ON" : "LOCK: OFF",
                        p.button_fill,
                        p.button_fill_hover,
                        p.button_fill_active,
                        p.button_border,
                        p.text_primary,
                        m.body_scale,
                        0,
                        ui_hovered(ui, button_rect));
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

static void apply_layer_rename_auto(DrawingProgramAppContext *ctx) {
    uint32_t active_index = 0u;
    if (!ctx || !active_layer_query(ctx, 0, &active_index, 0, 0) || active_index >= ctx->document.layer_count) {
        return;
    }
    (void)snprintf(ctx->document.layers[active_index].name,
                   sizeof(ctx->document.layers[active_index].name),
                   "Layer %u",
                   (unsigned)ctx->document.layers[active_index].layer_id);
}

static void apply_layer_duplicate_active(DrawingProgramAppContext *ctx) {
    uint32_t source_index = 0u;
    uint32_t source_layer_id = 0u;
    uint32_t source_opacity = 100u;
    uint8_t *copied_samples = 0;
    const uint8_t *source_samples = 0;
    uint32_t source_sample_count = 0u;
    char source_name[DRAWING_PROGRAM_LAYER_NAME_CAPACITY];
    CoreResult result;
    if (!ctx) {
        return;
    }
    if (!active_layer_query(ctx, &source_layer_id, &source_index, 0, 0) ||
        source_index >= ctx->document.layer_count ||
        source_layer_id == 0u) {
        return;
    }
    source_opacity = (uint32_t)visual_layer_opacity_get(ctx, source_layer_id);
    (void)snprintf(source_name, sizeof(source_name), "%s", ctx->document.layers[source_index].name);
    result = drawing_program_layer_raster_store_export_layer(&ctx->layer_rasters,
                                                             source_layer_id,
                                                             &source_samples,
                                                             &source_sample_count);
    if (result.code != CORE_OK ||
        !source_samples ||
        source_sample_count != ctx->document.raster_sample_count) {
        if (source_layer_id == 1u) {
            source_samples = ctx->document.raster_samples;
            source_sample_count = ctx->document.raster_sample_count;
        }
    }
    if (source_samples &&
        source_sample_count == ctx->document.raster_sample_count &&
        source_sample_count > 0u) {
        copied_samples = (uint8_t *)malloc((size_t)source_sample_count);
        if (copied_samples) {
            memcpy(copied_samples, source_samples, (size_t)source_sample_count);
        }
    }
    apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_ADD_LAYER);
    if (ctx->editor.active_layer_id == 0u || ctx->editor.active_layer_id == source_layer_id) {
        free(copied_samples);
        return;
    }
    if (copied_samples &&
        source_sample_count == ctx->document.raster_sample_count) {
        result = drawing_program_layer_raster_store_import_layer(&ctx->layer_rasters,
                                                                 ctx->editor.active_layer_id,
                                                                 copied_samples,
                                                                 source_sample_count);
        if (result.code != CORE_OK) {
            fprintf(stderr, "drawing_program: duplicate layer import failed: %s\n", result.message);
        }
    } else {
        fprintf(stderr, "drawing_program: duplicate layer source samples unavailable\n");
    }
    free(copied_samples);
    {
        uint32_t active_index = 0u;
        if (active_layer_query(ctx, 0, &active_index, 0, 0) && active_index < ctx->document.layer_count) {
            (void)snprintf(ctx->document.layers[active_index].name,
                           sizeof(ctx->document.layers[active_index].name),
                           "%s Copy",
                           source_name);
        }
    }
    visual_layer_opacity_set(ctx, ctx->editor.active_layer_id, (uint8_t)source_opacity);
}

static void handle_left_panel_click(DrawingProgramAppContext *ctx,
                                    SDL_Rect rect,
                                    int x,
                                    int y,
                                    VisualPanelUiState *ui,
                                    VisualSelectionState *selection) {
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
        int y_cursor = content_y;
        uint32_t active_option_count = visual_tool_option_count(ctx, ctx->editor.active_tool);
        for (i = 0u; i < sizeof(k_visual_tools) / sizeof(k_visual_tools[0]); ++i) {
            SDL_Rect row = { rect.x + m.pad_x, y_cursor, rect.w - (2 * m.pad_x), m.row_h };
            if (point_in_rect(row, x, y)) {
                apply_workflow_control_if_valid(ctx, workflow_control_for_tool(k_visual_tools[i]));
                return;
            }
            y_cursor += m.row_h;
            if (ctx->editor.active_tool == k_visual_tools[i] && active_option_count > 0u) {
                uint32_t option_i;
                for (option_i = 0u; option_i < active_option_count; ++option_i) {
                    VisualToolOptionKind option_kind = visual_tool_option_kind_for_index(ctx, ctx->editor.active_tool, option_i);
                    SDL_Rect option_row = left_tool_option_row_rect(rect, m, y_cursor);
                    if (visual_tool_option_is_action_button(option_kind)) {
                        if (point_in_rect(option_row, x, y) &&
                            option_kind == VISUAL_TOOL_OPTION_SELECT_DELETE &&
                            selection &&
                            selection->has_payload &&
                            active_layer_allows_edits_visual(ctx)) {
                            (void)drawing_program_selection_delete_payload(&ctx->document,
                                                                           &ctx->layer_rasters,
                                                                           ctx->editor.active_layer_id,
                                                                           &ctx->history,
                                                                           selection);
                            return;
                        }
                    } else {
                        SDL_Rect minus_rect = left_tool_option_minus_rect(option_row, m);
                        SDL_Rect plus_rect = left_tool_option_plus_rect(option_row, m);
                        if (point_in_rect(minus_rect, x, y)) {
                            visual_tool_option_adjust(ctx, option_kind, -1);
                            return;
                        }
                        if (point_in_rect(plus_rect, x, y)) {
                            visual_tool_option_adjust(ctx, option_kind, 1);
                            return;
                        }
                    }
                    y_cursor += m.row_h;
                }
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
                                     VisualPanelUiState *ui,
                                     VisualSelectionState *selection) {
    VisualPaneLayoutMetrics m;
    int content_y;
    SDL_Rect tab_canvas;
    SDL_Rect tab_layer;
    if (!ctx || !ui) {
        return;
    }
    m = make_pane_layout_metrics(ctx);
    visual_layer_opacity_sync_document(ctx);
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
        uint32_t display_i;
        SDL_Rect action;
        SDL_Rect opacity_row;
        SDL_Rect opacity_track;
        uint32_t active_layer_id = 0u;
        uint32_t active_layer_index = 0u;
        for (display_i = 0u; display_i < ctx->document.layer_count; ++display_i) {
            uint32_t model_i = (ctx->document.layer_count - 1u) - display_i;
            SDL_Rect row = right_layer_row_rect(rect, m, display_i);
            if (point_in_rect(row, x, y)) {
                (void)drawing_program_runtime_orchestration_set_active_layer_id(ctx, ctx->document.layers[model_i].layer_id);
                return;
            }
        }
        if (active_layer_query(ctx, &active_layer_id, &active_layer_index, 0, 0) &&
            active_layer_index < ctx->document.layer_count) {
            opacity_row = right_layer_opacity_row_rect(rect, m, ctx->document.layer_count);
            opacity_track = right_layer_opacity_track_rect(opacity_row, m);
            if (point_in_rect(opacity_row, x, y)) {
                int relative_x = x - opacity_track.x;
                int opacity = 100;
                if (relative_x < 0) {
                    relative_x = 0;
                }
                if (relative_x > opacity_track.w) {
                    relative_x = opacity_track.w;
                }
                if (opacity_track.w > 0) {
                    opacity = (relative_x * 100) / opacity_track.w;
                }
                if (opacity < 0) {
                    opacity = 0;
                }
                if (opacity > 100) {
                    opacity = 100;
                }
                visual_layer_opacity_set(ctx, active_layer_id, (uint8_t)opacity);
                return;
            }
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ADD);
        if (point_in_rect(action, x, y)) {
            apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_ADD_LAYER);
            visual_layer_opacity_sync_document(ctx);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_DUPLICATE);
        if (point_in_rect(action, x, y)) {
            apply_layer_duplicate_active(ctx);
            visual_layer_opacity_sync_document(ctx);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_RENAME);
        if (point_in_rect(action, x, y)) {
            apply_layer_rename_auto(ctx);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_DELETE);
        if (point_in_rect(action, x, y)) {
            apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_DELETE_ACTIVE_LAYER);
            visual_layer_opacity_sync_document(ctx);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ACTIVE_PREV);
        if (point_in_rect(action, x, y)) {
            apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_PREV);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ACTIVE_NEXT);
        if (point_in_rect(action, x, y)) {
            apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_NEXT);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_MOVE_UP);
        if (point_in_rect(action, x, y)) {
            apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_UP);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_MOVE_DOWN);
        if (point_in_rect(action, x, y)) {
            apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_DOWN);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_TOGGLE_VISIBLE);
        if (point_in_rect(action, x, y)) {
            apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY);
            return;
        }
        action = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_TOGGLE_LOCK);
        if (point_in_rect(action, x, y)) {
            apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_LOCK);
            return;
        }
    } else {
        uint8_t palette_i;
        SDL_Rect reset_view_button;
        SDL_Rect clear_canvas_button;
        SDL_Rect delete_selection_button;
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
        clear_canvas_button = right_canvas_clear_canvas_button_rect(rect, m);
        if (point_in_rect(clear_canvas_button, x, y)) {
            apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS);
            return;
        }
        delete_selection_button = right_canvas_delete_selection_button_rect(rect, m);
        if (point_in_rect(delete_selection_button, x, y) &&
            selection &&
            selection->has_payload &&
            active_layer_allows_edits_visual(ctx)) {
            (void)drawing_program_selection_delete_payload(&ctx->document,
                                                           &ctx->layer_rasters,
                                                           ctx->editor.active_layer_id,
                                                           &ctx->history,
                                                           selection);
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
    interaction->transform_active = 0u;
    interaction->transform_kind = (uint8_t)VISUAL_TRANSFORM_SESSION_NONE;
    interaction->move_axis_lock = 0u;
    interaction->marquee_commit_mode = (uint8_t)VISUAL_MARQUEE_COMMIT_REPLACE;
    interaction->has_last_sample = 0u;
}

static void cancel_selection_transient(VisualSelectionState *selection) {
    drawing_program_selection_cancel_transient(selection);
}

static void apply_selection_move_axis_lock(VisualSelectionState *selection,
                                           VisualCanvasInteractionState *interaction,
                                           SDL_Keymod mods) {
    int32_t dx;
    int32_t dy;
    int lock_shift;
    if (!selection || !interaction || !selection->moving) {
        return;
    }
    lock_shift = (mods & KMOD_SHIFT) ? 1 : 0;
    if (!lock_shift) {
        interaction->move_axis_lock = 0u;
        return;
    }
    dx = selection->offset_x - selection->move_anchor_offset_x;
    dy = selection->offset_y - selection->move_anchor_offset_y;
    if (interaction->move_axis_lock == 0u) {
        interaction->move_axis_lock = (abs(dx) >= abs(dy)) ? 1u : 2u;
    }
    if (interaction->move_axis_lock == 1u) {
        selection->offset_y = selection->move_anchor_offset_y;
    } else if (interaction->move_axis_lock == 2u) {
        selection->offset_x = selection->move_anchor_offset_x;
    }
}

static void apply_selection_move_canvas_bounds(const DrawingProgramDocument *document,
                                               VisualSelectionState *selection) {
    int32_t min_offset_x;
    int32_t max_offset_x;
    int32_t min_offset_y;
    int32_t max_offset_y;
    if (!document || !selection || !selection->has_payload) {
        return;
    }
    if (selection->width == 0u || selection->height == 0u ||
        document->raster_width == 0u || document->raster_height == 0u) {
        selection->offset_x = 0;
        selection->offset_y = 0;
        return;
    }
    min_offset_x = -(int32_t)selection->origin_x;
    min_offset_y = -(int32_t)selection->origin_y;
    max_offset_x = (int32_t)document->raster_width - (int32_t)(selection->origin_x + selection->width);
    max_offset_y = (int32_t)document->raster_height - (int32_t)(selection->origin_y + selection->height);
    if (selection->offset_x < min_offset_x) {
        selection->offset_x = min_offset_x;
    }
    if (selection->offset_x > max_offset_x) {
        selection->offset_x = max_offset_x;
    }
    if (selection->offset_y < min_offset_y) {
        selection->offset_y = min_offset_y;
    }
    if (selection->offset_y > max_offset_y) {
        selection->offset_y = max_offset_y;
    }
}

static void visual_transform_session_reset(VisualCanvasInteractionState *interaction) {
    if (!interaction) {
        return;
    }
    interaction->transform_active = 0u;
    interaction->transform_kind = (uint8_t)VISUAL_TRANSFORM_SESSION_NONE;
    interaction->move_axis_lock = 0u;
}

static int visual_transform_session_is_move_active(const VisualCanvasInteractionState *interaction) {
    if (!interaction) {
        return 0;
    }
    return interaction->transform_active &&
                   interaction->transform_kind == (uint8_t)VISUAL_TRANSFORM_SESSION_MOVE
               ? 1
               : 0;
}

static void visual_transform_session_begin_move(VisualCanvasInteractionState *interaction,
                                                VisualSelectionState *selection,
                                                uint32_t sample_x,
                                                uint32_t sample_y) {
    if (!interaction || !selection) {
        return;
    }
    drawing_program_selection_begin_move_tracking(selection, sample_x, sample_y);
    interaction->transform_active = 1u;
    interaction->transform_kind = (uint8_t)VISUAL_TRANSFORM_SESSION_MOVE;
    interaction->move_axis_lock = 0u;
}

static void visual_transform_session_update_move(const DrawingProgramAppContext *ctx,
                                                 VisualCanvasInteractionState *interaction,
                                                 VisualSelectionState *selection,
                                                 uint32_t sample_x,
                                                 uint32_t sample_y,
                                                 SDL_Keymod mods) {
    if (!ctx || !selection || !visual_transform_session_is_move_active(interaction) || !selection->moving) {
        return;
    }
    drawing_program_selection_update_move_offset(selection, sample_x, sample_y);
    apply_selection_move_axis_lock(selection, interaction, mods);
    apply_selection_move_canvas_bounds(&ctx->document, selection);
}

static CoreResult visual_transform_session_commit_move(DrawingProgramAppContext *ctx,
                                                       VisualCanvasInteractionState *interaction,
                                                       VisualSelectionState *selection) {
    CoreResult move_commit;
    if (!ctx || !selection || !visual_transform_session_is_move_active(interaction)) {
        return core_result_ok();
    }
    move_commit = visual_selection_commit_move(ctx, selection);
    if (move_commit.code != CORE_OK) {
        selection->moving = 0u;
        selection->offset_x = 0;
        selection->offset_y = 0;
    }
    visual_transform_session_reset(interaction);
    return move_commit;
}

static CoreResult visual_transform_session_nudge_move(DrawingProgramAppContext *ctx,
                                                      VisualCanvasInteractionState *interaction,
                                                      VisualSelectionState *selection,
                                                      int32_t dx,
                                                      int32_t dy) {
    uint32_t anchor_x = 0u;
    uint32_t anchor_y = 0u;
    if (!ctx || !selection || !selection->has_payload) {
        return core_result_ok();
    }
    anchor_x = selection->origin_x;
    anchor_y = selection->origin_y;
    visual_transform_session_begin_move(interaction, selection, anchor_x, anchor_y);
    selection->offset_x = dx;
    selection->offset_y = dy;
    apply_selection_move_canvas_bounds(&ctx->document, selection);
    return visual_transform_session_commit_move(ctx, interaction, selection);
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
    visual_transform_session_reset(interaction);
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
#define selection_state app.selection

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
    canvas_interaction.marquee_commit_mode = (uint8_t)VISUAL_MARQUEE_COMMIT_REPLACE;
    memset(&panel_ui, 0, sizeof(panel_ui));
    drawing_program_selection_cancel_transient(&selection_state);
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
                    handle_left_panel_click(&app, left_pane, click_x, click_y, &panel_ui, &selection_state);
                    continue;
                }
                if (event.button.button == SDL_BUTTON_LEFT && click_on_right) {
                    cancel_all_transient_interactions(&app, &canvas_interaction, &selection_state, 0);
                    handle_right_panel_click(&app, right_pane, click_x, click_y, &panel_ui, &selection_state);
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
                        drawing_program_selection_begin_marquee(&selection_state, sample_x, sample_y);
                        cancel_canvas_draw_and_shape(&canvas_interaction);
                        canvas_interaction.marquee_commit_mode =
                            (uint8_t)visual_marquee_commit_mode_from_mods(SDL_GetModState());
                    } else if (app.editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE &&
                               selection_state.has_payload &&
                               ((screen_to_canvas_sample(&app, canvas_pane, click_x, click_y, &sample_x, &sample_y) &&
                                 visual_selection_begin_move(&selection_state, sample_x, sample_y)) ||
                                selection_move_handle_hit(&app, canvas_pane, &selection_state, click_x, click_y)) &&
                               screen_to_canvas_sample_clamped(&app, canvas_pane, click_x, click_y, &sample_x, &sample_y)) {
                        cancel_canvas_draw_and_shape(&canvas_interaction);
                        visual_transform_session_begin_move(&canvas_interaction, &selection_state, sample_x, sample_y);
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
                        if (visual_transform_session_is_move_active(&canvas_interaction) && selection_state.moving) {
                            (void)screen_to_canvas_sample_clamped(&app, canvas_pane, release_x, release_y, &sample_x, &sample_y);
                            visual_transform_session_update_move(&app,
                                                                 &canvas_interaction,
                                                                 &selection_state,
                                                                 sample_x,
                                                                 sample_y,
                                                                 SDL_GetModState());
                        }
                    }
                    if (selection_state.selecting) {
                        VisualMarqueeCommitMode mode =
                            visual_marquee_commit_mode_clamp(canvas_interaction.marquee_commit_mode);
                        (void)visual_selection_capture_from_marquee(&app, &selection_state, mode);
                    }
                    if (visual_transform_session_is_move_active(&canvas_interaction) && selection_state.moving) {
                        CoreResult move_commit =
                            visual_transform_session_commit_move(&app, &canvas_interaction, &selection_state);
                        if (move_commit.code != CORE_OK) {
                            fprintf(stderr, "drawing_program: selection move commit failed: %s\n", move_commit.message);
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
                if (visual_transform_session_is_move_active(&canvas_interaction) &&
                    selection_state.moving &&
                    app.editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE) {
                    uint32_t sample_x = 0u;
                    uint32_t sample_y = 0u;
                    if (screen_to_canvas_sample_clamped(&app,
                                                        canvas_pane,
                                                        panel_ui.mouse_x,
                                                        panel_ui.mouse_y,
                                                        &sample_x,
                                                        &sample_y)) {
                        visual_transform_session_update_move(&app,
                                                             &canvas_interaction,
                                                             &selection_state,
                                                             sample_x,
                                                             sample_y,
                                                             SDL_GetModState());
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
                if (ctrl_or_cmd && event.key.keysym.sym == SDLK_c) {
                    (void)drawing_program_selection_copy_payload(&selection_state, &app.clipboard);
                    continue;
                }
                if (ctrl_or_cmd && event.key.keysym.sym == SDLK_x) {
                    if (active_layer_allows_edits_visual(&app)) {
                        cancel_canvas_draw_and_shape(&canvas_interaction);
                        cancel_selection_transient(&selection_state);
                        (void)drawing_program_selection_cut_to_clipboard(&app.document,
                                                                         &app.layer_rasters,
                                                                         app.editor.active_layer_id,
                                                                         &app.history,
                                                                         &selection_state,
                                                                         &app.clipboard);
                    }
                    continue;
                }
                if (ctrl_or_cmd && event.key.keysym.sym == SDLK_v) {
                    if (active_layer_allows_edits_visual(&app)) {
                        int32_t paste_x = 0;
                        int32_t paste_y = 0;
                        uint32_t sample_x = 0u;
                        uint32_t sample_y = 0u;
                        cancel_canvas_draw_and_shape(&canvas_interaction);
                        cancel_selection_transient(&selection_state);
                        if (selection_state.has_payload) {
                            paste_x = (int32_t)selection_state.origin_x;
                            paste_y = (int32_t)selection_state.origin_y;
                        } else if (app.clipboard.has_payload) {
                            if (app.document.raster_width > app.clipboard.width) {
                                paste_x = (int32_t)((app.document.raster_width - app.clipboard.width) / 2u);
                            }
                            if (app.document.raster_height > app.clipboard.height) {
                                paste_y = (int32_t)((app.document.raster_height - app.clipboard.height) / 2u);
                            }
                        }
                        if (has_canvas_pane &&
                            panel_ui.mouse_known &&
                            point_in_rect(canvas_pane, panel_ui.mouse_x, panel_ui.mouse_y) &&
                            screen_to_canvas_sample(&app,
                                                    canvas_pane,
                                                    panel_ui.mouse_x,
                                                    panel_ui.mouse_y,
                                                    &sample_x,
                                                    &sample_y)) {
                            paste_x = (int32_t)sample_x;
                            paste_y = (int32_t)sample_y;
                        }
                        (void)drawing_program_selection_paste_from_clipboard(&app.document,
                                                                             &app.layer_rasters,
                                                                             app.editor.active_layer_id,
                                                                             &app.history,
                                                                             &selection_state,
                                                                             &app.clipboard,
                                                                             paste_x,
                                                                             paste_y);
                    }
                    continue;
                }
                if (ctrl_or_cmd && shift && event.key.keysym.sym == SDLK_n) {
                    apply_workflow_control_if_valid(&app, DRAWING_PROGRAM_WORKFLOW_CONTROL_ADD_LAYER);
                    continue;
                }
                if (ctrl_or_cmd && event.key.keysym.sym == SDLK_d) {
                    drawing_program_selection_reset(&selection_state);
                    cancel_canvas_draw_and_shape(&canvas_interaction);
                    continue;
                }
                if (ctrl_or_cmd && event.key.keysym.sym == SDLK_a) {
                    app.editor.active_tool = DRAWING_PROGRAM_TOOL_SELECT;
                    app.tool_switch_total += 1u;
                    (void)drawing_program_selection_select_all(&app.document,
                                                               &app.layer_rasters,
                                                               app.editor.active_layer_id,
                                                               &selection_state);
                    cancel_canvas_draw_and_shape(&canvas_interaction);
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
                if (ctrl_or_cmd &&
                    (event.key.keysym.sym == SDLK_LEFTBRACKET || event.key.keysym.sym == SDLK_RIGHTBRACKET)) {
                    apply_workflow_control_if_valid(&app,
                                                    (event.key.keysym.sym == SDLK_LEFTBRACKET)
                                                        ? DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_PREV
                                                        : DRAWING_PROGRAM_WORKFLOW_CONTROL_SELECT_LAYER_NEXT);
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
                if (app.editor.active_tool == DRAWING_PROGRAM_TOOL_MOVE &&
                    selection_state.has_payload &&
                    (event.key.keysym.sym == SDLK_UP ||
                     event.key.keysym.sym == SDLK_DOWN ||
                     event.key.keysym.sym == SDLK_LEFT ||
                     event.key.keysym.sym == SDLK_RIGHT)) {
                    int32_t dx = 0;
                    int32_t dy = 0;
                    int32_t nudge_step = shift ? 10 : 1;
                    CoreResult move_commit;
                    switch (event.key.keysym.sym) {
                        case SDLK_UP: dy = -nudge_step; break;
                        case SDLK_DOWN: dy = nudge_step; break;
                        case SDLK_LEFT: dx = -nudge_step; break;
                        case SDLK_RIGHT: dx = nudge_step; break;
                        default: break;
                    }
                    cancel_canvas_draw_and_shape(&canvas_interaction);
                    cancel_selection_transient(&selection_state);
                    move_commit =
                        visual_transform_session_nudge_move(&app, &canvas_interaction, &selection_state, dx, dy);
                    if (move_commit.code != CORE_OK) {
                        fprintf(stderr, "drawing_program: selection nudge commit failed: %s\n", move_commit.message);
                        selection_state.offset_x = 0;
                        selection_state.offset_y = 0;
                    }
                    continue;
                }
                if ((event.key.keysym.sym == SDLK_BACKSPACE || event.key.keysym.sym == SDLK_DELETE) &&
                    selection_state.has_payload &&
                    active_layer_allows_edits_visual(&app)) {
                    (void)drawing_program_selection_delete_payload(&app.document,
                                                                   &app.layer_rasters,
                                                                   app.editor.active_layer_id,
                                                                   &app.history,
                                                                   &selection_state);
                    continue;
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
                    case SDLK_k:
                        control = DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_LOCK;
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
        visual_layer_opacity_sync_document(&app);
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

#undef selection_state

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
