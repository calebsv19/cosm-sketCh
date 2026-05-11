#ifndef DRAWING_PROGRAM_TEXTURE_PROJECT_TEMPLATE_H
#define DRAWING_PROGRAM_TEXTURE_PROJECT_TEMPLATE_H

#include <stdint.h>

#include "core_base.h"
#include "core_scene.h"
#include "drawing_program/drawing_program_texture_project.h"

#ifdef __cplusplus
extern "C" {
#endif

CoreResult drawing_program_texture_project_init_from_scene_object(
    DrawingProgramTextureProject *project,
    const CoreSceneRootContract *scene_root,
    const CoreSceneObjectContract *scene_object,
    uint32_t quality_preset);

#ifdef __cplusplus
}
#endif

#endif
