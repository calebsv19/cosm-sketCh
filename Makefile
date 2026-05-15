include make/config.mk
include make/target.mk
include make/shared.mk
include make/flags.mk
include make/paths.mk
include make/sources.mk
include make/objects.mk

.PHONY: all build clean run run-headless test visual-harness identity print-identity \
	export-snapshot-json snapshot-bridge-check snapshot-bridge-import \
	shared-mode shared-subtree-check shared-subtree-prepare \
	package-desktop package-desktop-smoke package-desktop-self-test \
	package-desktop-copy-desktop package-desktop-sync package-desktop-open \
	package-desktop-remove package-desktop-refresh \
	release-contract release-clean release-build release-bundle-audit release-sign \
	release-verify release-verify-signed release-notarize release-staple \
	release-verify-notarized release-artifact release-distribute

include make/rules-build.mk
include make/rules-test.mk
include make/package-macos.mk
include make/release.mk

-include $(APP_DEPS) $(HEADLESS_DEPS) $(TEST_DEPS)
