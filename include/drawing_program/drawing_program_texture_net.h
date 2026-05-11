#ifndef DRAWING_PROGRAM_TEXTURE_NET_H
#define DRAWING_PROGRAM_TEXTURE_NET_H

#include <stdint.h>

#include "core_base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_TEXTURE_NET_FACE_CORNER_COUNT 4u
#define DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT 4u
#define DRAWING_PROGRAM_TEXTURE_NET_UNKNOWN_ID 255u

typedef enum DrawingProgramTextureNetLayoutKind {
    DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_NONE = 0u,
    DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_PLANE_SINGLE = 1u,
    DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_RECT_PRISM_CROSS = 2u
} DrawingProgramTextureNetLayoutKind;

typedef enum DrawingProgramTextureNetOrientation {
    DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R0 = 0u,
    DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R90 = 1u,
    DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R180 = 2u,
    DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R270 = 3u
} DrawingProgramTextureNetOrientation;

typedef enum DrawingProgramTextureNetSlot {
    DRAWING_PROGRAM_TEXTURE_NET_SLOT_UNSPECIFIED = 0u,
    DRAWING_PROGRAM_TEXTURE_NET_SLOT_FRONT = 1u,
    DRAWING_PROGRAM_TEXTURE_NET_SLOT_BACK = 2u,
    DRAWING_PROGRAM_TEXTURE_NET_SLOT_LEFT = 3u,
    DRAWING_PROGRAM_TEXTURE_NET_SLOT_RIGHT = 4u,
    DRAWING_PROGRAM_TEXTURE_NET_SLOT_TOP = 5u,
    DRAWING_PROGRAM_TEXTURE_NET_SLOT_BOTTOM = 6u
} DrawingProgramTextureNetSlot;

typedef enum DrawingProgramTextureNetCornerSlot {
    DRAWING_PROGRAM_TEXTURE_NET_CORNER_TOP_LEFT = 0u,
    DRAWING_PROGRAM_TEXTURE_NET_CORNER_TOP_RIGHT = 1u,
    DRAWING_PROGRAM_TEXTURE_NET_CORNER_BOTTOM_RIGHT = 2u,
    DRAWING_PROGRAM_TEXTURE_NET_CORNER_BOTTOM_LEFT = 3u
} DrawingProgramTextureNetCornerSlot;

typedef enum DrawingProgramTextureNetEdgeSide {
    DRAWING_PROGRAM_TEXTURE_NET_EDGE_SIDE_TOP = 0u,
    DRAWING_PROGRAM_TEXTURE_NET_EDGE_SIDE_RIGHT = 1u,
    DRAWING_PROGRAM_TEXTURE_NET_EDGE_SIDE_BOTTOM = 2u,
    DRAWING_PROGRAM_TEXTURE_NET_EDGE_SIDE_LEFT = 3u
} DrawingProgramTextureNetEdgeSide;

typedef enum DrawingProgramTextureNetPrismVertexId {
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_TOP_LEFT = 0u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_TOP_RIGHT = 1u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_BOTTOM_RIGHT = 2u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_BOTTOM_LEFT = 3u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_BACK_TOP_LEFT = 4u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_BACK_TOP_RIGHT = 5u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_BACK_BOTTOM_RIGHT = 6u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_BACK_BOTTOM_LEFT = 7u
} DrawingProgramTextureNetPrismVertexId;

typedef enum DrawingProgramTextureNetPrismEdgeId {
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_FRONT_TOP = 0u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_FRONT_RIGHT = 1u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_FRONT_BOTTOM = 2u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_FRONT_LEFT = 3u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_BACK_TOP = 4u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_BACK_RIGHT = 5u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_BACK_BOTTOM = 6u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_BACK_LEFT = 7u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_LEFT_TOP = 8u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_RIGHT_TOP = 9u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_RIGHT_BOTTOM = 10u,
    DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_LEFT_BOTTOM = 11u
} DrawingProgramTextureNetPrismEdgeId;

typedef struct DrawingProgramTextureSurfaceSemantic {
    uint32_t layout_kind;
    uint32_t net_slot;
    uint32_t orientation;
    uint8_t corner_ids[DRAWING_PROGRAM_TEXTURE_NET_FACE_CORNER_COUNT];
    uint8_t edge_ids[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    uint8_t adjacent_face_roles[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    uint8_t reserved0[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
} DrawingProgramTextureSurfaceSemantic;

void drawing_program_texture_surface_semantic_clear(DrawingProgramTextureSurfaceSemantic *semantic);
uint32_t drawing_program_texture_net_default_layout_kind(uint32_t primitive_kind);
CoreResult drawing_program_texture_net_seed_surface_semantic(uint32_t primitive_kind,
                                                             uint32_t face_role,
                                                             DrawingProgramTextureSurfaceSemantic *out_semantic);
const char *drawing_program_texture_net_layout_kind_name(uint32_t layout_kind);
const char *drawing_program_texture_net_slot_name(uint32_t net_slot);
const char *drawing_program_texture_net_orientation_name(uint32_t orientation);

#ifdef __cplusplus
}
#endif

#endif
