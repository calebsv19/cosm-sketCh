#ifndef DRAWING_PROGRAM_VISUAL_ARTIFACT_H
#define DRAWING_PROGRAM_VISUAL_ARTIFACT_H

#include <SDL2/SDL.h>

typedef struct DrawingProgramVisualArtifactRequest {
    int enabled;
    const char *path;
} DrawingProgramVisualArtifactRequest;

DrawingProgramVisualArtifactRequest drawing_program_visual_artifact_parse_request(int argc,
                                                                                   char **argv);
int drawing_program_visual_artifact_filter_app_args(int argc,
                                                    char **argv,
                                                    char **filtered_argv,
                                                    int filtered_capacity);
int drawing_program_visual_artifact_write(SDL_Renderer *renderer, const char *path);

#endif
