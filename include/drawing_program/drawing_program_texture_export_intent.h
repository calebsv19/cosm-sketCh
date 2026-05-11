#ifndef DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_H
#define DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DrawingProgramTextureExportIntentKind {
    DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_NONE = 0u,
    DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_FLATTENED_ONLY = 1u,
    DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_BASE_PLUS_OVERLAY = 2u
} DrawingProgramTextureExportIntentKind;

const char *drawing_program_texture_export_intent_kind_name(uint32_t kind);
const char *drawing_program_texture_export_intent_button_label(uint32_t kind);
uint32_t drawing_program_texture_export_intent_cycle(uint32_t kind);
uint32_t drawing_program_texture_export_intent_emitted_output_kind(uint32_t kind);

#ifdef __cplusplus
}
#endif

#endif
