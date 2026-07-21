#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "core_pack.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_indexed_editor.h"
#include "drawing_program/drawing_program_layer_raster.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_texture_project.h"
#include "drawing_program/drawing_program_visual_indexed_canvas.h"
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

static int indexed_assert_editor_profile_routing(void) {
    DrawingProgramIndexedTilesetProfile profile;
    DrawingProgramAppContext ctx;
    SDL_Surface *surface = 0;
    SDL_Renderer *renderer = 0;
    VisualCanvasSheetMetrics metrics = { { 0, 0, 320, 160 }, 10.0f };
    uint8_t before_preview[512];
    uint32_t command_count;
    int status = 1;
    indexed_seed_profile(&profile);
    memset(&ctx, 0, sizeof(ctx));
    if (!indexed_init_project(&ctx.texture_project, &profile) ||
        !indexed_expect_ok(drawing_program_indexed_editor_select_slot(&ctx, 4u),
                           "indexed_editor_select_slot") ||
        !indexed_expect_ok(drawing_program_indexed_editor_apply_at(
                               &ctx, DRAWING_PROGRAM_TOOL_BRUSH, 1u, 1u),
                           "indexed_editor_brush") ||
        ctx.texture_project.indexed_raster.indices[33u] != 4u ||
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

int drawing_program_lifecycle_run_indexed_tileset_suite(void) {
    if (indexed_assert_model_and_history() != 0 ||
        indexed_assert_editor_profile_routing() != 0 ||
        indexed_assert_roundtrip() != 0 ||
        indexed_assert_malformed_inputs_fail_closed() != 0 ||
        indexed_assert_density_other_than_one_rejected() != 0 ||
        indexed_assert_standard_roundtrip_stays_standard() != 0) {
        return 1;
    }
    return 0;
}
