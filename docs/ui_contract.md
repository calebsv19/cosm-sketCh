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
policy before dispatch. Headers and registered content controls dispatch semantic
commands directly. Real pointer events reach pane ownership before control routing;
activations never re-enter pointer intake as fabricated presses.

The three header policies are a fixed app projection. No new pane topology,
dynamic provider registration or docking is introduced. Fixed-module lifecycle and accepted
layout persistence are described below.

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

## Fixed-module lifecycle and accepted persistence

`core_pane_module` validates stable module/instance/leaf bindings. The app's
`drawing_program_pane_lifecycle.c` prepares the complete candidate before using
`kit_pane_host` mount/unmount/resize/focus/cancel notifications. Changing an
instance, module or configuration at the same pane ID cancels capture, blurs and
unmounts the old controller before mounting its replacement. Hidden, disabled,
empty or removed panes have no controller, paint callback or input target.
The app-owned `DRAWING_PROGRAM_PANE_HIDDEN` binding flag persists visibility;
this adds no new hide/docking UI. Modal takeover cancels input while retaining
controllers. Unchanged frames and geometry-only resizing preserve controller
identity. Failed graph/binding candidates preserve the previous valid geometry
and live controllers. Explicit authoring Cancel restores its entry bindings/layout.

Controllers borrow document/model storage; renderer-owned font/texture caches
are shared by the existing panes. Layout preview, hiding and Cancel retain those
resources. Renderer shutdown releases them through existing owners. Before a
successful document replacement, a host hook cancels drawing transients against
the old document and drops pending surface-cache source pointers before freeing
old storage. Retained textures are then reconciled by the normal project epoch
and content-signature logic. A failed staged read invokes no replacement hook.

Snapshot loads stage a candidate document, layer store, texture project, UI and
pane state separately. Failed reads discard the candidate; accepted live state
and resources remain intact. Successful reads publish validated storage, retain
the live pane host/observer, clear unfinished gestures and return to runtime mode.
Valid accepted bindings/configurations survive reopening; only legacy shells
with no bindings are repaired to defaults. Saving during an explicit authoring
draft exports its entry baseline. Saving during a quiet runtime resize exports
the splitter's pre-drag nodes/revision without finishing that gesture. Accepted
changed drags still commit exactly one revision.

DPS3 shell payload version 3 uses the former reserved header word for root index;
version 2 remains readable with its historical root-zero meaning. The outer
core_pack format and legacy fallback chunks remain unchanged. New shared APIs,
module minimums and module VERSION transitions were unnecessary for this slice.

`make test-suite TEST_SUITE=pane-lifecycle` covers standard/indexed mount/remount,
hidden input exclusion, failure/Cancel, unfinished/accepted saves, module and
visibility persistence, nonzero roots and staged late-load failure. The
`surface-cache` suite verifies pending cancellation retains textures and permits
fresh enqueue. Optional `DRAWING_PROGRAM_PANE_LIFECYCLE_PROOF=<existing directory>`
runs six captured actual-loop stages: initial, draft, Cancel, hidden, restored and
accepted reopen. It checks standard RGBA surface textures and indexed raster
storage according to their real owner; indexed mode keeps its normal view.
Run independently of other probes with isolated runtime/input/output roots and
`--no-persist`. Qualification probes are inactive in routine operation.

The fixed-pane macOS UI adoption baseline is the bounded completion target.
Generalized docking/plugins, GPU-native canvas composition, other-platform/monitor/IME
qualification and canonical/release adoption remain separately scoped follow-ons.
The registered content-control coordinate bridge has been replaced as described below.

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

## Direct content commands — 2026-10-06

The shared `KitUiSurfaceKey` is the app input command: operation plus domain
identity. `drawing_program_ui_controls_route` returns that key and has no action
coordinate outputs. The app loop dispatches headers, authoring/font-theme controls
and both inspector panes by meaning, then consumes the activation. It never
constructs an SDL mouse event for a command. Each activation can be claimed once;
invalidation and scope takeover discard pending commands. Modal pointer presses
are blocked until the correct scope is collected, without a press-time chrome
action fallback.

`src/input/panel/drawing_program_ui_commands.c` validates the collected enabled
key, current modal/composition state and available pane content. Explicit typed
intents reach the existing product handlers; their operation/identity comparisons
select the authoritative action without spatial hit tests. Shared interaction,
focus and clipping remain in kit_ui/kit_pane. Product eligibility, history, file
panels, import/export and target resolution remain app-owned. No shared API or
module version changes are required.

Object rows and inspector actions carry object IDs. Layer/canvas rows carry their
existing IDs; indexed cells and scene entries resolve current stable domain string
identities rather than visible row offsets. Project-slot numbers retain their
existing stable slot meaning. Removed or changed targets cannot select the row
that replaced them. Commands named for the active target retain that product
meaning and recheck the current tab/profile/domain predicates.

The action bodies are shared between semantic and retained spatial entry points.
Spatial entry points remain necessary for color surfaces, swatches, opacity, wheel
scrolling and drawing gestures, and keep the existing domain/pointer regressions.
These APIs do not justify converting a command into a coordinate. Layout helpers
may still calculate content geometry within the common product handler; semantic
action selection does not use it. This slice does not introduce a generic command
registry, widget tree or new docking/provider model.

`make test-suite TEST_SUITE=ui-command` links production actions and checks pointer
and keyboard commands, disabled/replayed/invalidated activations, modal takeover,
hidden panes, standard/indexed eligibility, object target replacement and indexed
row reordering. Its spatial-hit hook deliberately fails if any semantic command
reaches a legacy hit test. Aggregate domain/history/persistence/export tests and
real-loop UI/native/package checks complete the qualification boundary; native
file-panel acceptance and every destructive action are not automated.
