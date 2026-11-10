.DEFAULT_GOAL := all
# Consumer build policy; shared implementation and VERSION files are unchanged.
SHARED_ADOPTION_MANIFEST ?= config/shared_adoption.json
HOST_CC_STAMP := $(HOST_TEST_DIR)/compiler.stamp
SHARED_CC_STAMP := $(SHARED_BUILD_DIR)/compiler.stamp
LINK_STAMP := $(TARGET_BUILD_DIR)/link.stamp
SHARED_DIRS := $(CORE_BASE_DIR) $(CORE_OBJECT_DIR) $(CORE_UNITS_DIR) $(CORE_SCENE_DIR) $(CORE_AUTHORED_TEXTURE_DIR) $(CORE_IO_DIR) $(CORE_THEME_DIR) $(CORE_FONT_DIR) $(CORE_PACK_DIR) $(CORE_PANE_DIR) $(CORE_LAYOUT_DIR) $(CORE_PANE_MODULE_DIR) $(CORE_VIEWPORT2D_DIR) $(KIT_RENDER_DIR) $(KIT_PANE_DIR) $(KIT_UI_DIR) $(KIT_WORKSPACE_AUTHORING_DIR) $(VK_RENDERER_DIR) $(VK_RUNTIME_DIR)
SHARED_ADOPTION_ARGS := --module "core/core_base=$(CORE_BASE_DIR)" --module "core/core_object=$(CORE_OBJECT_DIR)" --module "core/core_units=$(CORE_UNITS_DIR)" --module "core/core_scene=$(CORE_SCENE_DIR)" --module "core/core_authored_texture=$(CORE_AUTHORED_TEXTURE_DIR)" --module "core/core_io=$(CORE_IO_DIR)" --module "core/core_theme=$(CORE_THEME_DIR)" --module "core/core_font=$(CORE_FONT_DIR)" --module "core/core_pack=$(CORE_PACK_DIR)" --module "core/core_pane=$(CORE_PANE_DIR)" --module "core/core_layout=$(CORE_LAYOUT_DIR)" --module "core/core_pane_module=$(CORE_PANE_MODULE_DIR)" --module "core/core_viewport2d=$(CORE_VIEWPORT2D_DIR)" --module "kit/kit_render=$(KIT_RENDER_DIR)" --module "kit/kit_pane=$(KIT_PANE_DIR)" --module "kit/kit_ui=$(KIT_UI_DIR)" --module "kit/kit_workspace_authoring=$(KIT_WORKSPACE_AUTHORING_DIR)" --module "vk_renderer=$(VK_RENDERER_DIR)" --module "vk_runtime=$(VK_RUNTIME_DIR)"
SHARED_INPUTS := $(foreach dir,$(SHARED_DIRS),$(shell find $(dir)/src $(dir)/include -type f 2>/dev/null) $(dir)/Makefile $(dir)/VERSION)
# Expand after all rule files have been included, including rules-build.mk.
BUILD_RULES = $(filter Makefile makefile make/%.mk,$(MAKEFILE_LIST))

.PHONY: build-identity-force shared-input-check test-shared-adoption
build-identity-force:
shared-input-check:
	@python3 tools/verify_shared_adoption.py --manifest "$(SHARED_ADOPTION_MANIFEST)" $(SHARED_ADOPTION_ARGS)
test-shared-adoption: shared-input-check

$(PROGRAM_CC_STAMP): build-identity-force $(PROGRAM_CC_DEP) | shared-input-check
	@mkdir -p "$(@D)"
	@{ printf '%s\n' "$(PROGRAM_CC)" "$(PROGRAM_CFLAGS)" "$(PROGRAM_DEPFLAGS)"; $(HOST_CC) --version; cksum $(BUILD_RULES) $(PROGRAM_CC_DEP); } > "$@.tmp"
	@cmp -s "$@.tmp" "$@" 2>/dev/null || mv "$@.tmp" "$@"
	@rm -f "$@.tmp"

$(HOST_CC_STAMP): build-identity-force | shared-input-check
	@mkdir -p "$(@D)"
	@{ printf '%s\n' "$(HOST_CC)" "$(HOST_CFLAGS)" "$(HOST_DEPFLAGS)"; $(HOST_CC) --version; cksum $(BUILD_RULES); } > "$@.tmp"
	@cmp -s "$@.tmp" "$@" 2>/dev/null || mv "$@.tmp" "$@"
	@rm -f "$@.tmp"

$(SHARED_CC_STAMP): build-identity-force | shared-input-check
	@mkdir -p "$(@D)"
	@{ printf '%s\n' "$(SHARED_CC)" "$(PKG_CONFIG)" "$(CFLAGS)" "$(CPPFLAGS)" "$(COPT)" "$(CSTD)" "$(CWARN)" "$(SDL_CFLAGS)" "$(VULKAN_CFLAGS)" "$(SHARED_DIRS)"; $(HOST_CC) --version; cksum $(BUILD_RULES) tools/shared_compile.py; } > "$@.tmp"
	@cmp -s "$@.tmp" "$@" 2>/dev/null || mv "$@.tmp" "$@"
	@rm -f "$@.tmp"

$(LINK_STAMP): build-identity-force | shared-input-check
	@mkdir -p "$(@D)"
	@printf '%s\n' "$(HOST_CC)" "$(ARCH_FLAGS)" "$(APP_LDLIBS)" "$(SHARED_LIBS)" "$(FISICS_MEMCHECK_LINK_LIBS)" > "$@.tmp"
	@cmp -s "$@.tmp" "$@" 2>/dev/null || mv "$@.tmp" "$@"
	@rm -f "$@.tmp"

# Conservative module closure complements actual external compiler dependencies.
$(SHARED_LIBS): $(SHARED_CC_STAMP) $(SHARED_INPUTS) | shared-input-check
-include $(foreach dir,$(SHARED_DIRS),$(shell find $(dir)/build -name '*.l4.d' -type f 2>/dev/null))
