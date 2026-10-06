#include "drawing_program/drawing_program_ui_commands.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_visual_pane_bindings.h"
#include "drawing_program_panel_intent.h"

int drawing_program_ui_command_dispatch(
    DrawingProgramAppContext *app, KitUiSurfaceKey key,
    DrawingProgramSelectionState *selection, VisualPanelUiState *ui,
    const DrawingProgramVisualInputHandlersHooks *hooks) {
    uint32_t module = 0;
    if ((key.domain >= DRAWING_UI_PANEL_RENDER_TAB_TOOLS &&
         key.domain <= DRAWING_UI_PANEL_RENDER_VALUE_RECT) ||
        key.domain == DRAWING_UI_LEFT_TOOL || key.domain == DRAWING_UI_LEFT_OBJECT)
        module = 2u;
    else if ((key.domain >= DRAWING_UI_RIGHT_PANEL_COLOR_RENDER_SAVE_BUTTON_RECT &&
              key.domain <= DRAWING_UI_RIGHT_PANEL_RENDER_ROLE_BUTTON_RECT) ||
             (key.domain >= DRAWING_UI_FILE_PROJECT_SLOT &&
              key.domain <= DRAWING_UI_FILE_SCENE_ENTRY))
        module = 4u;
    if (!module) return 0;
    if (!drawing_program_ui_controls_claim_activation(key)) return 1;
    if (!app || !ui || !hooks || drawing_program_authoring_host_active(app) ||
        app->pane_host.composition_host.blocked) return 1;
    const KitUiSurface *surface = drawing_program_ui_controls_snapshot();
    int eligible = 0;
    if (surface && !surface->collecting && surface->scope == 1u)
        for (uint32_t i = 0; i < surface->count; ++i)
            if (surface->keys[i].domain == key.domain &&
                surface->keys[i].value == key.value && surface->controls[i].enabled) {
                eligible = 1;
                break;
            }
    SDL_Rect content;
    if (!eligible || !drawing_program_visual_pane_rect_for_module_type(app, module, &content) ||
        content.w <= 0 || content.h <= 0) return 1;
    DrawingProgramPanelIntent intent = {.key=key, .semantic=1};
    if (module == 2u)
        drawing_program_visual_input_handle_left_panel_click_payload_intent(
            app, content, intent, selection, ui, hooks);
    else
        drawing_program_visual_input_handle_right_panel_click_payload_intent(
            app, content, intent, selection, ui, hooks);
    return 1;
}
