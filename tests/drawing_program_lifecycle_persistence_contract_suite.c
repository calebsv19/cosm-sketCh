#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_project_state.h"
#include "drawing_program/drawing_program_texture_project_session.h"
#include "drawing_program_lifecycle_test_support.h"

static int persistence_contract_prepare_dir(const char *path) {
    if (mkdir(path, 0775) != 0 && access(path, F_OK) != 0) {
        fprintf(stderr, "lifecycle_test: failed to create persistence contract dir %s\n", path);
        return 0;
    }
    return 1;
}

static int persistence_contract_seed_project_sample(DrawingProgramAppContext *ctx,
                                                    uint32_t x,
                                                    uint32_t y,
                                                    uint8_t sample_value) {
    if (!expect_ok(drawing_program_document_sample_write(&ctx->document, x, y, sample_value, 0),
                   "persistence_contract_seed_project_sample")) {
        return 0;
    }
    return 1;
}

int drawing_program_lifecycle_run_persistence_contract_suite(void) {
    static DrawingProgramAppContext save_ctx;
    static DrawingProgramAppContext boot_ctx;
    static DrawingProgramAppContext prefs_ctx;
    static DrawingProgramAppContext load_ctx;
    static DrawingProgramAppContext override_ctx;
    static DrawingProgramAppContext bad_root_ctx;
    static DrawingProgramAppContext long_env_root_ctx;
    char suite_root[] = "/tmp/drawing_program_persistence_contract_suite";
    char runtime_root[] = "/tmp/drawing_program_persistence_contract_suite/runtime";
    char input_root[] = "/tmp/drawing_program_persistence_contract_suite/input";
    char output_root[] = "/tmp/drawing_program_persistence_contract_suite/output";
    char scene_root[] = "/tmp/drawing_program_persistence_contract_suite/scenes";
    char alt_input_root[] = "/tmp/drawing_program_persistence_contract_suite/alt_input";
    char alt_output_root[] = "/tmp/drawing_program_persistence_contract_suite/alt_output";
    char alt_scene_root[] = "/tmp/drawing_program_persistence_contract_suite/alt_scenes";
    char expected_project_path[] = "/tmp/drawing_program_persistence_contract_suite/input/icon_project_001.pack";
    char expected_preset_path[] = "/tmp/drawing_program_persistence_contract_suite/runtime/last_session.pack";
    char expected_alt_project_path[] = "/tmp/drawing_program_persistence_contract_suite/alt_input/persisted_current.pack";
    char blocked_runtime_root[] = "/tmp/drawing_program_persistence_contract_suite/blocked_runtime_root";
    char arg0[] = "drawing_program_persistence_contract_save";
    char arg1[] = "--headless";
    char arg2[] = "--smoke-frames";
    char arg3[] = "1";
    char arg4[] = "--runtime-root";
    char arg5[] = "/tmp/drawing_program_persistence_contract_suite/runtime";
    char arg6[] = "--input-root";
    char arg7[] = "/tmp/drawing_program_persistence_contract_suite/input";
    char arg8[] = "--output-root";
    char arg9[] = "/tmp/drawing_program_persistence_contract_suite/output";
    char *argv[] = { arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, 0 };
    char runtime_only_arg0[] = "drawing_program_persistence_contract_load";
    char runtime_only_arg1[] = "--headless";
    char runtime_only_arg2[] = "--smoke-frames";
    char runtime_only_arg3[] = "1";
    char runtime_only_arg4[] = "--runtime-root";
    char runtime_only_arg5[] = "/tmp/drawing_program_persistence_contract_suite/runtime";
    char *runtime_only_argv[] = {
        runtime_only_arg0, runtime_only_arg1, runtime_only_arg2, runtime_only_arg3, runtime_only_arg4, runtime_only_arg5, 0
    };
    char bad_arg0[] = "drawing_program_persistence_contract_bad_root";
    char bad_arg1[] = "--headless";
    char bad_arg2[] = "--runtime-root";
    char bad_arg3[] = "/tmp/drawing_program_persistence_contract_suite/blocked_runtime_root";
    char bad_arg4[] = "--no-persist";
    char *bad_root_argv[] = { bad_arg0, bad_arg1, bad_arg2, bad_arg3, bad_arg4, 0 };
    char long_env_arg0[] = "drawing_program_persistence_contract_long_env_root";
    char long_env_arg1[] = "--headless";
    char long_env_arg2[] = "--no-persist";
    char *long_env_argv[] = { long_env_arg0, long_env_arg1, long_env_arg2, 0 };
    char long_runtime_root[640];
    FILE *blocked_file = 0;
    CoreResult bad_root_result;
    CoreResult long_env_root_result;
    uint32_t project_sample_x = 13u;
    uint32_t project_sample_y = 17u;
    uint32_t active_surface_sample_x = 7u;
    uint32_t active_surface_sample_y = 9u;
    uint8_t project_sample_value = 91u;
    uint8_t preset_sample_value = 203u;
    uint8_t active_surface_sample_value = 144u;
    uint8_t loaded_sample_value = 0u;
    uint8_t expected_boot_sample_value = 0u;
    uint8_t expected_active_surface_sample_value = 0u;
    uint32_t imported_surface_index = 0u;
    uint32_t loaded_surface_count = 0u;

    (void)mkdir(suite_root, 0775);
    if (!persistence_contract_prepare_dir(runtime_root) ||
        !persistence_contract_prepare_dir(input_root) ||
        !persistence_contract_prepare_dir(output_root) ||
        !persistence_contract_prepare_dir(scene_root) ||
        !persistence_contract_prepare_dir(alt_input_root) ||
        !persistence_contract_prepare_dir(alt_output_root) ||
        !persistence_contract_prepare_dir(alt_scene_root)) {
        return 1;
    }
    (void)unlink(expected_project_path);
    (void)unlink(expected_preset_path);
    (void)unlink(expected_alt_project_path);
    (void)unlink("/tmp/drawing_program_persistence_contract_suite/runtime/session_prefs_v1.txt");
    (void)unlink(blocked_runtime_root);

    blocked_file = fopen(blocked_runtime_root, "wb");
    if (!blocked_file) {
        fprintf(stderr, "lifecycle_test: failed to seed blocked runtime root file %s\n", blocked_runtime_root);
        return 1;
    }
    (void)fputs("not a directory\n", blocked_file);
    (void)fclose(blocked_file);
    if (!expect_ok(drawing_program_app_bootstrap(&bad_root_ctx, 5, bad_root_argv),
                   "persistence_contract_bad_root_bootstrap")) {
        return 1;
    }
    bad_root_result = drawing_program_app_config_load(&bad_root_ctx);
    if (bad_root_result.code != CORE_ERR_IO ||
        !bad_root_result.message ||
        strstr(bad_root_result.message, "role=runtime_root") == 0 ||
        strstr(bad_root_result.message, blocked_runtime_root) == 0 ||
        strstr(bad_root_result.message, "not a directory") == 0) {
        fprintf(stderr,
                "lifecycle_test: expected runtime root diagnostic with role/path got code=%d message=%s\n",
                (int)bad_root_result.code,
                bad_root_result.message ? bad_root_result.message : "(null)");
        return 1;
    }
    (void)unlink(blocked_runtime_root);

    (void)snprintf(long_runtime_root,
                   sizeof(long_runtime_root),
                   "/tmp/drawing_program_persistence_contract_suite/%0560d",
                   1);
    if (setenv("DRAWING_PROGRAM_RUNTIME_DIR", long_runtime_root, 1) != 0) {
        fprintf(stderr, "lifecycle_test: failed to seed long runtime env root\n");
        return 1;
    }
    if (!expect_ok(drawing_program_app_bootstrap(&long_env_root_ctx,
                                                 3,
                                                 long_env_argv),
                   "persistence_contract_long_env_root_bootstrap")) {
        unsetenv("DRAWING_PROGRAM_RUNTIME_DIR");
        return 1;
    }
    long_env_root_result = drawing_program_app_config_load(&long_env_root_ctx);
    unsetenv("DRAWING_PROGRAM_RUNTIME_DIR");
    if (long_env_root_result.code != CORE_ERR_INVALID_ARG ||
        !long_env_root_result.message ||
        strstr(long_env_root_result.message, "role=runtime_root") == 0 ||
        strstr(long_env_root_result.message, "too long") == 0) {
        fprintf(stderr,
                "lifecycle_test: expected long runtime root to fail closed got code=%d message=%s\n",
                (int)long_env_root_result.code,
                long_env_root_result.message ? long_env_root_result.message : "(null)");
        return 1;
    }

    if (!expect_ok(drawing_program_app_bootstrap(&save_ctx, 10, argv), "persistence_contract_bootstrap_save")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&save_ctx), "persistence_contract_config_save")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&save_ctx), "persistence_contract_state_seed_save")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&save_ctx), "persistence_contract_subsystems_save")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&save_ctx), "persistence_contract_runtime_start_save")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_set_scene_authored_root(&save_ctx, scene_root),
                   "persistence_contract_set_scene_root")) {
        return 1;
    }
    if (!save_ctx.session.project_path || strcmp(save_ctx.session.project_path, expected_project_path) != 0) {
        fprintf(stderr,
                "lifecycle_test: expected initial persistence contract project path %s got %s\n",
                expected_project_path,
                save_ctx.session.project_path ? save_ctx.session.project_path : "(null)");
        return 1;
    }
    if (!persistence_contract_seed_project_sample(&save_ctx, project_sample_x, project_sample_y, project_sample_value)) {
        return 1;
    }
    if (!expect_ok(drawing_program_texture_project_session_add_surface_from_active(&save_ctx,
                                                                                   "Imported Surface",
                                                                                   &imported_surface_index),
                   "persistence_contract_add_surface")) {
        return 1;
    }
    if (!expect_ok(drawing_program_texture_project_session_select_surface(&save_ctx, imported_surface_index),
                   "persistence_contract_select_surface")) {
        return 1;
    }
    if (!persistence_contract_seed_project_sample(
            &save_ctx, active_surface_sample_x, active_surface_sample_y, active_surface_sample_value)) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_save_current(&save_ctx), "persistence_contract_save_project")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_load_current(&save_ctx), "persistence_contract_reload_saved_project")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&save_ctx.document,
                                                        project_sample_x,
                                                        project_sample_y,
                                                        &expected_boot_sample_value),
                   "persistence_contract_read_expected_boot_sample")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&save_ctx.document,
                                                        active_surface_sample_x,
                                                        active_surface_sample_y,
                                                        &expected_active_surface_sample_value),
                   "persistence_contract_read_expected_active_surface_sample")) {
        return 1;
    }
    save_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_ERASER;
    if (!persistence_contract_seed_project_sample(&save_ctx, project_sample_x, project_sample_y, preset_sample_value)) {
        return 1;
    }
    if (!expect_ok(drawing_program_snapshot_save(&save_ctx, expected_preset_path), "persistence_contract_save_preset")) {
        return 1;
    }

    if (!expect_ok(drawing_program_app_bootstrap(&boot_ctx, 10, argv), "persistence_contract_bootstrap_boot")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&boot_ctx), "persistence_contract_config_boot")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&boot_ctx), "persistence_contract_state_seed_boot")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&boot_ctx), "persistence_contract_subsystems_boot")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&boot_ctx), "persistence_contract_runtime_start_boot")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&boot_ctx.document,
                                                        project_sample_x,
                                                        project_sample_y,
                                                        &loaded_sample_value),
                   "persistence_contract_read_boot_sample")) {
        return 1;
    }
    if (loaded_sample_value != expected_boot_sample_value) {
        fprintf(stderr,
                "lifecycle_test: expected boot to prefer saved project sample=%u over runtime preset sample=%u got=%u\n",
                (unsigned)expected_boot_sample_value,
                (unsigned)preset_sample_value,
                (unsigned)loaded_sample_value);
        return 1;
    }

    if (!expect_ok(drawing_program_app_bootstrap(&prefs_ctx, 10, argv), "persistence_contract_bootstrap_prefs")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&prefs_ctx), "persistence_contract_config_prefs")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&prefs_ctx), "persistence_contract_state_seed_prefs")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&prefs_ctx), "persistence_contract_subsystems_prefs")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&prefs_ctx), "persistence_contract_runtime_start_prefs")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_set_input_root(&prefs_ctx, alt_input_root),
                   "persistence_contract_set_alt_input_root")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_set_output_root(&prefs_ctx, alt_output_root),
                   "persistence_contract_set_alt_output_root")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_set_scene_authored_root(&prefs_ctx, alt_scene_root),
                   "persistence_contract_set_alt_scene_root")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_set_current_path(&prefs_ctx, expected_alt_project_path),
                   "persistence_contract_set_alt_project_path")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_shutdown(&prefs_ctx), "persistence_contract_shutdown_prefs")) {
        return 1;
    }

    if (!expect_ok(drawing_program_app_bootstrap(&load_ctx, 6, runtime_only_argv), "persistence_contract_bootstrap_load")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&load_ctx), "persistence_contract_config_load")) {
        return 1;
    }
    if (strcmp(load_ctx.session.input_root_path, alt_input_root) != 0 ||
        strcmp(load_ctx.session.output_root_path, alt_output_root) != 0 ||
        strcmp(load_ctx.session.scene_authored_root_path, alt_scene_root) != 0 ||
        !load_ctx.session.project_path ||
        strcmp(load_ctx.session.project_path, expected_alt_project_path) != 0) {
        fprintf(stderr,
                "lifecycle_test: expected reloaded prefs input=%s output=%s scene=%s project=%s got input=%s output=%s scene=%s project=%s\n",
                alt_input_root,
                alt_output_root,
                alt_scene_root,
                expected_alt_project_path,
                load_ctx.session.input_root_path,
                load_ctx.session.output_root_path,
                load_ctx.session.scene_authored_root_path,
                load_ctx.session.project_path ? load_ctx.session.project_path : "(null)");
        return 1;
    }

    if (!expect_ok(drawing_program_app_bootstrap(&override_ctx, 10, argv), "persistence_contract_bootstrap_override")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&override_ctx), "persistence_contract_config_override")) {
        return 1;
    }
    if (strcmp(override_ctx.session.input_root_path, input_root) != 0 ||
        strcmp(override_ctx.session.output_root_path, output_root) != 0 ||
        strcmp(override_ctx.session.scene_authored_root_path, alt_scene_root) != 0 ||
        !override_ctx.session.project_path ||
        strcmp(override_ctx.session.project_path, expected_project_path) != 0) {
        fprintf(stderr,
                "lifecycle_test: expected CLI root overrides to keep input=%s output=%s project=%s while reloading scene=%s got input=%s output=%s scene=%s project=%s\n",
                input_root,
                output_root,
                expected_project_path,
                alt_scene_root,
                override_ctx.session.input_root_path,
                override_ctx.session.output_root_path,
                override_ctx.session.scene_authored_root_path,
                override_ctx.session.project_path ? override_ctx.session.project_path : "(null)");
        return 1;
    }

    if (!expect_ok(drawing_program_project_state_set_input_root(&load_ctx, input_root),
                   "persistence_contract_restore_input_root")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_set_output_root(&load_ctx, output_root),
                   "persistence_contract_restore_output_root")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_set_scene_authored_root(&load_ctx, scene_root),
                   "persistence_contract_restore_scene_root")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_set_current_path(&load_ctx, expected_project_path),
                   "persistence_contract_restore_project_path")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_load_current(&load_ctx), "persistence_contract_load_current")) {
        return 1;
    }
    loaded_surface_count = load_ctx.texture_project.surface_count;
    if (loaded_surface_count != 2u || load_ctx.texture_project.active_surface_index != imported_surface_index) {
        fprintf(stderr,
                "lifecycle_test: expected persistence contract load to keep two-surface project with active=%u got count=%u active=%u\n",
                (unsigned)imported_surface_index,
                (unsigned)loaded_surface_count,
                (unsigned)load_ctx.texture_project.active_surface_index);
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&load_ctx.document,
                                                        active_surface_sample_x,
                                                        active_surface_sample_y,
                                                        &loaded_sample_value),
                   "persistence_contract_read_active_surface_sample")) {
        return 1;
    }
    if (loaded_sample_value != expected_active_surface_sample_value) {
        fprintf(stderr,
                "lifecycle_test: expected active surface mirror sample=%u after load got=%u\n",
                (unsigned)expected_active_surface_sample_value,
                (unsigned)loaded_sample_value);
        return 1;
    }
    return 0;
}
