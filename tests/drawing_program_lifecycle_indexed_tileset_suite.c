#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core_pack.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_indexed_editor.h"
#include "drawing_program/drawing_program_indexed_png.h"
#include "drawing_program/drawing_program_indexed_tileset_export.h"
#include "drawing_program/drawing_program_layer_raster.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_snapshot.h"
#include "drawing_program/drawing_program_texture_project.h"
#include "drawing_program/drawing_program_texture_workspace.h"
#include "drawing_program/drawing_program_indexed_cell_board.h"
#include "drawing_program/drawing_program_visual_indexed_canvas.h"
#include "drawing_program/drawing_program_visual_text_render.h"
#include "drawing_program/drawing_program_viewport.h"
#include "drawing_program_lifecycle_indexed_tileset_suite.h"
#include "drawing_program_lifecycle_test_support.h"
#include "../src/io/session/drawing_program_snapshot_internal.h"
#include "../src/io/session/drawing_program_texture_project_snapshot_internal.h"

enum {
    INDEXED_TEST_PROFILE_CHUNK_VERSION_V1 = 1u,
    INDEXED_TEST_LAYER_CHUNK_VERSION_V1 = 1u,
    INDEXED_TEST_SLOT_COUNT = 9u
};

typedef struct IndexedTestProfileChunkV1 {
    uint32_t version;
    DrawingProgramIndexedTilesetProfile profile;
} IndexedTestProfileChunkV1;

typedef struct IndexedTestLayerChunkHeaderV1 {
    uint32_t version;
    uint32_t width;
    uint32_t height;
    uint32_t index_count;
    uint32_t slot_count;
} IndexedTestLayerChunkHeaderV1;

static int indexed_expect_ok(CoreResult result, const char *label) {
    if (result.code != CORE_OK) {
        fprintf(stderr,
                "lifecycle_test: %s failed: %s\n",
                label,
                result.message ? result.message : "(null)");
        return 0;
    }
    return 1;
}

static int indexed_expect_error(CoreResult result, const char *label) {
    if (result.code == CORE_OK) {
        fprintf(stderr, "lifecycle_test: %s unexpectedly succeeded\n", label);
        return 0;
    }
    return 1;
}

static void indexed_seed_profile(DrawingProgramIndexedTilesetProfile *profile) {
    static const char *const ids[INDEXED_TEST_SLOT_COUNT] = {
        "transparent",
        "void",
        "deep_shadow",
        "mortar",
        "stone_low",
        "stone_mid",
        "stone_high",
        "edge_highlight",
        "surface_accent"
    };
    uint32_t i;
    drawing_program_indexed_tileset_profile_clear(profile);
    profile->contract_revision = CORE_AUTHORED_TEXTURE_INDEXED_CONTRACT_REVISION_V1;
    profile->atlas_width = 32u;
    profile->atlas_height = 16u;
    profile->logical_cell_width = 16u;
    profile->logical_cell_height = 16u;
    profile->slot_count = INDEXED_TEST_SLOT_COUNT;
    profile->transparent_slot_index = 0u;
    (void)snprintf(profile->tileset_id, sizeof(profile->tileset_id), "%s", "cobble_master_v1");
    for (i = 0u; i < INDEXED_TEST_SLOT_COUNT; ++i) {
        (void)snprintf(profile->slots[i].id, sizeof(profile->slots[i].id), "%s", ids[i]);
        profile->slots[i].source_rgba.r = (uint8_t)(i * 17u);
        profile->slots[i].source_rgba.g = (uint8_t)(i * 13u + 1u);
        profile->slots[i].source_rgba.b = (uint8_t)(i * 11u + 2u);
        profile->slots[i].source_rgba.a = i == 0u ? 0u : 255u;
        profile->slots[i].preview_rgba.r = (uint8_t)(255u - i * 13u);
        profile->slots[i].preview_rgba.g = (uint8_t)(i * 19u);
        profile->slots[i].preview_rgba.b = (uint8_t)(i * 7u + 3u);
        profile->slots[i].preview_rgba.a = i == 0u ? 0u : 255u;
    }
}

static int indexed_init_project(DrawingProgramTextureProject *project,
                                const DrawingProgramIndexedTilesetProfile *profile) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    CoreResult result;
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(project, 0, sizeof(*project));
    result = drawing_program_document_init_with_shape(&document,
                                                      profile->atlas_width,
                                                      profile->atlas_height,
                                                      1u);
    if (!indexed_expect_ok(result, "indexed_document_init")) {
        return 0;
    }
    result = drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document);
    if (!indexed_expect_ok(result, "indexed_layer_store_init")) {
        return 0;
    }
    result = drawing_program_texture_project_init_single_surface(
        project,
        &document,
        &layer_rasters,
        "Indexed Atlas",
        DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD);
    if (!indexed_expect_ok(result, "indexed_project_init")) {
        status = 0;
        goto cleanup;
    }
    result = drawing_program_texture_project_enable_indexed_atlas(project, profile, 0u);
    if (!indexed_expect_ok(result, "indexed_project_enable")) {
        drawing_program_texture_project_dispose(project);
        status = 0;
    }
cleanup:
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status;
}

static int indexed_seed_two_cells(DrawingProgramTextureProject *project) {
    return indexed_expect_ok(drawing_program_indexed_cell_create(
                                 &project->indexed_cells, &project->indexed_cell_history,
                                 "fixture.left", 0u, 0u, 16u, 16u),
                             "indexed_cell_create_left") &&
           indexed_expect_ok(drawing_program_indexed_cell_create(
                                 &project->indexed_cells, &project->indexed_cell_history,
                                 "fixture.right", 16u, 0u, 16u, 16u),
                             "indexed_cell_create_right");
}

static int indexed_assert_cells_and_history(void) {
    DrawingProgramIndexedTilesetProfile profile;
    DrawingProgramTextureProject project;
    int status = 1;
    indexed_seed_profile(&profile);
    if (!indexed_init_project(&project, &profile) || !indexed_seed_two_cells(&project) ||
        !indexed_expect_ok(drawing_program_indexed_cell_rename(
                               &project.indexed_cells, &project.indexed_cell_history,
                               1u, "fixture.renamed"),
                           "indexed_cell_rename") ||
        !indexed_expect_ok(drawing_program_indexed_cell_set_rect(
                               &project.indexed_cells, &project.indexed_cell_history,
                               1u, 16u, 0u, 16u, 16u),
                           "indexed_cell_rect") ||
        !indexed_expect_ok(drawing_program_indexed_cell_reorder(
                               &project.indexed_cells, &project.indexed_cell_history, 1u, 0u),
                           "indexed_cell_reorder") ||
        strcmp(project.indexed_cells.cells[0].id, "fixture.renamed") != 0 ||
        !drawing_program_indexed_cell_table_find(&project.indexed_cells, "fixture.left") ||
        !indexed_expect_ok(drawing_program_indexed_cell_history_undo(
                               &project.indexed_cell_history, &project.indexed_cells),
                           "indexed_cell_undo") ||
        strcmp(project.indexed_cells.cells[1].id, "fixture.renamed") != 0 ||
        !indexed_expect_ok(drawing_program_indexed_cell_history_redo(
                               &project.indexed_cell_history, &project.indexed_cells),
                           "indexed_cell_redo") ||
        drawing_program_indexed_cell_table_validate(
            &project.indexed_cells, 32u, 16u, 16u, 16u).code != CORE_OK) {
        status = 0;
    }
    /* Mutation APIs record first; project validation is the atomic policy gate. */
    if (!indexed_expect_ok(drawing_program_indexed_cell_create(
                               &project.indexed_cells, &project.indexed_cell_history,
                               "fixture.overlap", 0u, 0u, 16u, 16u),
                           "indexed_cell_overlapping_create") ||
        drawing_program_indexed_cell_table_validate(
            &project.indexed_cells, 32u, 16u, 16u, 16u).code == CORE_OK ||
        !indexed_expect_ok(drawing_program_indexed_cell_history_undo(
                               &project.indexed_cell_history, &project.indexed_cells),
                           "indexed_cell_overlapping_undo")) {
        status = 0;
    }
    drawing_program_texture_project_dispose(&project);
    return status ? 0 : 1;
}

static int indexed_assert_texture_only_project_opens_in_app_shell(void) {
    const char *pack_path = "/tmp/drawing_program_indexed_texture_only_open.pack";
    DrawingProgramIndexedTilesetProfile profile;
    DrawingProgramTextureProject project;
    DrawingProgramAppContext load_ctx;
    CorePackWriter writer;
    char arg0[] = "drawing_program_test";
    char arg1[] = "--headless";
    char arg2[] = "--no-persist";
    char arg3[] = "--runtime-root";
    char arg4[] = "/tmp/drawing_program_indexed_texture_only_open_runtime";
    char *argv[] = { arg0, arg1, arg2, arg3, arg4, 0 };
    int writer_open = 0;
    int status = 1;
    indexed_seed_profile(&profile);
    memset(&project, 0, sizeof(project));
    memset(&load_ctx, 0, sizeof(load_ctx));
    memset(&writer, 0, sizeof(writer));
    (void)unlink(pack_path);
    if (!indexed_init_project(&project, &profile) || !indexed_seed_two_cells(&project) ||
        !indexed_expect_ok(core_pack_writer_open(pack_path, &writer), "texture_only_pack_open")) {
        status = 0;
        goto cleanup;
    }
    writer_open = 1;
    if (!indexed_expect_ok(drawing_program_texture_project_snapshot_write(&writer, &project),
                           "texture_only_project_write") ||
        !indexed_expect_ok(core_pack_writer_close(&writer), "texture_only_pack_close")) {
        writer_open = 0;
        status = 0;
        goto cleanup;
    }
    writer_open = 0;
    if (!indexed_expect_ok(drawing_program_app_bootstrap(&load_ctx, 5, argv),
                           "texture_only_open_bootstrap") ||
        !indexed_expect_ok(drawing_program_app_config_load(&load_ctx), "texture_only_open_config") ||
        !indexed_expect_ok(drawing_program_app_state_seed(&load_ctx), "texture_only_open_state_seed") ||
        !indexed_expect_ok(drawing_program_app_subsystems_init(&load_ctx), "texture_only_open_subsystems") ||
        !indexed_expect_ok(drawing_program_runtime_start(&load_ctx), "texture_only_open_runtime") ||
        !indexed_expect_ok(drawing_program_snapshot_load(&load_ctx, pack_path), "texture_only_open_snapshot") ||
        load_ctx.texture_project.profile_kind != DRAWING_PROGRAM_TEXTURE_PROJECT_PROFILE_INDEXED_ATLAS_V1 ||
        load_ctx.document.logical_width != profile.atlas_width ||
        load_ctx.document.logical_height != profile.atlas_height ||
        load_ctx.document.sample_density != 1u ||
        load_ctx.texture_project.indexed_cells.count != 2u ||
        strcmp(load_ctx.texture_project.indexed_cells.cells[0].id, "fixture.left") != 0 ||
        strcmp(load_ctx.texture_project.indexed_cells.cells[1].id, "fixture.right") != 0 ||
        load_ctx.editor.viewport.max_zoom < 64.0f ||
        !drawing_program_texture_workspace_focus_indexed_cell(
            &load_ctx, (SDL_Rect){0, 0, 800, 600}, 1u) ||
        load_ctx.editor.viewport.zoom <= 8.0f ||
        drawing_program_viewport_clamp_zoom(80.0f) != 64.0f) {
        fprintf(stderr, "lifecycle_test: texture-only indexed pack did not open into the app shell\n");
        status = 0;
    }
cleanup:
    if (writer_open) (void)core_pack_writer_close(&writer);
    drawing_program_texture_project_dispose(&project);
    if (load_ctx.runtime.state_seeded) (void)drawing_program_app_shutdown(&load_ctx);
    (void)unlink(pack_path);
    return status ? 0 : 1;
}

static int indexed_assert_model_and_history(void) {
    DrawingProgramIndexedTilesetProfile profile;
    DrawingProgramTextureProject project;
    DrawingProgramIndexedHistoryDelta deltas[2];
    uint8_t before_invalid;
    int status = 1;
    indexed_seed_profile(&profile);
    if (!indexed_expect_ok(drawing_program_indexed_tileset_profile_validate(&profile),
                           "indexed_profile_validate") ||
        !indexed_init_project(&project, &profile)) {
        return 1;
    }
    if (!indexed_expect_ok(drawing_program_indexed_history_apply_write(
                               &project.indexed_history, &project.indexed_raster, 2u, 3u, 4u),
                           "indexed_history_write") ||
        project.indexed_raster.indices[3u * 32u + 2u] != 4u ||
        !indexed_expect_ok(drawing_program_indexed_history_undo(
                               &project.indexed_history, &project.indexed_raster),
                           "indexed_history_undo") ||
        project.indexed_raster.indices[3u * 32u + 2u] != 0u ||
        !indexed_expect_ok(drawing_program_indexed_history_redo(
                               &project.indexed_history, &project.indexed_raster),
                           "indexed_history_redo") ||
        project.indexed_raster.indices[3u * 32u + 2u] != 4u) {
        status = 0;
        goto cleanup;
    }
    memset(deltas, 0, sizeof(deltas));
    deltas[0].index_offset = 0u;
    deltas[0].previous_index = 0u;
    deltas[0].new_index = 7u;
    deltas[1].index_offset = 511u;
    deltas[1].previous_index = 0u;
    deltas[1].new_index = 8u;
    if (!indexed_expect_ok(drawing_program_indexed_history_apply_delta_block(
                               &project.indexed_history, &project.indexed_raster, deltas, 2u),
                           "indexed_history_delta_block") ||
        project.indexed_raster.indices[0] != 7u ||
        project.indexed_raster.indices[511] != 8u) {
        status = 0;
        goto cleanup;
    }
    before_invalid = project.indexed_raster.indices[0];
    if (!indexed_expect_error(drawing_program_indexed_history_apply_write(
                                  &project.indexed_history, &project.indexed_raster, 0u, 0u, 9u),
                              "indexed_out_of_range_write") ||
        project.indexed_raster.indices[0] != before_invalid) {
        status = 0;
    }
cleanup:
    drawing_program_texture_project_dispose(&project);
    return status ? 0 : 1;
}

static int indexed_assert_roundtrip(void) {
    DrawingProgramIndexedTilesetProfile profile;
    DrawingProgramTextureProject source;
    DrawingProgramTextureProject loaded;
    CorePackWriter writer;
    CorePackReader reader;
    uint8_t found = 0u;
    char path[512];
    CoreResult result;
    uint32_t i;
    int writer_open = 0;
    int reader_open = 0;
    int status = 1;
    indexed_seed_profile(&profile);
    memset(&source, 0, sizeof(source));
    memset(&loaded, 0, sizeof(loaded));
    memset(&writer, 0, sizeof(writer));
    memset(&reader, 0, sizeof(reader));
    if (!lifecycle_test_artifact_path(path, sizeof(path), "dpt1_indexed_roundtrip.pack") ||
        !indexed_init_project(&source, &profile)) {
        return 1;
    }
    for (i = 0u; i < source.indexed_raster.index_count; ++i) {
        source.indexed_raster.indices[i] = (uint8_t)(i % INDEXED_TEST_SLOT_COUNT);
    }
    if (!indexed_seed_two_cells(&source)) {
        status = 0;
        goto cleanup;
    }
    result = core_pack_writer_open(path, &writer);
    if (!indexed_expect_ok(result, "indexed_roundtrip_writer_open")) {
        status = 0;
        goto cleanup;
    }
    writer_open = 1;
    result = drawing_program_texture_project_snapshot_write(&writer, &source);
    if (!indexed_expect_ok(result, "indexed_roundtrip_write") ||
        !indexed_expect_ok(core_pack_writer_close(&writer), "indexed_roundtrip_writer_close")) {
        writer_open = 0;
        status = 0;
        goto cleanup;
    }
    writer_open = 0;
    result = core_pack_reader_open(path, &reader);
    if (!indexed_expect_ok(result, "indexed_roundtrip_reader_open")) {
        status = 0;
        goto cleanup;
    }
    reader_open = 1;
    result = drawing_program_texture_project_snapshot_load(&loaded, &reader, &found);
    if (!indexed_expect_ok(result, "indexed_roundtrip_load") || found != 1u ||
        loaded.profile_kind != DRAWING_PROGRAM_TEXTURE_PROJECT_PROFILE_INDEXED_ATLAS_V1 ||
        loaded.indexed_raster.index_count != source.indexed_raster.index_count ||
        memcmp(loaded.indexed_raster.indices,
               source.indexed_raster.indices,
               source.indexed_raster.index_count) != 0 ||
        memcmp(&loaded.indexed_profile, &source.indexed_profile, sizeof(source.indexed_profile)) != 0 ||
        memcmp(&loaded.indexed_cells, &source.indexed_cells, sizeof(source.indexed_cells)) != 0 ||
        loaded.indexed_tile_canvases.count != 2u ||
        loaded.indexed_tile_canvases.slot_count != INDEXED_TEST_SLOT_COUNT ||
        loaded.indexed_tile_canvases.canvases[0].width != 16u ||
        loaded.indexed_tile_canvases.canvases[0].height != 16u ||
        loaded.indexed_tile_canvases.canvases[0].indices[0] != source.indexed_raster.indices[0] ||
        loaded.indexed_tile_canvases.canvases[1].indices[0] != source.indexed_raster.indices[16u] ||
        loaded.indexed_history.command_count != 0u) {
        fprintf(stderr, "lifecycle_test: indexed roundtrip identity mismatch\n");
        status = 0;
        goto cleanup;
    }
    for (i = 0u; i < INDEXED_TEST_SLOT_COUNT; ++i) {
        if (strcmp(loaded.indexed_profile.slots[i].id, profile.slots[i].id) != 0) {
            fprintf(stderr, "lifecycle_test: indexed slot id changed at %u\n", (unsigned)i);
            status = 0;
            break;
        }
    }
cleanup:
    if (reader_open) {
        (void)core_pack_reader_close(&reader);
    }
    if (writer_open) {
        (void)core_pack_writer_close(&writer);
    }
    (void)unlink(path);
    drawing_program_texture_project_dispose(&loaded);
    drawing_program_texture_project_dispose(&source);
    return status ? 0 : 1;
}

static int indexed_assert_transactional_export(void) {
    DrawingProgramIndexedTilesetProfile profile;
    DrawingProgramTextureProject project;
    DrawingProgramIndexedTilesetExportOptions options = { "crypt_dark", 0u };
    DrawingProgramIndexedTilesetExportReport report;
    DrawingProgramIndexedPngImage image;
    char destination[512];
    char marker[512];
    FILE *file;
    uint32_t i;
    int status = 1;
    indexed_seed_profile(&profile);
    memset(&image, 0, sizeof(image));
    if (!lifecycle_test_artifact_path(destination, sizeof(destination), "dpt4_tileset_export") ||
        !indexed_init_project(&project, &profile) || !indexed_seed_two_cells(&project)) return 1;
    for (i = 0u; i < project.indexed_raster.index_count; ++i) {
        project.indexed_raster.indices[i] = (uint8_t)(i % INDEXED_TEST_SLOT_COUNT);
    }
    if (!indexed_expect_ok(drawing_program_indexed_tileset_export(
                               &project, destination, &options, &report),
                           "indexed_transactional_export") ||
        !indexed_expect_ok(drawing_program_indexed_png_read(report.index_png_path, &image),
                           "indexed_export_png_reopen") ||
        image.width != project.indexed_raster.width || image.height != project.indexed_raster.height ||
        memcmp(image.indices, project.indexed_raster.indices, project.indexed_raster.index_count) != 0) {
        status = 0;
        goto cleanup;
    }
    drawing_program_indexed_png_dispose(&image);
    (void)snprintf(marker, sizeof(marker), "%s/keep.marker", destination);
    file = fopen(marker, "wb");
    if (!file || fwrite("keep", 1u, 4u, file) != 4u || fclose(file) != 0) {
        if (file) fclose(file);
        status = 0;
        goto cleanup;
    }
    options.force_failure_before_commit = 1u;
    if (!indexed_expect_error(drawing_program_indexed_tileset_export(
                                  &project, destination, &options, &report),
                              "indexed_forced_export_failure")) {
        status = 0;
        goto cleanup;
    }
    file = fopen(marker, "rb");
    if (!file) status = 0;
    if (file) fclose(file);
cleanup:
    drawing_program_indexed_png_dispose(&image);
    drawing_program_texture_project_dispose(&project);
    return status ? 0 : 1;
}

static int indexed_assert_editor_profile_routing(void) {
    DrawingProgramIndexedTilesetProfile profile;
    DrawingProgramAppContext ctx;
    SDL_Surface *surface = 0;
    SDL_Renderer *renderer = 0;
    VisualCanvasSheetMetrics metrics = { { 0, 0, 320, 160 }, 10.0f };
    VisualCanvasSheetMetrics clipped_metrics = { { -100, -20, 320, 160 }, 10.0f };
    VisualCanvasSheetMetrics fractional_metrics = { { 10, 10, 132, 66 }, 4.125f };
    SDL_Rect pane_rect = { 40, 20, 80, 80 };
    uint32_t pixels[160 * 120];
    uint32_t sentinel;
    uint32_t saved_cell_count;
    int pixel_x;
    int pixel_y;
    uint8_t before_preview[512];
    uint32_t command_count;
    int status = 1;
    indexed_seed_profile(&profile);
    memset(&ctx, 0, sizeof(ctx));
    if (!indexed_init_project(&ctx.texture_project, &profile) ||
        !indexed_seed_two_cells(&ctx.texture_project) ||
        !indexed_expect_ok(drawing_program_indexed_editor_select_slot(&ctx, 4u),
                           "indexed_editor_select_slot") ||
        !indexed_expect_ok(drawing_program_indexed_editor_apply_at(
                               &ctx, DRAWING_PROGRAM_TOOL_BRUSH, 1u, 1u),
                           "indexed_editor_brush") ||
        ctx.texture_project.indexed_raster.indices[33u] != 4u ||
        ctx.texture_project.indexed_tile_canvases.count != 2u ||
        ctx.texture_project.indexed_tile_canvases.canvases[0].indices[17u] != 4u ||
        ctx.texture_project.indexed_tile_canvases.canvases[1].indices[17u] != 0u ||
        ctx.texture_project.indexed_history.commands[0].kind !=
            DRAWING_PROGRAM_INDEXED_HISTORY_COMMAND_BRUSH) {
        status = 0;
        goto cleanup;
    }
    if (!indexed_expect_ok(drawing_program_indexed_editor_select_slot(&ctx, 5u),
                           "indexed_editor_select_fill_slot") ||
        !indexed_expect_ok(drawing_program_indexed_editor_apply_at(
                               &ctx, DRAWING_PROGRAM_TOOL_FILL, 0u, 0u),
                           "indexed_editor_fill") ||
        ctx.texture_project.indexed_raster.indices[0u] != 5u ||
        ctx.texture_project.indexed_raster.indices[33u] != 4u ||
        ctx.texture_project.indexed_history.commands[1].kind !=
            DRAWING_PROGRAM_INDEXED_HISTORY_COMMAND_FILL ||
        !indexed_expect_ok(drawing_program_indexed_editor_apply_at(
                               &ctx, DRAWING_PROGRAM_TOOL_ERASER, 1u, 1u),
                           "indexed_editor_eraser") ||
        ctx.texture_project.indexed_raster.indices[33u] != 0u ||
        ctx.texture_project.indexed_history.commands[2].kind !=
            DRAWING_PROGRAM_INDEXED_HISTORY_COMMAND_ERASER) {
        status = 0;
        goto cleanup;
    }
    if (!indexed_expect_ok(drawing_program_indexed_editor_undo(&ctx), "indexed_editor_undo") ||
        ctx.texture_project.indexed_raster.indices[33u] != 4u ||
        !indexed_expect_ok(drawing_program_indexed_editor_redo(&ctx), "indexed_editor_redo") ||
        ctx.texture_project.indexed_raster.indices[33u] != 0u) {
        status = 0;
        goto cleanup;
    }
    command_count = ctx.texture_project.indexed_history.command_count;
    if (!indexed_expect_error(drawing_program_indexed_editor_apply_at(
                                  &ctx, DRAWING_PROGRAM_TOOL_LINE, 2u, 2u),
                              "indexed_editor_reject_line") ||
        !indexed_expect_error(drawing_program_runtime_orchestration_apply_workflow_control(
                                  &ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_RECT),
                              "indexed_editor_reject_rect_control") ||
        ctx.texture_project.indexed_history.command_count != command_count) {
        status = 0;
        goto cleanup;
    }
    memcpy(before_preview,
           ctx.texture_project.indexed_raster.indices,
           ctx.texture_project.indexed_raster.index_count);
    surface = SDL_CreateRGBSurfaceWithFormat(0, 320, 160, 32, SDL_PIXELFORMAT_RGBA32);
    renderer = surface ? SDL_CreateSoftwareRenderer(surface) : 0;
    if (!renderer) {
        fprintf(stderr, "lifecycle_test: failed to create indexed preview renderer\n");
        status = 0;
        goto cleanup;
    }
    drawing_program_visual_draw_indexed_canvas(
        renderer, (SDL_Rect){0, 0, 320, 160}, &ctx, &metrics, 0);
    if (memcmp(before_preview,
               ctx.texture_project.indexed_raster.indices,
               ctx.texture_project.indexed_raster.index_count) != 0) {
        fprintf(stderr, "lifecycle_test: indexed preview mutated source bytes\n");
        status = 0;
    }
    SDL_DestroyRenderer(renderer);
    renderer = 0;
    SDL_FreeSurface(surface);
    surface = SDL_CreateRGBSurfaceWithFormat(0, 160, 120, 32, SDL_PIXELFORMAT_RGBA32);
    renderer = surface ? SDL_CreateSoftwareRenderer(surface) : 0;
    if (!renderer) {
        fprintf(stderr, "lifecycle_test: failed to create clipped indexed preview renderer\n");
        status = 0;
        goto cleanup;
    }
    sentinel = SDL_MapRGBA(surface->format, 7u, 11u, 13u, 255u);
    SDL_SetRenderDrawColor(renderer, 7u, 11u, 13u, 255u);
    (void)SDL_RenderClear(renderer);
    drawing_program_visual_draw_indexed_canvas(renderer,
                                                pane_rect,
                                                &ctx,
                                                &clipped_metrics,
                                                drawing_program_visual_draw_bitmap_text);
    if (SDL_RenderReadPixels(renderer,
                             0,
                             surface->format->format,
                             pixels,
                             (int)(sizeof(uint32_t) * 160u)) != 0) {
        fprintf(stderr, "lifecycle_test: failed to read clipped indexed preview\n");
        status = 0;
        goto cleanup;
    }
    for (pixel_y = 0; pixel_y < 120; ++pixel_y) {
        for (pixel_x = 0; pixel_x < 160; ++pixel_x) {
            if ((pixel_x < pane_rect.x || pixel_x >= pane_rect.x + pane_rect.w ||
                 pixel_y < pane_rect.y || pixel_y >= pane_rect.y + pane_rect.h) &&
                pixels[pixel_y * 160 + pixel_x] != sentinel) {
                fprintf(stderr, "lifecycle_test: indexed canvas escaped its pane clip at %d,%d\n",
                        pixel_x, pixel_y);
                status = 0;
                goto cleanup;
            }
        }
    }
    saved_cell_count = ctx.texture_project.indexed_cells.count;
    ctx.texture_project.indexed_cells.count = 0u;
    memset(ctx.texture_project.indexed_raster.indices,
           5,
           ctx.texture_project.indexed_raster.index_count);
    SDL_SetRenderDrawColor(renderer, 7u, 11u, 13u, 255u);
    (void)SDL_RenderClear(renderer);
    drawing_program_visual_draw_indexed_canvas(renderer,
                                                (SDL_Rect){ 0, 0, 160, 120 },
                                                &ctx,
                                                &fractional_metrics,
                                                0);
    if (SDL_RenderReadPixels(renderer,
                             0,
                             surface->format->format,
                             pixels,
                             (int)(sizeof(uint32_t) * 160u)) != 0) {
        fprintf(stderr, "lifecycle_test: failed to read fractional indexed preview\n");
        status = 0;
        goto cleanup;
    }
    for (pixel_y = fractional_metrics.sheet_rect.y;
         pixel_y < fractional_metrics.sheet_rect.y + fractional_metrics.sheet_rect.h;
         ++pixel_y) {
        for (pixel_x = fractional_metrics.sheet_rect.x;
             pixel_x < fractional_metrics.sheet_rect.x + fractional_metrics.sheet_rect.w;
             ++pixel_x) {
            if (pixels[pixel_y * 160 + pixel_x] == sentinel) {
                fprintf(stderr, "lifecycle_test: fractional indexed raster left a pixel gap at %d,%d\n",
                        pixel_x, pixel_y);
                status = 0;
                goto cleanup;
            }
        }
    }
    ctx.texture_project.indexed_cells.count = saved_cell_count;
cleanup:
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (surface) {
        SDL_FreeSurface(surface);
    }
    drawing_program_texture_project_dispose(&ctx.texture_project);
    return status ? 0 : 1;
}

static int indexed_assert_cell_board_workspace(void) {
    DrawingProgramIndexedTilesetProfile profile;
    DrawingProgramAppContext ctx;
    DrawingProgramIndexedCellBoardCard cards[DRAWING_PROGRAM_INDEXED_CELL_CAPACITY];
    SDL_Surface *surface = 0;
    SDL_Renderer *renderer = 0;
    SDL_Rect pane_rect = { 0, 0, 360, 240 };
    SDL_Rect parent_clip = { 10, 10, 340, 220 };
    uint8_t before[512];
    uint32_t count;
    uint32_t sample_x = 0u;
    uint32_t sample_y = 0u;
    int status = 1;
    indexed_seed_profile(&profile);
    memset(&ctx, 0, sizeof(ctx));
    if (!indexed_init_project(&ctx.texture_project, &profile) ||
        !indexed_seed_two_cells(&ctx.texture_project)) {
        status = 0;
        goto cleanup;
    }
    ctx.ui.indexed_workspace_mode = (uint8_t)DRAWING_PROGRAM_INDEXED_WORKSPACE_MODE_CELL_BOARD;
    ctx.ui.indexed_selected_cell = 1u;
    memset(ctx.texture_project.indexed_raster.indices,
           5,
           ctx.texture_project.indexed_raster.index_count);
    memcpy(before,
           ctx.texture_project.indexed_raster.indices,
           ctx.texture_project.indexed_raster.index_count);
    count = drawing_program_indexed_cell_board_layout(
        &ctx, pane_rect, cards, DRAWING_PROGRAM_INDEXED_CELL_CAPACITY);
    if (count != 2u || cards[0].content_rect.w <= 0 || cards[0].content_rect.h <= 0 ||
        SDL_HasIntersection(&cards[0].frame_rect, &cards[1].frame_rect) ||
        cards[0].title_rect.y + cards[0].title_rect.h > cards[0].content_rect.y ||
        !drawing_program_indexed_cell_board_screen_to_sample(
            &ctx, pane_rect, cards[0].content_rect.x + 1, cards[0].content_rect.y + 1,
            &sample_x, &sample_y) ||
        sample_x != 0u || sample_y != 0u ||
        !drawing_program_indexed_cell_board_screen_to_sample(
            &ctx, pane_rect, cards[1].content_rect.x + 1, cards[1].content_rect.y + 1,
            &sample_x, &sample_y) ||
        sample_x != 16u || sample_y != 0u) {
        fprintf(stderr, "lifecycle_test: indexed cell board layout or sample mapping failed\n");
        status = 0;
        goto cleanup;
    }
    surface = SDL_CreateRGBSurfaceWithFormat(0, 360, 240, 32, SDL_PIXELFORMAT_RGBA32);
    renderer = surface ? SDL_CreateSoftwareRenderer(surface) : 0;
    if (!renderer) {
        fprintf(stderr, "lifecycle_test: failed to create indexed cell board renderer\n");
        status = 0;
        goto cleanup;
    }
    (void)SDL_RenderSetClipRect(renderer, &parent_clip);
    drawing_program_visual_draw_indexed_cell_board(
        renderer, pane_rect, &ctx, drawing_program_visual_draw_bitmap_text);
    if (!SDL_RenderIsClipEnabled(renderer)) {
        fprintf(stderr, "lifecycle_test: indexed cell board did not restore parent clip\n");
        status = 0;
        goto cleanup;
    }
    {
        SDL_Rect restored;
        SDL_RenderGetClipRect(renderer, &restored);
        if (restored.x != parent_clip.x || restored.y != parent_clip.y ||
            restored.w != parent_clip.w || restored.h != parent_clip.h) {
            fprintf(stderr, "lifecycle_test: indexed cell board restored wrong parent clip\n");
            status = 0;
            goto cleanup;
        }
    }
    if (memcmp(before,
               ctx.texture_project.indexed_raster.indices,
               ctx.texture_project.indexed_raster.index_count) != 0) {
        fprintf(stderr, "lifecycle_test: indexed cell board mutated source bytes\n");
        status = 0;
    }
cleanup:
    if (renderer) SDL_DestroyRenderer(renderer);
    if (surface) SDL_FreeSurface(surface);
    drawing_program_texture_project_dispose(&ctx.texture_project);
    return status ? 0 : 1;
}

static int indexed_assert_malformed_inputs_fail_closed(void) {
    DrawingProgramIndexedTilesetProfile profile;
    DrawingProgramTextureProject project;
    IndexedTestProfileChunkV1 profile_chunk;
    struct {
        IndexedTestLayerChunkHeaderV1 header;
        uint8_t indices[512];
    } layer_chunk;
    CorePackWriter writer;
    CorePackReader reader;
    char path[512];
    CoreResult result;
    int writer_open = 0;
    int reader_open = 0;
    int status = 1;
    indexed_seed_profile(&profile);
    profile.slot_count = 0u;
    if (!indexed_expect_error(drawing_program_indexed_tileset_profile_validate(&profile),
                              "indexed_zero_slot_profile")) {
        return 1;
    }
    indexed_seed_profile(&profile);
    profile.slots[0].preview_rgba.a = 255u;
    if (!indexed_expect_error(drawing_program_indexed_tileset_profile_validate(&profile),
                              "indexed_opaque_transparent_slot")) {
        return 1;
    }
    indexed_seed_profile(&profile);
    profile.contract_revision = 99u;
    if (!indexed_expect_error(drawing_program_indexed_tileset_profile_validate(&profile),
                              "indexed_unknown_profile_revision")) {
        return 1;
    }
    indexed_seed_profile(&profile);
    memset(&project, 0, sizeof(project));
    memset(&profile_chunk, 0, sizeof(profile_chunk));
    memset(&layer_chunk, 0, sizeof(layer_chunk));
    memset(&writer, 0, sizeof(writer));
    memset(&reader, 0, sizeof(reader));
    if (!lifecycle_test_artifact_path(path, sizeof(path), "dpt1_indexed_malformed.pack")) {
        return 1;
    }
    result = core_pack_writer_open(path, &writer);
    if (!indexed_expect_ok(result, "indexed_malformed_writer_open")) {
        return 1;
    }
    writer_open = 1;
    profile_chunk.version = INDEXED_TEST_PROFILE_CHUNK_VERSION_V1;
    profile_chunk.profile = profile;
    layer_chunk.header.version = INDEXED_TEST_LAYER_CHUNK_VERSION_V1;
    layer_chunk.header.width = profile.atlas_width;
    layer_chunk.header.height = profile.atlas_height;
    layer_chunk.header.index_count = profile.atlas_width * profile.atlas_height;
    layer_chunk.header.slot_count = INDEXED_TEST_SLOT_COUNT;
    layer_chunk.indices[layer_chunk.header.index_count - 1u] = INDEXED_TEST_SLOT_COUNT;
    if (!indexed_expect_ok(core_pack_writer_add_chunk(&writer,
                                                      "DPIP",
                                                      &profile_chunk,
                                                      sizeof(profile_chunk)),
                           "indexed_malformed_profile_write") ||
        !indexed_expect_ok(core_pack_writer_add_chunk(&writer,
                                                      "DPIL",
                                                      &layer_chunk,
                                                      sizeof(layer_chunk)),
                           "indexed_malformed_layer_write") ||
        !indexed_expect_ok(core_pack_writer_close(&writer), "indexed_malformed_writer_close")) {
        writer_open = 0;
        status = 0;
        goto cleanup;
    }
    writer_open = 0;
    if (!indexed_expect_ok(core_pack_reader_open(path, &reader), "indexed_malformed_reader_open")) {
        status = 0;
        goto cleanup;
    }
    reader_open = 1;
    if (!indexed_expect_error(drawing_program_indexed_project_snapshot_load(&reader, &project),
                              "indexed_out_of_range_layer_load") ||
        project.profile_kind == DRAWING_PROGRAM_TEXTURE_PROJECT_PROFILE_INDEXED_ATLAS_V1) {
        status = 0;
        goto cleanup;
    }
    (void)core_pack_reader_close(&reader);
    reader_open = 0;
    (void)unlink(path);
    if (!indexed_expect_ok(core_pack_writer_open(path, &writer), "indexed_missing_writer_open") ||
        !indexed_expect_ok(core_pack_writer_close(&writer), "indexed_missing_writer_close")) {
        writer_open = 0;
        status = 0;
        goto cleanup;
    }
    writer_open = 0;
    if (!indexed_expect_ok(core_pack_reader_open(path, &reader), "indexed_missing_reader_open")) {
        status = 0;
        goto cleanup;
    }
    reader_open = 1;
    if (!indexed_expect_error(drawing_program_indexed_project_snapshot_load(&reader, &project),
                              "indexed_missing_chunks_load")) {
        status = 0;
    }
cleanup:
    if (reader_open) {
        (void)core_pack_reader_close(&reader);
    }
    if (writer_open) {
        (void)core_pack_writer_close(&writer);
    }
    (void)unlink(path);
    drawing_program_texture_project_dispose(&project);
    return status ? 0 : 1;
}

static int indexed_assert_density_other_than_one_rejected(void) {
    DrawingProgramIndexedTilesetProfile profile;
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramTextureProject project;
    CoreResult result;
    int status = 1;
    indexed_seed_profile(&profile);
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(&project, 0, sizeof(project));
    if (!indexed_expect_ok(drawing_program_document_init_with_shape(&document, 32u, 16u, 2u),
                           "indexed_density_document_init") ||
        !indexed_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                           "indexed_density_layer_store_init") ||
        !indexed_expect_ok(drawing_program_texture_project_init_single_surface(
                               &project,
                               &document,
                               &layer_rasters,
                               "Density Two",
                               DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD),
                           "indexed_density_project_init")) {
        status = 0;
        goto cleanup;
    }
    result = drawing_program_texture_project_enable_indexed_atlas(&project, &profile, 0u);
    if (!indexed_expect_error(result, "indexed_density_enable") ||
        project.profile_kind != DRAWING_PROGRAM_TEXTURE_PROJECT_PROFILE_STANDARD_RGBA ||
        project.indexed_raster.indices != 0) {
        status = 0;
    }
cleanup:
    drawing_program_texture_project_dispose(&project);
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

static int indexed_assert_standard_roundtrip_stays_standard(void) {
    DrawingProgramDocument document;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramTextureProject source;
    DrawingProgramTextureProject loaded;
    CorePackWriter writer;
    CorePackReader reader;
    uint8_t found = 0u;
    char path[512];
    int writer_open = 0;
    int reader_open = 0;
    int status = 1;
    memset(&document, 0, sizeof(document));
    memset(&layer_rasters, 0, sizeof(layer_rasters));
    memset(&source, 0, sizeof(source));
    memset(&loaded, 0, sizeof(loaded));
    memset(&writer, 0, sizeof(writer));
    memset(&reader, 0, sizeof(reader));
    if (!lifecycle_test_artifact_path(path, sizeof(path), "dpt1_standard_roundtrip.pack") ||
        !indexed_expect_ok(drawing_program_document_init_with_shape(&document, 8u, 8u, 1u),
                           "standard_document_init") ||
        !indexed_expect_ok(drawing_program_layer_raster_store_init_from_document(&layer_rasters, &document),
                           "standard_layer_store_init") ||
        !indexed_expect_ok(drawing_program_texture_project_init_single_surface(
                               &source,
                               &document,
                               &layer_rasters,
                               "Standard",
                               DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD),
                           "standard_project_init") ||
        !indexed_expect_ok(core_pack_writer_open(path, &writer), "standard_writer_open")) {
        status = 0;
        goto cleanup;
    }
    writer_open = 1;
    if (!indexed_expect_ok(drawing_program_texture_project_snapshot_write(&writer, &source),
                           "standard_project_write") ||
        !indexed_expect_ok(core_pack_writer_close(&writer), "standard_writer_close")) {
        writer_open = 0;
        status = 0;
        goto cleanup;
    }
    writer_open = 0;
    if (!indexed_expect_ok(core_pack_reader_open(path, &reader), "standard_reader_open")) {
        status = 0;
        goto cleanup;
    }
    reader_open = 1;
    if (!indexed_expect_ok(drawing_program_texture_project_snapshot_load(&loaded, &reader, &found),
                           "standard_project_load") ||
        found != 1u ||
        loaded.profile_kind != DRAWING_PROGRAM_TEXTURE_PROJECT_PROFILE_STANDARD_RGBA ||
        loaded.indexed_raster.indices != 0) {
        fprintf(stderr, "lifecycle_test: standard project changed profile during roundtrip\n");
        status = 0;
    }
cleanup:
    if (reader_open) {
        (void)core_pack_reader_close(&reader);
    }
    if (writer_open) {
        (void)core_pack_writer_close(&writer);
    }
    (void)unlink(path);
    drawing_program_texture_project_dispose(&loaded);
    drawing_program_texture_project_dispose(&source);
    drawing_program_layer_raster_store_dispose(&layer_rasters);
    return status ? 0 : 1;
}

static int indexed_emit_canonical_dungeon_fixture_if_requested(void) {
    static const char *const keys[20] = {
        "cobble.floor.a", "cobble.floor.b", "cobble.floor.worn", "cobble.solid.interior",
        "cobble.boundary.none", "cobble.boundary.n", "cobble.boundary.e", "cobble.boundary.s",
        "cobble.boundary.w", "cobble.boundary.ne", "cobble.boundary.ns", "cobble.boundary.nw",
        "cobble.boundary.es", "cobble.boundary.ew", "cobble.boundary.sw", "cobble.boundary.nes",
        "cobble.boundary.new", "cobble.boundary.nsw", "cobble.boundary.esw", "cobble.boundary.nesw"
    };
    static const CoreAuthoredTextureRgba8 source[INDEXED_TEST_SLOT_COUNT] = {
        {0,0,0,0}, {16,16,16,255}, {32,32,32,255}, {64,64,64,255}, {96,96,96,255},
        {128,128,128,255}, {176,176,176,255}, {224,224,224,255}, {255,0,255,255}
    };
    static const CoreAuthoredTextureRgba8 preview[INDEXED_TEST_SLOT_COUNT] = {
        {0,0,0,0}, {8,11,14,255}, {18,23,29,255}, {36,43,49,255}, {53,62,69,255},
        {76,86,94,255}, {105,116,123,255}, {143,151,154,255}, {92,104,96,255}
    };
    const char *pack_path = getenv("DRAWING_PROGRAM_DPT_CANONICAL_PACK");
    const char *export_path = getenv("DRAWING_PROGRAM_DPT_CANONICAL_EXPORT");
    DrawingProgramIndexedTilesetProfile profile;
    DrawingProgramTextureProject project;
    DrawingProgramTextureProject reopened;
    DrawingProgramIndexedTilesetExportOptions options = { "crypt_dark", 0u };
    DrawingProgramIndexedTilesetExportReport report;
    CorePackWriter writer;
    CorePackReader reader;
    uint8_t found = 0u;
    uint32_t i;
    int writer_open = 0;
    int status = 1;
    if (!pack_path || pack_path[0] == '\0') return 0;
    indexed_seed_profile(&profile);
    profile.atlas_width = 80u;
    profile.atlas_height = 64u;
    for (i = 0u; i < INDEXED_TEST_SLOT_COUNT; ++i) {
        profile.slots[i].source_rgba = source[i];
        profile.slots[i].preview_rgba = preview[i];
    }
    memset(&project, 0, sizeof(project));
    memset(&reopened, 0, sizeof(reopened));
    memset(&writer, 0, sizeof(writer));
    memset(&reader, 0, sizeof(reader));
    if (!indexed_init_project(&project, &profile)) return 1;
    for (i = 0u; i < 20u; ++i) {
        if (!indexed_expect_ok(drawing_program_indexed_cell_create(
                                   &project.indexed_cells, &project.indexed_cell_history,
                                   keys[i], (i % 5u) * 16u, (i / 5u) * 16u, 16u, 16u),
                               "canonical_cell_create")) {
            status = 0;
            goto cleanup;
        }
    }
    for (i = 0u; i < project.indexed_raster.index_count; ++i) {
        uint32_t x = i % project.indexed_raster.width;
        uint32_t y = i / project.indexed_raster.width;
        uint32_t cell = (x / 16u) + (y / 16u) * 5u;
        project.indexed_raster.indices[i] = (uint8_t)(((x % 16u == 0u) || (y % 16u == 0u))
            ? 3u : (cell % 7u) + 1u);
    }
    if (!indexed_expect_ok(drawing_program_indexed_history_apply_write(
                               &project.indexed_history, &project.indexed_raster, 1u, 1u, 8u),
                           "canonical_index_mutation") ||
        !indexed_expect_ok(drawing_program_indexed_history_undo(
                               &project.indexed_history, &project.indexed_raster),
                           "canonical_index_undo") ||
        !indexed_expect_ok(drawing_program_indexed_history_redo(
                               &project.indexed_history, &project.indexed_raster),
                           "canonical_index_redo") ||
        !indexed_expect_ok(drawing_program_indexed_cell_rename(
                               &project.indexed_cells, &project.indexed_cell_history,
                               0u, "cobble.floor.fixture"),
                           "canonical_cell_mutation") ||
        !indexed_expect_ok(drawing_program_indexed_cell_history_undo(
                               &project.indexed_cell_history, &project.indexed_cells),
                           "canonical_cell_undo") ||
        !indexed_expect_ok(drawing_program_indexed_cell_history_redo(
                               &project.indexed_cell_history, &project.indexed_cells),
                           "canonical_cell_redo") ||
        !indexed_expect_ok(drawing_program_indexed_cell_history_undo(
                               &project.indexed_cell_history, &project.indexed_cells),
                           "canonical_cell_restore")) {
        status = 0;
        goto cleanup;
    }
    drawing_program_indexed_history_clear(&project.indexed_history);
    drawing_program_indexed_cell_history_clear(&project.indexed_cell_history);
    if (!indexed_expect_ok(core_pack_writer_open(pack_path, &writer), "canonical_pack_open")) {
        status = 0;
        goto cleanup;
    }
    writer_open = 1;
    if (!indexed_expect_ok(drawing_program_texture_project_snapshot_write(&writer, &project),
                           "canonical_pack_write") ||
        !indexed_expect_ok(core_pack_writer_close(&writer), "canonical_pack_close")) {
        writer_open = 0;
        status = 0;
        goto cleanup;
    }
    writer_open = 0;
    if (!indexed_expect_ok(core_pack_reader_open(pack_path, &reader), "canonical_pack_reopen") ||
        !indexed_expect_ok(drawing_program_texture_project_snapshot_load(&reopened, &reader, &found),
                           "canonical_pack_reload") || found != 1u ||
        reopened.indexed_raster.index_count != project.indexed_raster.index_count ||
        memcmp(reopened.indexed_raster.indices, project.indexed_raster.indices,
               project.indexed_raster.index_count) != 0 ||
        memcmp(&reopened.indexed_cells, &project.indexed_cells, sizeof(project.indexed_cells)) != 0 ||
        core_pack_reader_close(&reader).code != CORE_OK) {
        status = 0;
        goto cleanup;
    }
    if (export_path && export_path[0] != '\0' &&
        !indexed_expect_ok(drawing_program_indexed_tileset_export(
                               &reopened, export_path, &options, &report),
                           "canonical_resource_export")) {
        status = 0;
    }
cleanup:
    if (writer_open) (void)core_pack_writer_close(&writer);
    drawing_program_texture_project_dispose(&reopened);
    drawing_program_texture_project_dispose(&project);
    return status ? 0 : 1;
}

int drawing_program_lifecycle_run_indexed_tileset_suite(void) {
    if (indexed_assert_model_and_history() != 0 ||
        indexed_assert_cells_and_history() != 0 ||
        indexed_assert_texture_only_project_opens_in_app_shell() != 0 ||
        indexed_assert_editor_profile_routing() != 0 ||
        indexed_assert_cell_board_workspace() != 0 ||
        indexed_assert_roundtrip() != 0 ||
        indexed_assert_transactional_export() != 0 ||
        indexed_assert_malformed_inputs_fail_closed() != 0 ||
        indexed_assert_density_other_than_one_rejected() != 0 ||
        indexed_assert_standard_roundtrip_stays_standard() != 0 ||
        indexed_emit_canonical_dungeon_fixture_if_requested() != 0) {
        return 1;
    }
    return 0;
}
