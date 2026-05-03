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
            "drawing_program trace visual after_runtime_start tool=%u theme=%u font=%u zoom=%d slot_l=%u slot_r=%u layout_mode=%u overlay_state=%u paused=%u\n",
            (unsigned)ctx->editor.active_tool,
            (unsigned)ctx->ui.theme_preset_id,
            (unsigned)ctx->ui.font_preset_id,
            (int)ctx->ui.font_zoom_step,
            (unsigned)ctx->ui.left_panel_slot,
            (unsigned)ctx->ui.right_panel_slot,
            (unsigned)ctx->pane_host.layout_state.mode,
            (unsigned)ctx->overlay_adapter.lifecycle_state,
            (unsigned)ctx->overlay_adapter.runtime_paused);
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
    static int last_authoring_active = -1;
    int authoring_active = 0;
    (void)selection;
    (void)present_count;

    if (!window || !ctx) {
        return;
    }
    authoring_active = ctx->pane_host.layout_state.mode == CORE_LAYOUT_MODE_AUTHORING ? 1 : 0;
    if (last_authoring_active == authoring_active) {
        return;
    }
    last_authoring_active = authoring_active;
    SDL_SetWindowTitle(window, authoring_active ? "sketCh [Authoring]" : "sketCh");
}
