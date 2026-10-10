"""Author six north-facing top-down enemies via the production Sketch MCP loop."""
import argparse,asyncio,json,hashlib
from pathlib import Path
from prepare_candidate import session
from candidate_review import verify_export
from prepare_theme_kit import poly
from prepare_room_theme_extensions import ellipse,dot
NAMES=['rat','spider','goblin','swordsman','gunman','wizard']

def rect(p,x,y,w,h,c):
    for yy in range(y,y+h):
        for xx in range(x,x+w):dot(p,xx,yy,c)

def line(p, a, b, color):
    x,y=a;xx,yy=b;steps=max(abs(xx-x),abs(yy-y))
    for i in range(steps+1):dot(p,round(x+(xx-x)*i/max(1,steps)),round(y+(yy-y)*i/max(1,steps)),color)

def sprite(name):
    p=[0]*256
    if name=='rat':
        rect(p,7,10,1,6,5)
        ellipse(p,7,8,4,4,1);ellipse(p,7,8,3,3,3);ellipse(p,6,7,1,2,4)
        poly(p,[(5,4),(7,0),(9,4),(10,6),(5,6)],1)
        poly(p,[(6,4),(7,2),(8,4),(9,5),(6,5)],3)
        rect(p,3,3,2,2,1);rect(p,10,3,2,2,1);dot(p,4,3,5);dot(p,10,3,5)
    elif name=='spider':
        # Four separated narrow limbs on each side, with distinct radial tips.
        for flip in [False,True]:
            for root,knee,tip in [((6,4),(3,2),(2,0)),((6,6),(2,5),(0,3)),((6,8),(2,9),(0,11)),((6,10),(4,12),(3,15))]:
                f=lambda q:(15-q[0],q[1]) if flip else q
                line(p,f(root),f(knee),1);line(p,f(knee),f(tip),1);dot(p,*f(knee),4)
        ellipse(p,7,9,2,3,1);ellipse(p,7,9,1,2,3)
        ellipse(p,7,5,2,2,1);rect(p,6,5,3,2,3);dot(p,6,4,11);dot(p,8,4,11)
    elif name=='goblin':
        # Overhead shoulders behind a large head; no standing torso or feet.
        ellipse(p,7,8,5,3,1);ellipse(p,7,8,4,2,8)
        ellipse(p,7,6,4,3,1);ellipse(p,7,6,3,2,6)
        poly(p,[(4,4),(0,2),(1,6),(4,6)],1);poly(p,[(10,4),(15,2),(14,6),(10,6)],1)
        line(p,(1,3),(3,5),6);line(p,(14,3),(11,5),6)
        ellipse(p,6,5,1,1,7);rect(p,2,8,2,2,6);rect(p,12,8,2,2,6)
    elif name in ['swordsman','gunman']:
        ellipse(p,7,8,5,3,1);ellipse(p,7,8,4,2,9 if name=='swordsman' else 8)
        ellipse(p,7,6,3,3,1)
        if name=='swordsman':
            ellipse(p,7,6,2,2,9);rect(p,6,4,2,4,10);dot(p,7,3,11)
            ellipse(p,3,8,1,1,10);ellipse(p,11,8,1,1,10)
        else:
            ellipse(p,7,6,2,2,11);rect(p,5,3,5,1,1);rect(p,6,3,3,1,11)
            dot(p,6,5,14);rect(p,2,8,2,2,14);rect(p,12,8,2,2,14)
    else:
        # Broad overhead hat brim and crown obscure most of the cloak below.
        ellipse(p,7,9,4,3,1);ellipse(p,7,9,3,2,12)
        ellipse(p,7,6,5,4,1);ellipse(p,7,6,4,3,12)
        poly(p,[(7,2),(4,6),(7,9),(10,6)],13)
        poly(p,[(7,3),(6,6),(7,8),(8,6)],12);dot(p,7,5,13)
        dot(p,7,9,16);rect(p,12,8,2,2,14)
    return p

WEAPONS=['sword.steel','sword.ice','gun.pistol','gun.shotgun','staff.ember']
KEYS=['enemy.'+n+'.idle' for n in NAMES]+['weapon.'+w for w in WEAPONS]

def weapon(name):
    p=[0]*256
    if name.startswith('sword'):
        poly(p,[(7,0),(5,3),(6,10),(9,10),(10,3),(8,0)],1)
        poly(p,[(7,1),(6,3),(7,9),(8,9),(9,3),(8,1)],17 if name.endswith('ice') else 9)
        line(p,(7,2),(7,8),10);rect(p,4,10,8,2,1);rect(p,5,10,6,1,16)
        rect(p,7,12,2,3,1);rect(p,7,12,1,2,15)
    elif name.startswith('gun'):
        shotgun=name.endswith('shotgun')
        rect(p,6 if shotgun else 7,1 if shotgun else 4,4 if shotgun else 3,11 if shotgun else 8,1)
        rect(p,7 if shotgun else 8,2 if shotgun else 5,2 if shotgun else 1,7 if shotgun else 4,9)
        if shotgun:rect(p,6,8,4,3,15);rect(p,7,12,2,3,15);dot(p,9,2,10)
        else:rect(p,7,10,2,4,15);dot(p,8,5,10)
    else:
        rect(p,7,4,2,11,1);rect(p,7,5,1,9,15)
        poly(p,[(7,0),(4,3),(5,6),(10,6),(11,3),(8,0)],1)
        poly(p,[(7,1),(5,3),(7,5),(9,4),(10,3),(8,1)],11);rect(p,7,2,2,2,16);dot(p,7,2,10)
    return p

async def prepare(args):
    source_hash=hashlib.sha256(args.source.read_bytes()).hexdigest()
    async def author(call,mutate):
        project=await call('inspect_project');assert project['tiles']==KEYS
        for n in NAMES:await mutate('replace_tile',tile='enemy.'+n+'.idle',indices=sprite(n))
        for w in WEAPONS:await mutate('replace_tile',tile='weapon.'+w,indices=weapon(w))
        after={key:await call('read_tile',tile=key) for key in project['tiles']}
        await mutate('undo');await mutate('redo');assert (await call('read_tile',tile=KEYS[-1]))['indices']==after[KEYS[-1]]['indices']
        await mutate('save_candidate',name='enemies');await mutate('export_candidate',name='candidate',palette='enemy_common');verify_export(args.output/'candidate',project,after,palette_id='enemy_common')
        return after
    after=await session(args,args.source,True,author)
    async def reopen(call,mutate):
        for k,v in after.items():assert (await call('read_tile',tile=k))['indices']==v['indices']
    await session(args,args.output/'enemies.pack',False,reopen)
    assert hashlib.sha256(args.source.read_bytes()).hexdigest()==source_hash
    manifest=dict(schema=2,atlas='enemy_topdown_v1',palette='enemy_common',forward=[0,-1],
                  sprites=[dict(catalog_id=i,cell='enemy.'+n+'.idle',pivot=[8,8],hand=[13,9],weapon=w)
                           for i,n,w in [(2,'rat',''),(3,'spider',''),(1,'goblin',''),(5,'swordsman','sword.steel'),(12,'gunman','gun.pistol'),(14,'wizard','staff.ember'),(16,'gunman','gun.shotgun')]],
                  weapons=[dict(id=w,cell='weapon.'+w,grip=[7.5,12.5]) for w in WEAPONS])
    (args.output/'candidate/enemy_sprites.json').write_text(json.dumps(manifest,indent=2)+'\n')
    (args.output/'readback.json').write_text(json.dumps(after));(args.output/'receipt.json').write_text(json.dumps(dict(status='pass',source_sha256=source_hash,checks=['MCP exact pixels','undo/redo','export readback','editable reopen','source unchanged']),indent=2)+'\n')
    print('PASS six refined bodies and five separate weapon assets via Sketch MCP')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for n in ['app','source','output']:p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();a.app=a.app.resolve();a.source=a.source.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);asyncio.run(prepare(a))
