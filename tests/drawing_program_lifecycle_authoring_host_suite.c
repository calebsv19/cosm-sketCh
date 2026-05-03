#include "drawing_program_lifecycle_authoring_host_suite.h"

#include <SDL2/SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core_layout.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_snapshot.h"
#include "drawing_program/drawing_program_visual_authoring_chrome.h"
#include "drawing_program_lifecycle_test_support.h"

static void authoring_key_event(SDL_Event *event, uint32_t type, SDL_Keycode key, uint16_t mod) {
    if (!event) {
        return;
    }
    SDL_zero(*event);
    event->type = type;
    event->key.type = type;
    event->key.keysym.sym = key;
    event->key.keysym.scancode = SDL_GetScancodeFromKey(key);
    event->key.keysym.mod = mod;
}

static void authoring_physical_key_event(SDL_Event *event,
                                         uint32_t type,
                                         SDL_Scancode scancode,
                                         SDL_Keycode key,
                                         uint16_t mod) {
    if (!event) {
        return;
    }
    SDL_zero(*event);
    event->type = type;
    event->key.type = type;
    event->key.keysym.sym = key;
    event->key.keysym.scancode = scancode;
    event->key.keysym.mod = mod;
}

static uint32_t authoring_first_split_node(const DrawingProgramAppContext *ctx) {
    uint32_t i;
    if (!ctx) {
        return 0u;
    }
    for (i = 0u; i < ctx->pane_host.node_count; ++i) {
        if (ctx->pane_host.nodes[i].type == CORE_PANE_NODE_SPLIT) {
            return i;
        }
    }
    return ctx->pane_host.node_count;
}

int drawing_program_lifecycle_run_authoring_host_suite(void) {
    static DrawingProgramAppContext ctx;
    static DrawingProgramAppContext applied_load_ctx;
    static DrawingProgramAppContext draft_load_ctx;
    SDL_Event event;
    char pane_rows[8][DRAWING_PROGRAM_AUTHORING_CHROME_ROW_TEXT_MAX];
    uint32_t pane_row_count = 0u;
    uint32_t split_node_index;
    uint32_t original_module_type_id;
    float original_ratio;
    float draft_ratio;
    float applied_ratio;
    float unsaved_draft_ratio;
    int8_t original_font_zoom;
    uint64_t revision_before_apply;
    const char *applied_pack_path = "/tmp/drawing_program_authoring_applied.pack";
    const char *active_draft_pack_path = "/tmp/drawing_program_authoring_active_draft.pack";
    char arg0[] = "drawing_program_authoring_host_test";
    char arg1[] = "--headless";
    char arg2[] = "--smoke-frames";
    char arg3[] = "1";
    char arg4[] = "--no-persist";
    char *argv[] = { arg0, arg1, arg2, arg3, arg4, 0 };

    if (!expect_ok(drawing_program_app_bootstrap(&ctx, 5, argv), "authoring_host_bootstrap")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&ctx), "authoring_host_config_load")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&ctx), "authoring_host_state_seed")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&ctx), "authoring_host_subsystems_init")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&ctx), "authoring_host_runtime_start")) {
        return 1;
    }
    if (drawing_program_authoring_host_active(&ctx) ||
        ctx.pane_host.layout_state.mode != CORE_LAYOUT_MODE_RUNTIME) {
        fprintf(stderr, "authoring_host_test: expected runtime mode at boot\n");
        return 1;
    }

    authoring_key_event(&event, SDL_KEYDOWN, SDLK_c, KMOD_NONE);
    if (drawing_program_authoring_host_handle_sdl_event(&ctx, &event)) {
        fprintf(stderr, "authoring_host_test: plain C should fall through while inactive\n");
        return 1;
    }
    if (drawing_program_authoring_host_active(&ctx)) {
        fprintf(stderr, "authoring_host_test: plain C should not enter authoring\n");
        return 1;
    }

    authoring_key_event(&event, SDL_KEYDOWN, SDLK_c, KMOD_ALT);
    if (!drawing_program_authoring_host_handle_sdl_event(&ctx, &event) ||
        drawing_program_authoring_host_active(&ctx)) {
        fprintf(stderr, "authoring_host_test: Alt+C should be consumed as partial chord only\n");
        return 1;
    }
    authoring_key_event(&event, SDL_KEYDOWN, SDLK_v, KMOD_ALT);
    if (!drawing_program_authoring_host_handle_sdl_event(&ctx, &event) ||
        !drawing_program_authoring_host_active(&ctx) ||
        ctx.authoring_host.enter_count != 1u ||
        ctx.pane_host.layout_state.mode != CORE_LAYOUT_MODE_AUTHORING ||
        !ctx.authoring_host.draft_baseline_valid ||
        ctx.pane_host.layout_state.has_pending_changes) {
        fprintf(stderr, "authoring_host_test: Alt+C+V should enter authoring\n");
        return 1;
    }

    split_node_index = authoring_first_split_node(&ctx);
    if (split_node_index >= ctx.pane_host.node_count) {
        fprintf(stderr, "authoring_host_test: expected at least one split node for draft lifecycle\n");
        return 1;
    }
    original_ratio = ctx.pane_host.nodes[split_node_index].ratio_01;
    draft_ratio = original_ratio > 0.55f ? original_ratio - 0.05f : original_ratio + 0.05f;
    ctx.pane_host.nodes[split_node_index].ratio_01 = draft_ratio;
    if (!expect_ok(drawing_program_pane_host_rebuild(&ctx), "authoring_host_draft_rebuild") ||
        !expect_ok(drawing_program_authoring_host_mark_draft_changed(&ctx), "authoring_host_mark_draft_changed")) {
        return 1;
    }
    if (!ctx.pane_host.layout_state.has_pending_changes ||
        ctx.authoring_host.draft_change_count != 1u) {
        fprintf(stderr, "authoring_host_test: draft mutation should mark pending changes\n");
        return 1;
    }

    original_font_zoom = ctx.ui.font_zoom_step;
    authoring_key_event(&event, SDL_KEYDOWN, SDLK_TAB, KMOD_NONE);
    if (!drawing_program_authoring_host_handle_sdl_event(&ctx, &event) ||
        !drawing_program_authoring_host_active(&ctx) ||
        !drawing_program_authoring_host_font_theme_overlay_active(&ctx) ||
        ctx.authoring_host.overlay_cycle_count != 1u) {
        fprintf(stderr, "authoring_host_test: Tab should enter font/theme overlay while authoring stays active\n");
        return 1;
    }
    authoring_key_event(&event, SDL_KEYDOWN, SDLK_EQUALS, KMOD_CTRL);
    if (!drawing_program_authoring_host_handle_sdl_event(&ctx, &event) ||
        ctx.ui.font_zoom_step != original_font_zoom + 1 ||
        !ctx.authoring_host.font_theme_pending_changes ||
        !ctx.pane_host.layout_state.has_pending_changes) {
        fprintf(stderr, "authoring_host_test: font/theme overlay should consume Ctrl+= and mark draft\n");
        return 1;
    }

    authoring_key_event(&event, SDL_KEYDOWN, SDLK_ESCAPE, KMOD_NONE);
    if (!drawing_program_authoring_host_handle_sdl_event(&ctx, &event) ||
        drawing_program_authoring_host_active(&ctx) ||
        ctx.authoring_host.cancel_count != 1u ||
        ctx.authoring_host.draft_baseline_valid ||
        ctx.pane_host.layout_state.mode != CORE_LAYOUT_MODE_RUNTIME ||
        ctx.pane_host.layout_state.has_pending_changes ||
        ctx.pane_host.nodes[split_node_index].ratio_01 != original_ratio ||
        ctx.ui.font_zoom_step != original_font_zoom) {
        fprintf(stderr, "authoring_host_test: Esc should cancel draft authoring and font/theme changes\n");
        return 1;
    }

    authoring_physical_key_event(&event, SDL_KEYDOWN, SDL_SCANCODE_C, SDLK_UNKNOWN, KMOD_ALT);
    if (!drawing_program_authoring_host_handle_sdl_event(&ctx, &event) ||
        drawing_program_authoring_host_active(&ctx)) {
        fprintf(stderr, "authoring_host_test: physical Alt+C should arm partial chord\n");
        return 1;
    }
    authoring_physical_key_event(&event, SDL_KEYUP, SDL_SCANCODE_C, SDLK_UNKNOWN, KMOD_ALT);
    (void)drawing_program_authoring_host_handle_sdl_event(&ctx, &event);
    authoring_physical_key_event(&event, SDL_KEYDOWN, SDL_SCANCODE_V, SDLK_UNKNOWN, KMOD_ALT);
    if (!drawing_program_authoring_host_handle_sdl_event(&ctx, &event) ||
        !drawing_program_authoring_host_active(&ctx) ||
        ctx.authoring_host.enter_count != 2u) {
        fprintf(stderr, "authoring_host_test: sequential physical Alt+C then Alt+V should enter authoring\n");
        return 1;
    }
    pane_row_count = drawing_program_visual_authoring_chrome_build_pane_rows(&ctx, pane_rows, 8u);
    if (pane_row_count != 4u ||
        strstr(pane_rows[0], "Menu Bar") == 0 ||
        strstr(pane_rows[1], "Palette") == 0 ||
        strstr(pane_rows[2], "Canvas") == 0 ||
        strstr(pane_rows[3], "Inspector") == 0) {
        fprintf(stderr,
                "authoring_host_test: authoring chrome pane/module rows did not match expected bindings count=%u\n",
                (unsigned)pane_row_count);
        return 1;
    }

    if (drawing_program_visual_authoring_chrome_hit_test(800, 600, &ctx, 300, 22) !=
            DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_APPLY ||
        drawing_program_visual_authoring_chrome_hit_test(800, 600, &ctx, 370, 22) !=
            DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CANCEL) {
        fprintf(stderr, "authoring_host_test: chrome Apply/Cancel hit test should resolve title-bar controls\n");
        return 1;
    }
    authoring_key_event(&event, SDL_KEYDOWN, SDLK_TAB, KMOD_NONE);
    if (!drawing_program_authoring_host_handle_sdl_event(&ctx, &event) ||
        !drawing_program_authoring_host_font_theme_overlay_active(&ctx) ||
        drawing_program_visual_authoring_chrome_hit_test(800, 600, &ctx, 60, 252) !=
            DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_ZOOM_DEC ||
        drawing_program_visual_authoring_chrome_hit_test(800, 600, &ctx, 340, 365) !=
            DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_MIDNIGHT) {
        fprintf(stderr, "authoring_host_test: font/theme overlay hit tests should resolve controls\n");
        return 1;
    }
    authoring_key_event(&event, SDL_KEYDOWN, SDLK_TAB, KMOD_NONE);
    if (!drawing_program_authoring_host_handle_sdl_event(&ctx, &event) ||
        !drawing_program_authoring_host_pane_overlay_active(&ctx)) {
        fprintf(stderr, "authoring_host_test: second Tab should return to pane authoring overlay\n");
        return 1;
    }

    revision_before_apply = ctx.pane_host.layout_state.active_revision;
    original_ratio = ctx.pane_host.nodes[split_node_index].ratio_01;
    draft_ratio = original_ratio > 0.55f ? original_ratio - 0.05f : original_ratio + 0.05f;
    ctx.pane_host.nodes[split_node_index].ratio_01 = draft_ratio;
    if (!expect_ok(drawing_program_pane_host_rebuild(&ctx), "authoring_host_apply_draft_rebuild") ||
        !expect_ok(drawing_program_authoring_host_mark_draft_changed(&ctx), "authoring_host_apply_mark_draft_changed")) {
        return 1;
    }
    authoring_key_event(&event, SDL_KEYDOWN, SDLK_RETURN, KMOD_NONE);
    if (!drawing_program_authoring_host_handle_sdl_event(&ctx, &event) ||
        drawing_program_authoring_host_active(&ctx) ||
        ctx.authoring_host.apply_count != 1u ||
        ctx.authoring_host.draft_baseline_valid ||
        ctx.pane_host.layout_state.mode != CORE_LAYOUT_MODE_RUNTIME ||
        ctx.pane_host.layout_state.has_pending_changes ||
        ctx.pane_host.layout_state.active_revision != revision_before_apply + 1u ||
        ctx.pane_host.nodes[split_node_index].ratio_01 != draft_ratio) {
        fprintf(stderr, "authoring_host_test: Enter should apply draft authoring changes\n");
        return 1;
    }
    applied_ratio = ctx.pane_host.nodes[split_node_index].ratio_01;
    if (!expect_ok(drawing_program_snapshot_save(&ctx, applied_pack_path),
                   "authoring_host_applied_snapshot_save")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_bootstrap(&applied_load_ctx, 5, argv),
                   "authoring_host_applied_load_bootstrap") ||
        !expect_ok(drawing_program_app_config_load(&applied_load_ctx),
                   "authoring_host_applied_load_config") ||
        !expect_ok(drawing_program_app_state_seed(&applied_load_ctx),
                   "authoring_host_applied_load_seed") ||
        !expect_ok(drawing_program_snapshot_load(&applied_load_ctx, applied_pack_path),
                   "authoring_host_applied_snapshot_load")) {
        return 1;
    }
    if (applied_load_ctx.pane_host.nodes[split_node_index].ratio_01 != applied_ratio ||
        applied_load_ctx.pane_host.layout_state.mode != CORE_LAYOUT_MODE_RUNTIME ||
        applied_load_ctx.pane_host.layout_state.has_pending_changes) {
        fprintf(stderr, "authoring_host_test: applied authoring pane state should persist as runtime state\n");
        return 1;
    }

    if (!expect_ok(drawing_program_authoring_host_enter(&ctx), "authoring_host_enter_for_active_draft_save")) {
        return 1;
    }
    original_ratio = ctx.pane_host.nodes[split_node_index].ratio_01;
    unsaved_draft_ratio = original_ratio > 0.55f ? original_ratio - 0.07f : original_ratio + 0.07f;
    ctx.pane_host.nodes[split_node_index].ratio_01 = unsaved_draft_ratio;
    if (!expect_ok(drawing_program_pane_host_rebuild(&ctx), "authoring_host_active_draft_rebuild") ||
        !expect_ok(drawing_program_authoring_host_mark_draft_changed(&ctx),
                   "authoring_host_active_draft_mark_changed") ||
        !expect_ok(drawing_program_snapshot_save(&ctx, active_draft_pack_path),
                   "authoring_host_active_draft_snapshot_save")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_bootstrap(&draft_load_ctx, 5, argv),
                   "authoring_host_draft_load_bootstrap") ||
        !expect_ok(drawing_program_app_config_load(&draft_load_ctx),
                   "authoring_host_draft_load_config") ||
        !expect_ok(drawing_program_app_state_seed(&draft_load_ctx),
                   "authoring_host_draft_load_seed") ||
        !expect_ok(drawing_program_snapshot_load(&draft_load_ctx, active_draft_pack_path),
                   "authoring_host_active_draft_snapshot_load")) {
        return 1;
    }
    if (draft_load_ctx.pane_host.nodes[split_node_index].ratio_01 != original_ratio ||
        draft_load_ctx.pane_host.nodes[split_node_index].ratio_01 == unsaved_draft_ratio ||
        draft_load_ctx.pane_host.layout_state.mode != CORE_LAYOUT_MODE_RUNTIME ||
        draft_load_ctx.pane_host.layout_state.has_pending_changes) {
        fprintf(stderr, "authoring_host_test: active draft snapshot save should persist accepted baseline only\n");
        return 1;
    }
    if (!expect_ok(drawing_program_authoring_host_cancel(&ctx),
                   "authoring_host_cancel_after_active_draft_save")) {
        return 1;
    }

    authoring_key_event(&event, SDL_KEYDOWN, SDLK_TAB, KMOD_NONE);
    if (drawing_program_authoring_host_handle_sdl_event(&ctx, &event)) {
        fprintf(stderr, "authoring_host_test: Tab should fall through while inactive\n");
        return 1;
    }

    authoring_key_event(&event, SDL_KEYDOWN, SDLK_c, KMOD_ALT);
    if (!drawing_program_authoring_host_handle_sdl_event(&ctx, &event)) {
        fprintf(stderr, "authoring_host_test: second Alt+C partial chord should consume\n");
        return 1;
    }
    authoring_key_event(&event, SDL_KEYDOWN, SDLK_v, KMOD_ALT);
    if (!drawing_program_authoring_host_handle_sdl_event(&ctx, &event) ||
        !drawing_program_authoring_host_active(&ctx) ||
        ctx.authoring_host.enter_count != 4u) {
        fprintf(stderr, "authoring_host_test: second Alt+C+V should enter authoring\n");
        return 1;
    }
    original_module_type_id = ctx.pane_host.module_bindings[0].module_type_id;
    ctx.pane_host.module_bindings[0].module_type_id = 9999u;
    if (!expect_ok(drawing_program_authoring_host_mark_draft_changed(&ctx),
                   "authoring_host_invalid_binding_mark_draft_changed")) {
        return 1;
    }
    if (drawing_program_authoring_host_apply(&ctx).code == CORE_OK ||
        !drawing_program_authoring_host_active(&ctx) ||
        !ctx.authoring_host.draft_baseline_valid ||
        ctx.pane_host.layout_state.mode != CORE_LAYOUT_MODE_AUTHORING) {
        fprintf(stderr, "authoring_host_test: invalid module binding should reject apply and keep draft active\n");
        return 1;
    }
    authoring_key_event(&event, SDL_KEYDOWN, SDLK_c, KMOD_ALT);
    if (!drawing_program_authoring_host_handle_sdl_event(&ctx, &event)) {
        fprintf(stderr, "authoring_host_test: active Alt+C partial chord should consume\n");
        return 1;
    }
    authoring_key_event(&event, SDL_KEYDOWN, SDLK_v, KMOD_ALT);
    if (!drawing_program_authoring_host_handle_sdl_event(&ctx, &event) ||
        drawing_program_authoring_host_active(&ctx) ||
        ctx.authoring_host.cancel_count != 3u) {
        fprintf(stderr, "authoring_host_test: active Alt+C+V should cancel authoring\n");
        return 1;
    }
    if (ctx.pane_host.module_bindings[0].module_type_id != original_module_type_id) {
        fprintf(stderr, "authoring_host_test: cancel should restore invalid module binding draft\n");
        return 1;
    }

    if (ctx.authoring_host.consumed_key_count < 7u) {
        fprintf(stderr,
                "authoring_host_test: expected consumed authoring key count >=7 got=%u\n",
                (unsigned)ctx.authoring_host.consumed_key_count);
        return 1;
    }
    if (!expect_ok(drawing_program_app_shutdown(&applied_load_ctx),
                   "authoring_host_applied_load_shutdown") ||
        !expect_ok(drawing_program_app_shutdown(&draft_load_ctx),
                   "authoring_host_draft_load_shutdown")) {
        return 1;
    }
    return 0;
}
