#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <json-c/json.h>
#include <png.h>

#include "core_authored_texture.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_snapshot.h"
#include "drawing_program/drawing_program_texture_export_intent.h"
#include "drawing_program/drawing_program_texture_layer_material_intent.h"
#include "drawing_program/drawing_program_texture_overlay_material_intent.h"
#include "drawing_program/drawing_program_texture_export.h"
#include "drawing_program/drawing_program_texture_project_session.h"
#include "drawing_program/drawing_program_visual_layer_actions.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"
#include "drawing_program/drawing_program_visual_layer_roles.h"
#include "drawing_program_lifecycle_snapshot_helpers.h"
#include "drawing_program_lifecycle_test_support.h"

static int texture_export_prepare_dir(const char *path) {
    if (mkdir(path, 0775) != 0 && access(path, F_OK) != 0) {
        fprintf(stderr, "lifecycle_test: failed to create texture export dir %s\n", path);
        return 0;
    }
    return 1;
}

static int texture_export_expect_material_intent_contract(void) {
    if (strcmp(drawing_program_texture_layer_material_intent_stable_id(
                   DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_CONCRETE),
               "concrete") != 0) {
        fprintf(stderr, "lifecycle_test: expected concrete material intent stable id\n");
        return 0;
    }
    if (!drawing_program_texture_layer_material_intent_is_base_family(
            DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_CONCRETE)) {
        fprintf(stderr, "lifecycle_test: expected concrete intent to be base family\n");
        return 0;
    }
    if (!drawing_program_texture_layer_material_intent_is_overlay_family(
            DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_OIL)) {
        fprintf(stderr, "lifecycle_test: expected oil intent to be overlay family\n");
        return 0;
    }
    if (drawing_program_texture_layer_material_intent_default_for_role(
            DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_BASE) !=
        DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_SOLID) {
        fprintf(stderr, "lifecycle_test: expected base role to default to solid intent\n");
        return 0;
    }
    if (drawing_program_texture_layer_material_intent_from_legacy_overlay_kind(
            DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_GRIME) !=
        DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_GRIME) {
        fprintf(stderr, "lifecycle_test: expected legacy grime overlay intent bridge\n");
        return 0;
    }
    if (drawing_program_texture_layer_material_intent_from_legacy_overlay_kind(
            DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_NONE) !=
        DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE) {
        fprintf(stderr, "lifecycle_test: expected legacy none overlay intent bridge\n");
        return 0;
    }
    return 1;
}

static int texture_export_expect_png_signature(const char *path) {
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

static int texture_export_read_png_first_pixel_rgba(const char *path,
                                                    uint8_t *out_r,
                                                    uint8_t *out_g,
                                                    uint8_t *out_b,
                                                    uint8_t *out_a) {
    FILE *png_file = 0;
    png_structp png_ptr = 0;
    png_infop info_ptr = 0;
    png_bytep image = 0;
    png_bytep *rows = 0;
    png_uint_32 width = 0u;
    png_uint_32 height = 0u;
    int bit_depth = 0;
    int color_type = 0;
    size_t stride = 0u;
    int ok = 0;
    if (!path || !out_r || !out_g || !out_b || !out_a) {
        return 0;
    }
    png_file = fopen(path, "rb");
    if (!png_file) {
        fprintf(stderr, "lifecycle_test: expected PNG file at %s for pixel read\n", path);
        return 0;
    }
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!png_ptr) {
        fclose(png_file);
        return 0;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, 0, 0);
        fclose(png_file);
        return 0;
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        free(rows);
        free(image);
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        fclose(png_file);
        return 0;
    }
    png_init_io(png_ptr, png_file);
    png_read_info(png_ptr, info_ptr);
    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    if (bit_depth == 16) {
        png_set_strip_16(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    }
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png_ptr, 0xFFu, PNG_FILLER_AFTER);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png_ptr);
    }
    png_read_update_info(png_ptr, info_ptr);
    stride = png_get_rowbytes(png_ptr, info_ptr);
    image = (png_bytep)malloc(stride * (size_t)height);
    rows = (png_bytep *)malloc(sizeof(*rows) * (size_t)height);
    if (!image || !rows) {
        free(rows);
        free(image);
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        fclose(png_file);
        return 0;
    }
    for (png_uint_32 y = 0u; y < height; ++y) {
        rows[y] = image + ((size_t)y * stride);
    }
    png_read_image(png_ptr, rows);
    if (width == 0u || height == 0u) {
        fprintf(stderr, "lifecycle_test: expected non-empty PNG at %s\n", path);
    } else {
        *out_r = rows[0][0];
        *out_g = rows[0][1];
        *out_b = rows[0][2];
        *out_a = rows[0][3];
        ok = 1;
    }
    free(rows);
    free(image);
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    fclose(png_file);
    return ok;
}

static int write_scene_fixture(const char *path) {
    const char *json =
        "{"
        "\"schema_family\":\"codework_scene\","
        "\"schema_variant\":\"scene_authoring_v1\","
        "\"schema_version\":1,"
        "\"scene_id\":\"scene_texture_exports\","
        "\"space_mode_default\":\"3d\","
        "\"objects\":["
          "{"
            "\"object_id\":\"obj_plane\","
            "\"object_type\":\"plane_primitive\","
            "\"dimensional_mode\":\"plane_locked\","
            "\"locked_plane\":\"yz\","
            "\"primitive\":{"
              "\"kind\":\"plane_primitive\","
              "\"width\":6.0,"
              "\"height\":5.0,"
              "\"lock_to_construction_plane\":true,"
              "\"lock_to_bounds\":false,"
              "\"frame\":{"
                "\"origin\":{\"x\":0.0,\"y\":0.0,\"z\":0.0},"
                "\"axis_u\":{\"x\":0.0,\"y\":1.0,\"z\":0.0},"
                "\"axis_v\":{\"x\":0.0,\"y\":0.0,\"z\":1.0},"
                "\"normal\":{\"x\":1.0,\"y\":0.0,\"z\":0.0}"
              "}"
            "}"
          "},"
          "{"
            "\"object_id\":\"obj_prism\","
            "\"object_type\":\"rect_prism_primitive\","
            "\"dimensional_mode\":\"full_3d\","
            "\"primitive\":{"
              "\"kind\":\"rect_prism_primitive\","
              "\"width\":3.0,"
              "\"height\":2.0,"
              "\"depth\":4.0,"
              "\"lock_to_construction_plane\":false,"
              "\"lock_to_bounds\":true,"
              "\"frame\":{"
                "\"origin\":{\"x\":1.0,\"y\":2.0,\"z\":3.0},"
                "\"axis_u\":{\"x\":1.0,\"y\":0.0,\"z\":0.0},"
                "\"axis_v\":{\"x\":0.0,\"y\":1.0,\"z\":0.0},"
                "\"normal\":{\"x\":0.0,\"y\":0.0,\"z\":1.0}"
              "}"
            "}"
          "}"
        "],"
        "\"materials\":[],"
        "\"lights\":[],"
        "\"cameras\":[]"
        "}";
    FILE *file = fopen(path, "wb");
    if (!file) {
        return 0;
    }
    if (fwrite(json, 1u, strlen(json), file) != strlen(json)) {
        fclose(file);
        return 0;
    }
    fclose(file);
    return 1;
}

static int texture_export_manifest_expect_string(json_object *object,
                                                 const char *key,
                                                 const char *expected) {
    json_object *value = 0;
    const char *actual = 0;
    if (!object || !key || !expected || !json_object_object_get_ex(object, key, &value) || !value) {
        fprintf(stderr, "lifecycle_test: expected manifest key %s\n", key ? key : "(null)");
        return 0;
    }
    actual = json_object_get_string(value);
    if (!actual || strcmp(actual, expected) != 0) {
        fprintf(stderr,
                "lifecycle_test: expected manifest %s=%s got=%s\n",
                key,
                expected,
                actual ? actual : "(null)");
        return 0;
    }
    return 1;
}

static int texture_export_manifest_expect_missing_key(json_object *object,
                                                      const char *key) {
    json_object *value = 0;
    if (!object || !key) {
        fprintf(stderr, "lifecycle_test: expected valid missing-key check\n");
        return 0;
    }
    if (json_object_object_get_ex(object, key, &value)) {
        fprintf(stderr, "lifecycle_test: expected manifest key %s to be absent\n", key);
        return 0;
    }
    return 1;
}

static int texture_export_manifest_expect_shared_contract(
    json_object *manifest,
    CoreAuthoredTexturePrimitiveKind expected_primitive_kind,
    CoreAuthoredTextureOutputKind expected_output_kind,
    int expect_overlay_lane) {
    json_object *schema_value = 0;
    json_object *binding_value = 0;
    json_object *output_value = 0;
    json_object *primitive_value = 0;
    CoreAuthoredTextureManifestContract contract;
    const char *binding_name = 0;
    const char *output_name = 0;
    const char *primitive_name = 0;
    memset(&contract, 0, sizeof(contract));
    if (!manifest ||
        !json_object_object_get_ex(manifest, "schema_version", &schema_value) ||
        !schema_value ||
        json_object_get_type(schema_value) != json_type_int ||
        !json_object_object_get_ex(manifest, "export_binding_kind", &binding_value) ||
        !binding_value ||
        !json_object_object_get_ex(manifest, "emitted_output_kind", &output_value) ||
        !output_value ||
        !json_object_object_get_ex(manifest, "primitive_kind", &primitive_value) ||
        !primitive_value) {
        fprintf(stderr, "lifecycle_test: expected authored-texture manifest contract keys\n");
        return 0;
    }
    binding_name = json_object_get_string(binding_value);
    output_name = json_object_get_string(output_value);
    primitive_name = json_object_get_string(primitive_value);
    contract.schema_version = (uint32_t)json_object_get_int(schema_value);
    contract.has_legacy_surfaces =
        json_object_object_get_ex(manifest, "surfaces", 0) ? true : false;
    contract.has_base_surfaces =
        json_object_object_get_ex(manifest, "base_surfaces", 0) ? true : false;
    contract.has_overlay_surfaces =
        json_object_object_get_ex(manifest, "overlay_surfaces", 0) ? true : false;
    if (!core_authored_texture_binding_kind_parse(binding_name, &contract.binding_kind) ||
        !core_authored_texture_output_kind_parse(output_name, &contract.output_kind) ||
        !core_authored_texture_primitive_kind_parse(primitive_name, &contract.primitive_kind)) {
        fprintf(stderr, "lifecycle_test: expected manifest contract values to parse through shared helpers\n");
        return 0;
    }
    if (!core_authored_texture_manifest_contract_validate(&contract)) {
        fprintf(stderr, "lifecycle_test: expected manifest contract to validate through shared helper\n");
        return 0;
    }
    if (contract.primitive_kind != expected_primitive_kind ||
        contract.output_kind != expected_output_kind ||
        (expect_overlay_lane ? !contract.has_overlay_surfaces : contract.has_overlay_surfaces)) {
        fprintf(stderr,
                "lifecycle_test: unexpected manifest contract primitive=%u output=%u overlay=%u\n",
                (unsigned)contract.primitive_kind,
                (unsigned)contract.output_kind,
                contract.has_overlay_surfaces ? 1u : 0u);
        return 0;
    }
    if (contract.schema_version == CORE_AUTHORED_TEXTURE_SCHEMA_V5 &&
        contract.has_legacy_surfaces) {
        fprintf(stderr, "lifecycle_test: schema v5 manifests must not emit legacy surfaces lane\n");
        return 0;
    }
    return 1;
}

static int texture_export_manifest_expect_int(json_object *object,
                                              const char *key,
                                              int expected) {
    json_object *value = 0;
    int actual = 0;
    if (!object || !key || !json_object_object_get_ex(object, key, &value) || !value) {
        fprintf(stderr, "lifecycle_test: expected manifest key %s\n", key ? key : "(null)");
        return 0;
    }
    actual = json_object_get_int(value);
    if (actual != expected) {
        fprintf(stderr,
                "lifecycle_test: expected manifest %s=%d got=%d\n",
                key,
                expected,
                actual);
        return 0;
    }
    return 1;
}

static int texture_export_manifest_expect_double(json_object *object,
                                                 const char *key,
                                                 double expected,
                                                 double tolerance) {
    json_object *value = 0;
    double actual = 0.0;
    if (!object || !key || !json_object_object_get_ex(object, key, &value) || !value) {
        fprintf(stderr, "lifecycle_test: expected manifest key %s\n", key ? key : "(null)");
        return 0;
    }
    actual = json_object_get_double(value);
    if (actual < expected - tolerance || actual > expected + tolerance) {
        fprintf(stderr,
                "lifecycle_test: expected manifest %s=%f got=%f\n",
                key,
                expected,
                actual);
        return 0;
    }
    return 1;
}

static int texture_export_manifest_expect_array_len(json_object *object,
                                                    const char *key,
                                                    size_t expected) {
    json_object *value = 0;
    if (!object || !key || !json_object_object_get_ex(object, key, &value) || !value ||
        json_object_get_type(value) != json_type_array ||
        json_object_array_length(value) != expected) {
        fprintf(stderr,
                "lifecycle_test: expected manifest array %s len=%zu\n",
                key ? key : "(null)",
                expected);
        return 0;
    }
    return 1;
}

static int texture_export_manifest_expect_string_array_item(json_object *object,
                                                            const char *key,
                                                            size_t index,
                                                            const char *expected) {
    json_object *value = 0;
    json_object *entry = 0;
    const char *actual = 0;
    if (!object || !key || !expected ||
        !json_object_object_get_ex(object, key, &value) || !value ||
        json_object_get_type(value) != json_type_array ||
        index >= json_object_array_length(value)) {
        fprintf(stderr,
                "lifecycle_test: expected manifest string array %s[%zu]\n",
                key ? key : "(null)",
                index);
        return 0;
    }
    entry = json_object_array_get_idx(value, (int)index);
    actual = entry ? json_object_get_string(entry) : 0;
    if (!actual || strcmp(actual, expected) != 0) {
        fprintf(stderr,
                "lifecycle_test: expected manifest %s[%zu]=%s got=%s\n",
                key,
                index,
                expected,
                actual ? actual : "(null)");
        return 0;
    }
    return 1;
}

int drawing_program_lifecycle_run_texture_export_suite(void) {
    static DrawingProgramAppContext direct_ctx;
    static DrawingProgramAppContext reload_ctx;
    static DrawingProgramAppContext legacy_ctx;
    static DrawingProgramAppContext cli_ctx;
    const char *suite_root = "/tmp/drawing_program_texture_export_suite";
    const char *runtime_root = "/tmp/drawing_program_texture_export_suite/runtime";
    const char *input_root = "/tmp/drawing_program_texture_export_suite/input";
    const char *output_root = "/tmp/drawing_program_texture_export_suite/output";
    const char *scene_path = "/tmp/drawing_program_texture_export_suite/scene_fixture.json";
    const char *project_pack_path = "/tmp/drawing_program_texture_export_suite/input/texture_export_project.pack";
    const char *legacy_v5_pack_path = "/tmp/drawing_program_texture_export_suite/input/texture_export_project_v5.pack";
    const char *direct_export_dir = "/tmp/drawing_program_texture_export_suite/output/obj_prism_texture_set";
    const char *direct_manifest_path =
        "/tmp/drawing_program_texture_export_suite/output/obj_prism_texture_set/obj_prism_texture_manifest.json";
    const char *direct_front_png =
        "/tmp/drawing_program_texture_export_suite/output/obj_prism_texture_set/obj_prism_front_base.png";
    const char *direct_back_png =
        "/tmp/drawing_program_texture_export_suite/output/obj_prism_texture_set/obj_prism_back_base.png";
    const char *direct_front_overlay_png =
        "/tmp/drawing_program_texture_export_suite/output/obj_prism_texture_set/obj_prism_front_overlay.png";
    const char *direct_top_png =
        "/tmp/drawing_program_texture_export_suite/output/obj_prism_texture_set/obj_prism_top_base.png";
    const char *hidden_overlay_export_dir =
        "/tmp/drawing_program_texture_export_suite/output/obj_prism_texture_set_hidden_overlay";
    const char *hidden_overlay_manifest_path =
        "/tmp/drawing_program_texture_export_suite/output/obj_prism_texture_set_hidden_overlay/obj_prism_texture_manifest.json";
    const char *reload_export_dir = "/tmp/drawing_program_texture_export_suite/output/obj_prism_texture_set_reload";
    const char *reload_front_png =
        "/tmp/drawing_program_texture_export_suite/output/obj_prism_texture_set_reload/obj_prism_front_base.png";
    const char *reload_front_overlay_png =
        "/tmp/drawing_program_texture_export_suite/output/obj_prism_texture_set_reload/obj_prism_front_overlay.png";
    const char *reload_back_png =
        "/tmp/drawing_program_texture_export_suite/output/obj_prism_texture_set_reload/obj_prism_back_base.png";
    const char *cli_export_dir = "/tmp/drawing_program_texture_export_suite/output/cli_plane_texture_set";
    const char *cli_manifest_path =
        "/tmp/drawing_program_texture_export_suite/output/cli_plane_texture_set/obj_plane_texture_manifest.json";
    const char *cli_front_png =
        "/tmp/drawing_program_texture_export_suite/output/cli_plane_texture_set/obj_plane_front.png";
    const char *legacy_export_dir = "/tmp/drawing_program_texture_export_suite/output/obj_prism_texture_set_legacy_v5";
    const char *legacy_manifest_path =
        "/tmp/drawing_program_texture_export_suite/output/obj_prism_texture_set_legacy_v5/obj_prism_texture_manifest.json";
    const char *legacy_front_png =
        "/tmp/drawing_program_texture_export_suite/output/obj_prism_texture_set_legacy_v5/obj_prism_front.png";
    char direct_arg0[] = "drawing_program_texture_export";
    char direct_arg1[] = "--headless";
    char direct_arg2[] = "--smoke-frames";
    char direct_arg3[] = "1";
    char direct_arg4[] = "--no-persist";
    char direct_arg5[] = "--runtime-root";
    char direct_arg6[] = "/tmp/drawing_program_texture_export_suite/runtime";
    char direct_arg7[] = "--input-root";
    char direct_arg8[] = "/tmp/drawing_program_texture_export_suite/input";
    char direct_arg9[] = "--output-root";
    char direct_arg10[] = "/tmp/drawing_program_texture_export_suite/output";
    char *direct_argv[] = {
        direct_arg0, direct_arg1, direct_arg2, direct_arg3, direct_arg4, direct_arg5,
        direct_arg6, direct_arg7, direct_arg8, direct_arg9, direct_arg10, 0
    };
    char cli_arg0[] = "drawing_program_texture_export_cli";
    char cli_arg1[] = "--headless";
    char cli_arg2[] = "--smoke-frames";
    char cli_arg3[] = "1";
    char cli_arg4[] = "--no-persist";
    char cli_arg5[] = "--runtime-root";
    char cli_arg6[] = "/tmp/drawing_program_texture_export_suite/runtime";
    char cli_arg7[] = "--input-root";
    char cli_arg8[] = "/tmp/drawing_program_texture_export_suite/input";
    char cli_arg9[] = "--output-root";
    char cli_arg10[] = "/tmp/drawing_program_texture_export_suite/output";
    char cli_arg11[] = "--texture-scene-import";
    char cli_arg12[] = "/tmp/drawing_program_texture_export_suite/scene_fixture.json";
    char cli_arg13[] = "--texture-scene-object";
    char cli_arg14[] = "obj_plane";
    char cli_arg15[] = "--texture-export";
    char cli_arg16[] = "--texture-export-dir";
    char cli_arg17[] = "/tmp/drawing_program_texture_export_suite/output/cli_plane_texture_set";
    char *cli_argv[] = {
        cli_arg0,  cli_arg1,  cli_arg2,  cli_arg3,  cli_arg4,  cli_arg5,
        cli_arg6,  cli_arg7,  cli_arg8,  cli_arg9,  cli_arg10, cli_arg11,
        cli_arg12, cli_arg13, cli_arg14, cli_arg15, cli_arg16, cli_arg17, 0
    };
    json_object *manifest = 0;
    json_object *surfaces = 0;
    json_object *surface = 0;
    char resolved_export_dir[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    uint8_t png_r = 0u;
    uint8_t png_g = 0u;
    uint8_t png_b = 0u;
    uint8_t png_a = 0u;

    if (!texture_export_expect_material_intent_contract()) {
        return 1;
    }

    (void)mkdir(suite_root, 0775);
    if (!texture_export_prepare_dir(runtime_root) ||
        !texture_export_prepare_dir(input_root) ||
        !texture_export_prepare_dir(output_root)) {
        return 1;
    }
    (void)unlink(scene_path);
    (void)unlink(project_pack_path);
    (void)unlink(legacy_v5_pack_path);
    (void)unlink(direct_front_png);
    (void)unlink(direct_back_png);
    (void)unlink(direct_front_overlay_png);
    (void)unlink(direct_top_png);
    (void)unlink(direct_manifest_path);
    (void)unlink(hidden_overlay_manifest_path);
    (void)unlink(reload_front_png);
    (void)unlink(reload_front_overlay_png);
    (void)unlink(reload_back_png);
    (void)unlink(cli_front_png);
    (void)unlink(cli_manifest_path);
    (void)unlink(legacy_front_png);
    (void)unlink(legacy_manifest_path);
    (void)rmdir(direct_export_dir);
    (void)rmdir(hidden_overlay_export_dir);
    (void)rmdir(reload_export_dir);
    (void)rmdir(cli_export_dir);
    (void)rmdir(legacy_export_dir);
    if (!write_scene_fixture(scene_path)) {
        fprintf(stderr, "lifecycle_test: failed to write texture export scene fixture\n");
        return 1;
    }

    if (!expect_ok(drawing_program_app_bootstrap(&direct_ctx, 11, direct_argv), "texture_export_direct_bootstrap")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&direct_ctx), "texture_export_direct_config")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&direct_ctx), "texture_export_direct_state_seed")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&direct_ctx), "texture_export_direct_subsystems")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&direct_ctx), "texture_export_direct_runtime_start")) {
        return 1;
    }
    if (!expect_ok(drawing_program_texture_project_session_import_scene_object(
                       &direct_ctx,
                       scene_path,
                       "obj_prism",
                       DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_HIGH),
                   "texture_export_import_prism")) {
        return 1;
    }
    direct_ctx.texture_project.export_intent_kind =
        DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_BASE_PLUS_OVERLAY;
    direct_ctx.texture_project.overlay_material_intent_kind =
        DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_GRIME;
    if (!expect_ok(drawing_program_document_sample_write(&direct_ctx.document,
                                                         0u,
                                                         0u,
                                                         drawing_program_color_value_from_rgba(255u, 0u, 0u, 255u),
                                                         0),
                   "texture_export_front_document_sample_write")) {
        return 1;
    }
    if (!expect_ok(drawing_program_layer_raster_store_sample_write(&direct_ctx.layer_rasters,
                                                                   direct_ctx.document.layers[0].layer_id,
                                                                   0u,
                                                                   0u,
                                                                   drawing_program_color_value_from_rgba(
                                                                       255u, 0u, 0u, 255u),
                                                                   0),
                   "texture_export_front_layer_sample_write")) {
        return 1;
    }
    drawing_program_visual_layer_opacity_set(&direct_ctx, direct_ctx.document.layers[0].layer_id, 35u);
    if (!expect_ok(drawing_program_texture_project_session_select_surface(&direct_ctx, 1u),
                   "texture_export_select_back_surface")) {
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_write(&direct_ctx.document,
                                                         0u,
                                                         0u,
                                                         drawing_program_color_value_from_rgba(0u, 255u, 0u, 255u),
                                                         0),
                   "texture_export_back_document_sample_write")) {
        return 1;
    }
    if (!expect_ok(drawing_program_layer_raster_store_sample_write(&direct_ctx.layer_rasters,
                                                                   direct_ctx.document.layers[0].layer_id,
                                                                   0u,
                                                                   0u,
                                                                   drawing_program_color_value_from_rgba(
                                                                       0u, 255u, 0u, 255u),
                                                                   0),
                   "texture_export_back_layer_sample_write")) {
        return 1;
    }
    drawing_program_visual_layer_opacity_set(&direct_ctx, direct_ctx.document.layers[0].layer_id, 80u);
    if (!expect_ok(drawing_program_texture_project_session_select_surface(&direct_ctx, 0u),
                   "texture_export_reselect_front_surface")) {
        return 1;
    }
    drawing_program_visual_apply_workflow_control_if_valid(&direct_ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_ADD_LAYER);
    if (direct_ctx.document.layer_count < 2u || direct_ctx.editor.active_layer_id == 0u) {
        fprintf(stderr, "lifecycle_test: expected added overlay layer on front surface\n");
        return 1;
    }
    drawing_program_visual_apply_layer_role_preset_active(
        &direct_ctx, DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_GRIME);
    {
        uint32_t overlay_layer_id = direct_ctx.editor.active_layer_id;
        uint32_t overlay_layer_index = 0u;
        if (!expect_ok(drawing_program_document_layer_index_for_id(&direct_ctx.document,
                                                                   overlay_layer_id,
                                                                   &overlay_layer_index),
                       "texture_export_overlay_layer_index")) {
            return 1;
        }
        snprintf(direct_ctx.document.layers[overlay_layer_index].name,
                 sizeof(direct_ctx.document.layers[overlay_layer_index].name),
                 "Surface Accent");
        if (drawing_program_visual_layer_role_detect_for_layer_id(&direct_ctx, overlay_layer_id) !=
            DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_GRIME) {
            fprintf(stderr,
                    "lifecycle_test: expected renamed overlay layer to retain grime role kind\n");
            return 1;
        }
        if (!expect_ok(drawing_program_layer_raster_store_sample_write(&direct_ctx.layer_rasters,
                                                                       overlay_layer_id,
                                                                       0u,
                                                                       0u,
                                                                       drawing_program_color_value_from_rgba(
                                                                           0u, 0u, 255u, 255u),
                                                                       0),
                       "texture_export_overlay_layer_sample_write")) {
            return 1;
        }
        drawing_program_visual_layer_opacity_set(&direct_ctx, overlay_layer_id, 60u);
        drawing_program_texture_project_set_surface_layer_material_intent(
            &direct_ctx.texture_project,
            direct_ctx.texture_project.active_surface_index,
            direct_ctx.document.layers[0].layer_id,
            DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_CONCRETE);
        drawing_program_texture_project_set_surface_layer_material_intent(
            &direct_ctx.texture_project,
            direct_ctx.texture_project.active_surface_index,
            overlay_layer_id,
            DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_OIL);
    }
    if (!expect_ok(drawing_program_texture_project_session_move_surface(&direct_ctx,
                                                                        direct_ctx.texture_project.active_surface_index,
                                                                        18.0f,
                                                                        -9.0f),
                   "texture_export_move_prism_surface")) {
        return 1;
    }
    if (!expect_ok(drawing_program_texture_project_session_commit_active_surface(&direct_ctx),
                   "texture_export_commit_front_surface_before_export")) {
        return 1;
    }
    {
        uint8_t committed_roles[DRAWING_PROGRAM_MAX_LAYERS];
        memset(committed_roles, 0, sizeof(committed_roles));
        drawing_program_texture_project_collect_surface_layer_role_by_index(&direct_ctx.texture_project,
                                                                            direct_ctx.texture_project.active_surface_index,
                                                                            committed_roles,
                                                                            direct_ctx.document.layer_count);
        if (direct_ctx.document.layer_count < 2u ||
            committed_roles[0] != DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_BASE ||
            committed_roles[1] != DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_GRIME) {
            fprintf(stderr,
                    "lifecycle_test: expected committed front roles base+grime got layer_count=%u roles=%u,%u\n",
                    (unsigned)direct_ctx.document.layer_count,
                    (unsigned)committed_roles[0],
                    (unsigned)committed_roles[1]);
            return 1;
        }
    }
    if (!expect_ok(drawing_program_texture_export_default_directory(&direct_ctx,
                                                                    resolved_export_dir,
                                                                    sizeof(resolved_export_dir)),
                   "texture_export_default_dir")) {
        return 1;
    }
    if (strcmp(resolved_export_dir, direct_export_dir) != 0) {
        fprintf(stderr,
                "lifecycle_test: expected texture export dir %s got %s\n",
                direct_export_dir,
                resolved_export_dir);
        return 1;
    }
    if (!expect_ok(drawing_program_texture_export_current_project(&direct_ctx, resolved_export_dir),
                   "texture_export_current_project")) {
        return 1;
    }
    if (direct_ctx.texture_project.export_binding_kind != DRAWING_PROGRAM_TEXTURE_BINDING_KIND_SEPARATE_FACES) {
        fprintf(stderr,
                "lifecycle_test: expected texture export binding kind separate faces got=%u\n",
                (unsigned)direct_ctx.texture_project.export_binding_kind);
        return 1;
    }
    if (!expect_ok(drawing_program_snapshot_save(&direct_ctx, project_pack_path), "texture_export_save_project_pack")) {
        return 1;
    }
    if (!write_texture_project_snapshot_with_v5_root(project_pack_path, legacy_v5_pack_path)) {
        fprintf(stderr, "lifecycle_test: failed to downgrade texture export pack to legacy v5 root\n");
        return 1;
    }
    if (!texture_export_expect_png_signature(direct_front_png) ||
        !texture_export_expect_png_signature(direct_back_png) ||
        !texture_export_expect_png_signature(direct_front_overlay_png) ||
        !texture_export_expect_png_signature(direct_top_png)) {
        return 1;
    }
    if (!texture_export_read_png_first_pixel_rgba(direct_front_png, &png_r, &png_g, &png_b, &png_a) ||
        png_r != 255u || png_g != 0u || png_b != 0u || png_a < 86u || png_a > 92u) {
        fprintf(stderr,
                "lifecycle_test: expected front export pixel rgba≈255,0,0,89 got=%u,%u,%u,%u\n",
                (unsigned)png_r,
                (unsigned)png_g,
                (unsigned)png_b,
                (unsigned)png_a);
        return 1;
    }
    if (!texture_export_read_png_first_pixel_rgba(direct_front_overlay_png, &png_r, &png_g, &png_b, &png_a) ||
        png_r != 0u || png_g != 0u || png_b != 255u || png_a < 151u || png_a > 155u) {
        fprintf(stderr,
                "lifecycle_test: expected front overlay export pixel rgba≈0,0,255,153 got=%u,%u,%u,%u\n",
                (unsigned)png_r,
                (unsigned)png_g,
                (unsigned)png_b,
                (unsigned)png_a);
        return 1;
    }
    if (!texture_export_read_png_first_pixel_rgba(direct_back_png, &png_r, &png_g, &png_b, &png_a) ||
        png_r != 0u || png_g != 255u || png_b != 0u || png_a < 202u || png_a > 206u) {
        fprintf(stderr,
                "lifecycle_test: expected back export pixel rgba≈0,255,0,204 got=%u,%u,%u,%u\n",
                (unsigned)png_r,
                (unsigned)png_g,
                (unsigned)png_b,
                (unsigned)png_a);
        return 1;
    }
    manifest = json_object_from_file(direct_manifest_path);
    if (!manifest) {
        fprintf(stderr, "lifecycle_test: expected texture export manifest at %s\n", direct_manifest_path);
        return 1;
    }
    if (!texture_export_manifest_expect_int(manifest, "schema_version", 5)) {
        json_object_put(manifest);
        return 1;
    }
    if (!texture_export_manifest_expect_string(manifest, "export_binding_kind", "SEPARATE_FACES") ||
        !texture_export_manifest_expect_string(manifest, "export_intent_kind", "BASE_PLUS_OVERLAY") ||
        !texture_export_manifest_expect_string(manifest, "emitted_output_kind", "BASE_PLUS_OVERLAY") ||
        !texture_export_manifest_expect_string(manifest, "overlay_material_intent_kind", "grime") ||
        !texture_export_manifest_expect_string(manifest, "primitive_kind", "RECT_PRISM") ||
        !texture_export_manifest_expect_string(manifest, "source_scene_id", "scene_texture_exports") ||
        !texture_export_manifest_expect_string(manifest, "source_object_id", "obj_prism")) {
        json_object_put(manifest);
        return 1;
    }
    if (!texture_export_manifest_expect_shared_contract(
            manifest,
            CORE_AUTHORED_TEXTURE_PRIMITIVE_KIND_RECT_PRISM,
            CORE_AUTHORED_TEXTURE_OUTPUT_KIND_BASE_PLUS_OVERLAY,
            1)) {
        json_object_put(manifest);
        return 1;
    }
    if (!json_object_object_get_ex(manifest, "base_surfaces", &surfaces) ||
        !surfaces ||
        json_object_get_type(surfaces) != json_type_array ||
        json_object_array_length(surfaces) != 6u) {
        fprintf(stderr, "lifecycle_test: expected six base texture export manifest surfaces for prism\n");
        json_object_put(manifest);
        return 1;
    }
    surface = json_object_array_get_idx(surfaces, 0);
    if (!texture_export_manifest_expect_string(surface, "face_role", "FRONT") ||
        !texture_export_manifest_expect_string(surface, "file_name", "obj_prism_front_base.png") ||
        !texture_export_manifest_expect_string(surface, "net_layout_kind", "PRISM_CROSS") ||
        !texture_export_manifest_expect_string(surface, "net_slot", "FRONT") ||
        !texture_export_manifest_expect_string(surface, "orientation", "R0") ||
        !texture_export_manifest_expect_string(surface, "base_material_intent_kind", "concrete") ||
        !texture_export_manifest_expect_missing_key(surface, "overlay_material_intent_kind") ||
        !texture_export_manifest_expect_array_len(surface, "corner_ids", 4u) ||
        !texture_export_manifest_expect_array_len(surface, "edge_ids", 4u) ||
        !texture_export_manifest_expect_array_len(surface, "adjacent_face_roles", 4u) ||
        !texture_export_manifest_expect_array_len(surface, "layer_material_intent_stable_ids", 1u) ||
        !texture_export_manifest_expect_string_array_item(surface, "layer_material_intent_stable_ids", 0u, "concrete") ||
        !texture_export_manifest_expect_double(surface, "layout_offset_x", 18.0, 0.01) ||
        !texture_export_manifest_expect_double(surface, "layout_offset_y", -9.0, 0.01)) {
        json_object_put(manifest);
        return 1;
    }
    surface = json_object_array_get_idx(surfaces, 4);
    if (!texture_export_manifest_expect_string(surface, "face_role", "TOP") ||
        !texture_export_manifest_expect_string(surface, "file_name", "obj_prism_top_base.png")) {
        json_object_put(manifest);
        return 1;
    }
    if (!json_object_object_get_ex(manifest, "overlay_surfaces", &surfaces) ||
        !surfaces ||
        json_object_get_type(surfaces) != json_type_array ||
        json_object_array_length(surfaces) != 6u) {
        fprintf(stderr, "lifecycle_test: expected six overlay texture export manifest surfaces for prism\n");
        json_object_put(manifest);
        return 1;
    }
    surface = json_object_array_get_idx(surfaces, 0);
    if (!texture_export_manifest_expect_string(surface, "face_role", "FRONT") ||
        !texture_export_manifest_expect_string(surface, "file_name", "obj_prism_front_overlay.png") ||
        !texture_export_manifest_expect_missing_key(surface, "base_material_intent_kind") ||
        !texture_export_manifest_expect_string(surface, "overlay_material_intent_kind", "oil") ||
        !texture_export_manifest_expect_array_len(surface, "layer_material_intent_stable_ids", 1u) ||
        !texture_export_manifest_expect_string_array_item(surface, "layer_material_intent_stable_ids", 0u, "oil")) {
        json_object_put(manifest);
        return 1;
    }
    json_object_put(manifest);

    drawing_program_visual_apply_workflow_control_if_valid(&direct_ctx,
                                                           DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY);
    if (!expect_ok(drawing_program_texture_export_current_project(&direct_ctx, hidden_overlay_export_dir),
                   "texture_export_current_project_hidden_overlay")) {
        return 1;
    }
    manifest = json_object_from_file(hidden_overlay_manifest_path);
    if (!manifest) {
        fprintf(stderr, "lifecycle_test: expected hidden overlay texture export manifest at %s\n",
                hidden_overlay_manifest_path);
        return 1;
    }
    if (!json_object_object_get_ex(manifest, "base_surfaces", &surfaces) ||
        !surfaces ||
        json_object_get_type(surfaces) != json_type_array ||
        json_object_array_length(surfaces) != 6u) {
        fprintf(stderr, "lifecycle_test: expected six hidden-overlay base surfaces for prism\n");
        json_object_put(manifest);
        return 1;
    }
    surface = json_object_array_get_idx(surfaces, 0);
    if (!texture_export_manifest_expect_string(surface, "base_material_intent_kind", "concrete") ||
        !texture_export_manifest_expect_missing_key(surface, "overlay_material_intent_kind") ||
        !texture_export_manifest_expect_array_len(surface, "layer_material_intent_stable_ids", 1u) ||
        !texture_export_manifest_expect_string_array_item(surface, "layer_material_intent_stable_ids", 0u, "concrete")) {
        json_object_put(manifest);
        return 1;
    }
    if (!json_object_object_get_ex(manifest, "overlay_surfaces", &surfaces) ||
        !surfaces ||
        json_object_get_type(surfaces) != json_type_array ||
        json_object_array_length(surfaces) != 6u) {
        fprintf(stderr, "lifecycle_test: expected six hidden-overlay overlay surfaces for prism\n");
        json_object_put(manifest);
        return 1;
    }
    surface = json_object_array_get_idx(surfaces, 0);
    if (!texture_export_manifest_expect_missing_key(surface, "base_material_intent_kind") ||
        !texture_export_manifest_expect_missing_key(surface, "overlay_material_intent_kind") ||
        !texture_export_manifest_expect_array_len(surface, "layer_material_intent_stable_ids", 0u)) {
        json_object_put(manifest);
        return 1;
    }
    json_object_put(manifest);

    if (!expect_ok(drawing_program_app_bootstrap(&reload_ctx, 11, direct_argv), "texture_export_reload_bootstrap")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&reload_ctx), "texture_export_reload_config")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&reload_ctx), "texture_export_reload_state_seed")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&reload_ctx), "texture_export_reload_subsystems")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&reload_ctx), "texture_export_reload_runtime_start")) {
        return 1;
    }
    if (!expect_ok(drawing_program_snapshot_load(&reload_ctx, project_pack_path), "texture_export_reload_project_pack")) {
        return 1;
    }
    if (reload_ctx.texture_project.export_binding_kind != DRAWING_PROGRAM_TEXTURE_BINDING_KIND_SEPARATE_FACES ||
        reload_ctx.texture_project.export_intent_kind !=
            DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_BASE_PLUS_OVERLAY ||
        reload_ctx.texture_project.overlay_material_intent_kind !=
            DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_GRIME ||
        strcmp(reload_ctx.texture_project.source_object_id, "obj_prism") != 0) {
        fprintf(stderr,
                "lifecycle_test: expected reloaded texture project export metadata binding=%u intent=%u overlay=%u object=%s\n",
                (unsigned)reload_ctx.texture_project.export_binding_kind,
                (unsigned)reload_ctx.texture_project.export_intent_kind,
                (unsigned)reload_ctx.texture_project.overlay_material_intent_kind,
                reload_ctx.texture_project.source_object_id);
        return 1;
    }
    {
        uint8_t reload_intents[DRAWING_PROGRAM_MAX_LAYERS];
        memset(reload_intents, 0, sizeof(reload_intents));
        drawing_program_texture_project_collect_surface_layer_material_intent_by_index(
            &reload_ctx.texture_project,
            0u,
            reload_intents,
            reload_ctx.document.layer_count);
        if (reload_ctx.document.layer_count < 2u ||
            reload_intents[0] != DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_CONCRETE ||
            reload_intents[1] != DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_OIL) {
            fprintf(stderr,
                    "lifecycle_test: expected reloaded front material intents concrete+oil got layer_count=%u intents=%u,%u\n",
                    (unsigned)reload_ctx.document.layer_count,
                    (unsigned)reload_intents[0],
                    (unsigned)reload_intents[1]);
            return 1;
        }
    }
    if (!expect_ok(drawing_program_texture_export_current_project(&reload_ctx, reload_export_dir),
                   "texture_export_reload_current_project")) {
        return 1;
    }
    if (!texture_export_expect_png_signature(reload_front_png) ||
        !texture_export_expect_png_signature(reload_back_png)) {
        return 1;
    }
    if (!texture_export_read_png_first_pixel_rgba(reload_front_png, &png_r, &png_g, &png_b, &png_a) ||
        png_r != 255u || png_g != 0u || png_b != 0u || png_a < 86u || png_a > 92u) {
        fprintf(stderr,
                "lifecycle_test: expected reloaded front export pixel rgba≈255,0,0,89 got=%u,%u,%u,%u\n",
                (unsigned)png_r,
                (unsigned)png_g,
                (unsigned)png_b,
                (unsigned)png_a);
        return 1;
    }
    if (!texture_export_read_png_first_pixel_rgba(reload_front_overlay_png, &png_r, &png_g, &png_b, &png_a) ||
        png_r != 0u || png_g != 0u || png_b != 255u || png_a < 151u || png_a > 155u) {
        fprintf(stderr,
                "lifecycle_test: expected reloaded front overlay export pixel rgba≈0,0,255,153 got=%u,%u,%u,%u\n",
                (unsigned)png_r,
                (unsigned)png_g,
                (unsigned)png_b,
                (unsigned)png_a);
        return 1;
    }
    if (!texture_export_read_png_first_pixel_rgba(reload_back_png, &png_r, &png_g, &png_b, &png_a) ||
        png_r != 0u || png_g != 255u || png_b != 0u || png_a < 202u || png_a > 206u) {
        fprintf(stderr,
                "lifecycle_test: expected reloaded back export pixel rgba≈0,255,0,204 got=%u,%u,%u,%u\n",
                (unsigned)png_r,
                (unsigned)png_g,
                (unsigned)png_b,
                (unsigned)png_a);
        return 1;
    }

    if (!expect_ok(drawing_program_app_bootstrap(&legacy_ctx, 11, direct_argv), "texture_export_legacy_bootstrap")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&legacy_ctx), "texture_export_legacy_config")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&legacy_ctx), "texture_export_legacy_state_seed")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&legacy_ctx), "texture_export_legacy_subsystems")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&legacy_ctx), "texture_export_legacy_runtime_start")) {
        return 1;
    }
    if (!expect_ok(drawing_program_snapshot_load(&legacy_ctx, legacy_v5_pack_path), "texture_export_legacy_project_pack")) {
        return 1;
    }
    if (legacy_ctx.texture_project.export_binding_kind != DRAWING_PROGRAM_TEXTURE_BINDING_KIND_SEPARATE_FACES ||
        legacy_ctx.texture_project.export_intent_kind !=
            DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_FLATTENED_ONLY ||
        strcmp(legacy_ctx.texture_project.source_object_id, "obj_prism") != 0) {
        fprintf(stderr,
                "lifecycle_test: expected legacy v5 texture project export metadata binding=%u intent=%u object=%s\n",
                (unsigned)legacy_ctx.texture_project.export_binding_kind,
                (unsigned)legacy_ctx.texture_project.export_intent_kind,
                legacy_ctx.texture_project.source_object_id);
        return 1;
    }
    if (!expect_ok(drawing_program_texture_export_current_project(&legacy_ctx, legacy_export_dir),
                   "texture_export_current_project_legacy_v5")) {
        return 1;
    }
    if (!texture_export_expect_png_signature(legacy_front_png)) {
        return 1;
    }
    manifest = json_object_from_file(legacy_manifest_path);
    if (!manifest) {
        fprintf(stderr, "lifecycle_test: expected legacy texture export manifest at %s\n", legacy_manifest_path);
        return 1;
    }
    if (!texture_export_manifest_expect_int(manifest, "schema_version", 5)) {
        json_object_put(manifest);
        return 1;
    }
    if (!texture_export_manifest_expect_string(manifest, "export_intent_kind", "FLATTENED_ONLY") ||
        !texture_export_manifest_expect_string(manifest, "overlay_material_intent_kind", "none") ||
        !texture_export_manifest_expect_string(manifest, "emitted_output_kind", "FLATTENED_ONLY") ||
        !texture_export_manifest_expect_string(manifest, "source_object_id", "obj_prism")) {
        json_object_put(manifest);
        return 1;
    }
    if (!texture_export_manifest_expect_shared_contract(
            manifest,
            CORE_AUTHORED_TEXTURE_PRIMITIVE_KIND_RECT_PRISM,
            CORE_AUTHORED_TEXTURE_OUTPUT_KIND_FLATTENED_ONLY,
            0)) {
        json_object_put(manifest);
        return 1;
    }
    if (!json_object_object_get_ex(manifest, "base_surfaces", &surfaces) ||
        !surfaces ||
        json_object_get_type(surfaces) != json_type_array ||
        json_object_array_length(surfaces) != 6u) {
        fprintf(stderr, "lifecycle_test: expected six legacy texture export manifest base surfaces for prism\n");
        json_object_put(manifest);
        return 1;
    }
    surface = json_object_array_get_idx(surfaces, 0);
    if (!texture_export_manifest_expect_string(surface, "base_material_intent_kind", "solid") ||
        !texture_export_manifest_expect_missing_key(surface, "overlay_material_intent_kind") ||
        !texture_export_manifest_expect_array_len(surface, "layer_material_intent_stable_ids", 2u) ||
        !texture_export_manifest_expect_string_array_item(surface, "layer_material_intent_stable_ids", 0u, "solid") ||
        !texture_export_manifest_expect_string_array_item(surface, "layer_material_intent_stable_ids", 1u, "none")) {
        json_object_put(manifest);
        return 1;
    }
    json_object_put(manifest);

    if (!expect_ok(drawing_program_app_bootstrap(&cli_ctx, 18, cli_argv), "texture_export_cli_bootstrap")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&cli_ctx), "texture_export_cli_config")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&cli_ctx), "texture_export_cli_state_seed")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&cli_ctx), "texture_export_cli_subsystems")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&cli_ctx), "texture_export_cli_runtime_start")) {
        return 1;
    }
    if (!texture_export_expect_png_signature(cli_front_png)) {
        return 1;
    }
    manifest = json_object_from_file(cli_manifest_path);
    if (!manifest) {
        fprintf(stderr, "lifecycle_test: expected CLI texture export manifest at %s\n", cli_manifest_path);
        return 1;
    }
    if (!texture_export_manifest_expect_int(manifest, "schema_version", 5)) {
        json_object_put(manifest);
        return 1;
    }
    if (!texture_export_manifest_expect_string(manifest, "export_intent_kind", "FLATTENED_ONLY") ||
        !texture_export_manifest_expect_string(manifest, "overlay_material_intent_kind", "none") ||
        !texture_export_manifest_expect_string(manifest, "emitted_output_kind", "FLATTENED_ONLY") ||
        !texture_export_manifest_expect_string(manifest, "primitive_kind", "PLANE") ||
        !texture_export_manifest_expect_string(manifest, "source_object_id", "obj_plane")) {
        json_object_put(manifest);
        return 1;
    }
    if (!texture_export_manifest_expect_shared_contract(
            manifest,
            CORE_AUTHORED_TEXTURE_PRIMITIVE_KIND_PLANE,
            CORE_AUTHORED_TEXTURE_OUTPUT_KIND_FLATTENED_ONLY,
            0)) {
        json_object_put(manifest);
        return 1;
    }
    if (!json_object_object_get_ex(manifest, "base_surfaces", &surfaces) ||
        !surfaces ||
        json_object_get_type(surfaces) != json_type_array ||
        json_object_array_length(surfaces) != 1u) {
        fprintf(stderr, "lifecycle_test: expected one texture export manifest base surface for plane\n");
        json_object_put(manifest);
        return 1;
    }
    surface = json_object_array_get_idx(surfaces, 0);
    if (!texture_export_manifest_expect_string(surface, "face_role", "FRONT") ||
        !texture_export_manifest_expect_string(surface, "file_name", "obj_plane_front.png") ||
        !texture_export_manifest_expect_string(surface, "net_layout_kind", "PLANE") ||
        !texture_export_manifest_expect_string(surface, "net_slot", "FRONT") ||
        !texture_export_manifest_expect_string(surface, "orientation", "R0") ||
        !texture_export_manifest_expect_string(surface, "base_material_intent_kind", "solid") ||
        !texture_export_manifest_expect_missing_key(surface, "overlay_material_intent_kind") ||
        !texture_export_manifest_expect_array_len(surface, "corner_ids", 4u) ||
        !texture_export_manifest_expect_array_len(surface, "edge_ids", 4u) ||
        !texture_export_manifest_expect_array_len(surface, "adjacent_face_roles", 4u) ||
        !texture_export_manifest_expect_array_len(surface, "layer_material_intent_stable_ids", 1u) ||
        !texture_export_manifest_expect_string_array_item(surface, "layer_material_intent_stable_ids", 0u, "solid")) {
        json_object_put(manifest);
        return 1;
    }
    json_object_put(manifest);
    return 0;
}
