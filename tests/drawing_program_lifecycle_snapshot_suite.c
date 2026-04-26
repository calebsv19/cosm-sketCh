#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core_pack.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_history.h"
#include "drawing_program/drawing_program_ui_color_state.h"
#include "drawing_program_lifecycle_snapshot_helpers.h"
#include "drawing_program_lifecycle_snapshot_suite.h"
#include "drawing_program_lifecycle_test_support.h"

int drawing_program_lifecycle_run_snapshot_suite(DrawingProgramAppContext *ctx) {
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
        static DrawingProgramAppContext layer_save_ctx;
        static DrawingProgramAppContext layer_load_ctx;
        static DrawingProgramAppContext legacy_load_ctx;
        char layer_arg0[] = "drawing_program_layer_snapshot_roundtrip";
        char layer_arg1[] = "--headless";
        char layer_arg2[] = "--smoke-frames";
        char layer_arg3[] = "1";
        char layer_arg4[] = "--preset";
        char layer_arg5[] = "/tmp/drawing_program_layer_roundtrip.pack";
        char *layer_argv[] = { layer_arg0, layer_arg1, layer_arg2, layer_arg3, layer_arg4, layer_arg5, 0 };
        const char *legacy_pack_path = "/tmp/drawing_program_layer_roundtrip_legacy.pack";
        uint32_t layer_sample_x = 13u;
        uint32_t layer_sample_y = 17u;
        uint8_t layer_sample_value = 173u;
        uint8_t loaded_layer_sample = 0u;
        uint8_t migrated_layer_sample = 0u;
        uint8_t document_seed_sample = 131u;
        uint32_t new_layer_id = 0u;
        (void)unlink(layer_arg5);
        (void)unlink(legacy_pack_path);
        if (!expect_ok(drawing_program_app_bootstrap(&layer_save_ctx, 6, layer_argv), "layer_snapshot_bootstrap_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&layer_save_ctx), "layer_snapshot_config_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&layer_save_ctx), "layer_snapshot_state_seed_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_subsystems_init(&layer_save_ctx), "layer_snapshot_subsystems_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_start(&layer_save_ctx), "layer_snapshot_runtime_start_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                           &layer_save_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_ADD_LAYER),
                       "layer_snapshot_add_layer")) {
            return 1;
        }
        new_layer_id = layer_save_ctx.editor.active_layer_id;
        if (!expect_ok(drawing_program_document_sample_write(&layer_save_ctx.document,
                                                             layer_sample_x,
                                                             layer_sample_y,
                                                             document_seed_sample,
                                                             0),
                       "layer_snapshot_seed_legacy_document_surface")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_write(&layer_save_ctx.layer_rasters,
                                                                       new_layer_id,
                                                                       layer_sample_x,
                                                                       layer_sample_y,
                                                                       layer_sample_value,
                                                                       0),
                       "layer_snapshot_seed_new_layer_surface")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_save(&layer_save_ctx, layer_arg5), "layer_snapshot_save")) {
            return 1;
        }
        if (!write_legacy_snapshot_without_layer_chunk(layer_arg5, legacy_pack_path)) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&layer_load_ctx, 6, layer_argv), "layer_snapshot_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&layer_load_ctx), "layer_snapshot_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&layer_load_ctx), "layer_snapshot_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&layer_load_ctx, layer_arg5), "layer_snapshot_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&layer_load_ctx.layer_rasters,
                                                                      new_layer_id,
                                                                      layer_sample_x,
                                                                      layer_sample_y,
                                                                      &loaded_layer_sample),
                       "layer_snapshot_loaded_new_layer_sample")) {
            return 1;
        }
        if (loaded_layer_sample != layer_sample_value) {
            fprintf(stderr,
                    "lifecycle_test: expected layer chunk sample roundtrip=%u got=%u\n",
                    (unsigned)layer_sample_value,
                    (unsigned)loaded_layer_sample);
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&legacy_load_ctx, 6, layer_argv), "legacy_layer_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&legacy_load_ctx), "legacy_layer_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&legacy_load_ctx), "legacy_layer_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&legacy_load_ctx, legacy_pack_path), "legacy_layer_snapshot_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_read(&legacy_load_ctx.layer_rasters,
                                                                      legacy_load_ctx.document.layers[0].layer_id,
                                                                      layer_sample_x,
                                                                      layer_sample_y,
                                                                      &migrated_layer_sample),
                       "legacy_layer_migration_seed_read")) {
            return 1;
        }
        if (migrated_layer_sample != document_seed_sample) {
            fprintf(stderr,
                    "lifecycle_test: expected legacy migration to seed base layer sample=%u got=%u\n",
                    (unsigned)document_seed_sample,
                    (unsigned)migrated_layer_sample);
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&layer_save_ctx), "layer_snapshot_shutdown_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&layer_load_ctx), "layer_snapshot_shutdown_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&legacy_load_ctx), "legacy_layer_shutdown_load")) {
            return 1;
        }
        (void)unlink(layer_arg5);
        (void)unlink(legacy_pack_path);
    }
    {
        static DrawingProgramAppContext object_save_ctx;
        static DrawingProgramAppContext object_load_ctx;
        static DrawingProgramAppContext legacy_object_ctx;
        static DrawingProgramAppContext legacy_v1_object_ctx;
        DrawingProgramObjectRecord seed_object;
        const DrawingProgramObjectRecord *loaded_object = 0;
        char object_arg0[] = "drawing_program_object_snapshot_roundtrip";
        char object_arg1[] = "--headless";
        char object_arg2[] = "--smoke-frames";
        char object_arg3[] = "1";
        char object_arg4[] = "--preset";
        char object_arg5[] = "/tmp/drawing_program_object_roundtrip.pack";
        char *object_argv[] = { object_arg0, object_arg1, object_arg2, object_arg3, object_arg4, object_arg5, 0 };
        const char *legacy_pack_path = "/tmp/drawing_program_object_roundtrip_legacy.pack";
        const char *legacy_v1_pack_path = "/tmp/drawing_program_object_roundtrip_legacy_v1.pack";
        uint32_t object_id = 0u;
        (void)unlink(object_arg5);
        (void)unlink(legacy_pack_path);
        (void)unlink(legacy_v1_pack_path);
        if (!expect_ok(drawing_program_app_bootstrap(&object_save_ctx, 6, object_argv), "object_snapshot_bootstrap_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&object_save_ctx), "object_snapshot_config_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&object_save_ctx), "object_snapshot_state_seed_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_subsystems_init(&object_save_ctx), "object_snapshot_subsystems_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_start(&object_save_ctx), "object_snapshot_runtime_start_save")) {
            return 1;
        }
        memset(&seed_object, 0, sizeof(seed_object));
        seed_object.layer_id = object_save_ctx.document.layers[0].layer_id;
        seed_object.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT;
        seed_object.visible = 1u;
        seed_object.locked = 0u;
        seed_object.stroke_color_value = drawing_program_color_value_from_index(7u);
        seed_object.fill_color_value = drawing_program_color_value_from_index(2u);
        seed_object.stroke_width = 3u;
        seed_object.style_mode = 2u;
        seed_object.origin_x = 48;
        seed_object.origin_y = 64;
        seed_object.width = 120u;
        seed_object.height = 90u;
        (void)snprintf(seed_object.name, sizeof(seed_object.name), "%s", "Rect Seed");
        if (!expect_ok(drawing_program_object_store_add(&object_save_ctx.object_store, &seed_object, &object_id),
                       "object_snapshot_seed_add")) {
            return 1;
        }
        if (object_id == 0u || object_save_ctx.object_store.object_count != 1u) {
            fprintf(stderr,
                    "lifecycle_test: expected object seed count=1 id!=0 got count=%u id=%u\n",
                    (unsigned)object_save_ctx.object_store.object_count,
                    (unsigned)object_id);
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_save(&object_save_ctx, object_arg5), "object_snapshot_save")) {
            return 1;
        }
        if (!write_legacy_snapshot_without_object_chunk(object_arg5, legacy_pack_path)) {
            return 1;
        }
        if (!write_legacy_snapshot_with_v1_object_chunk(object_arg5, legacy_v1_pack_path)) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&object_load_ctx, 6, object_argv), "object_snapshot_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&object_load_ctx), "object_snapshot_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&object_load_ctx), "object_snapshot_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&object_load_ctx, object_arg5), "object_snapshot_load")) {
            return 1;
        }
        loaded_object = drawing_program_object_store_get_by_id(&object_load_ctx.object_store, object_id);
        if (!loaded_object ||
            object_load_ctx.object_store.object_count != 1u ||
            loaded_object->width != seed_object.width ||
            loaded_object->height != seed_object.height ||
            loaded_object->origin_x != seed_object.origin_x ||
            loaded_object->origin_y != seed_object.origin_y ||
            loaded_object->layer_id != seed_object.layer_id ||
            loaded_object->type != seed_object.type) {
            fprintf(stderr,
                    "lifecycle_test: object snapshot roundtrip mismatch count=%u loaded=%p id=%u\n",
                    (unsigned)object_load_ctx.object_store.object_count,
                    (void *)loaded_object,
                    (unsigned)object_id);
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&legacy_object_ctx, 6, object_argv), "legacy_object_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&legacy_object_ctx), "legacy_object_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&legacy_object_ctx), "legacy_object_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&legacy_object_ctx, legacy_pack_path), "legacy_object_snapshot_load")) {
            return 1;
        }
        if (legacy_object_ctx.object_store.object_count != 0u) {
            fprintf(stderr,
                    "lifecycle_test: expected legacy object snapshot load to produce empty object store got=%u\n",
                    (unsigned)legacy_object_ctx.object_store.object_count);
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&legacy_v1_object_ctx, 6, object_argv), "legacy_v1_object_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&legacy_v1_object_ctx), "legacy_v1_object_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&legacy_v1_object_ctx), "legacy_v1_object_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&legacy_v1_object_ctx, legacy_v1_pack_path), "legacy_v1_object_snapshot_load")) {
            return 1;
        }
        loaded_object = drawing_program_object_store_get_by_id(&legacy_v1_object_ctx.object_store, object_id);
        if (!loaded_object ||
            legacy_v1_object_ctx.object_store.object_count != 1u ||
            loaded_object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT) {
            fprintf(stderr,
                    "lifecycle_test: expected legacy-v1 object snapshot to preserve rect seed object count=%u loaded=%p\n",
                    (unsigned)legacy_v1_object_ctx.object_store.object_count,
                    (void *)loaded_object);
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&object_save_ctx), "object_snapshot_shutdown_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&object_load_ctx), "object_snapshot_shutdown_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&legacy_object_ctx), "legacy_object_shutdown_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&legacy_v1_object_ctx), "legacy_v1_object_shutdown_load")) {
            return 1;
        }
        (void)unlink(object_arg5);
        (void)unlink(legacy_pack_path);
        (void)unlink(legacy_v1_pack_path);
    }
    {
        static DrawingProgramAppContext path_save_ctx;
        static DrawingProgramAppContext path_load_ctx;
        static DrawingProgramAppContext path_invalid_ctx;
        static DrawingProgramAppContext path_legacy_v2_ctx;
        DrawingProgramObjectRecord rect_seed;
        DrawingProgramObjectRecord path_style_seed;
        DrawingProgramPathPayload path_payload;
        DrawingProgramPathPayload loaded_path_payload;
        const DrawingProgramObjectRecord *loaded_path_object = 0;
        const DrawingProgramObjectRecord *loaded_rect_object = 0;
        const DrawingProgramObjectRecord *legacy_v2_path_object = 0;
        char path_arg0[] = "drawing_program_path_snapshot_roundtrip";
        char path_arg1[] = "--headless";
        char path_arg2[] = "--smoke-frames";
        char path_arg3[] = "1";
        char path_arg4[] = "--preset";
        char path_arg5[] = "/tmp/drawing_program_path_roundtrip.pack";
        char *path_argv[] = { path_arg0, path_arg1, path_arg2, path_arg3, path_arg4, path_arg5, 0 };
        const char *invalid_path_pack = "/tmp/drawing_program_path_roundtrip_invalid.pack";
        const char *legacy_v2_pack = "/tmp/drawing_program_path_roundtrip_legacy_v2.pack";
        uint32_t rect_object_id = 0u;
        uint32_t path_object_id = 0u;
        uint32_t i;
        (void)unlink(path_arg5);
        (void)unlink(invalid_path_pack);
        (void)unlink(legacy_v2_pack);
        if (!expect_ok(drawing_program_app_bootstrap(&path_save_ctx, 6, path_argv), "path_snapshot_bootstrap_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&path_save_ctx), "path_snapshot_config_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&path_save_ctx), "path_snapshot_state_seed_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_subsystems_init(&path_save_ctx), "path_snapshot_subsystems_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_runtime_start(&path_save_ctx), "path_snapshot_runtime_start_save")) {
            return 1;
        }
        memset(&rect_seed, 0, sizeof(rect_seed));
        rect_seed.layer_id = path_save_ctx.document.layers[0].layer_id;
        rect_seed.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT;
        rect_seed.visible = 1u;
        rect_seed.origin_x = 16;
        rect_seed.origin_y = 20;
        rect_seed.width = 24u;
        rect_seed.height = 12u;
        rect_seed.fill_color_value = drawing_program_color_value_from_index(3u);
        rect_seed.stroke_color_value = drawing_program_color_value_from_index(7u);
        rect_seed.stroke_width = 1u;
        rect_seed.style_mode = 2u;
        (void)snprintf(rect_seed.name, sizeof(rect_seed.name), "%s", "Rect Compat Seed");
        if (!expect_ok(drawing_program_object_store_add(&path_save_ctx.object_store, &rect_seed, &rect_object_id),
                       "path_snapshot_seed_rect_add")) {
            return 1;
        }
        memset(&path_style_seed, 0, sizeof(path_style_seed));
        path_style_seed.layer_id = path_save_ctx.document.layers[0].layer_id;
        path_style_seed.visible = 1u;
        path_style_seed.locked = 0u;
        path_style_seed.stroke_color_value = drawing_program_color_value_from_index(6u);
        path_style_seed.fill_color_value = drawing_program_color_value_from_index(2u);
        path_style_seed.stroke_width = 2u;
        path_style_seed.style_mode = 2u;
        (void)snprintf(path_style_seed.name, sizeof(path_style_seed.name), "%s", "Path Seed");
        memset(&path_payload, 0, sizeof(path_payload));
        path_payload.closed = 1u;
        path_payload.point_count = 4u;
        path_payload.points[0].x = 80;
        path_payload.points[0].y = 100;
        path_payload.points[1].x = 110;
        path_payload.points[1].y = 100;
        path_payload.points[1].handle_in_dx = -18;
        path_payload.points[1].handle_in_dy = 4;
        path_payload.points[1].handle_out_dx = 22;
        path_payload.points[1].handle_out_dy = -6;
        path_payload.points[1].bezier_enabled = 1u;
        path_payload.points[1].handle_linked = 1u;
        path_payload.points[2].x = 110;
        path_payload.points[2].y = 130;
        path_payload.points[3].x = 80;
        path_payload.points[3].y = 130;
        if (!expect_ok(drawing_program_object_store_add_path(&path_save_ctx.object_store,
                                                             &path_style_seed,
                                                             &path_payload,
                                                             &path_object_id),
                       "path_snapshot_seed_path_add")) {
            return 1;
        }
        if (path_object_id == 0u || rect_object_id == 0u || path_save_ctx.object_store.object_count != 2u) {
            fprintf(stderr,
                    "lifecycle_test: expected path+rect seed object count=2 ids non-zero got count=%u path_id=%u rect_id=%u\n",
                    (unsigned)path_save_ctx.object_store.object_count,
                    (unsigned)path_object_id,
                    (unsigned)rect_object_id);
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_save(&path_save_ctx, path_arg5), "path_snapshot_save")) {
            return 1;
        }
        if (!write_snapshot_with_invalid_path_payload(path_arg5, invalid_path_pack)) {
            return 1;
        }
        if (!write_legacy_snapshot_with_v2_object_chunk(path_arg5, legacy_v2_pack)) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&path_load_ctx, 6, path_argv), "path_snapshot_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&path_load_ctx), "path_snapshot_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&path_load_ctx), "path_snapshot_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&path_load_ctx, path_arg5), "path_snapshot_load")) {
            return 1;
        }
        loaded_path_object = drawing_program_object_store_get_by_id(&path_load_ctx.object_store, path_object_id);
        if (!loaded_path_object ||
            loaded_path_object->type != (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_PATH ||
            loaded_path_object->path_point_count != path_payload.point_count ||
            loaded_path_object->path_closed != 1u) {
            fprintf(stderr,
                    "lifecycle_test: path snapshot roundtrip mismatch loaded=%p type=%u points=%u closed=%u\n",
                    (void *)loaded_path_object,
                    loaded_path_object ? (unsigned)loaded_path_object->type : 0u,
                    loaded_path_object ? (unsigned)loaded_path_object->path_point_count : 0u,
                    loaded_path_object ? (unsigned)loaded_path_object->path_closed : 0u);
            return 1;
        }
        memset(&loaded_path_payload, 0, sizeof(loaded_path_payload));
        if (!expect_ok(drawing_program_object_store_get_path_payload(&path_load_ctx.object_store,
                                                                     path_object_id,
                                                                     &loaded_path_payload),
                       "path_snapshot_payload_get")) {
            return 1;
        }
        if (loaded_path_payload.point_count != path_payload.point_count ||
            loaded_path_payload.closed != path_payload.closed) {
            fprintf(stderr,
                    "lifecycle_test: path payload metadata mismatch count=%u/%u closed=%u/%u\n",
                    (unsigned)loaded_path_payload.point_count,
                    (unsigned)path_payload.point_count,
                    (unsigned)loaded_path_payload.closed,
                    (unsigned)path_payload.closed);
            return 1;
        }
        for (i = 0u; i < path_payload.point_count; ++i) {
            if (loaded_path_payload.points[i].x != path_payload.points[i].x ||
                loaded_path_payload.points[i].y != path_payload.points[i].y) {
                fprintf(stderr,
                        "lifecycle_test: path payload point mismatch idx=%u expected=%d,%d got=%d,%d\n",
                        (unsigned)i,
                        (int)path_payload.points[i].x,
                        (int)path_payload.points[i].y,
                        (int)loaded_path_payload.points[i].x,
                        (int)loaded_path_payload.points[i].y);
                return 1;
            }
        }
        if (loaded_path_payload.points[1].bezier_enabled != 1u ||
            loaded_path_payload.points[1].handle_linked != 1u ||
            loaded_path_payload.points[1].handle_in_dx != path_payload.points[1].handle_in_dx ||
            loaded_path_payload.points[1].handle_in_dy != path_payload.points[1].handle_in_dy ||
            loaded_path_payload.points[1].handle_out_dx != path_payload.points[1].handle_out_dx ||
            loaded_path_payload.points[1].handle_out_dy != path_payload.points[1].handle_out_dy) {
            fprintf(stderr,
                    "lifecycle_test: path payload bezier mismatch enabled=%u linked=%u in=%d,%d out=%d,%d\n",
                    (unsigned)loaded_path_payload.points[1].bezier_enabled,
                    (unsigned)loaded_path_payload.points[1].handle_linked,
                    (int)loaded_path_payload.points[1].handle_in_dx,
                    (int)loaded_path_payload.points[1].handle_in_dy,
                    (int)loaded_path_payload.points[1].handle_out_dx,
                    (int)loaded_path_payload.points[1].handle_out_dy);
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&path_legacy_v2_ctx, 6, path_argv), "path_legacy_v2_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&path_legacy_v2_ctx), "path_legacy_v2_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&path_legacy_v2_ctx), "path_legacy_v2_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&path_legacy_v2_ctx, legacy_v2_pack), "path_legacy_v2_snapshot_load")) {
            return 1;
        }
        legacy_v2_path_object = drawing_program_object_store_get_by_id(&path_legacy_v2_ctx.object_store, path_object_id);
        if (!legacy_v2_path_object ||
            legacy_v2_path_object->path_points[1].bezier_enabled != 0u ||
            legacy_v2_path_object->path_points[1].handle_linked != 0u ||
            legacy_v2_path_object->path_points[1].handle_in_dx != 0 ||
            legacy_v2_path_object->path_points[1].handle_in_dy != 0 ||
            legacy_v2_path_object->path_points[1].handle_out_dx != 0 ||
            legacy_v2_path_object->path_points[1].handle_out_dy != 0) {
            fprintf(stderr,
                    "lifecycle_test: expected legacy-v2 path load to zero bezier data loaded=%p enabled=%u linked=%u in=%d,%d out=%d,%d\n",
                    (void *)legacy_v2_path_object,
                    legacy_v2_path_object ? (unsigned)legacy_v2_path_object->path_points[1].bezier_enabled : 99u,
                    legacy_v2_path_object ? (unsigned)legacy_v2_path_object->path_points[1].handle_linked : 99u,
                    legacy_v2_path_object ? legacy_v2_path_object->path_points[1].handle_in_dx : 999,
                    legacy_v2_path_object ? legacy_v2_path_object->path_points[1].handle_in_dy : 999,
                    legacy_v2_path_object ? legacy_v2_path_object->path_points[1].handle_out_dx : 999,
                    legacy_v2_path_object ? legacy_v2_path_object->path_points[1].handle_out_dy : 999);
            return 1;
        }
        if (!expect_ok(drawing_program_app_bootstrap(&path_invalid_ctx, 6, path_argv), "path_invalid_bootstrap_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_config_load(&path_invalid_ctx), "path_invalid_config_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_state_seed(&path_invalid_ctx), "path_invalid_state_seed_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_load(&path_invalid_ctx, invalid_path_pack), "path_invalid_snapshot_load")) {
            return 1;
        }
        loaded_rect_object = drawing_program_object_store_get_by_id(&path_invalid_ctx.object_store, rect_object_id);
        loaded_path_object = drawing_program_object_store_get_by_id(&path_invalid_ctx.object_store, path_object_id);
        if (!loaded_rect_object || loaded_path_object) {
            fprintf(stderr,
                    "lifecycle_test: malformed path payload fallback failed rect=%p path=%p count=%u\n",
                    (void *)loaded_rect_object,
                    (void *)loaded_path_object,
                    (unsigned)path_invalid_ctx.object_store.object_count);
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&path_save_ctx), "path_snapshot_shutdown_save")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&path_load_ctx), "path_snapshot_shutdown_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&path_legacy_v2_ctx), "path_legacy_v2_shutdown_load")) {
            return 1;
        }
        if (!expect_ok(drawing_program_app_shutdown(&path_invalid_ctx), "path_invalid_shutdown_load")) {
            return 1;
        }
        (void)unlink(path_arg5);
        (void)unlink(invalid_path_pack);
        (void)unlink(legacy_v2_pack);
    }

    return 0;
}
