# sketCh Desktop Packaging

Last updated: 2026-04-25

## Bundle Contract

- output app: `dist/sketCh.app`
- Desktop copy target: `/Users/<user>/Desktop/sketCh.app`
- launcher: `Contents/MacOS/sketch-launcher`
- runtime binary: `Contents/MacOS/drawing-program-bin`
- app bundle metadata:
  - bundle id: `com.cosm.sketch`
  - product name: `sketCh`
  - version: `0.1.0`
  - icon file: `Contents/Resources/AppIcon.icns` via `CFBundleIconFile=AppIcon`
- bundled frameworks live under `Contents/Frameworks/`
- bundled resources currently include:
  - `Contents/Resources/shared/assets/fonts/*`
  - `Contents/Resources/AppIcon.icns` when `PACKAGE_APP_ICON_SRC` or `PACKAGE_APP_ICONSET_SRC` resolves

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

## Launcher Runtime Contract

- `--print-config` prints:
  - `DRAWING_PROGRAM_RUNTIME_DIR`
  - `DRAWING_PROGRAM_RESOURCES_DIR`
  - `DRAWING_PROGRAM_APP_BIN`
- `--self-test` verifies:
  - packaged runtime binary is executable
  - one bounded headless smoke run succeeds:
    - `drawing-program-bin --headless --smoke-frames 1 --no-persist`
- launcher runtime root:
  - default: `~/Library/Application Support/sketCh/runtime`
  - tmp fallback: `${TMPDIR:-/tmp}/sketch-runtime`
- launcher runtime directories created on boot:
  - `<runtime>/input`
  - `<runtime>/output`
- runtime override:
  - `DRAWING_PROGRAM_RUNTIME_DIR=<path>`

## Packaged Resource And Framework Contract

- package step copies fonts into:
  - `Contents/Resources/shared/assets/fonts/`
- default local icon store:
  - `drawing_program/tools/packaging/macos/local_app_icon/AppIcon.icns`
  - `drawing_program/tools/packaging/macos/local_app_icon/AppIcon.iconset`
  - plain `make -C drawing_program package-desktop-refresh` now consumes that local store by default when present
  - the local icon store is intentionally gitignored so icon refreshes do not pollute repo state
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

## Current Limits

- This doc describes the current local packaged-app workflow only.
- It does not document a public release-sign, notarize, staple, or release-distribute lane for `drawing_program`.
- It does not prove a fresh packaging rerun on its own; use current verification records and release docs for bounded milestone context.
