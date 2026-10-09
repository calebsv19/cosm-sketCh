#!/usr/bin/env python3
"""Local MCP adapter for owner-thread indexed Sketch commands."""
import json
import os
import socket
import struct
import zlib
from typing import Annotated
from mcp.server.fastmcp import FastMCP, Image
from mcp.types import ToolAnnotations
from pydantic import BaseModel, ConfigDict, Field

mcp = FastMCP("sketCh indexed tile authoring", log_level="WARNING")
READ = ToolAnnotations(readOnlyHint=True)
WRITE = ToolAnnotations(readOnlyHint=False, destructiveHint=False, idempotentHint=True)
Slot = Annotated[int, Field(strict=True, ge=0, le=31)]

class Pixel(BaseModel):
    model_config = ConfigDict(extra="forbid")
    x: int = Field(strict=True, ge=0, le=15)
    y: int = Field(strict=True, ge=0, le=15)
    slot: Slot

def bridge_call(payload: dict) -> dict:
    path = os.environ.get("SKETCH_CONTROL_SOCKET")
    if not path:
        raise ValueError("Set SKETCH_CONTROL_SOCKET to the running Sketch endpoint.")
    raw = json.dumps(payload, sort_keys=True, separators=(",", ":"), allow_nan=False).encode() + b"\n"
    if len(raw) > 16384:
        raise ValueError("Request exceeds 16 KiB.")
    try:
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
            connection.settimeout(5)
            connection.connect(path)
            connection.sendall(raw)
            data = bytearray()
            while len(data) <= 131072:
                chunk = connection.recv(min(8192, 131073 - len(data)))
                if not chunk:
                    raise ConnectionError("Incomplete endpoint response.")
                data.extend(chunk)
                if b"\n" in data:
                    return json.loads(data.split(b"\n", 1)[0])
            raise ValueError("Response exceeds limit.")
    except (OSError, ConnectionError) as error:
        raise RuntimeError("Sketch connection failed. A sent operation may have applied; query request_status with its original instance and request_id before retrying.") from error

def write(op, instance, request_id, expected_state, **fields):
    return bridge_call(dict(op=op, instance=instance, request_id=request_id,
                            expected_state=expected_state, **fields))

@mcp.tool(annotations=READ)
def capabilities() -> dict:
    """Read endpoint lifetime, write admission and supported operations."""
    return bridge_call({"op": "capabilities"})

@mcp.tool(annotations=READ)
def inspect_project() -> dict:
    """Read indexed profile, stable tile IDs, palette slot order, dirty state and expected_state."""
    return bridge_call({"op": "inspect_project"})

@mcp.tool(annotations=READ)
def read_tile(tile: str) -> dict:
    """Read 256 row-major exact indices and actual preview RGBA. Coordinates are tile-local 0..15."""
    return bridge_call({"op": "read_tile", "tile": tile})

@mcp.tool(annotations=WRITE)
def replace_tile(instance: str, request_id: str, expected_state: str, tile: str,
                 indices: list[Slot] = Field(min_length=256, max_length=256)) -> dict:
    """Replace one named 16x16 tile as one undo action; no-op adds no history. Check ok/code."""
    return write("replace_tile", instance, request_id, expected_state, tile=tile, indices=indices)

@mcp.tool(annotations=WRITE)
def patch_tile(instance: str, request_id: str, expected_state: str, tile: str,
               pixels: list[Pixel] = Field(max_length=256)) -> dict:
    """Apply one atomic batch; duplicate coordinates or invalid slots reject the whole patch."""
    return write("patch_tile", instance, request_id, expected_state, tile=tile,
                 pixels=[p.model_dump() for p in pixels])

@mcp.tool(annotations=WRITE)
def fill_tile(instance: str, request_id: str, expected_state: str, tile: str,
              x: int = Field(ge=0, le=15), y: int = Field(ge=0, le=15), slot: Slot = ...) -> dict:
    """Exact-index flood fill confined to one named tile; one history action."""
    return write("fill_tile", instance, request_id, expected_state, tile=tile, x=x, y=y, slot=slot)

@mcp.tool(annotations=WRITE)
def copy_tile(instance: str, request_id: str, expected_state: str, tile: str, source_tile: str) -> dict:
    """Copy pixels from an existing tile to another existing tile; IDs and geometry remain intact."""
    return write("copy_tile", instance, request_id, expected_state, tile=tile, source_tile=source_tile)

@mcp.tool(annotations=WRITE)
def undo(instance: str, request_id: str, expected_state: str) -> dict:
    """Undo the current indexed pixel-history entry, including GUI edits; no cell-metadata undo."""
    return write("undo", instance, request_id, expected_state)

@mcp.tool(annotations=WRITE)
def redo(instance: str, request_id: str, expected_state: str) -> dict:
    """Redo the next indexed pixel-history entry. Always use a freshly read expected_state."""
    return write("redo", instance, request_id, expected_state)

@mcp.tool(annotations=WRITE)
def save_candidate(instance: str, request_id: str, expected_state: str, name: str) -> dict:
    """Save an editable candidate .pack under the configured output root; refuses existing names.
    Does not replace the current source asset or change the GUI's selected save destination.
    """
    return write("save_candidate", instance, request_id, expected_state, name=name)

@mcp.tool(annotations=WRITE)
def export_candidate(instance: str, request_id: str, expected_state: str, name: str,
                     palette: str = "default") -> dict:
    """Export one validated tileset transaction under output root; refuses existing names.
    The palette argument names the current preview mapping, not a palette-family selector.
    """
    return write("export_candidate", instance, request_id, expected_state, name=name, palette=palette)

@mcp.tool(annotations=READ)
def request_status(instance: str, request_id: str) -> dict:
    """Recover original terminal response. Receipts live only in this endpoint instance."""
    return bridge_call(dict(op="request_status", instance=instance, request_id=request_id))

def preview_png(rgba: list[list[int]], scale: int) -> bytes:
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))
    rows = bytearray()
    for y in range(16):
        row = b"".join(bytes(rgba[y * 16 + x]) * scale for x in range(16))
        rows.extend((b"\0" + row) * scale)
    size = 16 * scale
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(bytes(rows))) + chunk(b"IEND", b"")

@mcp.tool(annotations=READ)
def preview_tile(tile: str, scale: int = Field(default=16, ge=1, le=32)) -> Image:
    """Return a nearest-neighbor PNG derived from live tile indices and preview palette.
    This is a source preview, not a screenshot of the editor or Dungeon rendering.
    """
    data = read_tile(tile)
    if not data.get("ok"):
        raise ValueError(f"Tile preview failed: {data.get('code')}")
    return Image(data=preview_png(data["preview_rgba"], scale), format="png")

if __name__ == "__main__":
    mcp.run(transport="stdio")
