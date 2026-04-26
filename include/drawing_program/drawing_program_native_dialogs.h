#ifndef DRAWING_PROGRAM_NATIVE_DIALOGS_H
#define DRAWING_PROGRAM_NATIVE_DIALOGS_H

#include <stddef.h>

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
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
