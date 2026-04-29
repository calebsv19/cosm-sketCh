# sketCh Keybind Reference

Last updated: 2026-04-16
Scope: current implemented runtime controls (`sdl-debug` lane)

## Tool Selection
- `B`: brush
- `E`: eraser
- `F`: fill
- `L`: line
- `R`: rect
- `C`: circle
- `S`: select
- `M`: move
- `I`: picker
- `P`: path (pen/polygon draft)
- `B` (`SELECT`, with selected retained `PATH` point): toggle bezier state for that point
- `Shift+L` (`SELECT`, with selected bezier `PATH` point): toggle linked/unlinked handle state for that point

## Color + Theme + Font
- `1..8`: set active palette swatch
- `[` / `]`: previous/next active palette swatch
- `Cmd/Ctrl+Shift+T`: next theme preset
- `Cmd/Ctrl+Shift+Y`: previous theme preset
- `Cmd/Ctrl +`: increase UI font zoom step
- `Cmd/Ctrl -`: decrease UI font zoom step
- `Cmd/Ctrl 0`: reset UI font zoom step
- `Cmd/Ctrl + Shift + 0`: fit canvas to viewport

## Selection + Clipboard
- `Cmd/Ctrl+A`: select all non-background content
- `Cmd/Ctrl+D`: clear selection
- `Cmd/Ctrl+C`: copy active selection payload
- `Cmd/Ctrl+X`: cut active selection payload
- `Cmd/Ctrl+V`: paste clipboard payload

## Layer Controls
- `V`: toggle active layer visibility
- `K`: toggle active layer lock
- `Cmd/Ctrl+Shift+N`: add layer
- `Cmd/Ctrl+[` : select previous layer
- `Cmd/Ctrl+]` : select next layer

## History
- `Z`: undo
- `Shift+Z`: redo
- `Y`: redo
- `Cmd/Ctrl+K`: clear history stack

## Canvas + Viewport
- `Left drag` (`BRUSH` / `ERASER`): draw stroke
- `Left click` (`FILL`): flood fill
- `Left drag` (`LINE` / `RECT` / `CIRCLE`): preview then commit on release
- `Left click` (`PATH`): insert on selected retained-path edge, otherwise add draft point
- `Enter` (`PATH`): commit closed path object (requires 3+ points)
- `Shift+Enter` (`PATH`): commit open path object (requires 2+ points)
- `Backspace/Delete` (`PATH`): remove last draft point
- `Esc` (`PATH`): cancel draft path
- `Left drag` (`SELECT`): marquee select
- `Shift + Left drag` (`SELECT`): add marquee payload to current selection
- `Alt/Option + Left drag` (`SELECT`): subtract marquee payload from current selection
- `Left drag` (`MOVE`, with active selection): move selection
- `Left drag` (`MOVE`, on selected bezier PATH handle): adjust the selected path point handle
- `Alt/Option + Left drag` (`MOVE`, with object selection): point-only drag intent (no object-move fallback on miss)
- `Arrow` (`MOVE`, with active selection): nudge by 1 sample
- `Shift+Arrow` (`MOVE`, with active selection): nudge by 10 samples
- `Right drag` over canvas: pan viewport
- `Mouse wheel` over canvas: cursor-anchor zoom viewport

## Diagnostics / Debug Harness
- `Cmd/Ctrl+Alt/Option+Shift+1..4`: center-pane debug module swap probe
- `Space`: stamp center sample (workflow seed action)

## Notes
- Layer visibility/lock currently gates mutation routes for the active layer.
- Composited canvas output resolves visible layers in stack order; background/eraser samples are treated as transparent-through for v1 layering.
- In `MOVE` mode, drag-start hit detection is payload-mask aware; transparent holes inside selection bounds do not start move drag unless a move handle is used.
- In `MOVE` mode, drag/nudge offsets are clamped to canvas bounds so committed selection payload remains fully in-raster.
- Marquee intent modifiers (`Shift` add, `Alt/Option` subtract) are captured at marquee start and held through mouse-up for deterministic commit mode.
- Live marquee drag visualization is border-only (no interior tint fill).
- `CLEAR CANVAS` is available as a right-panel `CANVAS` action button (no keybind assigned yet).
- `DELETE SELECTION` is available as a right-panel `CANVAS` action button (no keybind assigned yet).
- `DELETE SELECTED` is available as a right-panel `LAYER` action button (no keybind assigned yet).
- Selected `PATH` objects render point handles; dragging a selected handle in `MOVE` commits one history-backed point edit on release.
- In `MOVE`, selected/hovered PATH point handles render with stronger high-contrast markers and zoom-adaptive hit pick radius.
- In `SELECT`, clicking a point handle on an already-selected PATH object targets that specific point for follow-up edit/delete.
- `Delete/Backspace` removes a selected PATH point before falling back to whole-object delete; if too few points remain, the PATH object is removed.
- In `SELECT`, `B` on a selected retained `PATH` point toggles bezier state for that point; without a selected path point, `B` keeps its normal brush-tool behavior.
- In `SELECT`, `Shift+L` on a selected retained bezier `PATH` point toggles `handle_linked`; without a selected bezier path point, `L`/`Shift+L` keep the normal line-tool route.
- In `MOVE`, a selected bezier-active `PATH` point now renders direct handle nodes; dragging a handle commits one history-backed handle edit on release.
- In the `OBJECTS` inspector, `SET STROKE COLOR` / `SET FILL COLOR` arm a retained-object color-target mode that uses the existing palette.
- While object color-target mode is armed, the next palette click applies that color to the selected retained object.
- `Enter` or `Esc` cancels active object color-target mode.
