#ifndef DRAWING_PROGRAM_SNAPSHOT_H
#define DRAWING_PROGRAM_SNAPSHOT_H

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

CoreResult drawing_program_snapshot_save(const struct DrawingProgramAppContext *ctx, const char *path);
CoreResult drawing_program_snapshot_load(struct DrawingProgramAppContext *ctx, const char *path);
CoreResult drawing_program_snapshot_export_debug_json(const struct DrawingProgramAppContext *ctx,
                                                      const char *path);
CoreResult drawing_program_snapshot_bridge_check_workspace_preset(const char *workspace_preset_path);
CoreResult drawing_program_snapshot_bridge_import_workspace_preset(struct DrawingProgramAppContext *ctx,
                                                                   const char *workspace_preset_path);

#ifdef __cplusplus
}
#endif

#endif
