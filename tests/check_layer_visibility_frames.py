"""Check the painted center of the 32-bit BMP frames emitted by the SDL pilot."""
from pathlib import Path
import struct
import sys


def center_pixel(path):
    frame = path.read_bytes()
    offset = struct.unpack_from("<I", frame, 10)[0]
    width, height, planes, bits = struct.unpack_from("<iiHH", frame, 18)
    if frame[:2] != b"BM" or planes != 1 or bits != 32 or min(width, abs(height)) < 2:
        raise ValueError(f"Unsupported capture format in {path}")
    y = abs(height) // 2
    row = height - 1 - y if height > 0 else y
    start = offset + 4 * (row * width + width // 2)
    blue, green, red = frame[start:start + 3]
    return (red, green, blue)


if __name__ == "__main__":
    root = Path(sys.argv[1])
    before, hidden, shown, undone = (
        center_pixel(root / f"layer-{phase}.bmp") for phase in (28, 29, 31, 32)
    )
    if before == hidden or before != shown or hidden != undone:
        raise SystemExit(f"Unexpected painted pixel sequence: {before}, {hidden}, {shown}, {undone}")
    print(f"Rendered canvas center RGB: {before} -> {hidden} -> {shown} -> {undone}")
