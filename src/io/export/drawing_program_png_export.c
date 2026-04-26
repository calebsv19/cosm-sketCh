#include "drawing_program/drawing_program_png_export.h"

#include <stdio.h>

#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_export_image.h"

static CoreResult png_export_invalid(const char *message) {
    return (CoreResult){ CORE_ERR_INVALID_ARG, message };
}

CoreResult drawing_program_png_export_default_path(const struct DrawingProgramAppContext *ctx,
                                                   char *out_path,
                                                   size_t out_cap) {
    char base_name[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    CoreResult result;
    if (!ctx || !out_path || out_cap == 0u) {
        return png_export_invalid("invalid PNG export default path request");
    }
    result = drawing_program_export_image_default_basename(ctx, base_name, sizeof(base_name));
    if (result.code != CORE_OK) {
        return result;
    }
    (void)snprintf(out_path, out_cap, "%s/%s.png", ctx->session.output_root_path, base_name);
    return core_result_ok();
}

CoreResult drawing_program_png_export_current_document(const struct DrawingProgramAppContext *ctx,
                                                       const char *path) {
    uint8_t *rgba = 0;
    uint32_t width = 0u;
    uint32_t height = 0u;
    CoreResult result;
    if (!ctx || !path || path[0] == '\0') {
        return png_export_invalid("invalid PNG export request");
    }
    result = drawing_program_export_image_compose_document_rgba(ctx, &rgba, &width, &height);
    if (result.code != CORE_OK) {
        return result;
    }
    result = drawing_program_export_image_write_png_rgba(path, rgba, width, height);
    core_free(rgba);
    return result;
}
