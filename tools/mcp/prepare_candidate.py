#!/usr/bin/env python3
"""Prepare one compatible cobble sample through the official MCP client.

An isolated installed Sketch session authors, saves, exports and reopens the
candidate. Existing output names are never overwritten. This is a bounded sample,
not a palette-family/recipe framework or a Dungeon renderer.
"""
import argparse
import asyncio
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import uuid

from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client
from candidate_review import review, verify_export

ROOT = Path(__file__).resolve().parents[2]
SLOTS = ['transparent', 'void', 'deep_shadow', 'mortar', 'stone_low', 'stone_mid', 'stone_high', 'edge_highlight', 'surface_accent']
KEYS = ['cobble.floor.a', 'cobble.floor.b', 'cobble.floor.worn', 'cobble.solid.interior'] + [
    'cobble.boundary.' + suffix for suffix in ['none', 'n', 'e', 's', 'w', 'ne', 'ns', 'nw', 'es', 'ew', 'sw', 'nes', 'new', 'nsw', 'esw', 'nesw']]


def sample(key):
    """Integer-only staggered stone with top/left light and restrained wear."""
    floor = key.startswith('cobble.floor.')
    variant = {'a': 0, 'b': 1, 'worn': 2}.get(key.rsplit('.', 1)[-1], 0)
    edges = key.rsplit('.', 1)[-1] if key.startswith('cobble.boundary.') else ''
    edges = '' if edges == 'none' else edges
    result = []
    for y in range(16):
        for x in range(16):
            row = y // 8
            bx, by = (x + row * 4) % 8, y % 8
            block = (x + row * 4) // 8
            slot = 3 if bx == 0 or by == 0 else 4 + ((block + row + variant) % 2)
            if bx and by:
                if by == 1 or bx == 1: slot = 6
                elif by == 7 or bx == 7: slot = 2
                elif (bx * 3 + by * 5 + variant * 7) % 23 == 0: slot = 4
                if variant == 2 and ((bx == 4 and 3 <= by <= 5) or (bx == 5 and by == 5)): slot = 3
            if not floor:
                slot = max(2, slot - 1)
                # Crisp directional boundary lips; edges stay inside this tile.
                if ('n' in edges and y < 3) or ('w' in edges and x < 3): slot = 6 if y == 1 or x == 1 else 5
                if ('s' in edges and y >= 13) or ('e' in edges and x >= 13): slot = 2 if y == 15 or x == 15 else 4
            result.append(slot)
    return result


async def session(args, source, writable, operation):
    # Keep the socket name short even when the artifact root is a long path.
    with tempfile.TemporaryDirectory(prefix='sketch-candidate-') as scratch:
        scratch = Path(scratch)
        endpoint = scratch / 'control.sock'
        env = dict(os.environ, SDL_VIDEODRIVER='dummy', DRAWING_PROGRAM_RUNTIME_DIR=str(scratch / 'runtime'),
                   DRAWING_PROGRAM_LOG_DIR=str(scratch / 'logs'), SKETCH_CONTROL_SOCKET=str(endpoint),
                   SKETCH_CONTROL_ALLOW_WRITE='1' if writable else '0', SKETCH_CONTROL_OUTPUT_ROOT=str(args.output),
                   SKETCH_MCP_PYTHON=os.sys.executable)
        log = (args.output / ('author.log' if writable else 'reopen.log')).open('w')
        process = subprocess.Popen([str(args.app), '--render-backend', 'sdl-debug', '--preset', str(source), '--no-persist'], env=env, stdout=log, stderr=log)
        try:
            for _ in range(200):
                if endpoint.exists(): break
                if process.poll() is not None: raise RuntimeError('Sketch exited; see app log')
                await asyncio.sleep(.025)
            assert endpoint.exists(), 'Sketch endpoint did not start'
            params = StdioServerParameters(command=str(ROOT / 'tools/mcp/run.sh'), env=env)
            async with stdio_client(params) as (read, send):
                async with ClientSession(read, send) as client:
                    await client.initialize()
                    assert len((await client.list_tools()).tools) == 13
                    async def call(op, **fields):
                        response = await client.call_tool(op, fields)
                        assert not response.isError, response
                        value = json.loads(response.content[0].text)
                        assert value.get('ok'), value
                        return value
                    cap = await call('capabilities')
                    async def mutate(op, **fields):
                        project = await call('inspect_project')
                        return await call(op, instance=cap['instance'], request_id=str(uuid.uuid4()), expected_state=project['expected_state'], **fields)
                    return await operation(call, mutate)
        finally:
            process.terminate()
            try: process.wait(timeout=5)
            except subprocess.TimeoutExpired: process.kill(); process.wait()
            log.close()
            # Forced isolated process shutdown is not graceful endpoint proof.


async def prepare(args):
    source_hash = hashlib.sha256(args.source.read_bytes()).hexdigest()
    async def author(call, mutate):
        project = await call('inspect_project')
        assert project['profile'] == 'INDEXED_ATLAS_V1'
        assert project['slots'] == SLOTS
        before = {key: await call('read_tile', tile=key) for key in project['tiles']}
        assert list(before) == KEYS, 'This sample requires the existing cobble stable keys/order'
        await mutate('export_candidate', name='baseline', palette='crypt_dark')
        for key in before:
            expected = sample(key)
            await mutate('replace_tile', tile=key, indices=expected)
            assert (await call('read_tile', tile=key))['indices'] == expected
        last = list(before)[-1]
        await mutate('undo')
        assert (await call('read_tile', tile=last))['indices'] == before[last]['indices']
        await mutate('redo')
        assert (await call('read_tile', tile=last))['indices'] == sample(last)
        after = {key: await call('read_tile', tile=key) for key in before}
        await mutate('save_candidate', name='cobble_candidate')
        await mutate('export_candidate', name='candidate', palette='crypt_dark')
        baseline = verify_export(args.output / 'baseline', project, before)
        verify_export(args.output / 'candidate', project, after, baseline)
        return project, before, after
    project, before, after = await session(args, args.source, True, author)
    async def reopened(call, mutate):
        project2 = await call('inspect_project')
        assert project2['slots'] == project['slots'] and project2['tiles'] == project['tiles']
        for key in after:
            tile = await call('read_tile', tile=key)
            assert tile['indices'] == after[key]['indices'] and tile['preview_rgba'] == after[key]['preview_rgba']
    await session(args, args.output / 'cobble_candidate.pack', False, reopened)
    assert hashlib.sha256(args.source.read_bytes()).hexdigest() == source_hash, 'Source changed'
    review(args.output / 'review', before, after)
    (args.output / 'tiles.json').write_text(json.dumps(dict(project=project, baseline=before, candidate=after), indent=2) + '\n')
    hashes = {str(p.relative_to(args.output)): hashlib.sha256(p.read_bytes()).hexdigest()
              for p in sorted(args.output.rglob('*')) if p.is_file()}
    result = dict(status='pass', source=str(args.source), source_sha256=source_hash, installed_app=str(args.app),
                  installed_binary_sha256=hashlib.sha256(args.app.read_bytes()).hexdigest(), tools=13, tiles=20,
                  changed_pixels={k: sum(a != b for a, b in zip(before[k]['indices'], after[k]['indices'])) for k in before},
                  proof=['official MCP edit and exact readback', 'last tile undo/redo', 'candidate reopen exact for all tiles',
                         'independent export PNG indices/PLTE/alpha and unchanged manifest', 'source unchanged'],
                  limitation='Software owner-loop session and source previews; no native Dungeon render or final-art acceptance', sha256=hashes)
    (args.output / 'handoff.json').write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: installed Sketch MCP author/export/reopen; 20 compatible tiles; independent PNG checks; source unchanged')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, required=True, help='Installed Sketch GUI executable')
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True, help='New handoff directory; refuses existing paths')
    args = parser.parse_args()
    args.app, args.source, args.output = args.app.resolve(), args.source.resolve(), args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    asyncio.run(prepare(args))
