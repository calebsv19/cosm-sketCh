#include "drawing_program/drawing_program_snapshot.h"

#include <stdio.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_ui_color_state.h"

static CoreResult snapshot_export_invalid(const char *message) {
    return (CoreResult){ CORE_ERR_INVALID_ARG, message };
}

CoreResult drawing_program_snapshot_export_debug_json(const struct DrawingProgramAppContext *ctx,
                                                      const char *path) {
    FILE *f;
    uint32_t i;
    if (!ctx || !path) {
        return snapshot_export_invalid("invalid snapshot export request");
    }
    f = fopen(path, "wb");
    if (!f) {
        return (CoreResult){ CORE_ERR_IO, "failed to open snapshot json output" };
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"schema\": 1,\n");
    fprintf(f, "  \"document\": {\n");
    fprintf(f, "    \"schema_version\": %u,\n", ctx->document.schema_version);
    fprintf(f, "    \"logical_width\": %u,\n", ctx->document.logical_width);
    fprintf(f, "    \"logical_height\": %u,\n", ctx->document.logical_height);
    fprintf(f, "    \"sample_density\": %u,\n", ctx->document.sample_density);
    fprintf(f, "    \"layer_count\": %u,\n", ctx->document.layer_count);
    fprintf(f, "    \"raster_sample_count\": %u\n", ctx->document.raster_sample_count);
    fprintf(f, "  },\n");
    fprintf(f, "  \"layer_raster\": {\n");
    fprintf(f, "    \"slot_capacity\": %u,\n", ctx->layer_rasters.slot_capacity);
    fprintf(f, "    \"slot_count\": %u,\n", ctx->layer_rasters.slot_count);
    fprintf(f, "    \"sample_count\": %u\n", ctx->layer_rasters.sample_count);
    fprintf(f, "  },\n");
    fprintf(f, "  \"objects\": {\n");
    fprintf(f, "    \"schema_version\": %u,\n", (unsigned)ctx->object_store.schema_version);
    fprintf(f, "    \"count\": %u,\n", (unsigned)ctx->object_store.object_count);
    fprintf(f, "    \"next_object_id\": %u,\n", (unsigned)ctx->object_store.next_object_id);
    fprintf(f, "    \"items\": [\n");
    for (i = 0u; i < ctx->object_store.object_count && i < DRAWING_PROGRAM_MAX_OBJECTS; ++i) {
        const DrawingProgramObjectRecord *object = &ctx->object_store.objects[i];
        fprintf(f,
                "      {\"id\": %u, \"layer_id\": %u, \"type\": %u, \"visible\": %u, \"locked\": %u, \"x\": %d, \"y\": %d, \"w\": %u, \"h\": %u, \"path_point_count\": %u, \"path_closed\": %u}%s\n",
                (unsigned)object->object_id,
                (unsigned)object->layer_id,
                (unsigned)object->type,
                (unsigned)object->visible,
                (unsigned)object->locked,
                (int)object->origin_x,
                (int)object->origin_y,
                (unsigned)object->width,
                (unsigned)object->height,
                (unsigned)object->path_point_count,
                (unsigned)object->path_closed,
                ((i + 1u) < ctx->object_store.object_count && (i + 1u) < DRAWING_PROGRAM_MAX_OBJECTS) ? "," : "");
    }
    fprintf(f, "    ]\n");
    fprintf(f, "  },\n");
    fprintf(f, "  \"editor\": {\n");
    fprintf(f, "    \"active_tool\": %u,\n", (unsigned)ctx->editor.active_tool);
    fprintf(f, "    \"active_layer_id\": %u,\n", ctx->editor.active_layer_id);
    fprintf(f, "    \"zoom\": %.3f\n", (double)ctx->editor.viewport.zoom);
    fprintf(f, "  },\n");
    fprintf(f, "  \"ui\": {\n");
    fprintf(f, "    \"theme_preset_id\": %u,\n", ctx->ui.theme_preset_id);
    fprintf(f, "    \"font_preset_id\": %u,\n", ctx->ui.font_preset_id);
    fprintf(f, "    \"font_zoom_step\": %d,\n", (int)ctx->ui.font_zoom_step);
    fprintf(f, "    \"left_panel_slot\": %u,\n", (unsigned)ctx->ui.left_panel_slot);
    fprintf(f, "    \"right_panel_slot\": %u,\n", (unsigned)ctx->ui.right_panel_slot);
    fprintf(f, "    \"active_color_index\": %u,\n", (unsigned)ctx->ui.active_color_index);
    fprintf(f, "    \"active_paint_rgb\": [%u, %u, %u],\n",
            (unsigned)ctx->ui.active_paint_r,
            (unsigned)ctx->ui.active_paint_g,
            (unsigned)ctx->ui.active_paint_b);
    fprintf(f, "    \"active_color_sample\": %u\n",
            (unsigned)drawing_program_ui_color_active_paint_sample_value(ctx));
    fprintf(f, "  },\n");
    fprintf(f, "  \"tool_settings\": {\n");
    fprintf(f, "    \"brush_size\": %u,\n", (unsigned)ctx->ui.tool_brush_size);
    fprintf(f, "    \"brush_opacity\": %u,\n", (unsigned)ctx->ui.tool_brush_opacity);
    fprintf(f, "    \"brush_spacing\": %u,\n", (unsigned)ctx->ui.tool_brush_spacing);
    fprintf(f, "    \"brush_hardness\": %u,\n", (unsigned)ctx->ui.tool_brush_hardness);
    fprintf(f, "    \"eraser_size\": %u,\n", (unsigned)ctx->ui.tool_eraser_size);
    fprintf(f, "    \"shape_stroke_width\": %u,\n", (unsigned)ctx->ui.tool_shape_stroke_width);
    fprintf(f, "    \"shape_mode\": %u,\n", (unsigned)ctx->ui.tool_shape_mode);
    fprintf(f, "    \"shape_target_mode\": %u,\n", (unsigned)ctx->ui.tool_shape_target_mode);
    fprintf(f, "    \"fill_tolerance\": %u,\n", (unsigned)ctx->ui.tool_fill_tolerance);
    fprintf(f, "    \"select_mode\": %u\n", (unsigned)ctx->ui.tool_select_mode);
    fprintf(f, "  },\n");
    fprintf(f, "  \"layer_ui\": {\n");
    fprintf(f, "    \"opacity_entry_count\": %u,\n", (unsigned)ctx->ui.layer_opacity_entry_count);
    fprintf(f, "    \"opacity_entries\": [\n");
    for (i = 0u; i < ctx->ui.layer_opacity_entry_count && i < DRAWING_PROGRAM_MAX_LAYERS; ++i) {
        fprintf(f,
                "      {\"layer_id\": %u, \"opacity\": %u}%s\n",
                (unsigned)ctx->ui.layer_opacity_layer_ids[i],
                (unsigned)ctx->ui.layer_opacity_values[i],
                ((i + 1u) < ctx->ui.layer_opacity_entry_count && (i + 1u) < DRAWING_PROGRAM_MAX_LAYERS) ? "," : "");
    }
    fprintf(f, "    ]\n");
    fprintf(f, "  },\n");
    fprintf(f, "  \"selection\": {\n");
    fprintf(f, "    \"has_payload\": %u,\n", (unsigned)ctx->selection.has_payload);
    fprintf(f, "    \"selecting\": %u,\n", (unsigned)ctx->selection.selecting);
    fprintf(f, "    \"moving\": %u,\n", (unsigned)ctx->selection.moving);
    fprintf(f, "    \"origin_x\": %u,\n", (unsigned)ctx->selection.origin_x);
    fprintf(f, "    \"origin_y\": %u,\n", (unsigned)ctx->selection.origin_y);
    fprintf(f, "    \"width\": %u,\n", (unsigned)ctx->selection.width);
    fprintf(f, "    \"height\": %u,\n", (unsigned)ctx->selection.height);
    fprintf(f, "    \"offset_x\": %d,\n", (int)ctx->selection.offset_x);
    fprintf(f, "    \"offset_y\": %d,\n", (int)ctx->selection.offset_y);
    fprintf(f, "    \"payload_count\": %u\n", (unsigned)ctx->selection.payload_count);
    fprintf(f, "  },\n");
    fprintf(f, "  \"pane\": {\n");
    fprintf(f, "    \"node_count\": %u,\n", ctx->pane_host.node_count);
    fprintf(f, "    \"leaf_count\": %u,\n", ctx->pane_host.leaf_count);
    fprintf(f, "    \"module_binding_count\": %u,\n", ctx->pane_host.module_binding_count);
    fprintf(f, "    \"leaves\": [\n");
    for (i = 0u; i < ctx->pane_host.leaf_count; ++i) {
        const CorePaneLeafRect *leaf = &ctx->pane_host.leaves[i];
        fprintf(f,
                "      {\"id\": %u, \"x\": %.1f, \"y\": %.1f, \"w\": %.1f, \"h\": %.1f}%s\n",
                (unsigned)leaf->id,
                (double)leaf->rect.x,
                (double)leaf->rect.y,
                (double)leaf->rect.width,
                (double)leaf->rect.height,
                (i + 1u < ctx->pane_host.leaf_count) ? "," : "");
    }
    fprintf(f, "    ]\n");
    fprintf(f, "  }\n");
    fprintf(f, "}\n");

    if (fclose(f) != 0) {
        return (CoreResult){ CORE_ERR_IO, "failed to close snapshot json output" };
    }
    return core_result_ok();
}
