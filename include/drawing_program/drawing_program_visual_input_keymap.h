#ifndef DRAWING_PROGRAM_VISUAL_INPUT_KEYMAP_H
#define DRAWING_PROGRAM_VISUAL_INPUT_KEYMAP_H

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_runtime_orchestration.h"

DrawingProgramWorkflowControl drawing_program_visual_input_control_for_key(SDL_Keycode key, SDL_Keymod mod);
int drawing_program_visual_input_try_apply_palette_key(DrawingProgramAppContext *ctx, SDL_Keycode key);
int drawing_program_visual_input_try_module_slot_hotkey(int ctrl_or_cmd,
                                                        int shift,
                                                        int alt,
                                                        SDL_Keycode key,
                                                        uint32_t *out_module_type_id);
int drawing_program_visual_input_try_font_zoom_hotkey(int ctrl_or_cmd,
                                                      SDL_Keycode key,
                                                      int current_step,
                                                      int *out_step);
int drawing_program_visual_input_try_theme_cycle_hotkey(int ctrl_or_cmd,
                                                        int shift,
                                                        SDL_Keycode key,
                                                        int *out_direction);
int drawing_program_visual_input_try_move_nudge_key(SDL_Keycode key,
                                                    int shift,
                                                    int32_t *out_dx,
                                                    int32_t *out_dy);

#endif
