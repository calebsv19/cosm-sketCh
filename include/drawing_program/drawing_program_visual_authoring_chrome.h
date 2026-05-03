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
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CANCEL = 2,
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CYCLE_OVERLAY = 3,
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_ZOOM_DEC = 4,
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_ZOOM_INC = 5,
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_ZOOM_RESET = 6,
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_PRESET_DAW = 7,
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_PRESET_IDE = 8,
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_FONT_PRESET_CUSTOM = 9,
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_DAW = 10,
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_STANDARD_GREY = 11,
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_MIDNIGHT = 12,
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_SOFT_LIGHT = 13,
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_THEME_PRESET_GREYSCALE = 14,
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CUSTOM_THEME_CREATE = 15,
    DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CUSTOM_THEME_EDIT = 16
} DrawingProgramAuthoringChromeAction;

uint32_t drawing_program_visual_authoring_chrome_build_pane_rows(
    const DrawingProgramAppContext *ctx,
    char rows[][DRAWING_PROGRAM_AUTHORING_CHROME_ROW_TEXT_MAX],
    uint32_t row_capacity);

DrawingProgramAuthoringChromeAction drawing_program_visual_authoring_chrome_hit_test(
    int viewport_width,
    int viewport_height,
    const DrawingProgramAppContext *ctx,
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
