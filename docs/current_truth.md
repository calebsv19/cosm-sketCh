# drawing_program Current Truth

Last updated: 2026-07-20

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

## Indexed Tileset Foundation

- Current source has a separate `INDEXED_ATLAS_V1` texture-project profile.
  It owns fixed named source/preview RGBA slots, an exact one-byte-per-logical-
  pixel raster, and typed byte-delta undo/redo without changing the standard
  RGBA model.
- Indexed projects use DPTP v13 with required `DPIP` v1 profile and `DPIL` v1
  raw-index chunks. Standard projects continue to write and load DPTP v12.
- Indexed load rejects missing/malformed chunks, non-density-one surfaces,
  geometry mismatch, unknown profile revisions, and out-of-range indices.
- This is a data/roundtrip foundation only. Indexed painting UI, named atlas
  cell editing, indexed PNG/export generation, Dungeon asset changes, final
  artwork, and renderer cutover are not implemented.
- Workspace-linked proof against `core_authored_texture` 0.2.0 passes. The
  default/package build is not ready because the managed Drawing Program copy
  remains 0.1.3 pending an authorized shared commit and subtree rollout.

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
- Future renderer/cache work should reopen only if broader mixed
  partial-opacity scenes provide new measured evidence for another bounded
  optimization lane; the closed seam-lowering work is not an active blocker.

## Current Structure Baseline
- The 2026-06-24 R0 structure audit and R1 duplication/ownership pass are
  complete.
- The R2 app-state ownership pass is complete and archived; it clarified
  panel shell state, transient canvas confirmation/pick state, tool-option
  normalization, layer opacity ownership, and render-derived runtime reset
  ownership.
- The R3 error/logging pass is complete and archived; it clarified
  CLI/bootstrap, data-root/session-path, texture scene import/browser, texture
  export, and GUI runtime stage diagnostics.
- The R4 security audit is complete and archived. Current source evidence
  shows a local desktop file/import/export/package surface, not a public
  network/admin/CORS surface.
- R4-S1 file root containment is complete:
  - CLI/env runtime/input/output/export roots are treated as intentional local
    user-selected file authority
  - generated child paths under those roots now fail closed on overlong paths
    instead of silently truncating
  - checked joins cover env-derived session children, default texture export
    directories, PNG output paths, overlay PNG paths, and manifest paths
- R4-S2 texture scene import resource handling is complete:
  - authored texture scene JSON files are checked as regular files and capped
    at 1 MiB before parse
  - texture scene object arrays are capped at 1024 objects for direct import
    and browser object listing
  - imported scene/object/type text is bounded against empty values,
    truncation, and ASCII control characters before it reaches app scene state
  - browser listings skip oversized or invalid scene rows instead of exposing
    ambiguous partial state
- R4-S3 export/debug artifact privacy is complete:
  - texture export manifests are treated as portable artifact bodies and are
    covered by tests that reject local absolute/private path strings
  - texture export manifests emit source scene/object ids and relative PNG file
    names, not source scene paths or absolute output paths
  - snapshot debug JSON remains local state/count/geometry output and is
    covered by tests that reject serialized local export paths
  - release artifact manifest generation currently emits release-relative
    metadata; live release artifact creation remains approval-gated
- No first-party `src` or `include` file is above the current `1000` LOC
  structure threshold.
- R0 does not call for source extraction at this checkpoint; future extraction
  should be tied to a lowered threshold or substantial new behavior in one of
  the watch files named in `docs/architecture.md`.
- R1 consolidated high-confidence duplication/ownership seams in build source
  inventories, right-panel file tab contracts, simple themed button drawing,
  and post-load app/session rearm ownership.
- R3-S1 CLI/bootstrap diagnostics are complete:
  - unknown command-line options and unexpected positional arguments now fail
    deterministically during bootstrap instead of being ignored
  - value-taking options now report the specific missing option value
  - accepted argument defaults are unchanged
- R3-S2 data-root/session-path diagnostics are complete:
  - root setup failures identify the affected role and path
  - existing files at required root directory paths are reported as
    not-directory failures
  - session preference file failures include the prefs path and system error
    detail
- R3-S3 texture scene import/browser diagnostics are complete:
  - texture scene import failures identify scene path, object id, primitive
    kind, failing field, and detail at the parse/object/primitive boundary
  - texture scene browser root-open, scene parse, and missing-root-field
    failures include path and field context
- R3-S4 texture export output diagnostics are complete:
  - texture export failures can identify export directory, manifest path,
    output path, surface name, and detail at the export boundary
  - blocked export directory segments report not-directory diagnostics with the
    requested export directory and blocked segment path
- R3-S5 GUI runtime stage diagnostics are complete:
  - GUI app-stage `CoreResult` failures now report stage, code, and message
  - SDL setup failures report the GUI stage, `CORE_ERR_IO`, and SDL detail
  - invalid render-backend values provide a deterministic GUI-route diagnostic

## Current Verification Baseline
- DPT1 workspace-linked gates passed on 2026-07-20:
  - clean Clang build
  - `test-suite TEST_SUITE=indexed-tileset`
  - full `test`
  - `run-headless`
  - shared `core_authored_texture` tests
- The current `fisics-run-headless` gate is blocked before DPT1 compilation
  because the installed fisiCs binary rejects the Makefile's existing `-Wall`
  option. Default/package gates are separately blocked by vendored
  `core_authored_texture` 0.1.3.
- Recent bounded lanes have passed:
  - `make -C /Users/calebsv/Desktop/CodeWork/drawing_program`
  - `make -C /Users/calebsv/Desktop/CodeWork/drawing_program test`
  - `make -C /Users/calebsv/Desktop/CodeWork/drawing_program test-list`
  - `make -C /Users/calebsv/Desktop/CodeWork/drawing_program test-suite TEST_SUITE=texture-import`
  - `make -C /Users/calebsv/Desktop/CodeWork/drawing_program test-suite TEST_SUITE=texture-export`
  - `make -C /Users/calebsv/Desktop/CodeWork/drawing_program headless-probe-matrix`
  - `make -C /Users/calebsv/Desktop/CodeWork/drawing_program run-headless`
  - `make -C /Users/calebsv/Desktop/CodeWork/drawing_program fisics-run-headless`
  - `make -C /Users/calebsv/Desktop/CodeWork/drawing_program package-desktop-self-test`
  - `make -C /Users/calebsv/Desktop/CodeWork/drawing_program package-desktop-refresh`

## Next Boundary
- First commit and roll the already-developed shared `core_authored_texture`
  0.2.0 through the managed Drawing Program subtree, then rerun default,
  package, and installed-app proof. DPT2 UI work starts only after that gate.
- The R0-R6 refinement sequence is complete and the post-R0-R6 renderer/cache
  decision checkpoint is closed. The program is not currently in an active named
  refinement slice.
- The current renderer/cache decision is to stop at the bounded medium-risk
  state. Do not open a fresh renderer/cache follow-on unless broader mixed
  partial-opacity scenes produce new measured evidence that justifies a new
  bounded optimization lane.
- The R0 build-target reconciliation is complete: the fisiCs helper targets are
  part of this R0 lane and `fisics-run-headless` now passes with an explicit
  `FISICS_MAX_PROCS` build contract.
- R4 security refinement is complete and archived:
  - file root containment and path policy
  - texture scene import resource boundaries
  - export/debug artifact privacy
  - package/release command and secret handling
- `R4-S4` package/release command and secret handling is complete:
  `release-sign` no longer echoes the configured signing identity,
  `release-contract` runs a non-live `release-secret-audit` guard, and live
  notarize/artifact/distribute actions remain approval-gated.
- R5 testability refinement is complete and archived. It added lifecycle suite
  selection, artifact-rooted import/export fixtures, artifact-rooted snapshot
  layer fixtures, and the `headless-probe-matrix` target.
- `R5-S1` lifecycle test selection and fixture handling is complete:
  lifecycle tests now support `--list-suites` and `--suite <name>`, Make
  exposes `test-list` and `test-suite TEST_SUITE=<name>`, and new test
  fixtures can use `DRAWING_PROGRAM_TEST_ARTIFACT_ROOT` or the default
  `TMPDIR/drawing_program_lifecycle_tests` artifact root.
- `R5-S2` import/export fixture probes are complete: the `texture-import` and
  `texture-export` lifecycle selectors now construct scene fixtures, browser
  roots, runtime/input/output roots, packs, manifests, PNG paths, and CLI argv
  paths under the lifecycle artifact root contract.
- `R5-S3` snapshot/session fixture root is complete: the `snapshot-layer`
  lifecycle selector now constructs the layer roundtrip and legacy roundtrip
  pack fixtures under the lifecycle artifact root contract.
- `R5-S4` headless probe matrix is complete: `headless-probe-matrix` now
  creates build-local preset/JSON artifacts, then exercises snapshot debug JSON
  export, workspace preset bridge check, and workspace preset bridge import
  without depending on private `data/last_session.pack` state.
- R6 demo proof is active. The strongest current proof routes are:
  `visual-artifact`, `run-headless`, `fisics-run-headless`,
  `headless-probe-matrix`, and `package-desktop-self-test`.
- `R6-S1` source-run visual artifact proof is complete:
  - `make -C /Users/calebsv/Desktop/CodeWork/drawing_program visual-artifact`
    launches the source binary in one-shot visual proof mode
  - the target writes the ignored BMP artifact at
    `visual_artifacts/sketch_first_frame.bmp`
  - the artifact route uses `SDL_VIDEODRIVER=dummy` by default for agent
    environments without a visible display, validates nonblank renderer pixels
    before saving, and prints the final artifact path on success
- Existing non-visual demo proof routes remain:
  `run-headless`, `fisics-run-headless`, `headless-probe-matrix`, and
  `package-desktop-self-test`.
- `visual-harness` remains a build/readiness target; `visual-artifact` is the
  generated first-frame proof route.
- `R6-S2` demo proof documentation is complete:
  - public docs name the source-run proof command, final success line, ignored
    artifact root, default headless SDL behavior, live-display override, and
    troubleshooting expectations
  - packaging docs keep generated R6 proof artifacts out of packaged app
    payloads and keep package validation on `package-desktop-self-test`
- `R6-S3` package proof is complete:
  - package-level visual proof is deferred because the source-run
    `visual-artifact` route already provides the R6 first-frame image proof and
    no concrete package/runtime visual mismatch is currently evidenced
  - `package-desktop-self-test` remains the R6 package proof gate: it rebuilds
    the bundle, runs package smoke checks, and runs launcher `--self-test`
    against the packaged runtime binary
  - `package-visual-artifact` should only be added as a release-gated follow-on
    if package/runtime mismatch, release-candidate visual evidence, or bundle
    resource/sandbox behavior becomes the explicit target
- `R6-S4` R6 closeout and refinement sequence closeout is complete:
  - R6 final proof routes are `visual-artifact`, `run-headless`,
    `fisics-run-headless`, `headless-probe-matrix`, and
    `package-desktop-self-test`
  - the R6 source artifact path is
    `visual_artifacts/sketch_first_frame.bmp`
  - generated visual artifacts remain ignored local review outputs and are not
    packaged app payload
  - the completed R6 plan is archived in the private Drawing Program docs
    bucket
- The latest renderer residual-seams follow-on is complete and archived in:
  - `/Users/calebsv/Desktop/CodeWork/docs/private_program_docs/drawing_program/archive/2026-05-29_completed_renderer_residual_whole_raster_seams_snapshot/2026-05-28_drawing_program_renderer_residual_whole_raster_seams_plan.md`
- If renderer work resumes, it should start as a fresh bounded plan aimed at a
  newly measured mixed-scene CPU compose problem rather than reopening the
  completed whole-raster seam snapshot.
