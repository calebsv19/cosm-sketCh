#include "drawing_program/drawing_program_visual_canvas_world_render.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_text_render.h"
#include "drawing_program/drawing_program_visual_theme.h"
#include <stdio.h>

void drawing_program_visual_draw_canvas_content_readout(
    SDL_Renderer *renderer, SDL_Rect content,
    const DrawingProgramAppContext *ctx, const CoreThemePreset *theme) {
    if (!renderer || !ctx)
        return;
    VisualPaneLayoutMetrics m = make_pane_layout_metrics(ctx);
    VisualThemePalette p;
    resolve_visual_theme_palette(theme, &p);
    char line[96];
    (void)snprintf(line, sizeof(line), "WORLD VIEW  ZOOM: %.2fx",
                   (double)ctx->editor.viewport.zoom);
    drawing_program_visual_draw_bitmap_text(renderer, content,
        content.x + m.pad_x, content.y, line, p.text_muted, m.body_scale);
}
