"""Lossless source review and independent export checks; no Dungeon rendering."""
import html
import json
import struct
import zlib


def png(path, width, height, rgba, scale=1):
    assert len(rgba) == width * height
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    raw = bytearray()
    for y in range(height):
        row = b''.join(bytes(p) * scale for p in rgba[y * width:(y + 1) * width])
        raw.extend((b'\0' + row) * scale)
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width * scale, height * scale, 8, 6, 0, 0, 0))
                     + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))


def indexed_png(path):
    """Decode and CRC-check the exported noninterlaced 8-bit indexed PNG."""
    data = path.read_bytes()
    assert data[:8] == b'\x89PNG\r\n\x1a\n'
    at, compressed, palette, alpha = 8, bytearray(), None, b''
    while at < len(data):
        size = struct.unpack('>I', data[at:at + 4])[0]
        kind, payload = data[at + 4:at + 8], data[at + 8:at + 8 + size]
        assert zlib.crc32(kind + payload) == struct.unpack('>I', data[at + 8 + size:at + 12 + size])[0]
        if kind == b'IHDR':
            width, height, depth, color, compression, filtering, interlace = struct.unpack('>IIBBBBB', payload)
            assert (depth, color, compression, filtering, interlace) == (8, 3, 0, 0, 0)
        elif kind == b'PLTE': palette = [list(payload[i:i + 3]) for i in range(0, len(payload), 3)]
        elif kind == b'tRNS': alpha = payload
        elif kind == b'IDAT': compressed.extend(payload)
        elif kind == b'IEND': break
        at += size + 12
    raw = zlib.decompress(compressed)
    assert len(raw) == height * (width + 1) and palette is not None
    result, previous = [], [0] * width
    for y in range(height):
        start = y * (width + 1)
        mode, row = raw[start], list(raw[start + 1:start + width + 1])
        assert mode <= 4
        for x in range(width):
            a, b, c = row[x - 1] if x else 0, previous[x], previous[x - 1] if x else 0
            p = a + b - c
            pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
            predictor = [0, a, b, (a + b) // 2, a if pa <= pb and pa <= pc else b if pb <= pc else c][mode]
            row[x] = (row[x] + predictor) & 255
        result.extend(row)
        previous = row
    return width, height, result, [rgb + [alpha[i] if i < len(alpha) else 255] for i, rgb in enumerate(palette)]


def verify_export(directory, project, tiles, baseline=None, palette_id="crypt_dark"):
    manifest = json.loads((directory / 'tileset_manifest.json').read_text())
    validation = json.loads((directory / 'export_validation.json').read_text())
    assert validation['status'] == 'validated'
    assert manifest['logical_tile'] == dict(width=16, height=16)
    assert manifest['preview_palette'] == palette_id
    if baseline is not None:
        assert manifest == baseline, 'Candidate changed metadata or palette roles'
    w, h, indices, colors = indexed_png(directory / manifest['atlas']['image'])
    assert (w, h) == (manifest['atlas']['width'], manifest['atlas']['height'])
    palette = json.loads((directory / ('palettes/'+palette_id+'.palette.json')).read_text())
    assert [s['id'] for s in manifest['palette_slots']] == project['slots']
    assert colors[:len(project['slots'])] == [palette['colors'][slot] for slot in project['slots']]
    assert set(tiles) == {c['key'] for c in manifest['cells']}
    for c in manifest['cells']:
        tile = tiles[c['key']]
        actual = [indices[(c['y'] + y) * w + c['x'] + x] for y in range(16) for x in range(16)]
        assert actual == tile['indices'], c['key']
        assert tile['preview_rgba'] == [colors[i] for i in actual]
    return manifest


def review(directory, before, after):
    directory.mkdir()
    keys = list(after)
    cards = []
    for phase, tiles in [('before', before), ('candidate', after)]:
        sheet = [[13, 18, 24, 255] for _ in range(100 * 80)]
        for n, key in enumerate(keys):
            tile = tiles[key]['preview_rgba']
            for y in range(16):
                for x in range(16):
                    sheet[(n // 5 * 20 + y + 2) * 100 + n % 5 * 20 + x + 2] = tile[y * 16 + x]
            png(directory / f'{phase}-{n:02}.png', 16, 16, tile, 12)
        png(directory / f'{phase}-sheet.png', 100, 80, sheet, 8)
        # Three repeated floor variants in three bands, no invented room resolver.
        repeat = []
        for y in range(96):
            key = ['cobble.floor.a', 'cobble.floor.b', 'cobble.floor.worn'][y // 32]
            repeat.extend(tiles[key]['preview_rgba'][(y % 16) * 16 + x % 16] for x in range(128))
        png(directory / f'{phase}-repeat.png', 128, 96, repeat, 6)
    for n, key in enumerate(keys):
        changed = sum(a != b for a, b in zip(before[key]['indices'], after[key]['indices']))
        cards.append(f'<article><h3>{html.escape(key)}</h3><p>{changed}/256 pixels changed</p><img src="before-{n:02}.png" alt="Before {html.escape(key)}"><img src="candidate-{n:02}.png" alt="Candidate {html.escape(key)}"></article>')
    (directory / 'index.html').write_text('''<!doctype html><meta charset="utf-8"><title>Sketch cobble candidate review</title>
<style>body{background:#101820;color:#e4e9ec;font:16px system-ui;max-width:1100px;margin:32px auto}img{image-rendering:pixelated;max-width:100%}article{display:inline-block;width:48%;vertical-align:top}h3{font-size:16px}article img{width:45%;margin:1%}section{display:flex;gap:16px}section img{width:48%}</style>
<h1>Sketch cobble candidate</h1><p>Source previews only. Left: baseline fixture. Right: candidate. No Dungeon renderer, lighting, overlays or room-placement claim. All 20 stable keys, 16×16 geometry and crypt_dark palette are preserved. Wall letters identify exposed cardinal edges; inspect both pixel placement and repetition before accepting art.</p>
<h2>All tiles (manifest order)</h2><section><img src="before-sheet.png" alt="Baseline tile sheet"><img src="candidate-sheet.png" alt="Candidate tile sheet"></section>
<h2>Floor repetition (a, b, worn)</h2><section><img src="before-repeat.png" alt="Baseline repeated floors"><img src="candidate-repeat.png" alt="Candidate repeated floors"></section>
<h2>Exact tile comparisons</h2>''' + '\n'.join(cards))
