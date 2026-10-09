#!/usr/bin/env python3
"""MCP-author six independent indexed exports for the Dungeon theme library."""
import argparse,asyncio,json,hashlib
from pathlib import Path
from prepare_candidate import session,KEYS
from prepare_theme_kit import poly,rock
from candidate_review import verify_export
NAMES=['sandstone_chamber','deep_cave','royal_court','marble_sanctuary','frozen_hall','flooded_cistern']

def dot(p,x,y,color):
    if 0<=x<16 and 0<=y<16:p[y*16+x]=color

def ellipse(p,cx,cy,rx,ry,color):
    for y in range(16):
        for x in range(16):
            if (x-cx)**2*ry**2+(y-cy)**2*rx**2<=rx**2*ry**2:dot(p,x,y,color)

def slabs(v):
    p=[4]*256
    for y in range(16):
        for x in range(16):
            if x==0 or y==0 or y==8:p[y*16+x]=3
            elif y==1 or y==9:p[y*16+x]=6
            elif (x+y+v)%13==0:p[y*16+x]=5
    return p

def floor(t,v):
    p=slabs(v)
    if t==0:
        if v==2:
            poly(p,[(0,10),(4,7),(9,8),(16,5),(16,16),(0,16)],5)
            for x,y in [(2,11),(7,10),(11,8),(14,12)]:dot(p,x,y,6)
        else:
            for x,y in [(3+v,3),(12,11),(7,6)]:dot(p,x,y,2)
    elif t==1:
        # Broad fractured bedrock, without a repeated four-stone grout grid.
        p=[4]*256
        shapes=[[(0,3),(6,1),(11,5),(16,4),(16,9),(8,11),(0,8)],[(0,12),(5,7),(10,8),(16,13),(16,16),(0,16)],[(0,0),(16,0),(16,6),(10,3),(4,6),(0,4)]]
        poly(p,shapes[v],5)
        for x,y in [(2+v*3,7),(3+v*3,8),(8,12),(9,13)]:dot(p,x,y,3)
    elif t==2:
        p=[4]*256
        for x in range(16):dot(p,x,0,3)
        for y in range(16):dot(p,0,y,3)
        if v==1:
            p=[7]*256
            poly(p,[(8,3),(11,8),(8,13),(5,8)],5)
        elif v==2:
            dot(p,7,7,5)
    elif t==3:
        p=[4]*256
        for x in range(16):dot(p,x,0,3)
        for y in range(16):dot(p,0,y,3)
        if v==1:
            for y in range(3,14):dot(p,3+y//4,y,5)
        if v==2:
            for y in range(16):dot(p,1,y,8);dot(p,2,y,6)
            for x in range(16):dot(p,x,1,8);dot(p,x,2,6)
    elif t==4:
        # Quiet cold slabs; connected room-space ice supplies frost and cracks.
        p=[4]*256
        for x in range(16):dot(p,x,0,3)
        for y in range(16):dot(p,0,y,3)
        for x,y in [(3+v,6),(10,11)]:dot(p,x,y,5)
    else:
        if v:
            p=[6]*256
            for x,y in [(2,5),(3,5),(4,5),(10,11),(11,11),(12,11)]:dot(p,x,y,7)
            if v==2:
                for x,y in [(5,2),(6,2),(7,2),(2,13),(3,13)]:dot(p,x,y,2)
    return p

def column(p,pale):
    # Stepped square plinth and fluted cap: legible top-down architecture.
    poly(p,[(1,4),(14,4),(15,14),(1,14)],2)
    poly(p,[(2,3),(13,3),(13,12),(2,12)],4)
    poly(p,[(4,1),(11,1),(13,4),(13,9),(10,11),(5,11),(3,8),(3,4)],5)
    for y in range(3,10):dot(p,5,y,6);dot(p,8,y,3);dot(p,11,y,6)
    if not pale:
        for x in range(3,13):dot(p,x,12,8)

def coin(p,x,y):
    ellipse(p,x,y+1,3,2,3)
    ellipse(p,x,y,3,1,5)
    for dx in [-1,0,1]:dot(p,x+dx,y-1,6)
    dot(p,x+2,y,4)

def decoration(t,k):
    p=[0]*256
    if t==0:
        if k==0:rock(p,4,9,4,3);rock(p,10,11,3,3)
        elif k==1:
            poly(p,[(5,3),(10,3),(10,6),(13,8),(12,13),(4,13),(3,8),(5,6)],4)
            for x in range(5,11):dot(p,x,3,8);dot(p,x,9,8)
        else:
            poly(p,[(0,13),(3,9),(7,7),(11,8),(15,11),(15,15),(0,15)],5)
            poly(p,[(2,12),(7,8),(11,9),(13,11),(8,10)],6)
    elif t==1:
        if k==0:rock(p,2,9,5,4);rock(p,9,11,4,3)
        elif k==1:
            rock(p,1,5,8,8);rock(p,8,7,7,7);rock(p,6,11,4,4)
        else:
            # Several angular faces, not a stretched solitary square block.
            poly(p,[(0,8),(3,3),(7,2),(10,5),(13,4),(15,9),(14,14),(6,15),(1,12)],2)
            poly(p,[(1,8),(4,4),(7,3),(9,6),(7,11),(3,12)],5)
            poly(p,[(8,7),(12,5),(14,9),(12,13),(7,14),(6,11)],4)
            poly(p,[(3,6),(5,4),(7,4),(6,6)],6)
            poly(p,[(10,7),(12,6),(13,8),(11,8)],5)
    elif t==2:
        if k==2:column(p,False)
        elif k==0:
            for x,y in [(4,11),(9,12),(11,8),(7,7)]:coin(p,x,y)
        else:
            for x,y in [(4,12),(10,12),(7,10),(11,7),(4,8),(8,5),(7,8)]:coin(p,x,y)
    elif t==3:
        if k==2:column(p,True)
        else:
            # Strong handles, rim and resting shadow; blue-glazed amphora.
            poly(p,[(4,13),(11,13),(13,15),(2,15)],2)
            ellipse(p,3,7,2,3,8);ellipse(p,3,7,1,2,0)
            ellipse(p,12,7,2,3,8);ellipse(p,12,7,1,2,0)
            poly(p,[(6,2),(10,2),(10,5),(12,7),(11,12),(9,14),(6,14),(4,12),(3,7),(6,5)],7)
            poly(p,[(6,5),(9,5),(10,8),(9,12),(7,13),(5,10)],5)
            for y in range(6,12):dot(p,6,y,6)
            for x in range(5,11):dot(p,x,2,8);dot(p,x,3,3)
            if k==1:
                # Two vessels drawn into half-width cells; the 2x1 footprint
                # restores their individual aspect instead of stretching one.
                vessel=p[:];p=[0]*256
                for y in range(16):
                    for x in range(8):
                        p[y*16+x]=vessel[y*16+x*2]
                        ink=vessel[y*16+x*2];p[y*16+x+8]=8 if ink==7 else ink
    elif t==4:
        if k==0:poly(p,[(4,13),(5,8),(8,10),(7,14)],5);poly(p,[(10,14),(11,10),(14,12)],6)
        elif k==1:
            for x,y in [(3,12),(8,14),(12,12)]:poly(p,[(x-2,y),(x-1,y-7),(x+1,y-9),(x+2,y-1)],7)
        else:poly(p,[(1,13),(4,5),(7,2),(10,6),(12,4),(15,12),(11,15),(4,15)],5);poly(p,[(4,5),(7,2),(9,7),(6,13)],7)
    else:
        if k==0:
            ellipse(p,8,9,6,4,5);ellipse(p,8,8,5,3,6)
            poly(p,[(8,8),(14,9),(12,12)],0);dot(p,6,7,7)
        elif k==1:
            for x,y in [(4,13),(8,14),(11,13)]:
                for sy in range(4,y):dot(p,x,sy,3)
                poly(p,[(x,y),(x-3,y-5),(x-2,y-7),(x+1,y-2)],5)
                dot(p,x,3,8)
        else:rock(p,2,7,12,8);ellipse(p,7,5,5,3,5);dot(p,5,4,7)
    return p

BASES=[(126,108,78),(40,49,59),(66,49,74),(151,149,137),(83,103,119),(69,80,77)]
def metadata(t):
    b=BASES[t];lifts=[-12,-12,-10,-7,0,4,8,12,6];material=[[max(0,c+l) for c in b] for l in lifts]
    if t==2:material[7]=[73,43,78];material[8]=[111,94,63]
    if t==3:material[7]=[101,120,134];material[8]=[130,118,91]
    if t==4:material[7]=[82,116,136];material[8]=[126,150,163];material[2]=[48,81,105];material[6]=[140,162,173]
    if t==5:material[2]=[39,73,79];material[5]=[73,84,81];material[6]=[43,79,85];material[7]=[58,91,94]
    deco=[[0,0,0]]+[[max(0,min(255,c+l)) for c in b] for l in [-25,-18,-10,0,10,20,30,15]]
    if t==0:deco[8]=[141,97,60]
    if t==1:deco=[[0,0,0],[9,13,18],[13,18,24],[18,24,31],[22,29,36],[32,40,48],[47,57,65],[57,67,73],[36,46,56]]
    if t==2:deco=[[0,0,0],[28,21,16],[61,43,22],[113,78,29],[167,119,40],[201,157,64],[238,204,115],[83,47,86],[145,123,80]]
    if t==3:deco[7]=[91,116,129];deco[8]=[153,119,79]
    if t==5:deco[3]=[52,77,47];deco[5]=[62,92,53];deco[6]=[77,108,61];deco[7]=[143,160,108];deco[8]=[125,98,62]
    return dict(theme=NAMES[t],directory=NAMES[t],floors=KEYS[:3],decorations=KEYS[3:6],material_rgb=material,decoration_rgb=deco)

async def main(args):
    source_hash=hashlib.sha256(args.source.read_bytes()).hexdigest();receipts=[]
    for t,name in enumerate(NAMES):
        if name not in args.themes:continue
        out=args.output/name;out.mkdir();local=argparse.Namespace(app=args.app,source=args.source,output=out)
        async def author(call,mutate):
            project=await call('inspect_project');assert project['tiles']==KEYS
            for i in range(3):await mutate('replace_tile',tile=KEYS[i],indices=floor(t,i));await mutate('replace_tile',tile=KEYS[3+i],indices=decoration(t,i))
            after={k:await call('read_tile',tile=k) for k in KEYS}
            await mutate('undo');await mutate('redo');assert (await call('read_tile',tile=KEYS[5]))['indices']==after[KEYS[5]]['indices']
            await mutate('save_candidate',name='theme');await mutate('export_candidate',name='candidate',palette='crypt_dark');verify_export(out/'candidate',project,after);return after
        after=await session(local,args.source,True,author)
        async def reopen(call,mutate):
            for k in after:assert (await call('read_tile',tile=k))['indices']==after[k]['indices']
        await session(local,out/'theme.pack',False,reopen)
        (out/'readback.json').write_text(json.dumps(after));receipts.append(dict(theme=name,pack_sha256=hashlib.sha256((out/'theme.pack').read_bytes()).hexdigest(),verified='6 MCP roles; undo/redo; 20-cell readback/reopen; independent PNG export'))
        print('PASS',name,flush=True)
    assert hashlib.sha256(args.source.read_bytes()).hexdigest()==source_hash
    (args.output/'theme_extensions.json').write_text(json.dumps(dict(schema=1,themes=[metadata(t) for t in range(6)]),indent=2)+'\n')
    (args.output/'receipt.json').write_text(json.dumps(dict(status='pass',source_sha256=source_hash,themes=receipts),indent=2)+'\n')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for field in ['app','source','output']:p.add_argument('--'+field,type=Path,required=True)
    p.add_argument('--themes',nargs='+',choices=NAMES,default=NAMES)
    a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);asyncio.run(main(a))
