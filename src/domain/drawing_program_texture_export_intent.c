#include "drawing_program/drawing_program_texture_export_intent.h"

#include "core_authored_texture.h"

const char *drawing_program_texture_export_intent_kind_name(uint32_t kind) {
    switch ((DrawingProgramTextureExportIntentKind)kind) {
        case DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_FLATTENED_ONLY: return "FLATTENED_ONLY";
        case DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_BASE_PLUS_OVERLAY: return "BASE_PLUS_OVERLAY";
        case DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_NONE:
        default: return "NONE";
    }
}

const char *drawing_program_texture_export_intent_button_label(uint32_t kind) {
    switch ((DrawingProgramTextureExportIntentKind)kind) {
        case DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_BASE_PLUS_OVERLAY: return "INTENT: BASE+OVERLAY";
        case DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_FLATTENED_ONLY:
        case DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_NONE:
        default: return "INTENT: FLATTENED";
    }
}

uint32_t drawing_program_texture_export_intent_cycle(uint32_t kind) {
    switch ((DrawingProgramTextureExportIntentKind)kind) {
        case DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_BASE_PLUS_OVERLAY:
            return DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_FLATTENED_ONLY;
        case DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_FLATTENED_ONLY:
        case DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_NONE:
        default:
            return DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_BASE_PLUS_OVERLAY;
    }
}

uint32_t drawing_program_texture_export_intent_emitted_output_kind(uint32_t kind) {
    switch ((DrawingProgramTextureExportIntentKind)kind) {
        case DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_BASE_PLUS_OVERLAY:
            return CORE_AUTHORED_TEXTURE_OUTPUT_KIND_BASE_PLUS_OVERLAY;
        case DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_FLATTENED_ONLY:
        case DRAWING_PROGRAM_TEXTURE_EXPORT_INTENT_KIND_NONE:
        default:
            return CORE_AUTHORED_TEXTURE_OUTPUT_KIND_FLATTENED_ONLY;
    }
}
