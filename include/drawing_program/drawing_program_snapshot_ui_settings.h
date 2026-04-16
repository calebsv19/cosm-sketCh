#ifndef DRAWING_PROGRAM_SNAPSHOT_UI_SETTINGS_H
#define DRAWING_PROGRAM_SNAPSHOT_UI_SETTINGS_H

#include "core_base.h"
#include "core_pack.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

CoreResult drawing_program_snapshot_write_ui_settings_chunk(
    CorePackWriter *writer,
    const struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_snapshot_apply_ui_settings_chunk(
    struct DrawingProgramAppContext *ctx,
    CorePackReader *reader,
    const CorePackChunkInfo *chunk);

#ifdef __cplusplus
}
#endif

#endif
