#ifndef DRAWING_PROGRAM_PROJECT_STATE_H
#define DRAWING_PROGRAM_PROJECT_STATE_H

#include <stddef.h>

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DrawingProgramAppContext;

CoreResult drawing_program_project_state_configure_defaults(struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_project_state_prepare_new_path(struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_project_state_refresh_recent(struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_project_state_select_recent(struct DrawingProgramAppContext *ctx, uint32_t recent_index);
CoreResult drawing_program_project_state_select_slot(struct DrawingProgramAppContext *ctx, uint32_t slot_index);
CoreResult drawing_program_project_state_set_input_root(struct DrawingProgramAppContext *ctx, const char *path);
CoreResult drawing_program_project_state_set_output_root(struct DrawingProgramAppContext *ctx, const char *path);
CoreResult drawing_program_project_state_set_current_path(struct DrawingProgramAppContext *ctx, const char *path);
CoreResult drawing_program_project_state_set_save_as_path(struct DrawingProgramAppContext *ctx, const char *path);
CoreResult drawing_program_project_state_current_exists(const struct DrawingProgramAppContext *ctx, uint8_t *out_exists);
CoreResult drawing_program_project_state_begin_new_blank(struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_project_state_slot_path(const struct DrawingProgramAppContext *ctx,
                                                   uint32_t slot_index,
                                                   char *out_path,
                                                   size_t out_cap,
                                                   uint8_t *out_existing);
CoreResult drawing_program_project_state_save_current(struct DrawingProgramAppContext *ctx);
CoreResult drawing_program_project_state_load_current(struct DrawingProgramAppContext *ctx);
void drawing_program_project_state_mark_clean(struct DrawingProgramAppContext *ctx);
uint8_t drawing_program_project_state_current_is_dirty(const struct DrawingProgramAppContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
