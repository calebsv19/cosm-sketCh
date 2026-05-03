#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_export_image.h"
#include "drawing_program/drawing_program_history.h"
#include "drawing_program/drawing_program_icns_export.h"
#include "drawing_program/drawing_program_iconset_export.h"
#include "drawing_program/drawing_program_png_export.h"
#include "drawing_program/drawing_program_project_state.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_ui_color_state.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program_lifecycle_export_suite.h"
#include "drawing_program_lifecycle_test_support.h"

static const char *k_export_suite_iconset_files[] = {
    "icon_16x16.png",
    "icon_16x16@2x.png",
    "icon_32x32.png",
    "icon_32x32@2x.png",
    "icon_128x128.png",
    "icon_128x128@2x.png",
    "icon_256x256.png",
    "icon_256x256@2x.png",
    "icon_512x512.png",
    "icon_512x512@2x.png"
};

static int export_suite_prepare_dir(const char *path) {
    if (mkdir(path, 0775) != 0 && access(path, F_OK) != 0) {
        fprintf(stderr, "lifecycle_test: failed to create export suite dir %s\n", path);
        return 0;
    }
    return 1;
}

static int export_suite_expect_png_signature(const char *path) {
    unsigned char png_sig[8];
    FILE *png_file = fopen(path, "rb");
    if (!png_file) {
        fprintf(stderr, "lifecycle_test: expected PNG file at %s\n", path);
        return 0;
    }
    if (fread(png_sig, 1u, sizeof(png_sig), png_file) != sizeof(png_sig)) {
        (void)fclose(png_file);
        fprintf(stderr, "lifecycle_test: failed to read PNG signature from %s\n", path);
        return 0;
    }
    (void)fclose(png_file);
    if (!(png_sig[0] == 0x89u && png_sig[1] == 0x50u && png_sig[2] == 0x4Eu && png_sig[3] == 0x47u &&
          png_sig[4] == 0x0Du && png_sig[5] == 0x0Au && png_sig[6] == 0x1Au && png_sig[7] == 0x0Au)) {
        fprintf(stderr, "lifecycle_test: expected PNG signature at %s\n", path);
        return 0;
    }
    return 1;
}

static int export_suite_expect_icns_signature(const char *path) {
    unsigned char icns_sig[4];
    FILE *icns_file = fopen(path, "rb");
    if (!icns_file) {
        fprintf(stderr, "lifecycle_test: expected icns file at %s\n", path);
        return 0;
    }
    if (fread(icns_sig, 1u, sizeof(icns_sig), icns_file) != sizeof(icns_sig)) {
        (void)fclose(icns_file);
        fprintf(stderr, "lifecycle_test: failed to read icns signature from %s\n", path);
        return 0;
    }
    (void)fclose(icns_file);
    if (!(icns_sig[0] == 'i' && icns_sig[1] == 'c' && icns_sig[2] == 'n' && icns_sig[3] == 's')) {
        fprintf(stderr, "lifecycle_test: expected icns signature at %s\n", path);
        return 0;
    }
    return 1;
}

static int export_suite_expect_macos_icon_mask_shape(void) {
    uint8_t src_rgba[4u * 4u * 4u];
    uint8_t *icon_rgba = 0;
    CoreResult result;
    size_t top_left_index = 0u;
    size_t top_center_index = (((size_t)0u * (size_t)128u) + (size_t)64u) * 4u;
    size_t center_index = (((size_t)64u * (size_t)128u) + (size_t)64u) * 4u;
    memset(src_rgba, 255, sizeof(src_rgba));
    result = drawing_program_export_image_build_macos_icon_rgba(src_rgba, 4u, 4u, 128u, &icon_rgba);
    if (result.code != CORE_OK || !icon_rgba) {
        fprintf(stderr, "lifecycle_test: expected macOS icon helper to succeed\n");
        return 0;
    }
    if (icon_rgba[top_left_index + 3u] != 0u || icon_rgba[top_center_index + 3u] != 0u) {
        fprintf(stderr, "lifecycle_test: expected macOS icon mask to clear outer corners and top inset\n");
        core_free(icon_rgba);
        return 0;
    }
    if (icon_rgba[center_index + 0u] != 255u ||
        icon_rgba[center_index + 1u] != 255u ||
        icon_rgba[center_index + 2u] != 255u ||
        icon_rgba[center_index + 3u] != 255u) {
        fprintf(stderr, "lifecycle_test: expected macOS icon mask to preserve opaque center sample\n");
        core_free(icon_rgba);
        return 0;
    }
    core_free(icon_rgba);
    return 1;
}

int drawing_program_lifecycle_run_export_suite(void) {
    static DrawingProgramAppContext ctx;
    static DrawingProgramAppContext boot_ctx;
    static DrawingProgramAppContext prefs_save_ctx;
    static DrawingProgramAppContext prefs_load_ctx;
    char suite_root[] = "/tmp/drawing_program_export_suite";
    char runtime_root[] = "/tmp/drawing_program_export_suite/runtime";
    char input_root[] = "/tmp/drawing_program_export_suite/input";
    char alt_input_root[] = "/tmp/drawing_program_export_suite/alt_input";
    char prefs_input_root[] = "/tmp/drawing_program_export_suite/prefs_input";
    char prefs_output_root[] = "/tmp/drawing_program_export_suite/prefs_output";
    char output_root[] = "/tmp/drawing_program_export_suite/output";
    char expected_project_path[] = "/tmp/drawing_program_export_suite/input/icon_project_001.pack";
    char expected_blank_path[] = "/tmp/drawing_program_export_suite/input/icon_project_002.pack";
    char expected_preset_path[] = "/tmp/drawing_program_export_suite/runtime/last_session.pack";
    char expected_save_as_path[] = "/tmp/drawing_program_export_suite/alt_input/custom_icon.pack";
    char expected_png_path[] = "/tmp/drawing_program_export_suite/output/icon_project_001.png";
    char expected_iconset_path[] = "/tmp/drawing_program_export_suite/output/icon_project_001.iconset";
    char expected_icns_path[] = "/tmp/drawing_program_export_suite/output/icon_project_001.icns";
    char expected_prefs_project_path[] = "/tmp/drawing_program_export_suite/prefs_input/persisted_icon.pack";
    char expanded_queue_paths[][DRAWING_PROGRAM_PROJECT_PATH_CAPACITY] = {
        "/tmp/drawing_program_export_suite/input/icon_project_005.pack",
        "/tmp/drawing_program_export_suite/input/icon_project_006.pack",
        "/tmp/drawing_program_export_suite/input/icon_project_007.pack",
        "/tmp/drawing_program_export_suite/input/icon_project_008.pack",
        "/tmp/drawing_program_export_suite/input/icon_project_009.pack",
        "/tmp/drawing_program_export_suite/input/icon_project_010.pack",
        "/tmp/drawing_program_export_suite/input/icon_project_011.pack",
        "/tmp/drawing_program_export_suite/input/icon_project_012.pack",
        "/tmp/drawing_program_export_suite/input/icon_project_013.pack",
        "/tmp/drawing_program_export_suite/input/icon_project_014.pack"
    };
    char resolved_png_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    char resolved_iconset_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    char resolved_icns_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    DrawingProgramObjectRecord object_seed;
    uint32_t object_id = 0u;
    uint8_t project_exists = 0u;
    uint8_t sample_value = 0u;
    uint8_t *composed_rgba = 0;
    uint32_t composed_width = 0u;
    uint32_t composed_height = 0u;
    DrawingProgramRasterSample arbitrary_raster_sample;
    uint8_t arbitrary_r = 17u;
    uint8_t arbitrary_g = 89u;
    uint8_t arbitrary_b = 201u;
    uint8_t expected_fill_r = 0u;
    uint8_t expected_fill_g = 0u;
    uint8_t expected_fill_b = 0u;
    uint8_t queue_existing = 0u;
    char queue_slot_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    SDL_Rect right_rect = {960, 0, 320, 800};
    SDL_Rect queue_rect;
    SDL_Rect last_queue_row;
    VisualPaneLayoutMetrics metrics;
    uint32_t queue_slot_count = 0u;
    int queue_scroll_max = 0;
    char arg0[] = "drawing_program_export_suite";
    char arg1[] = "--headless";
    char arg2[] = "--smoke-frames";
    char arg3[] = "1";
    char arg4[] = "--no-persist";
    char *argv[] = { arg0, arg1, arg2, arg3, arg4, 0 };
    char boot_arg0[] = "drawing_program_export_suite_boot";
    char boot_arg1[] = "--headless";
    char boot_arg2[] = "--smoke-frames";
    char boot_arg3[] = "1";
    char boot_arg4[] = "--no-persist";
    char boot_arg5[] = "--runtime-root";
    char boot_arg6[] = "/tmp/drawing_program_export_suite/runtime";
    char boot_arg7[] = "--input-root";
    char boot_arg8[] = "/tmp/drawing_program_export_suite/input";
    char boot_arg9[] = "--output-root";
    char boot_arg10[] = "/tmp/drawing_program_export_suite/output";
    char *boot_argv[] = {
        boot_arg0, boot_arg1, boot_arg2, boot_arg3, boot_arg4, boot_arg5, boot_arg6, boot_arg7, boot_arg8, boot_arg9,
        boot_arg10, 0
    };
    char prefs_arg0[] = "drawing_program_export_suite_prefs_save";
    char prefs_arg1[] = "--headless";
    char prefs_arg2[] = "--smoke-frames";
    char prefs_arg3[] = "1";
    char prefs_arg4[] = "--runtime-root";
    char prefs_arg5[] = "/tmp/drawing_program_export_suite/runtime";
    char prefs_arg6[] = "--input-root";
    char prefs_arg7[] = "/tmp/drawing_program_export_suite/input";
    char prefs_arg8[] = "--output-root";
    char prefs_arg9[] = "/tmp/drawing_program_export_suite/output";
    char *prefs_save_argv[] = {
        prefs_arg0, prefs_arg1, prefs_arg2, prefs_arg3, prefs_arg4, prefs_arg5, prefs_arg6, prefs_arg7, prefs_arg8,
        prefs_arg9, 0
    };
    char prefs_load_arg0[] = "drawing_program_export_suite_prefs_load";
    char prefs_load_arg1[] = "--headless";
    char prefs_load_arg2[] = "--smoke-frames";
    char prefs_load_arg3[] = "1";
    char prefs_load_arg4[] = "--runtime-root";
    char prefs_load_arg5[] = "/tmp/drawing_program_export_suite/runtime";
    char *prefs_load_argv[] = {
        prefs_load_arg0, prefs_load_arg1, prefs_load_arg2, prefs_load_arg3, prefs_load_arg4, prefs_load_arg5, 0
    };

    uint32_t i;

    (void)mkdir(suite_root, 0775);
    if (!export_suite_prepare_dir(runtime_root) ||
        !export_suite_prepare_dir(input_root) ||
        !export_suite_prepare_dir(alt_input_root) ||
        !export_suite_prepare_dir(prefs_input_root) ||
        !export_suite_prepare_dir(prefs_output_root) ||
        !export_suite_prepare_dir(output_root)) {
        return 1;
    }
    (void)unlink(expected_project_path);
    (void)unlink(expected_blank_path);
    (void)unlink(expected_preset_path);
    (void)unlink(expected_prefs_project_path);
    (void)unlink(expected_save_as_path);
    (void)unlink(expected_png_path);
    (void)unlink(expected_icns_path);
    for (i = 0u; i < (uint32_t)(sizeof(expanded_queue_paths) / sizeof(expanded_queue_paths[0])); ++i) {
        (void)unlink(expanded_queue_paths[i]);
    }
    for (i = 0u; i < (uint32_t)(sizeof(k_export_suite_iconset_files) / sizeof(k_export_suite_iconset_files[0])); ++i) {
        char icon_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        (void)snprintf(icon_path, sizeof(icon_path), "%s/%s", expected_iconset_path, k_export_suite_iconset_files[i]);
        (void)unlink(icon_path);
    }
    (void)rmdir(expected_iconset_path);

    if (!expect_ok(drawing_program_app_bootstrap(&ctx, 5, argv), "export_suite_bootstrap")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&ctx), "export_suite_config")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&ctx), "export_suite_state_seed")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&ctx), "export_suite_subsystems")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&ctx), "export_suite_runtime_start")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_set_input_root(&ctx, input_root), "export_suite_set_input_root")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_set_output_root(&ctx, output_root), "export_suite_set_output_root")) {
        return 1;
    }
    if (!ctx.session.project_path || strcmp(ctx.session.project_path, expected_project_path) != 0) {
        fprintf(stderr, "lifecycle_test: expected export suite project path %s got %s\n",
                expected_project_path,
                ctx.session.project_path ? ctx.session.project_path : "(null)");
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_BRUSH),
                   "export_suite_set_brush")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE),
                   "export_suite_stamp")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_save_current(&ctx), "export_suite_save_project")) {
        return 1;
    }
    for (i = 0u; i < (uint32_t)(sizeof(expanded_queue_paths) / sizeof(expanded_queue_paths[0])); ++i) {
        if (!expect_ok(drawing_program_snapshot_save(&ctx, expanded_queue_paths[i]), "export_suite_seed_queue_pack")) {
            return 1;
        }
    }
    if (!expect_ok(drawing_program_project_state_refresh_recent(&ctx), "export_suite_refresh_recent_queue")) {
        return 1;
    }
    if (ctx.session.recent_project_count <= 8u) {
        fprintf(stderr, "lifecycle_test: expected expanded target queue count > 8 got=%u\n",
                (unsigned)ctx.session.recent_project_count);
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_slot_path(&ctx, 8u, queue_slot_path, sizeof(queue_slot_path), &queue_existing),
                   "export_suite_recent_queue_slot")) {
        return 1;
    }
    if (!queue_existing || queue_slot_path[0] == '\0') {
        fprintf(stderr, "lifecycle_test: expected slot 9 to resolve to an existing saved project in expanded queue\n");
        return 1;
    }
    metrics = make_pane_layout_metrics(&ctx);
    queue_rect = right_file_target_queue_rect(right_rect, metrics, 10u, 5u);
    queue_slot_count = right_file_target_queue_slot_count(&ctx);
    queue_scroll_max = right_file_target_queue_scroll_max(queue_rect, metrics, queue_slot_count);
    if (queue_slot_count <= 8u || queue_scroll_max <= 0) {
        fprintf(stderr,
                "lifecycle_test: expected target queue layout to scroll beyond 8 slots count=%u max=%d\n",
                (unsigned)queue_slot_count,
                queue_scroll_max);
        return 1;
    }
    last_queue_row = right_file_target_queue_row_rect(queue_rect, metrics, queue_slot_count - 1u, queue_scroll_max);
    if (last_queue_row.y != queue_rect.y) {
        fprintf(stderr,
                "lifecycle_test: expected bottom target queue scroll to align last row at top got row_y=%d queue_y=%d\n",
                last_queue_row.y,
                queue_rect.y);
        return 1;
    }
    if (right_file_target_queue_clamp_scroll(queue_rect, metrics, queue_slot_count, queue_scroll_max + 1000) !=
        queue_scroll_max) {
        fprintf(stderr, "lifecycle_test: expected target queue scroll clamp at max\n");
        return 1;
    }
    if (access(expected_project_path, F_OK) != 0) {
        fprintf(stderr, "lifecycle_test: expected export suite project pack to exist at %s\n", expected_project_path);
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&ctx.document,
                                                        ctx.document.raster_width / 2u,
                                                        ctx.document.raster_height / 2u,
                                                        &sample_value),
                   "export_suite_read_saved_project_sample")) {
        return 1;
    }
    if (sample_value ==
        drawing_program_color_legacy_sample_from_sample(drawing_program_color_eraser_value())) {
        fprintf(stderr, "lifecycle_test: expected saved project sample to be non-eraser before boot precedence check\n");
        return 1;
    }
    ctx.editor.active_tool = DRAWING_PROGRAM_TOOL_ERASER;
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE),
                   "export_suite_stamp_runtime_preset_source")) {
        return 1;
    }
    if (!expect_ok(drawing_program_snapshot_save(&ctx, expected_preset_path), "export_suite_save_runtime_preset")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_set_current_path(&ctx, expected_project_path),
                   "export_suite_restore_current_project_path")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_load_current(&ctx), "export_suite_restore_current_project")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_bootstrap(&boot_ctx, 11, boot_argv), "export_suite_bootstrap_project_boot")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&boot_ctx), "export_suite_config_project_boot")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&boot_ctx), "export_suite_state_seed_project_boot")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&boot_ctx), "export_suite_subsystems_project_boot")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&boot_ctx), "export_suite_runtime_start_project_boot")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&boot_ctx.document,
                                                        boot_ctx.document.raster_width / 2u,
                                                        boot_ctx.document.raster_height / 2u,
                                                        &sample_value),
                   "export_suite_read_boot_project_sample")) {
        return 1;
    }
    if (sample_value ==
        drawing_program_color_legacy_sample_from_sample(drawing_program_color_eraser_value())) {
        fprintf(stderr, "lifecycle_test: expected boot to prefer saved project over runtime autosave scratch\n");
        return 1;
    }
    if (!expect_ok(drawing_program_app_bootstrap(&prefs_save_ctx, 10, prefs_save_argv),
                   "export_suite_bootstrap_prefs_save")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&prefs_save_ctx), "export_suite_config_prefs_save")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&prefs_save_ctx), "export_suite_state_seed_prefs_save")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&prefs_save_ctx), "export_suite_subsystems_prefs_save")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&prefs_save_ctx), "export_suite_runtime_start_prefs_save")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_set_input_root(&prefs_save_ctx, prefs_input_root),
                   "export_suite_set_persisted_input_root")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_set_output_root(&prefs_save_ctx, prefs_output_root),
                   "export_suite_set_persisted_output_root")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_set_current_path(&prefs_save_ctx, expected_prefs_project_path),
                   "export_suite_set_persisted_project_path")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_shutdown(&prefs_save_ctx), "export_suite_shutdown_prefs_save")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_bootstrap(&prefs_load_ctx, 6, prefs_load_argv),
                   "export_suite_bootstrap_prefs_load")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&prefs_load_ctx), "export_suite_config_prefs_load")) {
        return 1;
    }
    if (strcmp(prefs_load_ctx.session.input_root_path, prefs_input_root) != 0 ||
        strcmp(prefs_load_ctx.session.output_root_path, prefs_output_root) != 0 ||
        !prefs_load_ctx.session.project_path ||
        strcmp(prefs_load_ctx.session.project_path, expected_prefs_project_path) != 0) {
        fprintf(stderr,
                "lifecycle_test: expected persisted roots/project input=%s output=%s project=%s got input=%s output=%s project=%s\n",
                prefs_input_root,
                prefs_output_root,
                expected_prefs_project_path,
                prefs_load_ctx.session.input_root_path,
                prefs_load_ctx.session.output_root_path,
                prefs_load_ctx.session.project_path ? prefs_load_ctx.session.project_path : "(null)");
        return 1;
    }
    memset(&object_seed, 0, sizeof(object_seed));
    object_seed.layer_id = ctx.editor.active_layer_id;
    object_seed.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT;
    object_seed.visible = 1u;
    object_seed.locked = 0u;
    object_seed.stroke_color_value = drawing_program_color_value_from_index(2u);
    object_seed.fill_color_value = drawing_program_color_value_from_index(4u);
    object_seed.stroke_width = 1u;
    object_seed.style_mode = 2u;
    object_seed.origin_x = 24;
    object_seed.origin_y = 24;
    object_seed.width = 32u;
    object_seed.height = 32u;
    if (!expect_ok(drawing_program_object_store_add(&ctx.object_store, &object_seed, &object_id), "export_suite_add_object")) {
        return 1;
    }
    if (ctx.object_store.object_count != 1u) {
        fprintf(stderr, "lifecycle_test: expected object count 1 before new blank project got=%u\n",
                (unsigned)ctx.object_store.object_count);
        return 1;
    }
    drawing_program_object_selection_replace_single(&ctx.object_selection, object_id);
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_CANVAS),
                   "export_suite_clear_canvas_preserve_objects")) {
        return 1;
    }
    if (ctx.object_store.object_count != 1u || ctx.object_selection.count != 1u) {
        fprintf(stderr,
                "lifecycle_test: expected clear canvas to preserve objects/selection got objects=%u selected=%u\n",
                (unsigned)ctx.object_store.object_count,
                (unsigned)ctx.object_selection.count);
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&ctx.document,
                                                        ctx.document.raster_width / 2u,
                                                        ctx.document.raster_height / 2u,
                                                        &sample_value),
                   "export_suite_clear_canvas_sample")) {
        return 1;
    }
    if (sample_value !=
        drawing_program_color_legacy_sample_from_sample(drawing_program_color_eraser_value())) {
        fprintf(stderr, "lifecycle_test: expected clear canvas to erase raster samples\n");
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_orchestration_apply_workflow_control(
                       &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_CLEAR_OBJECTS),
                   "export_suite_clear_objects")) {
        return 1;
    }
    if (ctx.object_store.object_count != 0u || ctx.object_selection.count != 0u || ctx.object_selection.active_object_id != 0u) {
        fprintf(stderr,
                "lifecycle_test: expected clear objects to reset object store/selection got objects=%u selected=%u active=%u\n",
                (unsigned)ctx.object_store.object_count,
                (unsigned)ctx.object_selection.count,
                (unsigned)ctx.object_selection.active_object_id);
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_begin_new_blank(&ctx), "export_suite_begin_new_blank")) {
        return 1;
    }
    if (!ctx.session.project_path || strcmp(ctx.session.project_path, expected_blank_path) != 0) {
        fprintf(stderr, "lifecycle_test: expected blank project target %s got %s\n",
                expected_blank_path,
                ctx.session.project_path ? ctx.session.project_path : "(null)");
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_current_exists(&ctx, &project_exists),
                   "export_suite_blank_project_exists_check")) {
        return 1;
    }
    if (project_exists) {
        fprintf(stderr, "lifecycle_test: expected blank new project target to be unsaved\n");
        return 1;
    }
    if (ctx.object_store.object_count != 0u || ctx.history.count != 0u || ctx.history.cursor != 0u) {
        fprintf(stderr,
                "lifecycle_test: expected blank new project to reset objects/history got objects=%u history=%u cursor=%u\n",
                (unsigned)ctx.object_store.object_count,
                (unsigned)ctx.history.count,
                (unsigned)ctx.history.cursor);
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&ctx.document,
                                                        ctx.document.raster_width / 2u,
                                                        ctx.document.raster_height / 2u,
                                                        &sample_value),
                   "export_suite_blank_project_sample")) {
        return 1;
    }
    if (sample_value !=
        drawing_program_color_legacy_sample_from_sample(drawing_program_color_eraser_value())) {
        fprintf(stderr, "lifecycle_test: expected blank new project raster to reset to eraser sample\n");
        return 1;
    }
    if (drawing_program_project_state_load_current(&ctx).code == CORE_OK) {
        fprintf(stderr, "lifecycle_test: expected load current on unsaved blank target to fail\n");
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_set_current_path(&ctx, expected_project_path),
                   "export_suite_reset_current_path_after_blank")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_load_current(&ctx), "export_suite_reload_saved_project_after_blank")) {
        return 1;
    }
    arbitrary_raster_sample = drawing_program_color_value_from_rgb(arbitrary_r, arbitrary_g, arbitrary_b);
    if (!expect_ok(drawing_program_history_apply_set_sample_value(&ctx.history,
                                                                  &ctx.document,
                                                                  &ctx.layer_rasters,
                                                                  ctx.editor.active_layer_id,
                                                                  8u,
                                                                  8u,
                                                                  arbitrary_raster_sample),
                   "export_suite_set_arbitrary_true_color_sample")) {
        return 1;
    }
    memset(&object_seed, 0, sizeof(object_seed));
    object_seed.layer_id = ctx.editor.active_layer_id;
    object_seed.type = (uint8_t)DRAWING_PROGRAM_OBJECT_TYPE_RECT;
    object_seed.visible = 1u;
    object_seed.locked = 0u;
    object_seed.stroke_color_value = drawing_program_color_value_from_index(2u);
    object_seed.fill_color_value = drawing_program_color_value_from_index(4u);
    object_seed.stroke_width = 1u;
    object_seed.style_mode = 2u;
    object_seed.origin_x = 24;
    object_seed.origin_y = 24;
    object_seed.width = 32u;
    object_seed.height = 32u;
    if (!expect_ok(drawing_program_object_store_add(&ctx.object_store, &object_seed, &object_id),
                   "export_suite_add_export_object")) {
        return 1;
    }
    drawing_program_color_rgb_from_sample(object_seed.fill_color_value,
                                          &expected_fill_r,
                                          &expected_fill_g,
                                          &expected_fill_b);
    if (!expect_ok(drawing_program_export_image_compose_document_rgba(&ctx,
                                                                      &composed_rgba,
                                                                      &composed_width,
                                                                      &composed_height),
                   "export_suite_compose_rgba_with_object")) {
        return 1;
    }
    if (!export_suite_expect_macos_icon_mask_shape()) {
        core_free(composed_rgba);
        return 1;
    }
    if (!composed_rgba || composed_width != ctx.document.raster_width || composed_height != ctx.document.raster_height) {
        fprintf(stderr, "lifecycle_test: expected composed RGBA buffer sized to document\n");
        core_free(composed_rgba);
        return 1;
    }
    {
        size_t raster_index = (((size_t)8u * (size_t)composed_width) + (size_t)8u) * 4u;
        size_t object_center_index = (((size_t)32u * (size_t)composed_width) + (size_t)32u) * 4u;
        if (composed_rgba[raster_index + 3u] != 255u ||
            composed_rgba[raster_index + 0u] != arbitrary_r ||
            composed_rgba[raster_index + 1u] != arbitrary_g ||
            composed_rgba[raster_index + 2u] != arbitrary_b) {
            fprintf(stderr,
                    "lifecycle_test: expected export compose raster pixel rgba=%u,%u,%u,%u got=%u,%u,%u,%u\n",
                    (unsigned)arbitrary_r,
                    (unsigned)arbitrary_g,
                    (unsigned)arbitrary_b,
                    255u,
                    (unsigned)composed_rgba[raster_index + 0u],
                    (unsigned)composed_rgba[raster_index + 1u],
                    (unsigned)composed_rgba[raster_index + 2u],
                    (unsigned)composed_rgba[raster_index + 3u]);
            core_free(composed_rgba);
            return 1;
        }
        if (composed_rgba[object_center_index + 3u] == 0u ||
            composed_rgba[object_center_index + 0u] != expected_fill_r ||
            composed_rgba[object_center_index + 1u] != expected_fill_g ||
            composed_rgba[object_center_index + 2u] != expected_fill_b) {
            fprintf(stderr,
                    "lifecycle_test: expected export compose object pixel rgba=%u,%u,%u,%u got=%u,%u,%u,%u\n",
                    (unsigned)expected_fill_r,
                    (unsigned)expected_fill_g,
                    (unsigned)expected_fill_b,
                    255u,
                    (unsigned)composed_rgba[object_center_index + 0u],
                    (unsigned)composed_rgba[object_center_index + 1u],
                    (unsigned)composed_rgba[object_center_index + 2u],
                    (unsigned)composed_rgba[object_center_index + 3u]);
            core_free(composed_rgba);
            return 1;
        }
    }
    core_free(composed_rgba);
    composed_rgba = 0;
    if (!expect_ok(drawing_program_png_export_default_path(&ctx, resolved_png_path, sizeof(resolved_png_path)),
                   "export_suite_default_png_path")) {
        return 1;
    }
    if (strcmp(resolved_png_path, expected_png_path) != 0) {
        fprintf(stderr, "lifecycle_test: expected export path %s got %s\n", expected_png_path, resolved_png_path);
        return 1;
    }
    if (!expect_ok(drawing_program_png_export_current_document(&ctx, resolved_png_path), "export_suite_export_png")) {
        return 1;
    }
    if (ctx.object_store.object_count != 1u) {
        fprintf(stderr, "lifecycle_test: expected PNG export to preserve live objects got=%u\n",
                (unsigned)ctx.object_store.object_count);
        return 1;
    }
    if (!export_suite_expect_png_signature(resolved_png_path)) {
        return 1;
    }
    if (!expect_ok(drawing_program_iconset_export_default_path(&ctx,
                                                               resolved_iconset_path,
                                                               sizeof(resolved_iconset_path)),
                   "export_suite_default_iconset_path")) {
        return 1;
    }
    if (strcmp(resolved_iconset_path, expected_iconset_path) != 0) {
        fprintf(stderr, "lifecycle_test: expected iconset path %s got %s\n", expected_iconset_path, resolved_iconset_path);
        return 1;
    }
    if (!expect_ok(drawing_program_iconset_export_current_document(&ctx, resolved_iconset_path),
                   "export_suite_export_iconset")) {
        return 1;
    }
    if (ctx.object_store.object_count != 1u) {
        fprintf(stderr, "lifecycle_test: expected iconset export to preserve live objects got=%u\n",
                (unsigned)ctx.object_store.object_count);
        return 1;
    }
    for (i = 0u; i < (uint32_t)(sizeof(k_export_suite_iconset_files) / sizeof(k_export_suite_iconset_files[0])); ++i) {
        char icon_path[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
        (void)snprintf(icon_path, sizeof(icon_path), "%s/%s", resolved_iconset_path, k_export_suite_iconset_files[i]);
        if (!export_suite_expect_png_signature(icon_path)) {
            return 1;
        }
    }
    if (access(resolved_iconset_path, F_OK) != 0) {
        fprintf(stderr, "lifecycle_test: expected iconset directory at %s\n", resolved_iconset_path);
        return 1;
    }
    if (!expect_ok(drawing_program_icns_export_default_path(&ctx, resolved_icns_path, sizeof(resolved_icns_path)),
                   "export_suite_default_icns_path")) {
        return 1;
    }
    if (strcmp(resolved_icns_path, expected_icns_path) != 0) {
        fprintf(stderr, "lifecycle_test: expected icns path %s got %s\n", expected_icns_path, resolved_icns_path);
        return 1;
    }
    if (!expect_ok(drawing_program_icns_export_current_document(&ctx, resolved_icns_path), "export_suite_export_icns")) {
        return 1;
    }
    if (ctx.object_store.object_count != 1u) {
        fprintf(stderr, "lifecycle_test: expected icns export to preserve live objects got=%u\n",
                (unsigned)ctx.object_store.object_count);
        return 1;
    }
    if (!export_suite_expect_icns_signature(resolved_icns_path)) {
        return 1;
    }
    drawing_program_ui_color_load_active_paint_from_swatch(&ctx, 7u);
    if (!expect_ok(drawing_program_project_state_set_save_as_path(&ctx,
                                                                  "/tmp/drawing_program_export_suite/alt_input/custom_icon"),
                   "export_suite_set_save_as_path")) {
        return 1;
    }
    if (!ctx.session.project_path || strcmp(ctx.session.project_path, expected_save_as_path) != 0) {
        fprintf(stderr, "lifecycle_test: expected save-as project path %s got %s\n",
                expected_save_as_path,
                ctx.session.project_path ? ctx.session.project_path : "(null)");
        return 1;
    }
    if (strcmp(ctx.session.input_root_path, alt_input_root) != 0) {
        fprintf(stderr, "lifecycle_test: expected save-as input root %s got %s\n",
                alt_input_root,
                ctx.session.input_root_path);
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_save_current(&ctx), "export_suite_save_as_project")) {
        return 1;
    }
    if (access(expected_save_as_path, F_OK) != 0) {
        fprintf(stderr, "lifecycle_test: expected save-as project pack at %s\n", expected_save_as_path);
        return 1;
    }
    drawing_program_ui_color_load_active_paint_from_swatch(&ctx, 3u);
    if (!expect_ok(drawing_program_project_state_set_current_path(&ctx, expected_save_as_path),
                   "export_suite_set_current_path")) {
        return 1;
    }
    if (!expect_ok(drawing_program_project_state_load_current(&ctx), "export_suite_load_save_as_project")) {
        return 1;
    }
    if (ctx.ui.active_color_index != 7u) {
        fprintf(stderr, "lifecycle_test: expected load from save-as path to restore saved ui state\n");
        return 1;
    }
    return 0;
}
