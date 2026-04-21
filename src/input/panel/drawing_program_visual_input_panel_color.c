#include "drawing_program/drawing_program_color_panel_internal.h"

static void visual_color_panel_apply_object_target(DrawingProgramAppContext *ctx, VisualPanelUiState *ui) {
    const DrawingProgramObjectRecord *object;
    uint8_t active_index;
    if (!ctx || !ui || ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_NONE ||
        ui->object_color_target_object_id == 0u) {
        return;
    }
    object = drawing_program_object_store_get_by_id(&ctx->object_store, ui->object_color_target_object_id);
    if (!object) {
        ui->object_color_target_kind = VISUAL_OBJECT_COLOR_TARGET_NONE;
        ui->object_color_target_object_id = 0u;
        return;
    }
    active_index = drawing_program_color_index_clamp(ctx->ui.active_color_index);
    if (ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_STROKE) {
        (void)drawing_program_history_apply_set_object_stroke_color(
            &ctx->history, &ctx->object_store, ui->object_color_target_object_id, active_index);
        return;
    }
    if (ui->object_color_target_kind == VISUAL_OBJECT_COLOR_TARGET_FILL) {
        (void)drawing_program_history_apply_set_object_fill_color(
            &ctx->history, &ctx->object_store, ui->object_color_target_object_id, active_index);
    }
}

int drawing_program_visual_input_handle_right_color_panel_click_payload(
    DrawingProgramAppContext *ctx,
    SDL_Rect rect,
    int x,
    int y,
    VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
    VisualPaneLayoutMetrics m;
    uint8_t recent_i;
    uint8_t palette_i;
    SDL_Rect hue_rect;
    SDL_Rect sv_rect;
    if (!ctx || !ui || !hooks) {
        return 0;
    }
    m = make_pane_layout_metrics(ctx);
    hue_rect = right_color_hue_slider_rect(rect, m);
    sv_rect = right_color_sv_grid_rect(rect, m);
    for (recent_i = 0u; recent_i < ctx->ui.recent_color_count; ++recent_i) {
        SDL_Rect recent_rect = right_color_recent_swatch_rect(rect, m, recent_i);
        if (hooks->point_in_rect(recent_rect, x, y)) {
            drawing_program_ui_color_apply_active_rgb(ctx,
                                                      ctx->ui.recent_color_rgb[recent_i][0],
                                                      ctx->ui.recent_color_rgb[recent_i][1],
                                                      ctx->ui.recent_color_rgb[recent_i][2]);
            drawing_program_ui_color_sync_selector_from_active(ctx);
            visual_color_panel_apply_object_target(ctx, ui);
            return 1;
        }
    }
    if (hooks->point_in_rect(hue_rect, x, y)) {
        int relative_x = x - hue_rect.x;
        if (relative_x < 0) {
            relative_x = 0;
        }
        if (relative_x >= hue_rect.w) {
            relative_x = hue_rect.w - 1;
        }
        ctx->ui.color_hue = (hue_rect.w > 1) ? (uint8_t)((relative_x * 255) / (hue_rect.w - 1)) : 0u;
        drawing_program_ui_color_apply_hsv_to_active(ctx);
        visual_color_panel_apply_object_target(ctx, ui);
        return 1;
    }
    if (hooks->point_in_rect(sv_rect, x, y)) {
        int relative_x = x - sv_rect.x;
        int relative_y = y - sv_rect.y;
        if (relative_x < 0) {
            relative_x = 0;
        }
        if (relative_x >= sv_rect.w) {
            relative_x = sv_rect.w - 1;
        }
        if (relative_y < 0) {
            relative_y = 0;
        }
        if (relative_y >= sv_rect.h) {
            relative_y = sv_rect.h - 1;
        }
        ctx->ui.color_saturation = (sv_rect.w > 1) ? (uint8_t)((relative_x * 255) / (sv_rect.w - 1)) : 0u;
        ctx->ui.color_value = (sv_rect.h > 1) ? (uint8_t)(255u - ((relative_y * 255) / (sv_rect.h - 1))) : 255u;
        drawing_program_ui_color_apply_hsv_to_active(ctx);
        visual_color_panel_apply_object_target(ctx, ui);
        return 1;
    }
    for (palette_i = 0u; palette_i < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT; ++palette_i) {
        SDL_Rect palette_rect = right_color_palette_swatch_rect(rect, m, palette_i);
        if (hooks->point_in_rect(palette_rect, x, y)) {
            ctx->ui.active_color_index = palette_i;
            drawing_program_ui_color_sync_selector_from_active(ctx);
            visual_color_panel_apply_object_target(ctx, ui);
            return 1;
        }
    }
    return 0;
}
