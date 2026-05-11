#include "drawing_program/drawing_program_texture_overlay_material_intent.h"

const char *drawing_program_texture_overlay_material_intent_kind_name(uint32_t kind) {
    switch ((DrawingProgramTextureOverlayMaterialIntentKind)kind) {
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_GRIME: return "grime";
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_OIL: return "oil";
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_RUST: return "rust";
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_FOG: return "fog";
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_NONE:
        default: return "none";
    }
}

const char *drawing_program_texture_overlay_material_intent_button_label(uint32_t kind) {
    switch ((DrawingProgramTextureOverlayMaterialIntentKind)kind) {
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_GRIME:
            return "OVERLAY INTENT: GRIME";
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_OIL:
            return "OVERLAY INTENT: OIL";
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_RUST:
            return "OVERLAY INTENT: RUST";
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_FOG:
            return "OVERLAY INTENT: FOG";
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_NONE:
        default:
            return "OVERLAY INTENT: NONE";
    }
}

uint32_t drawing_program_texture_overlay_material_intent_cycle(uint32_t kind) {
    switch ((DrawingProgramTextureOverlayMaterialIntentKind)kind) {
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_NONE:
            return DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_GRIME;
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_GRIME:
            return DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_OIL;
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_OIL:
            return DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_RUST;
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_RUST:
            return DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_FOG;
        case DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_FOG:
        default:
            return DRAWING_PROGRAM_TEXTURE_OVERLAY_MATERIAL_INTENT_KIND_NONE;
    }
}
