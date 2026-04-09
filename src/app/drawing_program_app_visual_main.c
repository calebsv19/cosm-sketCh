#include <SDL2/SDL.h>

#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_app_main.h"

static int has_flag(int argc, char **argv, const char *flag) {
    int i;
    if (!argv || !flag) {
        return 0;
    }
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], flag) == 0) {
            return 1;
        }
    }
    return 0;
}

static int run_visual_mode(int argc, char **argv) {
    CoreResult result;
    DrawingProgramAppContext app;
    SDL_Window *window = 0;
    int quit = 0;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "drawing_program: SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("sketCh",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              1280,
                              800,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        fprintf(stderr, "drawing_program: SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    result = drawing_program_app_bootstrap(&app, argc, argv);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: bootstrap failed: %s\n", result.message);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    result = drawing_program_app_config_load(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: config_load failed: %s\n", result.message);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    result = drawing_program_app_state_seed(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: state_seed failed: %s\n", result.message);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    result = drawing_program_app_subsystems_init(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: subsystems_init failed: %s\n", result.message);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    result = drawing_program_runtime_start(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: runtime_start failed: %s\n", result.message);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                quit = 1;
            }
        }

        result = drawing_program_app_run_loop(&app);
        if (result.code != CORE_OK) {
            fprintf(stderr, "drawing_program: run_loop failed: %s\n", result.message);
            break;
        }
        SDL_Delay(16);
    }

    result = drawing_program_app_shutdown(&app);
    if (result.code != CORE_OK) {
        fprintf(stderr, "drawing_program: shutdown failed: %s\n", result.message);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return (result.code == CORE_OK) ? 0 : 1;
}

int drawing_program_app_visual_main(int argc, char **argv) {
    if (has_flag(argc, argv, "--headless")) {
        return drawing_program_app_main(argc, argv);
    }
    return run_visual_mode(argc, argv);
}
