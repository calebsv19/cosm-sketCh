#ifndef DRAWING_PROGRAM_UI_PILOT_PROBE_H
#define DRAWING_PROGRAM_UI_PILOT_PROBE_H
#include "drawing_program_app_main.h"
#include <SDL2/SDL.h>
/* Opt-in bounded qualification; no routine automation policy. */
int drawing_program_ui_pilot_probe(SDL_Window *window, SDL_Renderer *renderer,
                                   const DrawingProgramAppContext *app);
int drawing_program_pane_header_probe(SDL_Window *window, SDL_Renderer *renderer,
                                      const DrawingProgramAppContext *app);
int drawing_program_splitter_probe(SDL_Window *window, SDL_Renderer *renderer,
                                   const DrawingProgramAppContext *app);
int drawing_program_pane_lifecycle_probe(SDL_Window *window, SDL_Renderer *renderer,
                                        DrawingProgramAppContext *app);
#endif
