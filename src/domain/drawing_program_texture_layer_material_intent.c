#include "drawing_program/drawing_program_texture_layer_material_intent.h"

#include "drawing_program/drawing_program_texture_layer_role.h"
#include "drawing_program/drawing_program_texture_overlay_material_intent.h"

const char *drawing_program_texture_layer_material_intent_name(uint32_t kind) {
    switch ((DrawingProgramTextureLayerMaterialIntentKind)kind) {
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_SOLID: return "Solid";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_METAL: return "Metal";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_WOOD: return "Wood";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_BRICK: return "Brick";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_CONCRETE: return "Concrete";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_STONE: return "Stone";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_RUST: return "Rust";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_FOG: return "Fog";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_GRIME: return "Grime";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_OIL: return "Oil";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE:
        default: return "None";
    }
}

const char *drawing_program_texture_layer_material_intent_stable_id(uint32_t kind) {
    switch ((DrawingProgramTextureLayerMaterialIntentKind)kind) {
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_SOLID: return "solid";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_METAL: return "metal";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_WOOD: return "wood";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_BRICK: return "brick";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_CONCRETE: return "concrete";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_STONE: return "stone";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_RUST: return "rust";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_FOG: return "fog";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_GRIME: return "grime";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_OIL: return "oil";
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE:
        default: return "none";
    }
}

uint8_t drawing_program_texture_layer_material_intent_is_overlay_family(uint32_t kind) {
    switch ((DrawingProgramTextureLayerMaterialIntentKind)kind) {
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_RUST:
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_FOG:
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_GRIME:
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_OIL:
            return 1u;
        default:
            return 0u;
    }
}

uint8_t drawing_program_texture_layer_material_intent_is_base_family(uint32_t kind) {
    switch ((DrawingProgramTextureLayerMaterialIntentKind)kind) {
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_SOLID:
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_METAL:
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_WOOD:
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_BRICK:
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_CONCRETE:
        case DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_STONE:
            return 1u;
        default:
            return 0u;
    }
}

uint32_t drawing_program_texture_layer_material_intent_default_for_role(uint32_t role_kind) {
    switch ((DrawingProgramTextureLayerRoleKind)role_kind) {
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_BASE:
            return DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_SOLID;
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_GRIME:
            return DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_GRIME;
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM:
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_MATERIAL_DETAIL:
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_DECAL:
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_DAMAGE:
        default:
            return DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE;
    }
}

uint32_t drawing_program_texture_layer_material_intent_from_legacy_overlay_kind(
    uint32_t legacy_overlay_intent_kind) {
    switch ((DrawingProgramTextureOverlayMaterialIntentKind)legacy_overlay_intent_kind) {
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_GRIME:
            return DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_GRIME;
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_OIL:
            return DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_OIL;
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_RUST:
            return DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_RUST;
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_FOG:
            return DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_FOG;
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_NONE:
        default:
            return DRAWING_PROGRAM_TEXTURE_LAYER_MATERIAL_INTENT_KIND_NONE;
    }
}
