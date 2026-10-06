#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_visual_pane_geometry.h"
#include <string.h>

void drawing_program_pane_host_cancel_splitter_drag(
    DrawingProgramAppContext *ctx) {
  if (!ctx)
    return;
  DrawingProgramPaneHost *host = &ctx->pane_host;
  if (host->splitter_edit.active) {
    memcpy(host->nodes, host->splitter_before, sizeof(host->nodes));
    (void)kit_pane_layout_edit_cancel(&host->splitter_edit,
                                      &host->layout_state);
    (void)drawing_program_pane_host_rebuild(ctx);
  }
  kit_pane_splitter_interaction_end_drag(&host->splitter_interaction);
  kit_pane_host_cancel(&host->composition_host, NULL, NULL);
}
CoreResult drawing_program_pane_host_compose(DrawingProgramAppContext *ctx,
                                             int blocked) {
  if (!ctx)
    return (CoreResult){CORE_ERR_INVALID_ARG, "missing pane host"};
  KitPaneCompositionSpec specs[DRAWING_PROGRAM_PANE_LEAF_CAPACITY];
  CorePaneRect viewport = {0};
  for (uint32_t i = 0; i < ctx->pane_host.leaf_count; i++) {
    CorePaneLeafRect leaf = ctx->pane_host.leaves[i];
    specs[i] = drawing_program_visual_pane_spec(ctx, &leaf);
    float right = leaf.rect.x + leaf.rect.width,
          bottom = leaf.rect.y + leaf.rect.height;
    if (right > viewport.width)
      viewport.width = right;
    if (bottom > viewport.height)
      viewport.height = bottom;
  }
  KitPaneComposition view;
  CoreResult result = kit_pane_composition_build(
      &view, specs, ctx->pane_host.leaf_count, viewport);
  if (result.code != CORE_OK)
    return result;
  return kit_pane_host_sync(&ctx->pane_host.composition_host, &view, blocked,
                            NULL, NULL);
}
