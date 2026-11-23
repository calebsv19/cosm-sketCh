#include "drawing_program/drawing_program_ui_pilot_probe.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drawing_program/drawing_program_render_backend.h"
#include "drawing_program/drawing_program_texture_canvas_resize.h"
#include "drawing_program/drawing_program_texture_workspace.h"
#include "drawing_program/drawing_program_ui_controls.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"

typedef struct CanvasGeometry {
    float x;
    float y;
    uint32_t width;
    uint32_t height;
} CanvasGeometry;

static CanvasGeometry canvas_geometry(const DrawingProgramAppContext *app) {
    const DrawingProgramTextureSurface *surface = drawing_program_texture_project_surface_at(
        &app->texture_project, app->texture_project.active_surface_index);
    return (CanvasGeometry){surface->layout_offset_x, surface->layout_offset_y,
                            surface->storage->document.logical_width,
                            surface->storage->document.logical_height};
}

static int same_geometry(CanvasGeometry a, CanvasGeometry b) {
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

static CanvasGeometry displaced(CanvasGeometry geometry, const VisualCanvasInteractionState *canvas,
                                int resize, int dx, int dy) {
    if (resize) {
        geometry.width += (uint32_t)lroundf(dx / canvas->canvas_resize_pixels_per_logical);
        geometry.height += (uint32_t)lroundf(dy / canvas->canvas_resize_pixels_per_logical);
    } else {
        geometry.x += dx / canvas->canvas_move_zoom;
        geometry.y += dy / canvas->canvas_move_zoom;
    }
    return geometry;
}

static int pointer(SDL_Window *window, SDL_Renderer *renderer,
                   Uint32 type, int x, int y, Uint32 buttons) {
    int dw, dh, ww, wh;
    if (drawing_program_render_backend_output_size(renderer, &dw, &dh) != 0 ||
        dw <= 0 || dh <= 0) return 0;
    SDL_GetWindowSize(window, &ww, &wh);
    x = (int)((double)x * ww / dw);
    y = (int)((double)y * wh / dh);
    SDL_Event event = {0};
    event.type = type;
    if (type == SDL_MOUSEMOTION) {
        event.motion.windowID = SDL_GetWindowID(window);
        event.motion.state = buttons;
        event.motion.x = x;
        event.motion.y = y;
    } else {
        event.button.windowID = SDL_GetWindowID(window);
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = x;
        event.button.y = y;
    }
    return SDL_PushEvent(&event) == 1;
}

static int choose_canvas_layout(SDL_Window *window, SDL_Renderer *renderer, Uint32 type) {
    const KitUiSurface *ui = drawing_program_ui_controls_snapshot();
    for (uint32_t i = 0; i < ui->count; ++i) {
        if (ui->keys[i].domain != DRAWING_UI_RIGHT_PANEL_RENDER_CANVAS_MODE_BUTTON ||
            !ui->controls[i].enabled) continue;
        KitRenderRect bounds = ui->controls[i].bounds;
        return pointer(window, renderer, type, (int)(bounds.x + bounds.width / 2),
                       (int)(bounds.y + bounds.height / 2), 0);
    }
    return 0;
}

static int start_canvas_drag(SDL_Window *window, SDL_Renderer *renderer, const DrawingProgramAppContext *app,
                             int resize, int *x, int *y) {
    SDL_Rect pane;
    uint32_t index = app->texture_project.active_surface_index;
    if (!drawing_program_visual_pane_rect_for_module_type(app, 1u, &pane)) return 0;
    if (resize) {
        SDL_Rect handle;
        if (!drawing_program_texture_canvas_resize_handle_rect_for_surface(app, pane, index, &handle)) return 0;
        *x = handle.x + handle.w / 2;
        *y = handle.y + handle.h / 2;
    } else {
        VisualCanvasSheetMetrics metrics;
        if (!drawing_program_texture_workspace_surface_sheet_metrics(app, pane, index, &metrics)) return 0;
        *x = metrics.sheet_rect.x + metrics.sheet_rect.w / 3;
        *y = metrics.sheet_rect.y + metrics.sheet_rect.h / 3;
    }
    return pointer(window, renderer, SDL_MOUSEBUTTONDOWN, *x, *y, 0);
}

static int cancel_canvas_drag(const char *scenario, SDL_Window *window, SDL_Renderer *renderer,
                              DrawingProgramAppContext *app, int x, int y) {
    if (strcmp(scenario, "missed-release") == 0)
        return pointer(window, renderer, SDL_MOUSEMOTION, x + 56, y + 44, 0);
    if (strcmp(scenario, "remount") == 0) {
        CorePaneId captured = app->pane_host.composition_host.pointer.captured_id;
        for (uint32_t i = 0; i < app->pane_host.module_binding_count; ++i) {
            CorePaneModuleBinding *binding = &app->pane_host.module_bindings[i];
            if (binding->pane_node_id == captured && binding->module_type_id == 1u) {
                binding->instance_id += 100;
                return drawing_program_pane_host_rebuild(app).code == CORE_OK;
            }
        }
        return 0;
    }
    SDL_Event event = {0};
    if (strcmp(scenario, "focus") == 0) {
        event.type = SDL_WINDOWEVENT;
        event.window.windowID = SDL_GetWindowID(window);
        event.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
        return SDL_PushEvent(&event) == 1;
    }
    if (strcmp(scenario, "escape") != 0) return 0;
    event.type = SDL_KEYDOWN;
    event.key.windowID = SDL_GetWindowID(window);
    event.key.keysym.sym = SDLK_ESCAPE;
    event.key.keysym.scancode = SDL_SCANCODE_ESCAPE;
    if (SDL_PushEvent(&event) != 1) return 0;
    event.key.repeat = 1;
    if (SDL_PushEvent(&event) != 1) return 0;
    event.type = SDL_KEYUP;
    event.key.repeat = 0;
    return SDL_PushEvent(&event) == 1;
}

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "CANVAS_TRANSFORM_PROOF status=fail scenario=%s phase=%d check=%s\n", \
            scenario, phase, #condition); return -1; } } while (0)

int drawing_program_canvas_transform_probe(SDL_Window *window, SDL_Renderer *renderer,
    DrawingProgramAppContext *app, const VisualCanvasInteractionState *canvas) {
    static int phase;
    static int x, y;
    static CanvasGeometry before, accepted, expected;
    static uint32_t history_cursor;
    const char *scenario = getenv("DRAWING_PROGRAM_CANVAS_TRANSFORM_SCENARIO");
    if (!scenario) scenario = "release";
    const int test_release = strcmp(scenario, "release") == 0;
    if (phase < 2) {
        CHECK(choose_canvas_layout(window, renderer, phase == 0 ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP));
        ++phase;
        return 0;
    }
    const int resize = phase >= 9;
    const int step = (phase - 2) % 7;
    const int active = canvas->canvas_move_active || canvas->canvas_resize_active;
    const CorePaneId captured = app->pane_host.composition_host.pointer.captured_id;
    const DrawingProgramTextureSurface *surface = drawing_program_texture_project_surface_at(
        &app->texture_project, app->texture_project.active_surface_index);
    CHECK(surface && surface->storage);
    CHECK(app->ui.canvas_control_mode == DRAWING_PROGRAM_UI_CANVAS_CONTROL_MODE_LAYOUT);
    if (step != 0) CHECK(app->history.cursor == history_cursor);
    switch (step) {
    case 0:
        before = canvas_geometry(app);
        history_cursor = app->history.cursor;
        CHECK(start_canvas_drag(window, renderer, app, resize, &x, &y));
        break;
    case 1:
        CHECK(captured && (resize ? canvas->canvas_resize_active : canvas->canvas_move_active));
        expected = displaced(before, canvas, resize, 24, 20);
        CHECK(pointer(window, renderer, test_release ? SDL_MOUSEBUTTONUP : SDL_MOUSEMOTION,
                      x + 24, y + 20, SDL_BUTTON_LMASK));
        break;
    case 2:
        accepted = canvas_geometry(app);
        CHECK(same_geometry(accepted, expected) && !same_geometry(accepted, before));
        if (test_release) {
            CHECK(!active && !captured);
            phase += 4; /* Advance to the first resize step. */
        } else {
            CHECK(active && captured);
            printf("CANVAS_TRANSFORM_PROOF scenario=%s gesture=%s accepted-live-motion=1\n",
                   scenario, resize ? "resize" : "move");
            CHECK(cancel_canvas_drag(scenario, window, renderer, app, x, y));
        }
        break;
    case 3:
        CHECK(!active && !captured && same_geometry(canvas_geometry(app), accepted));
        CHECK(pointer(window, renderer, SDL_MOUSEMOTION, x + 84, y + 60, SDL_BUTTON_LMASK));
        CHECK(pointer(window, renderer, SDL_MOUSEBUTTONUP, x + 96, y + 64, 0));
        break;
    case 4:
        CHECK(!active && !captured && same_geometry(canvas_geometry(app), accepted));
        CHECK(start_canvas_drag(window, renderer, app, resize, &x, &y));
        break;
    case 5:
        CHECK(captured && (resize ? canvas->canvas_resize_active : canvas->canvas_move_active));
        expected = displaced(accepted, canvas, resize, 20, 16);
        CHECK(pointer(window, renderer, SDL_MOUSEBUTTONUP, x + 20, y + 16, 0));
        break;
    case 6:
        CHECK(!active && !captured && same_geometry(canvas_geometry(app), expected) &&
              !same_geometry(canvas_geometry(app), accepted));
        break;
    }
    if ((test_release && step == 2) || step == 6) {
        printf("CANVAS_TRANSFORM_PROOF scenario=%s gesture=%s status=pass\n",
               scenario, resize ? "resize" : "move");
        if (resize) {
            puts("UI_PILOT status=complete");
            return 1;
        }
    }
    ++phase;
    return 0;
}
