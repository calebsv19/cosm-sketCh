#ifndef DRAWING_PROGRAM_PANE_HOST_H
#define DRAWING_PROGRAM_PANE_HOST_H

#include <stdint.h>

#include "core_base.h"
#include "core_layout.h"
#include "core_pane.h"
#include "core_pane_module.h"
#include "kit_pane.h"
#include "kit_pane_host.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

#define DRAWING_PROGRAM_PANE_NODE_CAPACITY 32u
#define DRAWING_PROGRAM_PANE_LEAF_CAPACITY 16u
#define DRAWING_PROGRAM_PANE_SPLITTER_HIT_CAPACITY DRAWING_PROGRAM_PANE_NODE_CAPACITY
#define DRAWING_PROGRAM_MODULE_REGISTRY_CAPACITY 16u
#define DRAWING_PROGRAM_MODULE_BINDING_CAPACITY 16u
#define DRAWING_PROGRAM_PANE_HIDDEN 1u /* App-owned persisted binding flag. */

typedef struct DrawingProgramPaneState {
    CoreLayoutState layout;
    CorePaneNode nodes[DRAWING_PROGRAM_PANE_NODE_CAPACITY];
    uint32_t node_count, root_index;
    CorePaneModuleBinding bindings[DRAWING_PROGRAM_MODULE_BINDING_CAPACITY];
    uint32_t binding_count;
} DrawingProgramPaneState;

/* Controllers borrow document/model state. Renderer caches stay host-owned. */
typedef struct DrawingProgramPaneController {
    CorePaneModuleBinding binding;
    uint64_t generation;
    int mounted, focused, captured;
} DrawingProgramPaneController;
typedef void (*DrawingProgramPaneLifecycleObserver)(void *user,
    const CorePaneModuleBinding *binding, const KitPaneHostEvent *event);

typedef struct DrawingProgramPaneHost {
    CoreLayoutState layout_state;
    CorePaneNode nodes[DRAWING_PROGRAM_PANE_NODE_CAPACITY];
    uint32_t node_count;
    uint32_t root_index;
    CorePaneLeafRect leaves[DRAWING_PROGRAM_PANE_LEAF_CAPACITY];
    uint32_t leaf_count;
    CorePaneSplitterHit splitter_hits[DRAWING_PROGRAM_PANE_SPLITTER_HIT_CAPACITY];
    uint32_t splitter_hit_count;
    float splitter_scale_x, splitter_scale_y;
    CorePaneModuleDescriptor module_entries[DRAWING_PROGRAM_MODULE_REGISTRY_CAPACITY];
    CorePaneModuleRegistry module_registry;
    CorePaneModuleBinding module_bindings[DRAWING_PROGRAM_MODULE_BINDING_CAPACITY];
    uint32_t module_binding_count;
    KitPaneSplitterInteraction splitter_interaction;
    KitPaneLayoutEdit splitter_edit;
    CorePaneNode splitter_before[DRAWING_PROGRAM_PANE_NODE_CAPACITY];
    KitPaneHost composition_host;
    DrawingProgramPaneController controllers[DRAWING_PROGRAM_MODULE_BINDING_CAPACITY];
    uint32_t controller_count;
    uint64_t next_controller_generation;
    DrawingProgramPaneLifecycleObserver lifecycle_observer;
    void *lifecycle_user;
    void (*document_swap_hook)(void *user);
    DrawingProgramPaneState valid_state;
    int valid_state_ready;
} DrawingProgramPaneHost;

void drawing_program_pane_host_capture_state(const DrawingProgramPaneHost *host,
    DrawingProgramPaneState *state);
void drawing_program_pane_host_restore_state(DrawingProgramPaneHost *host,
    const DrawingProgramPaneState *state);
CoreResult drawing_program_pane_host_validate_state(const DrawingProgramPaneHost *host,
    const DrawingProgramPaneState *state, CorePaneRect bounds);
void drawing_program_pane_host_observe(struct DrawingProgramAppContext *ctx,
    DrawingProgramPaneLifecycleObserver observer, void *user);
void drawing_program_pane_host_before_document_swap(struct DrawingProgramAppContext *ctx);
void drawing_program_pane_host_document_swap_hook(struct DrawingProgramAppContext *ctx,
    void (*hook)(void *user));
CoreResult drawing_program_pane_host_sync_controllers(struct DrawingProgramAppContext *ctx,
    const KitPaneComposition *view, int blocked);
void drawing_program_pane_host_cancel_input(struct DrawingProgramAppContext *ctx);
CorePaneId drawing_program_pane_host_route_pointer(struct DrawingProgramAppContext *ctx,
    KitPaneHostEventType type, float x, float y);
void drawing_program_pane_host_dispose(struct DrawingProgramAppContext *ctx);

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
CoreResult drawing_program_pane_host_set_splitter_scale(struct DrawingProgramAppContext *ctx,
                                                        float scale_x, float scale_y);
int drawing_program_pane_host_update_splitter_drag(struct DrawingProgramAppContext *ctx,
                                                   float point_x,
                                                   float point_y);
void drawing_program_pane_host_cancel_splitter_drag(struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_pane_host_compose(struct DrawingProgramAppContext *ctx, int blocked);
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
