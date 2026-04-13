# sketCh Keybind Reference

Last updated: 2026-04-12
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

## Color + Theme + Font
- `1..8`: set active palette swatch
- `[` / `]`: previous/next active palette swatch
- `Cmd/Ctrl+Shift+T`: next theme preset
- `Cmd/Ctrl+Shift+Y`: previous theme preset
- `Cmd/Ctrl +`: increase UI font zoom step
- `Cmd/Ctrl -`: decrease UI font zoom step
- `Cmd/Ctrl 0`: reset UI font zoom step

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
- `Left drag` (`SELECT`): marquee select
- `Shift + Left drag` (`SELECT`): add marquee payload to current selection
- `Alt/Option + Left drag` (`SELECT`): subtract marquee payload from current selection
- `Left drag` (`MOVE`, with active selection): move selection
- `Arrow` (`MOVE`, with active selection): nudge by 1 sample
- `Shift+Arrow` (`MOVE`, with active selection): nudge by 10 samples
- `Right drag` over canvas: pan viewport
- `Mouse wheel` over canvas: zoom viewport

## Diagnostics / Debug Harness
- `Cmd/Ctrl+Shift+1..4`: center-pane debug module swap probe
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
