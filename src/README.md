# drawing_program src layout

This program started with minimal `app` + `runtime` lanes for scaffold bootstrap.
As behavior depth grows, new code should be placed in focused lanes:

- `app/`: entry points, lifecycle wiring, top-level orchestration glue.
- `runtime/`: existing scaffold/runtime contracts and adapters (legacy baseline lane).
- `ui/`: pane chrome/components, theme/font-aware layout/render helpers.
- `input/`: intake/normalize/route/invalidate flow and interaction policies.
- `render/`: world/canvas render passes, projection glue, backend-facing render units.
- `domain/`: document/editor/history/tool domain behavior and command semantics.
- `io/`: snapshot/preset/runtime-root persistence and import/export bridges.

Migration rule:
- keep existing runtime behavior stable;
- move code from `runtime/` into focused lanes incrementally by slice,
  with build/test/smoke/package gates after each slice.
