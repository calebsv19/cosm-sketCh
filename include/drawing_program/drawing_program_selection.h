#ifndef DRAWING_PROGRAM_SELECTION_H
#define DRAWING_PROGRAM_SELECTION_H

#include <stdint.h>

#include "core_base.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_history.h"
#include "drawing_program/drawing_program_layer_raster.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_SELECTION_MAX_WIDTH 512u
#define DRAWING_PROGRAM_SELECTION_MAX_HEIGHT 512u
#define DRAWING_PROGRAM_SELECTION_MAX_AREA (DRAWING_PROGRAM_SELECTION_MAX_WIDTH * DRAWING_PROGRAM_SELECTION_MAX_HEIGHT)
#define DRAWING_PROGRAM_SELECTION_MAX_AFFECTED (DRAWING_PROGRAM_SELECTION_MAX_AREA * 2u)

typedef struct DrawingProgramSelectionState {
    uint8_t has_payload;
    uint8_t selecting;
    uint8_t moving;
    uint32_t layer_id;
    uint32_t origin_x;
    uint32_t origin_y;
    uint32_t width;
    uint32_t height;
    int32_t offset_x;
    int32_t offset_y;
    uint32_t marquee_start_x;
    uint32_t marquee_start_y;
    uint32_t marquee_end_x;
    uint32_t marquee_end_y;
    uint32_t move_anchor_sample_x;
    uint32_t move_anchor_sample_y;
    int32_t move_anchor_offset_x;
    int32_t move_anchor_offset_y;
    uint32_t payload_count;
    uint8_t payload_mask[DRAWING_PROGRAM_SELECTION_MAX_AREA];
    uint8_t payload_value[DRAWING_PROGRAM_SELECTION_MAX_AREA];
} DrawingProgramSelectionState;

typedef struct DrawingProgramClipboardState {
    uint8_t has_payload;
    uint32_t source_layer_id;
    uint32_t width;
    uint32_t height;
    uint32_t payload_count;
    uint8_t payload_mask[DRAWING_PROGRAM_SELECTION_MAX_AREA];
    uint8_t payload_value[DRAWING_PROGRAM_SELECTION_MAX_AREA];
} DrawingProgramClipboardState;

void drawing_program_selection_reset(DrawingProgramSelectionState *selection);
void drawing_program_selection_cancel_transient(DrawingProgramSelectionState *selection);
void drawing_program_clipboard_reset(DrawingProgramClipboardState *clipboard);
uint8_t drawing_program_selection_mask_at(const DrawingProgramSelectionState *selection,
                                          uint32_t local_x,
                                          uint32_t local_y);
uint8_t drawing_program_selection_value_at(const DrawingProgramSelectionState *selection,
                                           uint32_t local_x,
                                           uint32_t local_y);
int drawing_program_selection_capture_from_rect(const DrawingProgramDocument *document,
                                                const DrawingProgramLayerRasterStore *layer_rasters,
                                                uint32_t active_layer_id,
                                                DrawingProgramSelectionState *selection,
                                                int32_t x0,
                                                int32_t y0,
                                                uint32_t width,
                                                uint32_t height);
int drawing_program_selection_add_from_rect(const DrawingProgramDocument *document,
                                            const DrawingProgramLayerRasterStore *layer_rasters,
                                            uint32_t active_layer_id,
                                            DrawingProgramSelectionState *selection,
                                            int32_t x0,
                                            int32_t y0,
                                            uint32_t width,
                                            uint32_t height);
int drawing_program_selection_subtract_from_rect(const DrawingProgramDocument *document,
                                                 const DrawingProgramLayerRasterStore *layer_rasters,
                                                 uint32_t active_layer_id,
                                                 DrawingProgramSelectionState *selection,
                                                 int32_t x0,
                                                 int32_t y0,
                                                 uint32_t width,
                                                 uint32_t height);
int drawing_program_selection_capture_from_marquee(const DrawingProgramDocument *document,
                                                   const DrawingProgramLayerRasterStore *layer_rasters,
                                                   uint32_t active_layer_id,
                                                   DrawingProgramSelectionState *selection);
int drawing_program_selection_add_from_marquee(const DrawingProgramDocument *document,
                                               const DrawingProgramLayerRasterStore *layer_rasters,
                                               uint32_t active_layer_id,
                                               DrawingProgramSelectionState *selection);
int drawing_program_selection_subtract_from_marquee(const DrawingProgramDocument *document,
                                                    const DrawingProgramLayerRasterStore *layer_rasters,
                                                    uint32_t active_layer_id,
                                                    DrawingProgramSelectionState *selection);
int drawing_program_selection_contains_sample(const DrawingProgramSelectionState *selection,
                                              uint32_t sample_x,
                                              uint32_t sample_y);
int drawing_program_selection_begin_move(const DrawingProgramSelectionState *selection,
                                         uint32_t sample_x,
                                         uint32_t sample_y);
void drawing_program_selection_begin_marquee(DrawingProgramSelectionState *selection,
                                             uint32_t sample_x,
                                             uint32_t sample_y);
void drawing_program_selection_begin_move_tracking(DrawingProgramSelectionState *selection,
                                                   uint32_t sample_x,
                                                   uint32_t sample_y);
void drawing_program_selection_update_move_offset(DrawingProgramSelectionState *selection,
                                                  uint32_t sample_x,
                                                  uint32_t sample_y);
CoreResult drawing_program_selection_commit_move(DrawingProgramDocument *document,
                                                 DrawingProgramLayerRasterStore *layer_rasters,
                                                 uint32_t active_layer_id,
                                                 DrawingProgramHistory *history,
                                                 DrawingProgramSelectionState *selection);
CoreResult drawing_program_selection_delete_payload(DrawingProgramDocument *document,
                                                    DrawingProgramLayerRasterStore *layer_rasters,
                                                    uint32_t active_layer_id,
                                                    DrawingProgramHistory *history,
                                                    DrawingProgramSelectionState *selection);
int drawing_program_selection_copy_payload(const DrawingProgramSelectionState *selection,
                                           DrawingProgramClipboardState *clipboard);
CoreResult drawing_program_selection_cut_to_clipboard(DrawingProgramDocument *document,
                                                      DrawingProgramLayerRasterStore *layer_rasters,
                                                      uint32_t active_layer_id,
                                                      DrawingProgramHistory *history,
                                                      DrawingProgramSelectionState *selection,
                                                      DrawingProgramClipboardState *clipboard);
CoreResult drawing_program_selection_paste_from_clipboard(DrawingProgramDocument *document,
                                                          DrawingProgramLayerRasterStore *layer_rasters,
                                                          uint32_t active_layer_id,
                                                          DrawingProgramHistory *history,
                                                          DrawingProgramSelectionState *selection,
                                                          const DrawingProgramClipboardState *clipboard,
                                                          int32_t origin_x,
                                                          int32_t origin_y);
int drawing_program_selection_select_all(const DrawingProgramDocument *document,
                                         const DrawingProgramLayerRasterStore *layer_rasters,
                                         uint32_t active_layer_id,
                                         DrawingProgramSelectionState *selection);
const char *drawing_program_selection_scope_policy_label(void);

#ifdef __cplusplus
}
#endif

#endif
