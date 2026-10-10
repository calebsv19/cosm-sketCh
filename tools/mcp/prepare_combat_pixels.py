"""Author top-down campfire, directional bolts and sparse combat cues via MCP."""
import argparse, asyncio, hashlib, json, math
from pathlib import Path
from prepare_candidate import session
from candidate_review import verify_export
from prepare_enemy_sprites import rect, line
from prepare_room_theme_extensions import ellipse, dot
from prepare_theme_kit import poly

KEYS = ['camp.unlit'] + [f'camp.lit.{i}' for i in range(3)] + [
    'projectile.bullet', 'projectile.fire.0', 'projectile.fire.1', 'projectile.ice',
    'projectile.poison', 'projectile.arcane', 'projectile.heal', 'projectile.lightning'] + [
    f'effect.impact.{i}' for i in range(3)] + [f'effect.damage.{i}' for i in range(3)] + [
    'effect.aura.'+x for x in ['heal','poison','fire','ice','arcane','lightning','shield','speed']] + ['projectile.enemy_bullet']
COLORS = dict(heal=7, poison=23, fire=20, ice=22, arcane=13, lightning=21, shield=10, speed=17)

def pixels(key):
    p = [0]*256
    if key.startswith('camp.'):
        # Uneven stone ring, thick charred logs and scattered ash, all seen overhead.
        for x,y in [(4,2),(9,2),(12,5),(13,9),(9,13),(4,12),(2,8),(2,4)]:
            ellipse(p,x,y,1,1,2);dot(p,x,y,3);dot(p,x,y-1,4)
        poly(p,[(3,4),(5,3),(12,10),(11,12)],1)
        poly(p,[(4,4),(5,4),(11,10),(10,11)],8)
        line(p,(4,4),(10,10),15);dot(p,11,11,3)
        poly(p,[(10,3),(12,4),(5,12),(3,10)],1)
        poly(p,[(10,4),(11,4),(5,11),(4,10)],8)
        line(p,(10,4),(4,10),15);dot(p,5,11,3)
        for x,y in [(6,5),(8,10),(5,8),(10,7)]:dot(p,x,y,2)
        if key!='camp.unlit':
            phase=int(key[-1])
            # Separate licking patches expose wood between the hot centers.
            shapes=[
                [[(5,7),(5,5),(7,4),(8,6),(7,8)],[(8,8),(10,6),(11,7),(10,10),(8,11)],[(6,9),(7,8),(8,10),(6,11)]],
                [[(4,6),(6,4),(7,5),(7,7),(5,8)],[(8,6),(9,4),(10,6),(9,8)],[(7,9),(9,8),(11,10),(9,12),(7,11)]],
                [[(5,8),(4,6),(6,5),(8,7),(7,9)],[(9,7),(10,5),(12,7),(10,9)],[(6,10),(8,9),(9,11),(7,12)]]]
            for shape in shapes[phase]:poly(p,shape,11)
            cores=[[(6,6),(9,8),(7,10)],[(5,6),(9,6),(9,10)],[(6,7),(10,7),(7,11)]]
            for x,y in cores[phase]:
                rect(p,x,y,2,2,20);dot(p,x,y,21)
            dot(p,3+phase,6-phase,20)
    elif key.startswith('projectile.'):
        role=key.split('.')[1];c=COLORS.get(role,16)
        if role in ['bullet','enemy_bullet']:
            rect(p,6,4,3,8,1);rect(p,7,5,1,5,11 if role=='enemy_bullet' else 16);dot(p,7,4,20 if role=='enemy_bullet' else 10)
        elif role=='lightning':
            poly(p,[(8,1),(4,8),(7,8),(6,14),(11,6),(8,6)],1)
            line(p,(8,2),(5,7),21);line(p,(5,7),(9,7),21);line(p,(9,7),(6,13),21)
        else:
            poly(p,[(8,2),(4,6),(5,10),(7,14),(8,11),(11,10),(12,6)],1)
            poly(p,[(8,3),(5,6),(6,10),(8,12),(10,9),(11,6)],c)
            line(p,(8,4),(7,8),10 if role!='fire' else 21)
            if role=='fire':
                dot(p,6+int(key[-1]),11,20);dot(p,9,13-int(key[-1]),21)
            if role=='heal':line(p,(6,7),(10,7),7);line(p,(8,5),(8,9),10)
    elif '.aura.' in key:
        role=key.rsplit('.',1)[1];c=COLORS[role]
        if role=='heal':
            # Fixed-size cue; the renderer distributes these around a thin area boundary.
            line(p,(6,8),(10,8),7);line(p,(8,6),(8,10),7);dot(p,8,8,10)
            return p
        if role=='shield':
            # Small rim glint, never a shield-shaped cap above the character.
            line(p,(7,8),(9,8),22);line(p,(8,7),(8,9),22);dot(p,8,8,10)
            return p
        # Other statuses retain their existing segmented boundary.
        for a,b in [((5,1),(9,1)),((12,3),(14,5)),((14,9),(12,12)),
                    ((9,14),(5,14)),((3,12),(1,9)),((1,5),(3,3))]:line(p,a,b,c)
        if role=='heal':line(p,(6,2),(8,2),10);line(p,(7,1),(7,3),10)
        elif role=='ice':line(p,(12,3),(10,5),10);dot(p,12,2,10)
        elif role=='lightning':line(p,(11,2),(9,4),21);line(p,(9,4),(11,4),21)
        elif role=='shield':poly(p,[(6,1),(10,1),(10,3),(8,5),(6,3)],10)
        elif role=='speed':line(p,(6,1),(8,2),17);line(p,(8,2),(6,3),17)
        else:dot(p,11,2,c);dot(p,3,13,c)
    else:
        phase=int(key[-1]);c=11 if '.damage.' in key else 21
        for j in range(6):
            a=j*math.pi/3;radius=2+phase*2
            x,y=round(7.5+math.cos(a)*radius),round(7.5+math.sin(a)*radius)
            dot(p,x,y,c);dot(p,x+round(math.cos(a)),y+round(math.sin(a)),c)
        if phase==0:rect(p,7,7,2,2,10 if c==21 else 20)
    return p

async def prepare(a):
    digest=hashlib.sha256(a.source.read_bytes()).hexdigest()
    async def author(call,mutate):
        project=await call('inspect_project');assert project['tiles']==KEYS
        for k in KEYS:await mutate('replace_tile',tile=k,indices=pixels(k))
        after={k:await call('read_tile',tile=k) for k in KEYS}
        await mutate('undo');await mutate('redo')
        assert (await call('read_tile',tile=KEYS[-1]))['indices']==after[KEYS[-1]]['indices']
        await mutate('save_candidate',name='combat');await mutate('export_candidate',name='candidate',palette='combat_common')
        verify_export(a.output/'candidate',project,after,palette_id='combat_common');return after
    after=await session(a,a.source,True,author)
    async def reopen(call,mutate):
        for k,v in after.items():assert (await call('read_tile',tile=k))['indices']==v['indices']
    await session(a,a.output/'combat.pack',False,reopen)
    assert hashlib.sha256(a.source.read_bytes()).hexdigest()==digest
    (a.output/'candidate/combat_pixels.json').write_text(json.dumps(dict(schema=1,atlas='combat_pixels_v1',palette='combat_common',cells=KEYS),indent=2)+'\n')
    (a.output/'readback.json').write_text(json.dumps(after)+'\n')
    (a.output/'receipt.json').write_text(json.dumps(dict(status='pass',cells=len(KEYS),source_sha256=digest,checks=['exact MCP pixels','undo/redo','export validation','editable reopen','source unchanged']),indent=2)+'\n')
    print('PASS campfire, projectiles and combat cues through Sketch MCP')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for key in ['app','source','output']:p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args()
    for key in ['app','source','output']:setattr(a,key,getattr(a,key).resolve())
    a.output.mkdir(parents=True,exist_ok=False);asyncio.run(prepare(a))
