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
