#ifndef DRAWING_PROGRAM_VISUAL_AUTHORING_CHROME_H
#define DRAWING_PROGRAM_VISUAL_AUTHORING_CHROME_H

#include <stdint.h>

#include <SDL2/SDL.h>

#include "core_theme.h"
#include "drawing_program/drawing_program_app_main.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    DRAWING_PROGRAM_AUTHORING_CHROME_ROW_TEXT_MAX = 96
};

typedef enum DrawingProgramAuthoringChromeAction {
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_NONE = 0,
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_APPLY = 1,
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CANCEL = 2
} DrawingProgramAuthoringChromeAction;

uint32_t drawing_program_visual_authoring_chrome_build_pane_rows(
    const DrawingProgramAppContext *ctx,
    char rows[][DRAWING_PROGRAM_AUTHORING_CHROME_ROW_TEXT_MAX],
    uint32_t row_capacity);

DrawingProgramAuthoringChromeAction drawing_program_visual_authoring_chrome_hit_test(
    int viewport_width,
    int viewport_height,
    int point_x,
    int point_y);

void drawing_program_visual_authoring_chrome_draw(SDL_Renderer *renderer,
                                                  int viewport_width,
                                                  int viewport_height,
                                                  const DrawingProgramAppContext *ctx,
                                                  const CoreThemePreset *theme);

#ifdef __cplusplus
}
#endif

#endif
