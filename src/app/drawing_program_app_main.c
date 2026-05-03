#include "drawing_program/drawing_program_app_main.h"

#include <stdio.h>
#include <stdlib.h>

typedef enum DrawingProgramAppStage {
    DRAWING_PROGRAM_APP_STAGE_INIT = 0,
    DRAWING_PROGRAM_APP_STAGE_BOOTSTRAPPED,
    DRAWING_PROGRAM_APP_STAGE_CONFIG_LOADED,
    DRAWING_PROGRAM_APP_STAGE_STATE_SEEDED,
    DRAWING_PROGRAM_APP_STAGE_SUBSYSTEMS_READY,
    DRAWING_PROGRAM_APP_STAGE_RUNTIME_STARTED,
    DRAWING_PROGRAM_APP_STAGE_LOOP_COMPLETED,
    DRAWING_PROGRAM_APP_STAGE_SHUTDOWN_COMPLETED
} DrawingProgramAppStage;

static int drawing_program_transition_stage(DrawingProgramAppStage *stage,
                                            DrawingProgramAppStage expected,
                                            DrawingProgramAppStage next,
                                            const char *label) {
    if (!stage) {
        return 0;
    }
    if (*stage != expected) {
        fprintf(stderr,
                "drawing_program: lifecycle order violation label=%s expected=%d actual=%d next=%d\n",
                label ? label : "unknown",
                (int)expected,
                (int)(*stage),
                (int)next);
        return 0;
    }
    *stage = next;
    return 1;
}

static void drawing_program_lifecycle_note(const DrawingProgramAppContext *ctx, const char *stage_name) {
    if (!ctx || !ctx->session.print_lifecycle) {
        return;
    }
    printf("drawing_program lifecycle: %s\n", stage_name ? stage_name : "unknown");
}

int drawing_program_app_main(int argc, char **argv) {
    DrawingProgramAppContext *app = 0;
    DrawingProgramAppStage stage = DRAWING_PROGRAM_APP_STAGE_INIT;
    CoreResult result;

    app = (DrawingProgramAppContext *)calloc(1u, sizeof(*app));
    if (!app) {
        fprintf(stderr, "drawing_program: failed to allocate app context\n");
        return 1;
    }

    result = drawing_program_app_bootstrap(app, argc, argv);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: bootstrap failed code=%d message=%s\n", (int)result.code, result.message);
        free(app);
        return 1;
    }
    drawing_program_lifecycle_note(app, "bootstrap");
    if (!drawing_program_transition_stage(&stage,
                                          DRAWING_PROGRAM_APP_STAGE_INIT,
                                          DRAWING_PROGRAM_APP_STAGE_BOOTSTRAPPED,
                                          "bootstrap")) {
        free(app);
        return 1;
    }

    result = drawing_program_app_config_load(app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: config_load failed code=%d message=%s\n", (int)result.code, result.message);
        free(app);
        return 1;
    }
    drawing_program_lifecycle_note(app, "config_load");
    if (!drawing_program_transition_stage(&stage,
                                          DRAWING_PROGRAM_APP_STAGE_BOOTSTRAPPED,
                                          DRAWING_PROGRAM_APP_STAGE_CONFIG_LOADED,
                                          "config_load")) {
        free(app);
        return 1;
    }

    result = drawing_program_app_state_seed(app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: state_seed failed code=%d message=%s\n", (int)result.code, result.message);
        free(app);
        return 1;
    }
    drawing_program_lifecycle_note(app, "state_seed");
    if (!drawing_program_transition_stage(&stage,
                                          DRAWING_PROGRAM_APP_STAGE_CONFIG_LOADED,
                                          DRAWING_PROGRAM_APP_STAGE_STATE_SEEDED,
                                          "state_seed")) {
        free(app);
        return 1;
    }

    result = drawing_program_app_subsystems_init(app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: subsystems_init failed code=%d message=%s\n", (int)result.code, result.message);
        free(app);
        return 1;
    }
    drawing_program_lifecycle_note(app, "subsystems_init");
    if (!drawing_program_transition_stage(&stage,
                                          DRAWING_PROGRAM_APP_STAGE_STATE_SEEDED,
                                          DRAWING_PROGRAM_APP_STAGE_SUBSYSTEMS_READY,
                                          "subsystems_init")) {
        free(app);
        return 1;
    }

    result = drawing_program_runtime_start(app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: runtime_start failed code=%d message=%s\n", (int)result.code, result.message);
        free(app);
        return 1;
    }
    drawing_program_lifecycle_note(app, "runtime_start");
    if (!drawing_program_transition_stage(&stage,
                                          DRAWING_PROGRAM_APP_STAGE_SUBSYSTEMS_READY,
                                          DRAWING_PROGRAM_APP_STAGE_RUNTIME_STARTED,
                                          "runtime_start")) {
        free(app);
        return 1;
    }

    result = drawing_program_app_run_loop(app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: run_loop failed code=%d message=%s\n", (int)result.code, result.message);
        free(app);
        return 1;
    }
    drawing_program_lifecycle_note(app, "run_loop");
    if (!drawing_program_transition_stage(&stage,
                                          DRAWING_PROGRAM_APP_STAGE_RUNTIME_STARTED,
                                          DRAWING_PROGRAM_APP_STAGE_LOOP_COMPLETED,
                                          "run_loop")) {
        free(app);
        return 1;
    }

    result = drawing_program_app_shutdown(app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: shutdown failed code=%d message=%s\n", (int)result.code, result.message);
        free(app);
        return 1;
    }
    drawing_program_lifecycle_note(app, "shutdown");
    if (!drawing_program_transition_stage(&stage,
                                          DRAWING_PROGRAM_APP_STAGE_LOOP_COMPLETED,
                                          DRAWING_PROGRAM_APP_STAGE_SHUTDOWN_COMPLETED,
                                          "shutdown")) {
        free(app);
        return 1;
    }

    if (app->session.print_lifecycle) {
        printf("drawing_program lifecycle complete: frames=%llu headless=%u events=%llu actions=%llu "
               "route[g=%llu p=%llu f=%llu] invalidate[target=%llu full=%llu] viewport_probe_ok=%llu "
               "render[frames=%llu visible_layers=%llu full=%llu target=%llu module_calls=%llu]\n",
               (unsigned long long)app->runtime.frame_counter,
               (unsigned)app->session.headless,
               (unsigned long long)app->runtime.input_events_processed,
               (unsigned long long)app->runtime.input_actions_emitted,
               (unsigned long long)app->runtime.routed_global_total,
               (unsigned long long)app->runtime.routed_pane_total,
               (unsigned long long)app->runtime.routed_fallback_total,
               (unsigned long long)app->runtime.invalidation_target_total,
               (unsigned long long)app->runtime.invalidation_full_total,
               (unsigned long long)app->runtime.viewport_sample_probe_success_total,
               (unsigned long long)app->runtime.render_frames_projected_total,
               (unsigned long long)app->runtime.render_layers_visible_total,
               (unsigned long long)app->runtime.render_full_redraw_total,
               (unsigned long long)app->runtime.render_target_redraw_total,
               (unsigned long long)app->runtime.render_module_calls_total);
    }

    free(app);
    return 0;
}
