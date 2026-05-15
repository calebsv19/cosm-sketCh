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

$(CORE_AUTHORED_TEXTURE_LIB): $(CORE_AUTHORED_TEXTURE_LOCAL_SRCS) | $(SHARED_BUILD_DIR)
	@$(MAKE) -C "$(CORE_AUTHORED_TEXTURE_DIR)" clean CC="$(SHARED_CC)"
	@$(MAKE) -C "$(CORE_AUTHORED_TEXTURE_DIR)" CC="$(SHARED_CC)"
	@cp "$(CORE_AUTHORED_TEXTURE_DIR)/build/libcore_authored_texture.a" "$@"

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
