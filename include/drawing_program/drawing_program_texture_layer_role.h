#ifndef DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_H
#define DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DrawingProgramTextureLayerRoleKind {
    DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM = 0u,
    DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_BASE = 1u,
    DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_MATERIAL_DETAIL = 2u,
    DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_DECAL = 3u,
    DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_GRIME = 4u,
    DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_DAMAGE = 5u
} DrawingProgramTextureLayerRoleKind;

const char *drawing_program_texture_layer_role_name(uint32_t role_kind);
uint8_t drawing_program_texture_layer_role_prefers_overlay(uint32_t role_kind);
uint32_t drawing_program_texture_layer_role_detect_name(const char *name);

#ifdef __cplusplus
}
#endif

#endif
