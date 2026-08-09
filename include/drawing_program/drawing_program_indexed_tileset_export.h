#ifndef DRAWING_PROGRAM_INDEXED_TILESET_EXPORT_H
#define DRAWING_PROGRAM_INDEXED_TILESET_EXPORT_H

#include <stdint.h>

#include "core_base.h"
#include "drawing_program/drawing_program_texture_project.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DrawingProgramIndexedTilesetExportOptions {
    const char *palette_id;
    uint8_t force_failure_before_commit;
} DrawingProgramIndexedTilesetExportOptions;

typedef struct DrawingProgramIndexedTilesetExportReport {
    char index_digest[17];
    char palette_digest[17];
    char cell_digest[17];
    char index_png_path[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
    char manifest_path[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
    char validation_path[DRAWING_PROGRAM_TEXTURE_PROJECT_PATH_CAPACITY];
} DrawingProgramIndexedTilesetExportReport;

CoreResult drawing_program_indexed_tileset_export(
    const DrawingProgramTextureProject *project,
    const char *destination,
    const DrawingProgramIndexedTilesetExportOptions *options,
    DrawingProgramIndexedTilesetExportReport *out_report);

#ifdef __cplusplus
}
#endif

#endif
