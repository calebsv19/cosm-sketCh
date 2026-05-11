PROGRAM_KEY := drawing_program
RELEASE_PRODUCT_NAME := sketCh
RELEASE_BUNDLE_ID := com.cosm.sketch
LAUNCHER_BIN := sketch-launcher
APP_BIN := drawing-program-bin
VERSION_FILE := VERSION
RELEASE_CHANNEL ?= stable

HOST_CC ?= cc
FISICS_CC ?= /Users/calebsv/Desktop/CodeWork/fisiCs/fisics
BUILD_TOOLCHAIN ?= clang
PACKAGE_TOOLCHAIN ?= $(BUILD_TOOLCHAIN)
TEST_TOOLCHAIN := clang
PKG_CONFIG ?= pkg-config
TARGET_CONTRACT_HELPER ?= ../bin/desktop_release_target_contract.sh
HOST_ARCH := $(strip $(shell "$(TARGET_CONTRACT_HELPER)" get host_arch))
TARGET_OS_INPUT := $(TARGET_OS)
TARGET_ARCH_INPUT := $(TARGET_ARCH)
TARGET_VARIANT_INPUT := $(TARGET_VARIANT)
TARGET_OS ?= $(strip $(shell TARGET_OS="$(TARGET_OS_INPUT)" TARGET_ARCH="$(TARGET_ARCH_INPUT)" TARGET_VARIANT="$(TARGET_VARIANT_INPUT)" "$(TARGET_CONTRACT_HELPER)" get target_os))
TARGET_ARCH ?= $(strip $(shell TARGET_OS="$(TARGET_OS_INPUT)" TARGET_ARCH="$(TARGET_ARCH_INPUT)" TARGET_VARIANT="$(TARGET_VARIANT_INPUT)" "$(TARGET_CONTRACT_HELPER)" get target_arch))
TARGET_VARIANT ?= $(strip $(shell TARGET_OS="$(TARGET_OS_INPUT)" TARGET_ARCH="$(TARGET_ARCH_INPUT)" TARGET_VARIANT="$(TARGET_VARIANT_INPUT)" "$(TARGET_CONTRACT_HELPER)" get target_variant))
TARGET_TRIPLE := $(strip $(shell TARGET_OS="$(TARGET_OS)" TARGET_ARCH="$(TARGET_ARCH)" TARGET_VARIANT="$(TARGET_VARIANT)" "$(TARGET_CONTRACT_HELPER)" get target_triple))
RELEASE_PLATFORM := $(strip $(shell TARGET_OS="$(TARGET_OS)" TARGET_ARCH="$(TARGET_ARCH)" TARGET_VARIANT="$(TARGET_VARIANT)" "$(TARGET_CONTRACT_HELPER)" get release_platform))
RELEASE_ARCH := $(strip $(shell TARGET_OS="$(TARGET_OS)" TARGET_ARCH="$(TARGET_ARCH)" TARGET_VARIANT="$(TARGET_VARIANT)" "$(TARGET_CONTRACT_HELPER)" get release_arch))
TARGET_HOMEBREW_PREFIX ?= $(strip $(shell TARGET_OS="$(TARGET_OS)" TARGET_ARCH="$(TARGET_ARCH)" TARGET_VARIANT="$(TARGET_VARIANT)" "$(TARGET_CONTRACT_HELPER)" get homebrew_prefix))
TARGET_ALT_HOMEBREW_PREFIX ?= $(strip $(shell TARGET_OS="$(TARGET_OS)" TARGET_ARCH="$(TARGET_ARCH)" TARGET_VARIANT="$(TARGET_VARIANT)" "$(TARGET_CONTRACT_HELPER)" get alt_homebrew_prefix))
TARGET_PKG_CONFIG_LIBDIR ?= $(TARGET_HOMEBREW_PREFIX)/lib/pkgconfig:$(TARGET_HOMEBREW_PREFIX)/share/pkgconfig
TARGET_DEP_SEARCH_ROOTS ?= $(TARGET_HOMEBREW_PREFIX):$(TARGET_ALT_HOMEBREW_PREFIX)
ARCH_FLAGS := -arch $(TARGET_ARCH)
SHARED_VENDOR_DIR ?= third_party/codework_shared
SHARED_WORKSPACE_DIR ?= ../shared
SHARED_MODE ?= vendored-subtree

CORE_BASE_DIR ?= $(SHARED_VENDOR_DIR)/core/core_base
CORE_OBJECT_DIR ?= $(SHARED_VENDOR_DIR)/core/core_object
CORE_UNITS_DIR ?= $(SHARED_VENDOR_DIR)/core/core_units
CORE_SCENE_DIR ?= $(SHARED_VENDOR_DIR)/core/core_scene
CORE_IO_DIR ?= $(SHARED_VENDOR_DIR)/core/core_io
CORE_THEME_DIR ?= $(SHARED_VENDOR_DIR)/core/core_theme
CORE_FONT_DIR ?= $(SHARED_VENDOR_DIR)/core/core_font
CORE_PACK_DIR ?= $(SHARED_VENDOR_DIR)/core/core_pack
CORE_PANE_DIR ?= $(SHARED_VENDOR_DIR)/core/core_pane
CORE_LAYOUT_DIR ?= $(SHARED_VENDOR_DIR)/core/core_layout
CORE_PANE_MODULE_DIR ?= $(SHARED_VENDOR_DIR)/core/core_pane_module
CORE_VIEWPORT2D_DIR ?= $(SHARED_VENDOR_DIR)/core/core_viewport2d
KIT_RENDER_DIR ?= $(SHARED_VENDOR_DIR)/kit/kit_render
KIT_PANE_DIR ?= $(SHARED_VENDOR_DIR)/kit/kit_pane
KIT_WORKSPACE_AUTHORING_DIR ?= $(SHARED_VENDOR_DIR)/kit/kit_workspace_authoring

ifeq ($(SHARED_MODE),workspace-linked)
CORE_BASE_DIR := $(SHARED_WORKSPACE_DIR)/core/core_base
CORE_OBJECT_DIR := $(SHARED_WORKSPACE_DIR)/core/core_object
CORE_UNITS_DIR := $(SHARED_WORKSPACE_DIR)/core/core_units
CORE_SCENE_DIR := $(SHARED_WORKSPACE_DIR)/core/core_scene
CORE_IO_DIR := $(SHARED_WORKSPACE_DIR)/core/core_io
CORE_THEME_DIR := $(SHARED_WORKSPACE_DIR)/core/core_theme
CORE_FONT_DIR := $(SHARED_WORKSPACE_DIR)/core/core_font
CORE_PACK_DIR := $(SHARED_WORKSPACE_DIR)/core/core_pack
CORE_PANE_DIR := $(SHARED_WORKSPACE_DIR)/core/core_pane
CORE_LAYOUT_DIR := $(SHARED_WORKSPACE_DIR)/core/core_layout
CORE_PANE_MODULE_DIR := $(SHARED_WORKSPACE_DIR)/core/core_pane_module
CORE_VIEWPORT2D_DIR := $(SHARED_WORKSPACE_DIR)/core/core_viewport2d
KIT_RENDER_DIR := $(SHARED_WORKSPACE_DIR)/kit/kit_render
KIT_PANE_DIR := $(SHARED_WORKSPACE_DIR)/kit/kit_pane
KIT_WORKSPACE_AUTHORING_DIR := $(SHARED_WORKSPACE_DIR)/kit/kit_workspace_authoring
endif

SDL_CFLAGS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --cflags sdl2 2>/dev/null)
SDL_LIBS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --libs sdl2 2>/dev/null)
SDL_TTF_CFLAGS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --cflags sdl2_ttf 2>/dev/null)
SDL_TTF_LIBS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --libs sdl2_ttf 2>/dev/null)
PNG_CFLAGS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --cflags libpng 2>/dev/null)
PNG_LIBS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --libs libpng 2>/dev/null)
JSON_CFLAGS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --cflags json-c 2>/dev/null)
JSON_LIBS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --libs json-c 2>/dev/null)
ifeq ($(strip $(SDL_CFLAGS)),)
  ifneq ($(wildcard $(TARGET_HOMEBREW_PREFIX)/include/SDL2/SDL.h),)
    SDL_CFLAGS := -I$(TARGET_HOMEBREW_PREFIX)/include -D_THREAD_SAFE
  else ifneq ($(wildcard $(TARGET_ALT_HOMEBREW_PREFIX)/include/SDL2/SDL.h),)
    SDL_CFLAGS := -I$(TARGET_ALT_HOMEBREW_PREFIX)/include -D_THREAD_SAFE
  endif
endif
ifeq ($(strip $(SDL_LIBS)),)
  ifneq ($(wildcard $(TARGET_HOMEBREW_PREFIX)/lib/libSDL2.dylib),)
    SDL_LIBS := -L$(TARGET_HOMEBREW_PREFIX)/lib -lSDL2
  else ifneq ($(wildcard $(TARGET_ALT_HOMEBREW_PREFIX)/lib/libSDL2.dylib),)
    SDL_LIBS := -L$(TARGET_ALT_HOMEBREW_PREFIX)/lib -lSDL2
  else
    SDL_LIBS := -framework SDL2
  endif
endif
ifeq ($(strip $(SDL_TTF_CFLAGS)),)
  ifneq ($(wildcard $(TARGET_HOMEBREW_PREFIX)/include/SDL2/SDL_ttf.h),)
    SDL_TTF_CFLAGS := -I$(TARGET_HOMEBREW_PREFIX)/include -D_THREAD_SAFE
  else ifneq ($(wildcard $(TARGET_ALT_HOMEBREW_PREFIX)/include/SDL2/SDL_ttf.h),)
    SDL_TTF_CFLAGS := -I$(TARGET_ALT_HOMEBREW_PREFIX)/include -D_THREAD_SAFE
  endif
endif
ifeq ($(strip $(SDL_TTF_LIBS)),)
  ifneq ($(wildcard $(TARGET_HOMEBREW_PREFIX)/lib/libSDL2_ttf.dylib),)
    SDL_TTF_LIBS := -L$(TARGET_HOMEBREW_PREFIX)/lib -lSDL2_ttf
  else ifneq ($(wildcard $(TARGET_ALT_HOMEBREW_PREFIX)/lib/libSDL2_ttf.dylib),)
    SDL_TTF_LIBS := -L$(TARGET_ALT_HOMEBREW_PREFIX)/lib -lSDL2_ttf
  else
    SDL_TTF_LIBS :=
  endif
endif
ifeq ($(strip $(PNG_CFLAGS)),)
  ifneq ($(wildcard $(TARGET_HOMEBREW_PREFIX)/include/png.h),)
    PNG_CFLAGS := -I$(TARGET_HOMEBREW_PREFIX)/include
  else ifneq ($(wildcard $(TARGET_ALT_HOMEBREW_PREFIX)/include/png.h),)
    PNG_CFLAGS := -I$(TARGET_ALT_HOMEBREW_PREFIX)/include
  endif
endif
ifeq ($(strip $(PNG_LIBS)),)
  ifneq ($(wildcard $(TARGET_HOMEBREW_PREFIX)/lib/libpng.dylib),)
    PNG_LIBS := -L$(TARGET_HOMEBREW_PREFIX)/lib -lpng
  else ifneq ($(wildcard $(TARGET_HOMEBREW_PREFIX)/lib/libpng16.dylib),)
    PNG_LIBS := -L$(TARGET_HOMEBREW_PREFIX)/lib -lpng16
  else ifneq ($(wildcard $(TARGET_ALT_HOMEBREW_PREFIX)/lib/libpng.dylib),)
    PNG_LIBS := -L$(TARGET_ALT_HOMEBREW_PREFIX)/lib -lpng
  else ifneq ($(wildcard $(TARGET_ALT_HOMEBREW_PREFIX)/lib/libpng16.dylib),)
    PNG_LIBS := -L$(TARGET_ALT_HOMEBREW_PREFIX)/lib -lpng16
  endif
endif
ifeq ($(strip $(JSON_CFLAGS)),)
  ifneq ($(wildcard $(TARGET_HOMEBREW_PREFIX)/include/json-c/json.h),)
    JSON_CFLAGS := -I$(TARGET_HOMEBREW_PREFIX)/include
  else ifneq ($(wildcard $(TARGET_ALT_HOMEBREW_PREFIX)/include/json-c/json.h),)
    JSON_CFLAGS := -I$(TARGET_ALT_HOMEBREW_PREFIX)/include
  endif
endif
ifeq ($(strip $(JSON_LIBS)),)
  ifneq ($(wildcard $(TARGET_HOMEBREW_PREFIX)/lib/libjson-c.dylib),)
    JSON_LIBS := -L$(TARGET_HOMEBREW_PREFIX)/lib -ljson-c
  else ifneq ($(wildcard $(TARGET_HOMEBREW_PREFIX)/lib/libjson-c.5.dylib),)
    JSON_LIBS := -L$(TARGET_HOMEBREW_PREFIX)/lib -ljson-c
  else ifneq ($(wildcard $(TARGET_ALT_HOMEBREW_PREFIX)/lib/libjson-c.dylib),)
    JSON_LIBS := -L$(TARGET_ALT_HOMEBREW_PREFIX)/lib -ljson-c
  else ifneq ($(wildcard $(TARGET_ALT_HOMEBREW_PREFIX)/lib/libjson-c.5.dylib),)
    JSON_LIBS := -L$(TARGET_ALT_HOMEBREW_PREFIX)/lib -ljson-c
  else
    JSON_LIBS := -ljson-c
  endif
endif

COMMON_CFLAGS := -std=c11 -Wall -Wextra -pedantic \
	-Iinclude \
	-I$(CORE_BASE_DIR)/include \
	-I$(CORE_OBJECT_DIR)/include \
	-I$(CORE_UNITS_DIR)/include \
	-I$(CORE_SCENE_DIR)/include \
	-I$(CORE_THEME_DIR)/include \
	-I$(CORE_FONT_DIR)/include \
	-I$(CORE_PACK_DIR)/include \
	-I$(CORE_PANE_DIR)/include \
	-I$(CORE_LAYOUT_DIR)/include \
	-I$(CORE_PANE_MODULE_DIR)/include \
	-I$(CORE_VIEWPORT2D_DIR)/include \
	-I$(KIT_RENDER_DIR)/include \
	-I$(KIT_PANE_DIR)/include \
	-I$(KIT_WORKSPACE_AUTHORING_DIR)/include \
	$(SDL_CFLAGS) \
	$(JSON_CFLAGS) \
	$(PNG_CFLAGS) \
	$(SDL_TTF_CFLAGS)
PROGRAM_CFLAGS := $(COMMON_CFLAGS)
PROGRAM_DEPFLAGS :=
ifeq ($(BUILD_TOOLCHAIN),clang)
PROGRAM_CFLAGS += $(ARCH_FLAGS)
PROGRAM_DEPFLAGS := -MMD -MP
endif
HOST_CFLAGS := $(COMMON_CFLAGS) $(ARCH_FLAGS)
HOST_DEPFLAGS := -MMD -MP
LDLIBS := -lm
APP_LDLIBS := $(LDLIBS) $(SDL_LIBS) $(SDL_TTF_LIBS) $(PNG_LIBS) $(JSON_LIBS)

BUILD_DIR := build
TARGET_BUILD_DIR := $(BUILD_DIR)/targets/$(TARGET_TRIPLE)
HOST_TEST_DIR := $(TARGET_BUILD_DIR)/host/tests
TEST_OBJ_DIR := $(HOST_TEST_DIR)/obj
TEST_BIN_DIR := $(HOST_TEST_DIR)/bin
PROGRAM_BUILD_DIR := $(TARGET_BUILD_DIR)/toolchains/$(BUILD_TOOLCHAIN)
PROGRAM_OBJ_DIR := $(PROGRAM_BUILD_DIR)/obj
PROGRAM_BIN_DIR := $(PROGRAM_BUILD_DIR)/bin
PROGRAM_CC_STAMP := $(PROGRAM_BUILD_DIR)/compiler.stamp
SHARED_BUILD_DIR := $(TARGET_BUILD_DIR)/shared

ifeq ($(BUILD_TOOLCHAIN),clang)
PROGRAM_CC := $(HOST_CC)
PROGRAM_CC_DEP :=
else ifeq ($(BUILD_TOOLCHAIN),fisics)
PROGRAM_CC := $(FISICS_CC)
PROGRAM_CC_DEP := $(FISICS_CC)
else
$(error Unsupported BUILD_TOOLCHAIN '$(BUILD_TOOLCHAIN)'; expected clang or fisics)
endif

ifneq ($(filter $(PACKAGE_TOOLCHAIN),clang fisics),$(PACKAGE_TOOLCHAIN))
$(error Unsupported PACKAGE_TOOLCHAIN '$(PACKAGE_TOOLCHAIN)'; expected clang or fisics)
endif

APP_LOCAL_SRCS := \
	src/domain/drawing_program_color_model.c \
	src/domain/drawing_program_document.c \
	src/domain/drawing_program_editor_state.c \
	src/domain/drawing_program_history.c \
	src/domain/drawing_program_history_raster_deltas.c \
	src/domain/drawing_program_history_object_commands.c \
	src/domain/drawing_program_layer_raster.c \
	src/domain/drawing_program_object_geometry.c \
	src/domain/drawing_program_object_rasterize.c \
	src/domain/drawing_program_object_store.c \
	src/domain/drawing_program_object_store_path_hit_test.c \
	src/domain/drawing_program_object_selection.c \
	src/domain/drawing_program_texture_layer_material_intent.c \
	src/domain/drawing_program_texture_layer_role.c \
	src/domain/drawing_program_texture_net.c \
	src/domain/drawing_program_texture_export_intent.c \
	src/domain/drawing_program_texture_overlay_material_intent.c \
	src/domain/drawing_program_texture_project.c \
	src/domain/drawing_program_texture_project_template.c \
	src/domain/drawing_program_texture_workspace.c \
	src/domain/drawing_program_object_transform.c \
	src/domain/drawing_program_selection.c \
	src/domain/drawing_program_selection_edit_ops.c \
	src/io/export/drawing_program_export_image.c \
	src/io/export/drawing_program_icns_export.c \
	src/io/export/drawing_program_iconset_export.c \
	src/io/export/drawing_program_png_export.c \
	src/io/export/drawing_program_texture_export.c \
	src/io/session/drawing_program_project_state.c \
	src/io/session/drawing_program_session_prefs.c \
	src/io/session/drawing_program_native_dialogs.c \
	src/io/session/drawing_program_snapshot.c \
	src/io/session/drawing_program_snapshot_history_raster_delta_chunk.c \
	src/io/session/drawing_program_snapshot_layer_chunk.c \
	src/io/session/drawing_program_snapshot_object_chunk.c \
	src/io/session/drawing_program_snapshot_shell.c \
	src/io/session/drawing_program_snapshot_export_json.c \
	src/io/session/drawing_program_texture_canvas_ops.c \
	src/io/session/drawing_program_texture_project_session.c \
	src/io/session/drawing_program_texture_project_snapshot.c \
	src/io/session/drawing_program_texture_scene_browser.c \
	src/io/session/drawing_program_texture_scene_import.c \
	src/io/session/drawing_program_snapshot_ui_settings.c \
	src/io/session/drawing_program_snapshot_bridge.c \
	src/domain/drawing_program_viewport.c \
	src/runtime/render/drawing_program_render_domain.c \
	src/runtime/render/drawing_program_render_backend.c \
	src/runtime/render/drawing_program_render_revision.c \
	src/runtime/render/drawing_program_render_zoom_bucket.c \
	src/runtime/orchestration/drawing_program_runtime_orchestration.c \
	src/runtime/canvas/drawing_program_visual_canvas_stroke_ops.c \
	src/runtime/canvas/drawing_program_canvas_reflection.c \
	src/runtime/adapters/drawing_program_authoring_host.c \
	src/runtime/adapters/drawing_program_pane_host.c \
	src/runtime/adapters/drawing_program_overlay_adapter.c \
	src/render/canvas/drawing_program_visual_canvas_world_render.c \
	src/render/canvas/drawing_program_texture_net_guides.c \
	src/render/frame/drawing_program_visual_authoring_chrome.c \
	src/render/frame/drawing_program_visual_frame_render.c \
	src/render/overlay/drawing_program_visual_overlay_shared.c \
	src/render/overlay/drawing_program_visual_object_overlay.c \
	src/render/overlay/drawing_program_visual_object_overlay_shapes.c \
	src/render/overlay/drawing_program_visual_object_overlay_path_controls.c \
	src/render/overlay/drawing_program_visual_selection_overlay.c \
	src/render/overlay/drawing_program_visual_overlay_render.c \
	src/input/canvas/drawing_program_visual_canvas_action_ops.c \
	src/input/canvas/drawing_program_visual_canvas_coords.c \
	src/input/canvas/drawing_program_texture_canvas_resize.c \
	src/input/canvas/drawing_program_texture_canvas_move.c \
	src/input/canvas/drawing_program_visual_canvas_draw_action_ops.c \
	src/input/panel/drawing_program_visual_input_panel_color.c \
	src/input/panel/drawing_program_visual_input_right_file_tabs.c \
	src/input/panel/drawing_program_visual_input_panel_clicks.c \
	src/input/input_core/drawing_program_visual_input_selection_ops.c \
	src/input/tools/drawing_program_visual_shape_ops.c \
	src/input/tools/drawing_program_visual_transform_ops.c \
	src/input/layers/drawing_program_visual_layer_actions.c \
	src/input/layers/drawing_program_visual_layer_roles.c \
	src/input/input_core/drawing_program_visual_input_core.c \
	src/input/input_core/drawing_program_visual_input_keydown.c \
	src/input/input_core/drawing_program_visual_input_keymap.c \
	src/input/input_core/drawing_program_visual_input_handlers.c \
	src/ui/layout/drawing_program_visual_layout.c \
	src/ui/layout/drawing_program_visual_layout_color.c \
	src/ui/layers/drawing_program_visual_layer_opacity.c \
	src/ui/pane/drawing_program_visual_pane_bindings.c \
	src/ui/panel/drawing_program_visual_panel_render_common.c \
	src/ui/panel/drawing_program_visual_panel_render.c \
	src/ui/panel/drawing_program_visual_right_panel_color_render.c \
	src/ui/panel/drawing_program_visual_right_panel_file_tabs_render.c \
	src/ui/panel/drawing_program_visual_right_panel_render.c \
	src/ui/theme/drawing_program_visual_theme.c \
	src/ui/resources/drawing_program_visual_resources.c \
	src/ui/resources/drawing_program_visual_surface_cache.c \
	src/app/drawing_program_visual_runtime_debug.c \
	src/app/drawing_program_visual_loop_diag.c \
	src/app/drawing_program_visual_loop_policy.c \
	src/app/drawing_program_app_post_load.c \
	src/app/drawing_program_app_session.c \
	src/app/drawing_program_app_runtime.c \
	src/ui/tools/drawing_program_visual_tool_options.c \
	src/ui/text/drawing_program_visual_text_render.c \
	src/app/drawing_program_ui_color_state.c \
	src/app/drawing_program_app_main.c \
	src/app/main.c \
	src/app/drawing_program_app_visual_runtime.c \
	src/app/drawing_program_app_visual_runtime_loop.c \
	src/app/drawing_program_app_visual_main.c

HEADLESS_LOCAL_SRCS := \
	src/domain/drawing_program_color_model.c \
	src/domain/drawing_program_document.c \
	src/domain/drawing_program_editor_state.c \
	src/domain/drawing_program_history.c \
	src/domain/drawing_program_history_raster_deltas.c \
	src/domain/drawing_program_history_object_commands.c \
	src/domain/drawing_program_layer_raster.c \
	src/domain/drawing_program_object_geometry.c \
	src/domain/drawing_program_object_rasterize.c \
	src/domain/drawing_program_object_store.c \
	src/domain/drawing_program_object_store_path_hit_test.c \
	src/domain/drawing_program_object_selection.c \
	src/domain/drawing_program_texture_layer_material_intent.c \
	src/domain/drawing_program_texture_layer_role.c \
	src/domain/drawing_program_texture_net.c \
	src/domain/drawing_program_texture_export_intent.c \
	src/domain/drawing_program_texture_overlay_material_intent.c \
	src/domain/drawing_program_texture_project.c \
	src/domain/drawing_program_texture_project_template.c \
	src/domain/drawing_program_texture_workspace.c \
	src/domain/drawing_program_object_transform.c \
	src/domain/drawing_program_selection.c \
	src/domain/drawing_program_selection_edit_ops.c \
	src/io/export/drawing_program_export_image.c \
	src/io/export/drawing_program_icns_export.c \
	src/io/export/drawing_program_iconset_export.c \
	src/io/export/drawing_program_png_export.c \
	src/io/export/drawing_program_texture_export.c \
	src/io/session/drawing_program_project_state.c \
	src/io/session/drawing_program_session_prefs.c \
	src/io/session/drawing_program_native_dialogs.c \
	src/io/session/drawing_program_snapshot.c \
	src/io/session/drawing_program_snapshot_history_raster_delta_chunk.c \
	src/io/session/drawing_program_snapshot_layer_chunk.c \
	src/io/session/drawing_program_snapshot_object_chunk.c \
	src/io/session/drawing_program_snapshot_shell.c \
	src/io/session/drawing_program_snapshot_export_json.c \
	src/io/session/drawing_program_texture_canvas_ops.c \
	src/io/session/drawing_program_texture_project_session.c \
	src/io/session/drawing_program_texture_project_snapshot.c \
	src/io/session/drawing_program_texture_scene_browser.c \
	src/io/session/drawing_program_texture_scene_import.c \
	src/io/session/drawing_program_snapshot_ui_settings.c \
	src/io/session/drawing_program_snapshot_bridge.c \
	src/domain/drawing_program_viewport.c \
	src/runtime/render/drawing_program_render_domain.c \
	src/runtime/render/drawing_program_render_backend.c \
	src/runtime/render/drawing_program_render_revision.c \
	src/runtime/render/drawing_program_render_zoom_bucket.c \
	src/runtime/orchestration/drawing_program_runtime_orchestration.c \
	src/runtime/canvas/drawing_program_visual_canvas_stroke_ops.c \
	src/runtime/canvas/drawing_program_canvas_reflection.c \
	src/runtime/adapters/drawing_program_authoring_host.c \
	src/runtime/adapters/drawing_program_pane_host.c \
	src/runtime/adapters/drawing_program_overlay_adapter.c \
	src/render/canvas/drawing_program_visual_canvas_world_render.c \
	src/render/canvas/drawing_program_texture_net_guides.c \
	src/render/frame/drawing_program_visual_authoring_chrome.c \
	src/render/frame/drawing_program_visual_frame_render.c \
	src/render/overlay/drawing_program_visual_overlay_shared.c \
	src/render/overlay/drawing_program_visual_object_overlay.c \
	src/render/overlay/drawing_program_visual_object_overlay_shapes.c \
	src/render/overlay/drawing_program_visual_object_overlay_path_controls.c \
	src/render/overlay/drawing_program_visual_selection_overlay.c \
	src/render/overlay/drawing_program_visual_overlay_render.c \
	src/input/canvas/drawing_program_visual_canvas_action_ops.c \
	src/input/canvas/drawing_program_visual_canvas_coords.c \
	src/input/canvas/drawing_program_texture_canvas_resize.c \
	src/input/canvas/drawing_program_texture_canvas_move.c \
	src/input/canvas/drawing_program_visual_canvas_draw_action_ops.c \
	src/input/panel/drawing_program_visual_input_panel_color.c \
	src/input/panel/drawing_program_visual_input_right_file_tabs.c \
	src/input/panel/drawing_program_visual_input_panel_clicks.c \
	src/input/input_core/drawing_program_visual_input_selection_ops.c \
	src/input/tools/drawing_program_visual_shape_ops.c \
	src/input/tools/drawing_program_visual_transform_ops.c \
	src/input/layers/drawing_program_visual_layer_actions.c \
	src/input/layers/drawing_program_visual_layer_roles.c \
	src/input/input_core/drawing_program_visual_input_core.c \
	src/input/input_core/drawing_program_visual_input_keydown.c \
	src/input/input_core/drawing_program_visual_input_keymap.c \
	src/input/input_core/drawing_program_visual_input_handlers.c \
	src/ui/layout/drawing_program_visual_layout.c \
	src/ui/layout/drawing_program_visual_layout_color.c \
	src/ui/layers/drawing_program_visual_layer_opacity.c \
	src/ui/pane/drawing_program_visual_pane_bindings.c \
	src/ui/panel/drawing_program_visual_panel_render_common.c \
	src/ui/panel/drawing_program_visual_panel_render.c \
	src/ui/panel/drawing_program_visual_right_panel_color_render.c \
	src/ui/panel/drawing_program_visual_right_panel_file_tabs_render.c \
	src/ui/panel/drawing_program_visual_right_panel_render.c \
	src/ui/theme/drawing_program_visual_theme.c \
	src/ui/resources/drawing_program_visual_resources.c \
	src/ui/resources/drawing_program_visual_surface_cache.c \
	src/app/drawing_program_visual_runtime_debug.c \
	src/app/drawing_program_visual_loop_diag.c \
	src/app/drawing_program_visual_loop_policy.c \
	src/ui/tools/drawing_program_visual_tool_options.c \
	src/ui/text/drawing_program_visual_text_render.c \
	src/app/drawing_program_ui_color_state.c \
	src/app/drawing_program_app_post_load.c \
	src/app/drawing_program_app_session.c \
	src/app/drawing_program_app_runtime.c \
	src/app/drawing_program_app_main.c \
	src/app/drawing_program_app_headless_main.c

TEST_LOCAL_SRCS := \
	src/domain/drawing_program_color_model.c \
	src/domain/drawing_program_document.c \
	src/domain/drawing_program_editor_state.c \
	src/domain/drawing_program_history.c \
	src/domain/drawing_program_history_raster_deltas.c \
	src/domain/drawing_program_history_object_commands.c \
	src/domain/drawing_program_layer_raster.c \
	src/domain/drawing_program_object_geometry.c \
	src/domain/drawing_program_object_rasterize.c \
	src/domain/drawing_program_object_store.c \
	src/domain/drawing_program_object_store_path_hit_test.c \
	src/domain/drawing_program_object_selection.c \
	src/domain/drawing_program_texture_layer_material_intent.c \
	src/domain/drawing_program_texture_layer_role.c \
	src/domain/drawing_program_texture_net.c \
	src/domain/drawing_program_texture_export_intent.c \
	src/domain/drawing_program_texture_overlay_material_intent.c \
	src/domain/drawing_program_texture_project.c \
	src/domain/drawing_program_texture_project_template.c \
	src/domain/drawing_program_texture_workspace.c \
	src/domain/drawing_program_object_transform.c \
	src/domain/drawing_program_selection.c \
	src/domain/drawing_program_selection_edit_ops.c \
	src/io/export/drawing_program_export_image.c \
	src/io/export/drawing_program_icns_export.c \
	src/io/export/drawing_program_iconset_export.c \
	src/io/export/drawing_program_png_export.c \
	src/io/export/drawing_program_texture_export.c \
	src/io/session/drawing_program_project_state.c \
	src/io/session/drawing_program_session_prefs.c \
	src/io/session/drawing_program_native_dialogs.c \
	src/io/session/drawing_program_snapshot.c \
	src/io/session/drawing_program_snapshot_history_raster_delta_chunk.c \
	src/io/session/drawing_program_snapshot_layer_chunk.c \
	src/io/session/drawing_program_snapshot_object_chunk.c \
	src/io/session/drawing_program_snapshot_shell.c \
	src/io/session/drawing_program_snapshot_export_json.c \
	src/io/session/drawing_program_texture_canvas_ops.c \
	src/io/session/drawing_program_texture_project_session.c \
	src/io/session/drawing_program_texture_project_snapshot.c \
	src/io/session/drawing_program_texture_scene_browser.c \
	src/io/session/drawing_program_texture_scene_import.c \
	src/io/session/drawing_program_snapshot_ui_settings.c \
	src/io/session/drawing_program_snapshot_bridge.c \
	src/domain/drawing_program_viewport.c \
	src/runtime/render/drawing_program_render_domain.c \
	src/runtime/render/drawing_program_render_backend.c \
	src/runtime/render/drawing_program_render_revision.c \
	src/runtime/render/drawing_program_render_zoom_bucket.c \
	src/runtime/orchestration/drawing_program_runtime_orchestration.c \
	src/runtime/canvas/drawing_program_visual_canvas_stroke_ops.c \
	src/runtime/canvas/drawing_program_canvas_reflection.c \
	src/runtime/adapters/drawing_program_authoring_host.c \
	src/runtime/adapters/drawing_program_pane_host.c \
	src/runtime/adapters/drawing_program_overlay_adapter.c \
	src/render/canvas/drawing_program_visual_canvas_world_render.c \
	src/render/canvas/drawing_program_texture_net_guides.c \
	src/render/frame/drawing_program_visual_authoring_chrome.c \
	src/render/frame/drawing_program_visual_frame_render.c \
	src/render/overlay/drawing_program_visual_overlay_shared.c \
	src/render/overlay/drawing_program_visual_object_overlay.c \
	src/render/overlay/drawing_program_visual_object_overlay_shapes.c \
	src/render/overlay/drawing_program_visual_object_overlay_path_controls.c \
	src/render/overlay/drawing_program_visual_selection_overlay.c \
	src/render/overlay/drawing_program_visual_overlay_render.c \
	src/input/canvas/drawing_program_visual_canvas_action_ops.c \
	src/input/canvas/drawing_program_visual_canvas_coords.c \
	src/input/canvas/drawing_program_texture_canvas_resize.c \
	src/input/canvas/drawing_program_texture_canvas_move.c \
	src/input/canvas/drawing_program_visual_canvas_draw_action_ops.c \
	src/input/panel/drawing_program_visual_input_panel_color.c \
	src/input/panel/drawing_program_visual_input_right_file_tabs.c \
	src/input/panel/drawing_program_visual_input_panel_clicks.c \
	src/input/input_core/drawing_program_visual_input_selection_ops.c \
	src/input/tools/drawing_program_visual_shape_ops.c \
	src/input/tools/drawing_program_visual_transform_ops.c \
	src/input/layers/drawing_program_visual_layer_actions.c \
	src/input/layers/drawing_program_visual_layer_roles.c \
	src/input/input_core/drawing_program_visual_input_core.c \
	src/input/input_core/drawing_program_visual_input_keydown.c \
	src/input/input_core/drawing_program_visual_input_keymap.c \
	src/input/input_core/drawing_program_visual_input_handlers.c \
	src/ui/layout/drawing_program_visual_layout.c \
	src/ui/layout/drawing_program_visual_layout_color.c \
	src/ui/layers/drawing_program_visual_layer_opacity.c \
	src/ui/pane/drawing_program_visual_pane_bindings.c \
	src/ui/panel/drawing_program_visual_panel_render_common.c \
	src/ui/panel/drawing_program_visual_panel_render.c \
	src/ui/panel/drawing_program_visual_right_panel_color_render.c \
	src/ui/panel/drawing_program_visual_right_panel_file_tabs_render.c \
	src/ui/panel/drawing_program_visual_right_panel_render.c \
	src/ui/theme/drawing_program_visual_theme.c \
	src/ui/resources/drawing_program_visual_resources.c \
	src/ui/resources/drawing_program_visual_surface_cache.c \
	src/app/drawing_program_visual_runtime_debug.c \
	src/app/drawing_program_visual_loop_diag.c \
	src/app/drawing_program_visual_loop_policy.c \
	src/app/drawing_program_app_post_load.c \
	src/app/drawing_program_app_session.c \
	src/app/drawing_program_app_runtime.c \
	src/ui/tools/drawing_program_visual_tool_options.c \
	src/ui/text/drawing_program_visual_text_render.c \
	src/app/drawing_program_ui_color_state.c \
	src/app/drawing_program_app_main.c \
	src/app/drawing_program_app_visual_runtime.c \
	src/app/drawing_program_app_visual_runtime_loop.c \
	src/app/drawing_program_app_visual_main.c \
	tests/drawing_program_lifecycle_snapshot_suite.c \
	tests/drawing_program_lifecycle_snapshot_shell_suite.c \
	tests/drawing_program_lifecycle_snapshot_layer_suite.c \
	tests/drawing_program_lifecycle_snapshot_object_suite.c \
	tests/drawing_program_lifecycle_export_suite.c \
	tests/drawing_program_lifecycle_texture_export_suite.c \
	tests/drawing_program_lifecycle_snapshot_helpers.c \
	tests/drawing_program_lifecycle_snapshot_object_helpers.c \
	tests/drawing_program_lifecycle_selection_layer_suite.c \
	tests/drawing_program_lifecycle_selection_payload_suite.c \
	tests/drawing_program_lifecycle_authoring_host_suite.c \
	tests/drawing_program_lifecycle_baseline_history_suite.c \
	tests/drawing_program_lifecycle_object_path_suite.c \
	tests/drawing_program_lifecycle_object_path_history_suite.c \
	tests/drawing_program_lifecycle_object_path_history_mutation_suite.c \
	tests/drawing_program_lifecycle_runtime_render_suite.c \
	tests/drawing_program_lifecycle_runtime_path_suite.c \
	tests/drawing_program_lifecycle_runtime_path_pointer_suite.c \
	tests/drawing_program_lifecycle_texture_import_suite.c \
	tests/drawing_program_lifecycle_texture_workspace_suite.c \
	tests/drawing_program_lifecycle_runtime_ui_suite.c \
	tests/drawing_program_lifecycle_color_panel_suite.c \
	tests/drawing_program_lifecycle_test_support.c \
	tests/drawing_program_lifecycle_test.c

APP_OBJS := $(APP_LOCAL_SRCS:%=$(PROGRAM_OBJ_DIR)/%.o)
HEADLESS_OBJS := $(HEADLESS_LOCAL_SRCS:%=$(PROGRAM_OBJ_DIR)/%.o)
TEST_OBJS := $(TEST_LOCAL_SRCS:%=$(TEST_OBJ_DIR)/%.o)
APP_DEPS := $(APP_OBJS:.o=.d)
HEADLESS_DEPS := $(HEADLESS_OBJS:.o=.d)
TEST_DEPS := $(TEST_OBJS:.o=.d)

define app_bin_for
$(TARGET_BUILD_DIR)/toolchains/$(1)/bin/$(APP_BIN)
endef

APP_TARGET := $(PROGRAM_BIN_DIR)/$(APP_BIN)
HEADLESS_TARGET := $(PROGRAM_BIN_DIR)/drawing-program-headless
TEST_TARGET := $(TEST_BIN_DIR)/drawing-program-test
PACKAGE_SOURCE_BIN := $(call app_bin_for,$(PACKAGE_TOOLCHAIN))
SHARED_CC := $(HOST_CC) $(ARCH_FLAGS)

CORE_BASE_LOCAL_SRCS := $(wildcard $(CORE_BASE_DIR)/src/*.c) $(wildcard $(CORE_BASE_DIR)/include/*.h)
CORE_OBJECT_LOCAL_SRCS := $(wildcard $(CORE_OBJECT_DIR)/src/*.c) $(wildcard $(CORE_OBJECT_DIR)/include/*.h)
CORE_UNITS_LOCAL_SRCS := $(wildcard $(CORE_UNITS_DIR)/src/*.c) $(wildcard $(CORE_UNITS_DIR)/include/*.h)
CORE_SCENE_LOCAL_SRCS := $(wildcard $(CORE_SCENE_DIR)/src/*.c) $(wildcard $(CORE_SCENE_DIR)/include/*.h)
CORE_THEME_LOCAL_SRCS := $(wildcard $(CORE_THEME_DIR)/src/*.c) $(wildcard $(CORE_THEME_DIR)/include/*.h)
CORE_FONT_LOCAL_SRCS := $(wildcard $(CORE_FONT_DIR)/src/*.c) $(wildcard $(CORE_FONT_DIR)/include/*.h)
CORE_PACK_LOCAL_SRCS := $(wildcard $(CORE_PACK_DIR)/src/*.c) $(wildcard $(CORE_PACK_DIR)/include/*.h)
CORE_PANE_LOCAL_SRCS := $(wildcard $(CORE_PANE_DIR)/src/*.c) $(wildcard $(CORE_PANE_DIR)/include/*.h)
CORE_LAYOUT_LOCAL_SRCS := $(wildcard $(CORE_LAYOUT_DIR)/src/*.c) $(wildcard $(CORE_LAYOUT_DIR)/include/*.h)
CORE_PANE_MODULE_LOCAL_SRCS := $(wildcard $(CORE_PANE_MODULE_DIR)/src/*.c) $(wildcard $(CORE_PANE_MODULE_DIR)/include/*.h)
CORE_VIEWPORT2D_LOCAL_SRCS := $(wildcard $(CORE_VIEWPORT2D_DIR)/src/*.c) $(wildcard $(CORE_VIEWPORT2D_DIR)/include/*.h)
KIT_RENDER_LOCAL_SRCS := $(wildcard $(KIT_RENDER_DIR)/src/*.c) $(wildcard $(KIT_RENDER_DIR)/include/*.h)
KIT_PANE_LOCAL_SRCS := $(wildcard $(KIT_PANE_DIR)/src/*.c) $(wildcard $(KIT_PANE_DIR)/include/*.h)
KIT_WORKSPACE_AUTHORING_LOCAL_SRCS := $(wildcard $(KIT_WORKSPACE_AUTHORING_DIR)/src/*.c) $(wildcard $(KIT_WORKSPACE_AUTHORING_DIR)/src/ui/*.c) $(wildcard $(KIT_WORKSPACE_AUTHORING_DIR)/include/*.h)

CORE_BASE_LIB := $(SHARED_BUILD_DIR)/libcore_base.a
CORE_OBJECT_LIB := $(SHARED_BUILD_DIR)/libcore_object.a
CORE_UNITS_LIB := $(SHARED_BUILD_DIR)/libcore_units.a
CORE_SCENE_LIB := $(SHARED_BUILD_DIR)/libcore_scene.a
CORE_THEME_LIB := $(SHARED_BUILD_DIR)/libcore_theme.a
CORE_FONT_LIB := $(SHARED_BUILD_DIR)/libcore_font.a
CORE_PACK_LIB := $(SHARED_BUILD_DIR)/libcore_pack.a
CORE_PANE_LIB := $(SHARED_BUILD_DIR)/libcore_pane.a
CORE_LAYOUT_LIB := $(SHARED_BUILD_DIR)/libcore_layout.a
CORE_PANE_MODULE_LIB := $(SHARED_BUILD_DIR)/libcore_pane_module.a
CORE_VIEWPORT2D_LIB := $(SHARED_BUILD_DIR)/libcore_viewport2d.a
KIT_RENDER_LIB := $(SHARED_BUILD_DIR)/libkit_render.a
KIT_PANE_LIB := $(SHARED_BUILD_DIR)/libkit_pane.a
KIT_WORKSPACE_AUTHORING_LIB := $(SHARED_BUILD_DIR)/libkit_workspace_authoring.a
SHARED_LIBS := $(KIT_WORKSPACE_AUTHORING_LIB) $(KIT_PANE_LIB) $(KIT_RENDER_LIB) $(CORE_PACK_LIB) $(CORE_PANE_LIB) $(CORE_LAYOUT_LIB) $(CORE_PANE_MODULE_LIB) $(CORE_THEME_LIB) $(CORE_FONT_LIB) $(CORE_VIEWPORT2D_LIB) $(CORE_SCENE_LIB) $(CORE_OBJECT_LIB) $(CORE_UNITS_LIB) $(CORE_BASE_LIB)

DIST_DIR := dist
PACKAGE_APP_NAME := sketCh.app
PACKAGE_APP_DIR := $(DIST_DIR)/$(PACKAGE_APP_NAME)
PACKAGE_CONTENTS_DIR := $(PACKAGE_APP_DIR)/Contents
PACKAGE_MACOS_DIR := $(PACKAGE_CONTENTS_DIR)/MacOS
PACKAGE_RESOURCES_DIR := $(PACKAGE_CONTENTS_DIR)/Resources
PACKAGE_FRAMEWORKS_DIR := $(PACKAGE_CONTENTS_DIR)/Frameworks
PACKAGE_SHARED_FONTS_DIR := $(PACKAGE_RESOURCES_DIR)/shared/assets/fonts
PACKAGE_INFO_PLIST_SRC := tools/packaging/macos/Info.plist
PACKAGE_LAUNCHER_SRC := tools/packaging/macos/sketch-launcher
PACKAGE_DYLIB_BUNDLER := tools/packaging/macos/bundle-dylibs.sh
PACKAGE_LOCAL_ICON_DIR := tools/packaging/macos/local_app_icon
PACKAGE_APP_ICON_NAME := AppIcon
PACKAGE_APP_ICON_FILE := $(PACKAGE_APP_ICON_NAME).icns
PACKAGE_APP_ICON_SRC ?= $(PACKAGE_LOCAL_ICON_DIR)/$(PACKAGE_APP_ICON_FILE)
PACKAGE_APP_ICONSET_SRC ?= $(PACKAGE_LOCAL_ICON_DIR)/$(PACKAGE_APP_ICON_NAME).iconset
PACKAGE_BUNDLED_ICON_PATH := $(PACKAGE_RESOURCES_DIR)/$(PACKAGE_APP_ICON_FILE)
PACKAGE_ADHOC_SIGN_IDENTITY ?= -
DESKTOP_APP_DIR ?= $(HOME)/Desktop/$(PACKAGE_APP_NAME)
EXPORT_PRESET ?= data/last_session.pack
EXPORT_JSON ?= /tmp/drawing_program_snapshot_debug.json
WORKSPACE_PRESET ?= ../workspace_sandbox/data/presets/sketch_layout_v1.pack
PACKAGE_FONTS_SRC_PRIMARY := $(SHARED_VENDOR_DIR)/assets/fonts
PACKAGE_FONTS_SRC_WORKSPACE := $(SHARED_WORKSPACE_DIR)/assets/fonts
RELEASE_VERSION := $(strip $(shell cat "$(VERSION_FILE)" 2>/dev/null))
ifeq ($(RELEASE_VERSION),)
$(error Missing or empty VERSION file '$(VERSION_FILE)')
endif
RELEASE_DIR := build/release
RELEASE_BASENAME := $(RELEASE_PRODUCT_NAME)-$(RELEASE_VERSION)-$(RELEASE_PLATFORM)-$(RELEASE_ARCH)-$(RELEASE_CHANNEL)
RELEASE_APP_ZIP := $(RELEASE_DIR)/$(RELEASE_BASENAME).zip
RELEASE_APP_ZIP_SHA256 := $(RELEASE_APP_ZIP).sha256
RELEASE_MANIFEST := $(RELEASE_DIR)/$(RELEASE_BASENAME).manifest.txt

.PHONY: all build clean run run-headless test visual-harness identity print-identity \
	export-snapshot-json snapshot-bridge-check snapshot-bridge-import \
	shared-mode shared-subtree-check shared-subtree-prepare \
	package-desktop package-desktop-smoke package-desktop-self-test \
	package-desktop-copy-desktop package-desktop-sync package-desktop-open \
	package-desktop-remove package-desktop-refresh \
	release-contract release-clean release-build release-bundle-audit release-sign \
	release-verify release-verify-signed release-notarize release-staple \
	release-verify-notarized release-artifact release-distribute

all: build

build: $(APP_TARGET) $(HEADLESS_TARGET)

$(SHARED_BUILD_DIR):
	@mkdir -p "$@"

$(CORE_BASE_LIB): $(CORE_BASE_LOCAL_SRCS) | $(SHARED_BUILD_DIR)
	@$(MAKE) -C "$(CORE_BASE_DIR)" clean CC="$(SHARED_CC)"
	@$(MAKE) -C "$(CORE_BASE_DIR)" CC="$(SHARED_CC)"
	@cp "$(CORE_BASE_DIR)/build/libcore_base.a" "$@"

$(CORE_OBJECT_LIB): $(CORE_OBJECT_LOCAL_SRCS) $(CORE_BASE_LOCAL_SRCS) | $(SHARED_BUILD_DIR)
	@$(MAKE) -C "$(CORE_OBJECT_DIR)" clean CC="$(SHARED_CC)"
	@$(MAKE) -C "$(CORE_OBJECT_DIR)" CC="$(SHARED_CC)"
	@cp "$(CORE_OBJECT_DIR)/build/libcore_object.a" "$@"

$(CORE_UNITS_LIB): $(CORE_UNITS_LOCAL_SRCS) $(CORE_BASE_LOCAL_SRCS) | $(SHARED_BUILD_DIR)
	@$(MAKE) -C "$(CORE_UNITS_DIR)" clean CC="$(SHARED_CC)"
	@$(MAKE) -C "$(CORE_UNITS_DIR)" CC="$(SHARED_CC)"
	@cp "$(CORE_UNITS_DIR)/build/libcore_units.a" "$@"

$(CORE_SCENE_LIB): $(CORE_SCENE_LOCAL_SRCS) $(CORE_OBJECT_LOCAL_SRCS) $(CORE_UNITS_LOCAL_SRCS) $(CORE_BASE_LOCAL_SRCS) | $(SHARED_BUILD_DIR)
	@$(MAKE) -C "$(CORE_SCENE_DIR)" clean CC="$(SHARED_CC)"
	@$(MAKE) -C "$(CORE_SCENE_DIR)" CC="$(SHARED_CC)"
	@cp "$(CORE_SCENE_DIR)/build/libcore_scene.a" "$@"

$(CORE_PACK_LIB): $(CORE_PACK_LOCAL_SRCS) $(CORE_BASE_LOCAL_SRCS) | $(SHARED_BUILD_DIR)
	@$(MAKE) -C "$(CORE_PACK_DIR)" clean CC="$(SHARED_CC)"
	@$(MAKE) -C "$(CORE_PACK_DIR)" CC="$(SHARED_CC)"
	@cp "$(CORE_PACK_DIR)/build/libcore_pack.a" "$@"

$(CORE_THEME_LIB): $(CORE_THEME_LOCAL_SRCS) $(CORE_BASE_LOCAL_SRCS) | $(SHARED_BUILD_DIR)
	@$(MAKE) -C "$(CORE_THEME_DIR)" clean CC="$(SHARED_CC)"
	@$(MAKE) -C "$(CORE_THEME_DIR)" CC="$(SHARED_CC)"
	@cp "$(CORE_THEME_DIR)/build/libcore_theme.a" "$@"

$(CORE_FONT_LIB): $(CORE_FONT_LOCAL_SRCS) $(CORE_BASE_LOCAL_SRCS) | $(SHARED_BUILD_DIR)
	@$(MAKE) -C "$(CORE_FONT_DIR)" clean CC="$(SHARED_CC)"
	@$(MAKE) -C "$(CORE_FONT_DIR)" CC="$(SHARED_CC)"
	@cp "$(CORE_FONT_DIR)/build/libcore_font.a" "$@"

$(CORE_PANE_LIB): $(CORE_PANE_LOCAL_SRCS) | $(SHARED_BUILD_DIR)
	@$(MAKE) -C "$(CORE_PANE_DIR)" clean CC="$(SHARED_CC)"
	@$(MAKE) -C "$(CORE_PANE_DIR)" CC="$(SHARED_CC)"
	@cp "$(CORE_PANE_DIR)/build/libcore_pane.a" "$@"

$(CORE_LAYOUT_LIB): $(CORE_LAYOUT_LOCAL_SRCS) | $(SHARED_BUILD_DIR)
	@$(MAKE) -C "$(CORE_LAYOUT_DIR)" clean CC="$(SHARED_CC)"
	@$(MAKE) -C "$(CORE_LAYOUT_DIR)" CC="$(SHARED_CC)"
	@cp "$(CORE_LAYOUT_DIR)/build/libcore_layout.a" "$@"

$(CORE_PANE_MODULE_LIB): $(CORE_PANE_MODULE_LOCAL_SRCS) | $(SHARED_BUILD_DIR)
	@$(MAKE) -C "$(CORE_PANE_MODULE_DIR)" clean CC="$(SHARED_CC)"
	@$(MAKE) -C "$(CORE_PANE_MODULE_DIR)" CC="$(SHARED_CC)"
	@cp "$(CORE_PANE_MODULE_DIR)/build/libcore_pane_module.a" "$@"

$(CORE_VIEWPORT2D_LIB): $(CORE_VIEWPORT2D_LOCAL_SRCS) $(CORE_BASE_LOCAL_SRCS) | $(SHARED_BUILD_DIR)
	@$(MAKE) -C "$(CORE_VIEWPORT2D_DIR)" clean CC="$(SHARED_CC)"
	@$(MAKE) -C "$(CORE_VIEWPORT2D_DIR)" CC="$(SHARED_CC)"
	@cp "$(CORE_VIEWPORT2D_DIR)/build/libcore_viewport2d.a" "$@"

$(KIT_RENDER_LIB): $(KIT_RENDER_LOCAL_SRCS) $(CORE_THEME_LOCAL_SRCS) $(CORE_FONT_LOCAL_SRCS) $(CORE_BASE_LOCAL_SRCS) | $(SHARED_BUILD_DIR)
	@$(MAKE) -C "$(KIT_RENDER_DIR)" clean CC="$(SHARED_CC)" KIT_RENDER_ENABLE_VK=0
	@$(MAKE) -C "$(KIT_RENDER_DIR)" CC="$(SHARED_CC)" KIT_RENDER_ENABLE_VK=0
	@cp "$(KIT_RENDER_DIR)/build/default/libkit_render.a" "$@"

$(KIT_PANE_LIB): $(KIT_PANE_LOCAL_SRCS) $(KIT_RENDER_LOCAL_SRCS) $(CORE_PANE_LOCAL_SRCS) $(CORE_THEME_LOCAL_SRCS) $(CORE_FONT_LOCAL_SRCS) $(CORE_BASE_LOCAL_SRCS) | $(SHARED_BUILD_DIR)
	@$(MAKE) -C "$(KIT_PANE_DIR)" clean CC="$(SHARED_CC)" KIT_RENDER_ENABLE_VK=0
	@$(MAKE) -C "$(KIT_PANE_DIR)" CC="$(SHARED_CC)" KIT_RENDER_ENABLE_VK=0
	@cp "$(KIT_PANE_DIR)/build/libkit_pane.a" "$@"

$(KIT_WORKSPACE_AUTHORING_LIB): $(KIT_WORKSPACE_AUTHORING_LOCAL_SRCS) $(KIT_RENDER_LOCAL_SRCS) $(CORE_PANE_LOCAL_SRCS) $(CORE_THEME_LOCAL_SRCS) $(CORE_FONT_LOCAL_SRCS) $(CORE_BASE_LOCAL_SRCS) | $(SHARED_BUILD_DIR)
	@$(MAKE) -C "$(KIT_WORKSPACE_AUTHORING_DIR)" clean CC="$(SHARED_CC)" CORE_BASE_DIR="../../core/core_base" CORE_PANE_DIR="../../core/core_pane" CORE_THEME_DIR="../../core/core_theme" CORE_FONT_DIR="../../core/core_font" KIT_RENDER_DIR="../kit_render"
	@$(MAKE) -C "$(KIT_WORKSPACE_AUTHORING_DIR)" CC="$(SHARED_CC)" CORE_BASE_DIR="../../core/core_base" CORE_PANE_DIR="../../core/core_pane" CORE_THEME_DIR="../../core/core_theme" CORE_FONT_DIR="../../core/core_font" KIT_RENDER_DIR="../kit_render"
	@cp "$(KIT_WORKSPACE_AUTHORING_DIR)/build/libkit_workspace_authoring.a" "$@"

$(TARGET_BUILD_DIR) $(PROGRAM_BUILD_DIR) $(PROGRAM_BIN_DIR) $(HOST_TEST_DIR) $(TEST_OBJ_DIR) $(TEST_BIN_DIR):
	@mkdir -p "$@"

$(PROGRAM_CC_STAMP): makefile $(PROGRAM_CC_DEP) | $(PROGRAM_BUILD_DIR)
	@printf '%s\n' "$(PROGRAM_CC)" > "$@.tmp"
	@cmp -s "$@.tmp" "$@" 2>/dev/null || mv "$@.tmp" "$@"
	@rm -f "$@.tmp"

$(APP_OBJS) $(HEADLESS_OBJS): $(PROGRAM_CC_STAMP)

$(PROGRAM_OBJ_DIR)/%.c.o: %.c $(PROGRAM_CC_STAMP)
	@mkdir -p "$(dir $@)"
	$(PROGRAM_CC) $(PROGRAM_CFLAGS) $(PROGRAM_DEPFLAGS) -c "$<" -o "$@"

$(TEST_OBJ_DIR)/%.c.o: %.c
	@mkdir -p "$(dir $@)"
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_DEPFLAGS) -c "$<" -o "$@"

$(APP_TARGET): $(APP_OBJS) $(SHARED_LIBS)
	@mkdir -p "$(dir $@)"
	$(HOST_CC) $(ARCH_FLAGS) $(APP_OBJS) $(SHARED_LIBS) -o "$@" $(APP_LDLIBS)

$(HEADLESS_TARGET): $(HEADLESS_OBJS) $(SHARED_LIBS)
	@mkdir -p "$(dir $@)"
	$(HOST_CC) $(ARCH_FLAGS) $(HEADLESS_OBJS) $(SHARED_LIBS) -o "$@" $(APP_LDLIBS)

$(TEST_TARGET): $(TEST_OBJS) $(SHARED_LIBS)
	@mkdir -p "$(dir $@)"
	$(HOST_CC) $(ARCH_FLAGS) $(TEST_OBJS) $(SHARED_LIBS) -o "$@" $(APP_LDLIBS)

run: $(APP_TARGET)
	"$(APP_TARGET)"

run-headless: $(HEADLESS_TARGET)
	"$(HEADLESS_TARGET)" --headless --smoke-frames 2 --print-lifecycle --no-persist

test: $(TEST_TARGET)
	"$(TEST_TARGET)"

-include $(APP_DEPS) $(HEADLESS_DEPS) $(TEST_DEPS)

visual-harness: $(APP_TARGET)
	@echo "Built $(APP_TARGET)"

export-snapshot-json: $(HEADLESS_TARGET)
	"$(HEADLESS_TARGET)" --headless --smoke-frames 1 --preset "$(EXPORT_PRESET)" --export-json "$(EXPORT_JSON)"
	@echo "Drawing snapshot debug JSON exported: $(EXPORT_JSON)"

snapshot-bridge-check: $(HEADLESS_TARGET)
	"$(HEADLESS_TARGET)" --headless --smoke-frames 1 --bridge-workspace-preset "$(WORKSPACE_PRESET)"
	@echo "Workspace preset bridge check passed: $(WORKSPACE_PRESET)"

snapshot-bridge-import: $(HEADLESS_TARGET)
	"$(HEADLESS_TARGET)" --headless --smoke-frames 1 --bridge-workspace-preset "$(WORKSPACE_PRESET)" --bridge-workspace-import
	@echo "Workspace preset import applied: $(WORKSPACE_PRESET)"

shared-mode:
	@echo "SHARED_MODE=$(SHARED_MODE)"
	@echo "TARGET_OS=$(TARGET_OS)"
	@echo "TARGET_ARCH=$(TARGET_ARCH)"
	@echo "TARGET_VARIANT=$(TARGET_VARIANT)"
	@echo "TARGET_TRIPLE=$(TARGET_TRIPLE)"
	@echo "TARGET_HOMEBREW_PREFIX=$(TARGET_HOMEBREW_PREFIX)"
	@echo "TARGET_PKG_CONFIG_LIBDIR=$(TARGET_PKG_CONFIG_LIBDIR)"
	@echo "CORE_BASE_DIR=$(CORE_BASE_DIR)"
	@echo "CORE_THEME_DIR=$(CORE_THEME_DIR)"
	@echo "CORE_PACK_DIR=$(CORE_PACK_DIR)"
	@echo "CORE_PANE_DIR=$(CORE_PANE_DIR)"
	@echo "CORE_LAYOUT_DIR=$(CORE_LAYOUT_DIR)"
	@echo "CORE_PANE_MODULE_DIR=$(CORE_PANE_MODULE_DIR)"

shared-subtree-check:
	@if [ ! -d ".git" ]; then \
		echo "drawing_program is not a git repo yet; subtree squash pull cannot run."; \
		echo "Initialize drawing_program as a git repo first, then run:"; \
		echo "  ../bin/update_shared_subtrees.sh --check --only drawing_program --targets ../bin/shared_subtree_targets.tsv"; \
		echo "  ../bin/update_shared_subtrees.sh --update --only drawing_program --targets ../bin/shared_subtree_targets.tsv"; \
		exit 1; \
	fi

shared-subtree-prepare:
	@../bin/update_shared_subtrees.sh --update --only drawing_program --targets ../bin/shared_subtree_targets.tsv
	@echo "Prepared vendored shared snapshot at $(SHARED_VENDOR_DIR)"
	@echo "Default host mode is vendored-subtree; use SHARED_MODE=workspace-linked only for bounded local debugging"

clean:
	rm -rf "$(BUILD_DIR)"

identity: print-identity

print-identity:
	@echo "PROGRAM_KEY=$(PROGRAM_KEY)"
	@echo "RELEASE_PRODUCT_NAME=$(RELEASE_PRODUCT_NAME)"
	@echo "RELEASE_BUNDLE_ID=$(RELEASE_BUNDLE_ID)"
	@echo "LAUNCHER_BIN=$(LAUNCHER_BIN)"
	@echo "APP_BIN=$(APP_BIN)"
	@echo "VERSION=$$(cat $(VERSION_FILE))"

package-desktop:
	@$(MAKE) BUILD_TOOLCHAIN="$(PACKAGE_TOOLCHAIN)" "$(PACKAGE_SOURCE_BIN)"
	@rm -rf "$(PACKAGE_APP_DIR)"
	@mkdir -p "$(PACKAGE_MACOS_DIR)" "$(PACKAGE_RESOURCES_DIR)" "$(PACKAGE_FRAMEWORKS_DIR)" "$(PACKAGE_SHARED_FONTS_DIR)"
	@cp "$(PACKAGE_INFO_PLIST_SRC)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@cp "$(PACKAGE_SOURCE_BIN)" "$(PACKAGE_MACOS_DIR)/$(APP_BIN)"
	@cp "$(PACKAGE_LAUNCHER_SRC)" "$(PACKAGE_MACOS_DIR)/$(LAUNCHER_BIN)"
	@chmod +x "$(PACKAGE_MACOS_DIR)/$(APP_BIN)" "$(PACKAGE_MACOS_DIR)/$(LAUNCHER_BIN)"
	@if [ -d "$(PACKAGE_FONTS_SRC_PRIMARY)" ]; then \
		rsync -a "$(PACKAGE_FONTS_SRC_PRIMARY)"/ "$(PACKAGE_SHARED_FONTS_DIR)"/; \
	elif [ -d "$(PACKAGE_FONTS_SRC_WORKSPACE)" ]; then \
		rsync -a "$(PACKAGE_FONTS_SRC_WORKSPACE)"/ "$(PACKAGE_SHARED_FONTS_DIR)"/; \
	else \
		echo "warning: no font source dir found for packaging"; \
	fi
	@if [ -f "$(PACKAGE_APP_ICON_SRC)" ]; then \
		cp "$(PACKAGE_APP_ICON_SRC)" "$(PACKAGE_BUNDLED_ICON_PATH)"; \
		echo "Bundled app icon from $(PACKAGE_APP_ICON_SRC)"; \
	elif [ -d "$(PACKAGE_APP_ICONSET_SRC)" ]; then \
		/usr/bin/iconutil -c icns -o "$(PACKAGE_BUNDLED_ICON_PATH)" "$(PACKAGE_APP_ICONSET_SRC)" || exit 1; \
		echo "Bundled app icon from $(PACKAGE_APP_ICONSET_SRC)"; \
	else \
		echo "warning: no app icon source found at $(PACKAGE_APP_ICON_SRC) or $(PACKAGE_APP_ICONSET_SRC)"; \
	fi
	@PACKAGE_DEP_SEARCH_ROOTS="$(TARGET_DEP_SEARCH_ROOTS)" \
		"$(PACKAGE_DYLIB_BUNDLER)" "$(PACKAGE_MACOS_DIR)/$(APP_BIN)" "$(PACKAGE_FRAMEWORKS_DIR)"
	@for dylib in "$(PACKAGE_FRAMEWORKS_DIR)"/*.dylib; do \
		[ -f "$$dylib" ] || continue; \
		codesign --force --sign "$(PACKAGE_ADHOC_SIGN_IDENTITY)" "$$dylib"; \
	done
	@codesign --force --sign "$(PACKAGE_ADHOC_SIGN_IDENTITY)" "$(PACKAGE_MACOS_DIR)/$(APP_BIN)"
	@codesign --force --sign "$(PACKAGE_ADHOC_SIGN_IDENTITY)" "$(PACKAGE_MACOS_DIR)/$(LAUNCHER_BIN)"
	@codesign --force --sign "$(PACKAGE_ADHOC_SIGN_IDENTITY)" "$(PACKAGE_APP_DIR)"
	@echo "Desktop package ready: $(PACKAGE_APP_DIR)"

package-desktop-smoke: package-desktop
	@test -x "$(PACKAGE_MACOS_DIR)/$(LAUNCHER_BIN)" || (echo "Missing launcher"; exit 1)
	@test -x "$(PACKAGE_MACOS_DIR)/$(APP_BIN)" || (echo "Missing app binary"; exit 1)
	@test -f "$(PACKAGE_CONTENTS_DIR)/Info.plist" || (echo "Missing Info.plist"; exit 1)
	@if command -v lipo >/dev/null 2>&1; then \
		actual_archs="$$(lipo -archs "$(PACKAGE_MACOS_DIR)/$(APP_BIN)" 2>/dev/null || true)"; \
		case "$$actual_archs" in \
			*"$(TARGET_ARCH)"*) ;; \
			*) echo "App binary arch mismatch: expected $(TARGET_ARCH), got '$$actual_archs'"; exit 1 ;; \
		esac; \
		for dylib in "$(PACKAGE_FRAMEWORKS_DIR)"/*.dylib; do \
			[ -f "$$dylib" ] || continue; \
			dylib_archs="$$(lipo -archs "$$dylib" 2>/dev/null || true)"; \
			case "$$dylib_archs" in \
				*"$(TARGET_ARCH)"*) ;; \
				*) echo "Bundled dylib arch mismatch: $$dylib -> '$$dylib_archs'"; exit 1 ;; \
			esac; \
		done; \
	fi
	@if [ -f "$(PACKAGE_APP_ICON_SRC)" ] || [ -d "$(PACKAGE_APP_ICONSET_SRC)" ]; then \
		test -f "$(PACKAGE_BUNDLED_ICON_PATH)" || (echo "Missing bundled AppIcon.icns"; exit 1); \
	fi
	@echo "package-desktop-smoke passed."

package-desktop-self-test: package-desktop-smoke
	@"$(PACKAGE_MACOS_DIR)/$(LAUNCHER_BIN)" --self-test || (echo "package-desktop self-test failed."; exit 1)
	@echo "package-desktop-self-test passed."

package-desktop-copy-desktop: package-desktop
	@mkdir -p "$(dir $(DESKTOP_APP_DIR))"
	@rm -rf "$(DESKTOP_APP_DIR)"
	@ditto "$(PACKAGE_APP_DIR)" "$(DESKTOP_APP_DIR)"
	@echo "Copied $(PACKAGE_APP_NAME) to $(DESKTOP_APP_DIR)"

package-desktop-sync: package-desktop-copy-desktop
	@echo "Desktop package synchronized: $(DESKTOP_APP_DIR)"

package-desktop-open: package-desktop
	@open "$(PACKAGE_APP_DIR)"

package-desktop-remove:
	@rm -rf "$(DESKTOP_APP_DIR)"
	@echo "Removed desktop app copy: $(DESKTOP_APP_DIR)"

package-desktop-refresh: package-desktop
	@mkdir -p "$(dir $(DESKTOP_APP_DIR))"
	@rm -rf "$(DESKTOP_APP_DIR)"
	@ditto "$(PACKAGE_APP_DIR)" "$(DESKTOP_APP_DIR)"
	@echo "Refreshed $(PACKAGE_APP_NAME) at $(DESKTOP_APP_DIR)"

release-contract:
	@echo "PROGRAM_KEY=$(PROGRAM_KEY)"
	@echo "RELEASE_PRODUCT_NAME=$(RELEASE_PRODUCT_NAME)"
	@echo "RELEASE_BUNDLE_ID=$(RELEASE_BUNDLE_ID)"
	@echo "HOST_ARCH=$(HOST_ARCH)"
	@echo "TARGET_OS=$(TARGET_OS)"
	@echo "TARGET_ARCH=$(TARGET_ARCH)"
	@echo "TARGET_VARIANT=$(TARGET_VARIANT)"
	@echo "TARGET_TRIPLE=$(TARGET_TRIPLE)"
	@echo "RELEASE_PLATFORM=$(RELEASE_PLATFORM)"
	@echo "RELEASE_ARCH=$(RELEASE_ARCH)"
	@echo "TARGET_HOMEBREW_PREFIX=$(TARGET_HOMEBREW_PREFIX)"
	@echo "TARGET_PKG_CONFIG_LIBDIR=$(TARGET_PKG_CONFIG_LIBDIR)"
	@echo "RELEASE_VERSION=$(RELEASE_VERSION)"
	@echo "RELEASE_CHANNEL=$(RELEASE_CHANNEL)"
	@echo "RELEASE_APP_ZIP=$(RELEASE_APP_ZIP)"
	@echo "RELEASE_MANIFEST=$(RELEASE_MANIFEST)"

release-clean:
	@rm -rf "$(RELEASE_DIR)"

release-build:
	@$(MAKE) BUILD_TOOLCHAIN="$(PACKAGE_TOOLCHAIN)" PACKAGE_TOOLCHAIN="$(PACKAGE_TOOLCHAIN)" package-desktop
	@mkdir -p "$(RELEASE_DIR)"
	@echo "Release build prepared at $(PACKAGE_APP_DIR)"

release-bundle-audit: release-build
	@$(MAKE) BUILD_TOOLCHAIN="$(PACKAGE_TOOLCHAIN)" PACKAGE_TOOLCHAIN="$(PACKAGE_TOOLCHAIN)" package-desktop-smoke
	@echo "release-bundle-audit passed."

release-sign: release-build
	@echo "release-sign scaffold: using ad-hoc signed package output from package-desktop."

release-verify: release-bundle-audit
	@$(MAKE) BUILD_TOOLCHAIN="$(PACKAGE_TOOLCHAIN)" PACKAGE_TOOLCHAIN="$(PACKAGE_TOOLCHAIN)" package-desktop-self-test
	@echo "release-verify passed."

release-verify-signed: release-verify
	@echo "release-verify-signed scaffold passed."

release-notarize: release-sign
	@echo "release-notarize scaffold placeholder."

release-staple: release-notarize
	@echo "release-staple scaffold placeholder."

release-verify-notarized: release-verify
	@echo "release-verify-notarized scaffold placeholder."

release-artifact: release-verify
	@mkdir -p "$(RELEASE_DIR)"
	@rm -f "$(RELEASE_APP_ZIP)" "$(RELEASE_APP_ZIP_SHA256)" "$(RELEASE_MANIFEST)"
	@cd "$(DIST_DIR)" && zip -qr "../$(RELEASE_APP_ZIP)" "$(PACKAGE_APP_NAME)"
	@shasum -a 256 "$(RELEASE_APP_ZIP)" > "$(RELEASE_APP_ZIP_SHA256)"
	@{ \
		echo "product=$(RELEASE_PRODUCT_NAME)"; \
		echo "program=$(PROGRAM_KEY)"; \
		echo "host_arch=$(HOST_ARCH)"; \
		echo "target_os=$(TARGET_OS)"; \
		echo "target_arch=$(TARGET_ARCH)"; \
		echo "target_variant=$(TARGET_VARIANT)"; \
		echo "target_triple=$(TARGET_TRIPLE)"; \
		echo "release_platform=$(RELEASE_PLATFORM)"; \
		echo "release_arch=$(RELEASE_ARCH)"; \
		echo "version=$(RELEASE_VERSION)"; \
		echo "channel=$(RELEASE_CHANNEL)"; \
		echo "bundle_id=$(RELEASE_BUNDLE_ID)"; \
		echo "zip=$(RELEASE_APP_ZIP)"; \
		echo "sha256=$$(cut -d' ' -f1 "$(RELEASE_APP_ZIP_SHA256)")"; \
	} > "$(RELEASE_MANIFEST)"
	@echo "release-artifact complete: $(RELEASE_APP_ZIP)"

release-distribute: release-artifact
	@echo "release-distribute scaffold passed."
