#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_texture_scene_browser.h"
#include "drawing_program/drawing_program_texture_project_session.h"
#include "drawing_program_lifecycle_test_support.h"

static int write_scene_fixture(const char *path) {
    const char *json =
        "{"
        "\"schema_family\":\"codework_scene\","
        "\"schema_variant\":\"scene_authoring_v1\","
        "\"schema_version\":1,"
        "\"scene_id\":\"scene_texture_templates\","
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

int drawing_program_lifecycle_run_texture_import_suite(void) {
    static DrawingProgramAppContext direct_ctx;
    static DrawingProgramAppContext cli_ctx;
    const char *scene_path = "/tmp/drawing_program_texture_scene_import_fixture.json";
    const char *scene_root = "/tmp/drawing_program_texture_scene_browser_root";
    char arg0[] = "drawing_program_texture_import";
    char arg1[] = "--headless";
    char arg2[] = "--smoke-frames";
    char arg3[] = "1";
    char arg4[] = "--no-persist";
    char arg5[] = "--texture-scene-import";
    char arg6[] = "/tmp/drawing_program_texture_scene_import_fixture.json";
    char arg7[] = "--texture-scene-object";
    char arg8[] = "obj_prism";
    char arg9[] = "--texture-scene-quality";
    char arg10[] = "high";
    char *argv[] = { arg0, arg1, arg2, arg3, arg4, 0 };
    char *cli_argv[] = { arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, 0 };
    const DrawingProgramTextureSurface *surface = 0;
    uint8_t sample_value = 0u;
    DrawingProgramTextureSceneFileEntry scene_entries[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY];
    DrawingProgramTextureSceneObjectEntry object_entries[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY];
    uint32_t scene_entry_count = 0u;
    uint32_t object_entry_count = 0u;
    char listed_scene_id[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_ID_CAPACITY];

    (void)unlink(scene_path);
    if (!write_scene_fixture(scene_path)) {
        fprintf(stderr, "lifecycle_test: failed to write texture scene import fixture\n");
        return 1;
    }
    (void)mkdir(scene_root, 0775);
    if (!write_scene_fixture("/tmp/drawing_program_texture_scene_browser_root/fixture_scene.json")) {
        fprintf(stderr, "lifecycle_test: failed to write texture scene browser fixture\n");
        (void)unlink(scene_path);
        return 1;
    }
    if (!expect_ok(drawing_program_texture_scene_browser_list_scene_files(scene_root,
                                                                          scene_entries,
                                                                          DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY,
                                                                          &scene_entry_count),
                   "texture_scene_browser_list_scenes")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (scene_entry_count == 0u || strcmp(scene_entries[0].scene_id, "scene_texture_templates") != 0) {
        fprintf(stderr,
                "lifecycle_test: expected scene browser to list scene_texture_templates count=%u first=%s\n",
                (unsigned)scene_entry_count,
                scene_entry_count > 0u ? scene_entries[0].scene_id : "(none)");
        (void)unlink(scene_path);
        return 1;
    }
    if (!expect_ok(drawing_program_texture_scene_browser_list_supported_objects(scene_entries[0].scene_path,
                                                                                object_entries,
                                                                                DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY,
                                                                                &object_entry_count,
                                                                                listed_scene_id,
                                                                                sizeof(listed_scene_id)),
                   "texture_scene_browser_list_objects")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (strcmp(listed_scene_id, "scene_texture_templates") != 0 ||
        object_entry_count != 2u ||
        strcmp(object_entries[0].object_id, "obj_plane") != 0 ||
        strcmp(object_entries[1].object_id, "obj_prism") != 0) {
        fprintf(stderr,
                "lifecycle_test: expected scene browser objects plane/prism scene=%s count=%u first=%s second=%s\n",
                listed_scene_id,
                (unsigned)object_entry_count,
                object_entry_count > 0u ? object_entries[0].object_id : "(none)",
                object_entry_count > 1u ? object_entries[1].object_id : "(none)");
        (void)unlink(scene_path);
        return 1;
    }

    if (!expect_ok(drawing_program_app_bootstrap(&direct_ctx, 5, argv), "texture_import_direct_bootstrap")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&direct_ctx), "texture_import_direct_config")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&direct_ctx), "texture_import_direct_state_seed")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&direct_ctx), "texture_import_direct_subsystems")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&direct_ctx), "texture_import_direct_runtime_start")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (!expect_ok(drawing_program_texture_project_session_import_scene_object(
                       &direct_ctx,
                       scene_path,
                       "obj_plane",
                       DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_HIGH),
                   "texture_import_direct_plane")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (direct_ctx.texture_project.primitive_kind != DRAWING_PROGRAM_TEXTURE_PRIMITIVE_KIND_PLANE ||
        direct_ctx.texture_project.surface_count != 1u ||
        strcmp(direct_ctx.texture_project.source_scene_id, "scene_texture_templates") != 0 ||
        strcmp(direct_ctx.texture_project.source_object_id, "obj_plane") != 0 ||
        strcmp(direct_ctx.texture_project.source_scene_path, scene_path) != 0) {
        fprintf(stderr,
                "lifecycle_test: direct plane import metadata mismatch primitive=%u surfaces=%u scene=%s object=%s path=%s\n",
                (unsigned)direct_ctx.texture_project.primitive_kind,
                (unsigned)direct_ctx.texture_project.surface_count,
                direct_ctx.texture_project.source_scene_id,
                direct_ctx.texture_project.source_object_id,
                direct_ctx.texture_project.source_scene_path);
        (void)unlink(scene_path);
        return 1;
    }
    if (!direct_ctx.session.project_path ||
        strncmp(direct_ctx.session.project_path, direct_ctx.session.input_root_path, strlen(direct_ctx.session.input_root_path)) != 0 ||
        strstr(direct_ctx.session.project_path, "scene_texture_templates__obj_plane") == 0 ||
        strcmp(direct_ctx.session.selected_scene_path, scene_path) != 0 ||
        strcmp(direct_ctx.session.selected_scene_object_id, "obj_plane") != 0) {
        fprintf(stderr,
                "lifecycle_test: direct plane import target/provenance mismatch path=%s input_root=%s selected_scene=%s selected_object=%s\n",
                direct_ctx.session.project_path ? direct_ctx.session.project_path : "(null)",
                direct_ctx.session.input_root_path,
                direct_ctx.session.selected_scene_path,
                direct_ctx.session.selected_scene_object_id);
        (void)unlink(scene_path);
        return 1;
    }
    surface = drawing_program_texture_project_surface_at(&direct_ctx.texture_project, 0u);
    if (!surface ||
        surface->face_role != DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT ||
        surface->storage->document.raster_width != 1116u ||
        surface->storage->document.raster_height != 930u ||
        strcmp(surface->name, "Front") != 0 ||
        surface->semantic.layout_kind != DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_PLANE_SINGLE ||
        surface->semantic.net_slot != DRAWING_PROGRAM_TEXTURE_NET_SLOT_FRONT ||
        surface->semantic.orientation != DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R0) {
        fprintf(stderr,
                "lifecycle_test: direct plane surface mismatch role=%u size=%ux%u name=%s layout=%u slot=%u orientation=%u\n",
                surface ? (unsigned)surface->face_role : 0u,
                surface ? (unsigned)surface->storage->document.raster_width : 0u,
                surface ? (unsigned)surface->storage->document.raster_height : 0u,
                surface ? surface->name : "(null)",
                surface ? (unsigned)surface->semantic.layout_kind : 0u,
                surface ? (unsigned)surface->semantic.net_slot : 0u,
                surface ? (unsigned)surface->semantic.orientation : 0u);
        (void)unlink(scene_path);
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_write(&direct_ctx.document,
                                                         10u,
                                                         12u,
                                                         99u,
                                                         0),
                   "texture_import_direct_plane_sample_write")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (!expect_ok(drawing_program_texture_project_session_commit_active_surface(&direct_ctx),
                   "texture_import_direct_plane_commit")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (!expect_ok(drawing_program_document_sample_read(&direct_ctx.texture_project.surfaces[0].storage->document,
                                                        10u,
                                                        12u,
                                                        &sample_value),
                   "texture_import_direct_plane_storage_read")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (sample_value != 99u) {
        fprintf(stderr,
                "lifecycle_test: expected committed imported plane sample=99 got=%u\n",
                (unsigned)sample_value);
        (void)unlink(scene_path);
        return 1;
    }

    if (!expect_ok(drawing_program_app_bootstrap(&cli_ctx, 11, cli_argv), "texture_import_cli_bootstrap")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&cli_ctx), "texture_import_cli_config")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&cli_ctx), "texture_import_cli_state_seed")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&cli_ctx), "texture_import_cli_subsystems")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&cli_ctx), "texture_import_cli_runtime_start")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (cli_ctx.texture_project.primitive_kind != DRAWING_PROGRAM_TEXTURE_PRIMITIVE_KIND_RECT_PRISM ||
        cli_ctx.texture_project.surface_count != 6u ||
        strcmp(cli_ctx.texture_project.source_scene_id, "scene_texture_templates") != 0 ||
        strcmp(cli_ctx.texture_project.source_object_id, "obj_prism") != 0 ||
        strcmp(cli_ctx.texture_project.source_scene_path, scene_path) != 0) {
        fprintf(stderr,
                "lifecycle_test: CLI prism import metadata mismatch primitive=%u surfaces=%u scene=%s object=%s path=%s\n",
                (unsigned)cli_ctx.texture_project.primitive_kind,
                (unsigned)cli_ctx.texture_project.surface_count,
                cli_ctx.texture_project.source_scene_id,
                cli_ctx.texture_project.source_object_id,
                cli_ctx.texture_project.source_scene_path);
        (void)unlink(scene_path);
        return 1;
    }
    if (!cli_ctx.session.project_path ||
        strncmp(cli_ctx.session.project_path, cli_ctx.session.input_root_path, strlen(cli_ctx.session.input_root_path)) != 0 ||
        strstr(cli_ctx.session.project_path, "scene_texture_templates__obj_prism") == 0 ||
        strcmp(cli_ctx.session.selected_scene_path, scene_path) != 0 ||
        strcmp(cli_ctx.session.selected_scene_object_id, "obj_prism") != 0) {
        fprintf(stderr,
                "lifecycle_test: CLI prism import target/provenance mismatch path=%s input_root=%s selected_scene=%s selected_object=%s\n",
                cli_ctx.session.project_path ? cli_ctx.session.project_path : "(null)",
                cli_ctx.session.input_root_path,
                cli_ctx.session.selected_scene_path,
                cli_ctx.session.selected_scene_object_id);
        (void)unlink(scene_path);
        return 1;
    }
    surface = drawing_program_texture_project_surface_at(&cli_ctx.texture_project, 0u);
    if (!surface ||
        surface->face_role != DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT ||
        surface->storage->document.raster_width != 768u ||
        surface->storage->document.raster_height != 512u ||
        surface->semantic.layout_kind != DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_RECT_PRISM_CROSS ||
        surface->semantic.net_slot != DRAWING_PROGRAM_TEXTURE_NET_SLOT_FRONT ||
        surface->semantic.orientation != DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R0 ||
        surface->semantic.corner_ids[DRAWING_PROGRAM_TEXTURE_NET_CORNER_TOP_LEFT] !=
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_TOP_LEFT ||
        surface->semantic.corner_ids[DRAWING_PROGRAM_TEXTURE_NET_CORNER_TOP_RIGHT] !=
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_TOP_RIGHT ||
        surface->semantic.corner_ids[DRAWING_PROGRAM_TEXTURE_NET_CORNER_BOTTOM_RIGHT] !=
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_BOTTOM_RIGHT ||
        surface->semantic.corner_ids[DRAWING_PROGRAM_TEXTURE_NET_CORNER_BOTTOM_LEFT] !=
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_BOTTOM_LEFT) {
        fprintf(stderr,
                "lifecycle_test: CLI prism front surface mismatch role=%u size=%ux%u layout=%u slot=%u orientation=%u corners=%u,%u,%u,%u\n",
                surface ? (unsigned)surface->face_role : 0u,
                surface ? (unsigned)surface->storage->document.raster_width : 0u,
                surface ? (unsigned)surface->storage->document.raster_height : 0u,
                surface ? (unsigned)surface->semantic.layout_kind : 0u,
                surface ? (unsigned)surface->semantic.net_slot : 0u,
                surface ? (unsigned)surface->semantic.orientation : 0u,
                surface ? (unsigned)surface->semantic.corner_ids[0] : 0u,
                surface ? (unsigned)surface->semantic.corner_ids[1] : 0u,
                surface ? (unsigned)surface->semantic.corner_ids[2] : 0u,
                surface ? (unsigned)surface->semantic.corner_ids[3] : 0u);
        (void)unlink(scene_path);
        return 1;
    }
    surface = drawing_program_texture_project_surface_at(&cli_ctx.texture_project, 2u);
    if (!surface ||
        surface->face_role != DRAWING_PROGRAM_TEXTURE_FACE_ROLE_LEFT ||
        surface->storage->document.raster_width != 1024u ||
        surface->storage->document.raster_height != 512u ||
        surface->semantic.net_slot != DRAWING_PROGRAM_TEXTURE_NET_SLOT_LEFT ||
        surface->semantic.orientation != DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R270 ||
        surface->semantic.adjacent_face_roles[DRAWING_PROGRAM_TEXTURE_NET_EDGE_SIDE_RIGHT] !=
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT) {
        fprintf(stderr,
                "lifecycle_test: CLI prism left surface mismatch role=%u size=%ux%u slot=%u orientation=%u right_adj=%u\n",
                surface ? (unsigned)surface->face_role : 0u,
                surface ? (unsigned)surface->storage->document.raster_width : 0u,
                surface ? (unsigned)surface->storage->document.raster_height : 0u,
                surface ? (unsigned)surface->semantic.net_slot : 0u,
                surface ? (unsigned)surface->semantic.orientation : 0u,
                surface ? (unsigned)surface->semantic.adjacent_face_roles[DRAWING_PROGRAM_TEXTURE_NET_EDGE_SIDE_RIGHT]
                        : 0u);
        (void)unlink(scene_path);
        return 1;
    }
    surface = drawing_program_texture_project_surface_at(&cli_ctx.texture_project, 4u);
    if (!surface ||
        surface->face_role != DRAWING_PROGRAM_TEXTURE_FACE_ROLE_TOP ||
        surface->storage->document.raster_width != 768u ||
        surface->storage->document.raster_height != 1024u ||
        surface->semantic.net_slot != DRAWING_PROGRAM_TEXTURE_NET_SLOT_TOP ||
        surface->semantic.orientation != DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R0 ||
        surface->semantic.adjacent_face_roles[DRAWING_PROGRAM_TEXTURE_NET_EDGE_SIDE_BOTTOM] !=
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT) {
        fprintf(stderr,
                "lifecycle_test: CLI prism top surface mismatch role=%u size=%ux%u slot=%u orientation=%u bottom_adj=%u\n",
                surface ? (unsigned)surface->face_role : 0u,
                surface ? (unsigned)surface->storage->document.raster_width : 0u,
                surface ? (unsigned)surface->storage->document.raster_height : 0u,
                surface ? (unsigned)surface->semantic.net_slot : 0u,
                surface ? (unsigned)surface->semantic.orientation : 0u,
                surface ? (unsigned)surface->semantic.adjacent_face_roles[DRAWING_PROGRAM_TEXTURE_NET_EDGE_SIDE_BOTTOM]
                        : 0u);
        (void)unlink(scene_path);
        return 1;
    }

    if (!expect_ok(drawing_program_app_shutdown(&direct_ctx), "texture_import_direct_shutdown")) {
        (void)unlink(scene_path);
        return 1;
    }
    if (!expect_ok(drawing_program_app_shutdown(&cli_ctx), "texture_import_cli_shutdown")) {
        (void)unlink(scene_path);
        return 1;
    }
    (void)unlink(scene_path);
    return 0;
}
