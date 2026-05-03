# Architecture

Last updated: 2026-05-03

Current state:
- scaffold shell lane with lifecycle skeleton implemented
- canonical stage order implemented and guarded in app main path
- IR1 input routing pipeline implemented as explicit phases with typed contracts and counters
- pane host scaffold uses shared `core_pane` + `core_layout` + `core_pane_module`
- authoring host adapter now uses `kit_workspace_authoring` for the first WA1 entry/exit seam while keeping drawing-specific input behavior app-local
- authoring draft transaction state is host-owned: entry captures pane/module baselines, Apply validates/rebuilds before `core_layout_apply_authoring(...)`, and Cancel restores the entry baseline
- authoring draft transaction state also captures UI theme preset, font preset, and font zoom baselines so top-level authoring appearance edits stay accepted-only
- authoring chrome rendering is isolated in a frame-render adjunct that derives pane/module readout from the existing pane host registry and bindings and now also owns the font/theme takeover overlay
- overlay adapter seam exists with typed lifecycle/ownership hooks and dirty-exit guard policy
- document, editor/session, and command/history foundations are now explicit runtime modules
- snapshot persistence module now provides pack save/load and debug JSON export seams
- viewport/navigation transforms and render-domain projection baseline are implemented
- true-layer compositing foundation lanes are now explicit:
  - runtime layer-raster ownership (`drawing_program_layer_raster.*`)
  - runtime selection ownership (`drawing_program_selection.*`)
- remaining follow-on depth:
  - tool-domain behavior depth

Target subsystem boundaries:
- document domain
- editor/session domain
- tool domain
- command/history domain
- viewport/navigation domain
- render/cache domain
- export/output domain
- workspace-authoring adapter domain

Contract references:
- scaffold lifecycle v1
- IR1 input routing
- app packaging standard
- orChestra overlay adapter contract (Phase 6)
