#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core_pack.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_history.h"
#include "drawing_program/drawing_program_ui_color_state.h"
#include "drawing_program_lifecycle_snapshot_helpers.h"
#include "drawing_program_lifecycle_snapshot_suite_internal.h"
#include "drawing_program_lifecycle_test_support.h"

int drawing_program_lifecycle_run_snapshot_shell_suite(DrawingProgramAppContext *ctx) {
    {
        const char *pack_path = "/tmp/drawing_program_test_snapshot.pack";
        const char *json_path = "/tmp/drawing_program_test_snapshot.json";
        if (!expect_ok(drawing_program_snapshot_save(ctx, pack_path), "snapshot_save")) {
            return 1;
        }
        ctx->runtime.render_projection.raster_sample_count = 777u;
        ctx->runtime.render_projection.raster_hash32 = 4242u;
        ctx->runtime.render_canvas_last_raster_hash = 31337u;
        ctx->runtime.render_canvas_last_nonzero_samples = 99u;
        ctx->runtime.render_last_active_layer_id = 17u;
        ctx->runtime.render_last_has_active_layer = 1u;
        if (!expect_ok(drawing_program_snapshot_load(ctx, pack_path), "snapshot_load")) {
            return 1;
        }
        if (ctx->runtime.render_projection.raster_sample_count != 0u ||
            ctx->runtime.render_projection.raster_hash32 != 0u ||
            ctx->runtime.render_canvas_last_raster_hash != 0u ||
            ctx->runtime.render_canvas_last_nonzero_samples != 0u ||
            ctx->runtime.render_last_active_layer_id != 0u ||
            ctx->runtime.render_last_has_active_layer != 0u) {
            fprintf(stderr,
                    "lifecycle_test: expected snapshot load to invalidate stale render/runtime state after document swap\n");
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_export_debug_json(ctx, json_path), "snapshot_export_json")) {
            return 1;
        }
        if (access(json_path, F_OK) != 0) {
            fprintf(stderr, "lifecycle_test: expected debug json output at %s\n", json_path);
            return 1;
        }
    }
    {
        static DrawingProgramAppContext repaired_load_ctx;
        char arg0[] = "drawing_program_module_binding_repair";
        char arg1[] = "--headless";
        char arg2[] = "--smoke-frames";
        char arg3[] = "1";
        char arg4[] = "--no-persist";
        char *argv[] = { arg0, arg1, arg2, arg3, arg4, 0 };
        const char *pack_path = "/tmp/drawing_program_module_binding_repair.pack";
        (void)unlink(pack_path);
        ctx->pane_host.module_binding_count = 0u;
        if (!expect_ok(drawing_program_snapshot_save(ctx, pack_path), "binding_repair_snapshot_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_pane_host_rebind_default_modules(ctx), "binding_repair_restore_default_modules") ||
            !expect_ok(drawing_program_pane_host_rebuild(ctx), "binding_repair_restore_rebuild")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&repaired_load_ctx, 5, argv), "binding_repair_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&repaired_load_ctx), "binding_repair_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&repaired_load_ctx), "binding_repair_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&repaired_load_ctx, pack_path), "binding_repair_snapshot_load")) {
            return 1;
        }
        if (!drawing_program_pane_host_default_modules_ready(&repaired_load_ctx)) {
            fprintf(stderr,
                    "lifecycle_test: expected snapshot load to repair default pane module bindings after stale shell load\n");
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&repaired_load_ctx), "binding_repair_shutdown_load")) {
            return 1;
        }
        (void)unlink(pack_path);
    }
    {
        static DrawingProgramAppContext save_ctx;
        static DrawingProgramAppContext load_ctx;
        char arg0[] = "drawing_program_post_load_edit_visibility";
        char arg1[] = "--headless";
        char arg2[] = "--smoke-frames";
        char arg3[] = "2";
        char arg4[] = "--no-persist";
        char *argv[] = { arg0, arg1, arg2, arg3, arg4, 0 };
        const char *pack_path = "/tmp/drawing_program_post_load_edit_visibility.pack";
        uint32_t center_x = 0u;
        uint32_t center_y = 0u;
        uint8_t center_before_load = 0u;
        uint8_t center_after_run = 0u;
        uint8_t clear_value =
            drawing_program_color_legacy_sample_from_sample(drawing_program_color_eraser_value());
        uint8_t expected_draw_value = drawing_program_color_value_from_index(
            drawing_program_color_index_clamp(save_ctx.ui.active_color_index));
        (void)unlink(pack_path);
        if (!expect_ok(drawing_program_app_bootstrap(&save_ctx, 5, argv), "post_load_edit_bootstrap_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&save_ctx), "post_load_edit_config_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&save_ctx), "post_load_edit_state_seed_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_subsystems_init(&save_ctx), "post_load_edit_subsystems_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_start(&save_ctx), "post_load_edit_runtime_start_save")) {
            return 1;
        }
        center_x = save_ctx.document.raster_width / 2u;
        center_y = save_ctx.document.raster_height / 2u;
        expected_draw_value = drawing_program_color_value_from_index(
            drawing_program_color_index_clamp(save_ctx.ui.active_color_index));
        if (!expect_ok(drawing_program_history_apply_set_sample_value(&save_ctx.history,
                                                                      &save_ctx.document,
                                                                      &save_ctx.layer_rasters,
                                                                      save_ctx.editor.active_layer_id,
                                                                      center_x,
                                                                      center_y,
                                                                      clear_value),
                       "post_load_edit_seed_center_background")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_save(&save_ctx, pack_path), "post_load_edit_snapshot_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&load_ctx, 5, argv), "post_load_edit_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&load_ctx), "post_load_edit_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&load_ctx), "post_load_edit_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_subsystems_init(&load_ctx), "post_load_edit_subsystems_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_start(&load_ctx), "post_load_edit_runtime_start_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&load_ctx, pack_path), "post_load_edit_snapshot_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&load_ctx.document,
                                                            center_x,
                                                            center_y,
                                                            &center_before_load),
                       "post_load_edit_center_before_run")) {
            return 1;
        }
        if (center_before_load != clear_value) {
            fprintf(stderr,
                    "lifecycle_test: expected loaded center background=%u before immediate edit got=%u\n",
                    (unsigned)clear_value,
                    (unsigned)center_before_load);
            return 1;
        }
        if (!expect_ok(drawing_program_app_run_loop(&load_ctx), "post_load_edit_run_loop")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&load_ctx.document,
                                                            center_x,
                                                            center_y,
                                                            &center_after_run),
                       "post_load_edit_center_after_run")) {
            return 1;
        }
        if (center_after_run != expected_draw_value) {
            fprintf(stderr,
                    "lifecycle_test: expected immediate post-load brush edit=%u got=%u\n",
                    (unsigned)expected_draw_value,
                    (unsigned)center_after_run);
            return 1;
        }
        if (load_ctx.runtime.render_projection.raster_sample_count == 0u ||
            load_ctx.runtime.render_projection.raster_nonzero_count == 0u ||
            load_ctx.runtime.render_projection.raster_hash32 == 0u) {
            fprintf(stderr,
                    "lifecycle_test: expected post-load run loop to repopulate render projection immediately\n");
            return 1;
        }
        if (load_ctx.runtime.render_canvas_last_raster_hash != load_ctx.runtime.render_projection.raster_hash32 ||
            load_ctx.runtime.render_canvas_last_nonzero_samples != load_ctx.runtime.render_projection.raster_nonzero_count) {
            fprintf(stderr,
                    "lifecycle_test: expected post-load edit to update visible canvas signature in the same session\n");
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&save_ctx), "post_load_edit_shutdown_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&load_ctx), "post_load_edit_shutdown_load")) {
            return 1;
        }
        (void)unlink(pack_path);
    }
    {
        static DrawingProgramAppContext color_migration_save_ctx;
        static DrawingProgramAppContext color_migration_load_ctx;
        static DrawingProgramAppContext color_migration_reload_ctx;
        char color_arg0[] = "drawing_program_color_snapshot_migration";
        char color_arg1[] = "--headless";
        char color_arg2[] = "--smoke-frames";
        char color_arg3[] = "1";
        char color_arg4[] = "--preset";
        char color_arg5[] = "/tmp/drawing_program_color_migration.pack";
        char *color_argv[] = { color_arg0, color_arg1, color_arg2, color_arg3, color_arg4, color_arg5, 0 };
        const char *upgraded_pack_path = "/tmp/drawing_program_color_migration_upgraded.pack";
        uint32_t sample_x = 21u;
        uint32_t sample_y = 29u;
        uint8_t migrated_sample = 0u;
        uint8_t reloaded_sample = 0u;
        CorePackReader upgraded_reader;
        CorePackChunkInfo upgraded_shell_chunk;
        (void)unlink(color_arg5);
        (void)unlink(upgraded_pack_path);
        if (!expect_ok(drawing_program_app_bootstrap(&color_migration_save_ctx, 6, color_argv),
                       "color_snapshot_bootstrap_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&color_migration_save_ctx), "color_snapshot_config_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&color_migration_save_ctx), "color_snapshot_state_seed_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_subsystems_init(&color_migration_save_ctx), "color_snapshot_subsystems_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_start(&color_migration_save_ctx), "color_snapshot_runtime_start_save")) {
            return 1;
        }
        color_migration_save_ctx.document.schema_version = 2u;
        if (!expect_ok(drawing_program_document_sample_write(&color_migration_save_ctx.document,
                                                             sample_x,
                                                             sample_y,
                                                             168u,
                                                             0),
                       "color_snapshot_seed_legacy_grayscale")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_save(&color_migration_save_ctx, color_arg5), "color_snapshot_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&color_migration_load_ctx, 6, color_argv),
                       "color_snapshot_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&color_migration_load_ctx), "color_snapshot_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&color_migration_load_ctx), "color_snapshot_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&color_migration_load_ctx, color_arg5), "color_snapshot_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&color_migration_load_ctx.document,
                                                            sample_x,
                                                            sample_y,
                                                            &migrated_sample),
                       "color_snapshot_migrated_sample_read")) {
            return 1;
        }
        if (migrated_sample != 168u ||
            color_migration_load_ctx.document.schema_version != DRAWING_PROGRAM_DOCUMENT_SCHEMA_VERSION_TRUE_COLOR) {
            fprintf(stderr,
                    "lifecycle_test: expected legacy grayscale snapshot sample to migrate to true-color grayscale 168 schema=%u got sample=%u schema=%u\n",
                    (unsigned)DRAWING_PROGRAM_DOCUMENT_SCHEMA_VERSION_TRUE_COLOR,
                    (unsigned)migrated_sample,
                    (unsigned)color_migration_load_ctx.document.schema_version);
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_save(&color_migration_load_ctx, upgraded_pack_path),
                       "color_snapshot_save_upgraded")) {
            return 1;
        }
        memset(&upgraded_shell_chunk, 0, sizeof(upgraded_shell_chunk));
        if (!expect_ok(core_pack_reader_open(upgraded_pack_path, &upgraded_reader), "color_snapshot_open_upgraded_pack")) {
            return 1;
        }
        if (!expect_ok(core_pack_reader_find_chunk(&upgraded_reader, "DPS3", 0u, &upgraded_shell_chunk),
                       "color_snapshot_find_dps3")) {
            (void)core_pack_reader_close(&upgraded_reader);
            return 1;
        }
        if (!expect_ok(core_pack_reader_close(&upgraded_reader), "color_snapshot_close_upgraded_pack")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&color_migration_reload_ctx, 6, color_argv),
                       "color_snapshot_bootstrap_reload")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&color_migration_reload_ctx), "color_snapshot_config_reload")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&color_migration_reload_ctx), "color_snapshot_state_seed_reload")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&color_migration_reload_ctx, upgraded_pack_path),
                       "color_snapshot_reload_upgraded")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&color_migration_reload_ctx.document,
                                                            sample_x,
                                                            sample_y,
                                                            &reloaded_sample),
                       "color_snapshot_reloaded_sample_read")) {
            return 1;
        }
        if (reloaded_sample != 168u ||
            color_migration_reload_ctx.document.schema_version != DRAWING_PROGRAM_DOCUMENT_SCHEMA_VERSION_TRUE_COLOR) {
            fprintf(stderr,
                    "lifecycle_test: expected upgraded DPS3 snapshot reload to preserve grayscale 168 got sample=%u schema=%u\n",
                    (unsigned)reloaded_sample,
                    (unsigned)color_migration_reload_ctx.document.schema_version);
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&color_migration_save_ctx), "color_snapshot_shutdown_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&color_migration_load_ctx), "color_snapshot_shutdown_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&color_migration_reload_ctx), "color_snapshot_shutdown_reload")) {
            return 1;
        }
        (void)unlink(color_arg5);
        (void)unlink(upgraded_pack_path);
    }
    {
        static DrawingProgramAppContext legacy_shell_save_ctx;
        static DrawingProgramAppContext legacy_shell_load_ctx;
        DrawingProgramObjectRecord seed_object;
        const DrawingProgramObjectRecord *loaded_object = 0;
        char arg0[] = "drawing_program_legacy_dps2_chunk_fallback";
        char arg1[] = "--headless";
        char arg2[] = "--smoke-frames";
        char arg3[] = "1";
        char arg4[] = "--preset";
        char arg5[] = "/tmp/drawing_program_legacy_dps2_chunk_fallback.pack";
        char *argv[] = { arg0, arg1, arg2, arg3, arg4, arg5, 0 };
        const char *truncated_pack_path = "/tmp/drawing_program_legacy_dps2_chunk_fallback_truncated.pack";
        uint32_t sample_x = 31u;
        uint32_t sample_y = 27u;
        uint8_t sample_value = 201u;
        uint8_t loaded_sample = 0u;
        uint32_t object_id = 0u;
        (void)unlink(arg5);
        (void)unlink(truncated_pack_path);
        if (!expect_ok(drawing_program_app_bootstrap(&legacy_shell_save_ctx, 6, argv),
                       "legacy_dps2_fallback_bootstrap_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&legacy_shell_save_ctx), "legacy_dps2_fallback_config_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&legacy_shell_save_ctx), "legacy_dps2_fallback_state_seed_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_subsystems_init(&legacy_shell_save_ctx),
                       "legacy_dps2_fallback_subsystems_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_start(&legacy_shell_save_ctx), "legacy_dps2_fallback_runtime_start_save")) {
            return 1;
        }
        legacy_shell_save_ctx.ui.right_panel_slot = 2u;
        legacy_shell_save_ctx.ui.active_color_index = 5u;
        legacy_shell_save_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_PICKER;
        drawing_program_ui_color_set_active_paint_rgb(&legacy_shell_save_ctx, 20u, 40u, 180u, 0u);
        legacy_shell_save_ctx.ui.color_palette_rgb[5u][0] = 20u;
        legacy_shell_save_ctx.ui.color_palette_rgb[5u][1] = 40u;
        legacy_shell_save_ctx.ui.color_palette_rgb[5u][2] = 180u;
        legacy_shell_save_ctx.ui.recent_color_rgb[0][0] = 20u;
        legacy_shell_save_ctx.ui.recent_color_rgb[0][1] = 40u;
        legacy_shell_save_ctx.ui.recent_color_rgb[0][2] = 180u;
        legacy_shell_save_ctx.ui.selected_recent_color_index = 0u;
        if (!expect_ok(drawing_program_document_sample_write(&legacy_shell_save_ctx.document,
                                                             sample_x,
                                                             sample_y,
                                                             sample_value,
                                                             0),
                       "legacy_dps2_fallback_seed_sample")) {
            return 1;
        }
        memset(&seed_object, 0, sizeof(seed_object));
        seed_object.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT;
        seed_object.layer_id = legacy_shell_save_ctx.editor.active_layer_id;
        seed_object.origin_x = 12;
        seed_object.origin_y = 14;
        seed_object.width = 22u;
        seed_object.height = 18u;
        seed_object.visible = 1u;
        seed_object.stroke_width = 1u;
        seed_object.style_mode = 2u;
        seed_object.stroke_color_value = drawing_program_color_value_from_rgb(255u, 0u, 0u);
        seed_object.fill_color_value = drawing_program_color_value_from_rgb(0u, 255u, 0u);
        if (!expect_ok(drawing_program_object_store_add(&legacy_shell_save_ctx.object_store, &seed_object, &object_id),
                       "legacy_dps2_fallback_seed_object")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_save(&legacy_shell_save_ctx, arg5), "legacy_dps2_fallback_snapshot_save")) {
            return 1;
        }
        if (!write_snapshot_with_truncated_dps2_shell(arg5, truncated_pack_path)) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&legacy_shell_load_ctx, 6, argv),
                       "legacy_dps2_fallback_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&legacy_shell_load_ctx), "legacy_dps2_fallback_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&legacy_shell_load_ctx), "legacy_dps2_fallback_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&legacy_shell_load_ctx, truncated_pack_path),
                       "legacy_dps2_fallback_snapshot_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&legacy_shell_load_ctx.document,
                                                            sample_x,
                                                            sample_y,
                                                            &loaded_sample),
                       "legacy_dps2_fallback_loaded_sample")) {
            return 1;
        }
        loaded_object = drawing_program_object_store_get_by_id(&legacy_shell_load_ctx.object_store, object_id);
        if (loaded_sample != sample_value ||
            !loaded_object ||
            legacy_shell_load_ctx.ui.right_panel_slot != 2u ||
            legacy_shell_load_ctx.ui.active_color_index != 5u) {
            fprintf(stderr,
                    "lifecycle_test: expected truncated DPS2 fallback load to preserve sample=%u object=%u slot=%u color=%u got sample=%u object=%u slot=%u color=%u tool=%u\n",
                    (unsigned)sample_value,
                    1u,
                    2u,
                    5u,
                    (unsigned)loaded_sample,
                    loaded_object ? 1u : 0u,
                    (unsigned)legacy_shell_load_ctx.ui.right_panel_slot,
                    (unsigned)legacy_shell_load_ctx.ui.active_color_index,
                    (unsigned)legacy_shell_load_ctx.editor.active_tool);
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&legacy_shell_save_ctx), "legacy_dps2_fallback_shutdown_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&legacy_shell_load_ctx), "legacy_dps2_fallback_shutdown_load")) {
            return 1;
        }
        (void)unlink(arg5);
        (void)unlink(truncated_pack_path);
    }
    {
        static DrawingProgramAppContext dps3_fallback_save_ctx;
        static DrawingProgramAppContext dps3_fallback_load_ctx;
        DrawingProgramObjectRecord seed_object;
        const DrawingProgramObjectRecord *loaded_object = 0;
        char arg0[] = "drawing_program_legacy_dps3_chunk_fallback";
        char arg1[] = "--headless";
        char arg2[] = "--smoke-frames";
        char arg3[] = "1";
        char arg4[] = "--preset";
        char arg5[] = "/tmp/drawing_program_legacy_dps3_chunk_fallback.pack";
        char *argv[] = { arg0, arg1, arg2, arg3, arg4, arg5, 0 };
        const char *truncated_pack_path = "/tmp/drawing_program_legacy_dps3_chunk_fallback_truncated.pack";
        uint32_t sample_x = 19u;
        uint32_t sample_y = 11u;
        uint8_t sample_value = 143u;
        uint8_t loaded_sample = 0u;
        uint32_t object_id = 0u;
        (void)unlink(arg5);
        (void)unlink(truncated_pack_path);
        if (!expect_ok(drawing_program_app_bootstrap(&dps3_fallback_save_ctx, 6, argv),
                       "legacy_dps3_fallback_bootstrap_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&dps3_fallback_save_ctx), "legacy_dps3_fallback_config_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&dps3_fallback_save_ctx), "legacy_dps3_fallback_state_seed_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_subsystems_init(&dps3_fallback_save_ctx),
                       "legacy_dps3_fallback_subsystems_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_start(&dps3_fallback_save_ctx), "legacy_dps3_fallback_runtime_start_save")) {
            return 1;
        }
        dps3_fallback_save_ctx.ui.right_panel_slot = 3u;
        dps3_fallback_save_ctx.ui.active_color_index = 4u;
        dps3_fallback_save_ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_BRUSH;
        if (!expect_ok(drawing_program_document_sample_write(&dps3_fallback_save_ctx.document,
                                                             sample_x,
                                                             sample_y,
                                                             sample_value,
                                                             0),
                       "legacy_dps3_fallback_seed_sample")) {
            return 1;
        }
        memset(&seed_object, 0, sizeof(seed_object));
        seed_object.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT;
        seed_object.layer_id = dps3_fallback_save_ctx.editor.active_layer_id;
        seed_object.origin_x = 7;
        seed_object.origin_y = 9;
        seed_object.width = 14u;
        seed_object.height = 12u;
        seed_object.visible = 1u;
        seed_object.stroke_width = 1u;
        seed_object.style_mode = 2u;
        seed_object.stroke_color_value = drawing_program_color_value_from_rgb(0u, 0u, 255u);
        seed_object.fill_color_value = drawing_program_color_value_from_rgb(255u, 255u, 0u);
        if (!expect_ok(drawing_program_object_store_add(&dps3_fallback_save_ctx.object_store, &seed_object, &object_id),
                       "legacy_dps3_fallback_seed_object")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_save(&dps3_fallback_save_ctx, arg5), "legacy_dps3_fallback_snapshot_save")) {
            return 1;
        }
        if (!write_snapshot_with_truncated_dps3_shell(arg5, truncated_pack_path)) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&dps3_fallback_load_ctx, 6, argv),
                       "legacy_dps3_fallback_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&dps3_fallback_load_ctx), "legacy_dps3_fallback_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&dps3_fallback_load_ctx), "legacy_dps3_fallback_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&dps3_fallback_load_ctx, truncated_pack_path),
                       "legacy_dps3_fallback_snapshot_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_document_sample_read(&dps3_fallback_load_ctx.document,
                                                            sample_x,
                                                            sample_y,
                                                            &loaded_sample),
                       "legacy_dps3_fallback_loaded_sample")) {
            return 1;
        }
        loaded_object = drawing_program_object_store_get_by_id(&dps3_fallback_load_ctx.object_store, object_id);
        if (loaded_sample != sample_value ||
            !loaded_object ||
            dps3_fallback_load_ctx.ui.right_panel_slot != 3u ||
            dps3_fallback_load_ctx.ui.active_color_index != 4u) {
            fprintf(stderr,
                    "lifecycle_test: expected truncated DPS3 fallback load to preserve sample=%u object=%u slot=%u color=%u got sample=%u object=%u slot=%u color=%u tool=%u\n",
                    (unsigned)sample_value,
                    1u,
                    3u,
                    4u,
                    (unsigned)loaded_sample,
                    loaded_object ? 1u : 0u,
                    (unsigned)dps3_fallback_load_ctx.ui.right_panel_slot,
                    (unsigned)dps3_fallback_load_ctx.ui.active_color_index,
                    (unsigned)dps3_fallback_load_ctx.editor.active_tool);
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&dps3_fallback_save_ctx), "legacy_dps3_fallback_shutdown_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&dps3_fallback_load_ctx), "legacy_dps3_fallback_shutdown_load")) {
            return 1;
        }
        (void)unlink(arg5);
        (void)unlink(truncated_pack_path);
    }

    return 0;
}
