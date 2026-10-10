"""Author three chest states and one overhead player through Sketch MCP."""
import argparse,asyncio,hashlib,json
from pathlib import Path
from prepare_candidate import session
from candidate_review import verify_export
from prepare_enemy_sprites import rect,line,weapon
from prepare_theme_kit import poly
from prepare_room_theme_extensions import ellipse,dot
KEYS=['chest.wood.closed','chest.wood.locked','chest.wood.open','player.body.idle','weapon.sword.wood','weapon.sword.fire']

def pixels(key):
    p=[0]*256
    if key.startswith('weapon.'):
        p=weapon('sword.steel')
        for i,c in enumerate(p):
            if c==9:p[i]=15 if key.endswith('.wood') else 11
            elif c==10:p[i]=4 if key.endswith('.wood') else 16
    elif key=='player.body.idle':
        ellipse(p,7,8,5,3,1);ellipse(p,7,8,4,2,18)
        ellipse(p,7,6,3,3,1);ellipse(p,7,6,2,2,14)
        # The top of the hair and shoulders are visible, never a standing figure.
        rect(p,5,4,5,2,3);dot(p,6,4,4);rect(p,3,7,2,2,19)
        rect(p,2,8,2,2,14);rect(p,12,8,2,2,14)
    else:
        opened=key.endswith('.open')
        rect(p,2,5,12,8,1);rect(p,3,6,10,6,15)
        rect(p,4,6,1,6,9);rect(p,11,6,1,6,9)
        if opened:
            rect(p,3,6,10,5,2);rect(p,4,7,8,3,1)
            # Open lid lies behind the cavity, viewed from overhead.
            rect(p,2,1,12,4,1);rect(p,3,2,10,2,15)
            rect(p,4,2,1,2,9);rect(p,11,2,1,2,9)
        else:
            rect(p,3,6,10,1,15);line(p,(5,8),(10,8),8)
            rect(p,7,11,2,1,16)
            if key.endswith('.locked'):
                rect(p,6,9,4,3,1);rect(p,7,9,2,2,16);dot(p,7,10,1)
    return p

async def prepare(args):
    digest=hashlib.sha256(args.source.read_bytes()).hexdigest()
    async def author(call,mutate):
        project=await call('inspect_project');assert project['tiles']==KEYS
        for k in KEYS:await mutate('replace_tile',tile=k,indices=pixels(k))
        after={k:await call('read_tile',tile=k) for k in KEYS}
        await mutate('undo');await mutate('redo');assert (await call('read_tile',tile=KEYS[-1]))['indices']==after[KEYS[-1]]['indices']
        await mutate('save_candidate',name='objects');await mutate('export_candidate',name='candidate',palette='objects_common')
        verify_export(args.output/'candidate',project,after,palette_id='objects_common');return after
    after=await session(args,args.source,True,author)
    async def reopen(call,mutate):
        for k,v in after.items():assert (await call('read_tile',tile=k))['indices']==v['indices']
    await session(args,args.output/'objects.pack',False,reopen)
    assert hashlib.sha256(args.source.read_bytes()).hexdigest()==digest
    metadata=dict(schema=1,atlas='game_objects_v1',palette='objects_common',player=dict(cell=KEYS[3],pivot=[8,8],hand=[13,9]),chests=dict(closed=KEYS[0],locked=KEYS[1],opened=KEYS[2]),weapons=dict(wood=dict(cell=KEYS[4],grip=[7.5,12.5]),fire=dict(cell=KEYS[5],grip=[7.5,12.5])))
    (args.output/'candidate/game_objects.json').write_text(json.dumps(metadata,indent=2)+'\n')
    (args.output/'readback.json').write_text(json.dumps(after)+'\n')
    (args.output/'receipt.json').write_text(json.dumps(dict(status='pass',source_sha256=digest,checks=['MCP exact pixels','undo/redo','export validation','editable reopen','source unchanged']),indent=2)+'\n')
    print('PASS three chest states and overhead player via Sketch MCP')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for key in ['app','source','output']:p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args();a.app=a.app.resolve();a.source=a.source.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);asyncio.run(prepare(a))
