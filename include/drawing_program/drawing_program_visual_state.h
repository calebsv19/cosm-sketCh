#ifndef DRAWING_PROGRAM_VISUAL_STATE_H
#define DRAWING_PROGRAM_VISUAL_STATE_H

#include <stdint.h>
#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_app_main.h"

typedef DrawingProgramSelectionState VisualSelectionState;

typedef struct VisualCanvasSheetMetrics {
    SDL_Rect sheet_rect;
    float pixel_size;
} VisualCanvasSheetMetrics;

typedef struct VisualCanvasInteractionState {
    uint8_t drawing_active;
    uint8_t panning_active;
    uint8_t shape_active;
    uint8_t shape_tool;
    uint8_t transform_active;
    uint8_t transform_kind;
    uint8_t move_axis_lock;
    uint8_t marquee_commit_mode;
    uint8_t has_last_sample;
    uint32_t last_sample_x;
    uint32_t last_sample_y;
    uint32_t shape_start_sample_x;
    uint32_t shape_start_sample_y;
    int last_mouse_x;
    int last_mouse_y;
} VisualCanvasInteractionState;

typedef struct VisualPanelUiState {
    uint8_t left_slot;
    uint8_t right_slot;
    uint8_t mouse_known;
    int mouse_x;
    int mouse_y;
} VisualPanelUiState;

typedef enum VisualMarqueeCommitMode {
    VISUAL_MARQUEE_COMMIT_REPLACE = 0,
    VISUAL_MARQUEE_COMMIT_ADD = 1,
    VISUAL_MARQUEE_COMMIT_SUBTRACT = 2
} VisualMarqueeCommitMode;

typedef enum VisualTransformSessionKind {
    VISUAL_TRANSFORM_SESSION_NONE = 0,
    VISUAL_TRANSFORM_SESSION_MOVE = 1
} VisualTransformSessionKind;

#endif
