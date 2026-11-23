#include "drawing_program/drawing_program_ui_pilot_probe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drawing_program/drawing_program_indexed_editor.h"
#include "drawing_program/drawing_program_render_backend.h"
#include "drawing_program/drawing_program_ui_controls.h"
#include "drawing_program/drawing_program_visual_canvas_coords.h"
#include "drawing_program/drawing_program_visual_layout.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program/drawing_program_visual_right_panel_defs.h"

static int pointer(SDL_Window *window, SDL_Renderer *renderer,
                   Uint32 type, Uint8 button, int x, int y, Uint32 held) {
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
        event.motion.state = held;
        event.motion.x = x;
        event.motion.y = y;
    } else {
        event.button.windowID = SDL_GetWindowID(window);
        event.button.button = button;
        event.button.x = x;
        event.button.y = y;
    }
    return SDL_PushEvent(&event) == 1;
}

static int press_control(SDL_Window *window, SDL_Renderer *renderer, uint32_t key) {
    const KitUiSurface *surface = drawing_program_ui_controls_snapshot();
    for (uint32_t i = 0; i < surface->count; ++i) {
        if (surface->keys[i].domain != key || !surface->controls[i].enabled) continue;
        KitRenderRect bounds = surface->controls[i].bounds;
        int x = (int)(bounds.x + bounds.width / 2);
        int y = (int)(bounds.y + bounds.height / 2);
        return pointer(window, renderer, SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, x, y, 0) &&
               pointer(window, renderer, SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, x, y, 0);
    }
    return 0;
}

static int send_key(SDL_Window *window, SDL_Keycode code, Uint32 type, Uint8 repeat, Uint16 mods) {
    SDL_Event event = {0};
    event.type = type;
    event.key.windowID = SDL_GetWindowID(window);
    event.key.keysym.sym = code;
    event.key.keysym.scancode = SDL_GetScancodeFromKey(code);
    event.key.repeat = repeat;
    event.key.keysym.mod = mods;
    return SDL_PushEvent(&event) == 1;
}

static int chord(SDL_Window *window, SDL_Keycode code, Uint16 mods) {
    return send_key(window, code, SDL_KEYDOWN, 0, mods) && send_key(window, code, SDL_KEYUP, 0, mods);
}

static int tap(SDL_Window *window, SDL_Keycode code) {
    return chord(window, code, KMOD_NONE);
}

static int window_event(SDL_Window *window, Uint8 kind) {
    SDL_Event event = {0};
    event.type = SDL_WINDOWEVENT;
    event.window.windowID = SDL_GetWindowID(window);
    event.window.event = kind;
    return SDL_PushEvent(&event) == 1;
}

static int sample(DrawingProgramAppContext *app, SDL_Rect pane, int x, int y,
                  DrawingProgramRasterSample *value) {
    uint32_t sx, sy;
    if (!drawing_program_visual_screen_to_canvas_sample(app, pane, x, y, &sx, &sy)) return 0;
    if (drawing_program_indexed_editor_is_active(app)) {
        const DrawingProgramIndexedLayerRaster *raster = &app->texture_project.indexed_raster;
        *value = raster->indices[(size_t)sy * raster->width + sx];
        return 1;
    }
    return drawing_program_layer_raster_store_raster_sample_read(
        &app->layer_rasters, app->editor.active_layer_id, sx, sy, value).code == CORE_OK;
}

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "L5_CLOSEOUT status=fail scenario=%s phase=%d check=%s\n", \
            scenario, phase, #condition); return -1; } } while (0)
#define POINTER(type, button, dx, dy, held) pointer(window, renderer, type, button, x + dx, y + dy, held)

static int point(SDL_Window *window, SDL_Renderer *renderer, int x, int y) {
    return pointer(window, renderer, SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, x, y, 0) &&
           pointer(window, renderer, SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, x, y, 0);
}

int drawing_program_l5_closeout_probe(SDL_Window *window, SDL_Renderer *renderer,
    DrawingProgramAppContext *app, const VisualPanelUiState *panel, const VisualCanvasInteractionState *canvas) {
    static int phase, x, y;
    static DrawingProgramRasterSample initial, accepted, untouched;
    static float pan_x, pan_y;
    static uint32_t cursor;
    const char *scenario = getenv("DRAWING_PROGRAM_L5_CLOSEOUT_SCENARIO");
    if (!scenario) scenario = "brush";
    const int indexed = drawing_program_indexed_editor_is_active(app);
    SDL_Rect pane;
    CHECK(drawing_program_visual_pane_rect_for_module_type(app, 1u, &pane));

    if (strcmp(scenario, "confirm") == 0) {
        uint32_t count = app->texture_project.surface_count;
        if (phase == 0) {
            CHECK(count == 1 && press_control(window, renderer, DRAWING_UI_RIGHT_PANEL_RENDER_DUPLICATE_SURFACE_BUTTON));
        } else if (phase == 1 || phase == 4) {
            CHECK(count == 2 && !panel->right_canvas_delete_confirm_pending);
            CHECK(press_control(window, renderer, DRAWING_UI_RIGHT_PANEL_RENDER_DELETE_CANVAS_BUTTON));
        } else if (phase == 2) {
            CHECK(count == 2 && panel->right_canvas_delete_confirm_pending);
            puts("L5_CLOSEOUT canvas-delete confirmation armed");
            CHECK(tap(window, SDLK_ESCAPE));
        } else if (phase == 3 || phase == 6) {
            CHECK(count == 2 && !panel->right_canvas_delete_confirm_pending);
            if (phase == 6) {
                puts("UI_PILOT status=complete");
                return 1;
            }
            CHECK(window_event(window, SDL_WINDOWEVENT_EXPOSED));
        } else {
            CHECK(count == 2 && panel->right_canvas_delete_confirm_pending);
            CHECK(window_event(window, SDL_WINDOWEVENT_FOCUS_LOST));
        }
    } else if (strcmp(scenario, "footer") == 0) {
        if (phase == 0) {
            CHECK(press_control(window, renderer, DRAWING_UI_RIGHT_PANEL_RENDER_TAB_EXPORT));
        } else if (phase == 1) {
            SDL_Rect right;
            CHECK(drawing_program_visual_pane_rect_for_module_type(app, 4u, &right));
            VisualPaneLayoutMetrics metrics = make_pane_layout_metrics(app);
            int start = right_file_state_start_y(right, metrics, VISUAL_RIGHT_PANEL_EXPORT_TAB_FOOTER_LINE_COUNT);
            /* Output, intent, emit, overlay, PNG, textures, and action. */
            CHECK(start + 7 * metrics.line_h <= right.y + right.h);
            CHECK(press_control(window, renderer, DRAWING_UI_RIGHT_PANEL_FILE_TABS_RENDER_EXPORT_PNG_BUTTON));
        } else {
            CHECK(strcmp(app->session.file_action_status_message, "PNG EXPORTED") == 0);
            puts("UI_PILOT status=complete");
            return 1;
        }
    } else if (strcmp(scenario, "pan") == 0) {
        if (phase == 0 || phase == 2) {
            pan_x = app->editor.viewport.pan_x;
            pan_y = app->editor.viewport.pan_y;
            x = pane.x + pane.w / 2;
            y = pane.y + pane.h / 2;
            CHECK(POINTER(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_RIGHT, 0, 0, 0));
        } else if (phase == 1) {
            CHECK(canvas->panning_active && POINTER(SDL_MOUSEBUTTONUP, SDL_BUTTON_RIGHT, 28, 24, 0));
        } else if (phase == 3) {
            CHECK(canvas->panning_active && POINTER(SDL_MOUSEMOTION, 0, 50, 24, 0));
        } else if (phase == 4) {
            CHECK(!canvas->panning_active && pan_x == app->editor.viewport.pan_x &&
                  pan_y == app->editor.viewport.pan_y);
            CHECK(POINTER(SDL_MOUSEBUTTONUP, SDL_BUTTON_RIGHT, 60, 40, 0));
        } else {
            CHECK(!canvas->panning_active && pan_x == app->editor.viewport.pan_x &&
                  pan_y == app->editor.viewport.pan_y);
            puts("UI_PILOT status=complete");
            return 1;
        }
        if (phase == 2) CHECK(app->editor.viewport.pan_x != 0.0f || app->editor.viewport.pan_y != 0.0f);
    } else if (strcmp(scenario, "path") == 0) {
        if (phase == 0) {
            CHECK(tap(window, SDLK_p));
        } else if (phase == 1) {
            VisualCanvasSheetMetrics sheet;
            drawing_program_visual_compute_canvas_sheet_metrics(app, pane, &sheet);
            x = sheet.sheet_rect.x + sheet.sheet_rect.w / 3;
            y = sheet.sheet_rect.y + sheet.sheet_rect.h / 3;
            CHECK(point(window, renderer, x, y));
        } else if (phase == 2) {
            CHECK(canvas->path_draft_active && canvas->path_draft_point_count == 1u);
            CHECK(tap(window, SDLK_ESCAPE));
        } else if (phase == 3) {
            CHECK(!canvas->path_draft_active && app->object_selection.count == 0u);
            CHECK(point(window, renderer, x, y));
            CHECK(point(window, renderer, x + 44, y + 28));
        } else if (phase == 4) {
            CHECK(canvas->path_draft_active && canvas->path_draft_point_count == 2u);
            CHECK(tap(window, SDLK_RETURN));
        } else if (phase == 5) {
            /* A two-point draft cannot close. Shift+Enter accepts the open path. */
            CHECK(canvas->path_draft_active && canvas->path_draft_point_count == 2u &&
                  app->object_selection.count == 0u);
            CHECK(chord(window, SDLK_RETURN, KMOD_SHIFT));
        } else {
            const DrawingProgramObjectRecord *path = drawing_program_object_store_get_by_id(
                &app->object_store, app->object_selection.active_object_id);
            CHECK(!canvas->path_draft_active && path && path->type == DRAWING_PROGRAM_OBJECT_TYPE_PATH &&
                  path->path_point_count == 2u && app->object_selection.count == 1u);
            puts("UI_PILOT status=complete");
            return 1;
        }
    } else if (strcmp(scenario, "selection") == 0) {
        if (phase == 0) {
            VisualCanvasSheetMetrics sheet;
            drawing_program_visual_compute_canvas_sheet_metrics(app, pane, &sheet);
            x = sheet.sheet_rect.x + sheet.sheet_rect.w / 3;
            y = sheet.sheet_rect.y + sheet.sheet_rect.h / 3;
            CHECK(point(window, renderer, x, y));
        } else if (phase == 1) {
            cursor = app->history.cursor;
            CHECK(cursor && tap(window, SDLK_s));
        } else if (phase == 2) {
            CHECK(POINTER(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, -12, -12, 0));
        } else if (phase == 3) {
            CHECK(app->selection.selecting && tap(window, SDLK_ESCAPE));
            CHECK(POINTER(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, 12, 12, 0));
        } else if (phase == 4) {
            CHECK(!app->selection.selecting && !app->selection.has_payload && app->history.cursor == cursor);
            CHECK(POINTER(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, -12, -12, 0));
            CHECK(POINTER(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, 12, 12, 0));
        } else if (phase == 5) {
            CHECK(app->selection.has_payload && chord(window, SDLK_c, KMOD_GUI));
        } else if (phase == 6) {
            CHECK(app->clipboard.has_payload);
            CHECK(POINTER(SDL_MOUSEMOTION, 0, 74, 60, 0) && chord(window, SDLK_v, KMOD_GUI));
        } else {
            CHECK(app->history.cursor > cursor);
            puts("UI_PILOT status=complete");
            return 1;
        }
    } else {
        CHECK(strcmp(scenario, "brush") == 0 || strcmp(scenario, "brush-escape") == 0);
        const int escape = strcmp(scenario, "brush-escape") == 0;
        if (phase == 0) {
            if (indexed) {
                CHECK(drawing_program_indexed_editor_select_slot(app, 4u).code == CORE_OK);
                x = pane.x + 85;
                y = pane.y + 100;
            } else {
                VisualCanvasSheetMetrics sheet;
                drawing_program_visual_compute_canvas_sheet_metrics(app, pane, &sheet);
                x = sheet.sheet_rect.x + sheet.sheet_rect.w / 3;
                y = sheet.sheet_rect.y + sheet.sheet_rect.h / 3;
            }
            CHECK(sample(app, pane, x, y, &initial) && sample(app, pane, x + 60, y, &untouched));
            CHECK(POINTER(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, 0, 0, 0));
        } else if (phase == 1) {
            CHECK(canvas->drawing_active && sample(app, pane, x, y, &accepted) && accepted != initial);
            puts("L5_CLOSEOUT brush stroke has an accepted sample");
            if (escape) {
                CHECK(send_key(window, SDLK_ESCAPE, SDL_KEYDOWN, 0, KMOD_NONE));
                CHECK(send_key(window, SDLK_ESCAPE, SDL_KEYDOWN, 1, KMOD_NONE));
                CHECK(send_key(window, SDLK_ESCAPE, SDL_KEYUP, 0, KMOD_NONE));
            } else {
                CHECK(POINTER(SDL_MOUSEMOTION, 0, 60, 0, 0));
            }
        } else if (phase == 2 || phase == 3) {
            DrawingProgramRasterSample actual;
            CHECK(!canvas->drawing_active && !app->pane_host.composition_host.pointer.captured_id);
            CHECK(sample(app, pane, x, y, &actual) && actual == accepted);
            CHECK(sample(app, pane, x + 60, y, &actual) && actual == untouched);
            if (phase == 2) {
                CHECK(POINTER(SDL_MOUSEMOTION, 0, 60, 0, SDL_BUTTON_LMASK));
                CHECK(POINTER(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, 60, 0, 0));
            } else {
                CHECK(POINTER(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, 60, 0, 0));
                CHECK(POINTER(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, 60, 0, 0));
            }
        } else {
            DrawingProgramRasterSample actual;
            CHECK(!canvas->drawing_active && sample(app, pane, x + 60, y, &actual) && actual != untouched);
            puts("UI_PILOT status=complete");
            return 1;
        }
    }
    ++phase;
    return 0;
}
