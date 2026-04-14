#include "drawing_program/drawing_program_visual_layer_actions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drawing_program/drawing_program_runtime_orchestration.h"
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

void drawing_program_visual_apply_workflow_control_if_valid(DrawingProgramAppContext *ctx,
                                                            DrawingProgramWorkflowControl control) {
    CoreResult control_result;
    if (!ctx || control == DRAWING_PROGRAM_WORKFLOW_CONTROL_NONE) {
        return;
    }
    control_result = drawing_program_runtime_orchestration_apply_workflow_control(ctx, control);
    if (control_result.code != CORE_OK && control_result.code != CORE_ERR_NOT_FOUND) {
        fprintf(stderr, "drawing_program: workflow control failed: %s\n", control_result.message);
    }
}

void drawing_program_visual_apply_layer_rename_auto(DrawingProgramAppContext *ctx) {
    uint32_t active_index = 0u;
    if (!ctx || !active_layer_query(ctx, 0, &active_index, 0, 0) || active_index >= ctx->document.layer_count) {
        return;
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
    uint8_t *copied_samples = 0;
    const uint8_t *source_samples = 0;
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
    (void)snprintf(source_name, sizeof(source_name), "%s", ctx->document.layers[source_index].name);
    result = drawing_program_layer_raster_store_export_layer(&ctx->layer_rasters,
                                                             source_layer_id,
                                                             &source_samples,
                                                             &source_sample_count);
    if (result.code != CORE_OK ||
        !source_samples ||
        source_sample_count != ctx->document.raster_sample_count) {
        if (source_layer_id == 1u) {
            source_samples = ctx->document.raster_samples;
            source_sample_count = ctx->document.raster_sample_count;
        }
    }
    if (source_samples &&
        source_sample_count == ctx->document.raster_sample_count &&
        source_sample_count > 0u) {
        copied_samples = (uint8_t *)malloc((size_t)source_sample_count);
        if (copied_samples) {
            memcpy(copied_samples, source_samples, (size_t)source_sample_count);
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
        }
    }
    drawing_program_visual_layer_opacity_set(ctx, ctx->editor.active_layer_id, (uint8_t)source_opacity);
}
