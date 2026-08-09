test: $(TEST_TARGET)
	"$(TEST_TARGET)"

test-list: $(TEST_TARGET)
	"$(TEST_TARGET)" --list-suites

test-suite: $(TEST_TARGET)
	@test -n "$(TEST_SUITE)" || (echo "TEST_SUITE is required, for example: make test-suite TEST_SUITE=texture-import"; exit 1)
	"$(TEST_TARGET)" --suite "$(TEST_SUITE)"

-include $(APP_DEPS) $(HEADLESS_DEPS) $(TEST_DEPS)

visual-harness: $(APP_TARGET)
	@echo "Built $(APP_TARGET)"

visual-artifact: $(APP_TARGET)
	@mkdir -p "$(VISUAL_ARTIFACT_DIR)"
	@rm -f "$(VISUAL_ARTIFACT_PATH)"
	SDL_VIDEODRIVER="$(VISUAL_ARTIFACT_SDL_VIDEODRIVER)" "$(APP_TARGET)" --visual-artifact "$(VISUAL_ARTIFACT_PATH)"
	@test -s "$(VISUAL_ARTIFACT_PATH)"
	@echo "Drawing visual artifact ready: $(VISUAL_ARTIFACT_PATH)"

vulkan-rollout-contract:
	@PYTHONDONTWRITEBYTECODE=1 python3 tools/verify-vulkan-rollout.py \
		--shared-root "$(SHARED_VENDOR_DIR)"

vulkan-rollout-self-test: $(APP_TARGET) vulkan-rollout-contract
	@mkdir -p "$(VULKAN_ROLLOUT_DIR)"
	@PYTHONDONTWRITEBYTECODE=1 python3 tools/verify-vulkan-rollout.py \
		--shared-root "$(SHARED_VENDOR_DIR)" \
		--app "$(APP_TARGET)" \
		--shader-root "$(VK_RENDERER_DIR)" \
		--initial-capture "$(VULKAN_ROLLOUT_DIR)/initial.bmp" \
		--resized-capture "$(VULKAN_ROLLOUT_DIR)/resized.bmp" \
		--log "$(VULKAN_ROLLOUT_DIR)/rollout.log" \
		--actual-app-capture "$(VULKAN_ROLLOUT_DIR)/application.bmp" \
		--actual-app-log "$(VULKAN_ROLLOUT_DIR)/application.log"

export-snapshot-json: $(HEADLESS_TARGET)
	"$(HEADLESS_TARGET)" --headless --smoke-frames 1 --preset "$(EXPORT_PRESET)" --export-json "$(EXPORT_JSON)"
	@echo "Drawing snapshot debug JSON exported: $(EXPORT_JSON)"

snapshot-bridge-check: $(HEADLESS_TARGET)
	"$(HEADLESS_TARGET)" --headless --smoke-frames 1 --bridge-workspace-preset "$(WORKSPACE_PRESET)"
	@echo "Workspace preset bridge check passed: $(WORKSPACE_PRESET)"

snapshot-bridge-import: $(HEADLESS_TARGET)
	"$(HEADLESS_TARGET)" --headless --smoke-frames 1 --bridge-workspace-preset "$(WORKSPACE_PRESET)" --bridge-workspace-import
	@echo "Workspace preset import applied: $(WORKSPACE_PRESET)"

headless-probe-matrix: $(HEADLESS_TARGET)
	@mkdir -p "$(HEADLESS_PROBE_DIR)"
	@rm -f "$(HEADLESS_PROBE_PRESET)" "$(HEADLESS_PROBE_JSON)"
	"$(HEADLESS_TARGET)" --headless --smoke-frames 1 --preset "$(HEADLESS_PROBE_PRESET)"
	"$(HEADLESS_TARGET)" --headless --smoke-frames 1 --preset "$(HEADLESS_PROBE_PRESET)" --export-json "$(HEADLESS_PROBE_JSON)" --no-persist
	"$(HEADLESS_TARGET)" --headless --smoke-frames 1 --preset "$(HEADLESS_PROBE_PRESET)" --bridge-workspace-preset "$(WORKSPACE_PRESET)" --no-persist
	"$(HEADLESS_TARGET)" --headless --smoke-frames 1 --preset "$(HEADLESS_PROBE_PRESET)" --bridge-workspace-preset "$(WORKSPACE_PRESET)" --bridge-workspace-import --no-persist
	@test -s "$(HEADLESS_PROBE_PRESET)"
	@test -s "$(HEADLESS_PROBE_JSON)"
	@echo "Headless probe matrix passed: preset=$(HEADLESS_PROBE_PRESET) json=$(HEADLESS_PROBE_JSON) workspace=$(WORKSPACE_PRESET)"
