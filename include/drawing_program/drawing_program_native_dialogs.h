#ifndef DRAWING_PROGRAM_NATIVE_DIALOGS_H
#define DRAWING_PROGRAM_NATIVE_DIALOGS_H

#include <stddef.h>

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DrawingProgramNativeDialogKind {
    DRAWING_PROGRAM_NATIVE_DIALOG_DIRECTORY,
    DRAWING_PROGRAM_NATIVE_DIALOG_OPEN,
    DRAWING_PROGRAM_NATIVE_DIALOG_SAVE
} DrawingProgramNativeDialogKind;

typedef CoreResult (*DrawingProgramNativeDialogPicker)(DrawingProgramNativeDialogKind kind,
                                                      const char *prompt,
                                                      const char *initial_dir,
                                                      const char *default_name,
                                                      char *out_path,
                                                      size_t out_cap);

/* The desktop host binds its picker on the UI thread. A successful chooser
 * supplies a file-system path. CORE_ERR_NOT_FOUND means the user canceled. */
void drawing_program_native_dialog_set_picker(DrawingProgramNativeDialogPicker picker);

#if defined(__APPLE__)
void drawing_program_native_dialog_bind_mac(void);
#endif

CoreResult drawing_program_native_choose_directory(const char *prompt,
                                                   const char *initial_dir,
                                                   char *out_path,
                                                   size_t out_cap);
CoreResult drawing_program_native_choose_file(const char *prompt,
                                              const char *initial_dir,
                                              char *out_path,
                                              size_t out_cap);
CoreResult drawing_program_native_choose_save_file(const char *prompt,
                                                   const char *initial_dir,
                                                   const char *default_name,
                                                   char *out_path,
                                                   size_t out_cap);

#ifdef __cplusplus
}
#endif

#endif
