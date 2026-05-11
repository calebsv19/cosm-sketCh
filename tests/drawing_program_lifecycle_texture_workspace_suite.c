#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "drawing_program/drawing_program_texture_canvas_ops.h"
#include "drawing_program/drawing_program_texture_canvas_move.h"
#include "drawing_program/drawing_program_texture_canvas_resize.h"
#include "drawing_program/drawing_program_texture_project.h"
#include "drawing_program/drawing_program_texture_project_session.h"
#include "drawing_program/drawing_program_texture_workspace.h"
#include "drawing_program/drawing_program_visual_state.h"
#include "drawing_program_lifecycle_test_support.h"
#include "drawing_program_lifecycle_texture_workspace_suite.h"

static int write_workspace_prism_scene_fixture(const char *path) {
    const char *json =
        "{"
        "\"schema_family\":\"codework_scene\","
        "\"schema_variant\":\"scene_authoring_v1\","
        "\"schema_version\":1,"
        "\"scene_id\":\"scene_workspace_semantic_layout\","
        "\"space_mode_default\":\"3d\","
        "\"objects\":["
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
                "\"origin\":{\"x\":0.0,\"y\":0.0,\"z\":0.0},"
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

static int texture_workspace_find_surface_index_by_role(const DrawingProgramTextureProject *project,
                                                        uint32_t face_role,
                                                        uint32_t *out_surface_index) {
    uint32_t i;
    if (!project || !out_surface_index) {
        return 0;
    }
    for (i = 0u; i < project->surface_count; ++i) {
        const DrawingProgramTextureSurface *surface = drawing_program_texture_project_surface_at(project, i);
        if (surface && surface->face_role == face_role) {
            *out_surface_index = i;
            return 1;
        }
    }
    return 0;
}

int drawing_program_lifecycle_run_texture_workspace_suite(DrawingProgramAppContext *ctx_ptr) {
    SDL_Rect pane_rect = {120, 80, 720, 460};
    VisualCanvasSheetMetrics base_metrics;
    VisualCanvasSheetMetrics side_metrics;
    VisualCanvasSheetMetrics active_metrics;
    VisualCanvasInteractionState interaction;
    uint32_t side_surface_index = 0u;
    uint32_t top_surface_index = 0u;
    uint32_t blank_surface_index = 0u;
    uint32_t hit_surface_index = 0u;
    uint32_t sample_x = 0u;
    uint32_t sample_y = 0u;
    const char *scene_path = "/tmp/drawing_program_texture_workspace_semantic_fixture.json";
#define ctx (*ctx_ptr)

    if (!expect_ok(drawing_program_texture_project_session_seed_blank(
                       &ctx, 128u, 96u, DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD),
                   "texture_workspace_seed_blank")) {
        return 1;
    }
    if (!expect_ok(drawing_program_texture_project_session_add_surface(&ctx,
                                                                       "Right",
                                                                       64u,
                                                                       96u,
                                                                       1u,
                                                                       DRAWING_PROGRAM_TEXTURE_FACE_ROLE_RIGHT,
                                                                       DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD,
                                                                       &side_surface_index),
                   "texture_workspace_add_right_surface")) {
        return 1;
    }
    if (!expect_ok(drawing_program_texture_project_session_add_surface(&ctx,
                                                                       "Top",
                                                                       96u,
                                                                       64u,
                                                                       1u,
                                                                       DRAWING_PROGRAM_TEXTURE_FACE_ROLE_TOP,
                                                                       DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD,
                                                                       &top_surface_index),
                   "texture_workspace_add_top_surface")) {
        return 1;
    }
    if (ctx.texture_project.surface_count != 3u) {
        fprintf(stderr, "lifecycle_test: expected texture workspace surface_count=3 got=%u\n",
                (unsigned)ctx.texture_project.surface_count);
        return 1;
    }
    if (!drawing_program_texture_workspace_fit_all(&ctx, pane_rect)) {
        fprintf(stderr, "lifecycle_test: expected workspace fit-all to succeed\n");
        return 1;
    }
    if (!drawing_program_texture_workspace_surface_sheet_metrics(&ctx, pane_rect, 0u, &base_metrics) ||
        !drawing_program_texture_workspace_surface_sheet_metrics(&ctx, pane_rect, side_surface_index, &side_metrics)) {
        fprintf(stderr, "lifecycle_test: expected workspace sheet metrics for base and side surfaces\n");
        return 1;
    }
    if (base_metrics.sheet_rect.w <= 0 || base_metrics.sheet_rect.h <= 0 || side_metrics.sheet_rect.w <= 0 ||
        side_metrics.sheet_rect.h <= 0) {
        fprintf(stderr, "lifecycle_test: expected non-empty workspace sheet rects\n");
        return 1;
    }
    if (!drawing_program_texture_workspace_hit_test_surface(&ctx,
                                                            pane_rect,
                                                            side_metrics.sheet_rect.x + (side_metrics.sheet_rect.w / 2),
                                                            side_metrics.sheet_rect.y + (side_metrics.sheet_rect.h / 2),
                                                            &hit_surface_index) ||
        hit_surface_index != side_surface_index) {
        fprintf(stderr,
                "lifecycle_test: expected hit-test to resolve side surface=%u got=%u\n",
                (unsigned)side_surface_index,
                (unsigned)hit_surface_index);
        return 1;
    }
    if (!expect_ok(drawing_program_texture_project_session_select_surface(&ctx, side_surface_index),
                   "texture_workspace_select_side_surface")) {
        return 1;
    }
    if (!drawing_program_texture_workspace_fit_surface(&ctx, pane_rect, side_surface_index)) {
        fprintf(stderr, "lifecycle_test: expected workspace fit-surface to succeed\n");
        return 1;
    }
    if (!drawing_program_texture_workspace_active_sheet_metrics(&ctx, pane_rect, &active_metrics)) {
        fprintf(stderr, "lifecycle_test: expected active sheet metrics after fit-surface\n");
        return 1;
    }
    if (ctx.document.raster_width != 64u || ctx.document.raster_height != 96u) {
        fprintf(stderr,
                "lifecycle_test: expected selected side surface document 64x96 got=%ux%u\n",
                (unsigned)ctx.document.raster_width,
                (unsigned)ctx.document.raster_height);
        return 1;
    }
    if (!drawing_program_texture_workspace_screen_to_active_sample(&ctx,
                                                                   pane_rect,
                                                                   active_metrics.sheet_rect.x + (active_metrics.sheet_rect.w / 2),
                                                                   active_metrics.sheet_rect.y + (active_metrics.sheet_rect.h / 2),
                                                                   &sample_x,
                                                                   &sample_y)) {
        fprintf(stderr, "lifecycle_test: expected active surface center to resolve to a sample\n");
        return 1;
    }
    if (sample_x >= ctx.document.raster_width || sample_y >= ctx.document.raster_height) {
        fprintf(stderr,
                "lifecycle_test: active sample out of range (%u,%u) for %ux%u surface\n",
                (unsigned)sample_x,
                (unsigned)sample_y,
                (unsigned)ctx.document.raster_width,
                (unsigned)ctx.document.raster_height);
        return 1;
    }
    if (!drawing_program_texture_workspace_screen_to_active_sample_clamped(&ctx,
                                                                           pane_rect,
                                                                           active_metrics.sheet_rect.x - 12,
                                                                           active_metrics.sheet_rect.y - 8,
                                                                           &sample_x,
                                                                           &sample_y) ||
        sample_x != 0u ||
        sample_y != 0u) {
        fprintf(stderr,
                "lifecycle_test: expected clamped sample to land at origin got=(%u,%u)\n",
                (unsigned)sample_x,
                (unsigned)sample_y);
        return 1;
    }
    if (!expect_ok(drawing_program_texture_canvas_add_blank_from_active(&ctx, &top_surface_index),
                   "texture_workspace_add_blank_from_active")) {
        return 1;
    }
    blank_surface_index = top_surface_index;
    if (ctx.texture_project.surface_count != 4u || ctx.texture_project.active_surface_index != top_surface_index) {
        fprintf(stderr,
                "lifecycle_test: expected add blank op to select new surface count=4 active=%u got count=%u active=%u\n",
                (unsigned)top_surface_index,
                (unsigned)ctx.texture_project.surface_count,
                (unsigned)ctx.texture_project.active_surface_index);
        return 1;
    }
    {
        const DrawingProgramTextureSurface *added_surface =
            drawing_program_texture_project_surface_at(&ctx.texture_project, top_surface_index);
        if (!added_surface || !added_surface->user_created || !added_surface->is_blank || added_surface->resize_locked) {
            fprintf(stderr,
                    "lifecycle_test: expected added surface to be user blank/free user=%u blank=%u lock=%u\n",
                    (unsigned)(added_surface ? added_surface->user_created : 0u),
                    (unsigned)(added_surface ? added_surface->is_blank : 0u),
                    (unsigned)(added_surface ? added_surface->resize_locked : 0u));
            return 1;
        }
    }
    memset(&interaction, 0, sizeof(interaction));
    ctx.ui.canvas_control_mode = (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT;
    if (!drawing_program_texture_workspace_surface_sheet_metrics(&ctx, pane_rect, blank_surface_index, &active_metrics)) {
        fprintf(stderr, "lifecycle_test: expected blank surface metrics before resize\n");
        return 1;
    }
    {
        SDL_Rect handle_rect;
        int drag_dx = 0;
        int drag_dy = 0;
        int handle_cx = 0;
        int handle_cy = 0;
        uint32_t target_width = 80u;
        uint32_t target_height = 112u;
        const DrawingProgramTextureSurface *resized_surface = 0;
        if (!drawing_program_texture_canvas_resize_handle_rect_for_surface(&ctx,
                                                                           pane_rect,
                                                                           blank_surface_index,
                                                                           &handle_rect)) {
            fprintf(stderr, "lifecycle_test: expected blank surface to expose resize handle\n");
            return 1;
        }
        handle_cx = handle_rect.x + (handle_rect.w / 2);
        handle_cy = handle_rect.y + (handle_rect.h / 2);
        if (!drawing_program_texture_canvas_resize_begin(&ctx,
                                                         pane_rect,
                                                         &interaction,
                                                         blank_surface_index,
                                                         handle_cx,
                                                         handle_cy)) {
            fprintf(stderr, "lifecycle_test: expected blank surface resize begin to succeed\n");
            return 1;
        }
        drag_dx = (int)((float)((int32_t)target_width - (int32_t)ctx.document.logical_width) *
                        interaction.canvas_resize_pixels_per_logical);
        drag_dy = (int)((float)((int32_t)target_height - (int32_t)ctx.document.logical_height) *
                        interaction.canvas_resize_pixels_per_logical);
        if (!drawing_program_texture_canvas_resize_update(&ctx,
                                                          &interaction,
                                                          handle_cx + drag_dx,
                                                          handle_cy + drag_dy)) {
            fprintf(stderr, "lifecycle_test: expected blank surface resize update to change dimensions\n");
            return 1;
        }
        drawing_program_texture_canvas_resize_end(&interaction);
        resized_surface = drawing_program_texture_project_surface_at(&ctx.texture_project, blank_surface_index);
        if (!resized_surface || !resized_surface->storage ||
            ctx.document.logical_width != target_width ||
            ctx.document.logical_height != target_height ||
            resized_surface->storage->document.logical_width != target_width ||
            resized_surface->storage->document.logical_height != target_height ||
            !resized_surface->is_blank ||
            resized_surface->resize_locked) {
            fprintf(stderr,
                    "lifecycle_test: expected resized blank surface %ux%u free/blank got doc=%ux%u storage=%ux%u blank=%u lock=%u\n",
                    (unsigned)target_width,
                    (unsigned)target_height,
                    (unsigned)ctx.document.logical_width,
                    (unsigned)ctx.document.logical_height,
                    (unsigned)(resized_surface ? resized_surface->storage->document.logical_width : 0u),
                    (unsigned)(resized_surface ? resized_surface->storage->document.logical_height : 0u),
                    (unsigned)(resized_surface ? resized_surface->is_blank : 0u),
                    (unsigned)(resized_surface ? resized_surface->resize_locked : 0u));
            return 1;
        }
    }
    if (!expect_ok(drawing_program_document_sample_write(&ctx.document, 3u, 4u, 88u, 0),
                   "texture_workspace_seed_duplicate_source")) {
        return 1;
    }
    if (!expect_ok(drawing_program_texture_canvas_duplicate_active(&ctx, &top_surface_index),
                   "texture_workspace_duplicate_active")) {
        return 1;
    }
    if (ctx.texture_project.surface_count != 5u || ctx.texture_project.active_surface_index != top_surface_index) {
        fprintf(stderr,
                "lifecycle_test: expected duplicate op to select new surface count=5 active=%u got count=%u active=%u\n",
                (unsigned)top_surface_index,
                (unsigned)ctx.texture_project.surface_count,
                (unsigned)ctx.texture_project.active_surface_index);
        return 1;
    }
    {
        const DrawingProgramTextureSurface *duplicated_surface =
            drawing_program_texture_project_surface_at(&ctx.texture_project, top_surface_index);
        if (!duplicated_surface || !duplicated_surface->user_created || duplicated_surface->is_blank ||
            !duplicated_surface->resize_locked) {
            fprintf(stderr,
                    "lifecycle_test: expected duplicated surface to be user painted/locked user=%u blank=%u lock=%u\n",
                    (unsigned)(duplicated_surface ? duplicated_surface->user_created : 0u),
                    (unsigned)(duplicated_surface ? duplicated_surface->is_blank : 0u),
                    (unsigned)(duplicated_surface ? duplicated_surface->resize_locked : 0u));
            return 1;
        }
        if (drawing_program_texture_canvas_resize_handle_rect_for_surface(&ctx,
                                                                          pane_rect,
                                                                          top_surface_index,
                                                                          &(SDL_Rect){ 0, 0, 0, 0 })) {
            fprintf(stderr, "lifecycle_test: expected painted duplicated surface to block resize handle\n");
            return 1;
        }
    }
    if (!expect_ok(drawing_program_texture_project_session_select_surface(&ctx, top_surface_index),
                   "texture_workspace_select_top_surface")) {
        return 1;
    }
    if (ctx.texture_project.active_surface_index != top_surface_index) {
        fprintf(stderr,
                "lifecycle_test: expected active workspace surface=%u got=%u\n",
                (unsigned)top_surface_index,
                (unsigned)ctx.texture_project.active_surface_index);
        return 1;
    }
    (void)unlink(scene_path);
    if (!write_workspace_prism_scene_fixture(scene_path)) {
        fprintf(stderr, "lifecycle_test: failed to write workspace semantic prism fixture\n");
        return 1;
    }
    if (!expect_ok(drawing_program_texture_project_session_import_scene_object(
                       &ctx,
                       scene_path,
                       "obj_prism",
                       DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_HIGH),
                   "texture_workspace_import_semantic_prism")) {
        (void)unlink(scene_path);
        return 1;
    }
    {
        VisualCanvasSheetMetrics front_semantic_metrics;
        VisualCanvasSheetMetrics back_semantic_metrics;
        VisualCanvasSheetMetrics left_semantic_metrics;
        VisualCanvasSheetMetrics right_semantic_metrics;
        VisualCanvasSheetMetrics top_semantic_metrics;
        VisualCanvasSheetMetrics bottom_semantic_metrics;
        VisualCanvasSheetMetrics extra_semantic_metrics;
        int front_center_x = 0;
        int top_center_x = 0;
        int bottom_center_x = 0;
        uint32_t front_surface_index = 0u;
        uint32_t back_surface_index = 0u;
        uint32_t left_surface_index = 0u;
        uint32_t right_surface_index = 0u;
        uint32_t semantic_top_surface_index = 0u;
        uint32_t bottom_surface_index = 0u;
        uint32_t extra_surface_index = 0u;
        if (!texture_workspace_find_surface_index_by_role(
                &ctx.texture_project, DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT, &front_surface_index) ||
            !texture_workspace_find_surface_index_by_role(
                &ctx.texture_project, DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BACK, &back_surface_index) ||
            !texture_workspace_find_surface_index_by_role(
                &ctx.texture_project, DRAWING_PROGRAM_TEXTURE_FACE_ROLE_LEFT, &left_surface_index) ||
            !texture_workspace_find_surface_index_by_role(
                &ctx.texture_project, DRAWING_PROGRAM_TEXTURE_FACE_ROLE_RIGHT, &right_surface_index) ||
            !texture_workspace_find_surface_index_by_role(
                &ctx.texture_project, DRAWING_PROGRAM_TEXTURE_FACE_ROLE_TOP, &semantic_top_surface_index) ||
            !texture_workspace_find_surface_index_by_role(
                &ctx.texture_project, DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BOTTOM, &bottom_surface_index)) {
            fprintf(stderr, "lifecycle_test: expected semantic prism import to expose all six face roles\n");
            (void)unlink(scene_path);
            return 1;
        }
        if (!drawing_program_texture_workspace_fit_all(&ctx, pane_rect) ||
            !drawing_program_texture_workspace_surface_sheet_metrics(
                &ctx, pane_rect, front_surface_index, &front_semantic_metrics) ||
            !drawing_program_texture_workspace_surface_sheet_metrics(
                &ctx, pane_rect, back_surface_index, &back_semantic_metrics) ||
            !drawing_program_texture_workspace_surface_sheet_metrics(
                &ctx, pane_rect, left_surface_index, &left_semantic_metrics) ||
            !drawing_program_texture_workspace_surface_sheet_metrics(
                &ctx, pane_rect, right_surface_index, &right_semantic_metrics) ||
            !drawing_program_texture_workspace_surface_sheet_metrics(
                &ctx, pane_rect, semantic_top_surface_index, &top_semantic_metrics) ||
            !drawing_program_texture_workspace_surface_sheet_metrics(
                &ctx, pane_rect, bottom_surface_index, &bottom_semantic_metrics)) {
            fprintf(stderr, "lifecycle_test: expected semantic prism workspace metrics after fit-all\n");
            (void)unlink(scene_path);
            return 1;
        }
        if (!(left_semantic_metrics.sheet_rect.x + left_semantic_metrics.sheet_rect.w <
                  front_semantic_metrics.sheet_rect.x &&
              front_semantic_metrics.sheet_rect.x + front_semantic_metrics.sheet_rect.w <
                  right_semantic_metrics.sheet_rect.x &&
              right_semantic_metrics.sheet_rect.x + right_semantic_metrics.sheet_rect.w <
                  back_semantic_metrics.sheet_rect.x)) {
            fprintf(stderr,
                    "lifecycle_test: expected semantic prism strip order left/front/right/back got lx=%d fx=%d rx=%d bx=%d\n",
                    left_semantic_metrics.sheet_rect.x,
                    front_semantic_metrics.sheet_rect.x,
                    right_semantic_metrics.sheet_rect.x,
                    back_semantic_metrics.sheet_rect.x);
            (void)unlink(scene_path);
            return 1;
        }
        if (abs(left_semantic_metrics.sheet_rect.y - front_semantic_metrics.sheet_rect.y) > 3 ||
            abs(right_semantic_metrics.sheet_rect.y - front_semantic_metrics.sheet_rect.y) > 3 ||
            abs(back_semantic_metrics.sheet_rect.y - front_semantic_metrics.sheet_rect.y) > 3) {
            fprintf(stderr,
                    "lifecycle_test: expected semantic prism strip faces to align on one row got ly=%d fy=%d ry=%d by=%d\n",
                    left_semantic_metrics.sheet_rect.y,
                    front_semantic_metrics.sheet_rect.y,
                    right_semantic_metrics.sheet_rect.y,
                    back_semantic_metrics.sheet_rect.y);
            (void)unlink(scene_path);
            return 1;
        }
        if (!(top_semantic_metrics.sheet_rect.y + top_semantic_metrics.sheet_rect.h <
                  front_semantic_metrics.sheet_rect.y &&
              bottom_semantic_metrics.sheet_rect.y >
                  front_semantic_metrics.sheet_rect.y + front_semantic_metrics.sheet_rect.h)) {
            fprintf(stderr,
                    "lifecycle_test: expected semantic prism top/bottom above and below front got ty=%d th=%d fy=%d by=%d\n",
                    top_semantic_metrics.sheet_rect.y,
                    top_semantic_metrics.sheet_rect.h,
                    front_semantic_metrics.sheet_rect.y,
                    bottom_semantic_metrics.sheet_rect.y);
            (void)unlink(scene_path);
            return 1;
        }
        front_center_x = front_semantic_metrics.sheet_rect.x + (front_semantic_metrics.sheet_rect.w / 2);
        top_center_x = top_semantic_metrics.sheet_rect.x + (top_semantic_metrics.sheet_rect.w / 2);
        bottom_center_x = bottom_semantic_metrics.sheet_rect.x + (bottom_semantic_metrics.sheet_rect.w / 2);
        if (abs(top_center_x - front_center_x) > 3 || abs(bottom_center_x - front_center_x) > 3) {
            fprintf(stderr,
                    "lifecycle_test: expected semantic prism top/bottom to center on front got top=%d front=%d bottom=%d\n",
                    top_center_x,
                    front_center_x,
                    bottom_center_x);
            (void)unlink(scene_path);
            return 1;
        }
        memset(&interaction, 0, sizeof(interaction));
        ctx.ui.canvas_control_mode = (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT;
        if (!drawing_program_texture_canvas_move_begin(&ctx,
                                                       pane_rect,
                                                       &interaction,
                                                       front_surface_index,
                                                       front_semantic_metrics.sheet_rect.x +
                                                           (front_semantic_metrics.sheet_rect.w / 2),
                                                       front_semantic_metrics.sheet_rect.y +
                                                           (front_semantic_metrics.sheet_rect.h / 2)) ||
            !drawing_program_texture_canvas_move_update(&ctx,
                                                        &interaction,
                                                        front_semantic_metrics.sheet_rect.x +
                                                            (front_semantic_metrics.sheet_rect.w / 2) + 120,
                                                        front_semantic_metrics.sheet_rect.y +
                                                            (front_semantic_metrics.sheet_rect.h / 2) + 40)) {
            fprintf(stderr, "lifecycle_test: expected semantic front-face layout move to succeed\n");
            (void)unlink(scene_path);
            return 1;
        }
        drawing_program_texture_canvas_move_end(&interaction);
        if (!drawing_program_texture_workspace_surface_sheet_metrics(
                &ctx, pane_rect, front_surface_index, &extra_semantic_metrics) ||
            extra_semantic_metrics.sheet_rect.x <= front_semantic_metrics.sheet_rect.x ||
            extra_semantic_metrics.sheet_rect.y <= front_semantic_metrics.sheet_rect.y) {
            fprintf(stderr,
                    "lifecycle_test: expected moved semantic front-face to shift right/down before reset old=(%d,%d) new=(%d,%d)\n",
                    front_semantic_metrics.sheet_rect.x,
                    front_semantic_metrics.sheet_rect.y,
                    extra_semantic_metrics.sheet_rect.x,
                    extra_semantic_metrics.sheet_rect.y);
            (void)unlink(scene_path);
            return 1;
        }
        if (!expect_ok(drawing_program_texture_canvas_reset_object_layout(&ctx),
                       "texture_workspace_reset_object_layout") ||
            !drawing_program_texture_workspace_fit_all(&ctx, pane_rect) ||
            !drawing_program_texture_workspace_surface_sheet_metrics(
                &ctx, pane_rect, front_surface_index, &front_semantic_metrics)) {
            fprintf(stderr, "lifecycle_test: expected reset-to-object-layout to restore semantic net\n");
            (void)unlink(scene_path);
            return 1;
        }
        {
            const DrawingProgramTextureSurface *reset_front =
                drawing_program_texture_project_surface_at(&ctx.texture_project, front_surface_index);
            if (!reset_front || reset_front->layout_offset_x != 0.0f || reset_front->layout_offset_y != 0.0f) {
                fprintf(stderr,
                        "lifecycle_test: expected reset object layout to clear front offsets got=(%.2f,%.2f)\n",
                        reset_front ? reset_front->layout_offset_x : 0.0f,
                        reset_front ? reset_front->layout_offset_y : 0.0f);
                (void)unlink(scene_path);
                return 1;
            }
        }
        if (!drawing_program_texture_workspace_hit_test_surface(
                &ctx,
                pane_rect,
                top_semantic_metrics.sheet_rect.x + (top_semantic_metrics.sheet_rect.w / 2),
                top_semantic_metrics.sheet_rect.y + (top_semantic_metrics.sheet_rect.h / 2),
                &hit_surface_index) ||
            hit_surface_index != semantic_top_surface_index) {
            fprintf(stderr,
                    "lifecycle_test: expected semantic top-face hit-test=%u got=%u\n",
                    (unsigned)semantic_top_surface_index,
                    (unsigned)hit_surface_index);
            (void)unlink(scene_path);
            return 1;
        }
        if (!expect_ok(drawing_program_texture_project_session_select_surface(&ctx, semantic_top_surface_index),
                       "texture_workspace_select_semantic_top_surface") ||
            !drawing_program_texture_workspace_fit_surface(&ctx, pane_rect, semantic_top_surface_index) ||
            !drawing_program_texture_workspace_active_sheet_metrics(&ctx, pane_rect, &active_metrics) ||
            !drawing_program_texture_workspace_screen_to_active_sample(&ctx,
                                                                       pane_rect,
                                                                       active_metrics.sheet_rect.x +
                                                                           (active_metrics.sheet_rect.w / 2),
                                                                       active_metrics.sheet_rect.y +
                                                                           (active_metrics.sheet_rect.h / 2),
                                                                       &sample_x,
                                                                       &sample_y)) {
            fprintf(stderr, "lifecycle_test: expected semantic top-face fit/sample workflow to succeed\n");
            (void)unlink(scene_path);
            return 1;
        }
        if (!expect_ok(drawing_program_texture_canvas_add_blank_from_active(&ctx, &extra_surface_index),
                       "texture_workspace_add_extra_canvas_after_semantic_import") ||
            !drawing_program_texture_workspace_fit_all(&ctx, pane_rect) ||
            !drawing_program_texture_workspace_surface_sheet_metrics(
                &ctx, pane_rect, bottom_surface_index, &bottom_semantic_metrics) ||
            !drawing_program_texture_workspace_surface_sheet_metrics(
                &ctx, pane_rect, extra_surface_index, &extra_semantic_metrics)) {
            fprintf(stderr, "lifecycle_test: expected extra generic canvas after semantic import\n");
            (void)unlink(scene_path);
            return 1;
        }
        if (!(extra_semantic_metrics.sheet_rect.y >
              bottom_semantic_metrics.sheet_rect.y + bottom_semantic_metrics.sheet_rect.h)) {
            fprintf(stderr,
                    "lifecycle_test: expected extra user canvas below semantic prism net got extra_y=%d bottom_y=%d bottom_h=%d\n",
                    extra_semantic_metrics.sheet_rect.y,
                    bottom_semantic_metrics.sheet_rect.y,
                    bottom_semantic_metrics.sheet_rect.h);
            (void)unlink(scene_path);
            return 1;
        }
    }
    (void)unlink(scene_path);
    ctx.ui.canvas_control_mode = (uint8_t)DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_PAINT;
    return 0;
}
