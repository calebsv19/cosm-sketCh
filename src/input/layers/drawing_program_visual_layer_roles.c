#include "drawing_program/drawing_program_visual_layer_roles.h"

#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_document.h"

static int layer_role_name_is_generated_generic(const char *name) {
    unsigned layer_id = 0u;
    char extra = '\0';
    if (!name || name[0] == '\0') {
        return 1;
    }
    return (sscanf(name, "Layer %u%c", &layer_id, &extra) == 1) ? 1 : 0;
}

static uint32_t layer_role_kind_from_preset(DrawingProgramVisualLayerRolePreset role);

static uint32_t layer_role_count_existing(const DrawingProgramDocument *document,
                                          const DrawingProgramAppContext *ctx,
                                          DrawingProgramVisualLayerRolePreset role,
                                          uint32_t exclude_layer_id) {
    uint32_t count = 0u;
    uint32_t i = 0u;
    uint32_t target_role_kind = layer_role_kind_from_preset(role);
    if (!document || !ctx) {
        return 0u;
    }
    for (i = 0u; i < document->layer_count; ++i) {
        const DrawingProgramLayer *layer = &document->layers[i];
        if (layer->layer_id == exclude_layer_id) {
            continue;
        }
        if (drawing_program_visual_layer_role_detect_for_layer_id(ctx, layer->layer_id) ==
            target_role_kind) {
            count += 1u;
        }
    }
    return count;
}

static void layer_role_assign_name(char *buffer,
                                   size_t buffer_size,
                                   DrawingProgramVisualLayerRolePreset role,
                                   uint32_t existing_count) {
    const char *base_name = drawing_program_visual_layer_role_name(role);
    if (!buffer || buffer_size == 0u || !base_name) {
        return;
    }
    if (existing_count == 0u) {
        (void)snprintf(buffer, buffer_size, "%s", base_name);
    } else {
        (void)snprintf(buffer, buffer_size, "%s %u", base_name, (unsigned)(existing_count + 1u));
    }
}

static DrawingProgramLayer *active_layer_mutable(DrawingProgramAppContext *ctx) {
    uint32_t index = 0u;
    if (!ctx || ctx->editor.active_layer_id == 0u ||
        drawing_program_document_layer_index_for_id(&ctx->document, ctx->editor.active_layer_id, &index).code != CORE_OK ||
        index >= ctx->document.layer_count) {
        return 0;
    }
    return &ctx->document.layers[index];
}

static uint32_t layer_role_surface_metadata_detect(const DrawingProgramAppContext *ctx, uint32_t layer_id) {
    const DrawingProgramTextureSurface *surface = 0;
    uint32_t i;
    if (!ctx || layer_id == 0u) {
        return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM;
    }
    surface = drawing_program_texture_project_surface_at(&ctx->texture_project,
                                                         ctx->texture_project.active_surface_index);
    if (!surface || !surface->storage) {
        return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM;
    }
    for (i = 0u; i < surface->layer_role_entry_count; ++i) {
        if (surface->layer_role_layer_ids[i] == layer_id) {
            return surface->layer_role_values[i];
        }
    }
    return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM;
}

static uint32_t layer_role_kind_from_preset(DrawingProgramVisualLayerRolePreset role) {
    switch (role) {
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_BASE:
            return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_BASE;
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_MATERIAL_DETAIL:
            return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_MATERIAL_DETAIL;
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DECAL:
            return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_DECAL;
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_GRIME:
            return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_GRIME;
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DAMAGE:
            return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_DAMAGE;
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_CUSTOM:
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_COUNT:
        default:
            return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM;
    }
}

static DrawingProgramVisualLayerRolePreset layer_role_preset_from_kind(uint32_t role_kind) {
    switch ((DrawingProgramTextureLayerRoleKind)role_kind) {
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_BASE:
            return DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_BASE;
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_MATERIAL_DETAIL:
            return DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_MATERIAL_DETAIL;
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_DECAL:
            return DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DECAL;
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_GRIME:
            return DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_GRIME;
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_DAMAGE:
            return DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DAMAGE;
        case DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM:
        default:
            return DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_CUSTOM;
    }
}

const char *drawing_program_visual_layer_role_name(DrawingProgramVisualLayerRolePreset role) {
    switch (role) {
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_BASE: return "Base";
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_MATERIAL_DETAIL: return "Material Detail";
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DECAL: return "Decal";
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_GRIME: return "Grime";
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DAMAGE: return "Damage";
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_CUSTOM:
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_COUNT:
        default: return "Custom";
    }
}

const char *drawing_program_visual_layer_role_button_label(DrawingProgramVisualLayerRolePreset role) {
    switch (role) {
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_BASE: return "BASE";
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_MATERIAL_DETAIL: return "DETAIL";
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DECAL: return "DECAL";
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_GRIME: return "GRIME";
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DAMAGE: return "DAMAGE";
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_CUSTOM:
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_COUNT:
        default: return "CUSTOM";
    }
}

const char *drawing_program_visual_layer_role_lane_label(DrawingProgramVisualLayerRolePreset role) {
    switch (role) {
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_BASE:
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_MATERIAL_DETAIL:
            return "BASE LANE";
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DECAL:
            return "OVERLAY PREF";
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_GRIME:
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DAMAGE:
            return "OVERLAY LANE";
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_CUSTOM:
        case DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_COUNT:
        default:
            return "CUSTOM";
    }
}

int drawing_program_visual_layer_role_prefers_overlay(DrawingProgramVisualLayerRolePreset role) {
    return drawing_program_texture_layer_role_prefers_overlay(layer_role_kind_from_preset(role)) ? 1 : 0;
}

DrawingProgramVisualLayerRolePreset drawing_program_visual_layer_role_detect_name(const char *name) {
    return layer_role_preset_from_kind(drawing_program_texture_layer_role_detect_name(name));
}

uint32_t drawing_program_visual_layer_role_detect_for_layer_id(const DrawingProgramAppContext *ctx,
                                                               uint32_t layer_id) {
    uint32_t role_kind = layer_role_surface_metadata_detect(ctx, layer_id);
    uint32_t layer_index = 0u;
    if (role_kind != DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM) {
        return role_kind;
    }
    if (!ctx || layer_id == 0u ||
        drawing_program_document_layer_index_for_id(&ctx->document, layer_id, &layer_index).code != CORE_OK ||
        layer_index >= ctx->document.layer_count) {
        return DRAWING_PROGRAM_TEXTURE_LAYER_ROLE_KIND_CUSTOM;
    }
    return drawing_program_texture_layer_role_detect_name(ctx->document.layers[layer_index].name);
}

DrawingProgramVisualLayerRolePreset drawing_program_visual_layer_role_detect_active(
    const DrawingProgramAppContext *ctx) {
    uint32_t index = 0u;
    if (!ctx || ctx->editor.active_layer_id == 0u ||
        drawing_program_document_layer_index_for_id(&ctx->document, ctx->editor.active_layer_id, &index).code != CORE_OK ||
        index >= ctx->document.layer_count) {
        return DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_CUSTOM;
    }
    return layer_role_preset_from_kind(
        drawing_program_visual_layer_role_detect_for_layer_id(ctx, ctx->document.layers[index].layer_id));
}

void drawing_program_visual_apply_layer_role_preset_active(DrawingProgramAppContext *ctx,
                                                           DrawingProgramVisualLayerRolePreset role) {
    DrawingProgramLayer *layer = 0;
    uint32_t existing_count = 0u;
    if (!ctx || role >= DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_COUNT) {
        return;
    }
    layer = active_layer_mutable(ctx);
    if (!layer) {
        return;
    }
    existing_count = layer_role_count_existing(&ctx->document, ctx, role, layer->layer_id);
    layer_role_assign_name(layer->name, sizeof(layer->name), role, existing_count);
    drawing_program_texture_project_set_surface_layer_role(&ctx->texture_project,
                                                           ctx->texture_project.active_surface_index,
                                                           layer->layer_id,
                                                           layer_role_kind_from_preset(role));
}

void drawing_program_visual_apply_layer_role_suggest_for_active_if_generic(DrawingProgramAppContext *ctx) {
    DrawingProgramLayer *layer = 0;
    DrawingProgramVisualLayerRolePreset role = DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_CUSTOM;
    if (!ctx) {
        return;
    }
    layer = active_layer_mutable(ctx);
    if (!layer || !layer_role_name_is_generated_generic(layer->name)) {
        return;
    }
    if (layer_role_count_existing(&ctx->document, ctx, DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_BASE, layer->layer_id) == 0u) {
        role = DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_BASE;
    } else if (layer_role_count_existing(&ctx->document,
                                         ctx,
                                         DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_MATERIAL_DETAIL,
                                         layer->layer_id) == 0u) {
        role = DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_MATERIAL_DETAIL;
    } else if (layer_role_count_existing(&ctx->document,
                                         ctx,
                                         DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DECAL,
                                         layer->layer_id) == 0u) {
        role = DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DECAL;
    } else if (layer_role_count_existing(&ctx->document,
                                         ctx,
                                         DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_GRIME,
                                         layer->layer_id) == 0u) {
        role = DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_GRIME;
    } else if (layer_role_count_existing(&ctx->document,
                                         ctx,
                                         DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DAMAGE,
                                         layer->layer_id) == 0u) {
        role = DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_DAMAGE;
    } else {
        role = DRAWING_PROGRAM_VISUAL_LAYER_ROLE_PRESET_MATERIAL_DETAIL;
    }
    drawing_program_visual_apply_layer_role_preset_active(ctx, role);
}
