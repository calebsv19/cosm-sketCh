# sketCh indexed tile MCP

The stdio MCP adapter controls an explicitly enabled local GUI endpoint. All
project reads and edits execute on Sketch's application owner thread. There is
no endpoint when `SKETCH_CONTROL_SOCKET` is absent. This initial slice supports
existing named 16×16 indexed tiles, not standard RGBA drawing or Dungeon rendering.

## Setup and launch

Python 3.11+ with the official MCP SDK is required. The lock is the tested
Python 3.14/macOS dependency set (`mcp==1.30.0`). An existing qualified SDK
runtime can be selected with `SKETCH_MCP_PYTHON`; otherwise provision the local
runtime without changing global Python:

```sh
python3 -m venv build/mcp-venv
build/mcp-venv/bin/python -m pip install -r tools/mcp/requirements.lock
```

For this development checkpoint, `build/mcp-venv` contains an independent
copy-on-write clone of the already tested local SDK environment. It is a runtime
input for the MCP adapter and is deliberately retained, not disposable test output.

Create an owner-only endpoint directory and an existing candidate output directory:

```sh
mkdir -p -m 700 /tmp/sketch-control-$UID
mkdir -p /tmp/sketch-candidates-$UID
export SKETCH_CONTROL_SOCKET=/tmp/sketch-control-$UID/control.sock
export SKETCH_CONTROL_OUTPUT_ROOT=/tmp/sketch-candidates-$UID
export SKETCH_CONTROL_ALLOW_WRITE=1
```

Launch `sketCh Main Edit.app/Contents/MacOS/sketch-launcher` from that shell so
it inherits these variables, or run a newly built GUI binary. Open the Dungeon
indexed source `.pack` with OPEN PROJECT. For isolated work, pass `--preset
<working-copy.pack> --no-persist` to the GUI binary. Do not run two applications
against the same endpoint. Paths must be absolute and the socket path shorter
than 104 bytes. Startup refuses existing paths or an unsafe endpoint directory;
normal shutdown removes only its own socket inode. After a crash, confirm the
old process is stopped before removing its stale socket. Omit write enablement
for read-only tools. Writes additionally require an existing output root.

```sh
python3 tools/mcp/client_config.py
```

This prints a stdio client configuration with absolute paths; it does not modify
client settings. Configure/restart the desired MCP client to discover tools.
Building or refreshing Sketch does not inject tools into an already running chat.
The endpoint is same-user access to the enabled operations, not client-specific
approval or isolation; it creates no TCP listener.

## Authoring loop

1. `capabilities` obtains the current `instance` and write admission.
2. `inspect_project` lists stable tile IDs, ordered palette slot IDs, dirty state
   and opaque `expected_state`.
3. `read_tile` returns exact row-major indices and live preview RGBA.
4. `patch_tile` applies up to 256 unique tile-local `(x,y,slot)` changes as one
   history action. `replace_tile` supplies all 256 indices. `fill_tile` uses the
   existing exact-index flood fill within one cell. `copy_tile` copies pixels
   between two existing cells without changing their identities.
5. `preview_tile` returns a nearest-neighbor PNG from actual source pixels.
   It is a source preview, not an editor screenshot or Dungeon capture.
6. `undo`/`redo` operate on the current shared indexed pixel-history cursor,
   including GUI pixel edits. They do not undo cell metadata or skip entries.
7. `save_candidate` saves a new editable `<name>.pack` beneath output root.
   It preserves the selected GUI source/save destination and does not mark that
   source clean. `export_candidate` writes a new validated artifact directory.
   Both refuse existing names. Export `palette` names the current preview mapping;
   it does not select a palette family. Use fresh candidate names for iteration.

Every mutation requires `instance`, a fresh `request_id` (UUID recommended), and
current `expected_state`. Check `ok` and `code`. The token is an opaque state
fingerprint including document generation and history cursors, not a monotonic
revision number or authentication credential. GUI pixel/cell edits and document
replacement invalidate stale state. Mutations reject while drawing/panning,
workspace authoring or composition blocks the owner. An identical no-op adds no
history. Slot indices must fit the actual project palette; no RGB inference,
antialiasing or blended indexed layers are introduced.

After a timeout or disconnect, query `request_status` with the original instance
and request ID. Exact canonical requests replay the original terminal result;
changed payloads using that ID reject. The adapter sorts JSON fields, so replay
is stable through MCP. Raw wire clients must replay the exact request bytes.
The in-memory ledger retains 128 admitted attempts, including refused attempts;
it never evicts outcomes and refuses new IDs when full. Restart changes instance
and loses receipts. Reconcile current project/candidate files after restart;
do not blindly resend an uncertain write. A replayed result is historical, so
read the tile again to obtain current state.

## Verification

Build/test output should be contained: the legacy Make shared-library rules run
recursive clean recipes even with a separate target triple. Use a task-owned
source copy and pass the workspace `TARGET_CONTRACT_HELPER` explicitly; preserve
pre-existing source/build roots.

```sh
build/mcp-venv/bin/python tests/mcp_control_test.py \
  --app <fresh-gui-binary> --source <isolated-indexed-pack> --output <fresh-proof-root>
```

This negotiates the official SDK with the real production GUI owner loop using
SDL's dummy driver and the software backend. It checks atomicity, state/replay,
history, preview, save/reopen and export. It needs local Unix-socket permission.
The test terminates its isolated GUI processes; that is not graceful shutdown
proof. The separate visual-artifact run checks normal endpoint cleanup.
The existing `indexed-tileset` suite also covers atomic tile commands, busy,
document-swap/read-only admission and receipt capacity; full lifecycle tests
cover ordinary standard/indexed behavior. Native Vulkan visual acceptance and
Dungeon authored terrain remain separate proof layers.

## Prepare a compatible sample and review sheet

`prepare_candidate.py` is a repeatable official-SDK client for one bounded cobble
sample. It starts its own isolated installed GUI process with the software backend,
opts in to a private local endpoint, and reads the source pack without persisting to
it. It requires the current 20 cobble keys/order and nine palette roles. A source
hash check confirms the original is unchanged. It does not connect to or edit an
already open GUI session.

```sh
build/mcp-venv/bin/python tools/mcp/prepare_candidate.py \
  --app "/Users/calebsv/Desktop/sketCh Main Edit.app/Contents/MacOS/drawing-program-bin" \
  --source <absolute-compatible-source.pack> \
  --output <new-absolute-handoff-directory>
```

The output directory must be new. The command authors all 20 named tiles through
`replace_tile`, checks each exact result, exercises undo/redo, saves an editable
`cobble_candidate.pack`, and exports `baseline/` and `candidate/` with the current
`crypt_dark` palette. It reopens the candidate in a separate read-only session and
checks every index and preview color. An independent PNG decoder checks chunk CRCs,
exported indices, palette/alpha and unchanged manifest metadata against live reads.

`review/index.html` contains labeled before/after tiles, all wall-edge masks and
repeated floor strips; its lossless PNGs use integer nearest-neighbor scaling.
`tiles.json` records exact baseline/candidate readback. `handoff.json` records source,
installed binary and artifact hashes, changed-pixel counts and verification limits.
The recipe is an intentionally simple sample with a fixed light direction and worn
variant, not a generic recipe system or accepted final art. Review images show
source pixels; they do not simulate Dungeon room selection, lighting or overlays.

If a request fails or times out, the command stops without automatically retrying.
Keep that partial directory for diagnosis; a new isolated run requires a fresh
output directory. Its process is terminated after the session, so this workflow
does not add a graceful-shutdown claim. No Dungeon source, asset destination, save
or client configuration is changed. No Desktop refresh is needed for these Python
workflow modules when the S1-capable app is already installed.

## Rejected sparse detail experiment

The user rejected the sparse-mask/tint compositor. Its editable pack and evidence
are retained under `build/quiet-details-20261009/` for historical review, not as
inputs to the corrected game style. The supported sample preparation remains
`prepare_candidate.py`, which authors full cobble tiles. Dungeon's corrected
prototype preserves that continuous material with restrained neutral values and
retains original theme-specific decorations. Palette families and future theme
object authoring remain explicit next work.

## Three-theme material/decor kit

`prepare_theme_kit.py --app <installed-Sketch-binary> --source <accepted-cobble.pack>
--output <fresh-directory>` authors 18 roles through the existing MCP session.
Run with the official MCP Python SDK available, as for `prepare_candidate.py`.
Three floor variants and three decorations per theme produce cobble rocks,
moss plants/flowers/vines, and magma fragments/flames/vents. Exact readback,
undo/redo, indexed export verification and editable reopen are checked.

Outputs include `theme_kit.pack`, `candidate/`, `readback.json` and `receipt.json`.
`candidate/theme_kit.json` assigns existing carrier cells and separate theme
material/decor ramps; embedded palette remains `crypt_dark`. These role aliases
do not establish general cell creation or palette-family UI. Dungeon layered
mode interprets the metadata and index-zero transparency. Its native seed
matrix, rather than the source export palette preview, establishes game colors.
The accepted source pack is not modified. Retained run:
`build/room-theme-kit-20261009/`. Human review and targeted edit/undo-to-game
reload remain open before broader theme production.

## 2026-10-09 — Restore accepted detail and scale decoration coverage

User rejected the sparse three-theme kit and authorized this correction. The
kit's early return bypassed the accepted procedural floor-detail pass; that pass
is now explicitly restored for cobble, moss and magma. Decoration budgets scale
with eligible room area (small/medium/large) and boundary length (vines), with
room-sized storage rather than a 24-object room cap. Small objects cover the room
in every arrangement; some medium groups cluster or favor edges. Large objects
reserve footprints first. Winding/branched MCP-authored vine stamps attach on
north/east/south/west boundaries; an off-by-one sampling bug blocking east/south
roots is fixed. Magma seam role is subdued red-brown RGB 39,24,25. Arbitrary
wall-contour following and animation are not claimed.

Fresh compile and all 51 contracts pass. Focused tests cover exact restored
material-plus-detail composition, area scaling, all four vine root orientations,
90 deterministic plans, clearances, RNG and reload retention. Revised Sketch MCP
art/export/reopen checks pass. Native final matrix replays all 14 states exactly,
with stable repeat/revisit and a distinct neighbor; validation clean and zero
full-frame uploads. Fixture adds 68 cobble/magma objects and 126–128 moss objects
above the restored distributed material detail. Source/view recipe unchanged
except the authorized corrections. Human visual acceptance remains open.

Current review/launcher: `_worktrees/dungeon_main_edit/build/room-theme-density-final-20261009/`.
Editable kit: `_worktrees/drawing_program_main_edit/build/room-theme-kit-density-20261009/`.
Previous sparse prototype launcher forwards to this correction; historical
images/binaries remain unchanged. Task-owned disposable build and intermediate
matrix retire after consumer checks; compact review and editable pack remain.
No Desktop install, canonical adoption, VERSION or personal-memory write. Minor
release recommended. Existing Atlas parent record updated; full generation
remains blocked by the foreign compute-node inventory snapshot digest.

## Six additional room-theme exports

`prepare_room_theme_extensions.py --app <binary> --source <accepted-cobble.pack>
--output <fresh-root>` authors six independent theme packs/exports, validates
readback/undo/redo/PNG/reopen and writes `theme_extensions.json`. Each theme uses
six roles in its own stable 20-cell carrier. Source pack stays unchanged; game
color ramps live in metadata, embedded export palette remains crypt_dark.
Retained final editable outputs: `build/room-theme-extensions-final-20261009/`.

## Theme refinement checkpoint

Latest editable packs: `build/room-theme-refinement-20261009/`. Cave now uses
charcoal bedrock/rubble; royal ornaments are overlapping gold coin piles; marble
medallions are replaced by handled urns and stepped column bases. Frozen base
cells are quiet slabs; Dungeon adds seeded room-wide ice shelves/fractures.
All six MCP exports pass readback, undo/redo, independent PNG validation and
saved reopen. Final semantic ice palette is generated from the same recipe
without changing exported indices. Native review: Dungeon Main Edit
`build/room-theme-refinement-final-20261009/`.

## Focused cave/marble authoring

`prepare_room_theme_extensions.py --themes deep_cave marble_sanctuary` authors
only the selected theme packs. A subset output is not a complete standalone
six-theme kit: merge its selected child exports/metadata into the existing
library. Latest two editable packs: `build/marble-cave-authoring-20261009/`.
Paired urns use half-width source silhouettes mapped to a 2x1 native footprint,
keeping each vessel proportionate. Cave formations use several angular faces.
Native review: Dungeon `build/marble-cave-final-20261009/`.
