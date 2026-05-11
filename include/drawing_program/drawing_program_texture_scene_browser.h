#ifndef DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_H
#define DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_H

#include <stddef.h>
#include <stdint.h>

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_LIST_CAPACITY 64u
#define DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_PATH_CAPACITY 512u
#define DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_NAME_CAPACITY 128u
#define DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_ID_CAPACITY 64u
#define DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_TYPE_CAPACITY 32u

typedef struct DrawingProgramTextureSceneFileEntry {
    char scene_id[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_ID_CAPACITY];
    char file_name[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_NAME_CAPACITY];
    char scene_path[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_PATH_CAPACITY];
} DrawingProgramTextureSceneFileEntry;

typedef struct DrawingProgramTextureSceneObjectEntry {
    char object_id[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_ID_CAPACITY];
    char object_type[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_TYPE_CAPACITY];
    char summary[DRAWING_PROGRAM_TEXTURE_SCENE_BROWSER_NAME_CAPACITY];
} DrawingProgramTextureSceneObjectEntry;

CoreResult drawing_program_texture_scene_browser_list_scene_files(
    const char *scene_root_path,
    DrawingProgramTextureSceneFileEntry *out_entries,
    uint32_t entry_capacity,
    uint32_t *out_entry_count);
CoreResult drawing_program_texture_scene_browser_list_supported_objects(
    const char *scene_json_path,
    DrawingProgramTextureSceneObjectEntry *out_entries,
    uint32_t entry_capacity,
    uint32_t *out_entry_count,
    char *out_scene_id,
    size_t out_scene_id_capacity);

#ifdef __cplusplus
}
#endif

#endif
