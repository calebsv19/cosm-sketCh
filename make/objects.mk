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
CORE_AUTHORED_TEXTURE_LOCAL_SRCS := $(wildcard $(CORE_AUTHORED_TEXTURE_DIR)/src/*.c) $(wildcard $(CORE_AUTHORED_TEXTURE_DIR)/include/*.h)
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
CORE_AUTHORED_TEXTURE_LIB := $(SHARED_BUILD_DIR)/libcore_authored_texture.a
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
SHARED_LIBS := $(KIT_WORKSPACE_AUTHORING_LIB) $(KIT_PANE_LIB) $(KIT_RENDER_LIB) $(CORE_PACK_LIB) $(CORE_PANE_LIB) $(CORE_LAYOUT_LIB) $(CORE_PANE_MODULE_LIB) $(CORE_THEME_LIB) $(CORE_FONT_LIB) $(CORE_VIEWPORT2D_LIB) $(CORE_AUTHORED_TEXTURE_LIB) $(CORE_SCENE_LIB) $(CORE_OBJECT_LIB) $(CORE_UNITS_LIB) $(CORE_BASE_LIB)
