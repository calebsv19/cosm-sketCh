#include "drawing_program/drawing_program_visual_runtime_debug.h"

#include <stdio.h>
#include <stdlib.h>

#include "drawing_program/drawing_program_app_main.h"

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
    static int title_applied = 0;
    (void)ctx;
    (void)selection;
    (void)present_count;

    if (!window || !ctx) {
        return;
    }
    if (title_applied) {
        return;
    }
    title_applied = 1;
    SDL_SetWindowTitle(window, "sketCh");
}
