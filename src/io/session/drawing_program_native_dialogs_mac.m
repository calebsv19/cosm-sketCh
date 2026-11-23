#import <AppKit/AppKit.h>

#include <SDL2/SDL.h>

#include "drawing_program/drawing_program_native_dialogs.h"

static CoreResult pick_on_mac(DrawingProgramNativeDialogKind kind,
                              const char *prompt,
                              const char *initial_dir,
                              const char *default_name,
                              char *out_path,
                              size_t out_cap) {
    if (!out_path || out_cap == 0) {
        return (CoreResult){CORE_ERR_INVALID_ARG, "missing native picker output"};
    }
    out_path[0] = '\0';
    if (!prompt || (kind == DRAWING_PROGRAM_NATIVE_DIALOG_SAVE && !default_name) ||
        ![NSThread isMainThread] || !NSApp) {
        return (CoreResult){CORE_ERR_INVALID_ARG, "native picker requires the main Cocoa thread"};
    }

    @autoreleasepool {
        NSString *title = [NSString stringWithUTF8String:prompt];
        NSSavePanel *panel;
        if (kind == DRAWING_PROGRAM_NATIVE_DIALOG_SAVE) {
            NSString *name = [NSString stringWithUTF8String:default_name];
            if (!name) {
                return (CoreResult){CORE_ERR_INVALID_ARG, "invalid native save name"};
            }
            panel = [NSSavePanel savePanel];
            panel.nameFieldStringValue = name;
        } else {
            NSOpenPanel *open_panel = [NSOpenPanel openPanel];
            open_panel.canChooseFiles = kind == DRAWING_PROGRAM_NATIVE_DIALOG_OPEN;
            open_panel.canChooseDirectories = kind == DRAWING_PROGRAM_NATIVE_DIALOG_DIRECTORY;
            open_panel.allowsMultipleSelection = NO;
            panel = open_panel;
        }
        if (!panel || !title) {
            return (CoreResult){CORE_ERR_IO, "could not create the Cocoa picker"};
        }
        panel.title = title;
        panel.message = title;
        if (initial_dir && initial_dir[0]) {
            panel.directoryURL = [NSURL fileURLWithFileSystemRepresentation:initial_dir
                                                                 isDirectory:YES
                                                                relativeToURL:nil];
        }

        NSWindow *editor_window = NSApp.keyWindow;
        if (!editor_window) {
            editor_window = NSApp.mainWindow;
        }
        const NSModalResponse response = [panel runModal];
        if (editor_window.isVisible) {
            [editor_window makeKeyWindow];
        }
        /* AppKit takes input while the panel is active. Queued presses/releases
         * belong to that scope. Retain SDL window and quit notifications. */
        SDL_FlushEvents(SDL_KEYDOWN, SDL_TEXTINPUT);
        SDL_FlushEvents(SDL_MOUSEMOTION, SDL_MOUSEWHEEL);
        if (response == NSModalResponseCancel) {
            return (CoreResult){CORE_ERR_NOT_FOUND, "native picker cancelled"};
        }
        if (response != NSModalResponseOK ||
            !panel.URL.isFileURL ||
            ![panel.URL getFileSystemRepresentation:out_path maxLength:out_cap]) {
            out_path[0] = '\0';
            return (CoreResult){CORE_ERR_IO, "native picker did not return a usable path"};
        }
        return core_result_ok();
    }
}

void drawing_program_native_dialog_bind_mac(void) {
    drawing_program_native_dialog_set_picker(pick_on_mac);
}
