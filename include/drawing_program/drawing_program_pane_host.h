#ifndef DRAWING_PROGRAM_PANE_HOST_H
#define DRAWING_PROGRAM_PANE_HOST_H

#include <stdint.h>

#include "core_base.h"
#include "core_layout.h"
#include "core_pane.h"
#include "core_pane_module.h"
#include "kit_pane.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

#define DRAWING_PROGRAM_PANE_NODE_CAPACITY 32u
#define DRAWING_PROGRAM_PANE_LEAF_CAPACITY 16u
#define DRAWING_PROGRAM_PANE_SPLITTER_HIT_CAPACITY DRAWING_PROGRAM_PANE_NODE_CAPACITY
#define DRAWING_PROGRAM_MODULE_REGISTRY_CAPACITY 16u
#define DRAWING_PROGRAM_MODULE_BINDING_CAPACITY 16u

typedef struct DrawingProgramPaneHost {
    CoreLayoutState layout_state;
    CorePaneNode nodes[DRAWING_PROGRAM_PANE_NODE_CAPACITY];
    uint32_t node_count;
    uint32_t root_index;
    CorePaneLeafRect leaves[DRAWING_PROGRAM_PANE_LEAF_CAPACITY];
    uint32_t leaf_count;
    CorePaneSplitterHit splitter_hits[DRAWING_PROGRAM_PANE_SPLITTER_HIT_CAPACITY];
    uint32_t splitter_hit_count;
    CorePaneModuleDescriptor module_entries[DRAWING_PROGRAM_MODULE_REGISTRY_CAPACITY];
    CorePaneModuleRegistry module_registry;
    CorePaneModuleBinding module_bindings[DRAWING_PROGRAM_MODULE_BINDING_CAPACITY];
    uint32_t module_binding_count;
    KitPaneSplitterInteraction splitter_interaction;
} DrawingProgramPaneHost;

CoreResult drawing_program_pane_host_init(struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_pane_host_rebuild(struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_pane_host_rebind_default_modules(struct DrawingProgramAppContext *ctx);
int drawing_program_pane_host_default_modules_ready(const struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_pane_host_render(struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_pane_host_update_pointer(struct DrawingProgramAppContext *ctx,
                                                    float point_x,
                                                    float point_y);
int drawing_program_pane_host_begin_splitter_drag(struct DrawingProgramAppContext *ctx,
                                                  float point_x,
                                                  float point_y);
int drawing_program_pane_host_update_splitter_drag(struct DrawingProgramAppContext *ctx,
                                                   float point_x,
                                                   float point_y);
void drawing_program_pane_host_end_splitter_drag(struct DrawingProgramAppContext *ctx);
int drawing_program_pane_host_splitter_drag_active(const struct DrawingProgramAppContext *ctx);
int drawing_program_pane_host_visible_splitter(const struct DrawingProgramAppContext *ctx,
                                               CorePaneRect *out_bounds,
                                               int *out_hovered,
                                               int *out_active);

#ifdef __cplusplus
}
#endif

#endif
