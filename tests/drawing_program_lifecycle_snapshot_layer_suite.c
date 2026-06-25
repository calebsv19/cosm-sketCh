#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program_lifecycle_snapshot_helpers.h"
#include "drawing_program_lifecycle_snapshot_suite_internal.h"
#include "drawing_program_lifecycle_test_support.h"

int drawing_program_lifecycle_run_snapshot_layer_suite(void) {
    {
        static DrawingProgramAppContext layer_save_ctx;
        static DrawingProgramAppContext layer_load_ctx;
        static DrawingProgramAppContext legacy_load_ctx;
        char layer_arg0[] = "drawing_program_layer_snapshot_roundtrip";
        char layer_arg1[] = "--headless";
        char layer_arg2[] = "--smoke-frames";
        char layer_arg3[] = "1";
        char layer_arg4[] = "--preset";
        char suite_root[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        char layer_pack_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        char legacy_pack_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        char *layer_argv[] = { layer_arg0, layer_arg1, layer_arg2, layer_arg3, layer_arg4, layer_pack_path, 0 };
        uint32_t layer_sample_x = 13u;
        uint32_t layer_sample_y = 17u;
        uint8_t layer_sample_value = 173u;
        uint8_t loaded_layer_sample = 0u;
        uint8_t migrated_layer_sample = 0u;
        uint8_t document_seed_sample = 131u;
        DrawingProgramRasterSample base_layer_sample =
            drawing_program_color_value_from_rgba(220u, 32u, 24u, 255u);
        DrawingProgramRasterSample top_layer_sample =
            drawing_program_color_value_from_rgba(24u, 64u, 220u, 255u);
        DrawingProgramRasterSample composed_document_sample = top_layer_sample;
        DrawingProgramRasterSample loaded_base_layer_sample = drawing_program_color_eraser_value();
        DrawingProgramRasterSample loaded_top_layer_sample = drawing_program_color_eraser_value();
        DrawingProgramRasterSample loaded_surface_base_layer_sample = drawing_program_color_eraser_value();
        uint32_t new_layer_id = 0u;
        if (!lifecycle_test_artifact_path(suite_root, sizeof(suite_root), "snapshot_layer_suite") ||
            !lifecycle_test_artifact_child_path(layer_pack_path,
                                                sizeof(layer_pack_path),
                                                suite_root,
                                                "drawing_program_layer_roundtrip.pack") ||
            !lifecycle_test_artifact_child_path(legacy_pack_path,
                                                sizeof(legacy_pack_path),
                                                suite_root,
                                                "drawing_program_layer_roundtrip_legacy.pack")) {
            fprintf(stderr, "lifecycle_test: failed to build snapshot layer fixture paths\n");
            return 1;
        }
        (void)mkdir(suite_root, 0775);
        (void)unlink(layer_pack_path);
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
        if (!expect_ok(drawing_program_layer_raster_store_sample_write(&layer_save_ctx.layer_rasters,
                                                                       layer_save_ctx.document.layers[0].layer_id,
                                                                       layer_sample_x + 1u,
                                                                       layer_sample_y,
                                                                       base_layer_sample,
                                                                       0),
                       "layer_snapshot_seed_base_true_color_surface") ||
            !expect_ok(drawing_program_layer_raster_store_sample_write(&layer_save_ctx.layer_rasters,
                                                                       new_layer_id,
                                                                       layer_sample_x + 1u,
                                                                       layer_sample_y,
                                                                       top_layer_sample,
                                                                       0),
                       "layer_snapshot_seed_top_true_color_surface") ||
            !expect_ok(drawing_program_document_sample_write(&layer_save_ctx.document,
                                                             layer_sample_x + 1u,
                                                             layer_sample_y,
                                                             composed_document_sample,
                                                             0),
                       "layer_snapshot_seed_composed_document_surface")) {
            return 1;
        }
        if (!expect_ok(drawing_program_snapshot_save(&layer_save_ctx, layer_pack_path), "layer_snapshot_save")) {
            return 1;
        }
        if (!write_legacy_snapshot_without_layer_chunk(layer_pack_path, legacy_pack_path)) {
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
        if (!expect_ok(drawing_program_snapshot_load(&layer_load_ctx, layer_pack_path), "layer_snapshot_load")) {
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
        if (!expect_ok(drawing_program_layer_raster_store_raster_sample_read(&layer_load_ctx.layer_rasters,
                                                                             layer_load_ctx.document.layers[0].layer_id,
                                                                             layer_sample_x + 1u,
                                                                             layer_sample_y,
                                                                             &loaded_base_layer_sample),
                       "layer_snapshot_loaded_base_true_color_sample") ||
            !expect_ok(drawing_program_layer_raster_store_raster_sample_read(&layer_load_ctx.layer_rasters,
                                                                             new_layer_id,
                                                                             layer_sample_x + 1u,
                                                                             layer_sample_y,
                                                                             &loaded_top_layer_sample),
                       "layer_snapshot_loaded_top_true_color_sample")) {
            return 1;
        }
        if (loaded_base_layer_sample != base_layer_sample ||
            loaded_top_layer_sample != top_layer_sample ||
            loaded_base_layer_sample == composed_document_sample) {
            fprintf(stderr,
                    "lifecycle_test: expected saved layer rasters to keep base/top separate got base=%08x top=%08x composite=%08x\n",
                    (unsigned)loaded_base_layer_sample,
                    (unsigned)loaded_top_layer_sample,
                    (unsigned)composed_document_sample);
            return 1;
        }
        if (layer_load_ctx.texture_project.surface_count > 0u) {
            const DrawingProgramTextureSurface *loaded_surface = drawing_program_texture_project_surface_at(
                &layer_load_ctx.texture_project,
                layer_load_ctx.texture_project.active_surface_index);
            if (!loaded_surface || !loaded_surface->storage ||
                !expect_ok(drawing_program_layer_raster_store_raster_sample_read(&loaded_surface->storage->layer_rasters,
                                                                                 loaded_surface->storage->document.layers[0].layer_id,
                                                                                 layer_sample_x + 1u,
                                                                                 layer_sample_y,
                                                                                 &loaded_surface_base_layer_sample),
                           "layer_snapshot_loaded_surface_base_true_color_sample")) {
                return 1;
            }
            if (loaded_surface_base_layer_sample != base_layer_sample ||
                loaded_surface_base_layer_sample == composed_document_sample) {
                fprintf(stderr,
                        "lifecycle_test: expected texture surface base layer to avoid composed snapshot bake got base=%08x composite=%08x\n",
                        (unsigned)loaded_surface_base_layer_sample,
                        (unsigned)composed_document_sample);
                return 1;
            }
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
        (void)unlink(layer_pack_path);
        (void)unlink(legacy_pack_path);
    }

    return 0;
}
