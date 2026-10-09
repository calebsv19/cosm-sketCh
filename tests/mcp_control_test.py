"""Official SDK negotiation against the real Sketch GUI owner loop, with isolated assets."""
import argparse
import asyncio
import base64
import hashlib
import json
import os
from pathlib import Path
import socket
import subprocess
import time
import uuid
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

ROOT = Path(__file__).resolve().parents[1]

def wire(path, payload):
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        s.settimeout(5); s.connect(str(path))
        s.sendall(payload if isinstance(payload, bytes) else json.dumps(payload).encode()+b"\n")
        data=bytearray()
        while not data.endswith(b"\n"):
            chunk=s.recv(8192); assert chunk; data.extend(chunk)
        return json.loads(data)

async def check(args, name, source, writable, previous=None):
    run=args.output/name; run.mkdir(); endpoint=run/'control.sock'; os.chmod(run,0o700)
    env=dict(os.environ, SDL_VIDEODRIVER='dummy', DRAWING_PROGRAM_RUNTIME_DIR=str(run/'runtime'),
             SKETCH_CONTROL_SOCKET=str(endpoint), SKETCH_CONTROL_ALLOW_WRITE='1' if writable else '0',
             SKETCH_CONTROL_OUTPUT_ROOT=str(args.output), SKETCH_MCP_PYTHON=os.sys.executable)
    log=(run/'app.log').open('w')
    app=subprocess.Popen([str(args.app),'--render-backend','sdl-debug','--preset',str(source),'--no-persist'],env=env,stdout=log,stderr=log)
    try:
        for _ in range(200):
            if endpoint.exists(): break
            assert app.poll() is None, (run/'app.log').read_text()
            await asyncio.sleep(.025)
        assert endpoint.exists(), 'No production endpoint'
        assert endpoint.stat().st_mode & 0o777 == 0o600
        params=StdioServerParameters(command=str(ROOT/'tools/mcp/run.sh'),env=env)
        async with stdio_client(params) as (read,send):
            async with ClientSession(read,send) as client:
                await client.initialize()
                assert len((await client.list_tools()).tools)==13
                async def call(op, **kw):
                    response=await client.call_tool(op,kw)
                    assert not response.isError, response
                    return json.loads(response.content[0].text)
                cap=await call('capabilities'); instance=cap['instance']
                inspect=await call('inspect_project'); assert inspect['profile']=='INDEXED_ATLAS_V1'
                assert len(inspect['tiles'])==20 and len(inspect['slots'])==9
                tile='cobble.floor.a'; before=await call('read_tile',tile=tile)
                async def mutate(op, expected=None, **kw):
                    state=expected or (await call('inspect_project'))['expected_state']
                    return await call(op,instance=instance,request_id=str(uuid.uuid4()),expected_state=state,**kw)
                if previous:
                    assert (await call('request_status',instance=previous,request_id='prior'))['code']=='stale_instance'
                if not writable:
                    assert (await mutate('replace_tile',tile=tile,indices=[0]*256))['code']=='read_only'
                    return before['indices'], instance
                no_op=await mutate('replace_tile',tile=tile,indices=before['indices'])
                assert no_op['ok'] and not no_op['changed'] and no_op['expected_state']==before['expected_state']
                assert (await mutate('undo'))['code']=='history_unavailable'
                request=dict(instance=instance,request_id=str(uuid.uuid4()),expected_state=before['expected_state'],tile=tile,
                             pixels=[dict(x=1,y=1,slot=(before['indices'][17]+1)%9),dict(x=2,y=1,slot=8)])
                patch=await call('patch_tile',**request); assert patch['ok'] and patch['changed']
                after=await call('read_tile',tile=tile); assert after['indices'][17]==request['pixels'][0]['slot'] and after['indices'][18]==8
                assert await call('patch_tile',**request)==patch
                assert (await call('request_status',instance=instance,request_id=request['request_id']))==patch
                conflicting=dict(request,pixels=[dict(x=3,y=3,slot=8)])
                assert (await call('patch_tile',**conflicting))['code']=='request_id_conflict'
                assert (await mutate('replace_tile',expected=before['expected_state'],tile=tile,indices=[0]*256))['code']=='stale_state'
                assert (await mutate('patch_tile',tile=tile,pixels=[dict(x=1,y=1,slot=2),dict(x=1,y=1,slot=3)]))['code']=='invalid_patch'
                assert (await mutate('patch_tile',tile=tile,pixels=[dict(x=0,y=0,slot=1),dict(x=4,y=4,slot=31)]))['code']=='invalid_patch'
                assert (await call('read_tile',tile=tile))['indices']==after['indices']
                assert (await mutate('undo'))['ok']
                assert (await call('read_tile',tile=tile))['indices']==before['indices']
                assert (await mutate('redo'))['ok']
                assert (await call('read_tile',tile=tile))['indices']==after['indices']
                second=await call('read_tile',tile='cobble.floor.b')
                assert (await mutate('copy_tile',tile='cobble.floor.b',source_tile=tile))['ok']
                assert (await call('read_tile',tile='cobble.floor.b'))['indices']==after['indices']
                assert (await mutate('undo'))['ok']
                assert (await call('read_tile',tile='cobble.floor.b'))['indices']==second['indices']
                assert (await mutate('fill_tile',tile=tile,x=1,y=1,slot=(after['indices'][17]+1)%9))['changed']
                assert (await call('read_tile',tile='cobble.floor.b'))['indices']==second['indices']
                assert (await mutate('undo'))['ok']
                assert (await call('read_tile',tile=tile))['indices']==after['indices']
                preview=await client.call_tool('preview_tile',dict(tile=tile,scale=16)); assert not preview.isError
                block=next(c for c in preview.content if c.type=='image')
                (args.output/'edited-tile.png').write_bytes(base64.b64decode(block.data))
                saved=await mutate('save_candidate',name='candidate'); assert saved['ok'],saved
                assert Path(saved['path']).is_file()
                assert (await mutate('save_candidate',name='candidate'))['code']=='destination_exists'
                assert (await mutate('save_candidate',name='../escape'))['code']=='invalid_output'
                exported=await mutate('export_candidate',name='export',palette='crypt_dark'); assert exported['ok'],exported
                manifest=json.loads((Path(exported['path'])/'tileset_manifest.json').read_text())
                assert len(manifest['cells'])==20 and manifest['preview_palette']=='crypt_dark'
                assert (Path(exported['path'])/'export_validation.json').is_file()
                assert (await mutate('export_candidate',name='export'))['code']=='destination_exists'
                assert wire(endpoint,b'{"op":"inspect_project"} trailing\n')['code']=='invalid_request'
                assert wire(endpoint,{'op':'inspect_project','surprise':1})['code']=='invalid_request'
                (args.output/'mcp-result.json').write_text(json.dumps(dict(status='pass',tools=13,tiles=20,slots=9,instance=instance,edited_indices=after['indices']),indent=2))
                return after['indices'],instance
    finally:
        app.terminate()
        try: app.wait(timeout=5)
        except subprocess.TimeoutExpired: app.kill(); app.wait()
        log.close()
        # The forced test process termination is not a graceful shutdown claim.
        if endpoint.exists(): endpoint.unlink()

async def main(args):
    digest=hashlib.sha256(args.source.read_bytes()).hexdigest()
    expected,instance=await check(args,'write',args.source,True)
    actual,_=await check(args,'reopen',args.output/'candidate.pack',False,instance)
    assert actual==expected, 'Saved candidate pixels did not reopen exactly'
    assert hashlib.sha256(args.source.read_bytes()).hexdigest()==digest
    print('PASS: 13 MCP tools, live owner-loop editing, replay/state/atomicity/history, PNG preview, save/reopen/export; source unchanged')

if __name__=='__main__':
    parser=argparse.ArgumentParser(); parser.add_argument('--app',type=Path,required=True)
    parser.add_argument('--source',type=Path,required=True); parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args(); args.output.mkdir(parents=True,exist_ok=True)
    asyncio.run(main(args))
