#ifndef DRAWING_PROGRAM_APP_MAIN_H
#define DRAWING_PROGRAM_APP_MAIN_H

#include <stdint.h>

#include "core_base.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_editor_state.h"
#include "drawing_program/drawing_program_history.h"
#include "drawing_program/drawing_program_layer_raster.h"
#include "drawing_program/drawing_program_object_store.h"
#include "drawing_program/drawing_program_overlay_adapter.h"
#include "drawing_program/drawing_program_pane_host.h"
#include "drawing_program/drawing_program_render_domain.h"
#include "drawing_program/drawing_program_selection.h"
#include "drawing_program/drawing_program_snapshot.h"
#include "drawing_program/drawing_program_viewport.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWING_PROGRAM_UI_FILL_TOLERANCE_MAX 32u
#define DRAWING_PROGRAM_UI_FILL_TOLERANCE_SAMPLE_SCALE 8u
#define DRAWING_PROGRAM_PROJECT_PATH_CAPACITY 512u
#define DRAWING_PROGRAM_RECENT_PROJECT_CAPACITY 512u

typedef enum DrawingProgramUiShapeTargetMode {
    DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_PIXEL = 0u,
    DRAWING_PROGRAM_UI_SHAPE_TARGET_MODE_OBJECT = 1u
} DrawingProgramUiShapeTargetMode;

typedef enum DrawingProgramUiSelectMode {
    DRAWING_PROGRAM_UI_SELECT_MODE_REPLACE = 0u,
    DRAWING_PROGRAM_UI_SELECT_MODE_ADD = 1u,
    DRAWING_PROGRAM_UI_SELECT_MODE_SUBTRACT = 2u
} DrawingProgramUiSelectMode;

typedef enum DrawingProgramAuthoringOverlayMode {
    DRAWING_PROGRAM_AUTHORING_OVERLAY_PANE = 0u,
    DRAWING_PROGRAM_AUTHORING_OVERLAY_FONT_THEME = 1u
} DrawingProgramAuthoringOverlayMode;

typedef struct DrawingProgramAppRuntimeState {
    uint64_t frame_counter;
    uint8_t state_seeded;
    uint8_t subsystems_ready;
    uint8_t runtime_started;
    uint8_t snapshot_loaded_from_preset;
    uint64_t input_events_processed;
    uint64_t input_actions_emitted;
    uint64_t routed_global_total;
    uint64_t routed_pane_total;
    uint64_t routed_fallback_total;
    uint64_t invalidation_target_total;
    uint64_t invalidation_full_total;
    uint64_t invalidation_reason_bits_total;
    uint64_t viewport_sample_probe_success_total;
    uint64_t render_frames_projected_total;
    uint64_t render_layers_visible_total;
    uint64_t render_full_redraw_total;
    uint64_t render_target_redraw_total;
    uint64_t render_module_calls_total;
    uint64_t render_module_canvas_calls_total;
    uint64_t render_module_palette_calls_total;
    uint32_t render_canvas_last_raster_hash;
    uint32_t render_canvas_last_nonzero_samples;
    uint64_t tool_switch_total;
    uint32_t render_last_active_layer_id;
    uint8_t render_last_has_active_layer;
    DrawingProgramRenderFrameProjection render_projection;
} DrawingProgramAppRuntimeState;

typedef struct DrawingProgramAppUiState {
    uint32_t theme_preset_id;
    uint32_t font_preset_id;
    uint8_t left_panel_slot;
    uint8_t right_panel_slot;
    uint8_t active_color_index;
    uint8_t active_paint_r;
    uint8_t active_paint_g;
    uint8_t active_paint_b;
    uint8_t color_hue;
    uint8_t color_saturation;
    uint8_t color_value;
    uint8_t tool_brush_size;
    uint8_t tool_brush_opacity;
    uint8_t tool_brush_spacing;
    uint8_t tool_brush_hardness;
    uint8_t tool_eraser_size;
    uint8_t tool_shape_stroke_width;
    uint8_t tool_shape_mode;
    uint8_t tool_shape_target_mode;
    uint8_t tool_fill_tolerance;
    uint8_t tool_select_mode;
    uint8_t layer_opacity_entry_count;
    uint8_t recent_color_count;
    uint8_t selected_recent_color_index;
    int8_t font_zoom_step;
    uint8_t layer_opacity_values[DRAWING_PROGRAM_MAX_LAYERS];
    uint32_t layer_opacity_layer_ids[DRAWING_PROGRAM_MAX_LAYERS];
    uint8_t recent_color_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3];
    uint8_t color_palette_rgb[DRAWING_PROGRAM_UI_COLOR_PALETTE_COUNT][3];
} DrawingProgramAppUiState;

typedef struct DrawingProgramAuthoringHostState {
    uint8_t key_c_down;
    uint8_t key_v_down;
    uint8_t entry_chord_armed_key;
    uint8_t draft_baseline_valid;
    uint8_t overlay_mode;
    uint8_t font_theme_pending_changes;
    uint8_t last_event_consumed;
    uint8_t last_event_entered;
    uint8_t last_event_exited;
    uint8_t custom_theme_status_active;
    uint32_t enter_count;
    uint32_t exit_count;
    uint32_t apply_count;
    uint32_t cancel_count;
    uint32_t draft_change_count;
    uint32_t consumed_key_count;
    uint32_t overlay_cycle_count;
    uint32_t font_theme_change_count;
    uint32_t custom_theme_stub_count;
    uint32_t baseline_theme_preset_id;
    uint32_t baseline_font_preset_id;
    int8_t baseline_font_zoom_step;
    char custom_theme_status[128];
    CorePaneNode baseline_nodes[DRAWING_PROGRAM_PANE_NODE_CAPACITY];
    uint32_t baseline_node_count;
    uint32_t baseline_root_index;
    CorePaneModuleBinding baseline_module_bindings[DRAWING_PROGRAM_MODULE_BINDING_CAPACITY];
    uint32_t baseline_module_binding_count;
} DrawingProgramAuthoringHostState;

typedef struct DrawingProgramAppSessionState {
    uint8_t headless;
    uint8_t print_lifecycle;
    uint8_t export_json_requested;
    uint8_t bridge_workspace_check_requested;
    uint8_t bridge_workspace_import_requested;
    uint8_t preset_path_cli_override;
    uint8_t persist_enabled;
    uint8_t runtime_root_cli_override;
    uint8_t input_root_cli_override;
    uint8_t output_root_cli_override;
    uint8_t canvas_size_cli_override;
    uint32_t smoke_frames;
    uint32_t seed_canvas_logical_width;
    uint32_t seed_canvas_logical_height;
    char runtime_root_path[512];
    char input_root_path[512];
    char output_root_path[512];
    char preset_path_buffer[512];
    char project_path_buffer[DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    char recent_project_paths[DRAWING_PROGRAM_RECENT_PROJECT_CAPACITY][DRAWING_PROGRAM_PROJECT_PATH_CAPACITY];
    char file_action_status_message[160];
    uint32_t project_saved_history_count;
    uint32_t project_saved_history_cursor;
    uint8_t project_has_saved_state;
    uint16_t recent_project_count;
    uint8_t ui_prefs_loaded;
    uint8_t reserved0;
    uint8_t reserved1;
    uint8_t reserved2;
    uint32_t ui_theme_preset_id;
    uint32_t ui_font_preset_id;
    int8_t ui_font_zoom_step;
    const char *preset_path;
    const char *project_path;
    const char *export_json_path;
    const char *bridge_workspace_preset_path;
} DrawingProgramAppSessionState;

typedef struct DrawingProgramAppContext {
    DrawingProgramAppSessionState session;
    DrawingProgramAppRuntimeState runtime;
    DrawingProgramDocument document;
    DrawingProgramEditorState editor;
    DrawingProgramHistory history;
    DrawingProgramLayerRasterStore layer_rasters;
    DrawingProgramObjectStore object_store;
    DrawingProgramObjectSelectionState object_selection;
    DrawingProgramAuthoringHostState authoring_host;
    DrawingProgramPaneHost pane_host;
    DrawingProgramOverlayAdapterState overlay_adapter;
    float pane_host_bounds_width;
    float pane_host_bounds_height;
    DrawingProgramAppUiState ui;
    DrawingProgramSelectionState selection;
    DrawingProgramClipboardState clipboard;
} DrawingProgramAppContext;

typedef enum DrawingProgramInputRouteTargetPolicy {
    DRAWING_PROGRAM_INPUT_ROUTE_TARGET_FALLBACK = 0,
    DRAWING_PROGRAM_INPUT_ROUTE_TARGET_GLOBAL = 1,
    DRAWING_PROGRAM_INPUT_ROUTE_TARGET_PANE = 2
} DrawingProgramInputRouteTargetPolicy;

typedef enum DrawingProgramInputInvalidateReasonBits {
    DRAWING_PROGRAM_INPUT_INVALIDATE_REASON_QUIT = 1u << 0,
    DRAWING_PROGRAM_INPUT_INVALIDATE_REASON_WINDOW = 1u << 1,
    DRAWING_PROGRAM_INPUT_INVALIDATE_REASON_KEYBOARD = 1u << 2,
    DRAWING_PROGRAM_INPUT_INVALIDATE_REASON_POINTER = 1u << 3,
    DRAWING_PROGRAM_INPUT_INVALIDATE_REASON_WHEEL = 1u << 4
} DrawingProgramInputInvalidateReasonBits;

typedef struct DrawingProgramInputEventRaw {
    uint64_t frame_index;
    uint32_t event_count;
    uint32_t quit_event_count;
    uint32_t window_event_count;
    uint32_t key_event_count;
    uint32_t pointer_event_count;
    uint32_t wheel_event_count;
    uint32_t other_event_count;
    uint8_t quit_requested;
} DrawingProgramInputEventRaw;

typedef struct DrawingProgramInputEventNormalized {
    uint8_t has_quit_action;
    uint8_t has_window_action;
    uint8_t has_keyboard_action;
    uint8_t has_pointer_action;
    uint8_t has_wheel_action;
    uint32_t action_count;
    uint32_t immediate_action_count;
    uint32_t queued_action_count;
    uint32_t ignored_action_count;
    uint8_t has_tool_switch_action;
    uint8_t requested_tool_kind;
} DrawingProgramInputEventNormalized;

typedef struct DrawingProgramInputRouteResult {
    uint8_t consumed;
    DrawingProgramInputRouteTargetPolicy target_policy;
    uint32_t routed_global_count;
    uint32_t routed_pane_count;
    uint32_t routed_fallback_count;
} DrawingProgramInputRouteResult;

typedef struct DrawingProgramInputInvalidationResult {
    uint8_t full_invalidate;
    uint32_t invalidation_reason_bits;
    uint32_t target_invalidation_count;
    uint32_t full_invalidation_count;
} DrawingProgramInputInvalidationResult;

CoreResult drawing_program_app_bootstrap(DrawingProgramAppContext *ctx, int argc, char **argv);
CoreResult drawing_program_app_config_load(DrawingProgramAppContext *ctx);
CoreResult drawing_program_app_state_seed(DrawingProgramAppContext *ctx);
CoreResult drawing_program_app_subsystems_init(DrawingProgramAppContext *ctx);
CoreResult drawing_program_runtime_start(DrawingProgramAppContext *ctx);
CoreResult drawing_program_app_run_loop(DrawingProgramAppContext *ctx);
CoreResult drawing_program_app_shutdown(DrawingProgramAppContext *ctx);
CoreResult drawing_program_app_set_pane_host_bounds(DrawingProgramAppContext *ctx,
                                                     float width,
                                                     float height);
CoreResult drawing_program_app_shape_commit_samples(DrawingProgramAppContext *ctx,
                                                    DrawingProgramToolKind tool,
                                                    uint32_t start_x,
                                                    uint32_t start_y,
                                                    uint32_t end_x,
                                                    uint32_t end_y);

int drawing_program_app_main(int argc, char **argv);
int drawing_program_app_visual_main(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif
