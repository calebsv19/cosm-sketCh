# Architecture

Last updated: 2026-07-21

`drawing_program` is organized as a retained-object drawing editor,
texture-project host, and atlas-style multi-surface workspace shell. The app is
past scaffold bootstrap: current architecture is split by domain ownership,
runtime orchestration, input routing, render/cache ownership, UI chrome, and
session/export IO.

## Current Source Layout

- `src/app/`: entry points, lifecycle wiring, app runtime setup, visual runtime
  loop policy, session startup, and top-level app orchestration glue.
- `src/domain/`: document/editor/history/object/selection/color/texture
  project state and pure behavior contracts.
- `src/io/`: project/session persistence, snapshot pack/debug JSON bridges,
  texture scene import, native dialogs, and image/texture/icon export.
- `src/input/`: normalized input routing, panel/canvas/layer/tool handlers,
  workspace browser/surface selection, and interaction policy glue.
- `src/render/`: canvas/world rendering, authoring chrome, frame render passes,
  and object/selection overlay rendering.
- `src/runtime/`: backend-facing render/cache contracts, composed-source
  ownership, canvas stroke/reflection runtime support, pane/overlay adapters,
  and orchestration seams.
- `src/ui/`: layout, panel/right-panel rendering, theme/resources/text helpers,
  layer opacity UI, pane bindings, and tool-option presentation.
- `tests/`: lifecycle and contract suites covering persistence, render/cache,
  runtime paths, object/path behavior, selection/layers, export, texture
  import/export/workspace behavior, and authoring-host seams.

## Ownership Boundaries

- Document and editor state live in the domain lane; app/runtime code should
  coordinate state rather than own drawing semantics directly.
- Object, selection, history, layer-raster, texture-project, reflection, and
  color behavior are app-local unless a later pass proves a stable shared
  contract.
- Snapshot and session persistence live under `src/io/session/`; export-specific
  behavior lives under `src/io/export/`.
- Indexed tileset state is deliberately split by role:
  - `src/model/indexed_tileset/` owns exact indices, palette profile, named-cell
    tables, and typed raster/cell history
  - `src/io/session/drawing_program_indexed_project_snapshot.c` owns `DPIP`,
    `DPIL`, and optional `DPIC` pack chunks
  - `src/io/export/indexed_tileset/` owns indexed PNG encode/decode,
    manifest/palette/report emission, reopen comparison, and destination commit
  - Dungeon owns its nine-slot policy, stable tile-key vocabulary, canonical
    source descriptor, generated resources, and consumer decoder
- Input handlers should normalize and route intent, while domain/runtime owners
  perform the actual semantic mutation.
- Renderer/cache behavior is split between domain render facts, runtime
  composed-source/cache contracts, UI surface-cache resources, and render pass
  presentation.
- UI layout/panel code owns screen presentation and control geometry; it should
  not become the authority for document or texture-project state.

## Shared Dependencies

The app intentionally reuses shared CodeWork libraries where their contracts are
already stable:

- `core_base`, `core_units`, `core_object`, `core_scene`, `core_pack`
- `core_theme`, `core_font`, `core_pane`, `core_layout`,
  `core_pane_module`, `core_viewport2d`
- `core_authored_texture`
- `kit_render`, `kit_pane`, `kit_ui`, `kit_workspace_authoring`

Shared extraction remains conservative. Drawing-specific behavior stays
app-local until a later pass proves that a cross-app contract is stable enough
to move into `shared/`.

## Current Structural State

- Workspace Authoring `WA1` first-host attach is complete and is treated as a
  host baseline, not an active attach lane.
- The authored-texture roundtrip, reflection roadmap, renderer compose hot-path
  lane, renderer residual-seams lane, and large-file decomposition lane are
  complete and archived in private program docs.
- No first-party `src` or `include` file is above the current `1000` LOC
  structure threshold as of the 2026-06-24 R0 audit.
- Nearest source-shape watch files are:
  - `src/domain/drawing_program_texture_workspace.c`
  - `src/domain/drawing_program_texture_project.c`
  - `src/io/session/drawing_program_texture_project_snapshot_load.c`
  - `src/domain/drawing_program_object_store.c`

## Current Risk Areas

- Renderer/cache risk is now bounded to broader mixed partial-opacity CPU
  compose cost, not the closed whole-raster seam contracts.
- Persistence/input/workspace coverage remains broad, so changes in those lanes
  should keep using focused lifecycle tests plus headless smoke verification.
- Build/source-list ownership is manual enough that build-target changes should
  be reconciled deliberately before they are treated as public verification
  contracts.
- Indexed export intentionally remains separate from both the standard RGBA PNG
  path and schema-v5 face-oriented authored-texture export. Merging those
  output families would weaken exact-index guarantees.

## Contract References

- scaffold lifecycle v1
- IR1 input routing
- app packaging standard
- orChestra overlay adapter contract
- shared authored-texture vocabulary
- workspace-authoring adapter contract
