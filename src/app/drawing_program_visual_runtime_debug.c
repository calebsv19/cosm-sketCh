#include "drawing_program/drawing_program_visual_runtime_debug.h"

#include <stdio.h>
#include <stdlib.h>

#include "core_font.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_visual_resources.h"

int drawing_program_visual_trace_ui_state_enabled(void) {
    const char *value = getenv("DRAWING_PROGRAM_TRACE_UI_STATE");
    if (!value || value[0] == '\0' || value[0] == '0') {
        return 0;
    }
    return 1;
}

void drawing_program_visual_trace_after_runtime_start(const struct DrawingProgramAppContext *ctx) {
    if (!ctx || !drawing_program_visual_trace_ui_state_enabled()) {
        return;
    }
    fprintf(stderr,
            "drawing_program trace visual after_runtime_start tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u\n",
            (unsigned)ctx->editor.active_tool,
            (unsigned)ctx->ui.theme_preset_id,
            (unsigned)ctx->ui.font_preset_id,
            (int)ctx->ui.font_zoom_step,
            (unsigned)ctx->ui.left_panel_slot,
            (unsigned)ctx->ui.right_panel_slot);
}

void drawing_program_visual_trace_after_ui_resolve(const struct DrawingProgramAppContext *ctx,
                                                   int env_override_applied) {
    if (!ctx || !drawing_program_visual_trace_ui_state_enabled()) {
        return;
    }
    fprintf(stderr,
            "drawing_program trace visual after_ui_resolve tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u env_override=%d\n",
            (unsigned)ctx->editor.active_tool,
            (unsigned)ctx->ui.theme_preset_id,
            (unsigned)ctx->ui.font_preset_id,
            (int)ctx->ui.font_zoom_step,
            (unsigned)ctx->ui.left_panel_slot,
            (unsigned)ctx->ui.right_panel_slot,
            env_override_applied);
}

void drawing_program_visual_update_window_title(SDL_Window *window,
                                                const struct DrawingProgramAppContext *ctx,
                                                const struct DrawingProgramSelectionState *selection,
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
    center_module_type_id = drawing_program_visual_module_type_for_pane(ctx, 6u);
    if (ctx->ui.font_preset_id < (uint32_t)CORE_FONT_PRESET_COUNT) {
        font_name = core_font_preset_name((CoreFontPresetId)ctx->ui.font_preset_id);
    }
    if (drawing_program_visual_font_backend_is_ready()) {
        text_backend = "ttf";
    }
    (void)snprintf(title,
                   sizeof(title),
                   "sketCh | backend=sdl-debug text_backend=%s frame=%llu present=%llu panes=%u center_module=%u active_tool=%u color=%u visible_layers=%u active_layer=%u sel=%ux%u d=%d,%d theme=%u font=%s zoomstep=%d",
                   text_backend,
                   (unsigned long long)ctx->runtime.frame_counter,
                   (unsigned long long)present_count,
                   ctx->pane_host.leaf_count,
                   center_module_type_id,
                   (unsigned)ctx->editor.active_tool,
                   (unsigned)drawing_program_color_index_clamp(ctx->ui.active_color_index),
                   ctx->runtime.render_projection.visible_layer_count,
                   ctx->runtime.render_projection.active_layer_id,
                   selection_w,
                   selection_h,
                   (int)selection_dx,
                   (int)selection_dy,
                   ctx->ui.theme_preset_id,
                   font_name ? font_name : "unknown",
                   (int)ctx->ui.font_zoom_step);
    SDL_SetWindowTitle(window, title);
}
