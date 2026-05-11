#include "drawing_program/drawing_program_texture_net.h"

#include <string.h>

#include "drawing_program/drawing_program_texture_project.h"

typedef struct DrawingProgramTexturePrismSemanticSeed {
    uint32_t face_role;
    uint32_t net_slot;
    uint32_t orientation;
    uint8_t corner_ids[DRAWING_PROGRAM_TEXTURE_NET_FACE_CORNER_COUNT];
    uint8_t edge_ids[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
    uint8_t adjacent_face_roles[DRAWING_PROGRAM_TEXTURE_NET_FACE_EDGE_COUNT];
} DrawingProgramTexturePrismSemanticSeed;

static const DrawingProgramTexturePrismSemanticSeed g_texture_prism_semantic_seeds[] = {
    {
        DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT,
        DRAWING_PROGRAM_TEXTURE_NET_SLOT_FRONT,
        DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R0,
        {
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_TOP_LEFT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_TOP_RIGHT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_BOTTOM_RIGHT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_BOTTOM_LEFT,
        },
        {
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_FRONT_TOP,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_FRONT_RIGHT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_FRONT_BOTTOM,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_FRONT_LEFT,
        },
        {
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_TOP,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_RIGHT,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BOTTOM,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_LEFT,
        },
    },
    {
        DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BACK,
        DRAWING_PROGRAM_TEXTURE_NET_SLOT_BACK,
        DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R180,
        {
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_BACK_TOP_RIGHT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_BACK_TOP_LEFT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_BACK_BOTTOM_LEFT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_BACK_BOTTOM_RIGHT,
        },
        {
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_BACK_TOP,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_BACK_LEFT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_BACK_BOTTOM,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_BACK_RIGHT,
        },
        {
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_TOP,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_LEFT,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BOTTOM,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_RIGHT,
        },
    },
    {
        DRAWING_PROGRAM_TEXTURE_FACE_ROLE_LEFT,
        DRAWING_PROGRAM_TEXTURE_NET_SLOT_LEFT,
        DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R270,
        {
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_BACK_TOP_LEFT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_TOP_LEFT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_BOTTOM_LEFT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_BACK_BOTTOM_LEFT,
        },
        {
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_LEFT_TOP,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_FRONT_LEFT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_LEFT_BOTTOM,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_BACK_LEFT,
        },
        {
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_TOP,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BOTTOM,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BACK,
        },
    },
    {
        DRAWING_PROGRAM_TEXTURE_FACE_ROLE_RIGHT,
        DRAWING_PROGRAM_TEXTURE_NET_SLOT_RIGHT,
        DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R90,
        {
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_TOP_RIGHT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_BACK_TOP_RIGHT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_BACK_BOTTOM_RIGHT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_BOTTOM_RIGHT,
        },
        {
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_RIGHT_TOP,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_BACK_RIGHT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_RIGHT_BOTTOM,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_FRONT_RIGHT,
        },
        {
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_TOP,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BACK,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BOTTOM,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT,
        },
    },
    {
        DRAWING_PROGRAM_TEXTURE_FACE_ROLE_TOP,
        DRAWING_PROGRAM_TEXTURE_NET_SLOT_TOP,
        DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R0,
        {
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_BACK_TOP_LEFT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_BACK_TOP_RIGHT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_TOP_RIGHT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_TOP_LEFT,
        },
        {
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_BACK_TOP,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_RIGHT_TOP,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_FRONT_TOP,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_LEFT_TOP,
        },
        {
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BACK,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_RIGHT,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_LEFT,
        },
    },
    {
        DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BOTTOM,
        DRAWING_PROGRAM_TEXTURE_NET_SLOT_BOTTOM,
        DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R180,
        {
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_BOTTOM_LEFT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_FRONT_BOTTOM_RIGHT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_BACK_BOTTOM_RIGHT,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_VERTEX_BACK_BOTTOM_LEFT,
        },
        {
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_FRONT_BOTTOM,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_RIGHT_BOTTOM,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_BACK_BOTTOM,
            DRAWING_PROGRAM_TEXTURE_NET_PRISM_EDGE_LEFT_BOTTOM,
        },
        {
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_RIGHT,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_BACK,
            DRAWING_PROGRAM_TEXTURE_FACE_ROLE_LEFT,
        },
    },
};

static CoreResult texture_net_invalid(const char *message) {
    CoreResult r = { CORE_ERR_INVALID_ARG, message };
    return r;
}

void drawing_program_texture_surface_semantic_clear(DrawingProgramTextureSurfaceSemantic *semantic) {
    uint32_t i;
    if (!semantic) {
        return;
    }
    memset(semantic, 0, sizeof(*semantic));
    semantic->layout_kind = DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_NONE;
    semantic->net_slot = DRAWING_PROGRAM_TEXTURE_NET_SLOT_UNSPECIFIED;
    semantic->orientation = DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R0;
    for (i = 0u; i < DRAWING_PROGRAM_TEXTURE_NET_FACE_CORNER_COUNT; ++i) {
        semantic->corner_ids[i] = DRAWING_PROGRAM_TEXTURE_NET_UNKNOWN_ID;
        semantic->edge_ids[i] = DRAWING_PROGRAM_TEXTURE_NET_UNKNOWN_ID;
        semantic->adjacent_face_roles[i] = DRAWING_PROGRAM_TEXTURE_FACE_ROLE_UNSPECIFIED;
    }
}

uint32_t drawing_program_texture_net_default_layout_kind(uint32_t primitive_kind) {
    switch ((DrawingProgramTexturePrimitiveKind)primitive_kind) {
        case DRAWING_PROGRAM_TEXTURE_PRIMITIVE_KIND_PLANE:
            return DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_PLANE_SINGLE;
        case DRAWING_PROGRAM_TEXTURE_PRIMITIVE_KIND_RECT_PRISM:
            return DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_RECT_PRISM_CROSS;
        default:
            return DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_NONE;
    }
}

CoreResult drawing_program_texture_net_seed_surface_semantic(uint32_t primitive_kind,
                                                             uint32_t face_role,
                                                             DrawingProgramTextureSurfaceSemantic *out_semantic) {
    uint32_t i;
    if (!out_semantic) {
        return texture_net_invalid("invalid texture net semantic seed request");
    }
    drawing_program_texture_surface_semantic_clear(out_semantic);
    switch ((DrawingProgramTexturePrimitiveKind)primitive_kind) {
        case DRAWING_PROGRAM_TEXTURE_PRIMITIVE_KIND_PLANE:
            if (face_role != DRAWING_PROGRAM_TEXTURE_FACE_ROLE_FRONT) {
                return texture_net_invalid("unsupported plane face role for texture net semantic seed");
            }
            out_semantic->layout_kind = DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_PLANE_SINGLE;
            out_semantic->net_slot = DRAWING_PROGRAM_TEXTURE_NET_SLOT_FRONT;
            out_semantic->orientation = DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R0;
            return core_result_ok();
        case DRAWING_PROGRAM_TEXTURE_PRIMITIVE_KIND_RECT_PRISM:
            for (i = 0u; i < (uint32_t)(sizeof(g_texture_prism_semantic_seeds) / sizeof(g_texture_prism_semantic_seeds[0]));
                 ++i) {
                if (g_texture_prism_semantic_seeds[i].face_role == face_role) {
                    out_semantic->layout_kind = DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_RECT_PRISM_CROSS;
                    out_semantic->net_slot = g_texture_prism_semantic_seeds[i].net_slot;
                    out_semantic->orientation = g_texture_prism_semantic_seeds[i].orientation;
                    memcpy(out_semantic->corner_ids,
                           g_texture_prism_semantic_seeds[i].corner_ids,
                           sizeof(out_semantic->corner_ids));
                    memcpy(out_semantic->edge_ids,
                           g_texture_prism_semantic_seeds[i].edge_ids,
                           sizeof(out_semantic->edge_ids));
                    memcpy(out_semantic->adjacent_face_roles,
                           g_texture_prism_semantic_seeds[i].adjacent_face_roles,
                           sizeof(out_semantic->adjacent_face_roles));
                    return core_result_ok();
                }
            }
            return texture_net_invalid("unsupported rect prism face role for texture net semantic seed");
        default:
            return texture_net_invalid("primitive kind does not support texture net semantic seed");
    }
}

const char *drawing_program_texture_net_layout_kind_name(uint32_t layout_kind) {
    switch ((DrawingProgramTextureNetLayoutKind)layout_kind) {
        case DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_PLANE_SINGLE: return "PLANE";
        case DRAWING_PROGRAM_TEXTURE_NET_LAYOUT_KIND_RECT_PRISM_CROSS: return "PRISM_CROSS";
        default: return "NONE";
    }
}

const char *drawing_program_texture_net_slot_name(uint32_t net_slot) {
    switch ((DrawingProgramTextureNetSlot)net_slot) {
        case DRAWING_PROGRAM_TEXTURE_NET_SLOT_FRONT: return "FRONT";
        case DRAWING_PROGRAM_TEXTURE_NET_SLOT_BACK: return "BACK";
        case DRAWING_PROGRAM_TEXTURE_NET_SLOT_LEFT: return "LEFT";
        case DRAWING_PROGRAM_TEXTURE_NET_SLOT_RIGHT: return "RIGHT";
        case DRAWING_PROGRAM_TEXTURE_NET_SLOT_TOP: return "TOP";
        case DRAWING_PROGRAM_TEXTURE_NET_SLOT_BOTTOM: return "BOTTOM";
        default: return "NONE";
    }
}

const char *drawing_program_texture_net_orientation_name(uint32_t orientation) {
    switch ((DrawingProgramTextureNetOrientation)orientation) {
        case DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R90: return "R90";
        case DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R180: return "R180";
        case DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R270: return "R270";
        case DRAWING_PROGRAM_TEXTURE_NET_ORIENTATION_R0:
        default: return "R0";
    }
}
