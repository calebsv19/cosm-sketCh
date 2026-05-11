#include "drawing_program/drawing_program_texture_layer_role.h"

#include <stddef.h>
#include <string.h>

static int texture_layer_role_name_starts_with(const char *name, const char *prefix) {
    size_t prefix_len = 0u;
    if (!name || !prefix) {
        return 0;
    }
    prefix_len = strlen(prefix);
    return strncmp(name, prefix, prefix_len) == 0 ? 1 : 0;
}

const char *drawing_program_texture_layer_role_name(uint32_t role_kind) {
    switch ((DrawingProgramTextureLayerRoleKind)role_kind) {
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_BASE: return "Base";
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_MATERIAL_DETAIL: return "Material Detail";
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_DECAL: return "Decal";
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_GRIME: return "Grime";
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_DAMAGE: return "Damage";
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM:
        default: return "Custom";
    }
}

uint8_t drawing_program_texture_layer_role_prefers_overlay(uint32_t role_kind) {
    switch ((DrawingProgramTextureLayerRoleKind)role_kind) {
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_DECAL:
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_GRIME:
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_DAMAGE:
            return 1u;
        default:
            return 0u;
    }
}

uint32_t drawing_program_texture_layer_role_detect_name(const char *name) {
    if (!name || name[0] == '\0') {
        return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM;
    }
    if (texture_layer_role_name_starts_with(name, "Base") || strcmp(name, "Base Layer") == 0) {
        return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_BASE;
    }
    if (texture_layer_role_name_starts_with(name, "Material Detail")) {
        return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_MATERIAL_DETAIL;
    }
    if (texture_layer_role_name_starts_with(name, "Decal")) {
        return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_DECAL;
    }
    if (texture_layer_role_name_starts_with(name, "Grime")) {
        return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_GRIME;
    }
    if (texture_layer_role_name_starts_with(name, "Damage")) {
        return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_DAMAGE;
    }
    return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM;
}
