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

#define DRAWING_PROGRAM_PATH_DRAFT_PREVIEW_FLAT_MAX_POINTS (DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS * 24u)

typedef struct VisualCanvasInteractionState {
    uint8_t drawing_active;
    uint8_t panning_active;
    uint8_t shape_active;
    uint8_t shape_tool;
    uint8_t path_draft_active;
    uint8_t path_draft_closed;
    uint8_t path_draft_drag_active;
    uint8_t path_draft_drag_reserved0;
    uint8_t transform_active;
    uint8_t transform_kind;
    uint8_t move_axis_lock;
    uint8_t marquee_commit_mode;
    uint8_t has_last_sample;
    uint32_t last_sample_x;
    uint32_t last_sample_y;
    uint32_t shape_start_sample_x;
    uint32_t shape_start_sample_y;
    uint32_t path_preview_sample_x;
    uint32_t path_preview_sample_y;
    uint16_t path_draft_point_count;
    uint16_t path_draft_drag_point_index;
    DrawingProgramPathPoint path_draft_points[DRAWING_PROGRAM_OBJECT_PATH_MAX_POINTS];
    uint8_t path_preview_flatten_dirty;
    uint8_t path_preview_flatten_valid;
    uint16_t path_preview_flatten_point_count;
    double path_preview_flatten_xy[DRAWING_PROGRAM_PATH_DRAFT_PREVIEW_FLAT_MAX_POINTS * 2u];
    uint8_t object_move_active;
    uint32_t object_move_anchor_sample_x;
    uint32_t object_move_anchor_sample_y;
    int32_t object_move_offset_x;
    int32_t object_move_offset_y;
    uint8_t object_path_point_move_active;
    uint16_t object_path_point_index;
    uint16_t object_path_point_reserved0;
    uint32_t object_path_point_object_id;
    uint32_t object_path_point_anchor_sample_x;
    uint32_t object_path_point_anchor_sample_y;
    int32_t object_path_point_offset_x;
    int32_t object_path_point_offset_y;
    uint8_t object_path_handle_move_active;
    uint8_t object_path_handle_kind;
    uint16_t object_path_handle_point_index;
    uint32_t object_path_handle_object_id;
    int32_t object_path_handle_dx;
    int32_t object_path_handle_dy;
    int last_mouse_x;
    int last_mouse_y;
} VisualCanvasInteractionState;

typedef struct VisualPanelUiState {
    uint8_t left_slot;
    uint8_t right_slot;
    uint8_t mouse_known;
    uint8_t object_color_target_kind;
    int mouse_x;
    int mouse_y;
    uint32_t object_color_target_object_id;
} VisualPanelUiState;

typedef enum VisualObjectColorTargetKind {
    VISUAL_OBJECT_COLOR_TARGET_NONE = 0,
    VISUAL_OBJECT_COLOR_TARGET_STROKE = 1,
    VISUAL_OBJECT_COLOR_TARGET_FILL = 2
} VisualObjectColorTargetKind;

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
