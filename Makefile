PROGRAM_KEY := drawing_program
RELEASE_PRODUCT_NAME := sketCh
RELEASE_BUNDLE_ID := com.cosm.sketch
LAUNCHER_BIN := sketch-launcher
APP_BIN := drawing-program-bin
VERSION_FILE := VERSION

HOST_CC ?= cc
FISICS_CC ?= /Users/calebsv/Desktop/CodeWork/fisiCs/fisics
BUILD_TOOLCHAIN ?= clang
PACKAGE_TOOLCHAIN ?= $(BUILD_TOOLCHAIN)
TEST_TOOLCHAIN := clang
PKG_CONFIG ?= pkg-config
SHARED_VENDOR_DIR ?= third_party/codework_shared
SHARED_WORKSPACE_DIR ?= ../shared
SHARED_MODE ?= vendored-subtree

CORE_BASE_DIR ?= $(SHARED_VENDOR_DIR)/core/core_base
CORE_THEME_DIR ?= $(SHARED_VENDOR_DIR)/core/core_theme
CORE_FONT_DIR ?= $(SHARED_VENDOR_DIR)/core/core_font
CORE_PACK_DIR ?= $(SHARED_VENDOR_DIR)/core/core_pack
CORE_PANE_DIR ?= $(SHARED_VENDOR_DIR)/core/core_pane
CORE_LAYOUT_DIR ?= $(SHARED_VENDOR_DIR)/core/core_layout
CORE_PANE_MODULE_DIR ?= $(SHARED_VENDOR_DIR)/core/core_pane_module
CORE_VIEWPORT2D_DIR ?= $(SHARED_VENDOR_DIR)/core/core_viewport2d

ifeq ($(SHARED_MODE),workspace-linked)
CORE_BASE_DIR := $(SHARED_WORKSPACE_DIR)/core/core_base
CORE_THEME_DIR := $(SHARED_WORKSPACE_DIR)/core/core_theme
CORE_FONT_DIR := $(SHARED_WORKSPACE_DIR)/core/core_font
CORE_PACK_DIR := $(SHARED_WORKSPACE_DIR)/core/core_pack
CORE_PANE_DIR := $(SHARED_WORKSPACE_DIR)/core/core_pane
CORE_LAYOUT_DIR := $(SHARED_WORKSPACE_DIR)/core/core_layout
CORE_PANE_MODULE_DIR := $(SHARED_WORKSPACE_DIR)/core/core_pane_module
CORE_VIEWPORT2D_DIR := $(SHARED_WORKSPACE_DIR)/core/core_viewport2d
endif

SDL_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2 2>/dev/null)
SDL_LIBS := $(shell $(PKG_CONFIG) --libs sdl2 2>/dev/null)
SDL_TTF_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2_ttf 2>/dev/null)
SDL_TTF_LIBS := $(shell $(PKG_CONFIG) --libs sdl2_ttf 2>/dev/null)
PNG_CFLAGS := $(shell $(PKG_CONFIG) --cflags libpng 2>/dev/null)
PNG_LIBS := $(shell $(PKG_CONFIG) --libs libpng 2>/dev/null)
ifeq ($(strip $(SDL_CFLAGS)),)
  ifneq ($(wildcard /opt/homebrew/include/SDL2/SDL.h),)
    SDL_CFLAGS := -I/opt/homebrew/include -D_THREAD_SAFE
  else ifneq ($(wildcard /usr/local/include/SDL2/SDL.h),)
    SDL_CFLAGS := -I/usr/local/include -D_THREAD_SAFE
  endif
endif
ifeq ($(strip $(SDL_LIBS)),)
  ifneq ($(wildcard /opt/homebrew/lib/libSDL2.dylib),)
    SDL_LIBS := -L/opt/homebrew/lib -lSDL2
  else ifneq ($(wildcard /usr/local/lib/libSDL2.dylib),)
    SDL_LIBS := -L/usr/local/lib -lSDL2
  else
    SDL_LIBS := -framework SDL2
  endif
endif
ifeq ($(strip $(SDL_TTF_CFLAGS)),)
  ifneq ($(wildcard /opt/homebrew/include/SDL2/SDL_ttf.h),)
    SDL_TTF_CFLAGS := -I/opt/homebrew/include -D_THREAD_SAFE
  else ifneq ($(wildcard /usr/local/include/SDL2/SDL_ttf.h),)
    SDL_TTF_CFLAGS := -I/usr/local/include -D_THREAD_SAFE
  endif
endif
ifeq ($(strip $(SDL_TTF_LIBS)),)
  ifneq ($(wildcard /opt/homebrew/lib/libSDL2_ttf.dylib),)
    SDL_TTF_LIBS := -L/opt/homebrew/lib -lSDL2_ttf
  else ifneq ($(wildcard /usr/local/lib/libSDL2_ttf.dylib),)
    SDL_TTF_LIBS := -L/usr/local/lib -lSDL2_ttf
  else
    SDL_TTF_LIBS :=
  endif
endif
ifeq ($(strip $(PNG_CFLAGS)),)
  ifneq ($(wildcard /opt/homebrew/include/png.h),)
    PNG_CFLAGS := -I/opt/homebrew/include
  else ifneq ($(wildcard /usr/local/include/png.h),)
    PNG_CFLAGS := -I/usr/local/include
  endif
endif
ifeq ($(strip $(PNG_LIBS)),)
  ifneq ($(wildcard /opt/homebrew/lib/libpng.dylib),)
    PNG_LIBS := -L/opt/homebrew/lib -lpng
  else ifneq ($(wildcard /opt/homebrew/lib/libpng16.dylib),)
    PNG_LIBS := -L/opt/homebrew/lib -lpng16
  else ifneq ($(wildcard /usr/local/lib/libpng.dylib),)
    PNG_LIBS := -L/usr/local/lib -lpng
  else ifneq ($(wildcard /usr/local/lib/libpng16.dylib),)
    PNG_LIBS := -L/usr/local/lib -lpng16
  endif
endif

CFLAGS := -std=c11 -Wall -Wextra -pedantic \
	-Iinclude \
	-I$(CORE_BASE_DIR)/include \
	-I$(CORE_THEME_DIR)/include \
	-I$(CORE_FONT_DIR)/include \
	-I$(CORE_PACK_DIR)/include \
	-I$(CORE_PANE_DIR)/include \
	-I$(CORE_LAYOUT_DIR)/include \
	-I$(CORE_PANE_MODULE_DIR)/include \
	-I$(CORE_VIEWPORT2D_DIR)/include \
	$(SDL_CFLAGS) \
	$(PNG_CFLAGS) \
	$(SDL_TTF_CFLAGS)
LDLIBS := -lm
APP_LDLIBS := $(LDLIBS) $(SDL_LIBS) $(SDL_TTF_LIBS) $(PNG_LIBS)

BUILD_DIR := build
HOST_TEST_DIR := $(BUILD_DIR)/host/tests
TEST_OBJ_DIR := $(HOST_TEST_DIR)/obj
TEST_BIN_DIR := $(HOST_TEST_DIR)/bin
PROGRAM_BUILD_DIR := $(BUILD_DIR)/toolchains/$(BUILD_TOOLCHAIN)
PROGRAM_OBJ_DIR := $(PROGRAM_BUILD_DIR)/obj
PROGRAM_BIN_DIR := $(PROGRAM_BUILD_DIR)/bin
PROGRAM_CC_STAMP := $(PROGRAM_BUILD_DIR)/compiler.stamp

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
	src/domain/drawing_program_history_object_commands.c \
	src/domain/drawing_program_layer_raster.c \
	src/domain/drawing_program_object_geometry.c \
	src/domain/drawing_program_object_rasterize.c \
	src/domain/drawing_program_object_store.c \
	src/domain/drawing_program_object_store_path_hit_test.c \
	src/domain/drawing_program_object_selection.c \
	src/domain/drawing_program_object_transform.c \
	src/domain/drawing_program_selection.c \
	src/domain/drawing_program_selection_edit_ops.c \
	src/io/export/drawing_program_export_image.c \
	src/io/export/drawing_program_icns_export.c \
	src/io/export/drawing_program_iconset_export.c \
	src/io/export/drawing_program_png_export.c \
	src/io/session/drawing_program_project_state.c \
	src/io/session/drawing_program_session_prefs.c \
	src/io/session/drawing_program_native_dialogs.c \
	src/io/session/drawing_program_snapshot.c \
	src/io/session/drawing_program_snapshot_shell.c \
	src/io/session/drawing_program_snapshot_export_json.c \
	src/io/session/drawing_program_snapshot_ui_settings.c \
	src/io/session/drawing_program_snapshot_bridge.c \
	src/domain/drawing_program_viewport.c \
	src/runtime/render/drawing_program_render_domain.c \
	src/runtime/render/drawing_program_render_backend.c \
	src/runtime/orchestration/drawing_program_runtime_orchestration.c \
	src/runtime/canvas/drawing_program_visual_canvas_stroke_ops.c \
	src/runtime/adapters/drawing_program_pane_host.c \
	src/runtime/adapters/drawing_program_overlay_adapter.c \
	src/render/canvas/drawing_program_visual_canvas_world_render.c \
	src/render/frame/drawing_program_visual_frame_render.c \
	src/render/overlay/drawing_program_visual_overlay_shared.c \
	src/render/overlay/drawing_program_visual_object_overlay.c \
	src/render/overlay/drawing_program_visual_selection_overlay.c \
	src/render/overlay/drawing_program_visual_overlay_render.c \
	src/input/canvas/drawing_program_visual_canvas_action_ops.c \
	src/input/canvas/drawing_program_visual_canvas_coords.c \
	src/input/canvas/drawing_program_visual_canvas_draw_action_ops.c \
	src/input/panel/drawing_program_visual_input_panel_color.c \
	src/input/panel/drawing_program_visual_input_panel_clicks.c \
	src/input/input_core/drawing_program_visual_input_selection_ops.c \
	src/input/tools/drawing_program_visual_shape_ops.c \
	src/input/tools/drawing_program_visual_transform_ops.c \
	src/input/layers/drawing_program_visual_layer_actions.c \
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
	src/ui/panel/drawing_program_visual_right_panel_render.c \
	src/ui/theme/drawing_program_visual_theme.c \
	src/ui/resources/drawing_program_visual_resources.c \
	src/app/drawing_program_visual_runtime_debug.c \
	src/app/drawing_program_visual_loop_diag.c \
	src/app/drawing_program_visual_loop_policy.c \
	src/app/drawing_program_app_post_load.c \
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
	src/domain/drawing_program_history_object_commands.c \
	src/domain/drawing_program_layer_raster.c \
	src/domain/drawing_program_object_geometry.c \
	src/domain/drawing_program_object_rasterize.c \
	src/domain/drawing_program_object_store.c \
	src/domain/drawing_program_object_store_path_hit_test.c \
	src/domain/drawing_program_object_selection.c \
	src/domain/drawing_program_object_transform.c \
	src/domain/drawing_program_selection.c \
	src/domain/drawing_program_selection_edit_ops.c \
	src/io/export/drawing_program_export_image.c \
	src/io/export/drawing_program_icns_export.c \
	src/io/export/drawing_program_iconset_export.c \
	src/io/export/drawing_program_png_export.c \
	src/io/session/drawing_program_project_state.c \
	src/io/session/drawing_program_session_prefs.c \
	src/io/session/drawing_program_native_dialogs.c \
	src/io/session/drawing_program_snapshot.c \
	src/io/session/drawing_program_snapshot_shell.c \
	src/io/session/drawing_program_snapshot_export_json.c \
	src/io/session/drawing_program_snapshot_ui_settings.c \
	src/io/session/drawing_program_snapshot_bridge.c \
	src/domain/drawing_program_viewport.c \
	src/runtime/render/drawing_program_render_domain.c \
	src/runtime/render/drawing_program_render_backend.c \
	src/runtime/orchestration/drawing_program_runtime_orchestration.c \
	src/runtime/canvas/drawing_program_visual_canvas_stroke_ops.c \
	src/runtime/adapters/drawing_program_pane_host.c \
	src/runtime/adapters/drawing_program_overlay_adapter.c \
	src/app/drawing_program_ui_color_state.c \
	src/app/drawing_program_app_post_load.c \
	src/app/drawing_program_app_main.c \
	src/app/drawing_program_app_headless_main.c

TEST_LOCAL_SRCS := \
	src/domain/drawing_program_color_model.c \
	src/domain/drawing_program_document.c \
	src/domain/drawing_program_editor_state.c \
	src/domain/drawing_program_history.c \
	src/domain/drawing_program_history_object_commands.c \
	src/domain/drawing_program_layer_raster.c \
	src/domain/drawing_program_object_geometry.c \
	src/domain/drawing_program_object_rasterize.c \
	src/domain/drawing_program_object_store.c \
	src/domain/drawing_program_object_store_path_hit_test.c \
	src/domain/drawing_program_object_selection.c \
	src/domain/drawing_program_object_transform.c \
	src/domain/drawing_program_selection.c \
	src/domain/drawing_program_selection_edit_ops.c \
	src/io/export/drawing_program_export_image.c \
	src/io/export/drawing_program_icns_export.c \
	src/io/export/drawing_program_iconset_export.c \
	src/io/export/drawing_program_png_export.c \
	src/io/session/drawing_program_project_state.c \
	src/io/session/drawing_program_session_prefs.c \
	src/io/session/drawing_program_native_dialogs.c \
	src/io/session/drawing_program_snapshot.c \
	src/io/session/drawing_program_snapshot_shell.c \
	src/io/session/drawing_program_snapshot_export_json.c \
	src/io/session/drawing_program_snapshot_ui_settings.c \
	src/io/session/drawing_program_snapshot_bridge.c \
	src/domain/drawing_program_viewport.c \
	src/runtime/render/drawing_program_render_domain.c \
	src/runtime/render/drawing_program_render_backend.c \
	src/runtime/orchestration/drawing_program_runtime_orchestration.c \
	src/runtime/canvas/drawing_program_visual_canvas_stroke_ops.c \
	src/runtime/adapters/drawing_program_pane_host.c \
	src/runtime/adapters/drawing_program_overlay_adapter.c \
	src/render/canvas/drawing_program_visual_canvas_world_render.c \
	src/render/frame/drawing_program_visual_frame_render.c \
	src/render/overlay/drawing_program_visual_overlay_shared.c \
	src/render/overlay/drawing_program_visual_object_overlay.c \
	src/render/overlay/drawing_program_visual_selection_overlay.c \
	src/render/overlay/drawing_program_visual_overlay_render.c \
	src/input/canvas/drawing_program_visual_canvas_action_ops.c \
	src/input/canvas/drawing_program_visual_canvas_coords.c \
	src/input/canvas/drawing_program_visual_canvas_draw_action_ops.c \
	src/input/panel/drawing_program_visual_input_panel_color.c \
	src/input/panel/drawing_program_visual_input_panel_clicks.c \
	src/input/input_core/drawing_program_visual_input_selection_ops.c \
	src/input/tools/drawing_program_visual_shape_ops.c \
	src/input/tools/drawing_program_visual_transform_ops.c \
	src/input/layers/drawing_program_visual_layer_actions.c \
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
	src/ui/panel/drawing_program_visual_right_panel_render.c \
	src/ui/theme/drawing_program_visual_theme.c \
	src/ui/resources/drawing_program_visual_resources.c \
	src/app/drawing_program_visual_runtime_debug.c \
	src/app/drawing_program_visual_loop_diag.c \
	src/app/drawing_program_visual_loop_policy.c \
	src/app/drawing_program_app_post_load.c \
	src/ui/tools/drawing_program_visual_tool_options.c \
	src/ui/text/drawing_program_visual_text_render.c \
	src/app/drawing_program_ui_color_state.c \
	src/app/drawing_program_app_main.c \
	src/app/drawing_program_app_visual_runtime.c \
	src/app/drawing_program_app_visual_runtime_loop.c \
	src/app/drawing_program_app_visual_main.c \
	tests/drawing_program_lifecycle_snapshot_suite.c \
	tests/drawing_program_lifecycle_export_suite.c \
	tests/drawing_program_lifecycle_snapshot_helpers.c \
	tests/drawing_program_lifecycle_snapshot_object_helpers.c \
	tests/drawing_program_lifecycle_selection_layer_suite.c \
	tests/drawing_program_lifecycle_selection_payload_suite.c \
	tests/drawing_program_lifecycle_baseline_history_suite.c \
	tests/drawing_program_lifecycle_object_path_suite.c \
	tests/drawing_program_lifecycle_object_path_history_suite.c \
	tests/drawing_program_lifecycle_object_path_history_mutation_suite.c \
	tests/drawing_program_lifecycle_runtime_render_suite.c \
	tests/drawing_program_lifecycle_runtime_path_suite.c \
	tests/drawing_program_lifecycle_runtime_path_pointer_suite.c \
	tests/drawing_program_lifecycle_runtime_ui_suite.c \
	tests/drawing_program_lifecycle_color_panel_suite.c \
	tests/drawing_program_lifecycle_test_support.c \
	tests/drawing_program_lifecycle_test.c

APP_OBJS := $(APP_LOCAL_SRCS:%=$(PROGRAM_OBJ_DIR)/%.o)
HEADLESS_OBJS := $(HEADLESS_LOCAL_SRCS:%=$(PROGRAM_OBJ_DIR)/%.o)
TEST_OBJS := $(TEST_LOCAL_SRCS:%=$(TEST_OBJ_DIR)/%.o)

define app_bin_for
$(BUILD_DIR)/toolchains/$(1)/bin/$(APP_BIN)
endef

APP_TARGET := $(PROGRAM_BIN_DIR)/$(APP_BIN)
HEADLESS_TARGET := $(PROGRAM_BIN_DIR)/drawing-program-headless
TEST_TARGET := $(TEST_BIN_DIR)/drawing-program-test
PACKAGE_SOURCE_BIN := $(call app_bin_for,$(PACKAGE_TOOLCHAIN))

CORE_BASE_LIB := $(CORE_BASE_DIR)/build/libcore_base.a
CORE_THEME_LIB := $(CORE_THEME_DIR)/build/libcore_theme.a
CORE_FONT_LIB := $(CORE_FONT_DIR)/build/libcore_font.a
CORE_PACK_LIB := $(CORE_PACK_DIR)/build/libcore_pack.a
CORE_PANE_LIB := $(CORE_PANE_DIR)/build/libcore_pane.a
CORE_LAYOUT_LIB := $(CORE_LAYOUT_DIR)/build/libcore_layout.a
CORE_PANE_MODULE_LIB := $(CORE_PANE_MODULE_DIR)/build/libcore_pane_module.a
CORE_VIEWPORT2D_LIB := $(CORE_VIEWPORT2D_DIR)/build/libcore_viewport2d.a
SHARED_LIBS := $(CORE_PACK_LIB) $(CORE_PANE_LIB) $(CORE_LAYOUT_LIB) $(CORE_PANE_MODULE_LIB) $(CORE_THEME_LIB) $(CORE_FONT_LIB) $(CORE_VIEWPORT2D_LIB) $(CORE_BASE_LIB)

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

.PHONY: all build clean run run-headless test visual-harness identity print-identity \
	export-snapshot-json snapshot-bridge-check snapshot-bridge-import \
	shared-mode shared-subtree-check shared-subtree-prepare \
	package-desktop package-desktop-smoke package-desktop-self-test \
	package-desktop-copy-desktop package-desktop-sync package-desktop-open \
	package-desktop-remove package-desktop-refresh

all: build

build: $(APP_TARGET) $(HEADLESS_TARGET)

$(CORE_BASE_LIB):
	$(MAKE) -C $(CORE_BASE_DIR) CC="$(HOST_CC)"

$(CORE_PACK_LIB): $(CORE_BASE_LIB)
	$(MAKE) -C $(CORE_PACK_DIR) CC="$(HOST_CC)"

$(CORE_THEME_LIB): $(CORE_BASE_LIB)
	$(MAKE) -C $(CORE_THEME_DIR) CC="$(HOST_CC)"

$(CORE_FONT_LIB): $(CORE_BASE_LIB)
	$(MAKE) -C $(CORE_FONT_DIR) CC="$(HOST_CC)"

$(CORE_PANE_LIB):
	$(MAKE) -C $(CORE_PANE_DIR) CC="$(HOST_CC)"

$(CORE_LAYOUT_LIB):
	$(MAKE) -C $(CORE_LAYOUT_DIR) CC="$(HOST_CC)"

$(CORE_PANE_MODULE_LIB):
	$(MAKE) -C $(CORE_PANE_MODULE_DIR) CC="$(HOST_CC)"

$(CORE_VIEWPORT2D_LIB): $(CORE_BASE_LIB)
	$(MAKE) -C $(CORE_VIEWPORT2D_DIR) CC="$(HOST_CC)"

$(PROGRAM_BUILD_DIR) $(PROGRAM_BIN_DIR) $(HOST_TEST_DIR) $(TEST_OBJ_DIR) $(TEST_BIN_DIR):
	@mkdir -p "$@"

$(PROGRAM_CC_STAMP): makefile $(PROGRAM_CC_DEP) | $(PROGRAM_BUILD_DIR)
	@printf '%s\n' "$(PROGRAM_CC)" > "$@.tmp"
	@cmp -s "$@.tmp" "$@" 2>/dev/null || mv "$@.tmp" "$@"
	@rm -f "$@.tmp"

$(APP_OBJS) $(HEADLESS_OBJS): $(PROGRAM_CC_STAMP)

$(PROGRAM_OBJ_DIR)/%.c.o: %.c $(PROGRAM_CC_STAMP)
	@mkdir -p "$(dir $@)"
	$(PROGRAM_CC) $(CFLAGS) -c "$<" -o "$@"

$(TEST_OBJ_DIR)/%.c.o: %.c
	@mkdir -p "$(dir $@)"
	$(HOST_CC) $(CFLAGS) -c "$<" -o "$@"

$(APP_TARGET): $(APP_OBJS) $(SHARED_LIBS)
	@mkdir -p "$(dir $@)"
	$(HOST_CC) $(APP_OBJS) $(SHARED_LIBS) -o "$@" $(APP_LDLIBS)

$(HEADLESS_TARGET): $(HEADLESS_OBJS) $(SHARED_LIBS)
	@mkdir -p "$(dir $@)"
	$(HOST_CC) $(HEADLESS_OBJS) $(SHARED_LIBS) -o "$@" $(LDLIBS) $(PNG_LIBS)

$(TEST_TARGET): $(TEST_OBJS) $(SHARED_LIBS)
	@mkdir -p "$(dir $@)"
	$(HOST_CC) $(TEST_OBJS) $(SHARED_LIBS) -o "$@" $(APP_LDLIBS)

run: $(APP_TARGET)
	"$(APP_TARGET)"

run-headless: $(HEADLESS_TARGET)
	"$(HEADLESS_TARGET)" --headless --smoke-frames 2 --print-lifecycle --no-persist

test: $(TEST_TARGET)
	"$(TEST_TARGET)"

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
	@"$(PACKAGE_DYLIB_BUNDLER)" "$(PACKAGE_MACOS_DIR)/$(APP_BIN)" "$(PACKAGE_FRAMEWORKS_DIR)"
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
