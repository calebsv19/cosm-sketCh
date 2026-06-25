#!/bin/sh
set -eu

RELEASE_MK="make/release.mk"

if /usr/bin/grep -En 'echo[^\\]*\$\((RELEASE_CODESIGN_IDENTITY|APPLE_SIGN_IDENTITY|APPLE_NOTARY_PROFILE)\)' "$RELEASE_MK" >/dev/null; then
    echo "release-secret-audit failed: release recipes must not echo signing or notary secret values" >&2
    exit 1
fi

if ! /usr/bin/grep -Fq -- '--keychain-profile "$(APPLE_NOTARY_PROFILE)"' "$RELEASE_MK"; then
    echo "release-secret-audit failed: release-notarize must use the configured keychain profile" >&2
    exit 1
fi

if ! /usr/bin/grep -Fq -- '> "$(RELEASE_DIR)/notary_submit.json"' "$RELEASE_MK"; then
    echo "release-secret-audit failed: notary output must be captured under the release directory" >&2
    exit 1
fi

echo "release-secret-audit passed."
