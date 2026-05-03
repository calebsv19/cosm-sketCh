# drawing_program Current Truth

Last updated: 2026-05-03

## Program Identity
- Repository directory: `drawing_program/`
- Public references:
  - `docs/keybind_reference.md` (canonical keybind map)
  - `docs/architecture.md` (boundary map)

## Current Shipped State
- The app is in a post-foundation feature-depth lane with retained-object editing, vector tooling, and color-system migration active.
- Shared host/font baseline and shared-subtree workflow are normalized for this repo.
- Workspace Authoring `WA1-S1` through `WA1-S4` plus the font/theme overlay correction are active as the first production host slices.

## Major Lane Summary (Compressed)
- Foundations complete:
  - Phase 3/4 runtime shell and backend boundary locks
  - Phase 5/6 canvas + pane-host interaction baseline
  - Phase 7-12 selection/composition/tool-depth and true-layer compositing foundations
- Retained object/vector lanes complete:
  - Phase 13 object model foundation
  - Phase 14 path tooling v1
  - Phase 15 retained-object inspector v1
  - Phase 16 retained-object edit parity
  - Phase 17 bezier path authoring v1
  - Phase 18 curve editing depth
- Color lane:
  - Phase 19 `S1` complete (`CANVAS | LAYER | COLOR` right-panel scaffold)
  - true-color migration is structurally complete through the current active-paint, raster/object sample, snapshot, color-panel, picker, render, and export lanes

## Current Implemented Behavior (Condensed)
- Editing and composition:
  - layer stack semantics, active-layer mutation routing, and grouped history units are active
  - clipboard composition (`copy/cut/paste`) is history-safe and app-owned
  - retained object selection/move/property edits are history-backed
- Path/vector behavior:
  - path authoring/editing supports point insert/remove/append semantics
  - bezier point toggles, handle drag/edit states, and curved segment rendering are active
- Color contract:
  - raster/document sample storage uses explicit packed true-color `RGBA8` samples, with legacy indexed payloads normalized on load
  - retained object stroke/fill, raster-tool flows, picker sampling, and export composition share the packed-sample color contract
- Snapshot/runtime continuity:
  - snapshot shell lanes support modern true-color migration while preserving legacy compatibility fallback paths
  - workspace preset bridge-check/import flows are present
  - file-panel target queue discovery supports up to 512 `.pack` targets from the active input root and renders them in a clipped scrollable sub-pane
- Workspace authoring:
  - `Alt+C+V` enters authoring mode and toggles back to runtime mode when already active
  - sequential `Alt+C`, release, `Alt+V` also enters/toggles authoring mode, with physical scancode detection for macOS Option-modified keys
  - `Esc` cancels authoring edits and exits authoring mode
  - `Enter` applies a valid authoring draft and exits authoring mode
  - the window title changes to `sketCh [Authoring]` while authoring mode is active
  - authoring mode renders a compact `AUTHORING` panel, functional `APPLY`/`CANCEL` controls, draft status, pane/module rows, and per-pane module labels
  - `Tab` cycles authoring between the pane/module overlay and a font/theme takeover overlay that clears the drawing UI while active
  - the font/theme overlay exposes font preset, font zoom, theme preset, and custom-preset stub controls
  - authoring entry captures a pane/module baseline; Cancel restores it and Apply commits through `core_layout_apply_authoring(...)`
  - authoring entry also captures theme preset, font preset, and font zoom baseline state so font/theme changes stay draft-only until Apply
  - snapshot persistence writes accepted pane/module state only: applied authoring changes survive close/reopen, while active un-applied drafts save as their entry baseline
  - authoring-reserved trigger keys are consumed only while authoring mode is active
  - startup normalizes any saved draft/authoring layout state back to runtime mode
  - module-content swapping is not implemented yet
- Viewport behavior:
  - canvas viewport routing now bridges through shared `core_viewport2d`
  - mouse-wheel zoom preserves the sample under the cursor; right-drag pan uses the same pane-aware viewport state
  - viewport reset now fits the canvas back into the active canvas pane with a small padding margin

## Structure and Ownership
- Required lanes: `docs/`, `src/`, `include/`, `tests/`, `build/`
- Runtime model:
  - explicit run-loop seams for input routing (`intake/normalize/route/invalidate`)
  - explicit update/derive/submit render split (`RS1` contract)
- Data roots (default policy):
  - runtime: `data/runtime`
  - input: `data/input`
  - output: `data/output`
  - optional override: `DRAWING_PROGRAM_RUNTIME_DIR`

## Verification Contract
- Core gates:
  - `make -C drawing_program`
  - `make -C drawing_program test`
  - `make -C drawing_program run-headless`
  - `make -C drawing_program visual-harness`
- Packaging gates:
  - `make -C drawing_program package-desktop-self-test`
  - `make -C drawing_program package-desktop-refresh`
- Snapshot/bridge helpers:
  - `make -C drawing_program export-snapshot-json EXPORT_PRESET=<pack> EXPORT_JSON=<json>`
  - `make -C drawing_program snapshot-bridge-check WORKSPACE_PRESET=../workspace_sandbox/data/presets/sketch_layout_v1.pack`
  - `make -C drawing_program snapshot-bridge-import WORKSPACE_PRESET=../workspace_sandbox/data/presets/sketch_layout_v1.pack`
- Shared mode helpers:
  - `make -C drawing_program shared-mode`
  - `make -C drawing_program shared-subtree-check`
  - `make -C drawing_program shared-subtree-prepare`

## Packaging and Shared Workflow
- `package-desktop*` lane is active with desktop self-test and refresh coverage.
- Shared subtree prepare now routes through `../bin/update_shared_subtrees.sh` (not direct rsync-from-workspace shared tree).

## Current Boundary
- Active Workspace Authoring next slice: `WA1-S5` closeout only.
- Existing color/runtime lanes remain active context, but WA1 should not begin module-content swapping, plugin loading, or `line_drawing` attach inside S5.

## History and Deep Lane References
- Full per-slice historical ledgers and archived plan docs are intentionally kept in private docs:
  - `/Users/calebsv/Desktop/CodeWork/docs/private_program_docs/drawing_program/`
- Use this file as compressed current-state truth, not a full phase-by-phase archive.
