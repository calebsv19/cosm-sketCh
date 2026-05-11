#ifndef DRAWING_PROGRAM_VISUAL_RIGHT_PANEL_DEFS_H
#define DRAWING_PROGRAM_VISUAL_RIGHT_PANEL_DEFS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum VisualRightPanelSlot {
    VISUAL_RIGHT_PANEL_SLOT_CANVAS = 0,
    VISUAL_RIGHT_PANEL_SLOT_LAYER = 1,
    VISUAL_RIGHT_PANEL_SLOT_COLOR = 2,
    VISUAL_RIGHT_PANEL_SLOT_FILE = 3,
    VISUAL_RIGHT_PANEL_SLOT_ASSET = 4,
    VISUAL_RIGHT_PANEL_SLOT_EXPORT = 5,
    VISUAL_RIGHT_PANEL_SLOT_COUNT = 6
} VisualRightPanelSlot;

#ifdef __cplusplus
}
#endif

#endif
