#!/usr/bin/env python3
"""Author a three-theme material/decor kit through the installed Sketch MCP owner loop.
Existing 20-cell indexed carrier is retained; theme_kit.json names each role.
"""
import argparse,asyncio,json,hashlib
from pathlib import Path
from prepare_candidate import session,sample,KEYS
from candidate_review import verify_export

def poly(p,points,ink):
    for y in range(16):
        for x in range(16):
            inside=False
            for a,b in zip(points,points[1:]+points[:1]):
                if (a[1]>y)!=(b[1]>y) and x<(b[0]-a[0])*(y-a[1])/(b[1]-a[1])+a[0]:inside=not inside
            if inside:p[y*16+x]=ink

def rock(p,x,y,w,h):
    poly(p,[(x,y+h//3),(x+w//3,y),(x+w-2,y+1),(x+w,y+h//2),(x+w-1,y+h),(x+1,y+h)],2)
    poly(p,[(x+1,y+h//3),(x+w//3,y+1),(x+w-2,y+2),(x+w-2,y+h-1),(x+2,y+h-1)],5)
    poly(p,[(x+1,y+h//3),(x+w//3,y+1),(x+w-2,y+2),(x+w//2,y+h//2)],6)

def decoration(theme,kind):
    p=[0]*256
    if theme==0:
        if kind==0:rock(p,5,8,5,4);rock(p,10,11,3,3)
        elif kind==1:rock(p,2,7,7,7);rock(p,8,4,6,6);rock(p,10,11,4,3)
        else:rock(p,1,2,14,12);poly(p,[(8,4),(9,5),(8,9),(10,12),(9,12),(7,9)],3)
    elif theme==1:
        if kind==0:
            for points in [[(8,14),(2,10),(1,6),(6,7)],[(8,13),(10,5),(14,3),(14,8)],[(8,13),(6,6),(7,2),(10,6)],[(8,14),(11,9),(15,9),(12,13)]]:poly(p,points,5)
            for y in range(7,15):p[y*16+8]=3
            poly(p,[(3,7),(6,8),(8,12)],6)
        elif kind==1:
            for x,y in [(5,6),(11,9)]:
                for sy in range(y,14):p[sy*16+x]=3
                for dx,dy in [(-1,0),(1,0),(0,-1),(0,1)]:p[(y+dy)*16+x+dx]=7
                p[y*16+x]=8
            poly(p,[(4,13),(1,10),(4,10),(6,14)],5)
        else:
            # Wall-rooted winding stem, side branches and staggered leaf pairs.
            path=[(8,0),(8,1),(7,2),(6,3),(5,4),(5,5),(6,6),(7,7),(9,8),(10,9),(10,10),(9,11),(7,12),(6,13),(6,14),(7,15)]
            for x,y in path:p[y*16+x]=3
            for x,y,side in [(6,3,-1),(5,5,1),(7,7,-1),(10,9,1),(9,11,-1),(6,13,1)]:
                poly(p,[(x,y),(x+side*3,y-1),(x+side*4,y-3),(x+side,y-2)],5)
                poly(p,[(x,y+1),(x-side*2,y+2),(x-side*3,y),(x-side,y)],6)
    else:
        if kind==0:rock(p,3,8,7,6);rock(p,10,10,4,4)
        elif kind==1:
            poly(p,[(5,14),(3,10),(5,7),(6,9),(8,3),(9,6),(12,10),(11,14)],8)
            poly(p,[(6,14),(5,11),(7,9),(8,11),(9,7),(10,12),(9,14)],7)
            poly(p,[(7,14),(7,12),(8,10),(9,13),(9,14)],6)
        else:
            rock(p,1,6,14,9);poly(p,[(4,10),(6,6),(10,5),(12,9),(10,12),(6,13)],8);poly(p,[(6,10),(7,7),(10,7),(10,10),(8,11)],7)
    return p

def floor(theme,v):
    if theme<2:
        p=sample(KEYS[v])
        if theme==1:
            for x,y in [(3+v,5),(4+v,5),(4+v,6),(11-v,12)]:p[y*16+x]=8
        return p
    p=[3]*256
    poly(p,[(1,1),(8,1),(10,4),(8,8),(2,9),(1,6)],4+v%2)
    poly(p,[(10,1),(15,2),(15,7),(12,8),(10,5)],5)
    poly(p,[(2,11),(8,9),(12,10),(15,12),(14,15),(4,15),(1,13)],4+(v+1)%2)
    for x,y in [(3,2),(4,2),(11,3),(5,12)]:p[y*16+x]=6
    return p

def kit():
    bases=[(76,81,84),(66,77,65),(29,24,34)]
    lifts=[-10,-10,-8,-10,-2,3,8,9,4]
    deco=[[[0,0,0],[39,44,47],[49,54,57],[67,72,75],[85,90,93],[103,108,111],[121,126,129],[136,141,144],[117,122,125]],
          [[0,0,0],[24,38,25],[33,49,32],[39,60,37],[54,78,46],[70,96,54],[90,118,68],[164,128,164],[198,185,106]],
          [[0,0,0],[11,9,15],[18,15,22],[31,24,37],[43,34,49],[58,47,63],[255,194,78],[244,112,30],[157,49,24]]]
    themes=[]
    for t,name in enumerate(['crypt_cobble','moss_grotto','obsidian_lava']):
        rgb=[[max(0,min(255,c+l)) for c in bases[t]] for l in lifts]
        if t==1:rgb[8]=[62,82,56]
        if t==2:rgb[3]=[39,24,25] # Muted warm seam, independent of stone faces.
        themes.append(dict(theme=name,floors=KEYS[t*3:t*3+3],decorations=KEYS[9+t*3:12+t*3],material_rgb=rgb,decoration_rgb=deco[t]))
    return dict(schema=1,themes=themes)

async def prepare(args):
    source_hash=hashlib.sha256(args.source.read_bytes()).hexdigest()
    async def author(call,mutate):
        project=await call('inspect_project');assert project['tiles']==KEYS
        for t in range(3):
            for v in range(3):await mutate('replace_tile',tile=KEYS[t*3+v],indices=floor(t,v))
            for kind in range(3):await mutate('replace_tile',tile=KEYS[9+t*3+kind],indices=decoration(t,kind))
        after={k:await call('read_tile',tile=k) for k in KEYS}
        await mutate('undo');await mutate('redo');assert (await call('read_tile',tile=KEYS[17]))['indices']==after[KEYS[17]]['indices']
        await mutate('save_candidate',name='theme_kit');await mutate('export_candidate',name='candidate',palette='crypt_dark');verify_export(args.output/'candidate',project,after)
        return after
    after=await session(args,args.source,True,author)
    async def reopen(call,mutate):
        for k in after:assert (await call('read_tile',tile=k))['indices']==after[k]['indices']
    await session(args,args.output/'theme_kit.pack',False,reopen)
    (args.output/'candidate/theme_kit.json').write_text(json.dumps(kit(),indent=2)+'\n')
    assert hashlib.sha256(args.source.read_bytes()).hexdigest()==source_hash
    (args.output/'readback.json').write_text(json.dumps(after))
    (args.output/'receipt.json').write_text(json.dumps(dict(status='pass',source_sha256=source_hash,proof=['18 cell MCP author/readback','undo/redo','exact 20-cell saved reopen','independent export validation','original unchanged'],theme_kit_sha256=hashlib.sha256((args.output/'candidate/theme_kit.json').read_bytes()).hexdigest()),indent=2)+'\n')
    print('PASS three-theme MCP material/decor kit')
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ['app','source','output']:parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();args.app=args.app.resolve();args.source=args.source.resolve();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False);asyncio.run(prepare(args))
