#include "drawing_program/drawing_program_visual_input_core.h"

#include <math.h>

#include "drawing_program/drawing_program_viewport.h"

static int point_in_rect(SDL_Rect r, int x, int y) {
    return x >= r.x && y >= r.y && x < (r.x + r.w) && y < (r.y + r.h);
}

static DrawingProgramViewportFrame drawing_program_viewport_frame_from_rect(SDL_Rect rect) {
    DrawingProgramViewportFrame frame;
    frame.x = (float)rect.x;
    frame.y = (float)rect.y;
    frame.width = (float)rect.w;
    frame.height = (float)rect.h;
    return frame;
}

static void map_input_to_render_coords(SDL_Window *window,
                                       SDL_Renderer *renderer,
                                       int input_x,
                                       int input_y,
                                       int *out_x,
                                       int *out_y) {
    int window_w = 0;
    int window_h = 0;
    int render_w = 0;
    int render_h = 0;
    float sx = 1.0f;
    float sy = 1.0f;
    int mapped_x = input_x;
    int mapped_y = input_y;

    if (renderer &&
        SDL_GetRendererOutputSize(renderer, &render_w, &render_h) == 0 &&
        render_w > 0 && render_h > 0 &&
        window) {
        SDL_GetWindowSize(window, &window_w, &window_h);
    }
    if (render_w > 0 && render_h > 0 && window_w > 0 && window_h > 0) {
        sx = (float)render_w / (float)window_w;
        sy = (float)render_h / (float)window_h;
        mapped_x = (int)((float)input_x * sx);
        mapped_y = (int)((float)input_y * sy);
    }
    if (out_x) {
        *out_x = mapped_x;
    }
    if (out_y) {
        *out_y = mapped_y;
    }
}

void drawing_program_visual_input_map_event_position(SDL_Window *window,
                                                     SDL_Renderer *renderer,
                                                     const SDL_Event *event,
                                                     int *out_x,
                                                     int *out_y,
                                                     int *out_has_position) {
    int has_position = 0;
    int event_x = 0;
    int event_y = 0;

    if (!event) {
        if (out_has_position) {
            *out_has_position = 0;
        }
        return;
    }

    if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) {
        has_position = 1;
        map_input_to_render_coords(window, renderer, event->button.x, event->button.y, &event_x, &event_y);
    } else if (event->type == SDL_MOUSEMOTION) {
        has_position = 1;
        map_input_to_render_coords(window, renderer, event->motion.x, event->motion.y, &event_x, &event_y);
    }

    if (out_x) {
        *out_x = event_x;
    }
    if (out_y) {
        *out_y = event_y;
    }
    if (out_has_position) {
        *out_has_position = has_position;
    }
}

DrawingProgramVisualPaneHitState drawing_program_visual_input_classify_hit(int x,
                                                                           int y,
                                                                           int has_left_pane,
                                                                           SDL_Rect left_pane,
                                                                           int has_right_pane,
                                                                           SDL_Rect right_pane,
                                                                           int has_canvas_pane,
                                                                           SDL_Rect canvas_pane) {
    DrawingProgramVisualPaneHitState hit = {0, 0, 0};
    hit.on_left = has_left_pane && point_in_rect(left_pane, x, y);
    hit.on_right = has_right_pane && point_in_rect(right_pane, x, y);
    hit.on_canvas = has_canvas_pane && point_in_rect(canvas_pane, x, y);
    return hit;
}

void drawing_program_visual_input_apply_mouse_wheel_zoom(DrawingProgramAppContext *ctx,
                                                         SDL_Window *window,
                                                         SDL_Renderer *renderer,
                                                         int has_canvas_pane,
                                                         SDL_Rect canvas_pane,
                                                         const SDL_Event *event) {
    int mx = 0;
    int my = 0;
    float zoom_factor;
    if (!ctx || !event || event->type != SDL_MOUSEWHEEL || !has_canvas_pane) {
        return;
    }
    SDL_GetMouseState(&mx, &my);
    map_input_to_render_coords(window, renderer, mx, my, &mx, &my);
    if (!point_in_rect(canvas_pane, mx, my)) {
        return;
    }
    if (event->wheel.y == 0) {
        return;
    }
    zoom_factor = powf(1.1f, (float)event->wheel.y);
    (void)drawing_program_viewport_zoom_at_screen_anchor_in_frame(&ctx->editor.viewport,
                                                                  &ctx->document,
                                                                  drawing_program_viewport_frame_from_rect(canvas_pane),
                                                                  (float)mx,
                                                                  (float)my,
                                                                  zoom_factor);
}

void drawing_program_visual_input_window_event_flags(const SDL_Event *event,
                                                     uint8_t *out_clear_mouse_known,
                                                     uint8_t *out_cancel_transients) {
    uint8_t clear_mouse_known = 0u;
    uint8_t cancel_transients = 0u;
    if (event && event->type == SDL_WINDOWEVENT) {
        if (event->window.event == SDL_WINDOWEVENT_LEAVE) {
            clear_mouse_known = 1u;
        } else if (event->window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            clear_mouse_known = 1u;
            cancel_transients = 1u;
        }
    }
    if (out_clear_mouse_known) {
        *out_clear_mouse_known = clear_mouse_known;
    }
    if (out_cancel_transients) {
        *out_cancel_transients = cancel_transients;
    }
}

DrawingProgramVisualMouseDownAction drawing_program_visual_input_classify_mouse_down(uint8_t button,
                                                                                     DrawingProgramVisualPaneHitState hit) {
    if (button == SDL_BUTTON_RIGHT && hit.on_canvas) {
        return DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_START_PAN;
    }
    if (button == SDL_BUTTON_LEFT && hit.on_left) {
        return DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_HANDLE_LEFT_PANEL;
    }
    if (button == SDL_BUTTON_LEFT && hit.on_right) {
        return DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_HANDLE_RIGHT_PANEL;
    }
    if (button == SDL_BUTTON_LEFT && !hit.on_left && !hit.on_right && !hit.on_canvas) {
        return DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_CLEAR_OUTSIDE_PANES;
    }
    if (button == SDL_BUTTON_LEFT && hit.on_canvas) {
        return DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_CANVAS_LEFT;
    }
    return DRAWING_PROGRAM_VISUAL_MOUSE_DOWN_ACTION_NONE;
}

DrawingProgramVisualMouseUpAction drawing_program_visual_input_classify_mouse_up(uint8_t button) {
    if (button == SDL_BUTTON_LEFT) {
        return DRAWING_PROGRAM_VISUAL_MOUSE_UP_ACTION_LEFT_RELEASE;
    }
    if (button == SDL_BUTTON_RIGHT) {
        return DRAWING_PROGRAM_VISUAL_MOUSE_UP_ACTION_RIGHT_RELEASE;
    }
    return DRAWING_PROGRAM_VISUAL_MOUSE_UP_ACTION_NONE;
}

void drawing_program_visual_input_resolve_button_coords(const SDL_Event *event,
                                                        int event_has_position,
                                                        int event_x,
                                                        int event_y,
                                                        int *out_x,
                                                        int *out_y) {
    int x = event_x;
    int y = event_y;
    if (!event_has_position && event) {
        x = event->button.x;
        y = event->button.y;
    }
    if (out_x) {
        *out_x = x;
    }
    if (out_y) {
        *out_y = y;
    }
}

void drawing_program_visual_input_begin_pan(int click_x,
                                            int click_y,
                                            uint8_t *io_panning_active,
                                            int *io_last_mouse_x,
                                            int *io_last_mouse_y) {
    if (io_panning_active) {
        *io_panning_active = 1u;
    }
    if (io_last_mouse_x) {
        *io_last_mouse_x = click_x;
    }
    if (io_last_mouse_y) {
        *io_last_mouse_y = click_y;
    }
}

int drawing_program_visual_input_apply_pan_motion(DrawingProgramAppContext *ctx,
                                                  uint8_t panning_active,
                                                  SDL_Rect canvas_pane,
                                                  int mouse_x,
                                                  int mouse_y,
                                                  int *io_last_mouse_x,
                                                  int *io_last_mouse_y) {
    int dx;
    int dy;
    if (!ctx || !panning_active || !io_last_mouse_x || !io_last_mouse_y) {
        return 0;
    }
    dx = mouse_x - *io_last_mouse_x;
    dy = mouse_y - *io_last_mouse_y;
    (void)drawing_program_viewport_pan_in_frame(&ctx->editor.viewport,
                                                &ctx->document,
                                                drawing_program_viewport_frame_from_rect(canvas_pane),
                                                (float)dx,
                                                (float)dy);
    *io_last_mouse_x = mouse_x;
    *io_last_mouse_y = mouse_y;
    return 1;
}
