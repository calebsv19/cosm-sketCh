#include <string.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_vulkan_rollout.h"
#include "drawing_program_app_visual_runtime.h"

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

int drawing_program_app_visual_main(int argc, char **argv) {
    if (has_flag(argc, argv, "--headless")) {
        return drawing_program_app_main(argc, argv);
    }
    if (has_flag(argc, argv, "--vulkan-rollout-self-test")) {
        return drawing_program_vulkan_rollout_self_test();
    }
    return drawing_program_app_visual_run_mode(argc, argv);
}
