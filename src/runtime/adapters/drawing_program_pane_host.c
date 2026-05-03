#include "drawing_program/drawing_program_pane_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_authoring_host.h"

enum {
    DRAWING_PROGRAM_PANE_SPLITTER_HANDLE_THICKNESS = 8,
    DRAWING_PROGRAM_PANE_HOST_DEFAULT_BOUNDS_WIDTH = 1200,
    DRAWING_PROGRAM_PANE_HOST_DEFAULT_BOUNDS_HEIGHT = 800
};

static CoreResult pane_host_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

static CoreResult pane_host_validation_failure(const CorePaneValidationReport *report, CorePaneRect bounds) {
    static char message[160];
    const char *code_string = "validation_failed";
    if (report) {
        code_string = core_pane_validation_code_string(report->code);
        (void)snprintf(message,
                       sizeof(message),
                       "%s node=%u related=%u bounds=%.2fx%.2f",
                       code_string,
                       (unsigned)report->node_index,
                       (unsigned)report->related_index,
                       (double)bounds.width,
                       (double)bounds.height);
    } else {
        (void)snprintf(message,
                       sizeof(message),
                       "validation_failed bounds=%.2fx%.2f",
                       (double)bounds.width,
                       (double)bounds.height);
    }
    return (CoreResult){ CORE_ERR_FORMAT, message };
}

static CorePaneRect drawing_program_pane_host_bounds(const struct DrawingProgramAppContext *ctx) {
    float width = 0.0f;
    float height = 0.0f;
    if (!ctx) {
        return (CorePaneRect){ 0.0f, 0.0f, 0.0f, 0.0f };
    }
    width = ctx->pane_host_bounds_width;
    height = ctx->pane_host_bounds_height;
    if (width < 64.0f || height < 64.0f) {
        width = (float)DRAWING_PROGRAM_PANE_HOST_DEFAULT_BOUNDS_WIDTH;
        height = (float)DRAWING_PROGRAM_PANE_HOST_DEFAULT_BOUNDS_HEIGHT;
    }
    return (CorePaneRect){ 0.0f, 0.0f, width, height };
}

static CoreResult drawing_program_pane_host_refresh_splitter_hits(struct DrawingProgramAppContext *ctx,
                                                                  CorePaneRect bounds) {
    if (!ctx) {
        return pane_host_invalid("null app context");
    }
    if (!core_pane_collect_splitter_hits(ctx->pane_host.nodes,
                                         ctx->pane_host.node_count,
                                         ctx->pane_host.root_index,
                                         bounds,
                                         (float)DRAWING_PROGRAM_PANE_SPLITTER_HANDLE_THICKNESS,
                                         ctx->pane_host.splitter_hits,
                                         DRAWING_PROGRAM_PANE_SPLITTER_HIT_CAPACITY,
                                         &ctx->pane_host.splitter_hit_count)) {
        CoreResult r = { CORE_ERR_FORMAT, "core_pane_collect_splitter_hits failed" };
        return r;
    }
    return core_result_ok();
}

static void drawing_program_module_render_canvas(void *host_context,
                                                 uint32_t pane_node_id,
                                                 uint32_t instance_id) {
    struct DrawingProgramAppContext *ctx = (struct DrawingProgramAppContext *)host_context;
    (void)pane_node_id;
    (void)instance_id;
    if (!ctx) {
        return;
    }
    ctx->runtime.render_module_calls_total += 1u;
    ctx->runtime.render_module_canvas_calls_total += 1u;
    ctx->runtime.render_canvas_last_raster_hash = ctx->runtime.render_projection.raster_hash32;
    ctx->runtime.render_canvas_last_nonzero_samples = ctx->runtime.render_projection.raster_nonzero_count;
}

static void drawing_program_module_render_palette(void *host_context,
                                                  uint32_t pane_node_id,
                                                  uint32_t instance_id) {
    struct DrawingProgramAppContext *ctx = (struct DrawingProgramAppContext *)host_context;
    (void)pane_node_id;
    (void)instance_id;
    if (!ctx) {
        return;
    }
    ctx->runtime.render_module_calls_total += 1u;
    ctx->runtime.render_module_palette_calls_total += 1u;
}

static void drawing_program_module_render_stub(void *host_context,
                                               uint32_t pane_node_id,
                                               uint32_t instance_id) {
    struct DrawingProgramAppContext *ctx = (struct DrawingProgramAppContext *)host_context;
    (void)pane_node_id;
    (void)instance_id;
    if (!ctx) {
        return;
    }
    ctx->runtime.render_module_calls_total += 1u;
}

static int drawing_program_pane_host_has_module_binding(const struct DrawingProgramAppContext *ctx,
                                                        uint32_t pane_node_id,
                                                        uint32_t module_type_id) {
    uint32_t i;
    if (!ctx) {
        return 0;
    }
    for (i = 0u; i < ctx->pane_host.module_binding_count; ++i) {
        const CorePaneModuleBinding *binding = &ctx->pane_host.module_bindings[i];
        if (binding->pane_node_id == pane_node_id && binding->module_type_id == module_type_id) {
            return 1;
        }
    }
    return 0;
}

static CoreResult drawing_program_pane_host_register_modules(struct DrawingProgramAppContext *ctx) {
    CorePaneModuleDescriptor canvas_descriptor;
    CorePaneModuleDescriptor palette_descriptor;
    CorePaneModuleDescriptor menu_descriptor;
    CorePaneModuleDescriptor inspector_descriptor;
    CorePaneModuleResult module_result;
    if (!ctx) {
        return pane_host_invalid("null app context");
    }

    module_result = core_pane_module_registry_init(&ctx->pane_host.module_registry,
                                                   ctx->pane_host.module_entries,
                                                   DRAWING_PROGRAM_MODULE_REGISTRY_CAPACITY);
    if (module_result != CORE_PANE_MODULE_OK) {
        CoreResult r = { CORE_ERR_FORMAT, "core_pane_module_registry_init failed" };
        return r;
    }

    memset(&canvas_descriptor, 0, sizeof(canvas_descriptor));
    canvas_descriptor.module_type_id = 1u;
    canvas_descriptor.module_key = "canvas";
    canvas_descriptor.display_name = "Canvas";
    canvas_descriptor.version_major = 1u;
    canvas_descriptor.version_minor = 0u;
    canvas_descriptor.capabilities = CORE_PANE_MODULE_CAP_RENDER;
    canvas_descriptor.provider_kind = CORE_PANE_MODULE_PROVIDER_INTERNAL;
    canvas_descriptor.render = drawing_program_module_render_canvas;
    module_result = core_pane_module_register(&ctx->pane_host.module_registry, &canvas_descriptor);
    if (module_result != CORE_PANE_MODULE_OK) {
        CoreResult r = { CORE_ERR_FORMAT, "register canvas descriptor failed" };
        return r;
    }

    memset(&palette_descriptor, 0, sizeof(palette_descriptor));
    palette_descriptor.module_type_id = 2u;
    palette_descriptor.module_key = "palette";
    palette_descriptor.display_name = "Palette";
    palette_descriptor.version_major = 1u;
    palette_descriptor.version_minor = 0u;
    palette_descriptor.capabilities = CORE_PANE_MODULE_CAP_RENDER;
    palette_descriptor.provider_kind = CORE_PANE_MODULE_PROVIDER_INTERNAL;
    palette_descriptor.render = drawing_program_module_render_palette;
    module_result = core_pane_module_register(&ctx->pane_host.module_registry, &palette_descriptor);
    if (module_result != CORE_PANE_MODULE_OK) {
        CoreResult r = { CORE_ERR_FORMAT, "register palette descriptor failed" };
        return r;
    }

    memset(&menu_descriptor, 0, sizeof(menu_descriptor));
    menu_descriptor.module_type_id = 3u;
    menu_descriptor.module_key = "menu_bar";
    menu_descriptor.display_name = "Menu Bar";
    menu_descriptor.version_major = 1u;
    menu_descriptor.version_minor = 0u;
    menu_descriptor.capabilities = CORE_PANE_MODULE_CAP_RENDER;
    menu_descriptor.provider_kind = CORE_PANE_MODULE_PROVIDER_INTERNAL;
    menu_descriptor.render = drawing_program_module_render_stub;
    module_result = core_pane_module_register(&ctx->pane_host.module_registry, &menu_descriptor);
    if (module_result != CORE_PANE_MODULE_OK) {
        CoreResult r = { CORE_ERR_FORMAT, "register menu descriptor failed" };
        return r;
    }

    memset(&inspector_descriptor, 0, sizeof(inspector_descriptor));
    inspector_descriptor.module_type_id = 4u;
    inspector_descriptor.module_key = "inspector";
    inspector_descriptor.display_name = "Inspector";
    inspector_descriptor.version_major = 1u;
    inspector_descriptor.version_minor = 0u;
    inspector_descriptor.capabilities = CORE_PANE_MODULE_CAP_RENDER;
    inspector_descriptor.provider_kind = CORE_PANE_MODULE_PROVIDER_INTERNAL;
    inspector_descriptor.render = drawing_program_module_render_stub;
    module_result = core_pane_module_register(&ctx->pane_host.module_registry, &inspector_descriptor);
    if (module_result != CORE_PANE_MODULE_OK) {
        CoreResult r = { CORE_ERR_FORMAT, "register inspector descriptor failed" };
        return r;
    }

    return core_result_ok();
}

CoreResult drawing_program_pane_host_rebuild(struct DrawingProgramAppContext *ctx) {
    CorePaneRect bounds;
    uint32_t leaf_pane_ids[DRAWING_PROGRAM_PANE_LEAF_CAPACITY];
    uint32_t i;
    CorePaneValidationReport report;
    CorePaneModuleResult module_result;
    DrawingProgramAppUiState preserved_ui;
    CoreResult final_result = core_result_ok();
    if (!ctx) {
        return pane_host_invalid("null app context");
    }

    preserved_ui = ctx->ui;
    bounds = drawing_program_pane_host_bounds(ctx);
    memset(&report, 0, sizeof(report));
    if (!core_pane_validate_graph(ctx->pane_host.nodes,
                                  ctx->pane_host.node_count,
                                  ctx->pane_host.root_index,
                                  bounds,
                                  &report)) {
        final_result = pane_host_validation_failure(&report, bounds);
        goto restore_ui;
    }
    if (!core_pane_solve(ctx->pane_host.nodes,
                         ctx->pane_host.node_count,
                         ctx->pane_host.root_index,
                         bounds,
                         ctx->pane_host.leaves,
                         DRAWING_PROGRAM_PANE_LEAF_CAPACITY,
                         &ctx->pane_host.leaf_count)) {
        final_result = (CoreResult){ CORE_ERR_FORMAT, "core_pane_solve failed" };
        goto restore_ui;
    }
    if (ctx->pane_host.leaf_count == 0u) {
        final_result = (CoreResult){ CORE_ERR_FORMAT, "no pane leaves" };
        goto restore_ui;
    }
    if (drawing_program_pane_host_refresh_splitter_hits(ctx, bounds).code != CORE_OK) {
        final_result = (CoreResult){ CORE_ERR_FORMAT, "splitter hit refresh failed" };
        goto restore_ui;
    }
    if (drawing_program_pane_host_register_modules(ctx).code != CORE_OK) {
        final_result = (CoreResult){ CORE_ERR_FORMAT, "pane host module registry rebuild failed" };
        goto restore_ui;
    }

    for (i = 0u; i < ctx->pane_host.leaf_count; ++i) {
        leaf_pane_ids[i] = (uint32_t)ctx->pane_host.leaves[i].id;
    }
    module_result = core_pane_module_validate_bindings(&ctx->pane_host.module_registry,
                                                       ctx->pane_host.module_bindings,
                                                       ctx->pane_host.module_binding_count,
                                                       leaf_pane_ids,
                                                       ctx->pane_host.leaf_count);
    if (module_result != CORE_PANE_MODULE_OK) {
        static char module_message[160];
        (void)snprintf(module_message,
                       sizeof(module_message),
                       "core_pane_module_validate_bindings failed code=%u leaves=%u bindings=%u",
                       (unsigned)module_result,
                       (unsigned)ctx->pane_host.leaf_count,
                       (unsigned)ctx->pane_host.module_binding_count);
        final_result = (CoreResult){ CORE_ERR_FORMAT, module_message };
        goto restore_ui;
    }

restore_ui:
    ctx->ui = preserved_ui;
    return final_result;
}

int drawing_program_pane_host_default_modules_ready(const struct DrawingProgramAppContext *ctx) {
    uint32_t menu_idx = 0u;
    uint32_t i;
    uint32_t candidate_indices[DRAWING_PROGRAM_PANE_LEAF_CAPACITY];
    uint32_t candidate_count = 0u;
    uint32_t left_idx;
    uint32_t right_idx;
    uint32_t center_idx = 0u;
    float min_x = 0.0f;
    float max_x = 0.0f;
    float target_mid = 0.0f;
    float best_distance = 0.0f;
    int center_set = 0;
    if (!ctx || ctx->pane_host.leaf_count < 4u || ctx->pane_host.module_binding_count < 4u) {
        return 0;
    }

    for (i = 1u; i < ctx->pane_host.leaf_count; ++i) {
        const CorePaneLeafRect *candidate = &ctx->pane_host.leaves[i];
        const CorePaneLeafRect *best = &ctx->pane_host.leaves[menu_idx];
        if (candidate->rect.y < best->rect.y ||
            (candidate->rect.y == best->rect.y && candidate->rect.width > best->rect.width)) {
            menu_idx = i;
        }
    }
    for (i = 0u; i < ctx->pane_host.leaf_count; ++i) {
        if (i == menu_idx) {
            continue;
        }
        candidate_indices[candidate_count++] = i;
    }
    if (candidate_count < 3u) {
        return 0;
    }

    left_idx = candidate_indices[0];
    right_idx = candidate_indices[0];
    min_x = ctx->pane_host.leaves[left_idx].rect.x;
    max_x = min_x;
    for (i = 1u; i < candidate_count; ++i) {
        uint32_t idx = candidate_indices[i];
        float x = ctx->pane_host.leaves[idx].rect.x;
        if (x < min_x) {
            min_x = x;
            left_idx = idx;
        }
        if (x > max_x) {
            max_x = x;
            right_idx = idx;
        }
    }

    target_mid = (min_x + max_x) * 0.5f;
    for (i = 0u; i < candidate_count; ++i) {
        uint32_t idx = candidate_indices[i];
        float center_x;
        float distance;
        if (idx == left_idx || idx == right_idx) {
            continue;
        }
        center_x = ctx->pane_host.leaves[idx].rect.x + (ctx->pane_host.leaves[idx].rect.width * 0.5f);
        distance = fabsf(center_x - target_mid);
        if (!center_set || distance < best_distance) {
            center_set = 1;
            best_distance = distance;
            center_idx = idx;
        }
    }
    if (!center_set) {
        return 0;
    }

    return drawing_program_pane_host_has_module_binding(ctx, (uint32_t)ctx->pane_host.leaves[menu_idx].id, 3u) &&
           drawing_program_pane_host_has_module_binding(ctx, (uint32_t)ctx->pane_host.leaves[left_idx].id, 2u) &&
           drawing_program_pane_host_has_module_binding(ctx, (uint32_t)ctx->pane_host.leaves[center_idx].id, 1u) &&
           drawing_program_pane_host_has_module_binding(ctx, (uint32_t)ctx->pane_host.leaves[right_idx].id, 4u);
}

CoreResult drawing_program_pane_host_rebind_default_modules(struct DrawingProgramAppContext *ctx) {
    uint32_t menu_idx = 0u;
    uint32_t i;
    uint32_t candidate_indices[DRAWING_PROGRAM_PANE_LEAF_CAPACITY];
    uint32_t candidate_count = 0u;
    uint32_t left_idx;
    uint32_t right_idx;
    uint32_t center_idx = 0u;
    float min_x = 0.0f;
    float max_x = 0.0f;
    float target_mid = 0.0f;
    float best_distance = 0.0f;
    int center_set = 0;
    if (!ctx) {
        return pane_host_invalid("invalid module rebind request");
    }
    if (ctx->pane_host.leaf_count < 4u) {
        CoreResult r = { CORE_ERR_FORMAT, "pane host requires at least 4 pane leaves for module rebind" };
        return r;
    }

    for (i = 1u; i < ctx->pane_host.leaf_count; ++i) {
        const CorePaneLeafRect *candidate = &ctx->pane_host.leaves[i];
        const CorePaneLeafRect *best = &ctx->pane_host.leaves[menu_idx];
        if (candidate->rect.y < best->rect.y ||
            (candidate->rect.y == best->rect.y && candidate->rect.width > best->rect.width)) {
            menu_idx = i;
        }
    }
    for (i = 0u; i < ctx->pane_host.leaf_count; ++i) {
        if (i == menu_idx) {
            continue;
        }
        candidate_indices[candidate_count++] = i;
    }
    if (candidate_count < 3u) {
        CoreResult r = { CORE_ERR_FORMAT, "pane host missing side/canvas panes for module rebind" };
        return r;
    }

    left_idx = candidate_indices[0];
    right_idx = candidate_indices[0];
    min_x = ctx->pane_host.leaves[left_idx].rect.x;
    max_x = min_x;
    for (i = 1u; i < candidate_count; ++i) {
        uint32_t idx = candidate_indices[i];
        float x = ctx->pane_host.leaves[idx].rect.x;
        if (x < min_x) {
            min_x = x;
            left_idx = idx;
        }
        if (x > max_x) {
            max_x = x;
            right_idx = idx;
        }
    }

    target_mid = (min_x + max_x) * 0.5f;
    for (i = 0u; i < candidate_count; ++i) {
        uint32_t idx = candidate_indices[i];
        float center_x;
        float distance;
        if (idx == left_idx || idx == right_idx) {
            continue;
        }
        center_x = ctx->pane_host.leaves[idx].rect.x + (ctx->pane_host.leaves[idx].rect.width * 0.5f);
        distance = fabsf(center_x - target_mid);
        if (!center_set || distance < best_distance) {
            center_set = 1;
            best_distance = distance;
            center_idx = idx;
        }
    }
    if (!center_set) {
        for (i = 0u; i < candidate_count; ++i) {
            uint32_t idx = candidate_indices[i];
            if (idx != left_idx && idx != right_idx) {
                center_idx = idx;
                center_set = 1;
                break;
            }
        }
    }
    if (!center_set || left_idx == right_idx || center_idx == left_idx || center_idx == right_idx) {
        CoreResult r = { CORE_ERR_FORMAT, "pane host failed to resolve default pane roles for module rebind" };
        return r;
    }

    ctx->pane_host.module_binding_count = 4u;
    ctx->pane_host.module_bindings[0] = (CorePaneModuleBinding){
        .instance_id = 1u,
        .pane_node_id = (uint32_t)ctx->pane_host.leaves[menu_idx].id,
        .module_type_id = 3u,
        .config_variant = 0u,
        .runtime_flags = 0u
    };
    ctx->pane_host.module_bindings[1] = (CorePaneModuleBinding){
        .instance_id = 2u,
        .pane_node_id = (uint32_t)ctx->pane_host.leaves[left_idx].id,
        .module_type_id = 2u,
        .config_variant = 0u,
        .runtime_flags = 0u
    };
    ctx->pane_host.module_bindings[2] = (CorePaneModuleBinding){
        .instance_id = 3u,
        .pane_node_id = (uint32_t)ctx->pane_host.leaves[center_idx].id,
        .module_type_id = 1u,
        .config_variant = 0u,
        .runtime_flags = 0u
    };
    ctx->pane_host.module_bindings[3] = (CorePaneModuleBinding){
        .instance_id = 4u,
        .pane_node_id = (uint32_t)ctx->pane_host.leaves[right_idx].id,
        .module_type_id = 4u,
        .config_variant = 0u,
        .runtime_flags = 0u
    };
    return core_result_ok();
}

CoreResult drawing_program_pane_host_init(struct DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return pane_host_invalid("null app context");
    }

    memset(&ctx->pane_host, 0, sizeof(ctx->pane_host));
    core_layout_state_init(&ctx->pane_host.layout_state);
    kit_pane_splitter_interaction_init(&ctx->pane_host.splitter_interaction,
                                       (float)DRAWING_PROGRAM_PANE_SPLITTER_HANDLE_THICKNESS);

    ctx->pane_host.node_count = 7u;
    ctx->pane_host.root_index = 0u;
    ctx->pane_host.nodes[0] = (CorePaneNode){
        .type = CORE_PANE_NODE_SPLIT,
        .id = 1u,
        .axis = CORE_PANE_AXIS_VERTICAL,
        .ratio_01 = 0.08f,
        .child_a = 1u,
        .child_b = 2u,
        .constraints = { 48.0f, 220.0f }
    };
    ctx->pane_host.nodes[1] = (CorePaneNode){
        .type = CORE_PANE_NODE_LEAF,
        .id = 2u
    };
    ctx->pane_host.nodes[2] = (CorePaneNode){
        .type = CORE_PANE_NODE_SPLIT,
        .id = 3u,
        .axis = CORE_PANE_AXIS_HORIZONTAL,
        .ratio_01 = 0.18f,
        .child_a = 3u,
        .child_b = 4u,
        .constraints = { 180.0f, 280.0f }
    };
    ctx->pane_host.nodes[3] = (CorePaneNode){
        .type = CORE_PANE_NODE_LEAF,
        .id = 4u
    };
    ctx->pane_host.nodes[4] = (CorePaneNode){
        .type = CORE_PANE_NODE_SPLIT,
        .id = 5u,
        .axis = CORE_PANE_AXIS_HORIZONTAL,
        .ratio_01 = 0.72f,
        .child_a = 5u,
        .child_b = 6u,
        .constraints = { 280.0f, 200.0f }
    };
    ctx->pane_host.nodes[5] = (CorePaneNode){
        .type = CORE_PANE_NODE_LEAF,
        .id = 6u
    };
    ctx->pane_host.nodes[6] = (CorePaneNode){
        .type = CORE_PANE_NODE_LEAF,
        .id = 7u
    };

    ctx->pane_host.module_binding_count = 4u;
    ctx->pane_host.module_bindings[0] = (CorePaneModuleBinding){
        .instance_id = 1u,
        .pane_node_id = 2u,
        .module_type_id = 3u,
        .config_variant = 0u,
        .runtime_flags = 0u
    };
    ctx->pane_host.module_bindings[1] = (CorePaneModuleBinding){
        .instance_id = 2u,
        .pane_node_id = 4u,
        .module_type_id = 2u,
        .config_variant = 0u,
        .runtime_flags = 0u
    };
    ctx->pane_host.module_bindings[2] = (CorePaneModuleBinding){
        .instance_id = 3u,
        .pane_node_id = 6u,
        .module_type_id = 1u,
        .config_variant = 0u,
        .runtime_flags = 0u
    };
    ctx->pane_host.module_bindings[3] = (CorePaneModuleBinding){
        .instance_id = 4u,
        .pane_node_id = 7u,
        .module_type_id = 4u,
        .config_variant = 0u,
        .runtime_flags = 0u
    };

    return drawing_program_pane_host_rebuild(ctx);
}

CoreResult drawing_program_pane_host_render(struct DrawingProgramAppContext *ctx) {
    uint32_t i;
    if (!ctx) {
        return pane_host_invalid("null app context");
    }
    for (i = 0u; i < ctx->pane_host.module_binding_count; ++i) {
        const CorePaneModuleBinding *binding = &ctx->pane_host.module_bindings[i];
        const CorePaneModuleDescriptor *descriptor = 0;
        CorePaneModuleResult find_result = core_pane_module_find_by_type_id(&ctx->pane_host.module_registry,
                                                                            binding->module_type_id,
                                                                            &descriptor);
        if (find_result != CORE_PANE_MODULE_OK || !descriptor || !descriptor->render) {
            CoreResult r = { CORE_ERR_FORMAT, "pane render binding lookup failed" };
            return r;
        }
        descriptor->render(ctx, binding->pane_node_id, binding->instance_id);
    }
    return core_result_ok();
}

CoreResult drawing_program_pane_host_update_pointer(struct DrawingProgramAppContext *ctx,
                                                    float point_x,
                                                    float point_y) {
    CoreResult result;
    if (!ctx) {
        return pane_host_invalid("null app context");
    }
    result = kit_pane_splitter_interaction_set_hover_from_hits(&ctx->pane_host.splitter_interaction,
                                                               ctx->pane_host.splitter_hits,
                                                               ctx->pane_host.splitter_hit_count,
                                                               point_x,
                                                               point_y);
    if (result.code == CORE_ERR_NOT_FOUND) {
        return core_result_ok();
    }
    return result;
}

int drawing_program_pane_host_begin_splitter_drag(struct DrawingProgramAppContext *ctx,
                                                  float point_x,
                                                  float point_y) {
    CoreResult result;
    if (!ctx) {
        return 0;
    }
    if (ctx->overlay_adapter.lifecycle_state != DRAWING_PROGRAM_OVERLAY_STATE_RUNTIME_ACTIVE ||
        ctx->overlay_adapter.runtime_paused) {
        return 0;
    }
    result = kit_pane_splitter_interaction_begin_drag_from_hits(&ctx->pane_host.splitter_interaction,
                                                                ctx->pane_host.splitter_hits,
                                                                ctx->pane_host.splitter_hit_count,
                                                                point_x,
                                                                point_y);
    return result.code == CORE_OK ? 1 : 0;
}

int drawing_program_pane_host_update_splitter_drag(struct DrawingProgramAppContext *ctx,
                                                   float point_x,
                                                   float point_y) {
    CoreResult result;
    int changed = 0;
    if (!ctx) {
        return 0;
    }
    result = kit_pane_splitter_interaction_update_drag(&ctx->pane_host.splitter_interaction,
                                                       ctx->pane_host.nodes,
                                                       ctx->pane_host.node_count,
                                                       point_x,
                                                       point_y,
                                                       &changed);
    if (result.code != CORE_OK) {
        return 0;
    }
    if (changed) {
        if (drawing_program_pane_host_rebuild(ctx).code != CORE_OK) {
            return 0;
        }
        if (drawing_program_authoring_host_mark_draft_changed(ctx).code != CORE_OK) {
            return 0;
        }
    }
    return changed;
}

void drawing_program_pane_host_end_splitter_drag(struct DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return;
    }
    kit_pane_splitter_interaction_end_drag(&ctx->pane_host.splitter_interaction);
}

int drawing_program_pane_host_splitter_drag_active(const struct DrawingProgramAppContext *ctx) {
    if (!ctx) {
        return 0;
    }
    return ctx->pane_host.splitter_interaction.drag_active ? 1 : 0;
}

int drawing_program_pane_host_visible_splitter(const struct DrawingProgramAppContext *ctx,
                                               CorePaneRect *out_bounds,
                                               int *out_hovered,
                                               int *out_active) {
    if (!ctx) {
        return 0;
    }
    return kit_pane_splitter_interaction_current(&ctx->pane_host.splitter_interaction,
                                                 out_bounds,
                                                 out_hovered,
                                                 out_active);
}
