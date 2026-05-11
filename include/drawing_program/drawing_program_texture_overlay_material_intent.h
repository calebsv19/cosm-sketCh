#ifndef DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_H
#define DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DrawingProgramTextureOverlayMaterialIntentKind {
    DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_NONE = 0u,
    DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_GRIME = 1u,
    DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_OIL = 2u,
    DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_RUST = 3u,
    DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_FOG = 4u
} DrawingProgramTextureOverlayMaterialIntentKind;

const char *drawing_program_texture_overlay_material_intent_kind_name(uint32_t kind);
const char *drawing_program_texture_overlay_material_intent_button_label(uint32_t kind);
uint32_t drawing_program_texture_overlay_material_intent_cycle(uint32_t kind);

#ifdef __cplusplus
}
#endif

#endif
