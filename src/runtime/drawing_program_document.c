#include "drawing_program/drawing_program_document.h"

#include <stdio.h>
#include <string.h>

static CoreResult drawing_program_document_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

CoreResult drawing_program_document_init_default(DrawingProgramDocument *document) {
    if (!document) {
        return drawing_program_document_invalid("null document");
    }

    memset(document, 0, sizeof(*document));
    document->schema_version = 1u;
    document->logical_width = 512u;
    document->logical_height = 512u;
    document->sample_density = 1u;
    document->next_layer_id = 2u;
    document->layer_count = 1u;

    document->layers[0].layer_id = 1u;
    (void)snprintf(document->layers[0].name,
                   sizeof(document->layers[0].name),
                   "%s",
                   "Base Layer");
    document->layers[0].visible = 1u;
    document->layers[0].locked = 0u;
    return core_result_ok();
}

CoreResult drawing_program_document_set_layer_visibility(DrawingProgramDocument *document,
                                                         uint32_t layer_id,
                                                         uint8_t visible,
                                                         uint8_t *out_previous_visibility) {
    uint32_t i;
    if (!document || layer_id == 0u) {
        return drawing_program_document_invalid("invalid layer visibility request");
    }
    for (i = 0u; i < document->layer_count; ++i) {
        if (document->layers[i].layer_id == layer_id) {
            if (out_previous_visibility) {
                *out_previous_visibility = document->layers[i].visible;
            }
            document->layers[i].visible = visible ? 1u : 0u;
            return core_result_ok();
        }
    }
    return (CoreResult){ CORE_ERR_NOT_FOUND, "layer not found" };
}
