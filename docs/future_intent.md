# Future Intent

Last updated: 2026-06-25

## Near-term
- The R0-R6 refinement sequence is complete at this checkpoint:
  - R0 source-shape and build-target reconciliation are complete
  - R1 duplication/ownership slices are complete and archived
  - R2 app-state ownership slices are complete and archived
  - R3 error/logging slices are complete and archived
  - R4 security slices are complete and archived; `R4-S1` file root containment/path policy, `R4-S2`
    texture scene import resource boundaries, `R4-S3` export/debug artifact
    privacy, and `R4-S4` package/release command secret handling are complete
  - R5 testability slices are complete and archived
  - R6 demo proof slices are complete and archived
- The authored-texture roundtrip/hardening lane is complete and archived in:
  - `/Users/calebsv/Desktop/CodeWork/docs/private_program_docs/drawing_program/archive/2026-05-13_completed_authored_texture_roundtrip_snapshot/`
- The reflection roadmap is complete and archived in:
  - `/Users/calebsv/Desktop/CodeWork/docs/private_program_docs/drawing_program/archive/2026-05-26_completed_reflection_v1_v2_snapshot/2026-05-10_drawing_program_reflection_v1_v2_plan.md`
- The first bounded renderer/compose hot-path stabilization lane is complete and
  archived in:
  - `/Users/calebsv/Desktop/CodeWork/docs/private_program_docs/drawing_program/archive/2026-05-28_completed_renderer_compose_hot_path_snapshot/2026-05-26_drawing_program_renderer_compose_hot_path_plan.md`
- The bounded renderer residual-seams follow-on is now complete and archived
  in:
  - `/Users/calebsv/Desktop/CodeWork/docs/private_program_docs/drawing_program/archive/2026-05-29_completed_renderer_residual_whole_raster_seams_snapshot/2026-05-28_drawing_program_renderer_residual_whole_raster_seams_plan.md`

## Current Follow-on Rule
- Reflection should remain closed unless a fresh bounded plan is opened.
- Vulkan or another new live backend should remain deferred unless the current
  renderer/compose state proves unacceptable even after bounded SDL/cache work.
- Any new authored-texture or mixed-material semantics work should start as its
  own bounded roadmap rather than being folded into the renderer lane.
- Any renderer follow-on should remain bounded and target one named residual
  seam at a time rather than reopening broad cache architecture work.

## Expected Next Outcomes
- After the R0-R6 refinement sequence reaches renderer/cache decision points,
  decide whether the current renderer/cache state is good enough to stop at
  this checkpoint or whether a fresh bounded follow-on is justified for broader
  mixed partial-opacity CPU compose cost.
- Keep the same contract style if renderer work resumes: frozen hot paths,
  bounded optimization, and focused cache-drift validation.
