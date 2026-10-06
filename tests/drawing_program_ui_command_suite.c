#include "drawing_program/drawing_program_ui_commands.h"
#include "drawing_program/drawing_program_visual_text_render.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_visual_authoring_chrome.h"
#include "drawing_program/drawing_program_indexed_cells.h"
#include "drawing_program/drawing_program_visual_panel_ui_state.h"
#include "../src/app/drawing_program_app_visual_runtime_support.h"
#include "../src/ui/panel/drawing_program_ui_button.h"
#include "kit_workspace_authoring_ui.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"UI command line %d: %s\n",__LINE__,#c); return 1; } } while(0)
static SDL_Renderer *renderer;
static VisualPanelUiState ui;
static DrawingProgramVisualInputHandlersHooks hooks;
static int spatial_hits;
static int forbidden_hit(SDL_Rect r,int x,int y) {
    (void)r; (void)x; (void)y; ++spatial_hits; return 1;
}
static int collect(DrawingProgramAppContext *ctx, KitUiSurfaceKey key, int enabled) {
    CHECK(drawing_program_ui_controls_begin(ctx).code==CORE_OK);
    drawing_program_ui_controls_key(key.domain,key.value,enabled);
    KitUiButtonSpec spec; kit_ui_button_spec_init(&spec,"command");
    KitUiButtonTheme theme={0}; DrawingProgramVisualPanelRenderHooks paint={0};
    paint.measure_bitmap_text_width=drawing_program_visual_measure_bitmap_text_width;
    paint.draw_bitmap_text=drawing_program_visual_draw_bitmap_text;
    /* Deliberately unrelated to the destination pane or its current content
     * layout: accepted command meaning must never be inferred from this point. */
    SDL_Rect r={10,10,60,30},clip={0,0,100,80};
    CHECK(!drawing_program_ui_button_draw_spec(renderer,clip,r,&spec,&theme,1,&paint));
    CHECK(drawing_program_ui_controls_end(renderer).code==CORE_OK);
    return 0;
}
static int activate(DrawingProgramAppContext *ctx, KitUiSurfaceKey key) {
    CHECK(!collect(ctx,key,1));
    SDL_Event e={0}; e.type=SDL_MOUSEBUTTONDOWN; e.button.button=SDL_BUTTON_LEFT;
    KitUiSurfaceKey command;
    CHECK(drawing_program_ui_controls_route(ctx,&e,20,20,&command) && !command.domain);
    e.type=SDL_MOUSEBUTTONUP;
    CHECK(drawing_program_ui_controls_route(ctx,&e,20,20,&command) && command.domain);
    CHECK(command.domain==key.domain && command.value==key.value);
    return 0;
}
static int send(DrawingProgramAppContext *ctx, KitUiSurfaceKey key) {
    CHECK(!activate(ctx,key));
    CHECK(drawing_program_ui_command_dispatch(ctx,key,&ctx->selection,&ui,&hooks));
    CHECK(!spatial_hits);
    return 0;
}
static int keyboard_and_disabled(DrawingProgramAppContext *ctx) {
    KitUiSurfaceKey key={DRAWING_UI_LEFT_TOOL,DRAWING_PROGRAM_TOOL_ERASER};
    CHECK(!collect(ctx,key,0));
    SDL_Event e={0}; e.type=SDL_MOUSEBUTTONDOWN; e.button.button=SDL_BUTTON_LEFT;
    KitUiSurfaceKey command;
    (void)drawing_program_ui_controls_route(ctx,&e,20,20,&command);
    e.type=SDL_MOUSEBUTTONUP;
    (void)drawing_program_ui_controls_route(ctx,&e,20,20,&command);
    CHECK(!command.domain);
    CHECK(drawing_program_ui_command_dispatch(ctx,key,&ctx->selection,&ui,&hooks));
    CHECK(ctx->editor.active_tool!=DRAWING_PROGRAM_TOOL_ERASER);
    CHECK(!collect(ctx,key,1));
    e.type=SDL_KEYDOWN; e.key.keysym.sym=SDLK_TAB;
    CHECK(drawing_program_ui_controls_route(ctx,&e,0,0,&command) && !command.domain);
    e.key.keysym.sym=SDLK_SPACE;
    CHECK(drawing_program_ui_controls_route(ctx,&e,0,0,&command) && !command.domain);
    e.type=SDL_KEYUP;
    CHECK(drawing_program_ui_controls_route(ctx,&e,0,0,&command));
    CHECK(command.domain==key.domain && command.value==key.value);
    CHECK(drawing_program_ui_command_dispatch(ctx,command,&ctx->selection,&ui,&hooks));
    CHECK(ctx->editor.active_tool==DRAWING_PROGRAM_TOOL_ERASER && !spatial_hits);
    return 0;
}
static int standard(DrawingProgramAppContext *ctx) {
    KitUiSurfaceKey tool={DRAWING_UI_LEFT_TOOL,DRAWING_PROGRAM_TOOL_RECT};
    CHECK(drawing_program_ui_command_dispatch(ctx,tool,&ctx->selection,&ui,&hooks));
    CHECK(ctx->editor.active_tool!=DRAWING_PROGRAM_TOOL_RECT); /* no routed activation */
    CHECK(!send(ctx,tool)); CHECK(ctx->editor.active_tool==DRAWING_PROGRAM_TOOL_RECT);
    CHECK(!send(ctx,(KitUiSurfaceKey){DRAWING_UI_RIGHT_PANEL_RENDER_TAB_LAYER,0}));
    CHECK(ctx->ui.right_panel_slot==1);
    uint32_t count=ctx->document.layer_count;
    KitUiSurfaceKey add={DRAWING_UI_RIGHT_PANEL_RENDER_ADD_LAYER,0};
    CHECK(!send(ctx,add)); CHECK(ctx->document.layer_count==count+1);
    CHECK(drawing_program_ui_command_dispatch(ctx,add,&ctx->selection,&ui,&hooks));
    CHECK(ctx->document.layer_count==count+1); /* consumed once */
    uint32_t id=ctx->document.layers[0].layer_id;
    CHECK(!send(ctx,(KitUiSurfaceKey){DRAWING_UI_RIGHT_PANEL_RENDER_SELECT_LAYER,id}));
    CHECK(ctx->editor.active_layer_id==id);
    CHECK(!activate(ctx,add)); drawing_program_ui_controls_invalidate();
    CHECK(drawing_program_ui_command_dispatch(ctx,add,&ctx->selection,&ui,&hooks));
    CHECK(ctx->document.layer_count==count+1);
    CHECK(!send(ctx,(KitUiSurfaceKey){DRAWING_UI_PANEL_RENDER_TAB_OBJECTS,0}));
    DrawingProgramObjectRecord object={0}; object.type=DRAWING_PROGRAM_OBJECT_TYPE_RECT;
    object.layer_id=id; object.visible=1; object.stroke_width=2; object.width=12; object.height=9;
    uint32_t first,second;
    CHECK(drawing_program_object_store_add(&ctx->object_store,&object,&first).code==CORE_OK);
    CHECK(drawing_program_object_store_add(&ctx->object_store,&object,&second).code==CORE_OK);
    CHECK(!send(ctx,(KitUiSurfaceKey){DRAWING_UI_LEFT_OBJECT,first}));
    CHECK(ctx->object_selection.active_object_id==first);
    KitUiSurfaceKey plus={DRAWING_UI_PANEL_RENDER_PLUS_RECT,first};
    CHECK(!activate(ctx,plus));
    drawing_program_object_selection_replace_single(&ctx->object_selection,second);
    CHECK(drawing_program_ui_command_dispatch(ctx,plus,&ctx->selection,&ui,&hooks));
    CHECK(drawing_program_object_store_get_by_id(&ctx->object_store,first)->stroke_width==2);
    CHECK(drawing_program_object_store_get_by_id(&ctx->object_store,second)->stroke_width==2);
    CHECK(!send(ctx,(KitUiSurfaceKey){DRAWING_UI_LEFT_OBJECT,first}));
    CHECK(!send(ctx,plus));
    CHECK(drawing_program_object_store_get_by_id(&ctx->object_store,first)->stroke_width==3);
    CHECK(drawing_program_object_store_get_by_id(&ctx->object_store,second)->stroke_width==2);
    /* A vanished target must not become whichever row replaces it. */
    CHECK(!send(ctx,(KitUiSurfaceKey){DRAWING_UI_LEFT_OBJECT,UINT32_MAX}));
    CHECK(ctx->object_selection.active_object_id==first && !spatial_hits);
    CHECK(!send(ctx,(KitUiSurfaceKey){DRAWING_UI_RIGHT_PANEL_RENDER_TAB_CANVAS,0}));
    uint8_t before=ctx->editor.symmetry_horizontal;
    CHECK(!send(ctx,(KitUiSurfaceKey){DRAWING_UI_RIGHT_PANEL_RENDER_REFLECT_HORIZONTAL_BUTTON,0}));
    CHECK(ctx->editor.symmetry_horizontal!=before);
    /* Wrong content tab rejects the meaning, rather than matching a reused row. */
    count=ctx->document.layer_count; CHECK(!send(ctx,add)); CHECK(ctx->document.layer_count==count);
    CHECK(!activate(ctx,(KitUiSurfaceKey){DRAWING_UI_RIGHT_PANEL_RENDER_REFLECT_HORIZONTAL_BUTTON,0}));
    CHECK(drawing_program_authoring_host_enter(ctx).code==CORE_OK);
    before=ctx->editor.symmetry_horizontal;
    CHECK(drawing_program_ui_command_dispatch(ctx,(KitUiSurfaceKey){DRAWING_UI_RIGHT_PANEL_RENDER_REFLECT_HORIZONTAL_BUTTON,0},&ctx->selection,&ui,&hooks));
    CHECK(ctx->editor.symmetry_horizontal==before);
    SDL_Event takeover={0}; takeover.type=SDL_MOUSEBUTTONDOWN; takeover.button.button=SDL_BUTTON_LEFT;
    KitUiSurfaceKey blocked;
    CHECK(drawing_program_ui_controls_route(ctx,&takeover,20,20,&blocked) && !blocked.domain);
    CHECK(drawing_program_visual_authoring_chrome_command(ctx,(KitUiSurfaceKey){DRAWING_UI_AUTHORING_ACTION,KIT_WORKSPACE_AUTHORING_OVERLAY_BUTTON_CANCEL})==DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_CANCEL);
    CHECK(drawing_program_visual_authoring_chrome_command(ctx,(KitUiSurfaceKey){DRAWING_UI_AUTHORING_FONT_THEME,UINT64_MAX})==DRAWING_PROGRAM_AUTHORING_CHROME_ACTION_NONE);
    CHECK(drawing_program_authoring_host_cancel(ctx).code==CORE_OK);
    CHECK(drawing_program_pane_host_compose(ctx,0).code==CORE_OK);
    CHECK(!activate(ctx,add));
    ctx->pane_host.module_bindings[3].runtime_flags=DRAWING_PROGRAM_PANE_HIDDEN;
    CHECK(drawing_program_pane_host_rebuild(ctx).code==CORE_OK);
    CHECK(drawing_program_pane_host_compose(ctx,0).code==CORE_OK);
    CHECK(drawing_program_ui_command_dispatch(ctx,add,&ctx->selection,&ui,&hooks));
    CHECK(ctx->document.layer_count==count && !spatial_hits);
    ctx->pane_host.module_bindings[3].runtime_flags=0;
    CHECK(drawing_program_pane_host_rebuild(ctx).code==CORE_OK);
    CHECK(drawing_program_pane_host_compose(ctx,0).code==CORE_OK);
    return 0;
}
static int indexed(DrawingProgramAppContext *ctx) {
    DrawingProgramIndexedTilesetProfile p={0};
    p.contract_revision=CORE_AUTHORED_TEXTURE_INDEXED_CONTRACT_REVISION_V1;
    p.atlas_width=ctx->document.logical_width; p.atlas_height=ctx->document.logical_height;
    p.logical_cell_width=8; p.logical_cell_height=8; p.slot_count=2;
    snprintf(p.tileset_id,sizeof(p.tileset_id),"command.fixture");
    snprintf(p.slots[0].id,sizeof(p.slots[0].id),"transparent");
    snprintf(p.slots[1].id,sizeof(p.slots[1].id),"solid");
    p.slots[1].source_rgba=(CoreAuthoredTextureRgba8){255,0,0,255}; p.slots[1].preview_rgba=p.slots[1].source_rgba;
    CHECK(drawing_program_texture_project_enable_indexed_atlas(&ctx->texture_project,&p,0).code==CORE_OK);
    uint32_t count=ctx->document.layer_count;
    CHECK(!send(ctx,(KitUiSurfaceKey){DRAWING_UI_RIGHT_PANEL_RENDER_TAB_LAYER,0}));
    CHECK(ctx->ui.right_panel_slot!=1 && ctx->document.layer_count==count);
    CHECK(!send(ctx,(KitUiSurfaceKey){DRAWING_UI_RIGHT_PANEL_RENDER_TAB_ASSET,0}));
    KitUiSurfaceKey add={DRAWING_UI_RIGHT_PANEL_FILE_TABS_RENDER_ADD_BUTTON,0};
    CHECK(!send(ctx,add)); CHECK(!send(ctx,add));
    DrawingProgramIndexedCellTable *t=&ctx->texture_project.indexed_cells; CHECK(t->count>=2);
    uint64_t id=drawing_program_ui_controls_string_id(t->cells[0].id);
    DrawingProgramIndexedCell swapped=t->cells[0]; t->cells[0]=t->cells[1]; t->cells[1]=swapped;
    CHECK(!send(ctx,(KitUiSurfaceKey){DRAWING_UI_FILE_INDEXED_CELL,id}));
    CHECK(ctx->ui.indexed_selected_cell==1); /* domain identity survives row reorder */
    CHECK(!activate(ctx,(KitUiSurfaceKey){DRAWING_UI_FILE_INDEXED_CELL,id}));
    snprintf(t->cells[1].id,sizeof(t->cells[1].id),"changed-after-press");
    ctx->ui.indexed_selected_cell=0;
    CHECK(drawing_program_ui_command_dispatch(ctx,(KitUiSurfaceKey){DRAWING_UI_FILE_INDEXED_CELL,id},&ctx->selection,&ui,&hooks));
    CHECK(ctx->ui.indexed_selected_cell==0 && !spatial_hits);
    return 0;
}
int drawing_program_ui_command_suite(void) {
    static DrawingProgramAppContext ctx;
    char *args[]={"ui-command","--headless","--no-persist",NULL};
    CHECK(drawing_program_app_bootstrap(&ctx,3,args).code==CORE_OK);
    CHECK(drawing_program_app_config_load(&ctx).code==CORE_OK);
    CHECK(drawing_program_app_state_seed(&ctx).code==CORE_OK);
    CHECK(drawing_program_app_subsystems_init(&ctx).code==CORE_OK);
    CHECK(drawing_program_runtime_start(&ctx).code==CORE_OK);
    CHECK(drawing_program_app_set_pane_host_bounds(&ctx,1200,800).code==CORE_OK);
    CHECK(drawing_program_pane_host_compose(&ctx,0).code==CORE_OK);
    SDL_Surface *pixels=SDL_CreateRGBSurfaceWithFormat(0,100,80,32,SDL_PIXELFORMAT_ARGB8888);
    CHECK(pixels); renderer=SDL_CreateSoftwareRenderer(pixels); CHECK(renderer);
    hooks=*drawing_program_visual_input_handlers_hooks(); hooks.point_in_rect=forbidden_hit;
    hooks.sync_panel_ui_from_app(&ctx,&ui); spatial_hits=0; drawing_program_ui_controls_reset();
    CHECK(!keyboard_and_disabled(&ctx)); CHECK(!standard(&ctx)); CHECK(!indexed(&ctx));
    drawing_program_ui_controls_reset(); SDL_DestroyRenderer(renderer); SDL_FreeSurface(pixels);
    CHECK(drawing_program_app_shutdown(&ctx).code==CORE_OK);
    puts("Direct UI command, once-only activation and stale identity contracts passed (standard/indexed)");
    return 0;
}
