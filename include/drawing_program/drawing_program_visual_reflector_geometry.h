#ifndef DRAWING_PROGRAM_VISUAL_REFLECTOR_GEOMETRY_H
#define DRAWING_PROGRAM_VISUAL_REFLECTOR_GEOMETRY_H

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_reflection_state.h"
#include "drawing_program/drawing_program_visual_state.h"

#ifdef __cplusplus
extern "C" {
#endif

int drawing_program_visual_reflector_screen_line(const VisualCanvasSheetMetrics *metrics,
                                                 const DrawingProgramReflectorLine *line,
                                                 SDL_Point *out_start,
                                                 SDL_Point *out_end);
int drawing_program_visual_reflector_screen_handles(const VisualCanvasSheetMetrics *metrics,
                                                    const DrawingProgramReflectorLine *line,
                                                    SDL_Point *out_anchor,
                                                    SDL_Point *out_direction);

#ifdef __cplusplus
}
#endif

#endif
