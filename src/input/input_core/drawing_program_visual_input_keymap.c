#include "drawing_program/drawing_program_visual_input_keymap.h"

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_ui_color_state.h"

DrawingProgramWorkflowControl drawing_program_visual_input_control_for_key(SDL_Keycode key, SDL_Keymod mod) {
    switch (key) {
        case SDLK_b:
            return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_BRUSH;
        case SDLK_e:
            return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_ERASER;
        case SDLK_f:
            return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_FILL;
        case SDLK_l:
            return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_LINE;
        case SDLK_r:
            return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_RECT;
        case SDLK_c:
            return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_CIRCLE;
        case SDLK_s:
            return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_SELECT;
        case SDLK_m:
            return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_MOVE;
        case SDLK_i:
            return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_PICKER;
        case SDLK_p:
            return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_PATH;
        case SDLK_v:
            return DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_VISIBILITY;
        case SDLK_k:
            return DRAWING_PROGRAM_WORKFLOW_CONTROL_TOGGLE_ACTIVE_LAYER_LOCK;
        case SDLK_SPACE:
            return DRAWING_PROGRAM_WORKFLOW_CONTROL_STAMP_CENTER_SAMPLE;
        case SDLK_z:
            return (mod & KMOD_SHIFT)
                       ? DRAWING_PROGRAM_WORKFLOW_CONTROL_REDO
                       : DRAWING_PROGRAM_WORKFLOW_CONTROL_UNDO;
        case SDLK_y:
            return DRAWING_PROGRAM_WORKFLOW_CONTROL_REDO;
        default:
            return DRAWING_PROGRAM_WORKFLOW_CONTROL_NONE;
    }
}

int drawing_program_visual_input_try_apply_palette_key(DrawingProgramAppContext *ctx, SDL_Keycode key) {
    int color;
    uint8_t selected;
    if (!ctx) {
        return 0;
    }
    if (key == SDLK_LEFTBRACKET || key == SDLK_RIGHTBRACKET) {
        color = (int)drawing_program_color_index_clamp(ctx->ui.active_color_index);
        if (key == SDLK_LEFTBRACKET) {
            color -= 1;
        } else {
            color += 1;
        }
        if (color < 0) {
            color = (int)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT - 1;
        }
        if (color >= (int)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT) {
            color = 0;
        }
        drawing_program_ui_color_load_active_paint_from_swatch(ctx, (uint8_t)color);
        return 1;
    }
    if (key >= SDLK_1 && key <= SDLK_8) {
        selected = (uint8_t)(key - SDLK_1);
        if (selected < (uint8_t)DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT) {
            drawing_program_ui_color_load_active_paint_from_swatch(ctx, selected);
            return 1;
        }
    }
    return 0;
}

int drawing_program_visual_input_try_module_slot_hotkey(int ctrl_or_cmd,
                                                        int shift,
                                                        SDL_Keycode key,
                                                        uint32_t *out_module_type_id) {
    if (!out_module_type_id || !ctrl_or_cmd || !shift) {
        return 0;
    }
    if (key >= SDLK_1 && key <= SDLK_4) {
        *out_module_type_id = (uint32_t)(key - SDLK_1) + 1u;
        return 1;
    }
    return 0;
}

int drawing_program_visual_input_try_font_zoom_hotkey(int ctrl_or_cmd,
                                                      SDL_Keycode key,
                                                      int current_step,
                                                      int *out_step) {
    int step = current_step;
    if (!out_step || !ctrl_or_cmd) {
        return 0;
    }
    if (key == SDLK_EQUALS || key == SDLK_PLUS || key == SDLK_KP_PLUS) {
        step += 1;
    } else if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
        step -= 1;
    } else if (key == SDLK_0 || key == SDLK_KP_0) {
        step = 0;
    } else {
        return 0;
    }
    *out_step = step;
    return 1;
}

int drawing_program_visual_input_try_theme_cycle_hotkey(int ctrl_or_cmd,
                                                        int shift,
                                                        SDL_Keycode key,
                                                        int *out_direction) {
    if (!out_direction || !ctrl_or_cmd || !shift) {
        return 0;
    }
    if (key == SDLK_t) {
        *out_direction = 1;
        return 1;
    }
    if (key == SDLK_y) {
        *out_direction = -1;
        return 1;
    }
    return 0;
}

int drawing_program_visual_input_try_move_nudge_key(SDL_Keycode key,
                                                    int shift,
                                                    int32_t *out_dx,
                                                    int32_t *out_dy) {
    int32_t nudge_step = shift ? 10 : 1;
    int32_t dx = 0;
    int32_t dy = 0;
    if (!out_dx || !out_dy) {
        return 0;
    }
    switch (key) {
        case SDLK_UP: dy = -nudge_step; break;
        case SDLK_DOWN: dy = nudge_step; break;
        case SDLK_LEFT: dx = -nudge_step; break;
        case SDLK_RIGHT: dx = nudge_step; break;
        default: return 0;
    }
    *out_dx = dx;
    *out_dy = dy;
    return 1;
}
