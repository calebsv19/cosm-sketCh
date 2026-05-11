#include "drawing_program/drawing_program_visual_right_panel_render.h"

#include <stdio.h>

#include "drawing_program/drawing_program_canvas_reflection.h"
#include "drawing_program/drawing_program_object_store.h"
#include "drawing_program/drawing_program_texture_net_guides.h"
#include "drawing_program/drawing_program_texture_project.h"
#include "drawing_program/drawing_program_visual_layer_roles.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_panel_render_common.h"
#include "drawing_program/drawing_program_visual_right_panel_defs.h"
#include "drawing_program/drawing_program_visual_right_panel_color_render.h"
#include "drawing_program/drawing_program_visual_right_panel_file_tabs_render.h"
#include "drawing_program/drawing_program_visual_theme.h"

void drawing_program_visual_render_right_panel_chrome(SDL_Renderer *renderer,
                                                      SDL_Rect rect,
                                                      const DrawingProgramAppContext *ctx,
                                                      const CoreThemePreset *theme,
                                                      const VisualPanelUiState *ui,
                                                      const VisualSelectionState *selection,
                                                      const VisualCanvasInteractionState *interaction,
                                                      const DrawingProgramVisualPanelRenderHooks *hooks) {
    char line[160];
    uint8_t right_slot;
    VisualPaneLayoutMetrics m;
    VisualThemePalette p;
    SDL_Rect tab_canvas;
    SDL_Rect tab_layer;
    SDL_Rect tab_color;
    SDL_Rect tab_file;
    SDL_Rect tab_asset;
    SDL_Rect tab_export;
    SDL_Rect row;
    int y;
    uint8_t active_visible = 0u;
    uint8_t active_locked = 0u;
    uint8_t active_opacity = 100u;
    uint32_t i;
    if (!renderer || !ctx || !hooks || !hooks->draw_bitmap_text || !hooks->clamp_right_slot || !hooks->tool_name ||
        !hooks->active_layer_allows_edits_visual || !hooks->tool_brush_radius_samples ||
        !hooks->tool_brush_spacing_samples || !hooks->tool_uses_shape_commit || !hooks->clamp_setting_u8 ||
        !hooks->shape_mode_name || !hooks->tool_shape_mode || !hooks->tool_fill_tolerance_setting ||
        !hooks->fill_tolerance_sample_delta || !hooks->selection_component_count || !hooks->pane_rect_for_module_type ||
        !hooks->color_index_clamp || !hooks->color_rgb_from_index) {
        return;
    }
    (void)interaction;
    right_slot = hooks->clamp_right_slot(ctx->ui.right_panel_slot);
    m = make_pane_layout_metrics(ctx);
    resolve_visual_theme_palette(theme, &p);

    for (i = 0u; i < ctx->document.layer_count; ++i) {
        if (ctx->document.layers[i].layer_id == ctx->editor.active_layer_id) {
            active_visible = ctx->document.layers[i].visible;
            active_locked = ctx->document.layers[i].locked;
            active_opacity = drawing_program_visual_layer_opacity_get(ctx, ctx->editor.active_layer_id);
            break;
        }
    }

    y = rect.y + m.pad_y;
    hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "RIGHT PANEL", p.text_primary, m.title_scale);
    y += m.title_glyph_h + m.section_gap;
    tab_canvas = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_CANVAS, VISUAL_RIGHT_PANEL_SLOT_COUNT);
    tab_layer = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_LAYER, VISUAL_RIGHT_PANEL_SLOT_COUNT);
    tab_color = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_COLOR, VISUAL_RIGHT_PANEL_SLOT_COUNT);
    tab_file = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_FILE, VISUAL_RIGHT_PANEL_SLOT_COUNT);
    tab_asset = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_ASSET, VISUAL_RIGHT_PANEL_SLOT_COUNT);
    tab_export = right_panel_slot_tab_rect(rect, m, VISUAL_RIGHT_PANEL_SLOT_EXPORT, VISUAL_RIGHT_PANEL_SLOT_COUNT);
    drawing_program_visual_panel_draw_tab_button(renderer,
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
                                                 drawing_program_visual_panel_ui_hovered(ui, tab_canvas, hooks),
                                                 hooks);
    drawing_program_visual_panel_draw_tab_button(renderer,
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
                                                 drawing_program_visual_panel_ui_hovered(ui, tab_layer, hooks),
                                                 hooks);
    drawing_program_visual_panel_draw_tab_button(renderer,
                                                 rect,
                                                 tab_color,
                                                 "COLOR",
                                                 p.button_fill,
                                                 p.button_fill_hover,
                                                 p.button_fill_active,
                                                 p.button_border,
                                                 (right_slot == VISUAL_RIGHT_PANEL_SLOT_COLOR) ? p.text_primary : p.text_muted,
                                                 m.body_scale,
                                                 right_slot == VISUAL_RIGHT_PANEL_SLOT_COLOR,
                                                 drawing_program_visual_panel_ui_hovered(ui, tab_color, hooks),
                                                 hooks);
    drawing_program_visual_panel_draw_tab_button(renderer,
                                                 rect,
                                                 tab_file,
                                                 "FILE",
                                                 p.button_fill,
                                                 p.button_fill_hover,
                                                 p.button_fill_active,
                                                 p.button_border,
                                                 (right_slot == VISUAL_RIGHT_PANEL_SLOT_FILE) ? p.text_primary : p.text_muted,
                                                 m.body_scale,
                                                 right_slot == VISUAL_RIGHT_PANEL_SLOT_FILE,
                                                 drawing_program_visual_panel_ui_hovered(ui, tab_file, hooks),
                                                 hooks);
    drawing_program_visual_panel_draw_tab_button(renderer,
                                                 rect,
                                                 tab_asset,
                                                 "ASSET",
                                                 p.button_fill,
                                                 p.button_fill_hover,
                                                 p.button_fill_active,
                                                 p.button_border,
                                                 (right_slot == VISUAL_RIGHT_PANEL_SLOT_ASSET) ? p.text_primary : p.text_muted,
                                                 m.body_scale,
                                                 right_slot == VISUAL_RIGHT_PANEL_SLOT_ASSET,
                                                 drawing_program_visual_panel_ui_hovered(ui, tab_asset, hooks),
                                                 hooks);
    drawing_program_visual_panel_draw_tab_button(renderer,
                                                 rect,
                                                 tab_export,
                                                 "EXPORT",
                                                 p.button_fill,
                                                 p.button_fill_hover,
                                                 p.button_fill_active,
                                                 p.button_border,
                                                 (right_slot == VISUAL_RIGHT_PANEL_SLOT_EXPORT) ? p.text_primary
                                                                                               : p.text_muted,
                                                 m.body_scale,
                                                 right_slot == VISUAL_RIGHT_PANEL_SLOT_EXPORT,
                                                 drawing_program_visual_panel_ui_hovered(ui, tab_export, hooks),
                                                 hooks);
    y += m.tab_h + m.section_gap;

    if (right_slot == VISUAL_RIGHT_PANEL_SLOT_CANVAS) {
        const DrawingProgramTextureSurface *active_surface =
            drawing_program_texture_project_surface_at(&ctx->texture_project, ctx->texture_project.active_surface_index);
        SDL_Rect add_surface_button;
        SDL_Rect duplicate_surface_button;
        SDL_Rect canvas_mode_button;
        SDL_Rect canvas_guide_button;
        SDL_Rect reflect_horizontal_button;
        SDL_Rect reflect_vertical_button;
        SDL_Rect center_pick_button;
        SDL_Rect center_reset_button;
        SDL_Rect delete_canvas_button;
        SDL_Rect reset_layout_button;
        SDL_Rect reset_view_button;
        SDL_Rect clear_canvas_button;
        SDL_Rect clear_objects_button;
        SDL_Rect delete_selection_button;
        SDL_Rect clear_history_button;
        uint32_t surface_index = 0u;
        uint32_t brush_radius = hooks->tool_brush_radius_samples(ctx, ctx->editor.active_tool);
        uint32_t brush_spacing = hooks->tool_brush_spacing_samples(ctx, ctx->editor.active_tool, brush_radius);
        uint32_t reflection_center_x = 0u;
        uint32_t reflection_center_y = 0u;
        int delete_selection_enabled = (selection &&
                                        selection->has_payload &&
                                        hooks->active_layer_allows_edits_visual(ctx))
                                           ? 1
                                           : 0;
        (void)drawing_program_canvas_reflection_active_center(ctx, &reflection_center_x, &reflection_center_y);
        y = right_canvas_metrics_start_y(rect, m);
        if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_BRUSH) {
            (void)snprintf(line,
                           sizeof(line),
                           "TOOL %s  MODE %s  R%u S%u O%u%% H%u%%",
                           hooks->tool_name(ctx->editor.active_tool),
                           (ctx->ui.canvas_control_mode == (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT)
                               ? "LAYOUT"
                               : "PAINT",
                           brush_radius,
                           brush_spacing,
                           (unsigned)hooks->clamp_setting_u8(ctx->ui.tool_brush_opacity, 1u, 100u),
                           (unsigned)hooks->clamp_setting_u8(ctx->ui.tool_brush_hardness, 1u, 100u));
        } else if (ctx->editor.active_tool == DRAWING_PROGRAM_TOOL_ERASER) {
            (void)snprintf(line,
                           sizeof(line),
                           "TOOL %s  MODE %s  R%u S%u",
                           hooks->tool_name(ctx->editor.active_tool),
                           (ctx->ui.canvas_control_mode == (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT)
                               ? "LAYOUT"
                               : "PAINT",
                           brush_radius,
                           brush_spacing);
        } else if (hooks->tool_uses_shape_commit(ctx->editor.active_tool)) {
            (void)snprintf(line,
                           sizeof(line),
                           "TOOL %s  CANVAS %s  W%u %s",
                           hooks->tool_name(ctx->editor.active_tool),
                           (ctx->ui.canvas_control_mode == (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT)
                               ? "LAYOUT"
                               : "PAINT",
                           (unsigned)hooks->clamp_setting_u8(ctx->ui.tool_shape_stroke_width, 1u, 16u),
                           hooks->shape_mode_name(hooks->tool_shape_mode(ctx)));
        } else {
            (void)snprintf(line,
                           sizeof(line),
                           "TOOL %s  MODE %s",
                           hooks->tool_name(ctx->editor.active_tool),
                           (ctx->ui.canvas_control_mode == (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT)
                               ? "LAYOUT"
                               : "PAINT");
        }
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        (void)snprintf(line,
                       sizeof(line),
                       "SURFACES %u  ACTIVE %u/%u",
                       (unsigned)ctx->texture_project.surface_count,
                       (unsigned)(ctx->texture_project.active_surface_index + 1u),
                       (unsigned)(ctx->texture_project.surface_count > 0u ? ctx->texture_project.surface_count : 1u));
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        if (active_surface && active_surface->storage) {
            (void)snprintf(line,
                           sizeof(line),
                           "ACTIVE %s %ux%u %s",
                           drawing_program_texture_project_face_role_name(active_surface->face_role),
                           (unsigned)active_surface->storage->document.raster_width,
                           (unsigned)active_surface->storage->document.raster_height,
                           drawing_program_texture_project_quality_preset_name(active_surface->quality_preset));
        } else {
            (void)snprintf(line, sizeof(line), "ACTIVE SURFACE n/a");
        }
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        if (active_surface && active_surface->semantic.layout_kind != DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_NONE) {
            (void)snprintf(line,
                           sizeof(line),
                           "NET %s %s %s",
                           drawing_program_texture_net_layout_kind_name(active_surface->semantic.layout_kind),
                           drawing_program_texture_net_slot_name(active_surface->semantic.net_slot),
                           drawing_program_texture_net_orientation_name(active_surface->semantic.orientation));
            hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
            y += m.line_h;
        }
        (void)snprintf(line,
                       sizeof(line),
                       "GUIDES %s  REFLECT %c/%c @ %u,%u  VIEW Z%.2fx PAN %d,%d",
                       drawing_program_ui_canvas_guide_mode_name(ctx->ui.canvas_guide_mode),
                       ctx->editor.symmetry_horizontal ? 'H' : '-',
                       ctx->editor.symmetry_vertical ? 'V' : '-',
                       (unsigned)reflection_center_x,
                       (unsigned)reflection_center_y,
                       (double)ctx->editor.viewport.zoom,
                       (int)ctx->editor.viewport.pan_x,
                       (int)ctx->editor.viewport.pan_y);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
        y += m.line_h;
        if (ctx->texture_project.source_object_id[0] != '\0') {
            const char *primitive_name = "none";
            switch (ctx->texture_project.primitive_kind) {
                case DRAWING_PROGRAM_TEXTURE_PRIMITIVE_KIND_PLANE: primitive_name = "plane"; break;
                case DRAWING_PROGRAM_TEXTURE_PRIMITIVE_KIND_RECT_PRISM: primitive_name = "rect_prism"; break;
                default: break;
            }
            if (ctx->texture_project.source_scene_id[0] != '\0') {
                (void)snprintf(line,
                               sizeof(line),
                               "SOURCE %s / %s", ctx->texture_project.source_object_id, primitive_name);
            } else {
                (void)snprintf(line,
                               sizeof(line),
                               "SOURCE %s %s", ctx->texture_project.source_object_id, primitive_name);
            }
            hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, line, p.text_muted, m.body_scale);
            y += m.line_h;
        }
        add_surface_button = right_canvas_add_surface_button_rect(rect, m);
        duplicate_surface_button = right_canvas_duplicate_surface_button_rect(rect, m);
        canvas_mode_button = right_canvas_mode_toggle_button_rect(rect, m);
        canvas_guide_button = right_canvas_guide_mode_button_rect(rect, m);
        reflect_horizontal_button = right_canvas_reflect_horizontal_button_rect(rect, m);
        reflect_vertical_button = right_canvas_reflect_vertical_button_rect(rect, m);
        center_pick_button = right_canvas_center_pick_button_rect(rect, m);
        center_reset_button = right_canvas_center_reset_button_rect(rect, m);
        delete_canvas_button = right_canvas_delete_canvas_button_rect(rect, m);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     add_surface_button,
                                                     "ADD CANVAS",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     0,
                                                     drawing_program_visual_panel_ui_hovered(ui, add_surface_button, hooks),
                                                     hooks);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     duplicate_surface_button,
                                                     "DUPLICATE CANVAS",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     0,
                                                     drawing_program_visual_panel_ui_hovered(ui, duplicate_surface_button, hooks),
                                                     hooks);
        drawing_program_visual_panel_draw_tab_button(
            renderer,
            rect,
            canvas_mode_button,
            (ctx->ui.canvas_control_mode == (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT)
                ? "CANVAS MODE: LAYOUT"
                : "CANVAS MODE: PAINT",
            p.button_fill,
            p.button_fill_hover,
            p.button_fill_active,
            p.button_border,
            p.text_primary,
            m.body_scale,
            ctx->ui.canvas_control_mode == (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT,
            drawing_program_visual_panel_ui_hovered(ui, canvas_mode_button, hooks),
            hooks);
        drawing_program_visual_panel_draw_tab_button(
            renderer,
            rect,
            canvas_guide_button,
            (ctx->ui.canvas_guide_mode == (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_OFF)
                ? "GUIDES: OFF"
                : ((ctx->ui.canvas_guide_mode == (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_CORNERS)
                       ? "GUIDES: CORNERS"
                       : "GUIDES: CORNERS+EDGES"),
            p.button_fill,
            p.button_fill_hover,
            p.button_fill_active,
            p.button_border,
            p.text_primary,
            m.body_scale,
            ctx->ui.canvas_guide_mode != (uint8_t)DRAWING_PROGRAM_UI_CANVAS_GUIDE_MODE_OFF,
            drawing_program_visual_panel_ui_hovered(ui, canvas_guide_button, hooks),
            hooks);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     reflect_horizontal_button,
                                                     "REFLECT H",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     ctx->editor.symmetry_horizontal ? 1 : 0,
                                                     drawing_program_visual_panel_ui_hovered(ui,
                                                                                            reflect_horizontal_button,
                                                                                            hooks),
                                                     hooks);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     reflect_vertical_button,
                                                     "REFLECT V",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     ctx->editor.symmetry_vertical ? 1 : 0,
                                                     drawing_program_visual_panel_ui_hovered(ui,
                                                                                            reflect_vertical_button,
                                                                                            hooks),
                                                     hooks);
        drawing_program_visual_panel_draw_tab_button(renderer,
                                                     rect,
                                                     center_pick_button,
                                                     (ui && ui->right_canvas_reflection_center_pick_pending)
                                                         ? "PICK CENTER"
                                                         : "SET CENTER",
                                                     p.button_fill,
                                                     p.button_fill_hover,
                                                     p.button_fill_active,
                                                     p.button_border,
                                                     p.text_primary,
                                                     m.body_scale,
                                                     (ui && ui->right_canvas_reflection_center_pick_pending) ? 1 : 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, center_pick_button, hooks),
                                                     hooks);
        {
            char center_label[64];
            (void)snprintf(center_label,
                           sizeof(center_label),
                           "RESET %u,%u",
                           (unsigned)reflection_center_x,
                           (unsigned)reflection_center_y);
            drawing_program_visual_panel_draw_tab_button(renderer,
                                                         rect,
                                                         center_reset_button,
                                                         center_label,
                                                         p.button_fill,
                                                         p.button_fill_hover,
                                                         p.button_fill_active,
                                                         p.button_border,
                                                         p.text_primary,
                                                         m.body_scale,
                                                         0,
                                                         drawing_program_visual_panel_ui_hovered(ui,
                                                                                                center_reset_button,
                                                                                                hooks),
                                                         hooks);
        }
        {
            int delete_canvas_armed =
                ui &&
                ui->right_canvas_delete_confirm_pending &&
                ui->right_canvas_delete_confirm_surface_index == ctx->texture_project.active_surface_index;
            uint8_t delete_fill_r = (p.button_fill_active.r > 227u) ? 255u : (uint8_t)(p.button_fill_active.r + 28u);
            uint8_t delete_hover_r = (p.button_fill_hover.r > 219u) ? 255u : (uint8_t)(p.button_fill_hover.r + 36u);
            SDL_Color delete_fill = delete_canvas_armed
                                        ? (SDL_Color){ delete_fill_r,
                                                       p.button_fill_active.g,
                                                       p.button_fill_active.b,
                                                       p.button_fill_active.a }
                                        : p.button_fill;
            SDL_Color delete_hover = delete_canvas_armed
                                         ? (SDL_Color){ delete_hover_r,
                                                        p.button_fill_hover.g,
                                                        p.button_fill_hover.b,
                                                        p.button_fill_hover.a }
                                         : p.button_fill_hover;
            drawing_program_visual_panel_draw_tab_button(renderer,
                                                         rect,
                                                         delete_canvas_button,
                                                         delete_canvas_armed ? "DELETE CANVAS: CONFIRM"
                                                                             : "DELETE CANVAS",
                                                         delete_fill,
                                                         delete_hover,
                                                         p.button_fill_active,
                                                         p.button_border,
                                                         (ctx->texture_project.surface_count > 1u) ? p.text_primary
                                                                                                  : p.text_muted,
                                                         m.body_scale,
                                                         delete_canvas_armed,
                                                         drawing_program_visual_panel_ui_hovered(ui,
                                                                                                delete_canvas_button,
                                                                                                hooks),
                                                         hooks);
        }
        for (surface_index = 0u; surface_index < ctx->texture_project.surface_count; ++surface_index) {
            const DrawingProgramTextureSurface *surface =
                drawing_program_texture_project_surface_at(&ctx->texture_project, surface_index);
            SDL_Color fill;
            SDL_Color text_color;
            if (!surface || !surface->storage) {
                continue;
            }
            row = right_canvas_surface_row_rect(rect, m, surface_index);
            fill = (surface_index == ctx->texture_project.active_surface_index) ? p.button_fill_active : p.button_fill;
            text_color =
                (surface_index == ctx->texture_project.active_surface_index) ? p.text_primary : p.text_muted;
            if (drawing_program_visual_panel_ui_hovered(ui, row, hooks)) {
                fill = (surface_index == ctx->texture_project.active_surface_index) ? p.button_fill_active : p.button_fill_hover;
            }
            SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
            (void)SDL_RenderFillRect(renderer, &row);
            SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
            (void)SDL_RenderDrawRect(renderer, &row);
            (void)snprintf(line,
                           sizeof(line),
                           "%u %s %ux%u %s %s",
                           (unsigned)(surface_index + 1u),
                           surface->name,
                           (unsigned)surface->storage->document.logical_width,
                           (unsigned)surface->storage->document.logical_height,
                           surface->is_blank ? "BLANK" : "PAINT",
                           surface->resize_locked ? "LOCK" : "FREE");
            hooks->draw_bitmap_text(renderer, rect, row.x + 6, row.y + m.row_text_y, line, text_color, m.body_scale);
        }

        reset_layout_button = right_canvas_reset_object_layout_button_rect(rect, m);
        reset_view_button = right_canvas_reset_view_button_rect(rect, m);
        clear_canvas_button = right_canvas_clear_canvas_button_rect(rect, m);
        clear_objects_button = right_canvas_clear_objects_button_rect(rect, m);
        delete_selection_button = right_canvas_delete_selection_button_rect(rect, m);
        clear_history_button = right_canvas_clear_history_button_rect(rect, m);

        drawing_program_visual_panel_draw_tab_button(renderer, rect, reset_layout_button, "RESET TO OBJECT LAYOUT",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, reset_layout_button, hooks), hooks);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, reset_view_button, "FIT ALL",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, reset_view_button, hooks), hooks);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, clear_canvas_button, "CLEAR CANVAS",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, clear_canvas_button, hooks), hooks);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, clear_objects_button, "CLEAR OBJECTS",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, clear_objects_button, hooks), hooks);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, delete_selection_button, "DELETE SELECTION",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     delete_selection_enabled ? p.text_primary : p.text_muted, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, delete_selection_button, hooks), hooks);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, clear_history_button, "CLEAR HISTORY",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, clear_history_button, hooks), hooks);
    } else if (right_slot == VISUAL_RIGHT_PANEL_SLOT_LAYER) {
        uint32_t display_i;
        SDL_Rect button_rect;
        SDL_Rect opacity_row_rect;
        SDL_Rect role_button_rect;
        int opacity_label_y;
        int role_section_y;
        DrawingProgramVisualLayerRolePreset active_role = DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_CUSTOM;
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "LAYER STACK", p.text_primary, m.body_scale);
        y += m.line_h;
        if (ctx->document.layer_count == 0u) {
            hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "NO LAYERS", p.text_muted, m.body_scale);
            y += m.line_h;
        }
        for (display_i = 0u; display_i < ctx->document.layer_count; ++display_i) {
            uint32_t model_i = (ctx->document.layer_count - 1u) - display_i;
            const DrawingProgramLayer *layer = &ctx->document.layers[model_i];
            int is_active = (layer->layer_id == ctx->editor.active_layer_id) ? 1 : 0;
            uint8_t layer_opacity = drawing_program_visual_layer_opacity_get(ctx, layer->layer_id);
            SDL_Color fill = is_active ? p.button_fill_active : p.button_fill;
            row = right_layer_row_rect(rect, m, display_i);
            if (drawing_program_visual_panel_ui_hovered(ui, row, hooks)) {
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
            hooks->draw_bitmap_text(renderer, rect, row.x + 6, row.y + m.row_text_y, line, p.text_primary, m.body_scale);
            y = row.y + row.h + m.section_gap;
        }

        opacity_row_rect = right_layer_opacity_row_rect(rect, m, ctx->document.layer_count);
        SDL_SetRenderDrawColor(renderer, p.button_fill.r, p.button_fill.g, p.button_fill.b, p.button_fill.a);
        (void)SDL_RenderFillRect(renderer, &opacity_row_rect);
        SDL_SetRenderDrawColor(renderer, p.button_border.r, p.button_border.g, p.button_border.b, p.button_border.a);
        (void)SDL_RenderDrawRect(renderer, &opacity_row_rect);
        {
            SDL_Rect opacity_track = right_layer_opacity_track_rect(opacity_row_rect, m);
            opacity_label_y = opacity_row_rect.y + ((m.line_h - m.body_glyph_h) / 2);
            if (opacity_label_y < opacity_row_rect.y + 1) {
                opacity_label_y = opacity_row_rect.y + 1;
            }
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
            hooks->draw_bitmap_text(renderer, rect, opacity_row_rect.x + 6, opacity_label_y, line, p.text_primary, m.body_scale);
        }

        y = right_layer_actions_label_y(rect, m, ctx->document.layer_count);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, y, "ACTIONS", p.text_primary, m.body_scale);

        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ADD);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, button_rect, "ADD LAYER",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_DUPLICATE);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, button_rect, "DUPLICATE SELECTED",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_RENAME);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, button_rect, "AUTO ROLE NAME",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_DELETE);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, button_rect, "DELETE SELECTED",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ACTIVE_PREV);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, button_rect, "ACTIVE PREV",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_ACTIVE_NEXT);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, button_rect, "ACTIVE NEXT",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_MOVE_UP);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, button_rect, "MOVE UP",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_MOVE_DOWN);
        drawing_program_visual_panel_draw_tab_button(renderer, rect, button_rect, "MOVE DOWN",
                                                     p.button_fill, p.button_fill_hover, p.button_fill_active, p.button_border,
                                                     p.text_primary, m.body_scale, 0,
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks), hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_TOGGLE_VISIBLE);
        drawing_program_visual_panel_draw_tab_button(renderer,
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
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks),
                                                     hooks);
        button_rect = right_layer_action_button_rect(rect, m, ctx->document.layer_count, VISUAL_LAYER_ACTION_TOGGLE_LOCK);
        drawing_program_visual_panel_draw_tab_button(renderer,
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
                                                     drawing_program_visual_panel_ui_hovered(ui, button_rect, hooks),
                                                     hooks);
        active_role = drawing_program_visual_layer_role_detect_active(ctx);
        role_section_y = right_layer_role_section_start_y(rect, m, ctx->document.layer_count);
        hooks->draw_bitmap_text(renderer, rect, rect.x + m.pad_x, role_section_y, "AUTHORING ROLE", p.text_primary,
                                m.body_scale);
        (void)snprintf(line,
                       sizeof(line),
                       "ACTIVE %s  %s",
                       drawing_program_visual_layer_role_name(active_role),
                       drawing_program_visual_layer_role_lane_label(active_role));
        hooks->draw_bitmap_text(renderer,
                                rect,
                                rect.x + m.pad_x,
                                role_section_y + m.line_h,
                                line,
                                p.text_muted,
                                m.body_scale);
        hooks->draw_bitmap_text(renderer,
                                rect,
                                rect.x + m.pad_x,
                                role_section_y + (2 * m.line_h),
                                "BASE+DETAIL -> BASE  OTHERS -> OVERLAY",
                                p.text_muted,
                                m.body_scale);
        for (i = 0u; i < (uint32_t)DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_COUNT; ++i) {
            DrawingProgramVisualLayerRolePreset role = (DrawingProgramVisualLayerRolePreset)i;
            role_button_rect = right_layer_role_button_rect(rect, m, ctx->document.layer_count, i);
            drawing_program_visual_panel_draw_tab_button(
                renderer,
                rect,
                role_button_rect,
                drawing_program_visual_layer_role_button_label(role),
                p.button_fill,
                p.button_fill_hover,
                p.button_fill_active,
                p.button_border,
                p.text_primary,
                m.body_scale,
                active_role == role,
                drawing_program_visual_panel_ui_hovered(ui, role_button_rect, hooks),
                hooks);
        }
    } else if (right_slot == VISUAL_RIGHT_PANEL_SLOT_FILE) {
        drawing_program_visual_render_right_file_tab(renderer, rect, ctx, ui, m, p, hooks);
    } else if (right_slot == VISUAL_RIGHT_PANEL_SLOT_ASSET) {
        drawing_program_visual_render_right_asset_tab(renderer, rect, ctx, ui, m, p, hooks);
    } else if (right_slot == VISUAL_RIGHT_PANEL_SLOT_EXPORT) {
        drawing_program_visual_render_right_export_tab(renderer, rect, ctx, ui, m, p, hooks);
    } else {
        drawing_program_visual_render_right_panel_color_tab(renderer, rect, y, ctx, ui, m, p, hooks);
    }
}
