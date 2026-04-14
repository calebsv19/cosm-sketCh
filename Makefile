PROGRAM_KEY := drawing_program
RELEASE_PRODUCT_NAME := sketCh
RELEASE_BUNDLE_ID := com.cosm.sketch
LAUNCHER_BIN := sketch-launcher
APP_BIN := drawing-program-bin
VERSION_FILE := VERSION

CC := cc
PKG_CONFIG ?= pkg-config
CORE_BASE_DIR ?= ../shared/core/core_base
CORE_THEME_DIR ?= ../shared/core/core_theme
CORE_FONT_DIR ?= ../shared/core/core_font
CORE_PACK_DIR ?= ../shared/core/core_pack
CORE_PANE_DIR ?= ../shared/core/core_pane
CORE_LAYOUT_DIR ?= ../shared/core/core_layout
CORE_PANE_MODULE_DIR ?= ../shared/core/core_pane_module
SHARED_VENDOR_DIR ?= third_party/codework_shared
SHARED_MODE ?= workspace-linked

ifeq ($(SHARED_MODE),vendored-subtree)
CORE_BASE_DIR := $(SHARED_VENDOR_DIR)/core/core_base
CORE_THEME_DIR := $(SHARED_VENDOR_DIR)/core/core_theme
CORE_FONT_DIR := $(SHARED_VENDOR_DIR)/core/core_font
CORE_PACK_DIR := $(SHARED_VENDOR_DIR)/core/core_pack
CORE_PANE_DIR := $(SHARED_VENDOR_DIR)/core/core_pane
CORE_LAYOUT_DIR := $(SHARED_VENDOR_DIR)/core/core_layout
CORE_PANE_MODULE_DIR := $(SHARED_VENDOR_DIR)/core/core_pane_module
endif

SDL_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2 2>/dev/null)
SDL_LIBS := $(shell $(PKG_CONFIG) --libs sdl2 2>/dev/null)
SDL_TTF_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2_ttf 2>/dev/null)
SDL_TTF_LIBS := $(shell $(PKG_CONFIG) --libs sdl2_ttf 2>/dev/null)
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

CFLAGS := -std=c11 -Wall -Wextra -pedantic \
	-Iinclude \
	-I$(CORE_BASE_DIR)/include \
	-I$(CORE_THEME_DIR)/include \
	-I$(CORE_FONT_DIR)/include \
	-I$(CORE_PACK_DIR)/include \
	-I$(CORE_PANE_DIR)/include \
	-I$(CORE_LAYOUT_DIR)/include \
	-I$(CORE_PANE_MODULE_DIR)/include \
	$(SDL_CFLAGS) \
	$(SDL_TTF_CFLAGS)
LDLIBS := -lm
APP_LDLIBS := $(LDLIBS) $(SDL_LIBS) $(SDL_TTF_LIBS)

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin

APP_LOCAL_SRCS := \
	src/domain/drawing_program_color_model.c \
	src/domain/drawing_program_document.c \
	src/domain/drawing_program_editor_state.c \
	src/domain/drawing_program_history.c \
	src/domain/drawing_program_layer_raster.c \
	src/domain/drawing_program_selection.c \
	src/io/session/drawing_program_snapshot.c \
	src/domain/drawing_program_viewport.c \
	src/runtime/render/drawing_program_render_domain.c \
	src/runtime/render/drawing_program_render_backend.c \
	src/runtime/orchestration/drawing_program_runtime_orchestration.c \
	src/runtime/canvas/drawing_program_visual_canvas_stroke_ops.c \
	src/runtime/adapters/drawing_program_pane_host.c \
	src/runtime/adapters/drawing_program_overlay_adapter.c \
	src/render/canvas/drawing_program_visual_canvas_world_render.c \
	src/render/frame/drawing_program_visual_frame_render.c \
	src/render/overlay/drawing_program_visual_overlay_render.c \
	src/input/canvas/drawing_program_visual_canvas_action_ops.c \
	src/input/canvas/drawing_program_visual_canvas_coords.c \
	src/input/canvas/drawing_program_visual_canvas_draw_action_ops.c \
	src/input/tools/drawing_program_visual_shape_ops.c \
	src/input/tools/drawing_program_visual_transform_ops.c \
	src/input/layers/drawing_program_visual_layer_actions.c \
	src/input/input_core/drawing_program_visual_input_core.c \
	src/input/input_core/drawing_program_visual_input_keymap.c \
	src/input/input_core/drawing_program_visual_input_handlers.c \
	src/ui/layout/drawing_program_visual_layout.c \
	src/ui/layers/drawing_program_visual_layer_opacity.c \
	src/ui/pane/drawing_program_visual_pane_bindings.c \
	src/ui/panel/drawing_program_visual_panel_render.c \
	src/ui/theme/drawing_program_visual_theme.c \
	src/ui/resources/drawing_program_visual_resources.c \
	src/ui/tools/drawing_program_visual_tool_options.c \
	src/ui/text/drawing_program_visual_text_render.c \
	src/app/drawing_program_app_main.c \
	src/app/main.c \
	src/app/drawing_program_app_visual_main.c

HEADLESS_LOCAL_SRCS := \
	src/domain/drawing_program_color_model.c \
	src/domain/drawing_program_document.c \
	src/domain/drawing_program_editor_state.c \
	src/domain/drawing_program_history.c \
	src/domain/drawing_program_layer_raster.c \
	src/domain/drawing_program_selection.c \
	src/io/session/drawing_program_snapshot.c \
	src/domain/drawing_program_viewport.c \
	src/runtime/render/drawing_program_render_domain.c \
	src/runtime/render/drawing_program_render_backend.c \
	src/runtime/orchestration/drawing_program_runtime_orchestration.c \
	src/runtime/canvas/drawing_program_visual_canvas_stroke_ops.c \
	src/runtime/adapters/drawing_program_pane_host.c \
	src/runtime/adapters/drawing_program_overlay_adapter.c \
	src/app/drawing_program_app_main.c \
	src/app/drawing_program_app_headless_main.c

TEST_LOCAL_SRCS := \
	src/domain/drawing_program_color_model.c \
	src/domain/drawing_program_document.c \
	src/domain/drawing_program_editor_state.c \
	src/domain/drawing_program_history.c \
	src/domain/drawing_program_layer_raster.c \
	src/domain/drawing_program_selection.c \
	src/io/session/drawing_program_snapshot.c \
	src/domain/drawing_program_viewport.c \
	src/runtime/render/drawing_program_render_domain.c \
	src/runtime/render/drawing_program_render_backend.c \
	src/runtime/orchestration/drawing_program_runtime_orchestration.c \
	src/runtime/canvas/drawing_program_visual_canvas_stroke_ops.c \
	src/runtime/adapters/drawing_program_pane_host.c \
	src/runtime/adapters/drawing_program_overlay_adapter.c \
	src/render/canvas/drawing_program_visual_canvas_world_render.c \
	src/render/frame/drawing_program_visual_frame_render.c \
	src/render/overlay/drawing_program_visual_overlay_render.c \
	src/input/canvas/drawing_program_visual_canvas_action_ops.c \
	src/input/canvas/drawing_program_visual_canvas_coords.c \
	src/input/canvas/drawing_program_visual_canvas_draw_action_ops.c \
	src/input/tools/drawing_program_visual_shape_ops.c \
	src/input/tools/drawing_program_visual_transform_ops.c \
	src/input/layers/drawing_program_visual_layer_actions.c \
	src/input/input_core/drawing_program_visual_input_core.c \
	src/input/input_core/drawing_program_visual_input_keymap.c \
	src/input/input_core/drawing_program_visual_input_handlers.c \
	src/ui/layout/drawing_program_visual_layout.c \
	src/ui/layers/drawing_program_visual_layer_opacity.c \
	src/ui/pane/drawing_program_visual_pane_bindings.c \
	src/ui/panel/drawing_program_visual_panel_render.c \
	src/ui/theme/drawing_program_visual_theme.c \
	src/ui/resources/drawing_program_visual_resources.c \
	src/ui/tools/drawing_program_visual_tool_options.c \
	src/ui/text/drawing_program_visual_text_render.c \
	src/app/drawing_program_app_main.c \
	src/app/drawing_program_app_visual_main.c \
	tests/drawing_program_lifecycle_test.c

APP_OBJS := $(APP_LOCAL_SRCS:%=$(OBJ_DIR)/%.o)
HEADLESS_OBJS := $(HEADLESS_LOCAL_SRCS:%=$(OBJ_DIR)/%.o)
TEST_OBJS := $(TEST_LOCAL_SRCS:%=$(OBJ_DIR)/%.o)

APP_TARGET := $(BIN_DIR)/$(APP_BIN)
HEADLESS_TARGET := $(BIN_DIR)/drawing-program-headless
TEST_TARGET := $(BIN_DIR)/drawing-program-test

CORE_BASE_LIB := $(CORE_BASE_DIR)/build/libcore_base.a
CORE_THEME_LIB := $(CORE_THEME_DIR)/build/libcore_theme.a
CORE_FONT_LIB := $(CORE_FONT_DIR)/build/libcore_font.a
CORE_PACK_LIB := $(CORE_PACK_DIR)/build/libcore_pack.a
CORE_PANE_LIB := $(CORE_PANE_DIR)/build/libcore_pane.a
CORE_LAYOUT_LIB := $(CORE_LAYOUT_DIR)/build/libcore_layout.a
CORE_PANE_MODULE_LIB := $(CORE_PANE_MODULE_DIR)/build/libcore_pane_module.a
SHARED_LIBS := $(CORE_PACK_LIB) $(CORE_PANE_LIB) $(CORE_LAYOUT_LIB) $(CORE_PANE_MODULE_LIB) $(CORE_THEME_LIB) $(CORE_FONT_LIB) $(CORE_BASE_LIB)

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
PACKAGE_ADHOC_SIGN_IDENTITY ?= -
DESKTOP_APP_DIR ?= $(HOME)/Desktop/$(PACKAGE_APP_NAME)
EXPORT_PRESET ?= data/last_session.pack
EXPORT_JSON ?= /tmp/drawing_program_snapshot_debug.json
WORKSPACE_PRESET ?= ../workspace_sandbox/data/presets/sketch_layout_v1.pack
PACKAGE_FONTS_SRC_PRIMARY := ../shared/assets/fonts
PACKAGE_FONTS_SRC_VENDOR := third_party/codework_shared/assets/fonts

.PHONY: all build clean run run-headless test visual-harness identity print-identity \
	export-snapshot-json snapshot-bridge-check snapshot-bridge-import \
	shared-mode shared-subtree-check shared-subtree-prepare \
	package-desktop package-desktop-smoke package-desktop-self-test \
	package-desktop-copy-desktop package-desktop-sync package-desktop-open \
	package-desktop-remove package-desktop-refresh

all: build

build: $(APP_TARGET) $(HEADLESS_TARGET)

$(CORE_BASE_LIB):
	$(MAKE) -C $(CORE_BASE_DIR)

$(CORE_PACK_LIB): $(CORE_BASE_LIB)
	$(MAKE) -C $(CORE_PACK_DIR)

$(CORE_THEME_LIB): $(CORE_BASE_LIB)
	$(MAKE) -C $(CORE_THEME_DIR)

$(CORE_FONT_LIB): $(CORE_BASE_LIB)
	$(MAKE) -C $(CORE_FONT_DIR)

$(CORE_PANE_LIB):
	$(MAKE) -C $(CORE_PANE_DIR)

$(CORE_LAYOUT_LIB):
	$(MAKE) -C $(CORE_LAYOUT_DIR)

$(CORE_PANE_MODULE_LIB):
	$(MAKE) -C $(CORE_PANE_MODULE_DIR)

$(OBJ_DIR)/%.c.o: %.c
	@mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) -c "$<" -o "$@"

$(APP_TARGET): $(APP_OBJS) $(SHARED_LIBS)
	@mkdir -p "$(BIN_DIR)"
	$(CC) $(APP_OBJS) $(SHARED_LIBS) -o "$@" $(APP_LDLIBS)

$(HEADLESS_TARGET): $(HEADLESS_OBJS) $(SHARED_LIBS)
	@mkdir -p "$(BIN_DIR)"
	$(CC) $(HEADLESS_OBJS) $(SHARED_LIBS) -o "$@" $(LDLIBS)

$(TEST_TARGET): $(TEST_OBJS) $(SHARED_LIBS)
	@mkdir -p "$(BIN_DIR)"
	$(CC) $(TEST_OBJS) $(SHARED_LIBS) -o "$@" $(APP_LDLIBS)

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
	@mkdir -p "$(SHARED_VENDOR_DIR)"
	@rsync -a --delete "../shared/" "$(SHARED_VENDOR_DIR)/"
	@if [ -d "../shared/.git" ]; then \
		git -C "../shared" rev-parse HEAD > "$(SHARED_VENDOR_DIR)/.codework_shared_source_commit"; \
	fi
	@echo "Prepared vendored shared snapshot at $(SHARED_VENDOR_DIR)"
	@echo "To build against vendored shared: make SHARED_MODE=vendored-subtree"

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

package-desktop: $(APP_TARGET)
	@rm -rf "$(PACKAGE_APP_DIR)"
	@mkdir -p "$(PACKAGE_MACOS_DIR)" "$(PACKAGE_RESOURCES_DIR)" "$(PACKAGE_FRAMEWORKS_DIR)" "$(PACKAGE_SHARED_FONTS_DIR)"
	@cp "$(PACKAGE_INFO_PLIST_SRC)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@cp "$(APP_TARGET)" "$(PACKAGE_MACOS_DIR)/$(APP_BIN)"
	@cp "$(PACKAGE_LAUNCHER_SRC)" "$(PACKAGE_MACOS_DIR)/$(LAUNCHER_BIN)"
	@chmod +x "$(PACKAGE_MACOS_DIR)/$(APP_BIN)" "$(PACKAGE_MACOS_DIR)/$(LAUNCHER_BIN)"
	@if [ -d "$(PACKAGE_FONTS_SRC_PRIMARY)" ]; then \
		cp -R "$(PACKAGE_FONTS_SRC_PRIMARY)"/. "$(PACKAGE_SHARED_FONTS_DIR)"/; \
	elif [ -d "$(PACKAGE_FONTS_SRC_VENDOR)" ]; then \
		cp -R "$(PACKAGE_FONTS_SRC_VENDOR)"/. "$(PACKAGE_SHARED_FONTS_DIR)"/; \
	else \
		echo "warning: no font source dir found for packaging"; \
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
	@echo "package-desktop-smoke passed."

package-desktop-self-test: package-desktop-smoke
	@"$(PACKAGE_MACOS_DIR)/$(LAUNCHER_BIN)" --self-test || (echo "package-desktop self-test failed."; exit 1)
	@echo "package-desktop-self-test passed."

package-desktop-copy-desktop: package-desktop
	@mkdir -p "$(dir $(DESKTOP_APP_DIR))"
	@rm -rf "$(DESKTOP_APP_DIR)"
	@cp -R "$(PACKAGE_APP_DIR)" "$(DESKTOP_APP_DIR)"
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
	@cp -R "$(PACKAGE_APP_DIR)" "$(DESKTOP_APP_DIR)"
	@echo "Refreshed $(PACKAGE_APP_NAME) at $(DESKTOP_APP_DIR)"
