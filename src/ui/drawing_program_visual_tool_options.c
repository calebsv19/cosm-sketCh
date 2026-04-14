#include "drawing_program/drawing_program_visual_tool_options.h"

#include <stdio.h>

#include "drawing_program/drawing_program_visual_shape_ops.h"

typedef enum VisualToolOptionKind {
    VISUAL_TOOL_OPTION_BRUSH_SIZE = 0,
    VISUAL_TOOL_OPTION_BRUSH_OPACITY,
    VISUAL_TOOL_OPTION_BRUSH_SPACING,
    VISUAL_TOOL_OPTION_BRUSH_HARDNESS,
    VISUAL_TOOL_OPTION_ERASER_SIZE,
    VISUAL_TOOL_OPTION_SHAPE_STROKE_WIDTH,
    VISUAL_TOOL_OPTION_SHAPE_MODE,
    VISUAL_TOOL_OPTION_FILL_TOLERANCE,
    VISUAL_TOOL_OPTION_SELECT_DELETE
} VisualToolOptionKind;

static const DrawingProgramToolKind k_visual_tools[] = {
    DRAWING_PROGRAM_TOOL_BRUSH,
    DRAWING_PROGRAM_TOOL_ERASER,
    DRAWING_PROGRAM_TOOL_FILL,
    DRAWING_PROGRAM_TOOL_LINE,
    DRAWING_PROGRAM_TOOL_RECT,
    DRAWING_PROGRAM_TOOL_CIRCLE,
    DRAWING_PROGRAM_TOOL_SELECT,
    DRAWING_PROGRAM_TOOL_MOVE,
    DRAWING_PROGRAM_TOOL_PICKER
};

static VisualToolOptionKind visual_tool_option_kind_for_index(const DrawingProgramAppContext *ctx,
                                                              DrawingProgramToolKind tool,
                                                              uint32_t index) {
    (void)ctx;
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_BRUSH:
            if (index == 0u) return VISUAL_TOOL_OPTION_BRUSH_SIZE;
            if (index == 1u) return VISUAL_TOOL_OPTION_BRUSH_OPACITY;
            if (index == 2u) return VISUAL_TOOL_OPTION_BRUSH_SPACING;
            return VISUAL_TOOL_OPTION_BRUSH_HARDNESS;
        case DRAWING_PROGRAM_TOOL_ERASER:
            return VISUAL_TOOL_OPTION_ERASER_SIZE;
        case DRAWING_PROGRAM_TOOL_LINE:
            return VISUAL_TOOL_OPTION_SHAPE_STROKE_WIDTH;
        case DRAWING_PROGRAM_TOOL_RECT:
        case DRAWING_PROGRAM_TOOL_CIRCLE:
            return (index == 0u) ? VISUAL_TOOL_OPTION_SHAPE_STROKE_WIDTH : VISUAL_TOOL_OPTION_SHAPE_MODE;
        case DRAWING_PROGRAM_TOOL_FILL:
            return VISUAL_TOOL_OPTION_FILL_TOLERANCE;
        case DRAWING_PROGRAM_TOOL_SELECT:
            return VISUAL_TOOL_OPTION_SELECT_DELETE;
        default:
            return VISUAL_TOOL_OPTION_BRUSH_SIZE;
    }
}

static const char *visual_tool_option_label(VisualToolOptionKind option) {
    switch (option) {
        case VISUAL_TOOL_OPTION_BRUSH_SIZE: return "SIZE";
        case VISUAL_TOOL_OPTION_BRUSH_OPACITY: return "OPACITY";
        case VISUAL_TOOL_OPTION_BRUSH_SPACING: return "SPACING";
        case VISUAL_TOOL_OPTION_BRUSH_HARDNESS: return "HARDNESS";
        case VISUAL_TOOL_OPTION_ERASER_SIZE: return "SIZE";
        case VISUAL_TOOL_OPTION_SHAPE_STROKE_WIDTH: return "STROKE";
        case VISUAL_TOOL_OPTION_SHAPE_MODE: return "MODE";
        case VISUAL_TOOL_OPTION_FILL_TOLERANCE: return "TOLERANCE";
        case VISUAL_TOOL_OPTION_SELECT_DELETE: return "SELECTION";
        default: return "VALUE";
    }
}

static void visual_tool_option_value_text(const DrawingProgramAppContext *ctx,
                                          VisualToolOptionKind option,
                                          char *out_text,
                                          size_t out_cap) {
    if (!out_text || out_cap == 0u) {
        return;
    }
    switch (option) {
        case VISUAL_TOOL_OPTION_BRUSH_SIZE:
            (void)snprintf(out_text,
                           out_cap,
                           "%u",
                           (unsigned)drawing_program_visual_clamp_setting_u8(ctx ? ctx->ui_tool_brush_size : 2u, 1u, 16u));
            break;
        case VISUAL_TOOL_OPTION_BRUSH_OPACITY:
            (void)snprintf(out_text,
                           out_cap,
                           "%u%%",
                           (unsigned)drawing_program_visual_clamp_setting_u8(ctx ? ctx->ui_tool_brush_opacity : 100u,
                                                                             1u,
                                                                             100u));
            break;
        case VISUAL_TOOL_OPTION_BRUSH_SPACING:
            (void)snprintf(out_text,
                           out_cap,
                           "%u",
                           (unsigned)drawing_program_visual_clamp_setting_u8(ctx ? ctx->ui_tool_brush_spacing : 2u, 1u, 16u));
            break;
        case VISUAL_TOOL_OPTION_BRUSH_HARDNESS:
            (void)snprintf(out_text,
                           out_cap,
                           "%u%%",
                           (unsigned)drawing_program_visual_clamp_setting_u8(ctx ? ctx->ui_tool_brush_hardness : 100u,
                                                                             1u,
                                                                             100u));
            break;
        case VISUAL_TOOL_OPTION_ERASER_SIZE:
            (void)snprintf(out_text,
                           out_cap,
                           "%u",
                           (unsigned)drawing_program_visual_clamp_setting_u8(ctx ? ctx->ui_tool_eraser_size : 4u, 1u, 16u));
            break;
        case VISUAL_TOOL_OPTION_SHAPE_STROKE_WIDTH:
            (void)snprintf(out_text,
                           out_cap,
                           "%u",
                           (unsigned)drawing_program_visual_clamp_setting_u8(ctx ? ctx->ui_tool_shape_stroke_width : 1u,
                                                                             1u,
                                                                             16u));
            break;
        case VISUAL_TOOL_OPTION_SHAPE_MODE:
            (void)snprintf(out_text, out_cap, "%s", drawing_program_visual_shape_mode_name(drawing_program_visual_tool_shape_mode(ctx)));
            break;
        case VISUAL_TOOL_OPTION_FILL_TOLERANCE:
            (void)snprintf(out_text, out_cap, "%u", (unsigned)drawing_program_visual_tool_fill_tolerance_setting(ctx));
            break;
        case VISUAL_TOOL_OPTION_SELECT_DELETE:
            (void)snprintf(out_text, out_cap, "DELETE");
            break;
        default:
            (void)snprintf(out_text, out_cap, "-");
            break;
    }
}

static void visual_tool_option_adjust(DrawingProgramAppContext *ctx, VisualToolOptionKind option, int delta) {
    if (!ctx || delta == 0) {
        return;
    }
    switch (option) {
        case VISUAL_TOOL_OPTION_BRUSH_SIZE: {
            int v = (int)ctx->ui_tool_brush_size + delta;
            if (v < 1) v = 1;
            if (v > 16) v = 16;
            ctx->ui_tool_brush_size = (uint8_t)v;
            break;
        }
        case VISUAL_TOOL_OPTION_BRUSH_OPACITY: {
            int v = (int)ctx->ui_tool_brush_opacity + (delta * 5);
            if (v < 1) v = 1;
            if (v > 100) v = 100;
            ctx->ui_tool_brush_opacity = (uint8_t)v;
            break;
        }
        case VISUAL_TOOL_OPTION_BRUSH_SPACING: {
            int v = (int)ctx->ui_tool_brush_spacing + delta;
            if (v < 1) v = 1;
            if (v > 16) v = 16;
            ctx->ui_tool_brush_spacing = (uint8_t)v;
            break;
        }
        case VISUAL_TOOL_OPTION_BRUSH_HARDNESS: {
            int v = (int)ctx->ui_tool_brush_hardness + (delta * 5);
            if (v < 1) v = 1;
            if (v > 100) v = 100;
            ctx->ui_tool_brush_hardness = (uint8_t)v;
            break;
        }
        case VISUAL_TOOL_OPTION_ERASER_SIZE: {
            int v = (int)ctx->ui_tool_eraser_size + delta;
            if (v < 1) v = 1;
            if (v > 16) v = 16;
            ctx->ui_tool_eraser_size = (uint8_t)v;
            break;
        }
        case VISUAL_TOOL_OPTION_SHAPE_STROKE_WIDTH: {
            int v = (int)ctx->ui_tool_shape_stroke_width + delta;
            if (v < 1) v = 1;
            if (v > 16) v = 16;
            ctx->ui_tool_shape_stroke_width = (uint8_t)v;
            break;
        }
        case VISUAL_TOOL_OPTION_SHAPE_MODE: {
            int v = (int)ctx->ui_tool_shape_mode + delta;
            while (v < 0) {
                v += 3;
            }
            while (v > 2) {
                v -= 3;
            }
            ctx->ui_tool_shape_mode = (uint8_t)v;
            break;
        }
        case VISUAL_TOOL_OPTION_FILL_TOLERANCE: {
            int v = (int)ctx->ui_tool_fill_tolerance + delta;
            if (v < 0) v = 0;
            if (v > (int)DRAWING_PROGRAM_UI_FILL_TOLERANCE_MAX) v = (int)DRAWING_PROGRAM_UI_FILL_TOLERANCE_MAX;
            ctx->ui_tool_fill_tolerance = (uint8_t)v;
            break;
        }
        case VISUAL_TOOL_OPTION_SELECT_DELETE:
            break;
        default:
            break;
    }
}

static int visual_tool_option_is_action_button(VisualToolOptionKind option) {
    return (option == VISUAL_TOOL_OPTION_SELECT_DELETE) ? 1 : 0;
}

uint32_t drawing_program_visual_tool_count(void) {
    return (uint32_t)(sizeof(k_visual_tools) / sizeof(k_visual_tools[0]));
}

DrawingProgramToolKind drawing_program_visual_tool_at(uint32_t index) {
    if (index >= drawing_program_visual_tool_count()) {
        return DRAWING_PROGRAM_TOOL_BRUSH;
    }
    return k_visual_tools[index];
}

DrawingProgramWorkflowControl drawing_program_visual_workflow_control_for_tool(DrawingProgramToolKind tool) {
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_BRUSH: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_BRUSH;
        case DRAWING_PROGRAM_TOOL_ERASER: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_ERASER;
        case DRAWING_PROGRAM_TOOL_FILL: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_FILL;
        case DRAWING_PROGRAM_TOOL_LINE: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_LINE;
        case DRAWING_PROGRAM_TOOL_RECT: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_RECT;
        case DRAWING_PROGRAM_TOOL_CIRCLE: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_CIRCLE;
        case DRAWING_PROGRAM_TOOL_SELECT: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_SELECT;
        case DRAWING_PROGRAM_TOOL_MOVE: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_MOVE;
        case DRAWING_PROGRAM_TOOL_PICKER: return DRAWING_PROGRAM_WORKFLOW_CONTROL_SET_TOOL_PICKER;
        default: return DRAWING_PROGRAM_WORKFLOW_CONTROL_NONE;
    }
}

uint32_t drawing_program_visual_tool_option_count(const DrawingProgramAppContext *ctx, DrawingProgramToolKind tool) {
    switch (tool) {
        case DRAWING_PROGRAM_TOOL_BRUSH:
            return 4u;
        case DRAWING_PROGRAM_TOOL_ERASER:
            return 1u;
        case DRAWING_PROGRAM_TOOL_LINE:
            return 1u;
        case DRAWING_PROGRAM_TOOL_RECT:
        case DRAWING_PROGRAM_TOOL_CIRCLE:
            return 2u;
        case DRAWING_PROGRAM_TOOL_FILL:
            return 1u;
        case DRAWING_PROGRAM_TOOL_SELECT:
            return (ctx && ctx->selection.has_payload) ? 1u : 0u;
        default:
            return 0u;
    }
}

uint32_t drawing_program_visual_tool_option_kind_for_index_raw(const DrawingProgramAppContext *ctx,
                                                               DrawingProgramToolKind tool,
                                                               uint32_t index) {
    return (uint32_t)visual_tool_option_kind_for_index(ctx, tool, index);
}

int drawing_program_visual_tool_option_is_action_button_raw(uint32_t option_kind_raw) {
    return visual_tool_option_is_action_button((VisualToolOptionKind)option_kind_raw);
}

int drawing_program_visual_tool_option_is_select_delete_raw(uint32_t option_kind_raw) {
    return ((VisualToolOptionKind)option_kind_raw == VISUAL_TOOL_OPTION_SELECT_DELETE) ? 1 : 0;
}

void drawing_program_visual_tool_option_adjust_raw(DrawingProgramAppContext *ctx, uint32_t option_kind_raw, int delta) {
    visual_tool_option_adjust(ctx, (VisualToolOptionKind)option_kind_raw, delta);
}

const char *drawing_program_visual_tool_option_label_raw(uint32_t option_kind_raw) {
    return visual_tool_option_label((VisualToolOptionKind)option_kind_raw);
}

void drawing_program_visual_tool_option_value_text_raw(const DrawingProgramAppContext *ctx,
                                                       uint32_t option_kind_raw,
                                                       char *out_text,
                                                       size_t out_cap) {
    visual_tool_option_value_text(ctx, (VisualToolOptionKind)option_kind_raw, out_text, out_cap);
}

uint8_t drawing_program_visual_clamp_left_slot(uint8_t slot) {
    return (slot <= 1u) ? slot : 0u;
}

uint8_t drawing_program_visual_clamp_right_slot(uint8_t slot) {
    return (slot <= 1u) ? slot : 0u;
}

void drawing_program_visual_sync_panel_ui_from_app(const DrawingProgramAppContext *ctx, VisualPanelUiState *ui) {
    if (!ctx || !ui) {
        return;
    }
    ui->left_slot = drawing_program_visual_clamp_left_slot(ctx->ui_left_panel_slot);
    ui->right_slot = drawing_program_visual_clamp_right_slot(ctx->ui_right_panel_slot);
}
