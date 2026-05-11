#ifndef DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_H
#define DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DrawingProgramTextureLayerMaterialIntentKind {
    DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE = 0u,
    DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_SOLID = 1u,
    DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_METAL = 2u,
    DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_WOOD = 3u,
    DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_BRICK = 4u,
    DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_CONCRETE = 5u,
    DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_STONE = 6u,
    DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_RUST = 7u,
    DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_FOG = 8u,
    DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_GRIME = 9u,
    DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_OIL = 10u
} DrawingProgramTextureLayerMaterialIntentKind;

const char *drawing_program_texture_layer_material_intent_name(uint32_t kind);
const char *drawing_program_texture_layer_material_intent_stable_id(uint32_t kind);
uint8_t drawing_program_texture_layer_material_intent_is_overlay_family(uint32_t kind);
uint8_t drawing_program_texture_layer_material_intent_is_base_family(uint32_t kind);
uint32_t drawing_program_texture_layer_material_intent_default_for_role(uint32_t role_kind);
uint32_t drawing_program_texture_layer_material_intent_from_legacy_overlay_kind(
    uint32_t legacy_overlay_intent_kind);

#ifdef __cplusplus
}
#endif

#endif
