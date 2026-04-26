#ifndef DRAWING_PROGRAM_SNAPSHOT_SHELL_H
#define DRAWING_PROGRAM_SNAPSHOT_SHELL_H

#include <stdint.h>

#include "core_base.h"
#include "core_pack.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

const char *drawing_program_snapshot_shell_chunk_id_current(void);
CoreResult drawing_program_snapshot_shell_write_current(
    CorePackWriter *writer,
    const struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_snapshot_shell_load_current(
    struct DrawingProgramAppContext *ctx,
    CorePackReader *reader,
    uint8_t *out_found);

#ifdef __cplusplus
}
#endif

#endif
