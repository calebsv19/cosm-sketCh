#include "drawing_program/drawing_program_authoring_host.h"

#include <stdio.h>
#include <string.h>

#include "core_font.h"
#include "core_layout.h"
#include "core_theme.h"
#include "kit_workspace_authoring.h"

#include "drawing_program/drawing_program_pane_host.h"

static CoreResult drawing_program_authoring_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static int drawing_program_authoring_clamp_font_zoom_step(int step) {
    if (step < -2) {
        return -2;
    }
    if (step > 4) {
        return 4;
    }
    return step;
}

static uint32_t drawing_program_authoring_mod_bits(SDL_Keymod mods) {
    uint32_t bits = 0u;
    if ((mods & KMOD_SHIFT) != 0) {
        bits |= KIT_WORKSPACE_AUTHORING_MOD_SHIFT;
    }
    if ((mods & KMOD_ALT) != 0) {
        bits |= KIT_WORKSPACE_AUTHORING_MOD_ALT;
    }
    if ((mods & KMOD_CTRL) != 0) {
        bits |= KIT_WORKSPACE_AUTHORING_MOD_CTRL;
    }
    if ((mods & KMOD_GUI) != 0) {
        bits |= KIT_WORKSPACE_AUTHORING_MOD_GUI;
    }
    return bits;
}

static KitWorkspaceAuthoringKey drawing_program_authoring_key_from_sdl_keysym(const SDL_Keysym *keysym) {
    if (!keysym) {
        return KIT_WORKSPACE_AUTHORING_KEY_UNKNOWN;
    }
    switch (keysym->scancode) {
        case SDL_SCANCODE_C: return KIT_WORKSPACE_AUTHORING_KEY_C;
        case SDL_SCANCODE_V: return KIT_WORKSPACE_AUTHORING_KEY_V;
        default: break;
    }
    switch (keysym->sym) {
        case SDLK_TAB: return KIT_WORKSPACE_AUTHORING_KEY_TAB;
        case SDLK_RETURN:
        case SDLK_KP_ENTER: return KIT_WORKSPACE_AUTHORING_KEY_ENTER;
        case SDLK_ESCAPE: return KIT_WORKSPACE_AUTHORING_KEY_ESCAPE;
        case SDLK_h: return KIT_WORKSPACE_AUTHORING_KEY_H;
        case SDLK_v: return KIT_WORKSPACE_AUTHORING_KEY_V;
        case SDLK_x: return KIT_WORKSPACE_AUTHORING_KEY_X;
        case SDLK_BACKSPACE:
        case SDLK_DELETE: return KIT_WORKSPACE_AUTHORING_KEY_BACKSPACE;
        case SDLK_r: return KIT_WORKSPACE_AUTHORING_KEY_R;
        case SDLK_0:
        case SDLK_KP_0: return KIT_WORKSPACE_AUTHORING_KEY_DIGIT_0;
        case SDLK_1:
        case SDLK_KP_1: return KIT_WORKSPACE_AUTHORING_KEY_DIGIT_1;
        case SDLK_2:
        case SDLK_KP_2: return KIT_WORKSPACE_AUTHORING_KEY_DIGIT_2;
        case SDLK_3:
        case SDLK_KP_3: return KIT_WORKSPACE_AUTHORING_KEY_DIGIT_3;
        case SDLK_4:
        case SDLK_KP_4: return KIT_WORKSPACE_AUTHORING_KEY_DIGIT_4;
        case SDLK_5:
        case SDLK_KP_5: return KIT_WORKSPACE_AUTHORING_KEY_DIGIT_5;
        case SDLK_6:
        case SDLK_KP_6: return KIT_WORKSPACE_AUTHORING_KEY_DIGIT_6;
        case SDLK_c: return KIT_WORKSPACE_AUTHORING_KEY_C;
        case SDLK_z: return KIT_WORKSPACE_AUTHORING_KEY_Z;
        default: return KIT_WORKSPACE_AUTHORING_KEY_UNKNOWN;
    }
}

static void drawing_program_authoring_note_consumed(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    ctx->authoring_host.last_event_consumed = 1u;
    ctx->authoring_host.consumed_key_count += 1u;
}

static void drawing_program_authoring_capture_baseline(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    memcpy(ctx->authoring_host.baseline_nodes,
           ctx->pane_host.nodes,
           sizeof(ctx->pane_host.nodes));
    ctx->authoring_host.baseline_node_count = ctx->pane_host.node_count;
    ctx->authoring_host.baseline_root_index = ctx->pane_host.root_index;
    memcpy(ctx->authoring_host.baseline_module_bindings,
           ctx->pane_host.module_bindings,
           sizeof(ctx->pane_host.module_bindings));
    ctx->authoring_host.baseline_module_binding_count = ctx->pane_host.module_binding_count;
    ctx->authoring_host.baseline_theme_preset_id = ctx->ui.theme_preset_id;
    ctx->authoring_host.baseline_font_preset_id = ctx->ui.font_preset_id;
    ctx->authoring_host.baseline_font_zoom_step = ctx->ui.font_zoom_step;
    ctx->authoring_host.draft_baseline_valid = 1u;
}

static CoreResult drawing_program_authoring_restore_baseline(DrawingProgramAppContext *ctx) {
    CoreResult rebuild_result;
    if (!ctx) {
        return drawing_program_authoring_invalid("null app context");
    }
    if (!ctx->authoring_host.draft_baseline_valid) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "missing authoring baseline" };
    }
    memcpy(ctx->pane_host.nodes,
           ctx->authoring_host.baseline_nodes,
           sizeof(ctx->pane_host.nodes));
    ctx->pane_host.node_count = ctx->authoring_host.baseline_node_count;
    ctx->pane_host.root_index = ctx->authoring_host.baseline_root_index;
    memcpy(ctx->pane_host.module_bindings,
           ctx->authoring_host.baseline_module_bindings,
           sizeof(ctx->pane_host.module_bindings));
    ctx->pane_host.module_binding_count = ctx->authoring_host.baseline_module_binding_count;
    ctx->ui.theme_preset_id = ctx->authoring_host.baseline_theme_preset_id;
    ctx->ui.font_preset_id = ctx->authoring_host.baseline_font_preset_id;
    ctx->ui.font_zoom_step = ctx->authoring_host.baseline_font_zoom_step;
    rebuild_result = drawing_program_pane_host_rebuild(ctx);
    if (rebuild_result.code != CORE_OK) {
        return rebuild_result;
    }
    return core_result_ok();
}

static void drawing_program_authoring_clear_baseline(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    ctx->authoring_host.draft_baseline_valid = 0u;
    ctx->authoring_host.baseline_node_count = 0u;
    ctx->authoring_host.baseline_root_index = 0u;
    ctx->authoring_host.baseline_module_binding_count = 0u;
    ctx->authoring_host.font_theme_pending_changes = 0u;
    ctx->authoring_host.custom_theme_status_active = 0u;
    ctx->authoring_host.custom_theme_status[0] = '\0';
}

void drawing_program_authoring_host_reset(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    memset(&ctx->authoring_host, 0, sizeof(ctx->authoring_host));
    ctx->pane_host.layout_state.mode = CORE_LAYOUT_MODE_RUNTIME;
    ctx->pane_host.layout_state.draft_revision = ctx->pane_host.layout_state.active_revision;
    ctx->pane_host.layout_state.has_pending_changes = false;
    ctx->pane_host.layout_state.rebuild_required = false;
    ctx->authoring_host.overlay_mode = DRAWING_PROGRAM_AUTHORING_OVERLAY_PANE;
    drawing_program_authoring_clear_baseline(ctx);
}

int drawing_program_authoring_host_active(const DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return 0;
    }
    return ctx->pane_host.layout_state.mode == CORE_LAYOUT_MODE_AUTHORING ? 1 : 0;
}

int drawing_program_authoring_host_pane_overlay_active(const DrawingProgramAppContext *ctx) {
    return ctx ? kit_workspace_authoring_pane_overlay_active(ctx->pane_host.layout_state.mode,
                                                             CORE_LAYOUT_MODE_AUTHORING,
                                                             (int)ctx->authoring_host.overlay_mode,
                                                             DRAWING_PROGRAM_AUTHORING_OVERLAY_PANE)
               : 0;
}

int drawing_program_authoring_host_font_theme_overlay_active(const DrawingProgramAppContext *ctx) {
    return drawing_program_authoring_host_active(ctx) &&
                   ctx->authoring_host.overlay_mode == DRAWING_PROGRAM_AUTHORING_OVERLAY_FONT_THEME
               ? 1
               : 0;
}

CoreResult drawing_program_authoring_host_enter(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return drawing_program_authoring_invalid("null app context");
    }
    if (!drawing_program_authoring_host_active(ctx)) {
        if (!core_layout_enter_authoring(&ctx->pane_host.layout_state)) {
            return (CoreResult){ CORE_ERR_INVALID_ARG, "failed to enter authoring mode" };
        }
        drawing_program_authoring_capture_baseline(ctx);
        ctx->authoring_host.overlay_mode = DRAWING_PROGRAM_AUTHORING_OVERLAY_PANE;
        ctx->authoring_host.enter_count += 1u;
    }
    ctx->authoring_host.last_event_entered = 1u;
    return core_result_ok();
}

CoreResult drawing_program_authoring_host_exit(DrawingProgramAppContext *ctx) {
    CoreResult restore_result;
    if (!ctx) {
        return drawing_program_authoring_invalid("null app context");
    }
    if (drawing_program_authoring_host_active(ctx)) {
        restore_result = drawing_program_authoring_restore_baseline(ctx);
        if (restore_result.code != CORE_OK) {
            return restore_result;
        }
        if (!core_layout_cancel_authoring(&ctx->pane_host.layout_state)) {
            return (CoreResult){ CORE_ERR_INVALID_ARG, "failed to exit authoring mode" };
        }
        drawing_program_authoring_clear_baseline(ctx);
        ctx->authoring_host.exit_count += 1u;
    }
    ctx->authoring_host.key_c_down = 0u;
    ctx->authoring_host.key_v_down = 0u;
    ctx->authoring_host.entry_chord_armed_key = KIT_WORKSPACE_AUTHORING_KEY_UNKNOWN;
    ctx->authoring_host.overlay_mode = DRAWING_PROGRAM_AUTHORING_OVERLAY_PANE;
    ctx->authoring_host.last_event_exited = 1u;
    return core_result_ok();
}

CoreResult drawing_program_authoring_host_apply(DrawingProgramAppContext *ctx) {
    CoreResult rebuild_result;
    if (!ctx) {
        return drawing_program_authoring_invalid("null app context");
    }
    if (!drawing_program_authoring_host_active(ctx)) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "authoring apply requires authoring mode" };
    }
    rebuild_result = drawing_program_pane_host_rebuild(ctx);
    if (rebuild_result.code != CORE_OK) {
        return rebuild_result;
    }
    if (!core_layout_apply_authoring(&ctx->pane_host.layout_state)) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "failed to apply authoring mode" };
    }
    drawing_program_authoring_clear_baseline(ctx);
    ctx->authoring_host.key_c_down = 0u;
    ctx->authoring_host.key_v_down = 0u;
    ctx->authoring_host.entry_chord_armed_key = KIT_WORKSPACE_AUTHORING_KEY_UNKNOWN;
    ctx->authoring_host.overlay_mode = DRAWING_PROGRAM_AUTHORING_OVERLAY_PANE;
    ctx->authoring_host.apply_count += 1u;
    ctx->authoring_host.last_event_exited = 1u;
    return core_result_ok();
}

CoreResult drawing_program_authoring_host_cancel(DrawingProgramAppContext *ctx) {
    CoreResult restore_result;
    if (!ctx) {
        return drawing_program_authoring_invalid("null app context");
    }
    if (!drawing_program_authoring_host_active(ctx)) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "authoring cancel requires authoring mode" };
    }
    restore_result = drawing_program_authoring_restore_baseline(ctx);
    if (restore_result.code != CORE_OK) {
        return restore_result;
    }
    if (!core_layout_cancel_authoring(&ctx->pane_host.layout_state)) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "failed to cancel authoring mode" };
    }
    drawing_program_authoring_clear_baseline(ctx);
    ctx->authoring_host.key_c_down = 0u;
    ctx->authoring_host.key_v_down = 0u;
    ctx->authoring_host.entry_chord_armed_key = KIT_WORKSPACE_AUTHORING_KEY_UNKNOWN;
    ctx->authoring_host.overlay_mode = DRAWING_PROGRAM_AUTHORING_OVERLAY_PANE;
    ctx->authoring_host.cancel_count += 1u;
    ctx->authoring_host.last_event_exited = 1u;
    return core_result_ok();
}

CoreResult drawing_program_authoring_host_mark_draft_changed(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return drawing_program_authoring_invalid("null app context");
    }
    if (!drawing_program_authoring_host_active(ctx)) {
        return core_result_ok();
    }
    if (!core_layout_mark_draft_changed(&ctx->pane_host.layout_state)) {
        return (CoreResult){ CORE_ERR_INVALID_ARG, "failed to mark authoring draft changed" };
    }
    ctx->authoring_host.draft_change_count += 1u;
    return core_result_ok();
}

static CoreResult drawing_program_authoring_host_mark_font_theme_changed(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return drawing_program_authoring_invalid("null app context");
    }
    ctx->authoring_host.font_theme_pending_changes = 1u;
    ctx->authoring_host.font_theme_change_count += 1u;
    return drawing_program_authoring_host_mark_draft_changed(ctx);
}

CoreResult drawing_program_authoring_host_cycle_overlay(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return drawing_program_authoring_invalid("null app context");
    }
    if (!drawing_program_authoring_host_active(ctx)) {
        return core_result_ok();
    }
    ctx->authoring_host.overlay_mode =
        (ctx->authoring_host.overlay_mode == DRAWING_PROGRAM_AUTHORING_OVERLAY_PANE)
            ? DRAWING_PROGRAM_AUTHORING_OVERLAY_FONT_THEME
            : DRAWING_PROGRAM_AUTHORING_OVERLAY_PANE;
    ctx->authoring_host.overlay_cycle_count += 1u;
    return core_result_ok();
}

CoreResult drawing_program_authoring_host_adjust_font_zoom(DrawingProgramAppContext *ctx, int direction) {
    int next_step;
    if (!ctx || direction == 0) {
        return drawing_program_authoring_invalid("invalid font zoom adjustment");
    }
    next_step = drawing_program_authoring_clamp_font_zoom_step((int)ctx->ui.font_zoom_step + (direction > 0 ? 1 : -1));
    if (next_step == (int)ctx->ui.font_zoom_step) {
        return core_result_ok();
    }
    ctx->ui.font_zoom_step = (int8_t)next_step;
    return drawing_program_authoring_host_mark_font_theme_changed(ctx);
}

CoreResult drawing_program_authoring_host_reset_font_zoom(DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return drawing_program_authoring_invalid("null app context");
    }
    if (ctx->ui.font_zoom_step == 0) {
        return core_result_ok();
    }
    ctx->ui.font_zoom_step = 0;
    return drawing_program_authoring_host_mark_font_theme_changed(ctx);
}

CoreResult drawing_program_authoring_host_set_font_preset(DrawingProgramAppContext *ctx, uint32_t font_preset_id) {
    if (!ctx || font_preset_id >= (uint32_t)CORE_FONT_PRESET_COUNT ||
        !core_font_preset_name((CoreFontPresetId)font_preset_id)) {
        return drawing_program_authoring_invalid("invalid font preset");
    }
    if (ctx->ui.font_preset_id == font_preset_id) {
        return core_result_ok();
    }
    ctx->ui.font_preset_id = font_preset_id;
    return drawing_program_authoring_host_mark_font_theme_changed(ctx);
}

CoreResult drawing_program_authoring_host_set_theme_preset(DrawingProgramAppContext *ctx, uint32_t theme_preset_id) {
    CoreThemePreset preset;
    if (!ctx || theme_preset_id >= (uint32_t)CORE_THEME_PRESET_COUNT ||
        core_theme_get_preset((CoreThemePresetId)theme_preset_id, &preset).code != CORE_OK) {
        return drawing_program_authoring_invalid("invalid theme preset");
    }
    if (ctx->ui.theme_preset_id == theme_preset_id) {
        return core_result_ok();
    }
    ctx->ui.theme_preset_id = theme_preset_id;
    return drawing_program_authoring_host_mark_font_theme_changed(ctx);
}

CoreResult drawing_program_authoring_host_note_custom_theme_stub(DrawingProgramAppContext *ctx,
                                                                 const char *status) {
    if (!ctx || !status) {
        return drawing_program_authoring_invalid("invalid custom theme status");
    }
    snprintf(ctx->authoring_host.custom_theme_status,
             sizeof(ctx->authoring_host.custom_theme_status),
             "%s",
             status);
    ctx->authoring_host.custom_theme_status_active = 1u;
    ctx->authoring_host.custom_theme_stub_count += 1u;
    return core_result_ok();
}

CoreResult drawing_program_authoring_host_export_accepted_pane_state(
    const DrawingProgramAppContext *ctx,
    CoreLayoutState *out_layout_state,
    CorePaneNode out_nodes[DRAWING_PROGRAM_PANE_NODE_CAPACITY],
    uint32_t *out_node_count,
    uint32_t *out_root_index,
    CorePaneModuleBinding out_module_bindings[DRAWING_PROGRAM_MODULE_BINDING_CAPACITY],
    uint32_t *out_module_binding_count) {
    if (!ctx || !out_layout_state || !out_nodes || !out_node_count || !out_root_index ||
        !out_module_bindings || !out_module_binding_count) {
        return drawing_program_authoring_invalid("invalid accepted pane export request");
    }

    *out_layout_state = ctx->pane_host.layout_state;
    if (drawing_program_authoring_host_active(ctx) && ctx->authoring_host.draft_baseline_valid) {
        memcpy(out_nodes,
               ctx->authoring_host.baseline_nodes,
               sizeof(ctx->authoring_host.baseline_nodes));
        *out_node_count = ctx->authoring_host.baseline_node_count;
        *out_root_index = ctx->authoring_host.baseline_root_index;
        memcpy(out_module_bindings,
               ctx->authoring_host.baseline_module_bindings,
               sizeof(ctx->authoring_host.baseline_module_bindings));
        *out_module_binding_count = ctx->authoring_host.baseline_module_binding_count;
    } else {
        memcpy(out_nodes, ctx->pane_host.nodes, sizeof(ctx->pane_host.nodes));
        *out_node_count = ctx->pane_host.node_count;
        *out_root_index = ctx->pane_host.root_index;
        memcpy(out_module_bindings,
               ctx->pane_host.module_bindings,
               sizeof(ctx->pane_host.module_bindings));
        *out_module_binding_count = ctx->pane_host.module_binding_count;
    }

    out_layout_state->mode = CORE_LAYOUT_MODE_RUNTIME;
    out_layout_state->draft_revision = out_layout_state->active_revision;
    out_layout_state->has_pending_changes = false;
    out_layout_state->rebuild_required = false;
    return core_result_ok();
}

int drawing_program_authoring_host_handle_sdl_event(DrawingProgramAppContext *ctx,
                                                    const SDL_Event *event) {
    KitWorkspaceAuthoringKey key;
    uint32_t mod_bits;
    int c_down;
    int v_down;
    int authoring_alt_only;
    int chord_pair_pressed;
    if (!ctx || !event) {
        return 0;
    }
    ctx->authoring_host.last_event_consumed = 0u;
    ctx->authoring_host.last_event_entered = 0u;
    ctx->authoring_host.last_event_exited = 0u;

    if (event->type != SDL_KEYDOWN && event->type != SDL_KEYUP) {
        return 0;
    }

    key = drawing_program_authoring_key_from_sdl_keysym(&event->key.keysym);
    if (event->type == SDL_KEYUP) {
        if (key == KIT_WORKSPACE_AUTHORING_KEY_C) {
            ctx->authoring_host.key_c_down = 0u;
        } else if (key == KIT_WORKSPACE_AUTHORING_KEY_V) {
            ctx->authoring_host.key_v_down = 0u;
        }
        return 0;
    }

    mod_bits = drawing_program_authoring_mod_bits((SDL_Keymod)event->key.keysym.mod);
    if (key == KIT_WORKSPACE_AUTHORING_KEY_C) {
        ctx->authoring_host.key_c_down = 1u;
    } else if (key == KIT_WORKSPACE_AUTHORING_KEY_V) {
        ctx->authoring_host.key_v_down = 1u;
    }

    c_down = ctx->authoring_host.key_c_down ? 1 : 0;
    v_down = ctx->authoring_host.key_v_down ? 1 : 0;
    chord_pair_pressed = kit_workspace_authoring_entry_chord_pressed(key, mod_bits, c_down, v_down);
    authoring_alt_only = ((mod_bits & KIT_WORKSPACE_AUTHORING_MOD_ALT) != 0u) &&
                         ((mod_bits & (KIT_WORKSPACE_AUTHORING_MOD_SHIFT |
                                       KIT_WORKSPACE_AUTHORING_MOD_CTRL |
                                       KIT_WORKSPACE_AUTHORING_MOD_GUI)) == 0u);
    if (authoring_alt_only &&
        (key == KIT_WORKSPACE_AUTHORING_KEY_C || key == KIT_WORKSPACE_AUTHORING_KEY_V) &&
        ctx->authoring_host.entry_chord_armed_key != KIT_WORKSPACE_AUTHORING_KEY_UNKNOWN &&
        ctx->authoring_host.entry_chord_armed_key != (uint8_t)key) {
        chord_pair_pressed = 1;
    }
    if (chord_pair_pressed) {
        if (drawing_program_authoring_host_active(ctx)) {
            (void)drawing_program_authoring_host_cancel(ctx);
        } else {
            (void)drawing_program_authoring_host_enter(ctx);
        }
        ctx->authoring_host.entry_chord_armed_key = KIT_WORKSPACE_AUTHORING_KEY_UNKNOWN;
        drawing_program_authoring_note_consumed(ctx);
        return 1;
    }

    if (authoring_alt_only &&
        (key == KIT_WORKSPACE_AUTHORING_KEY_C || key == KIT_WORKSPACE_AUTHORING_KEY_V)) {
        ctx->authoring_host.entry_chord_armed_key = (uint8_t)key;
        drawing_program_authoring_note_consumed(ctx);
        return 1;
    }

    if (!drawing_program_authoring_host_active(ctx)) {
        return 0;
    }
    if (key == KIT_WORKSPACE_AUTHORING_KEY_ESCAPE) {
        (void)drawing_program_authoring_host_cancel(ctx);
        drawing_program_authoring_note_consumed(ctx);
        return 1;
    }
    if (key == KIT_WORKSPACE_AUTHORING_KEY_ENTER) {
        (void)drawing_program_authoring_host_apply(ctx);
        drawing_program_authoring_note_consumed(ctx);
        return 1;
    }
    if (drawing_program_authoring_host_font_theme_overlay_active(ctx) &&
        ((mod_bits & (KIT_WORKSPACE_AUTHORING_MOD_CTRL | KIT_WORKSPACE_AUTHORING_MOD_GUI)) != 0u)) {
        if (key == KIT_WORKSPACE_AUTHORING_KEY_DIGIT_0) {
            (void)drawing_program_authoring_host_reset_font_zoom(ctx);
            drawing_program_authoring_note_consumed(ctx);
            return 1;
        }
        if (event->key.keysym.sym == SDLK_EQUALS || event->key.keysym.sym == SDLK_PLUS ||
            event->key.keysym.sym == SDLK_KP_PLUS) {
            (void)drawing_program_authoring_host_adjust_font_zoom(ctx, 1);
            drawing_program_authoring_note_consumed(ctx);
            return 1;
        }
        if (event->key.keysym.sym == SDLK_MINUS || event->key.keysym.sym == SDLK_KP_MINUS) {
            (void)drawing_program_authoring_host_adjust_font_zoom(ctx, -1);
            drawing_program_authoring_note_consumed(ctx);
            return 1;
        }
    }
    {
        const char *trigger = kit_workspace_authoring_trigger_from_key(key, mod_bits);
        if (trigger && strcmp(trigger, "tab") == 0) {
            (void)drawing_program_authoring_host_cycle_overlay(ctx);
            drawing_program_authoring_note_consumed(ctx);
            return 1;
        }
        if (trigger) {
            drawing_program_authoring_note_consumed(ctx);
            return 1;
        }
    }
    return 0;
}
