include make/config.mk
include make/target.mk
include make/shared.mk
include make/flags.mk
include make/paths.mk
include make/sources.mk
include make/objects.mk

.PHONY: all build clean run run-headless test test-list test-suite visual-harness visual-artifact identity print-identity \
	fisics-compiler fisics-build fisics-run fisics-run-headless \
	fisics-package-desktop-refresh \
	export-snapshot-json snapshot-bridge-check snapshot-bridge-import headless-probe-matrix \
	vulkan-rollout-contract vulkan-rollout-self-test \
	memory-check-build memory-check-run memory-check-audit \
	shared-mode shared-subtree-check shared-subtree-prepare \
	package-desktop package-desktop-smoke package-desktop-self-test \
	package-desktop-copy-desktop package-desktop-sync package-desktop-open \
	package-desktop-remove package-desktop-refresh \
	release-secret-audit release-contract release-clean release-build \
	release-bundle-audit release-sign release-verify release-verify-signed \
	release-notarize release-staple release-verify-notarized release-artifact \
	release-distribute

include make/rules-build.mk
include make/rules-test.mk
include make/rules-memory-check.mk
include make/package-macos.mk
include make/release.mk

-include $(APP_DEPS) $(HEADLESS_DEPS) $(TEST_DEPS)
