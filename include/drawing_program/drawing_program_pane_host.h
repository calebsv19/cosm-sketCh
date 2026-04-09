#ifndef DRAWING_PROGRAM_PANE_HOST_H
#define DRAWING_PROGRAM_PANE_HOST_H

#include <stdint.h>

#include "core_base.h"
#include "core_layout.h"
#include "core_pane.h"
#include "core_pane_module.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

#define DRAWING_PROGRAM_PANE_NODE_CAPACITY 8u
#define DRAWING_PROGRAM_PANE_LEAF_CAPACITY 8u
#define DRAWING_PROGRAM_MODULE_REGISTRY_CAPACITY 16u
#define DRAWING_PROGRAM_MODULE_BINDING_CAPACITY 8u

typedef struct DrawingProgramPaneHost {
    CoreLayoutState layout_state;
    CorePaneNode nodes[DRAWING_PROGRAM_PANE_NODE_CAPACITY];
    uint32_t node_count;
    uint32_t root_index;
    CorePaneLeafRect leaves[DRAWING_PROGRAM_PANE_LEAF_CAPACITY];
    uint32_t leaf_count;
    CorePaneModuleDescriptor module_entries[DRAWING_PROGRAM_MODULE_REGISTRY_CAPACITY];
    CorePaneModuleRegistry module_registry;
    CorePaneModuleBinding module_bindings[DRAWING_PROGRAM_MODULE_BINDING_CAPACITY];
    uint32_t module_binding_count;
} DrawingProgramPaneHost;

CoreResult drawing_program_pane_host_init(struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_pane_host_rebuild(struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_pane_host_render(struct DrawingProgramAppContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
