#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core_pack.h"
#include "core_theme.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_ui_color_state.h"
#include "drawing_program/drawing_program_visual_input_handlers.h"
#include "drawing_program/drawing_program_visual_input_selection_ops.h"
#include "drawing_program/drawing_program_visual_input_support.h"
#include "drawing_program/drawing_program_texture_workspace.h"
#include "drawing_program/drawing_program_visual_canvas_coords.h"
#include "drawing_program/drawing_program_visual_canvas_world_render.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_overlay_render.h"
#include "drawing_program/drawing_program_visual_right_panel_defs.h"
#include "drawing_program/drawing_program_visual_surface_cache.h"
#include "drawing_program/drawing_program_visual_text_render.h"
#include "drawing_program/drawing_program_visual_transform_ops.h"
#include "drawing_program_lifecycle_test_support.h"

#include "drawing_program_lifecycle_snapshot_suite_internal.h"
#include "drawing_program_lifecycle_snapshot_suite.h"
#include "drawing_program_lifecycle_selection_layer_suite.h"
#include "drawing_program_lifecycle_authoring_host_suite.h"
#include "drawing_program_lifecycle_baseline_history_suite.h"
#include "drawing_program_lifecycle_composed_source_suite.h"
#include "drawing_program_lifecycle_composed_source_rcp1_suite.h"
#include "drawing_program_lifecycle_composed_source_rws1_suite.h"
#include "drawing_program_lifecycle_export_suite.h"
#include "drawing_program_lifecycle_object_path_suite.h"
#include "drawing_program_lifecycle_persistence_contract_suite.h"
#include "drawing_program_lifecycle_render_domain_suite.h"
#include "drawing_program_lifecycle_runtime_render_suite.h"
#include "drawing_program_lifecycle_surface_cache_contract_suite.h"
#include "drawing_program_lifecycle_texture_export_suite.h"
#include "drawing_program_lifecycle_texture_import_suite.h"

const DrawingProgramVisualInputHandlersHooks *drawing_program_visual_input_handlers_hooks(void);

static void lifecycle_inspect_mouse_down_event(SDL_Event *event, int x, int y) {
    memset(event, 0, sizeof(*event));
    event->type = SDL_MOUSEBUTTONDOWN;
    event->button.button = SDL_BUTTON_LEFT;
    event->button.x = x;
    event->button.y = y;
}

static int lifecycle_inspect_dispatch_click(DrawingProgramAppContext *ctx,
                                            VisualCanvasInteractionState *interaction,
                                            VisualPanelUiState *panel_ui,
                                            SDL_Rect left_pane,
                                            SDL_Rect right_pane,
                                            SDL_Rect canvas_pane,
                                            int x,
                                            int y,
                                            const char *label) {
    SDL_Event event;
    const DrawingProgramVisualInputHandlersHooks *input_hooks = drawing_program_visual_input_handlers_hooks();
    lifecycle_inspect_mouse_down_event(&event, x, y);
    if (!drawing_program_visual_input_handle_mouse_button_down_payload(&event,
                                                                       1,
                                                                       event.button.x,
                                                                       event.button.y,
                                                                       0,
                                                                       left_pane,
                                                                       1,
                                                                       right_pane,
                                                                       1,
                                                                       canvas_pane,
                                                                       ctx,
                                                                       interaction,
                                                                       &ctx->selection,
                                                                       panel_ui,
                                                                       input_hooks)) {
        fprintf(stderr, "inspect.click %s consumed=0 x=%d y=%d\n", label ? label : "unknown", x, y);
        return 0;
    }
    fprintf(stderr, "inspect.click %s consumed=1 x=%d y=%d\n", label ? label : "unknown", x, y);
    return 1;
}

static void lifecycle_inspect_noop_selection_overlay(SDL_Renderer *renderer,
                                                     SDL_Rect pane_rect,
                                                     const DrawingProgramAppContext *ctx,
                                                     const CoreThemePreset *theme,
                                                     const VisualCanvasSheetMetrics *metrics,
                                                     const VisualSelectionState *selection) {
    (void)renderer;
    (void)pane_rect;
    (void)ctx;
    (void)theme;
    (void)metrics;
    (void)selection;
}

static void lifecycle_inspect_noop_shape_overlay(SDL_Renderer *renderer,
                                                 SDL_Rect pane_rect,
                                                 const DrawingProgramAppContext *ctx,
                                                 const CoreThemePreset *theme,
                                                 const VisualCanvasSheetMetrics *metrics,
                                                 const VisualCanvasInteractionState *interaction,
                                                 const VisualPanelUiState *ui) {
    (void)renderer;
    (void)pane_rect;
    (void)ctx;
    (void)theme;
    (void)metrics;
    (void)interaction;
    (void)ui;
}

static int lifecycle_inspect_sample_surface_rgba(SDL_Surface *surface,
                                                 int x,
                                                 int y,
                                                 uint8_t *out_r,
                                                 uint8_t *out_g,
                                                 uint8_t *out_b,
                                                 uint8_t *out_a) {
    uint8_t *pixel_ptr = 0;
    uint32_t pixel_value = 0u;
    if (!surface || !surface->pixels || !out_r || !out_g || !out_b || !out_a ||
        x < 0 || y < 0 || x >= surface->w || y >= surface->h) {
        return 0;
    }
    pixel_ptr = (uint8_t *)surface->pixels + (y * surface->pitch) + (x * surface->format->BytesPerPixel);
    memcpy(&pixel_value, pixel_ptr, sizeof(pixel_value));
    SDL_GetRGBA(pixel_value, surface->format, out_r, out_g, out_b, out_a);
    return 1;
}

static int lifecycle_inspect_find_first_nontransparent_sample(const DrawingProgramLayerRasterStore *layer_rasters,
                                                             uint32_t layer_id,
                                                             uint32_t *out_x,
                                                             uint32_t *out_y) {
    const DrawingProgramRasterSample *samples = 0;
    uint32_t sample_count = 0u;
    uint32_t i;
    if (!layer_rasters || !out_x || !out_y ||
        drawing_program_layer_raster_store_export_layer(layer_rasters, layer_id, &samples, &sample_count).code != CORE_OK ||
        !samples || layer_rasters->raster_width == 0u) {
        return 0;
    }
    for (i = 0u; i < sample_count; ++i) {
        if (!drawing_program_color_sample_is_transparent(samples[i])) {
            *out_x = i % layer_rasters->raster_width;
            *out_y = i / layer_rasters->raster_width;
            return 1;
        }
    }
    return 0;
}

static int lifecycle_inspect_find_first_distinct_layer_sample(const DrawingProgramLayerRasterStore *layer_rasters,
                                                             uint32_t lower_layer_id,
                                                             uint32_t upper_layer_id,
                                                             uint32_t *out_x,
                                                             uint32_t *out_y) {
    const DrawingProgramRasterSample *lower_samples = 0;
    const DrawingProgramRasterSample *upper_samples = 0;
    uint32_t lower_count = 0u;
    uint32_t upper_count = 0u;
    uint32_t i;
    if (!layer_rasters || !out_x || !out_y ||
        drawing_program_layer_raster_store_export_layer(layer_rasters, lower_layer_id, &lower_samples, &lower_count).code !=
            CORE_OK ||
        drawing_program_layer_raster_store_export_layer(layer_rasters, upper_layer_id, &upper_samples, &upper_count).code !=
            CORE_OK ||
        !lower_samples || !upper_samples || lower_count != upper_count || layer_rasters->raster_width == 0u) {
        return lifecycle_inspect_find_first_nontransparent_sample(layer_rasters, upper_layer_id, out_x, out_y);
    }
    for (i = 0u; i < upper_count; ++i) {
        if (!drawing_program_color_sample_is_transparent(upper_samples[i]) &&
            upper_samples[i] != lower_samples[i]) {
            *out_x = i % layer_rasters->raster_width;
            *out_y = i / layer_rasters->raster_width;
            return 1;
        }
    }
    return lifecycle_inspect_find_first_nontransparent_sample(layer_rasters, upper_layer_id, out_x, out_y);
}

static int lifecycle_inspect_render_active_sample(SDL_Renderer *renderer,
                                                  SDL_Surface *surface,
                                                  SDL_Rect canvas_pane,
                                                  DrawingProgramAppContext *ctx,
                                                  const CoreThemePreset *theme_preset,
                                                  uint32_t sample_x,
                                                  uint32_t sample_y,
                                                  uint8_t *out_r,
                                                  uint8_t *out_g,
                                                  uint8_t *out_b,
                                                  uint8_t *out_a) {
    DrawingProgramVisualCanvasWorldRenderHooks render_hooks;
    VisualSelectionState selection;
    VisualPanelUiState panel_ui;
    VisualCanvasInteractionState interaction;
    VisualCanvasSheetMetrics metrics;
    int screen_x = 0;
    int screen_y = 0;
    if (!renderer || !surface || !ctx || !theme_preset || !out_r || !out_g || !out_b || !out_a) {
        return 0;
    }
    if (!drawing_program_texture_workspace_active_sheet_metrics(ctx, canvas_pane, &metrics)) {
        return 0;
    }
    screen_x = metrics.sheet_rect.x +
               (int)(((uint64_t)sample_x * (uint64_t)metrics.sheet_rect.w) /
                     (uint64_t)ctx->document.raster_width);
    screen_y = metrics.sheet_rect.y +
               (int)(((uint64_t)sample_y * (uint64_t)metrics.sheet_rect.h) /
                     (uint64_t)ctx->document.raster_height);
    memset(&render_hooks, 0, sizeof(render_hooks));
    memset(&selection, 0, sizeof(selection));
    memset(&panel_ui, 0, sizeof(panel_ui));
    memset(&interaction, 0, sizeof(interaction));
    render_hooks.compute_canvas_sheet_metrics = drawing_program_visual_compute_canvas_sheet_metrics;
    render_hooks.draw_bitmap_text = drawing_program_visual_draw_bitmap_text;
    render_hooks.draw_selection_overlay = lifecycle_inspect_noop_selection_overlay;
    render_hooks.draw_object_overlay = drawing_program_visual_draw_object_overlay;
    render_hooks.draw_shape_preview_overlay = lifecycle_inspect_noop_shape_overlay;
    (void)SDL_SetRenderDrawColor(renderer, 0u, 0u, 0u, 255u);
    (void)SDL_RenderClear(renderer);
    drawing_program_visual_draw_canvas_world_view(
        renderer, canvas_pane, ctx, theme_preset, &selection, &panel_ui, &interaction, &render_hooks);
    (void)SDL_RenderPresent(renderer);
    return lifecycle_inspect_sample_surface_rgba(surface, screen_x, screen_y, out_r, out_g, out_b, out_a);
}

static uint32_t lifecycle_count_nontransparent_samples(const DrawingProgramLayerRasterStore *layer_rasters,
                                                       uint32_t layer_id) {
    const DrawingProgramRasterSample *samples = 0;
    uint32_t sample_count = 0u;
    uint32_t count = 0u;
    uint32_t i;
    if (!layer_rasters ||
        drawing_program_layer_raster_store_export_layer(layer_rasters, layer_id, &samples, &sample_count).code != CORE_OK ||
        !samples) {
        return 0u;
    }
    for (i = 0u; i < sample_count; ++i) {
        if (!drawing_program_color_sample_is_transparent(samples[i])) {
            count += 1u;
        }
    }
    return count;
}

static uint32_t lifecycle_count_layer_objects(const DrawingProgramObjectStore *object_store, uint32_t layer_id) {
    uint32_t count = 0u;
    uint32_t i;
    if (!object_store) {
        return 0u;
    }
    for (i = 0u; i < object_store->object_count; ++i) {
        if (object_store->objects[i].layer_id == layer_id && object_store->objects[i].visible) {
            count += 1u;
        }
    }
    return count;
}

static uint32_t lifecycle_count_layer_samples_distinct_from_lower(const DrawingProgramLayerRasterStore *layer_rasters,
                                                                  uint32_t lower_layer_id,
                                                                  uint32_t upper_layer_id) {
    const DrawingProgramRasterSample *lower_samples = 0;
    const DrawingProgramRasterSample *upper_samples = 0;
    uint32_t lower_count = 0u;
    uint32_t upper_count = 0u;
    uint32_t count = 0u;
    uint32_t i;
    if (!layer_rasters ||
        drawing_program_layer_raster_store_export_layer(layer_rasters, lower_layer_id, &lower_samples, &lower_count).code !=
            CORE_OK ||
        drawing_program_layer_raster_store_export_layer(layer_rasters, upper_layer_id, &upper_samples, &upper_count).code !=
            CORE_OK ||
        !lower_samples || !upper_samples || lower_count != upper_count) {
        return 0u;
    }
    for (i = 0u; i < upper_count; ++i) {
        if (!drawing_program_color_sample_is_transparent(upper_samples[i]) &&
            upper_samples[i] != lower_samples[i]) {
            count += 1u;
        }
    }
    return count;
}

static void lifecycle_print_layer_inspection(const char *prefix,
                                             const DrawingProgramDocument *document,
                                             const DrawingProgramLayerRasterStore *layer_rasters,
                                             const DrawingProgramObjectStore *object_store,
                                             const DrawingProgramAppUiState *ui,
                                             uint32_t active_layer_id) {
    uint32_t i;
    if (!prefix || !document || !layer_rasters || !ui) {
        return;
    }
    fprintf(stderr,
            "%s logical=%ux%u raster=%ux%u layers=%u active_layer=%u ui_opacity_entries=%u objects=%u\n",
            prefix,
            (unsigned)document->logical_width,
            (unsigned)document->logical_height,
            (unsigned)document->raster_width,
            (unsigned)document->raster_height,
            (unsigned)document->layer_count,
            (unsigned)active_layer_id,
            (unsigned)ui->layer_opacity_entry_count,
            (unsigned)(object_store ? object_store->object_count : 0u));
    for (i = 0u; i < document->layer_count; ++i) {
        const DrawingProgramLayer *layer = &document->layers[i];
        uint8_t opacity = 100u;
        uint32_t opacity_i;
        for (opacity_i = 0u; opacity_i < ui->layer_opacity_entry_count && opacity_i < DRAWING_PROGRAM_MAX_LAYERS;
             ++opacity_i) {
            if (ui->layer_opacity_layer_ids[opacity_i] == layer->layer_id) {
                opacity = ui->layer_opacity_values[opacity_i];
                break;
            }
        }
        uint32_t distinct_from_base =
            (i > 0u && document->layer_count > 0u)
                ? lifecycle_count_layer_samples_distinct_from_lower(layer_rasters,
                                                                    document->layers[0].layer_id,
                                                                    layer->layer_id)
                : 0u;
        fprintf(stderr,
                "%s layer[%u] id=%u active=%u visible=%u locked=%u opacity=%u raster_nontransparent=%u distinct_from_base=%u visible_objects=%u name=%s\n",
                prefix,
                (unsigned)i,
                (unsigned)layer->layer_id,
                (unsigned)(layer->layer_id == active_layer_id),
                (unsigned)layer->visible,
                (unsigned)layer->locked,
                (unsigned)opacity,
                (unsigned)lifecycle_count_nontransparent_samples(layer_rasters, layer->layer_id),
                (unsigned)distinct_from_base,
                (unsigned)lifecycle_count_layer_objects(object_store, layer->layer_id),
                layer->name);
    }
}

static int lifecycle_maybe_inspect_pack(void) {
    const char *path = getenv("DRAWING_PROGRAM_INSPECT_PACK");
    static DrawingProgramAppContext inspect_ctx;
    VisualCanvasInteractionState interaction;
    VisualPanelUiState panel_ui;
    VisualPaneLayoutMetrics pane_metrics;
    SDL_Rect left_pane = { 0, 0, 0, 0 };
    SDL_Rect canvas_pane = { 0, 0, 620, 1050 };
    SDL_Rect right_pane = { 620, 0, 360, 1050 };
    SDL_Rect tab_layer;
    SDL_Rect visible_button;
    SDL_Rect opacity_row;
    SDL_Rect opacity_track;
    SDL_Surface *render_surface = 0;
    SDL_Renderer *renderer = 0;
    CoreThemePreset theme_preset;
    uint32_t active_layer_id = 0u;
    uint32_t render_sample_x = 0u;
    uint32_t render_sample_y = 0u;
    uint8_t before_visible = 0u;
    uint8_t after_visible = 0u;
    uint8_t after_opacity = 0u;
    uint8_t render_r = 0u;
    uint8_t render_g = 0u;
    uint8_t render_b = 0u;
    uint8_t render_a = 0u;
    char arg0[] = "drawing_program_test";
    char arg1[] = "--headless";
    char arg2[] = "--smoke-frames";
    char arg3[] = "1";
    char arg4[] = "--no-persist";
    char *argv[] = { arg0, arg1, arg2, arg3, arg4, 0 };
    if (!path || path[0] == '\0') {
        return 0;
    }
    if (!expect_ok(drawing_program_app_bootstrap(&inspect_ctx, 5, argv), "inspect_bootstrap") ||
        !expect_ok(drawing_program_app_config_load(&inspect_ctx), "inspect_config_load") ||
        !expect_ok(drawing_program_app_state_seed(&inspect_ctx), "inspect_state_seed") ||
        !expect_ok(drawing_program_snapshot_load(&inspect_ctx, path), "inspect_snapshot_load")) {
        return 1;
    }
    lifecycle_print_layer_inspection("inspect.document",
                                     &inspect_ctx.document,
                                     &inspect_ctx.layer_rasters,
                                     &inspect_ctx.object_store,
                                     &inspect_ctx.ui,
                                     inspect_ctx.editor.active_layer_id);
    memset(&interaction, 0, sizeof(interaction));
    memset(&panel_ui, 0, sizeof(panel_ui));
    pane_metrics = make_pane_layout_metrics(&inspect_ctx);
    tab_layer = right_panel_slot_tab_rect(right_pane,
                                          pane_metrics,
                                          VISUAL_RIGHT_PANEL_SLOT_LAYER,
                                          VISUAL_RIGHT_PANEL_SLOT_COUNT);
    (void)lifecycle_inspect_dispatch_click(&inspect_ctx,
                                           &interaction,
                                           &panel_ui,
                                           left_pane,
                                           right_pane,
                                           canvas_pane,
                                           tab_layer.x + (tab_layer.w / 2),
                                           tab_layer.y + (tab_layer.h / 2),
                                           "layer-tab");
    active_layer_id = inspect_ctx.editor.active_layer_id;
    if (active_layer_id != 0u &&
        lifecycle_inspect_find_first_distinct_layer_sample(&inspect_ctx.layer_rasters,
                                                           inspect_ctx.document.layers[0].layer_id,
                                                           active_layer_id,
                                                           &render_sample_x,
                                                           &render_sample_y) &&
        drawing_program_texture_workspace_fit_all(&inspect_ctx, canvas_pane) &&
        core_theme_get_preset(CORE_THEME_PRESET_DARK_DEFAULT, &theme_preset).code == CORE_OK) {
        render_surface = SDL_CreateRGBSurfaceWithFormat(0, 980, 1090, 32, SDL_PIXELFORMAT_RGBA8888);
        if (render_surface) {
            renderer = SDL_CreateSoftwareRenderer(render_surface);
        }
        {
            DrawingProgramRasterSample base_sample = drawing_program_color_eraser_value();
            DrawingProgramRasterSample active_sample = drawing_program_color_eraser_value();
            uint8_t base_r = 0u;
            uint8_t base_g = 0u;
            uint8_t base_b = 0u;
            uint8_t base_a = 0u;
            uint8_t active_r = 0u;
            uint8_t active_g = 0u;
            uint8_t active_b = 0u;
            uint8_t active_a = 0u;
            (void)drawing_program_layer_raster_store_raster_sample_read(&inspect_ctx.layer_rasters,
                                                                         inspect_ctx.document.layers[0].layer_id,
                                                                         render_sample_x,
                                                                         render_sample_y,
                                                                         &base_sample);
            (void)drawing_program_layer_raster_store_raster_sample_read(&inspect_ctx.layer_rasters,
                                                                         active_layer_id,
                                                                         render_sample_x,
                                                                         render_sample_y,
                                                                         &active_sample);
            drawing_program_color_rgba_from_sample(base_sample, &base_r, &base_g, &base_b, &base_a);
            drawing_program_color_rgba_from_sample(active_sample, &active_r, &active_g, &active_b, &active_a);
            fprintf(stderr,
                    "inspect.samples sample=%u,%u base=%u,%u,%u,%u active=%u,%u,%u,%u raw_base=%08x raw_active=%08x\n",
                    (unsigned)render_sample_x,
                    (unsigned)render_sample_y,
                    (unsigned)base_r,
                    (unsigned)base_g,
                    (unsigned)base_b,
                    (unsigned)base_a,
                    (unsigned)active_r,
                    (unsigned)active_g,
                    (unsigned)active_b,
                    (unsigned)active_a,
                    (unsigned)base_sample,
                    (unsigned)active_sample);
        }
        drawing_program_visual_surface_cache_shutdown();
        if (renderer && lifecycle_inspect_render_active_sample(renderer,
                                                               render_surface,
                                                               canvas_pane,
                                                               &inspect_ctx,
                                                               &theme_preset,
                                                               render_sample_x,
                                                               render_sample_y,
                                                               &render_r,
                                                               &render_g,
                                                               &render_b,
                                                               &render_a)) {
            fprintf(stderr,
                    "inspect.render initial sample=%u,%u rgba=%u,%u,%u,%u\n",
                    (unsigned)render_sample_x,
                    (unsigned)render_sample_y,
                    (unsigned)render_r,
                    (unsigned)render_g,
                    (unsigned)render_b,
                    (unsigned)render_a);
        } else {
            fprintf(stderr,
                    "inspect.render initial failed sample=%u,%u\n",
                    (unsigned)render_sample_x,
                    (unsigned)render_sample_y);
        }
    }
    if (active_layer_id != 0u) {
        uint32_t active_index = 0u;
        if (drawing_program_document_layer_index_for_id(&inspect_ctx.document, active_layer_id, &active_index).code ==
                CORE_OK &&
            active_index < inspect_ctx.document.layer_count) {
            before_visible = inspect_ctx.document.layers[active_index].visible;
        }
    }
    opacity_row = right_layer_opacity_row_rect(right_pane, pane_metrics, inspect_ctx.document.layer_count);
    opacity_track = right_layer_opacity_track_rect(opacity_row, pane_metrics);
    (void)lifecycle_inspect_dispatch_click(&inspect_ctx,
                                           &interaction,
                                           &panel_ui,
                                           left_pane,
                                           right_pane,
                                           canvas_pane,
                                           opacity_track.x + (opacity_track.w / 4),
                                           opacity_track.y + (opacity_track.h / 2),
                                           "opacity-25");
    after_opacity = drawing_program_visual_layer_opacity_get(&inspect_ctx, active_layer_id);
    if (renderer && lifecycle_inspect_render_active_sample(renderer,
                                                           render_surface,
                                                           canvas_pane,
                                                           &inspect_ctx,
                                                           &theme_preset,
                                                           render_sample_x,
                                                           render_sample_y,
                                                           &render_r,
                                                           &render_g,
                                                           &render_b,
                                                           &render_a)) {
        fprintf(stderr,
                "inspect.render opacity sample=%u,%u rgba=%u,%u,%u,%u\n",
                (unsigned)render_sample_x,
                (unsigned)render_sample_y,
                (unsigned)render_r,
                (unsigned)render_g,
                (unsigned)render_b,
                (unsigned)render_a);
    }
    visible_button = right_layer_action_button_rect(right_pane,
                                                    pane_metrics,
                                                    inspect_ctx.document.layer_count,
                                                    VISUAL_LAYER_ACTION_TOGGLE_VISIBLE);
    (void)lifecycle_inspect_dispatch_click(&inspect_ctx,
                                           &interaction,
                                           &panel_ui,
                                           left_pane,
                                           right_pane,
                                           canvas_pane,
                                           visible_button.x + (visible_button.w / 2),
                                           visible_button.y + (visible_button.h / 2),
                                           "visible-toggle");
    if (active_layer_id != 0u) {
        uint32_t active_index = 0u;
        if (drawing_program_document_layer_index_for_id(&inspect_ctx.document, active_layer_id, &active_index).code ==
                CORE_OK &&
            active_index < inspect_ctx.document.layer_count) {
            after_visible = inspect_ctx.document.layers[active_index].visible;
        }
    }
    if (renderer && lifecycle_inspect_render_active_sample(renderer,
                                                           render_surface,
                                                           canvas_pane,
                                                           &inspect_ctx,
                                                           &theme_preset,
                                                           render_sample_x,
                                                           render_sample_y,
                                                           &render_r,
                                                           &render_g,
                                                           &render_b,
                                                           &render_a)) {
        fprintf(stderr,
                "inspect.render hidden sample=%u,%u rgba=%u,%u,%u,%u\n",
                (unsigned)render_sample_x,
                (unsigned)render_sample_y,
                (unsigned)render_r,
                (unsigned)render_g,
                (unsigned)render_b,
                (unsigned)render_a);
    }
    fprintf(stderr,
            "inspect.after_clicks active_layer=%u right_slot=%u visible_before=%u visible_after=%u opacity_after=%u "
            "surface_epoch=%llu active_revision=%llu opacity_revision=%llu\n",
            (unsigned)active_layer_id,
            (unsigned)inspect_ctx.ui.right_panel_slot,
            (unsigned)before_visible,
            (unsigned)after_visible,
            (unsigned)after_opacity,
            (unsigned long long)inspect_ctx.texture_project.runtime_cache_epoch,
            (unsigned long long)inspect_ctx.runtime.render_active_surface_content_revision,
            (unsigned long long)inspect_ctx.runtime.render_layer_opacity_revision);
    lifecycle_print_layer_inspection("inspect.after_document",
                                     &inspect_ctx.document,
                                     &inspect_ctx.layer_rasters,
                                     &inspect_ctx.object_store,
                                     &inspect_ctx.ui,
                                     inspect_ctx.editor.active_layer_id);
    fprintf(stderr,
            "inspect.texture_project surfaces=%u active_surface_index=%u\n",
            (unsigned)inspect_ctx.texture_project.surface_count,
            (unsigned)inspect_ctx.texture_project.active_surface_index);
    if (inspect_ctx.texture_project.surface_count > 0u) {
        const DrawingProgramTextureSurface *surface = drawing_program_texture_project_surface_at(
            &inspect_ctx.texture_project, inspect_ctx.texture_project.active_surface_index);
        if (surface && surface->storage) {
            DrawingProgramAppUiState surface_ui = inspect_ctx.ui;
            surface_ui.layer_opacity_entry_count = surface->layer_opacity_entry_count;
            memcpy(surface_ui.layer_opacity_values, surface->layer_opacity_values, sizeof(surface_ui.layer_opacity_values));
            memcpy(surface_ui.layer_opacity_layer_ids,
                   surface->layer_opacity_layer_ids,
                   sizeof(surface_ui.layer_opacity_layer_ids));
            fprintf(stderr,
                    "inspect.active_surface id=%u name=%s is_blank=%u opacity_entries=%u\n",
                    (unsigned)surface->surface_id,
                    surface->name,
                    (unsigned)surface->is_blank,
                    (unsigned)surface->layer_opacity_entry_count);
            lifecycle_print_layer_inspection("inspect.surface",
                                             &surface->storage->document,
                                             &surface->storage->layer_rasters,
                                             &inspect_ctx.object_store,
                                             &surface_ui,
                                             inspect_ctx.editor.active_layer_id);
        }
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (render_surface) {
        SDL_FreeSurface(render_surface);
    }
    drawing_program_visual_surface_cache_shutdown();
    drawing_program_app_shutdown(&inspect_ctx);
    return 1;
}

typedef int (*LifecycleStandaloneSuiteFn)(void);

typedef struct LifecycleStandaloneSuiteEntry {
    const char *name;
    const char *description;
    LifecycleStandaloneSuiteFn run;
} LifecycleStandaloneSuiteEntry;

static int lifecycle_run_test_support_contract_suite(void) {
    char artifact_path[512];
    if (!lifecycle_test_artifact_path(artifact_path, sizeof(artifact_path), "r5s1_test_support_probe.tmp")) {
        return 1;
    }
    if (strstr(artifact_path, "r5s1_test_support_probe.tmp") == 0) {
        fprintf(stderr, "lifecycle_test: expected artifact leaf in path got=%s\n", artifact_path);
        return 1;
    }
    return 0;
}

static const LifecycleStandaloneSuiteEntry g_lifecycle_standalone_suites[] = {
    { "test-support", "test support artifact-root contract", lifecycle_run_test_support_contract_suite },
    { "export", "image/export model contracts", drawing_program_lifecycle_run_export_suite },
    { "composed-source", "composed source cache/dirty-rect contracts", drawing_program_lifecycle_run_composed_source_suite },
    { "composed-source-rcp1", "RCP1 composed source contracts", drawing_program_lifecycle_run_composed_source_rcp1_suite },
    { "composed-source-rws1", "RWS1 composed source contracts", drawing_program_lifecycle_run_composed_source_rws1_suite },
    { "persistence", "root/session persistence contracts", drawing_program_lifecycle_run_persistence_contract_suite },
    { "render-domain", "render-domain model contracts", drawing_program_lifecycle_run_render_domain_suite },
    { "snapshot-layer", "snapshot layer fixture/root contracts", drawing_program_lifecycle_run_snapshot_layer_suite },
    { "texture-export", "authored texture export contracts", drawing_program_lifecycle_run_texture_export_suite },
    { "texture-import", "authored texture import contracts", drawing_program_lifecycle_run_texture_import_suite },
    { "surface-cache", "surface-cache contract checks", drawing_program_lifecycle_run_surface_cache_contract_suite },
    { "authoring-host", "workspace authoring host contracts", drawing_program_lifecycle_run_authoring_host_suite },
};

static size_t lifecycle_standalone_suite_count(void) {
    return sizeof(g_lifecycle_standalone_suites) / sizeof(g_lifecycle_standalone_suites[0]);
}

static void lifecycle_print_suite_list(FILE *stream) {
    size_t i;
    fprintf(stream, "available lifecycle suites:\n");
    fprintf(stream, "  all - full lifecycle suite\n");
    for (i = 0u; i < lifecycle_standalone_suite_count(); ++i) {
        fprintf(stream,
                "  %s - %s\n",
                g_lifecycle_standalone_suites[i].name,
                g_lifecycle_standalone_suites[i].description);
    }
}

static int lifecycle_run_named_standalone_suite(const char *suite_name) {
    size_t i;
    if (!suite_name || !suite_name[0] || strcmp(suite_name, "all") == 0) {
        return -1;
    }
    for (i = 0u; i < lifecycle_standalone_suite_count(); ++i) {
        if (strcmp(suite_name, g_lifecycle_standalone_suites[i].name) == 0) {
            return g_lifecycle_standalone_suites[i].run();
        }
    }
    fprintf(stderr, "lifecycle_test: unknown suite '%s'\n", suite_name);
    lifecycle_print_suite_list(stderr);
    return 2;
}

static int lifecycle_parse_selected_suite(int argc, char **test_argv, const char **out_suite_name) {
    int i;
    if (!out_suite_name) {
        return 2;
    }
    *out_suite_name = 0;
    for (i = 1; i < argc; ++i) {
        if (strcmp(test_argv[i], "--list-suites") == 0) {
            lifecycle_print_suite_list(stdout);
            return 1;
        }
        if (strcmp(test_argv[i], "--suite") == 0) {
            if (i + 1 >= argc || !test_argv[i + 1][0]) {
                fprintf(stderr, "lifecycle_test: missing value for --suite\n");
                return 2;
            }
            *out_suite_name = test_argv[i + 1];
            i += 1;
            continue;
        }
        fprintf(stderr, "lifecycle_test: unknown test option '%s'\n", test_argv[i]);
        lifecycle_print_suite_list(stderr);
        return 2;
    }
    return 0;
}

int main(int argc, char **test_argv) {
    static DrawingProgramAppContext ctx;
    static DrawingProgramAppContext workflow_ctx;
    static DrawingProgramAppContext size_ctx;
    static DrawingProgramAppContext bad_arg_ctx;
    static DrawingProgramClipboardState workflow_clipboard;
    CoreResult bad_arg_result;
    uint32_t center_x;
    uint32_t center_y;
    uint32_t workflow_center_x;
    uint32_t workflow_center_y;
    uint8_t workflow_center_value = 0u;
    uint8_t center_before = 0u;
    char arg0[] = "drawing_program_test";
    char arg1[] = "--headless";
    char arg2[] = "--smoke-frames";
    char arg3[] = "2";
    char arg4[] = "--no-persist";
    char arg5[] = "--canvas-size";
    char arg6[] = "640x360";
    char bad_arg1[] = "--definitely-unknown";
    char *argv[] = { arg0, arg1, arg2, arg3, arg4, 0 };
    char *size_argv[] = { arg0, arg1, arg2, arg3, arg4, arg5, arg6, 0 };
    char *unknown_argv[] = { arg0, bad_arg1, 0 };
    char *missing_canvas_size_argv[] = { arg0, arg5, 0 };
    uint8_t expected_draw_value = 0u;
    uint8_t expected_eraser_value =
        drawing_program_color_legacy_sample_from_sample(drawing_program_color_eraser_value());
    const char *selected_suite = 0;
    int parse_result = lifecycle_parse_selected_suite(argc, test_argv, &selected_suite);
    int suite_result = 0;
    drawing_program_clipboard_reset(&workflow_clipboard);

    if (parse_result == 1) {
        return 0;
    }
    if (parse_result != 0) {
        return parse_result;
    }
    if (selected_suite && strcmp(selected_suite, "all") != 0) {
        suite_result = lifecycle_run_named_standalone_suite(selected_suite);
        return suite_result < 0 ? 2 : suite_result;
    }

    if (lifecycle_maybe_inspect_pack() != 0) {
        return 0;
    }

    bad_arg_result = drawing_program_app_bootstrap(&bad_arg_ctx, 2, unknown_argv);
    if (bad_arg_result.code != CORE_ERR_INVALID_ARG ||
        !bad_arg_result.message ||
        strstr(bad_arg_result.message, "unknown command-line option: --definitely-unknown") == 0) {
        fprintf(stderr,
                "lifecycle_test: expected unknown option diagnostic got code=%d message=%s\n",
                (int)bad_arg_result.code,
                bad_arg_result.message ? bad_arg_result.message : "(null)");
        return 1;
    }
    bad_arg_result = drawing_program_app_bootstrap(&bad_arg_ctx, 2, missing_canvas_size_argv);
    if (bad_arg_result.code != CORE_ERR_INVALID_ARG ||
        !bad_arg_result.message ||
        strstr(bad_arg_result.message, "missing value for --canvas-size") == 0) {
        fprintf(stderr,
                "lifecycle_test: expected missing option value diagnostic got code=%d message=%s\n",
                (int)bad_arg_result.code,
                bad_arg_result.message ? bad_arg_result.message : "(null)");
        return 1;
    }

    if (!expect_ok(drawing_program_app_bootstrap(&ctx, 5, argv), "bootstrap")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&ctx), "config_load")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&ctx), "state_seed")) {
        return 1;
    }
    if (ctx.document.logical_width != DRAWING_PROGRAM_DEFAULT_LOGICAL_WIDTH ||
        ctx.document.logical_height != DRAWING_PROGRAM_DEFAULT_LOGICAL_HEIGHT ||
        ctx.document.raster_width != DRAWING_PROGRAM_DEFAULT_LOGICAL_WIDTH ||
        ctx.document.raster_height != DRAWING_PROGRAM_DEFAULT_LOGICAL_HEIGHT) {
        fprintf(stderr,
                "lifecycle_test: expected default canvas seed %ux%u got logical=%ux%u raster=%ux%u\n",
                (unsigned)DRAWING_PROGRAM_DEFAULT_LOGICAL_WIDTH,
                (unsigned)DRAWING_PROGRAM_DEFAULT_LOGICAL_HEIGHT,
                (unsigned)ctx.document.logical_width,
                (unsigned)ctx.document.logical_height,
                (unsigned)ctx.document.raster_width,
                (unsigned)ctx.document.raster_height);
        return 1;
    }
    if (!expect_ok(drawing_program_app_bootstrap(&workflow_ctx, 5, argv), "workflow_bootstrap")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&workflow_ctx), "workflow_config_load")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&workflow_ctx), "workflow_state_seed")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&workflow_ctx), "workflow_subsystems_init")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&workflow_ctx), "workflow_runtime_start")) {
        return 1;
    }
    expected_draw_value = drawing_program_ui_color_active_paint_sample_value(&workflow_ctx);
    if (!expect_ok(drawing_program_app_bootstrap(&size_ctx, 7, size_argv), "size_bootstrap")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&size_ctx), "size_config_load")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&size_ctx), "size_state_seed")) {
        return 1;
    }
    if (size_ctx.document.logical_width != 640u ||
        size_ctx.document.logical_height != 360u ||
        size_ctx.document.raster_width != 640u ||
        size_ctx.document.raster_height != 360u) {
        fprintf(stderr,
                "lifecycle_test: expected non-square canvas seed 640x360 got logical=%ux%u raster=%ux%u\n",
                (unsigned)size_ctx.document.logical_width,
                (unsigned)size_ctx.document.logical_height,
                (unsigned)size_ctx.document.raster_width,
                (unsigned)size_ctx.document.raster_height);
        return 1;
    }
    workflow_center_x = workflow_ctx.document.raster_width / 2u;
    workflow_center_y = workflow_ctx.document.raster_height / 2u;
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_ERASER),
                   "workflow_set_tool_eraser")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE),
                   "workflow_stamp_eraser")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document,
                                                        workflow_center_x,
                                                        workflow_center_y,
                                                        &workflow_center_value),
                   "workflow_sample_after_eraser_stamp")) {
        return 1;
    }
    if (workflow_center_value != expected_eraser_value) {
        fprintf(stderr,
                "lifecycle_test: expected workflow eraser stamp to set center sample to %u got=%u\n",
                (unsigned)expected_eraser_value,
                (unsigned)workflow_center_value);
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_BRUSH),
                   "workflow_set_tool_brush")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &workflow_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE),
                   "workflow_stamp_brush")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&workflow_ctx.document,
                                                        workflow_center_x,
                                                        workflow_center_y,
                                                        &workflow_center_value),
                   "workflow_sample_after_brush_stamp")) {
        return 1;
    }
    expected_draw_value = drawing_program_ui_color_active_paint_sample_value(&workflow_ctx);
    if (workflow_center_value != expected_draw_value) {
        fprintf(stderr,
                "lifecycle_test: expected workflow brush stamp to set center sample to %u got=%u\n",
                (unsigned)expected_draw_value,
                (unsigned)workflow_center_value);
        return 1;
    }
    if (drawing_program_lifecycle_run_selection_layer_suite(&workflow_ctx,
                                                            &workflow_clipboard,
                                                            argv,
                                                            workflow_center_x,
                                                            workflow_center_y,
                                                            expected_draw_value,
                                                            expected_eraser_value) != 0) {
        return 1;
    }
    if (workflow_ctx.runtime.tool_switch_total != 2u) {
        fprintf(stderr,
                "lifecycle_test: expected workflow tool switch total=2 got=%llu\n",
                (unsigned long long)workflow_ctx.runtime.tool_switch_total);
        return 1;
    }
    if (drawing_program_lifecycle_run_baseline_history_suite(&ctx,
                                                             expected_eraser_value,
                                                             &center_x,
                                                             &center_y,
                                                             &center_before) != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_snapshot_suite(&ctx) != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_export_suite() != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_composed_source_suite() != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_composed_source_rcp1_suite() != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_composed_source_rws1_suite() != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_persistence_contract_suite() != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_render_domain_suite() != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_texture_export_suite() != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_texture_import_suite() != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_object_path_suite(&workflow_ctx,
                                                        expected_draw_value,
                                                        expected_eraser_value) != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_runtime_render_suite(&ctx,
                                                           &workflow_ctx,
                                                           center_x,
                                                           center_y,
                                                           expected_draw_value,
                                                           expected_eraser_value) != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_surface_cache_contract_suite() != 0) {
        return 1;
    }
    if (drawing_program_lifecycle_run_authoring_host_suite() != 0) {
        return 1;
    }
    return 0;
}
