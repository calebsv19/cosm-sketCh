SDL_CFLAGS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --cflags sdl2 2>/dev/null)
SDL_LIBS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --libs sdl2 2>/dev/null)
SDL_TTF_CFLAGS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --cflags sdl2_ttf 2>/dev/null)
SDL_TTF_LIBS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --libs sdl2_ttf 2>/dev/null)
PNG_CFLAGS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --cflags libpng 2>/dev/null)
PNG_LIBS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --libs libpng 2>/dev/null)
JSON_CFLAGS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --cflags json-c 2>/dev/null)
JSON_LIBS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --libs json-c 2>/dev/null)
VULKAN_CFLAGS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --cflags vulkan 2>/dev/null)
VULKAN_LIBS := $(shell env PKG_CONFIG_LIBDIR="$(TARGET_PKG_CONFIG_LIBDIR)" $(PKG_CONFIG) --libs vulkan 2>/dev/null)
ifeq ($(strip $(SDL_CFLAGS)),)
  ifneq ($(wildcard $(TARGET_HOMEBREW_PREFIX)/include/SDL2/SDL.h),)
    SDL_CFLAGS := -I$(TARGET_HOMEBREW_PREFIX)/include -D_THREAD_SAFE
  else ifneq ($(wildcard $(TARGET_ALT_HOMEBREW_PREFIX)/include/SDL2/SDL.h),)
    SDL_CFLAGS := -I$(TARGET_ALT_HOMEBREW_PREFIX)/include -D_THREAD_SAFE
  endif
endif
ifeq ($(strip $(VULKAN_CFLAGS)),)
  VULKAN_CFLAGS := -I$(TARGET_HOMEBREW_PREFIX)/include
endif
ifeq ($(strip $(VULKAN_LIBS)),)
  VULKAN_LIBS := -L$(TARGET_HOMEBREW_PREFIX)/lib -lvulkan
endif

VULKAN_FRAMEWORKS :=
ifeq ($(TARGET_OS),macOS)
VULKAN_FRAMEWORKS := -framework Metal -framework QuartzCore -framework Cocoa -framework IOKit -framework CoreVideo
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

COMMON_INCLUDE_FLAGS := \
	-Iinclude \
	-I$(CORE_BASE_DIR)/include \
	-I$(CORE_OBJECT_DIR)/include \
	-I$(CORE_UNITS_DIR)/include \
	-I$(CORE_SCENE_DIR)/include \
	-I$(CORE_AUTHORED_TEXTURE_DIR)/include \
	-I$(CORE_THEME_DIR)/include \
	-I$(CORE_FONT_DIR)/include \
	-I$(CORE_PACK_DIR)/include \
	-I$(CORE_PANE_DIR)/include \
	-I$(CORE_LAYOUT_DIR)/include \
	-I$(CORE_PANE_MODULE_DIR)/include \
	-I$(CORE_VIEWPORT2D_DIR)/include \
	-I$(KIT_RENDER_DIR)/include \
	-I$(KIT_PANE_DIR)/include \
	-I$(KIT_UI_DIR)/include \
	-I$(KIT_WORKSPACE_AUTHORING_DIR)/include \
	-I$(VK_RENDERER_DIR)/include \
	-I$(VK_RUNTIME_DIR)/include \
	$(SDL_CFLAGS) \
	$(JSON_CFLAGS) \
	$(PNG_CFLAGS) \
	$(SDL_TTF_CFLAGS) \
	$(VULKAN_CFLAGS)
CLANG_LANGUAGE_FLAGS := -std=c11 -Wall -Wextra -pedantic
PROGRAM_CFLAGS := $(COMMON_INCLUDE_FLAGS) -DDRAWING_PROGRAM_USE_VULKAN=0
PROGRAM_DEPFLAGS :=
ifeq ($(BUILD_TOOLCHAIN),clang)
PROGRAM_CFLAGS := $(CLANG_LANGUAGE_FLAGS) $(COMMON_INCLUDE_FLAGS) $(ARCH_FLAGS) -DDRAWING_PROGRAM_USE_VULKAN=1
PROGRAM_DEPFLAGS := -MMD -MP
endif
HOST_CFLAGS := $(CLANG_LANGUAGE_FLAGS) $(COMMON_INCLUDE_FLAGS) $(ARCH_FLAGS) -DDRAWING_PROGRAM_USE_VULKAN=1
HOST_DEPFLAGS := -MMD -MP
LDLIBS := -lm
APP_LDLIBS := $(LDLIBS) $(SDL_LIBS) $(SDL_TTF_LIBS) $(PNG_LIBS) $(JSON_LIBS) $(VULKAN_LIBS) $(VULKAN_FRAMEWORKS)
