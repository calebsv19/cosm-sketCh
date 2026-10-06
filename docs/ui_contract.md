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
snapshot for input ownership and nested drawing clips. No new pane layout,
docking or provider persistence is introduced. Existing content titles remain
app-owned; shared header-action slots are reserved for a future header/content
separation rather than changing the drawing workspace in this adoption.

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

Optional qualification environments run finite probes in the actual event loop:
`CODEWORK_WINDOW_LIFECYCLE_PROOF=<existing directory>` covers resize, fullscreen,
hide/minimize and restore; `CODEWORK_WINDOW_PROOF_LARGE=1` adds a Retina extent
above the canvas bound. `DRAWING_PROGRAM_UI_PROOF=<existing directory>` covers
the six inspector regions (Layer disabled for indexed mode), keyboard release
and authoring scopes. These are engineering probes, inactive in routine use.
Always supply isolated runtime/input/output roots and `--no-persist`. Native
macOS proof does not establish human IME, external monitor/DPI, exclusive
fullscreen, device-loss, Linux or Windows acceptance.
