"""Author complete catalog weapon silhouettes and short action cues through Sketch MCP."""
import argparse,asyncio,hashlib,json
from pathlib import Path
from prepare_candidate import session
from candidate_review import verify_export
from prepare_enemy_sprites import weapon,rect,line
from prepare_game_objects import pixels as object_pixels
from prepare_theme_kit import poly
from prepare_room_theme_extensions import dot,ellipse
SWORDS=['wood','iron','steel','heavy','long','fire','ice','freeze','rage','poison','weakness','blind','dual_spin','dual','spinner']
GUNS=['pistol','heavy','semi','shotgun','wide','tight','machine','tracking','surround']
KEYS=['weapon.sword.'+s for s in SWORDS]+['weapon.gun.'+s for s in GUNS]+['cue.muzzle.0','cue.muzzle.1','cue.cast.0','cue.cast.1']
def pixels(key):
    role=key.split('.')[1];name=key.split('.')[2]
    if role=='sword':
        if name in ['wood','fire']:return object_pixels('weapon.sword.'+name)
        p=weapon('sword.ice' if name in ['ice','freeze'] else 'sword.steel')
        if name=='iron':
            p=[3 if v==9 else 4 if v==10 else v for v in p]
        if name in ['heavy','long']:
            rect(p,4,0,8,10,0)
            poly(p,[(7,0),(4 if name=='heavy' else 7,2),(5 if name=='heavy' else 7,9),(10 if name=='heavy' else 8,9),(11 if name=='heavy' else 8,2),(8,0)],1)
            rect(p,6 if name=='heavy' else 7,2,3 if name=='heavy' else 1,7,9);line(p,(7,1),(7,8),10)
        if name in ['rage','poison','weakness','blind']:
            color={'rage':11,'poison':23,'weakness':13,'blind':2}[name]
            line(p,(7,2),(7,8),color);dot(p,8,4,color);rect(p,5,10,6,1,color)
        if name=='freeze':dot(p,6,3,22);dot(p,9,5,22)
        if name in ['dual_spin','dual','spinner']:
            # Short blade with distinct guards; runtime draws actual blade count.
            rect(p,4,0,8,10,0);poly(p,[(7,4),(5,7),(6,10),(9,10),(10,7),(8,4)],1)
            line(p,(7,5),(7,9),9);dot(p,7,5,10);dot(p,5,11,13 if name=='dual_spin' else 9 if name=='dual' else 16)
        return p
    if role=='gun':
        p=weapon('gun.shotgun' if name in ['shotgun','wide','tight'] else 'gun.pistol')
        if name=='heavy':rect(p,6,4,5,7,1);rect(p,7,5,3,4,9);dot(p,8,4,10)
        elif name=='semi':rect(p,9,7,2,3,1);dot(p,9,7,9);dot(p,8,5,16)
        elif name=='wide':rect(p,5,1,6,2,1);rect(p,6,1,4,1,9)
        elif name=='tight':rect(p,7,0,2,3,1);line(p,(7,1),(7,3),9)
        elif name=='machine':
            rect(p,6,2,5,10,1);rect(p,7,3,3,8,9);rect(p,10,7,3,4,1);rect(p,10,8,2,2,3);rect(p,7,0,2,4,1);dot(p,7,1,10);rect(p,7,12,2,3,8)
        elif name=='tracking':
            poly(p,[(6,2),(10,2),(12,6),(10,10),(6,10),(4,6)],1);rect(p,6,4,4,4,9);rect(p,7,2,2,5,17);rect(p,7,10,2,4,8);dot(p,8,3,10)
        elif name=='surround':
            ellipse(p,8,6,5,5,1);ellipse(p,8,6,3,3,9);ellipse(p,8,6,1,1,16)
            for x,y in [(8,1),(12,3),(13,6),(11,10),(8,11),(4,9),(3,6),(4,2)]:dot(p,x,y,10)
            rect(p,7,11,2,4,8)
        return p
    p=[0]*256;phase=int(name)
    if role=='muzzle':
        poly(p,[(8,2+phase),(6,5),(3+phase,6),(6,8),(8,11-phase),(10,8),(13-phase,6),(10,5)],20)
        rect(p,7,5,2,3,21);dot(p,8,6,10)
    else:
        for a,b in [((5,3+phase),(3,5)),((11,3),(13-phase,5)),((3,10),(5,12-phase)),((11,12),(13,10))]:line(p,a,b,17)
        dot(p,8,3+phase,10);dot(p,8,12-phase,10)
    return p
async def prepare(a):
    digest=hashlib.sha256(a.source.read_bytes()).hexdigest()
    async def author(call,mutate):
        project=await call('inspect_project');assert project['tiles']==KEYS
        for key in KEYS:await mutate('replace_tile',tile=key,indices=pixels(key))
        after={k:await call('read_tile',tile=k) for k in KEYS}
        await mutate('undo');await mutate('redo');assert (await call('read_tile',tile=KEYS[-1]))['indices']==after[KEYS[-1]]['indices']
        await mutate('save_candidate',name='weapons');await mutate('export_candidate',name='candidate',palette='weapon_common')
        verify_export(a.output/'candidate',project,after,palette_id='weapon_common');return after
    after=await session(a,a.source,True,author)
    async def reopen(call,mutate):
        for k,v in after.items():assert (await call('read_tile',tile=k))['indices']==v['indices']
    await session(a,a.output/'weapons.pack',False,reopen)
    assert hashlib.sha256(a.source.read_bytes()).hexdigest()==digest
    metadata=dict(schema=1,atlas='weapon_pixels_v1',palette='weapon_common',cells=KEYS)
    (a.output/'candidate/weapon_pixels.json').write_text(json.dumps(metadata,indent=2)+'\n')
    (a.output/'receipt.json').write_text(json.dumps(dict(status='pass',cells=len(KEYS),source_sha256=digest,checks=['exact MCP pixels','undo/redo','export validation','editable reopen','source unchanged']),indent=2)+'\n')
    print('PASS weapon pixels MCP authoring')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for key in ['app','source','output']:p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args()
    for key in ['app','source','output']:setattr(a,key,getattr(a,key).resolve())
    a.output.mkdir(parents=True,exist_ok=False);asyncio.run(prepare(a))
