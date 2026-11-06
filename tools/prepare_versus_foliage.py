"""Extrai copas e arbustos do Versus; preserva madeira, bases e a grama aprovada."""
from pathlib import Path
import hashlib
import json
import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage
from prepare_menu_grass import align, signal
from prepare_menu_trees import polygon
from prepare_versus_grass import BOXES

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT/'assets/sprites/versus_background_generated_v1.png'
OUTPUT = ROOT/'assets/sprites/versus'
PREVIEW = ROOT/'build/versus_foliage_preview'
ATLAS_WIDTH = 2048
# Contornos aproximados servem só à extração; o alfa contém unicamente folhas.
REGIONS = [
    ('canopy_left', [(0,0),(334,0),(334,112),(277,166),(192,240),(0,259)], 4, 259),
    ('canopy_right', [(1324,0),(1671,0),(1671,297),(1518,261),(1364,202),(1324,124)], 4, 297),
    ('bush_left', [(83,276),(216,276),(320,420),(320,477),(139,485),(83,418)], 3, 477),
    ('bush_right', [(1328,276),(1568,276),(1568,466),(1328,466)], 3, 466),
    ('tree_left_small', [(273,95),(345,95),(345,152),(273,152)], 2, 152),
    ('tree_left', [(334,69),(438,69),(438,150),(334,150)], 2, 150),
    ('tree_center', [(577,45),(745,45),(745,147),(577,147)], 2, 147),
    ('tree_right', [(1056,72),(1193,72),(1193,149),(1056,149)], 2, 149),
    ('tree_right_small', [(1212,96),(1283,96),(1283,154),(1212,154)], 2, 154),
    ('tree_right_edge', [(1273,69),(1385,69),(1385,151),(1273,151)], 2, 151),
    ('bush_far_left', [(493,127),(625,127),(625,177),(493,177)], 2, 177),
    ('bush_far_center', [(704,143),(784,143),(784,175),(704,175)], 2, 175),
    ('bush_far_right', [(976,135),(1077,135),(1077,176),(976,176)], 2, 176),
]


def prepare():
    PREVIEW.mkdir(parents=True,exist_ok=True)
    protected_paths=[SOURCE,OUTPUT/'grass_base.png',OUTPUT/'grass_atlas.png',
                     ROOT/'include/versus_grass_data.h',ROOT/'tools/prepare_versus_grass.py']
    hashes={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in protected_paths}
    pixels=np.array(Image.open(SOURCE).convert('RGB'))
    ground=np.array(Image.open(OUTPUT/'grass_base.png').convert('RGB'))
    height,width=pixels.shape[:2]
    assert (width,height)==(1672,941)
    r,g,b=[pixels[:,:,i].astype(float) for i in range(3)]
    yy,xx=np.mgrid[:height,:width]
    grass_guard=np.zeros((height,width),bool)
    for x0,y0,x1,y1 in BOXES:
        grass_guard[max(0,y0-10):y1+10,max(0,x0-10):x1+10]=True
    grass_guard |= np.any(ground!=pixels,axis=2)
    wood=(r>g+5)&(b<r*.9)
    structural=wood|grass_guard
    distance=ndimage.distance_transform_edt(~structural)
    edge=np.minimum.reduce((xx,width-1-xx,yy,height-1-yy))
    green=(g>r+4)&(b<g*.72)&((r<126)|(b<58))
    occupied=np.zeros((height,width),bool)
    regions=[]
    for i,(name,points,amplitude,base) in enumerate(REGIONS):
        area=polygon(points,(width,height))
        mask=area&green&~structural&~occupied
        labels,_=ndimage.label(mask)
        sizes=np.bincount(labels.ravel())
        ids=np.flatnonzero(sizes>=14)
        mask &= np.isin(labels,ids[ids!=0])
        mask=ndimage.binary_fill_holes(mask)&area&~structural&~occupied
        assert mask.any(),name
        occupied |= mask
        ys,xs=np.where(mask)
        ox,oy=max(0,int(xs.min())-8),max(0,int(ys.min())-3)
        x1,y1=min(width,int(xs.max())+9),min(height,int(ys.max())+4)
        w,h=align(x1-ox),align(y1-oy)
        layer=np.zeros((h,w,4),np.uint8)
        layer[ys-oy,xs-ox,:3]=pixels[ys,xs]
        layer[ys-oy,xs-ox,3]=255
        # Fade perto de ligações, limites e bases: nenhuma madeira ou raiz se move.
        guard=10 if i<4 else 3
        weight=np.clip((distance-guard)/(45 if i<2 else 15),0,1)**1.25
        weight*=np.clip(edge/18,0,1)
        weight*=np.clip((ndimage.distance_transform_edt(area)-2)/12,0,1)
        if i>=2:
            weight*=np.clip((base-10-yy)/max(18,base-int(ys.min())-10),0,1)**1.2
        if i>=4:
            ax,ay=points[0]; bx=points[1][0]
            weight*=np.clip(np.minimum(xx-ax,bx-1-xx)/10,0,1)
        local_weight=np.zeros((h,w))
        local_weight[:y1-oy,:x1-ox]=weight[oy:y1,ox:x1]
        region=dict(name=name,origin=(ox,oy),size=(w,h),layer=layer,
                    weight=local_weight,steps=amplitude,fps=(6,7,8)[i%3],base=base)
        regions.append(region)
    # Evita cruzar a folhagem vizinha na borda das seleções distantes.
    static_green=green&~occupied
    exposed=np.zeros((height,width),bool)
    motion=np.zeros_like(exposed)
    annotation=Image.fromarray(pixels)
    draw=ImageDraw.Draw(annotation)
    for i,region in enumerate(regions):
        ox,oy=region['origin']; w,h=region['size']
        layer=region['layer']; local_weight=region['weight']
        own=np.zeros((height,width),bool)
        rh,rw=min(h,height-oy),min(w,width-ox)
        own[oy:oy+rh,ox:ox+rw]=layer[:rh,:rw,3]>0
        # As ligações entre copas sobrepostas e a floresta distante também ficam fixas.
        neighbors=(occupied&~own)|static_green
        neighbor_distance=ndimage.distance_transform_edt(~neighbors)
        local_weight[:rh,:rw]*=np.clip((neighbor_distance[oy:oy+rh,ox:ox+rw]-(4 if i<4 else 1))/(12 if i<4 else 4),0,1)
        ly,lx=np.mgrid[:h,:w]
        poses=[]
        phase=.31+i*.53+ox/width
        frames=np.rint([region['steps']*signal(f,phase) for f in range(32)]).astype(int)
        assert np.abs(frames-np.roll(frames,1)).max()<=1
        local_exposed=np.zeros((h,w),bool)
        fixed=local_weight==0
        for step in range(region['steps']+1):
            sx=lx-np.rint(step*local_weight).astype(int)
            valid=(sx>=0)&(sx<w)
            pose=np.zeros_like(layer)
            pose[valid]=layer[ly[valid],sx[valid]]
            pose[fixed]=layer[fixed]
            assert np.array_equal(pose[fixed],layer[fixed])
            local_exposed |= (layer[:,:,3]>0)&(pose[:,:,3]==0)
            changed=np.any(pose!=layer,axis=2)
            rh,rw=min(h,height-oy),min(w,width-ox)
            motion[oy:oy+rh,ox:ox+rw] |= changed[:rh,:rw]
            poses.append(Image.fromarray(pose))
        assert not np.array_equal(np.array(poses[0]),np.array(poses[-1])),region['name']
        region.update(poses=poses,frames=frames.tolist())
        rh,rw=min(h,height-oy),min(w,width-ox)
        exposed[oy:oy+rh,ox:ox+rw] |= local_exposed[:rh,:rw]
        Image.fromarray(layer).save(PREVIEW/f'{region["name"]}_layer.png')
        draw.rectangle((ox,oy,min(width-1,ox+w),min(height-1,oy+h)),outline='#ff6b34',width=2)
        draw.text((ox+3,oy+3),region['name'],fill='white',stroke_width=1,stroke_fill='black')
    exposed=ndimage.binary_dilation(exposed,iterations=2)&~structural&~static_green
    assert not (motion&structural).any()
    # Só as franjas que ficam expostas são reconstruídas, com cor do cenário vizinho.
    donors=~ndimage.binary_dilation(occupied,iterations=3)&~structural&(g>r+2)
    _,nearest=ndimage.distance_transform_edt(~donors,return_indices=True)
    background=ground.copy()
    background[exposed]=ground[nearest[0][exposed],nearest[1][exposed]]
    assert np.array_equal(background[~exposed],ground[~exposed])
    assert np.array_equal(background[structural|static_green],ground[structural|static_green])
    Image.fromarray(background).save(OUTPUT/'foliage_base.png')
    Image.fromarray(np.uint8(exposed)*255).save(PREVIEW/'exposed_mask.png')
    Image.fromarray(np.uint8(motion|exposed)*255).save(PREVIEW/'animated_pixels.png')
    Image.fromarray(np.uint8(structural)*255).save(PREVIEW/'protected_pixels.png')
    annotation.save(PREVIEW/'regions.png')
    cells=[(region,i,pose) for region in regions for i,pose in enumerate(region['poses'])]
    cx=cy=row=0
    for region,i,pose in sorted(cells,key=lambda c:c[2].height,reverse=True):
        w,h=pose.size
        if cx+w>ATLAS_WIDTH:
            cy+=row; cx=row=0
        region.setdefault('sources',{})[i]=(cx,cy,w,h)
        cx+=w; row=max(row,h)
    atlas=Image.new('RGBA',(ATLAS_WIDTH,align(cy+row)))
    for region,i,pose in cells:
        atlas.paste(pose,region['sources'][i][:2])
    atlas.save(OUTPUT/'foliage_atlas.png')
    header=['/* Copas e arbustos gerados por tools/prepare_versus_foliage.py. */',
            '#ifndef VERSUS_FOLIAGE_DATA_H','#define VERSUS_FOLIAGE_DATA_H','#include "raylib.h"',
            f'#define VERSUS_FOLIAGE_COUNT {len(regions)}',f'#define VERSUS_FOLIAGE_ATLAS_WIDTH {ATLAS_WIDTH}',
            f'#define VERSUS_FOLIAGE_ATLAS_HEIGHT {atlas.height}',
            'typedef struct { Vector2 origin; Rectangle poses[5]; unsigned char frames[32]; int fps; } VersusFoliageRegion;',
            'static const VersusFoliageRegion VERSUS_FOLIAGE_REGIONS[VERSUS_FOLIAGE_COUNT] = {']
    for region in regions:
        origin=', '.join(map(str,region['origin']))
        poses=', '.join('{'+', '.join(map(str,region['sources'][i]))+'}' for i in range(len(region['poses'])))
        frames=', '.join(map(str,region['frames']))
        header.append(f'    {{{{{origin}}}, {{{poses}}}, {{{frames}}}, {region["fps"]}}}, /* {region["name"]} */')
    header+=['};','#endif','']
    path=ROOT/'include/versus_foliage_data.h'; text='\n'.join(header)
    if not path.exists() or path.read_text(encoding='utf-8')!=text:
        path.write_text(text,encoding='utf-8')
    report=dict(protected_hashes=hashes,region_count=len(regions),atlas_size=atlas.size,
                exposed_pixels=int(exposed.sum()),outside_exposed_unchanged=True,
                regions=[{k:v for k,v in region.items() if k not in ('layer','poses','weight')} for region in regions])
    (PREVIEW/'inspection.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    # Contato de transições, extremos e retorno do ciclo; sem reamostrar os sprites.
    scenes=[]
    for f in range(64):
        scene=Image.fromarray(background).convert('RGBA')
        for region in regions:
            frame=int(f/8*region['fps'])%32
            scene.alpha_composite(region['poses'][region['frames'][frame]],region['origin'])
        assert np.array_equal(np.array(scene)[:,:,:3][structural|static_green],ground[structural|static_green])
        scenes.append(scene.convert('RGB'))
    for f in (0,1,15,31,32,63): scenes[f].save(PREVIEW/f'scene_{f:02d}.png')
    contact=Image.new('RGB',(1280,5*200),'#263b26'); draw=ImageDraw.Draw(contact)
    crops=[(200,100,302,204),(1365,100,1500,218),(190,321,310,462),(1340,330,1460,460),(590,45,745,145)]
    for row,box in enumerate(crops):
        for col,f in enumerate((0,1,15,31,32)):
            crop=scenes[f].crop(box)
            scale=min(245/crop.width,175/crop.height)
            crop=crop.resize((round(crop.width*scale),round(crop.height*scale)),Image.Resampling.NEAREST)
            contact.paste(crop,(col*256,row*200+24))
            draw.text((col*256,row*200),f'{f/8:.3f}s',fill='white')
    contact.save(PREVIEW/'closeup.png')
    for p in protected_paths:
        assert hashlib.sha256(p.read_bytes()).hexdigest()==hashes[str(p.relative_to(ROOT))]
    print(f'{len(regions)} copas/arbustos; atlas {atlas.size}; {exposed.sum()} pixels expostos; grama intacta.')


if __name__=='__main__':
    prepare()
