SHARED_VENDOR_DIR ?= third_party/codework_shared
SHARED_WORKSPACE_DIR ?= ../shared
SHARED_MODE ?= vendored-subtree

CORE_BASE_DIR ?= $(SHARED_VENDOR_DIR)/core/core_base
CORE_OBJECT_DIR ?= $(SHARED_VENDOR_DIR)/core/core_object
CORE_UNITS_DIR ?= $(SHARED_VENDOR_DIR)/core/core_units
CORE_SCENE_DIR ?= $(SHARED_VENDOR_DIR)/core/core_scene
CORE_AUTHORED_TEXTURE_DIR ?= $(SHARED_VENDOR_DIR)/core/core_authored_texture
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
CORE_AUTHORED_TEXTURE_DIR := $(SHARED_WORKSPACE_DIR)/core/core_authored_texture
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

ifeq ($(wildcard $(CORE_AUTHORED_TEXTURE_DIR)/include/core_authored_texture.h),)
CORE_AUTHORED_TEXTURE_DIR := $(SHARED_WORKSPACE_DIR)/core/core_authored_texture
endif
