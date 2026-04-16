#include <stdio.h>
#include <unistd.h>

#include "drawing_program/drawing_program_app_main.h"
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
        if (!expect_ok(drawing_program_snapshot_load(ctx, pack_path), "snapshot_load")) {
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
        DrawingProgramAppContext layer_save_ctx;
        DrawingProgramAppContext layer_load_ctx;
        DrawingProgramAppContext legacy_load_ctx;
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
        DrawingProgramAppContext object_save_ctx;
        DrawingProgramAppContext object_load_ctx;
        DrawingProgramAppContext legacy_object_ctx;
        DrawingProgramAppContext legacy_v1_object_ctx;
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
        seed_object.stroke_color_index = 7u;
        seed_object.fill_color_index = 2u;
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
        DrawingProgramAppContext path_save_ctx;
        DrawingProgramAppContext path_load_ctx;
        DrawingProgramAppContext path_invalid_ctx;
        DrawingProgramObjectRecord rect_seed;
        DrawingProgramObjectRecord path_style_seed;
        DrawingProgramPathPayload path_payload;
        DrawingProgramPathPayload loaded_path_payload;
        const DrawingProgramObjectRecord *loaded_path_object = 0;
        const DrawingProgramObjectRecord *loaded_rect_object = 0;
        char path_arg0[] = "drawing_program_path_snapshot_roundtrip";
        char path_arg1[] = "--headless";
        char path_arg2[] = "--smoke-frames";
        char path_arg3[] = "1";
        char path_arg4[] = "--preset";
        char path_arg5[] = "/tmp/drawing_program_path_roundtrip.pack";
        char *path_argv[] = { path_arg0, path_arg1, path_arg2, path_arg3, path_arg4, path_arg5, 0 };
        const char *invalid_path_pack = "/tmp/drawing_program_path_roundtrip_invalid.pack";
        uint32_t rect_object_id = 0u;
        uint32_t path_object_id = 0u;
        uint32_t i;
        (void)unlink(path_arg5);
        (void)unlink(invalid_path_pack);
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
        rect_seed.fill_color_index = 3u;
        rect_seed.stroke_color_index = 7u;
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
        path_style_seed.stroke_color_index = 6u;
        path_style_seed.fill_color_index = 2u;
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
        if (!expect_ok(drawing_program_app_shutdown(&path_invalid_ctx), "path_invalid_shutdown_load")) {
            return 1;
        }
        (void)unlink(path_arg5);
        (void)unlink(invalid_path_pack);
    }

    return 0;
}
