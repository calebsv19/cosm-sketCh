package-desktop:
	@$(MAKE) BUILD_TOOLCHAIN="$(PACKAGE_TOOLCHAIN)" "$(PACKAGE_SOURCE_BIN)"
	@rm -rf "$(PACKAGE_APP_DIR)"
	@mkdir -p "$(PACKAGE_MACOS_DIR)" "$(PACKAGE_RESOURCES_DIR)" "$(PACKAGE_FRAMEWORKS_DIR)" "$(PACKAGE_SHARED_FONTS_DIR)" "$(PACKAGE_RESOURCES_DIR)/vk_renderer"
	@cp "$(PACKAGE_INFO_PLIST_SRC)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier $(PACKAGE_BUNDLE_ID)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Set :CFBundleName $(PACKAGE_DISPLAY_NAME)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Set :CFBundleDisplayName $(PACKAGE_DISPLAY_NAME)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Set :CFBundleVersion $(PACKAGE_PROGRAM_VERSION)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $(PACKAGE_PROGRAM_VERSION)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Add :DrawingProgramPackageProfile string $(PACKAGE_PROFILE)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Add :DrawingProgramRuntimeNamespace string $(PACKAGE_RUNTIME_NAMESPACE)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Add :DrawingProgramLogNamespace string $(PACKAGE_LOG_NAMESPACE)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
	@/usr/libexec/PlistBuddy -c "Add :DrawingProgramBuildLabel string $(PACKAGE_BUILD_LABEL)" "$(PACKAGE_CONTENTS_DIR)/Info.plist"
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
	@cp -R "$(VK_RENDERER_DIR)/shaders" "$(PACKAGE_RESOURCES_DIR)/vk_renderer/"
	@PACKAGE_DEP_SEARCH_ROOTS="$(TARGET_DEP_SEARCH_ROOTS)" \
		"$(PACKAGE_DYLIB_BUNDLER)" "$(PACKAGE_MACOS_DIR)/$(APP_BIN)" "$(PACKAGE_FRAMEWORKS_DIR)"
	@for dylib in "$(PACKAGE_FRAMEWORKS_DIR)"/*.dylib; do \
		[ -f "$$dylib" ] || continue; \
		codesign --force --sign "$(PACKAGE_ADHOC_SIGN_IDENTITY)" "$$dylib"; \
	done
	@codesign --force --sign "$(PACKAGE_ADHOC_SIGN_IDENTITY)" "$(PACKAGE_MACOS_DIR)/$(APP_BIN)"
	@codesign --force --sign "$(PACKAGE_ADHOC_SIGN_IDENTITY)" "$(PACKAGE_MACOS_DIR)/$(LAUNCHER_BIN)"
	@if [ "$(PACKAGE_EMBED_BUILD_IDENTITY)" = "1" ]; then \
		python3 "$(MEW1_TOOL)" write-identity \
			--output "$(PACKAGE_RESOURCES_DIR)/build_identity.json" \
			--source-root "$(CURDIR)" \
			--binary "$(PACKAGE_MACOS_DIR)/$(APP_BIN)" \
			--profile "$(PACKAGE_PROFILE)" \
			--program drawing_program \
			--product sketCh \
			--version "$(PACKAGE_PROGRAM_VERSION)" \
			--architecture "$(TARGET_ARCH)" \
			--toolchain "$(PACKAGE_TOOLCHAIN)" \
			--build-label "$(PACKAGE_BUILD_LABEL)"; \
	fi
	@codesign --force --sign "$(PACKAGE_ADHOC_SIGN_IDENTITY)" "$(PACKAGE_APP_DIR)"
	@codesign --verify --deep --strict "$(PACKAGE_APP_DIR)"
	@echo "Desktop package ready: $(PACKAGE_APP_DIR)"

package-desktop-smoke: package-desktop
	@test -x "$(PACKAGE_MACOS_DIR)/$(LAUNCHER_BIN)" || (echo "Missing launcher"; exit 1)
	@test -x "$(PACKAGE_MACOS_DIR)/$(APP_BIN)" || (echo "Missing app binary"; exit 1)
	@test -f "$(PACKAGE_CONTENTS_DIR)/Info.plist" || (echo "Missing Info.plist"; exit 1)
	@test "$$(/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' "$(PACKAGE_CONTENTS_DIR)/Info.plist")" = "$(PACKAGE_PROGRAM_VERSION)" || (echo "Bundle build version mismatch"; exit 1)
	@test "$$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$(PACKAGE_CONTENTS_DIR)/Info.plist")" = "$(PACKAGE_PROGRAM_VERSION)" || (echo "Bundle short version mismatch"; exit 1)
	@test "$$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$(PACKAGE_CONTENTS_DIR)/Info.plist")" = "$(PACKAGE_BUNDLE_ID)" || (echo "Bundle identifier mismatch"; exit 1)
	@test "$$(/usr/libexec/PlistBuddy -c 'Print :DrawingProgramPackageProfile' "$(PACKAGE_CONTENTS_DIR)/Info.plist")" = "$(PACKAGE_PROFILE)" || (echo "Package profile mismatch"; exit 1)
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
	@test -f "$(PACKAGE_FRAMEWORKS_DIR)/libvulkan.1.dylib" || (echo "Missing bundled libvulkan"; exit 1)
	@test -f "$(PACKAGE_FRAMEWORKS_DIR)/libMoltenVK.dylib" || (echo "Missing bundled libMoltenVK"; exit 1)
	@test -f "$(PACKAGE_RESOURCES_DIR)/vk_renderer/shaders/textured.vert.spv" || (echo "Missing bundled Vulkan shader"; exit 1)
	@echo "package-desktop-smoke passed."

package-desktop-self-test: package-desktop-smoke
	@"$(PACKAGE_MACOS_DIR)/$(LAUNCHER_BIN)" --self-test || (echo "package-desktop self-test failed."; exit 1)
	@mkdir -p "$(VULKAN_ROLLOUT_PACKAGE_DIR)"
	@PYTHONDONTWRITEBYTECODE=1 python3 tools/verify-vulkan-rollout.py \
		--shared-root "$(SHARED_VENDOR_DIR)" \
		--app "$(PACKAGE_MACOS_DIR)/$(APP_BIN)" \
		--shader-root "$(PACKAGE_RESOURCES_DIR)/vk_renderer" \
		--moltenvk "$(PACKAGE_FRAMEWORKS_DIR)/libMoltenVK.dylib" \
		--initial-capture "$(VULKAN_ROLLOUT_PACKAGE_DIR)/initial.bmp" \
		--resized-capture "$(VULKAN_ROLLOUT_PACKAGE_DIR)/resized.bmp" \
		--log "$(VULKAN_ROLLOUT_PACKAGE_DIR)/rollout.log" \
		--actual-app-capture "$(VULKAN_ROLLOUT_PACKAGE_DIR)/application.bmp" \
		--actual-app-log "$(VULKAN_ROLLOUT_PACKAGE_DIR)/application.log"
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

package-desktop-main-edit:
	@test -s "$(PACKAGE_APP_ICON_SRC)" || test -d "$(PACKAGE_APP_ICONSET_SRC)" || (echo "Missing Main Edit icon input: $(PACKAGE_APP_ICON_SRC)"; exit 1)
	@test -f "$(MEW1_TOOL)" || (echo "Missing shared MEW1 helper: $(MEW1_TOOL)"; exit 1)
	@before="$$(python3 "$(MEW1_TOOL)" fingerprint --repo "$(CURDIR)")"; \
	$(MAKE) package-desktop-smoke \
		DIST_DIR="$(MAIN_EDIT_DIST_DIR)" \
		PACKAGE_APP_NAME="$(MAIN_EDIT_APP_NAME)" \
		PACKAGE_DISPLAY_NAME="$(MAIN_EDIT_DISPLAY_NAME)" \
		PACKAGE_BUNDLE_ID="$(MAIN_EDIT_BUNDLE_ID)" \
		PACKAGE_PROFILE="$(MAIN_EDIT_PROFILE)" \
		PACKAGE_RUNTIME_NAMESPACE="$(MAIN_EDIT_RUNTIME_NAMESPACE)" \
		PACKAGE_LOG_NAMESPACE="$(MAIN_EDIT_LOG_NAMESPACE)" \
		PACKAGE_BUILD_LABEL="$(MAIN_EDIT_BUILD_LABEL)" \
		PACKAGE_EMBED_BUILD_IDENTITY=1 || exit 1; \
	after="$$(python3 "$(MEW1_TOOL)" fingerprint --repo "$(CURDIR)")"; \
	if [ "$$before" != "$$after" ]; then \
		rm -rf "$(MAIN_EDIT_APP_DIR)"; \
		echo "Source changed during Main Edit packaging; discarded generated package."; \
		exit 1; \
	fi
	@echo "Main Edit desktop package ready: $(MAIN_EDIT_APP_DIR)"

package-desktop-main-edit-self-test: package-desktop-main-edit
	@test -s "$(MAIN_EDIT_APP_DIR)/Contents/Resources/$(PACKAGE_APP_ICON_FILE)" || (echo "Missing bundled Main Edit icon"; exit 1)
	@test "$$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIconFile' "$(MAIN_EDIT_APP_DIR)/Contents/Info.plist")" = "$(PACKAGE_APP_ICON_NAME)" || (echo "Main Edit icon metadata mismatch"; exit 1)
	@if [ -f "$(PACKAGE_APP_ICON_SRC)" ]; then cmp "$(PACKAGE_APP_ICON_SRC)" "$(MAIN_EDIT_APP_DIR)/Contents/Resources/$(PACKAGE_APP_ICON_FILE)" || exit 1; fi
	@test "$$('/usr/libexec/PlistBuddy' -c 'Print :CFBundleIdentifier' "$(MAIN_EDIT_APP_DIR)/Contents/Info.plist")" = "$(MAIN_EDIT_BUNDLE_ID)"
	@test "$$('/usr/libexec/PlistBuddy' -c 'Print :CFBundleDisplayName' "$(MAIN_EDIT_APP_DIR)/Contents/Info.plist")" = "$(MAIN_EDIT_DISPLAY_NAME)"
	@test "$$('/usr/libexec/PlistBuddy' -c 'Print :DrawingProgramPackageProfile' "$(MAIN_EDIT_APP_DIR)/Contents/Info.plist")" = "$(MAIN_EDIT_PROFILE)"
	@test "$$('/usr/libexec/PlistBuddy' -c 'Print :DrawingProgramRuntimeNamespace' "$(MAIN_EDIT_APP_DIR)/Contents/Info.plist")" = "$(MAIN_EDIT_RUNTIME_NAMESPACE)"
	@test "$$('/usr/libexec/PlistBuddy' -c 'Print :DrawingProgramLogNamespace' "$(MAIN_EDIT_APP_DIR)/Contents/Info.plist")" = "$(MAIN_EDIT_LOG_NAMESPACE)"
	@test -f "$(MAIN_EDIT_APP_DIR)/Contents/Resources/build_identity.json"
	@python3 "$(MEW1_TOOL)" verify-identity \
		--identity "$(MAIN_EDIT_APP_DIR)/Contents/Resources/build_identity.json" \
		--source-root "$(CURDIR)" \
		--binary "$(MAIN_EDIT_APP_DIR)/Contents/MacOS/$(APP_BIN)" \
		--profile "$(MAIN_EDIT_PROFILE)" \
		--program drawing_program \
		--product sketCh \
		--version "$(PACKAGE_PROGRAM_VERSION)"
	@set -e; \
	fake_home="$(CURDIR)/$(MAIN_EDIT_SELF_TEST_DIR)/home"; \
	isolated_runtime="$(CURDIR)/$(MAIN_EDIT_SELF_TEST_DIR)/runtime"; \
	isolated_logs="$(CURDIR)/$(MAIN_EDIT_SELF_TEST_DIR)/logs"; \
	rm -rf "$$fake_home" "$$isolated_runtime" "$$isolated_logs"; \
	mkdir -p "$$fake_home" "$$isolated_runtime" "$$isolated_logs"; \
	HOME="$$fake_home" DRAWING_PROGRAM_RUNTIME_DIR="$$isolated_runtime" DRAWING_PROGRAM_LOG_DIR="$$isolated_logs" "$(MAIN_EDIT_APP_DIR)/Contents/MacOS/$(LAUNCHER_BIN)" --self-test; \
	config="$$(HOME="$$fake_home" DRAWING_PROGRAM_RUNTIME_DIR="$$isolated_runtime" DRAWING_PROGRAM_LOG_DIR="$$isolated_logs" "$(MAIN_EDIT_APP_DIR)/Contents/MacOS/$(LAUNCHER_BIN)" --print-config)"; \
	printf '%s\n' "$$config"; \
	printf '%s\n' "$$config" | grep -Fqx "DRAWING_PROGRAM_PACKAGE_PROFILE=$(MAIN_EDIT_PROFILE)"; \
	printf '%s\n' "$$config" | grep -Fqx "DRAWING_PROGRAM_RUNTIME_NAMESPACE=$(MAIN_EDIT_RUNTIME_NAMESPACE)"; \
	printf '%s\n' "$$config" | grep -Fqx "DRAWING_PROGRAM_LOG_NAMESPACE=$(MAIN_EDIT_LOG_NAMESPACE)"
	@codesign --verify --deep --strict "$(MAIN_EDIT_APP_DIR)"
	@echo "package-desktop-main-edit-self-test passed."

package-desktop-main-edit-refresh: package-desktop-main-edit-self-test
	@test "$(MAIN_EDIT_DESKTOP_APP_DIR)" != "$(DESKTOP_APP_DIR)" || (echo "Refusing canonical Desktop destination"; exit 1)
	@mkdir -p "$(dir $(MAIN_EDIT_PROCESS_RECEIPT))"
	@python3 "$(MEW1_TOOL)" process-audit --match "$(MAIN_EDIT_DISPLAY_NAME)" --path "$(MAIN_EDIT_DESKTOP_APP_DIR)" > "$(MAIN_EDIT_PROCESS_RECEIPT)"
	@if grep -Fq '"running": true' "$(MAIN_EDIT_PROCESS_RECEIPT)"; then \
		echo "Refusing to replace a running $(MAIN_EDIT_APP_NAME); process receipt: $(MAIN_EDIT_PROCESS_RECEIPT)"; \
		exit 1; \
	fi
	@mkdir -p "$$(dirname "$(MAIN_EDIT_DESKTOP_APP_DIR)")"
	@rm -rf "$(MAIN_EDIT_DESKTOP_APP_DIR)"
	@/usr/bin/ditto "$(MAIN_EDIT_APP_DIR)" "$(MAIN_EDIT_DESKTOP_APP_DIR)"
	@echo "Refreshed $(MAIN_EDIT_APP_NAME) at $(MAIN_EDIT_DESKTOP_APP_DIR)"
