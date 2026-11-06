"""Prepara vento somente nas touceiras do Versus, preservando a fonte aprovada."""
from pathlib import Path
import hashlib
import json
import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage
from prepare_menu_grass import align, reconstruct, signal

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'assets/sprites/versus_background_generated_v1.png'
OUTPUT = ROOT / 'assets/sprites/versus'
PREVIEW = ROOT / 'build/versus_grass_preview'
FRAMES, COLUMNS, PAGE_WIDTH = 32, 8, 512
# Caixas de grama isolada; pedras, arbustos e raízes de árvores ficam fora delas.
BOXES = [
    (173,200,302,260), (502,218,553,243), (741,223,793,249),
    (1095,180,1169,218), (1218,281,1272,308), (366,327,423,358),
    (324,468,383,499), (935,426,988,456), (1270,455,1327,484),
    (1401,495,1479,535), (1593,514,1663,551),
    (558,603,612,632), (1111,602,1170,632), (90,677,153,707),
    (201,733,275,776), (18,766,89,809), (482,820,542,851),
    (1087,805,1153,845), (1389,750,1446,785), (1543,708,1610,744),
]


def prepare():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    PREVIEW.mkdir(parents=True, exist_ok=True)
    digest = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    original = Image.open(SOURCE).convert('RGB')
    assert original.size == (1672,941)
    pixels = np.array(original)
    background = pixels.copy()
    removal_all = np.zeros(pixels.shape[:2], bool)
    regions = []
    annotated = original.copy()
    draw = ImageDraw.Draw(annotated)
    for i, box in enumerate(BOXES):
        x0,y0,x1,y1 = box
        patch = pixels[y0:y1,x0:x1]
        r,g,b = [patch[:,:,c].astype(float) for c in range(3)]
        # A cor separa as lâminas do chão; nenhuma área retangular entra na camada.
        mask = (r < 132) & (g > r+15) & (b < 100)
        labels, _ = ndimage.label(mask)
        sizes = np.bincount(labels.ravel())
        ids = np.flatnonzero(sizes >= 8)
        borders = np.unique(np.concatenate((labels[0],labels[-1],labels[:,0],labels[:,-1])))
        ids = ids[(ids != 0) & ~np.isin(ids,borders)]
        mask &= np.isin(labels,ids)
        assert mask.any(), i
        assert not (mask[0].any() or mask[-1].any() or mask[:,0].any() or mask[:,-1].any()), i
        ys,xs = np.where(mask)
        base = int(ys.max())+y0
        origin = (x0+int(xs.min())-8, y0+int(ys.min())-4)
        size = (align(int(xs.max()-xs.min()+1)+16), align(int(ys.max()-ys.min()+1)+8))
        layer = np.zeros((size[1],size[0],4), np.uint8)
        layer[ys+y0-origin[1],xs+x0-origin[0],:3] = patch[ys,xs]
        layer[ys+y0-origin[1],xs+x0-origin[0],3] = 255
        area, restored, removal = reconstruct(pixels,mask,box,base)
        ax,ay,bx,by = area
        background[ay:by,ax:bx][removal] = restored[removal]
        removal_all[ay:by,ax:bx] |= removal
        phase = origin[0]/1672*2.1 + (i%4)*.49
        amplitude = 3.5 if size[1] > 45 else 2.2
        region = dict(origin=origin,size=size,base=base,box=box,phase=phase,
                      amplitude=amplitude,fps=(8,7,6)[i%3],layer=layer)
        regions.append(region)
        Image.fromarray(layer).save(PREVIEW / f'grass_{i:02d}.png')
        draw.rectangle(box,outline='#ff722b',width=2)
        draw.text((x0,y0-12),str(i),fill='white',stroke_width=1,stroke_fill='black')
    assert np.array_equal(background[~removal_all],pixels[~removal_all])
    # A reconstrução não pode conservar uma segunda silhueta escura da grama.
    assert (background[removal_all,0] > 85).all()
    Image.fromarray(background).save(OUTPUT / 'grass_base.png')
    Image.fromarray(removal_all.astype(np.uint8)*255).save(PREVIEW / 'removal_mask.png')
    annotated.save(PREVIEW / 'regions.png')
    cx=cy=row=0
    for region in sorted(regions,key=lambda r:r['size'][1],reverse=True):
        w,h=region['size']
        if cx+w > PAGE_WIDTH:
            cy+=row
            cx=row=0
        region['source']=(cx,cy,w,h)
        cx+=w
        row=max(row,h)
    page_height=align(cy+row)
    atlas=Image.new('RGBA',(PAGE_WIDTH*COLUMNS,page_height*(FRAMES//COLUMNS)))
    for region in regions:
        layer=region['layer']
        h,w=layer.shape[:2]
        yy,xx=np.mgrid[:h,:w]
        base=region['base']-region['origin'][1]
        weight=np.clip((base-4-yy)/max(base-5,1),0,1)**1.7
        poses=[]
        components=ndimage.label(layer[:,:,3]>0,structure=np.ones((3,3)))[1]
        for f in range(FRAMES):
            dx=np.rint(region['amplitude']*signal(f,region['phase'])*weight).astype(int)
            sx=xx-dx
            valid=(sx>=0)&(sx<w)
            animated=np.zeros_like(layer)
            animated[valid]=layer[yy[valid],sx[valid]]
            assert np.array_equal(animated[base-4:],layer[base-4:])
            assert not (animated[:,0,3].any() or animated[:,-1,3].any())
            assert np.array_equal(np.sort(animated[animated[:,:,3]>0,:3],axis=0),
                                  np.sort(layer[layer[:,:,3]>0,:3],axis=0))
            assert ndimage.label(animated[:,:,3]>0,structure=np.ones((3,3)))[1] == components
            poses.append(animated)
            x,y,_,_=region['source']
            atlas.paste(Image.fromarray(animated),(x+(f%COLUMNS)*PAGE_WIDTH,y+(f//COLUMNS)*page_height))
        offsets=np.rint([region['amplitude']*signal(f,region['phase']) for f in range(FRAMES)])
        assert np.abs(offsets-np.roll(offsets,1)).max() <= 1
        region['poses']=poses
    atlas.save(OUTPUT / 'grass_atlas.png')
    # Prévia temporal da composição exata em pixels da fonte.
    scenes=[]
    for f in range(128):
        scene=Image.fromarray(background).convert('RGBA')
        for region in regions:
            pose=int((f/16)*region['fps'])%FRAMES
            scene.alpha_composite(Image.fromarray(region['poses'][pose]),region['origin'])
        scenes.append(scene.convert('RGB'))
    small=[s.resize((1280,720),Image.Resampling.NEAREST) for s in scenes]
    palette=small[0].quantize(colors=256)
    indexed=[s.quantize(palette=palette,dither=Image.Dither.NONE) for s in small]
    indexed[0].save(PREVIEW/'scene.gif',save_all=True,append_images=indexed[1:],duration=62.5,loop=0)
    # Contato ampliado de momentos diferentes, incluindo a passagem entre poses.
    contact=Image.new('RGB',(1200,5*160),'#243b28')
    d=ImageDraw.Draw(contact)
    for row,i in enumerate((0,3,13,15,19)):
        x0,y0,x1,y1=regions[i]['box']
        for col,f in enumerate((0,17,37,63,97)):
            crop=scenes[f].crop((x0-5,y0-5,x1+6,y1+6))
            scale=min(230/crop.width,130/crop.height)
            crop=crop.resize((round(crop.width*scale),round(crop.height*scale)),Image.Resampling.NEAREST)
            contact.paste(crop,(col*240,row*160+22))
            d.text((col*240,row*160),f'Touceira {i} / {f/16:.2f}s',fill='white')
    contact.save(PREVIEW/'closeup.png')
    header=['/* Touceiras e atlas gerados por tools/prepare_versus_grass.py. */',
            '#ifndef VERSUS_GRASS_DATA_H','#define VERSUS_GRASS_DATA_H','#include "raylib.h"',
            f'#define VERSUS_GRASS_COUNT {len(regions)}',f'#define VERSUS_GRASS_FRAMES {FRAMES}',
            f'#define VERSUS_GRASS_COLUMNS {COLUMNS}',f'#define VERSUS_GRASS_PAGE_WIDTH {PAGE_WIDTH}',
            f'#define VERSUS_GRASS_PAGE_HEIGHT {page_height}',
            'typedef struct { Rectangle source; Vector2 origin; int fps; } VersusGrassRegion;',
            'static const VersusGrassRegion VERSUS_GRASS_REGIONS[VERSUS_GRASS_COUNT] = {']
    for region in regions:
        source=', '.join(map(str,region['source']))
        origin=', '.join(map(str,region['origin']))
        header.append(f'    {{{{{source}}}, {{{origin}}}, {region["fps"]}}},')
    header+=['};','#endif','']
    header_path=ROOT/'include/versus_grass_data.h'
    header_text='\n'.join(header)
    if not header_path.exists() or header_path.read_text(encoding='utf-8') != header_text:
        header_path.write_text(header_text,encoding='utf-8')
    report=dict(source_sha256=digest,region_count=len(regions),atlas_size=atlas.size,
                changed_background_pixels=int(removal_all.sum()),outside_mask_unchanged=True,
                fixed_root_rows=5,maximum_tip_step=1,period_seconds=32,
                regions=[{k:v for k,v in r.items() if k not in ('layer','poses')} for r in regions])
    (PREVIEW/'inspection.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==digest
    print(f'{len(regions)} touceiras, atlas {atlas.size}, {removal_all.sum()} pixels reconstruídos.')


if __name__ == '__main__':
    prepare()
