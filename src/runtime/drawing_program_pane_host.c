#include "drawing_program/drawing_program_pane_host.h"

#include <string.h>

#include "drawing_program/drawing_program_app_main.h"

static CoreResult pane_host_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
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
    ctx->render_module_calls_total += 1u;
    ctx->render_module_canvas_calls_total += 1u;
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
    ctx->render_module_calls_total += 1u;
    ctx->render_module_palette_calls_total += 1u;
}

CoreResult drawing_program_pane_host_rebuild(struct DrawingProgramAppContext *ctx) {
    CorePaneRect bounds;
    uint32_t leaf_pane_ids[DRAWING_PROGRAM_PANE_LEAF_CAPACITY];
    uint32_t i;
    CorePaneValidationReport report;
    CorePaneModuleResult module_result;
    if (!ctx) {
        return pane_host_invalid("null app context");
    }

    bounds = (CorePaneRect){ 0.0f, 0.0f, 1200.0f, 800.0f };
    memset(&report, 0, sizeof(report));
    if (!core_pane_validate_graph(ctx->pane_host.nodes,
                                  ctx->pane_host.node_count,
                                  ctx->pane_host.root_index,
                                  bounds,
                                  &report)) {
        CoreResult r = { CORE_ERR_FORMAT, core_pane_validation_code_string(report.code) };
        return r;
    }
    if (!core_pane_solve(ctx->pane_host.nodes,
                         ctx->pane_host.node_count,
                         ctx->pane_host.root_index,
                         bounds,
                         ctx->pane_host.leaves,
                         DRAWING_PROGRAM_PANE_LEAF_CAPACITY,
                         &ctx->pane_host.leaf_count)) {
        CoreResult r = { CORE_ERR_FORMAT, "core_pane_solve failed" };
        return r;
    }
    if (ctx->pane_host.leaf_count == 0u) {
        CoreResult r = { CORE_ERR_FORMAT, "no pane leaves" };
        return r;
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
        CoreResult r = { CORE_ERR_FORMAT, "core_pane_module_validate_bindings failed" };
        return r;
    }

    return core_result_ok();
}

CoreResult drawing_program_pane_host_init(struct DrawingProgramAppContext *ctx) {
    CorePaneModuleDescriptor canvas_descriptor;
    CorePaneModuleDescriptor palette_descriptor;
    CorePaneModuleResult module_result;
    if (!ctx) {
        return pane_host_invalid("null app context");
    }

    memset(&ctx->pane_host, 0, sizeof(ctx->pane_host));
    core_layout_state_init(&ctx->pane_host.layout_state);

    ctx->pane_host.node_count = 3u;
    ctx->pane_host.root_index = 0u;
    ctx->pane_host.nodes[0] = (CorePaneNode){
        .type = CORE_PANE_NODE_SPLIT,
        .id = 1u,
        .axis = CORE_PANE_AXIS_HORIZONTAL,
        .ratio_01 = 0.75f,
        .child_a = 1u,
        .child_b = 2u,
        .constraints = { 280.0f, 220.0f }
    };
    ctx->pane_host.nodes[1] = (CorePaneNode){
        .type = CORE_PANE_NODE_LEAF,
        .id = 2u
    };
    ctx->pane_host.nodes[2] = (CorePaneNode){
        .type = CORE_PANE_NODE_LEAF,
        .id = 3u
    };

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

    ctx->pane_host.module_binding_count = 2u;
    ctx->pane_host.module_bindings[0] = (CorePaneModuleBinding){
        .instance_id = 1u,
        .pane_node_id = 2u,
        .module_type_id = 1u,
        .config_variant = 0u,
        .runtime_flags = 0u
    };
    ctx->pane_host.module_bindings[1] = (CorePaneModuleBinding){
        .instance_id = 2u,
        .pane_node_id = 3u,
        .module_type_id = 2u,
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
