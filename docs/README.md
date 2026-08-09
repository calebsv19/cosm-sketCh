# drawing_program Docs

- `current_truth.md`: what is implemented right now
- `desktop_packaging.md`: packaged desktop app contract and validation flow
- `memory_check_audit.md`: default-off fisiCs memory-check audit command and
  latest clean headless smoke result
- `future_intent.md`: near-term roadmap and planned slices
- `architecture.md`: subsystem boundaries and ownership
- `keybind_reference.md`: canonical runtime control map

## Verification Entry Points

- `make -C drawing_program test`
- `make -C drawing_program test-list`
- `make -C drawing_program test-suite TEST_SUITE=<name>`
- `make -C drawing_program visual-artifact`
- `make -C drawing_program headless-probe-matrix`
- `make -C drawing_program run-headless`
- `make -C drawing_program fisics-run-headless`
- `make -C drawing_program memory-check-audit`
- `make -C drawing_program vulkan-rollout-contract`
- `make -C drawing_program vulkan-rollout-self-test`
- `make -C drawing_program package-desktop-self-test`

`vulkan-rollout-self-test` binds the vendored `vk_runtime` and `vk_renderer`
files to canonical shared commit
`cc340d78a3cea80b1086fc5e434ccbaf1118c34c`, then proves validation-clean
startup, native readback/capture, resize recreation, 2x Retina scaling,
shutdown/restart, and a real Drawing Program frame. It does not claim Vulkan
compute adoption.

## Indexed Tileset Roundtrip

`INDEXED_ATLAS_V1` projects store exact index bytes, stable named cells, and
fixed logical cell geometry. The `ASSET` tab selects and mutates cells; cell
frames/names are canvas overlays only. The `EXPORT` tab's texture action becomes
`EXPORT TILESET` for indexed projects and writes a separate transactional
artifact family: `<tileset>_indices.png`, `tileset_manifest.json`, palette JSON
and preview PNG, and `export_validation.json`. This path does not call the
standard RGBA PNG exporter or schema-v5 authored-texture exporter.

Dungeon canonical source packs are intentionally texture-project-only: they
contain indexed project chunks, not a saved Drawing Program window shell. Use
`OPEN PROJECT` to select a `.pack`; Drawing Program keeps its seeded shell and
activates the pack's selected surface. For `cobble_master_v1.pack`, the canvas
opens as the 80x64 logical atlas with its 20 stable 16x16 named cells and exact
nine-slot palette. Saving a working session writes a normal Drawing Program
snapshot separately; it does not rewrite the selected canonical source pack.
Indexed source packs automatically frame the atlas in the canvas pane after
startup or `OPEN PROJECT`, so their pixels are immediately large enough to
paint. Select a palette slot in the `COLOR` tab, then use `BRUSH`, `ERASER`, or
`FILL` on the canvas; use `ASSET` to select the named cell you are editing.
The canvas wheel now supports up to 64x zoom (48 screen pixels per indexed
source pixel). Selecting a named row in `ASSET` centers and frames that 16x16
cell for close inspection; right-drag pans between cells, and the wheel keeps
the pixel beneath the pointer anchored while changing zoom.

## Visual Artifact Proof

Run the source-render first-frame proof with:

```sh
make -C drawing_program visual-artifact
```

The target launches the source binary in one-shot visual proof mode and writes:

```text
visual_artifacts/sketch_first_frame.bmp
```

Expected final success line:

```text
Drawing visual artifact ready: visual_artifacts/sketch_first_frame.bmp
```

`visual_artifacts/` is a program-local ignored artifact root. The target uses
`SDL_VIDEODRIVER=dummy` by default so agent and headless desktop sessions can
produce the BMP without a visible display. Override
`VISUAL_ARTIFACT_SDL_VIDEODRIVER=<driver>` only when intentionally proving a
live display route.

Keep `visual-harness` and `visual-artifact` distinct:

- `visual-harness`: build/readiness setup for manual visual validation.
- `visual-artifact`: generated first-frame image proof.

Troubleshooting:

- If a live SDL driver cannot initialize, rerun the default
  `visual-artifact` target or set `VISUAL_ARTIFACT_SDL_VIDEODRIVER=dummy`.
- If capture reports a blank frame, treat it as a renderer/projection
  regression; the target exits nonzero instead of writing ambiguous proof.
- If the output path is missing or empty, the target failed and should not be
  counted as R6 proof.
