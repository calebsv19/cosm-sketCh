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
- `make -C drawing_program run-headless`
- `make -C drawing_program memory-check-audit`
- `make -C drawing_program package-desktop-self-test`
