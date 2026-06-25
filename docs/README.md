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
- `make -C drawing_program package-desktop-self-test`

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
