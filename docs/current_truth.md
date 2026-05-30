# drawing_program Current Truth

Last updated: 2026-05-29

## Program Identity
- Repository directory: `drawing_program/`
- Product name: `sketCh`
- Public references:
  - `docs/keybind_reference.md`
  - `docs/architecture.md`
  - `docs/future_intent.md`

## Current Shipped State
- `drawing_program` is a retained-object drawing editor and texture-project host
  with one active edit surface in practice inside a broader atlas-style
  multi-surface workspace shell.
- Workspace Authoring `WA1` first-host attach is complete and no longer treated
  as an experimental integration lane.
- The authored-texture roundtrip and hardening roadmap is complete:
  - texture-project state persists durable per-surface layer opacity, role, and
    material-intent metadata
  - exported manifests now use the strict shared authored-texture vocabulary and
    runtime-facing semantic summaries expected by downstream consumers
  - current export behavior remains compatibility-safe and intentionally bounded
- The reflection roadmap is complete and archived:
  - reflection now persists explicit reflector-line state instead of only legacy
    crosshair booleans
  - the `CANVAS` tab exposes bounded reflector authoring UX
  - current reflected authored output routes through the generalized reflection
    engine
  - current multi-reflector composition semantics are explicit:
    - whole-canvas only
    - one-pass subset composition
    - bounded duplicate collapse and reopen validation
- The right-side editor shell is currently organized around six top-level tabs:
  - `CANVAS`
  - `LAYER`
  - `COLOR`
  - `FILE`
  - `ASSET`
  - `EXPORT`
- The texture-project workspace remains one-active-surface-at-a-time for real
  editing even though the atlas shell can display multiple surfaces at once.

## Renderer And Cache Truth
- Atlas rendering is feature-capable and structurally narrower than earlier
  builds:
  - visible surfaces use per-surface cache entries rather than one active-only
    texture path
  - cached texture presentation is attempted before any slower fallback path
  - old atlas per-pixel presentation has been removed from normal surface draw
  - cache validation is revision-based, including layer-opacity-aware change
    tracking
  - pending rebuild work can drain behind the current frame when a last-good
    texture already exists
  - runtime telemetry records compose, upload, and rebuild timing so the cache
    path can be profiled from inside the app and test lanes
- The composed-source owner now has bounded reuse paths for several important
  cases:
  - direct fast paths for single-effective-layer and simpler full-opacity cases
  - lower-stack reuse for unchanged lower blended content
  - intermediate prefix-cache reuse when only the top of a broad partial-opacity
    suffix changes
  - cached suffix-band reuse when lower-band changes leave the upper suffix
    unchanged
- The dominant remaining renderer risk is still CPU-side compose cost during
  deeper mixed blended rebuilds. Current source/docs truth does not justify a
  Vulkan lane; backend migration remains explicitly deferred.

## Current Risk State
- Overall program risk is now medium rather than high.
- Reflection is no longer a top open-risk lane; the remaining risk is mostly in
  performance/stability of the renderer/cache path and the breadth of the app's
  verification surface.
- The main reasons the app is not yet low risk are:
  - broader mixed partial-opacity scenes can still justify more renderer/CPU
    compose measurement and possibly another bounded optimization lane
  - the app still has a wide persistence/input/workspace regression surface
    compared with a simpler single-mode editor
- The most important remaining product-facing renderer question is no longer
  the two named whole-raster seam contracts; those are now closed and validated
  through both composed-source and live surface-cache coverage.
- The next renderer decision is whether broader mixed partial-opacity scenes
  still justify another fresh bounded optimization lane, not whether the
  just-closed seam-lowering work is stable.

## Current Verification Baseline
- Recent bounded lanes have passed:
  - `make -C /Users/calebsv/Desktop/CodeWork/drawing_program`
  - `make -C /Users/calebsv/Desktop/CodeWork/drawing_program test`
  - `make -C /Users/calebsv/Desktop/CodeWork/drawing_program run-headless`
  - `make -C /Users/calebsv/Desktop/CodeWork/drawing_program package-desktop-self-test`
  - `make -C /Users/calebsv/Desktop/CodeWork/drawing_program package-desktop-refresh`

## Next Boundary
- There is no active renderer optimization lane at the moment.
- The latest renderer residual-seams follow-on is complete and archived in:
  - `/Users/calebsv/Desktop/CodeWork/docs/private_program_docs/drawing_program/archive/2026-05-29_completed_renderer_residual_whole_raster_seams_snapshot/2026-05-28_drawing_program_renderer_residual_whole_raster_seams_plan.md`
- If renderer work resumes, it should start as a fresh bounded plan aimed at a
  newly measured mixed-scene CPU compose problem rather than reopening the
  completed whole-raster seam snapshot.
