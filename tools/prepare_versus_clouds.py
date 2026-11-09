"""Remove somente a nuvem baixa e prepara as três nuvens do céu sem alterar recursos aprovados."""
from pathlib import Path
import hashlib
import json
import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage
from scipy.spatial import cKDTree
from prepare_trainer_selection_background import restore_sky

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'assets/sprites/versus_background_generated_v1.png'
OUTPUT=ROOT/'assets/sprites/versus'
PREVIEW=ROOT/'build/versus_cloud_preview'
BOXES=[(318,5,480,68),(768,0,930,46),(1178,5,1343,76)]
BAD_BOX=(1348,220,1524,285)
SECONDS=(512,768,1024)
SKY_HEIGHT=80


def extract(pixels,box):
    x0,y0,x1,y1=box
    rgb=pixels[y0:y1,x0:x1]
    r,g,b=[rgb[:,:,i].astype(float) for i in range(3)]
    mask=(r>195)&(g>185)&(b>125)
    labels,_=ndimage.label(mask)
    sizes=np.bincount(labels.ravel());sizes[0]=0
    mask=ndimage.binary_fill_holes(labels==sizes.argmax())
    assert mask.sum()>1000
    return mask


def restored_patch(pixels,box,mask):
    x0,y0,x1,y1=box
    ax,ay,bx,by=max(0,x0-12),max(0,y0-12),min(1672,x1+12),min(941,y1+12)
    rgb=pixels[ay:by,ax:bx]
    removal=np.zeros(rgb.shape[:2],bool)
    removal[y0-ay:y1-ay,x0-ax:x1-ax]=mask
    removal=ndimage.binary_dilation(removal,iterations=4)
    r,g,b=[rgb[:,:,i].astype(float) for i in range(3)]
    foliage=(g>r+4)&(b<g*.72)&((r<126)|(b<58))
    removal &= ~foliage
    # Uma borda extra permite reconstruir também a nuvem cortada pelo topo da fonte.
    work=np.pad(rgb,((1,1),(1,1),(0,0)),mode='edge')
    erased=np.pad(removal,((1,1),(1,1)))
    if ay==0 and removal[0].any():
        good=np.flatnonzero(~removal[0])
        assert len(good)>2
        for channel in range(3):
            work[0,1:-1,channel]=np.rint(np.interp(np.arange(rgb.shape[1]),good,rgb[0,good,channel])).astype(np.uint8)
    restored=restore_sky(work,erased)[1:-1,1:-1]
    # Quantiza apenas na paleta de fundo local; não deixa branco da nuvem removida.
    palette=np.unique(rgb[~removal & ~foliage],axis=0)
    _,nearest=cKDTree(palette).query(restored[removal].astype(float))
    restored[removal]=palette[nearest]
    layer=np.zeros((*rgb.shape[:2],4),np.uint8)
    layer[removal,:3]=restored[removal];layer[removal,3]=255
    assert np.array_equal(restored[~removal],rgb[~removal])
    return Image.fromarray(layer),(ax,ay),removal


def prepare():
    PREVIEW.mkdir(parents=True,exist_ok=True)
    protected=[SOURCE,*sorted(OUTPUT.glob('*.png')),*sorted((ROOT/'include').glob('versus_*_data.h'))]
    protected=[p for p in protected if not p.name.startswith('cloud_') and p.name!='versus_cloud_data.h']
    hashes={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}
    pixels=np.array(Image.open(SOURCE).convert('RGB'))
    assert pixels.shape==(941,1672,3)
    foliage_base=np.array(Image.open(OUTPUT/'foliage_base.png').convert('RGB'))
    grass_base=np.array(Image.open(OUTPUT/'grass_base.png').convert('RGB'))
    patches=[];clouds=[]
    for i,box in enumerate(BOXES):
        mask=extract(pixels,box)
        patch,origin,removal=restored_patch(pixels,box,mask)
        patches.append(dict(image=patch,origin=origin))
        x0,y0,x1,y1=box
        # Silhueta completa, sem retângulo de céu acompanhando a nuvem.
        sprite_mask=ndimage.binary_dilation(mask,iterations=1)
        rgb=pixels[y0:y1,x0:x1]
        layer=np.zeros((y1-y0,x1-x0,4),np.uint8)
        layer[sprite_mask,:3]=rgb[sprite_mask];layer[sprite_mask,3]=255
        ys,xs=np.where(sprite_mask)
        ax,ay=max(0,int(xs.min())-2),max(0,int(ys.min())-2)
        bx,by=min(x1-x0,int(xs.max())+3),min(y1-y0,int(ys.max())+3)
        sprite=Image.fromarray(layer[ay:by,ax:bx])
        clouds.append(dict(image=sprite,origin=(x0+ax,y0+ay),seconds=SECONDS[i]))
    bad_mask=extract(pixels,BAD_BOX)
    correction,correction_origin,removed=restored_patch(pixels,BAD_BOX,bad_mask)
    correction.save(OUTPUT/'cloud_correction.png')
    assert not ((np.array(correction)[:,:,:3][:,:,0]>220)&(np.array(correction)[:,:,3]>0)).any()
    # A remoção baixa não toca o trabalho aprovado da folhagem ou da grama.
    ox,oy=correction_origin;w,h=correction.size
    assert not (removed&np.any(foliage_base[oy:oy+h,ox:ox+w]!=grass_base[oy:oy+h,ox:ox+w],axis=2)).any()
    occluders=[]
    for base in (pixels,foliage_base):
        rgb=base[:SKY_HEIGHT]
        r,g,b=[rgb[:,:,i].astype(float) for i in range(3)]
        mask=(b<100)|((r<140)&(g<170)&(b<g*.8))
        # Conserva a franja das folhas do próprio fundo, inclusive nos modos de fallback.
        mask=ndimage.binary_dilation(mask,iterations=1)&(r<230)
        layer=np.zeros((SKY_HEIGHT,1672,4),np.uint8)
        layer[mask,:3]=rgb[mask];layer[mask,3]=255
        occluders.append(dict(image=Image.fromarray(layer)))
    # Atlas único para limpeza do céu, nuvens e oclusão; correção baixa também tem cópia de segurança.
    extra=dict(image=correction,origin=correction_origin)
    cells=[*patches,*clouds,*occluders,extra]
    cx=cy=row=0
    for cell in sorted(cells,key=lambda c:c['image'].height,reverse=True):
        w,h=cell['image'].size
        if cx+w>2048:cy+=row;cx=row=0
        cell['source']=(cx,cy,w,h);cx+=w;row=max(row,h)
    atlas=Image.new('RGBA',(2048,cy+row))
    for cell in cells:atlas.paste(cell['image'],cell['source'][:2])
    atlas.save(OUTPUT/'cloud_atlas.png')
    header=['/* Nuvens e remoção localizada geradas por tools/prepare_versus_clouds.py. */',
            '#ifndef VERSUS_CLOUD_DATA_H','#define VERSUS_CLOUD_DATA_H','#include "raylib.h"',
            '#define VERSUS_CLOUD_COUNT 3','#define VERSUS_CLOUD_SECONDS 3072.0f',
            f'#define VERSUS_CLOUD_ATLAS_HEIGHT {atlas.height}',f'#define VERSUS_CLOUD_SKY_HEIGHT {SKY_HEIGHT}',
            f'#define VERSUS_CLOUD_CORRECTION_WIDTH {correction.width}',f'#define VERSUS_CLOUD_CORRECTION_HEIGHT {correction.height}',
            'typedef struct { Rectangle source; Vector2 origin; } VersusCloudPatch;',
            'typedef struct { Rectangle source; Vector2 origin; float seconds; } VersusCloud;',
            'static const VersusCloudPatch VERSUS_CLOUD_PATCHES[3] = {']
    rect=lambda values:'{'+', '.join(map(str,values))+'}'
    for patch in patches:header.append('    {'+rect(patch['source'])+', '+rect(patch['origin'])+'},')
    header+=['};','static const VersusCloud VERSUS_CLOUDS[3] = {']
    for cloud in clouds:header.append('    {'+rect(cloud['source'])+', '+rect(cloud['origin'])+f', {cloud["seconds"]}.0f'+'},')
    header+=['};','static const Rectangle VERSUS_CLOUD_OCCLUDERS[2] = {']
    for cell in occluders:header.append('    '+rect(cell['source'])+',')
    header+=['};','static const VersusCloudPatch VERSUS_CLOUD_CORRECTION = {'+rect(extra['source'])+', '+rect(correction_origin)+'};','#endif','']
    path=ROOT/'include/versus_cloud_data.h';text='\n'.join(header)
    if not path.exists() or path.read_text(encoding='utf-8')!=text:path.write_text(text,encoding='utf-8')
    # Antes/depois local, incluindo margem de gramado intacto.
    original=Image.fromarray(pixels).convert('RGBA')
    corrected=original.copy();corrected.alpha_composite(correction,correction_origin)
    original.crop((1315,195,1550,312)).resize((940,468),Image.Resampling.NEAREST).save(PREVIEW/'removed_before.png')
    corrected.crop((1315,195,1550,312)).resize((940,468),Image.Resampling.NEAREST).save(PREVIEW/'removed_after.png')
    corrected.save(PREVIEW/'corrected.png')
    report=dict(protected_hashes=hashes,removed_cloud_box=BAD_BOX,correction_origin=correction_origin,
                correction_size=correction.size,removed_pixels=int(removed.sum()),atlas_size=atlas.size,
                clouds=[{k:v for k,v in c.items() if k!='image'} for c in clouds])
    (PREVIEW/'inspection.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    for p in protected:assert hashlib.sha256(p.read_bytes()).hexdigest()==hashes[str(p.relative_to(ROOT))]
    print(f'Uma nuvem baixa removida; tres nuvens do ceu preservadas; atlas {atlas.size}; recursos aprovados intactos.')


if __name__=='__main__':prepare()
