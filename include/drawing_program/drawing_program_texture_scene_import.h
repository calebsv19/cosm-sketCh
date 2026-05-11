#ifndef DRAWING_PROGRAM_TEXTURE_SCENE_IMPORT_H
#define DRAWING_PROGRAM_TEXTURE_SCENE_IMPORT_H

#include "core_base.h"
#include "core_scene.h"

#ifdef __cplusplus
extern "C" {
#endif

CoreResult drawing_program_texture_scene_import_load_object(
    const char *scene_json_path,
    const char *object_id,
    CoreSceneRootContract *out_scene_root,
    CoreSceneObjectContract *out_scene_object);

#ifdef __cplusplus
}
#endif

#endif
