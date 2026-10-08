# sketCh Desktop Packaging

Last updated: 2026-08-27

## Bundle Contract

- output app: `dist/sketCh.app`
- Desktop copy target: `/Users/<user>/Desktop/sketCh.app`
- launcher: `Contents/MacOS/sketch-launcher`
- runtime binary: `Contents/MacOS/drawing-program-bin`
- app bundle metadata:
  - bundle id: `com.cosm.sketch`
  - product name: `sketCh`
  - version: read from `VERSION` (currently `0.3.0`)
  - icon file: `Contents/Resources/AppIcon.icns` via `CFBundleIconFile=AppIcon`
- bundled frameworks live under `Contents/Frameworks/`
- bundled resources currently include:
  - `Contents/Resources/shared/assets/fonts/*`
  - `Contents/Resources/vk_renderer/shaders/*`
  - `Contents/Resources/AppIcon.icns` when `PACKAGE_APP_ICON_SRC` or `PACKAGE_APP_ICONSET_SRC` resolves
- packaged Vulkan closure includes `libvulkan.1.dylib` and
  `libMoltenVK.dylib`; the launcher writes a runtime-local ICD manifest pointing
  at the bundled MoltenVK driver.

## Make Targets

- local packaging:
  - `make -C drawing_program package-desktop`
  - `make -C drawing_program package-desktop-smoke`
  - `make -C drawing_program package-desktop-self-test`
  - `make -C drawing_program package-desktop-copy-desktop`
  - `make -C drawing_program package-desktop-sync`
  - `make -C drawing_program package-desktop-open`
  - `make -C drawing_program package-desktop-remove`
  - `make -C drawing_program package-desktop-refresh`
- isolated Main Edit packaging:
  - `make -C drawing_program package-desktop-main-edit`
  - `make -C drawing_program package-desktop-main-edit-self-test`
  - `make -C drawing_program package-desktop-main-edit-refresh`
- release/export prep:
  - `make -C drawing_program release-secret-audit`
  - `make -C drawing_program release-contract`
  - `make -C drawing_program release-build`
  - `make -C drawing_program release-bundle-audit`
  - `make -C drawing_program release-sign APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)"`
  - `make -C drawing_program release-verify`
  - `make -C drawing_program release-verify-signed`
  - `make -C drawing_program release-notarize APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)" APPLE_NOTARY_PROFILE="<profile>"`
  - `make -C drawing_program release-staple`
  - `make -C drawing_program release-verify-notarized`
  - `make -C drawing_program release-artifact TARGET_ARCH=arm64 RELEASE_ROOT=build/release-authenticated/<fresh-job-id>`
  - `make -C drawing_program release-artifact TARGET_ARCH=x86_64 RELEASE_ROOT=build/release-authenticated/<fresh-job-id>`
  - `make -C drawing_program release-distribute RELEASE_ROOT=build/release-authenticated/<fresh-job-id> APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)" APPLE_NOTARY_PROFILE="<profile>"`

Current `release-artifact` output uses the supplied create-only `RELEASE_ROOT`
(`build/release` paths below describe the retained historical layout):
- `build/release/sketCh-<version>-macOS-arm64-stable.zip`
- `build/release/sketCh-<version>-macOS-arm64-stable.zip.sha256`
- `build/release/sketCh-<version>-macOS-arm64-stable.manifest.txt`
- `build/release/sketCh-<version>-macOS-x86_64-stable.zip`
- `build/release/sketCh-<version>-macOS-x86_64-stable.zip.sha256`
- `build/release/sketCh-<version>-macOS-x86_64-stable.manifest.txt`

Current notarized pass:
- the 2026-06-06 `release-distribute` lane completed with accepted Apple notarization for `sketCh-0.2.0-macOS-arm64-stable`
- `release-sign` status output does not print the configured signing identity;
  use `release-secret-audit` or `release-contract` for non-live source readback
  before running any live signing/notary/distribution action

## Release Target Contract

- shared helper: `/Users/calebsv/Desktop/CodeWork/bin/desktop_release_target_contract.sh`
- local secret guard: `tools/packaging/macos/release-secret-audit.sh`
- target vars:
  - `TARGET_OS=macOS`
  - `TARGET_ARCH=arm64 | x86_64`
  - `TARGET_VARIANT=desktop-app`
  - `TARGET_TRIPLE=macOS-<arch>`
  - `RELEASE_PLATFORM=macOS`
  - `RELEASE_ARCH=<arch>`
- arch-specific intermediates now live under:
  - `build/targets/macOS-<arch>/...`
- `dist/sketCh.app` remains the active local bundle path; release artifact coexistence comes from the explicit arch token in `build/release/`

## Builder Constraints

- `TARGET_ARCH=arm64` is the default on Apple Silicon builders and prefers `/opt/homebrew` for pkg-config and dylib discovery.
- `TARGET_ARCH=x86_64` is intended for an Intel macOS builder first.
- `TARGET_ARCH=x86_64` can also work on an Apple Silicon builder only if the host has a usable x86_64 macOS SDK/toolchain path plus matching x86_64 Homebrew dependencies under `/usr/local`.
- mixed-builder overrides are supported through:
  - `TARGET_HOMEBREW_PREFIX=<prefix>`
  - `TARGET_PKG_CONFIG_LIBDIR=<prefix>/lib/pkgconfig:<prefix>/share/pkgconfig`
- package assembly now passes ordered dependency roots to the dylib bundler so `@rpath` resolution prefers the requested target architecture instead of whichever Homebrew prefix appears first.
- vendored shared static libraries are rebuilt into arch-specific copies under `build/targets/macOS-<arch>/shared/` before link/package work so arm64 and x86_64 release passes do not reuse stale archives across target switches.

## Launcher Runtime Contract

- `--print-config` prints:
  - `DRAWING_PROGRAM_PACKAGE_PROFILE`
  - `DRAWING_PROGRAM_RUNTIME_NAMESPACE`
  - `DRAWING_PROGRAM_LOG_NAMESPACE`
  - `DRAWING_PROGRAM_BUILD_LABEL`
  - `DRAWING_PROGRAM_RUNTIME_DIR`
  - `DRAWING_PROGRAM_LOG_DIR`
  - `DRAWING_PROGRAM_RESOURCES_DIR`
  - `VK_ICD_FILENAMES`
  - `VK_DRIVER_FILES`
  - `MOLTENVK_DYLIB`
  - `DRAWING_PROGRAM_APP_BIN`
- `--self-test` verifies:
  - packaged runtime binary is executable
  - one bounded headless smoke run succeeds:
    - `drawing-program-bin --headless --smoke-frames 1 --no-persist`

`package-desktop-self-test` now runs both the existing launcher headless smoke
and the checksum-bound managed Vulkan proof against the packaged binary. The
proof requires validation-clean startup, native captures before and after a
real resize, restart, 2x Retina drawable scale, and one real app-frame capture.
- launcher runtime root:
  - default: `~/Library/Application Support/sketCh/runtime`
  - tmp fallback: `${TMPDIR:-/tmp}/<runtime-namespace>/runtime`
- launcher runtime directories created on boot:
  - `<runtime>/input`
  - `<runtime>/output`
- runtime override:
  - `DRAWING_PROGRAM_RUNTIME_DIR=<path>`

## Persistent Main Edit Package

The Main Edit profile is a local-development artifact, not a release candidate:

- app: `sketCh Main Edit.app`
- bundle ID: `com.cosm.sketch.main-edit`
- profile: `main-edit`
- runtime/log namespace: `DrawingProgram-Main-Edit`
- build root: `build/targets/<target>/dist/dev/main-edit/`
- embedded identity: `Contents/Resources/build_identity.json`

The identity records the source branch, commit, dirty state, complete source
fingerprint, target architecture, toolchain, build label, and packaged binary
digest. Packaging compares source fingerprints before and after assembly and
discards the generated Main Edit bundle if source changes during the build.

`package-desktop-main-edit-self-test` verifies the identity, plist profile,
bundle ID, runtime/log separation, launcher headless smoke, and code signature
using isolated runtime and log roots. The refresh target refuses the canonical
Desktop destination and stops if a matching Main Edit app process is running.
It never refreshes `sketCh.app`.

## Packaged Resource And Framework Contract

- package step copies fonts into:
  - `Contents/Resources/shared/assets/fonts/`
- package step now sources those shared fonts from vendored `third_party/codework_shared/assets/fonts/` by default, with workspace-linked shared assets used only when `SHARED_MODE=workspace-linked`
- default local icon store:
  - `drawing_program/tools/packaging/macos/local_app_icon/AppIcon.icns`
  - `drawing_program/tools/packaging/macos/local_app_icon/AppIcon.iconset`
  - plain `make -C drawing_program package-desktop-refresh` now consumes that local store by default when present
  - the local icon store is intentionally gitignored so icon refreshes do not pollute repo state
- canonical local icon source:
  - `/Users/<user>/Desktop/icns/sketch.icns`
  - sync into the packaging lane with:
    - `bin/sync_desktop_icns.sh sketch`
    - `bin/sync_desktop_icns.sh --refresh sketch`
- package step bundles an app icon when a source artifact is available:
  - preferred direct source: `PACKAGE_APP_ICON_SRC=<path-to-AppIcon.icns>`
  - fallback structured source: `PACKAGE_APP_ICONSET_SRC=<path-to-AppIcon.iconset>`
  - default local lookup roots:
    - `tools/packaging/macos/local_app_icon/AppIcon.icns`
    - `tools/packaging/macos/local_app_icon/AppIcon.iconset`
- package step creates:
  - `Contents/Frameworks/`
- package step rewrites local dylib install names into the app bundle and ad-hoc signs:
  - bundled dylibs
  - `drawing-program-bin`
  - `sketch-launcher`
  - `sketCh.app`
- Desktop sync targets now copy the signed `.app` bundle with `ditto` instead of raw `cp -R` so the Desktop copy preserves valid framework signatures after packaging
- because `tools/packaging/macos/local_app_icon/` is gitignored, a fresh clone does not automatically carry your chosen icon until you copy one into that directory

## Recommended Local Validation

1. `make -C drawing_program clean && make -C drawing_program`
2. `make -C drawing_program test`
3. `make -C drawing_program run-headless`
4. `make -C drawing_program visual-harness`
5. `make -C drawing_program package-desktop-self-test PACKAGE_APP_ICON_SRC=<path-to-AppIcon.icns>`
6. `make -C drawing_program package-desktop-refresh PACKAGE_APP_ICON_SRC=<path-to-AppIcon.icns>`
7. `/Users/<user>/Desktop/sketCh.app/Contents/MacOS/sketch-launcher --print-config`
8. `open /Users/<user>/Desktop/sketCh.app`
9. `make -C drawing_program release-secret-audit`
10. `make -C drawing_program release-contract TARGET_ARCH=x86_64`
11. `make -C drawing_program release-bundle-audit TARGET_ARCH=x86_64`

## R6 Visual Proof Boundary

- `make -C drawing_program visual-artifact` is the source-run first-frame
  proof route and writes the ignored local artifact
  `visual_artifacts/sketch_first_frame.bmp`.
- `visual-harness` remains a build/readiness target; it is not the generated
  image proof route.
- Generated R6 proof images under `visual_artifacts/` are not package payloads.
- R6 package proof is `make -C drawing_program package-desktop-self-test`.
  That target rebuilds the package, runs package smoke checks for launcher,
  runtime binary, `Info.plist`, architecture, optional icon payload, and then
  runs launcher `--self-test` against the packaged runtime binary.
- No `package-visual-artifact` target is currently justified. Add one only for
  a release-gated follow-on where package/runtime mismatch risk is concrete,
  a release candidate needs visual evidence from the `.app`, or launcher
  resource paths/bundle sandbox behavior are the feature under review.

## Current Limits

- This doc describes the current local packaged-app workflow only.
- The 2026-06-06 pass did produce a fresh notarized artifact set for the current `0.2.0` macOS arm64 worktree.
- It does not prove a fresh packaging rerun on its own; use current verification records and release docs for bounded milestone context.

## Isolated release packaging

`make release-artifact-disposable RELEASE_ROOT=build/release-authenticated/<job-id>` creates a fresh sketCh.app, ZIP, checksum and source-bound manifest. Existing roots, traversal and symlink ancestors are rejected. The split-root Release Control route also accepts an absolute bound job or target under the configured data workspace at `drawing_program/build/release-authenticated/`; arbitrary absolute paths remain forbidden. The signed `release-artifact` entrypoint enforces the same create-only root and directs both its app and final ZIP/checksum/manifest there, preserving existing `dist` and release outputs. `release-package-self-test` preserves the full standard package self-test, including Vulkan validation, resize and real-app capture, while isolating runtime, logs and proof output under a temporary build directory. Installed apps are not replaced. Developer ID authentication and publication remain separate Decision 1 and Decision 2 stages.
