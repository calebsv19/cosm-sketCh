# sketCh UI contract

This reference describes the retained Main Edit pilot; it is not a production
release or canonical adoption claim. App version remains 0.3.0.

## Ownership and shared baseline

Drawing, indexed palette meaning, history, persistence, exports, native file
panels, pane topology and module policy remain app-owned. The managed subtree
imports immutable shared source 7b37ad8c6ca7eafa679ab820aab9cac033d46c11:
kit_ui 0.18.0, kit_pane 0.5.0, kit_render 0.14.6,
kit_workspace_authoring 0.6.1, core_layout 0.2.1, vk_renderer 1.6.0 and
vk_runtime 0.6.0. The exact-source verifier checks the linked UI/window/pane and
authored-texture modules. core_memdb and core_scene_compile are present in the
subtree but unlinked.

## Controls and text

Rounded buttons use the shared compact appearance and state resolver. Labels
use actual SDL_ttf font height and width, with bitmap fallback when fonts are
unavailable. The app adapter intersects the requested clip with its parent and
restores SDL clip, color and blend state. Text is drawn synchronously.

Named operation tags and domain IDs identify controls independently of their
label and visible row. Layer/object/canvas rows use their model IDs; file and
scene rows use fixed project-slot identity or stable source strings. Reordering
does not turn a pending press into another row. Discrete controls activate on
release inside their visible bound. Tab/Shift+Tab traverse eligible controls;
Space/Enter activate the focused control on release. Window changes and
geometry invalidation cancel stale presses. Model actions still recheck their
existing eligibility predicates. Continuous painting, pan, color sampling,
sliders and selection keep their existing product input paths. The indexed
profile's unavailable Layer tab is disabled in drawing, focus and pointer input.

Workspace authoring is one modal scope. Background controls cannot receive
activation during authoring. Escape cancels the draft; FONT changes the overlay;
Apply accepts it. Shared font/theme action IDs distinguish equal labels in
different sections. Valid background button focus returns after cancellation.
No inline editable text fields exist in the current visual source; bounded
editing, preedit and mixed field/button traversal therefore have no eligible
product target in this pilot. Native file panels retain platform text behavior.

## Panes and windows

Existing core_pane leaf IDs and solved rectangles feed one shared composition
snapshot for input ownership and nested drawing clips. The app's pane geometry
adapter supplies module-specific header height; kit_pane partitions shell,
header and content. The top-level menu keeps its existing chrome without an
additional header. Side-panel and viewport titles are painted by one header
adapter. Content callbacks, hit classification, view fitting and world/input
projection use the same inward-rounded content bounds. Side-panel layout
helpers no longer add their own title offset. Canvas projection is centered in
the content area below the header rather than under the old title overlay.

Shared header slots reserve measured title space before actions. The viewport
has FIT; either side panel has LAYOUT, opening the existing workspace authoring
session. Narrow/empty panes omit slots instead of overlapping the title; omitted
or hidden actions have no input target. Disabled actions cannot activate. Action
keys combine operation and stable pane ID, and the app checks the current module
policy before dispatch. Headers dispatch directly to existing domain actions;
older content controls still use their bounded app-handler bridge. Real pointer
events reach pane ownership before semantic control routing, so a synthesized
content action cannot become a second press or leave capture stuck after release.

The three header policies are a fixed app projection. No new pane topology,
dynamic provider registration, docking or provider persistence is introduced.

Ordinary splitter resizing is a quiet runtime interaction. Its internal layout
transaction can borrow authoring revision machinery without opening the explicit
authoring session, HUD, pane IDs or all-pane outlines. Those appear only after
LAYOUT or the established authoring entry chord. Explicit authoring takeover
cancels an unfinished runtime resize before taking its baseline. Closing an outer
authoring session also cancels any unfinished nested splitter edit.

Divider hit bands are 16 logical window pixels wide, centered on the edge (8 on
either side); paint stays a thin 2-render-pixel hover/drag highlight. The host
applies the logical-to-bounded-render scale independently on each axis, including
Retina and large drawables. Divider presses take priority over nearby content
buttons within this band. Explicit FIT/LAYOUT slots retain ownership inside their
visible click bounds so the generous divider band cannot swallow a header action.
Movement outside the band retains normal content interaction.

Splitter motion is a layout transaction. Cancel restores node payload and
revision state, including nested authoring state. A no-op drag creates no new
revision; an accepted runtime drag commits once. Escape and all ten shared
window invalidation classes cancel unfinished drags and drawing transients.

F11 toggles desktop fullscreen. Window observation continues while idle. Hidden
or minimized windows do not submit frames and resume on restoration. Native
drawable metrics remain separate from the Vulkan compatibility canvas; a
drawable above 4096 is uniformly scaled into the bounded canvas, presented over
the full native extent and mapped once for input. Pipeline recovery uses the
resolved source/package shader directory, including automatic recovery.

## Verification

`make test-suite TEST_SUITE=ui-contract` exercises production adapters, button
centering/state restoration, release ownership, modal exclusion/restoration,
window invalidation, splitter transactions and pane capture. `make test` keeps
existing drawing/history/indexed/persistence/export/authoring regressions.
`make vulkan-rollout-self-test` checks shared identity and native presentation.
`make test-suite TEST_SUITE=pane-header` exercises the production frame's
header/content isolation, content/input bounds, narrow/disabled/hidden slots,
semantic dispatch, FIT parity and authoring cancellation without a native window.

Optional qualification environments run finite probes in the actual event loop:
`CODEWORK_WINDOW_LIFECYCLE_PROOF=<existing directory>` covers resize, fullscreen,
hide/minimize and restore; `CODEWORK_WINDOW_PROOF_LARGE=1` adds a Retina extent
above the canvas bound. `DRAWING_PROGRAM_UI_PROOF=<existing directory>` covers
the six inspector regions (Layer disabled for indexed mode), keyboard release
and authoring scopes. These are engineering probes, inactive in routine use.
Always supply isolated runtime/input/output roots and `--no-persist`. Native
macOS proof does not establish human IME, external monitor/DPI, exclusive
fullscreen, device-loss, Linux or Windows acceptance.

`DRAWING_PROGRAM_PANE_HEADER_PROOF=<existing directory>` runs an independent,
finite actual-loop header qualification: pan the content, reject FIT on release
outside, apply FIT on accepted release, reach LAYOUT with Tab/Space, cancel the
modal and verify focus plus layout restoration. It records four fresh captures
and is inactive in routine use. Run this and other probes separately, checking
completion, exit status and capture existence; do not combine their drivers.

`DRAWING_PROGRAM_SPLITTER_PROOF=<existing directory>` independently qualifies
the actual loop: hit the wider band over an ordinary content tab, drag without the
authoring HUD, commit one revision, cancel another drag with Escape, then enter
and cancel explicit workspace authoring. It records six captures. Use the same
isolated no-persist roots and run each engineering probe separately.
