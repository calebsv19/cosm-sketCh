#ifndef DRAWING_PROGRAM_OBJECT_RASTERIZE_H
#define DRAWING_PROGRAM_OBJECT_RASTERIZE_H

#include <stdint.h>

#include "core_base.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_history.h"
#include "drawing_program/drawing_program_layer_raster.h"
#include "drawing_program/drawing_program_object_store.h"

#ifdef __cplusplus
extern "C" {
#endif

CoreResult drawing_program_object_rasterize_selection_to_layer(
    DrawingProgramDocument *document,
    DrawingProgramLayerRasterStore *layer_rasters,
    DrawingProgramHistory *history,
    DrawingProgramObjectStore *object_store,
    DrawingProgramObjectSelectionState *selection,
    uint32_t target_layer_id,
    uint32_t *out_rasterized_count);

#ifdef __cplusplus
}
#endif

#endif
