#ifndef DRAWING_PROGRAM_VISUAL_INPUT_CORE_H
#define DRAWING_PROGRAM_VISUAL_INPUT_CORE_H

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_app_main.h"

typedef struct DrawingProgramVisualPaneHitState {
    int on_left;
    int on_right;
    int on_canvas;
} DrawingProgramVisualPaneHitState;

typedef enum DrawingProgramVisualMouseDownAction {
    DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_NONE = 0,
    DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_START_PAN,
    DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_HANDLE_LEFT_PANEL,
    DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_HANDLE_RIGHT_PANEL,
    DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_CLEAR_OUTSIDE_PANES,
    DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_CANVAS_LEFT
} DrawingProgramVisualMouseDownAction;

typedef enum DrawingProgramVisualMouseUpAction {
    DRAWING_PROGRAM_VISUAL_MOUSE_UP_ACTION_NONE = 0,
    DRAWING_PROGRAM_VISUAL_MOUSE_UP_ACTION_LEFT_RELEASE,
    DRAWING_PROGRAM_VISUAL_MOUSE_UP_ACTION_RIGHT_RELEASE
} DrawingProgramVisualMouseUpAction;

void drawing_program_visual_input_map_event_position(SDL_Window *window,
                                                     SDL_Renderer *renderer,
                                                     const SDL_Event *event,
                                                     int *out_x,
                                                     int *out_y,
                                                     int *out_has_position);

DrawingProgramVisualPaneHitState drawing_program_visual_input_classify_hit(int x,
                                                                           int y,
                                                                           int has_left_pane,
                                                                           SDL_Rect left_pane,
                                                                           int has_right_pane,
                                                                           SDL_Rect right_pane,
                                                                           int has_canvas_pane,
                                                                           SDL_Rect canvas_pane);

void drawing_program_visual_input_apply_mouse_wheel_zoom(DrawingProgramAppContext *ctx,
                                                         SDL_Window *window,
                                                         SDL_Renderer *renderer,
                                                         int has_canvas_pane,
                                                         SDL_Rect canvas_pane,
                                                         const SDL_Event *event);

void drawing_program_visual_input_window_event_flags(const SDL_Event *event,
                                                     uint8_t *out_clear_mouse_known,
                                                     uint8_t *out_cancel_transients);

DrawingProgramVisualMouseDownAction drawing_program_visual_input_classify_mouse_down(uint8_t button,
                                                                                     DrawingProgramVisualPaneHitState hit);
DrawingProgramVisualMouseUpAction drawing_program_visual_input_classify_mouse_up(uint8_t button);

void drawing_program_visual_input_resolve_button_coords(const SDL_Event *event,
                                                        int event_has_position,
                                                        int event_x,
                                                        int event_y,
                                                        int *out_x,
                                                        int *out_y);
void drawing_program_visual_input_begin_pan(int click_x,
                                            int click_y,
                                            uint8_t *io_panning_active,
                                            int *io_last_mouse_x,
                                            int *io_last_mouse_y);
int drawing_program_visual_input_apply_pan_motion(DrawingProgramAppContext *ctx,
                                                  uint8_t panning_active,
                                                  SDL_Rect canvas_pane,
                                                  int mouse_x,
                                                  int mouse_y,
                                                  int *io_last_mouse_x,
                                                  int *io_last_mouse_y);

#endif
