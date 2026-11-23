#include "drawing_program/drawing_program_native_dialogs.h"

#include "drawing_program/drawing_program_app_main.h"

int main(int argc, char **argv) {
#if defined(__APPLE__)
    drawing_program_native_dialog_bind_mac();
#endif
    return drawing_program_app_visual_main(argc, argv);
}
