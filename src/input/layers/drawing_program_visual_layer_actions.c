#include "drawing_program/drawing_program_visual_layer_actions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drawing_program/drawing_program_runtime_orchestration.h"
#include "drawing_program/drawing_program_texture_project_session.h"
#include "drawing_program/drawing_program_visual_layer_opacity.h"

static int active_layer_query(const DrawingProgramAppContext *ctx,
                              uint32_t *out_layer_id,
                              uint32_t *out_index,
                              uint8_t *out_visible,
                              uint8_t *out_locked) {
    CoreResult result = drawing_program_runtime_orchestration_resolve_active_layer(ctx,
                                                                                    out_layer_id,
                                                                                    out_index,
                                                                                    out_visible,
                                                                                    out_locked);
    return (result.code == CORE_OK) ? 1 : 0;
}

static int workflow_control_changes_layer_surface(DrawingProgramWorkflowControl control) {
    return control == DRAWING_PROGRAM_WORKFLOW_CONTROL_ADD_LAYER ||
           control == DRAWING_PROGRAM_WORKFLOW_CONTROL_DELETE_ACTIVE_LAYER ||
           control == DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_UP ||
           control == DRAWING_PROGRAM_WORKFLOW_CONTROL_MOVE_ACTIVE_LAYER_DOWN ||
           control == DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY;
}

void drawing_program_visual_apply_workflow_control_if_valid(DrawingProgramAppContext *ctx,
                                                            DrawingProgramWorkflowControl control) {
    CoreResult control_result;
    uint32_t layer_count_before = 0u;
    if (!ctx || control == DRAWING_PROGRAM_WORKFLOW_CONTROL_NONE) {
        return;
    }
    layer_count_before = ctx->document.layer_count;
    control_result = drawing_program_runtime_orchestration_apply_workflow_control(ctx, control);
    if (control_result.code == CORE_OK &&
        control == DRAWING_PROGRAM_WORKFLOW_CONTROL_ADD_LAYER &&
        ctx->document.layer_count > layer_count_before) {
        drawing_program_visual_apply_layer_role_suggest_for_active_if_generic(ctx);
    }
    if (control_result.code == CORE_OK && workflow_control_changes_layer_surface(control)) {
        CoreResult commit_result = drawing_program_texture_project_session_commit_active_surface(ctx);
        if (commit_result.code != CORE_OK) {
            fprintf(stderr,
                    "drawing_program: active surface commit after layer control failed: %s\n",
                    commit_result.message);
        }
    }
    if (control_result.code != CORE_OK && control_result.code != CORE_ERR_NOT_FOUND) {
        fprintf(stderr, "drawing_program: workflow control failed: %s\n", control_result.message);
    }
}

void drawing_program_visual_apply_layer_rename_auto(DrawingProgramAppContext *ctx) {
    uint32_t active_index = 0u;
    DrawingProgramVisualLayerRolePreset detected_role = DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_CUSTOM;
    if (!ctx || !active_layer_query(ctx, 0, &active_index, 0, 0) || active_index >= ctx->document.layer_count) {
        return;
    }
    detected_role =
        (DrawingProgramVisualLayerRolePreset)drawing_program_visual_layer_role_detect_active(ctx);
    if (detected_role != DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_CUSTOM) {
        drawing_program_visual_apply_layer_role_preset_active(ctx, detected_role);
        return;
    }
    if (ctx->document.layers[active_index].name[0] == '\0' ||
        drawing_program_visual_layer_role_detect_name(ctx->document.layers[active_index].name) ==
            DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_CUSTOM) {
        drawing_program_visual_apply_layer_role_suggest_for_active_if_generic(ctx);
        if (drawing_program_visual_layer_role_detect_name(ctx->document.layers[active_index].name) !=
            DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_CUSTOM) {
            return;
        }
    }
    (void)snprintf(ctx->document.layers[active_index].name,
                   sizeof(ctx->document.layers[active_index].name),
                   "Layer %u",
                   (unsigned)ctx->document.layers[active_index].layer_id);
}

void drawing_program_visual_apply_layer_duplicate_active(DrawingProgramAppContext *ctx) {
    uint32_t source_index = 0u;
    uint32_t source_layer_id = 0u;
    uint32_t source_opacity = 100u;
    uint32_t source_role_kind = DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM;
    DrawingProgramRasterSample *copied_samples = 0;
    const DrawingProgramRasterSample *source_samples = 0;
    uint32_t source_sample_count = 0u;
    char source_name[DRAWING_PROGRAM_LAYER_NAME_CAPACITY];
    CoreResult result;
    if (!ctx) {
        return;
    }
    if (!active_layer_query(ctx, &source_layer_id, &source_index, 0, 0) ||
        source_index >= ctx->document.layer_count ||
        source_layer_id == 0u) {
        return;
    }
    source_opacity = (uint32_t)drawing_program_visual_layer_opacity_get(ctx, source_layer_id);
    source_role_kind = drawing_program_visual_layer_role_detect_for_layer_id(ctx, source_layer_id);
    (void)snprintf(source_name, sizeof(source_name), "%s", ctx->document.layers[source_index].name);
    result = drawing_program_layer_raster_store_export_layer_or_legacy_base(&ctx->layer_rasters,
                                                                            &ctx->document,
                                                                            source_layer_id,
                                                                            &source_samples,
                                                                            &source_sample_count);
    if (result.code != CORE_OK ||
        !source_samples ||
        source_sample_count != ctx->document.raster_sample_count) {
        source_samples = 0;
        source_sample_count = 0u;
    }
    if (source_samples &&
        source_sample_count == ctx->document.raster_sample_count &&
        source_sample_count > 0u) {
        copied_samples =
            (DrawingProgramRasterSample *)malloc((size_t)source_sample_count * sizeof(*copied_samples));
        if (copied_samples) {
            memcpy(copied_samples,
                   source_samples,
                   (size_t)source_sample_count * sizeof(*copied_samples));
        }
    }
    drawing_program_visual_apply_workflow_control_if_valid(ctx, DRAWING_PROGRAM_WORKFLOW_CONTROL_ADD_LAYER);
    if (ctx->editor.active_layer_id == 0u || ctx->editor.active_layer_id == source_layer_id) {
        free(copied_samples);
        return;
    }
    if (copied_samples &&
        source_sample_count == ctx->document.raster_sample_count) {
        result = drawing_program_layer_raster_store_import_layer(&ctx->layer_rasters,
                                                                 ctx->editor.active_layer_id,
                                                                 copied_samples,
                                                                 source_sample_count);
        if (result.code != CORE_OK) {
            fprintf(stderr, "drawing_program: duplicate layer import failed: %s\n", result.message);
        }
    } else {
        fprintf(stderr, "drawing_program: duplicate layer source samples unavailable\n");
    }
    free(copied_samples);
    {
        uint32_t active_index = 0u;
        if (active_layer_query(ctx, 0, &active_index, 0, 0) && active_index < ctx->document.layer_count) {
            (void)snprintf(ctx->document.layers[active_index].name,
                           sizeof(ctx->document.layers[active_index].name),
                           "%s Copy",
                           source_name);
            drawing_program_texture_project_set_surface_layer_role(&ctx->texture_project,
                                                                   ctx->texture_project.active_surface_index,
                                                                   ctx->document.layers[active_index].layer_id,
                                                                   source_role_kind);
        }
    }
    drawing_program_visual_layer_opacity_set(ctx, ctx->editor.active_layer_id, (uint8_t)source_opacity);
    result = drawing_program_texture_project_session_commit_active_surface(ctx);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: duplicate layer active surface commit failed: %s\n", result.message);
    }
}
