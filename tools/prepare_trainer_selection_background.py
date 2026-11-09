"""Extrai vento e nuvens da seleção; preserva fonte, paleta e estruturas fixas."""
from pathlib import Path
import hashlib
import json
import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage
from scipy.spatial import cKDTree
from scipy import sparse
from scipy.sparse.linalg import spsolve
from prepare_menu_grass import signal

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'assets/sprites/trainer_selection_background.png'
OUTPUT = ROOT / 'assets/sprites/selection/background'
PREVIEW = ROOT / 'build/trainer_selection_preview'
WIND_SECONDS = 4
CLOUD_SECONDS = 128
WIDTH, HEIGHT = 1672, 941
ATLAS_WIDTH = 2048


def restore(pixels, removal, donors):
    # Ajuste local quadrático; quantização exclusivamente na paleta dos doadores.
    yy, xx = np.mgrid[:pixels.shape[0], :pixels.shape[1]].astype(float)
    xx /= pixels.shape[1]
    yy /= pixels.shape[0]
    basis = np.stack((xx*0+1, xx, yy, xx*xx, xx*yy, yy*yy), axis=-1)
    coefficients = np.linalg.lstsq(basis[donors], pixels[donors], rcond=None)[0]
    _, nearest = ndimage.distance_transform_edt(~donors, return_indices=True)
    predicted = basis @ coefficients
    texture = pixels[nearest[0], nearest[1]] - predicted[nearest[0], nearest[1]]
    palette = np.unique(pixels[donors], axis=0)
    _, indices = cKDTree(palette).query((predicted + texture*.5)[removal])
    result = pixels.copy()
    result[removal] = palette[indices]
    return result


def restore_sky(pixels, removal):
    # Interpolação harmônica da borda local evita a silhueta clara deixada pela nuvem.
    ys,xs=np.where(removal)
    indices=np.full(removal.shape,-1,int)
    indices[removal]=np.arange(len(ys))
    rows=list(range(len(ys)))
    columns=rows.copy()
    values=[4.0]*len(ys)
    rhs=np.zeros((len(ys),3))
    for dy,dx in ((-1,0),(1,0),(0,-1),(0,1)):
        neighbor=indices[ys+dy,xs+dx]
        inside=neighbor>=0
        rows.extend(np.flatnonzero(inside))
        columns.extend(neighbor[inside])
        values.extend([-1.0]*int(inside.sum()))
        rhs[~inside]+=pixels[ys[~inside]+dy,xs[~inside]+dx]
    matrix=sparse.csr_matrix((values,(rows,columns)),shape=(len(ys),len(ys)))
    colors=spsolve(matrix,rhs)
    # Preserva a paleta; nenhuma cor interpolada entra nos recursos finais.
    palette=np.unique(pixels[~removal],axis=0)
    _,nearest=cKDTree(palette).query(colors)
    result=pixels.copy()
    result[removal]=palette[nearest]
    return result


def prepare():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    PREVIEW.mkdir(parents=True, exist_ok=True)
    digest = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    original = Image.open(SOURCE).convert('RGB')
    assert original.size == (WIDTH, HEIGHT)
    pixels = np.array(original)
    r, g, b = [pixels[:, :, i].astype(float) for i in range(3)]
    yy, xx = np.mgrid[:HEIGHT, :WIDTH]
    wood = (r > g+7) & (r < 195) & (b < r*.9)
    wood_distance = ndimage.distance_transform_edt(~wood)
    green = (g > r+3) & (r < 125) & (b < 100)
    regions = []
    occupied = np.zeros((HEIGHT, WIDTH), bool)
    # Caixas de touceiras isoladas, sem rochas ou raízes.
    grass_boxes = [(22,766,90,808), (202,730,278,776), (91,679,154,707),
                   (39,580,105,623), (289,518,349,549), (491,459,544,486),
                   (649,489,693,516), (947,453,999,483), (1133,485,1190,515),
                   (1301,469,1355,498), (1397,495,1475,535), (1597,523,1640,549),
                   (1334,587,1382,620), (1542,710,1606,742), (1388,750,1440,786),
                   (356,473,390,494)]
    selections = []
    for i, box in enumerate(grass_boxes):
        selections.append((f'grass_{i:02d}', box, 'grass', 4 if box[1] > 570 else 2))
    selections += [('canopy_left', (0,0,335,278), 'canopy', 5),
                   ('canopy_right', (1330,0,1672,264), 'canopy', 5),
                   ('distant_left', (327,305,448,446), 'distant', 2),
                   ('distant_center', (797,306,1023,447), 'distant', 2),
                   ('distant_right', (1178,333,1303,446), 'distant', 2)]
    annotation = original.copy()
    draw = ImageDraw.Draw(annotation)
    for name, box, kind, amplitude in selections:
        x0,y0,x1,y1 = box
        mask = np.zeros_like(occupied)
        mask[y0:y1,x0:x1] = green[y0:y1,x0:x1]
        mask &= ~occupied
        assert mask.any(), name
        occupied |= mask
        if kind == 'grass':
            base = np.where(mask)[0].max()
            weight = np.clip((base-5-yy)/max(1,base-y0-5),0,1)**1.3
            weight *= np.clip((wood_distance-6)/8,0,1)
        elif kind == 'canopy':
            # Ancoragem na madeira e na borda: nenhum tronco, raiz ou base acompanha o vento.
            weight = np.clip((wood_distance-12)/70,0,1)
            edge = np.minimum.reduce((xx,WIDTH-1-xx,yy))
            weight *= np.clip(edge/22,0,1)
        else:
            weight = np.clip((y1-12-yy)/60,0,1)
            weight *= np.clip(np.minimum(xx-x0,x1-1-xx)/25,0,1)
        # Margens transparentes recebem somente folhas; nunca retângulos opacos.
        origin = (max(0,x0-8),max(0,y0-2))
        ox,oy = origin
        w,h = min(WIDTH,x1+8)-ox,min(HEIGHT,y1+2)-oy
        local = mask[oy:oy+h,ox:ox+w]
        layer = np.zeros((h,w,4),np.uint8)
        layer[local,:3] = pixels[oy:oy+h,ox:ox+w][local]
        layer[local,3] = 255
        ly,lx = np.mgrid[:h,:w]
        poses = []
        exposed = np.zeros_like(local)
        local_weight = weight[oy:oy+h,ox:ox+w]
        for step in range(amplitude+1):
            sx = lx-np.rint(step*local_weight).astype(int)
            pose = layer[ly,np.clip(sx,0,w-1)].copy()
            pose[(sx<0)|(sx>=w)] = 0
            pose[local_weight == 0] = layer[local_weight == 0]
            exposed |= local & (pose[:,:,3] == 0)
            poses.append(Image.fromarray(pose))
        phase = x0/WIDTH*1.45+y0/HEIGHT*.35
        frames = [round(amplitude*signal(f,phase)) for f in range(32)]
        assert max(abs(np.roll(frames,-1)-frames)) <= 1
        region = dict(name=name,kind=kind,origin=origin,size=(w,h),frames=frames,
                      poses=poses,exposed=exposed,mask=mask,box=box)
        regions.append(region)
        Image.fromarray(np.uint8(mask)*255).save(PREVIEW/f'{name}_mask.png')
        draw.rectangle(box,outline='red')
        draw.text((x0+3,y0+3),name,fill='white',stroke_width=1,stroke_fill='black')
    background = pixels.copy()
    changed = np.zeros_like(occupied)
    for region in regions:
        ox,oy = region['origin']
        w,h = region['size']
        removal = ndimage.binary_dilation(region['exposed'],iterations=2)
        # Guardas fixas impedem clareamento junto a troncos e bases.
        mask = region['mask'][oy:oy+h,ox:ox+w]
        removal &= (~mask | (wood_distance[oy:oy+h,ox:ox+w]>12))
        if region['kind'] == 'grass':
            removal &= np.mgrid[oy:oy+h,ox:ox+w][0] < np.where(region['mask'])[0].max()-5
            removal &= wood_distance[oy:oy+h,ox:ox+w]>6
        removal &= ~wood[oy:oy+h,ox:ox+w]
        if not removal.any():
            continue
        ax,ay,bx,by = max(0,ox-32),max(0,oy-32),min(WIDTH,ox+w+32),min(HEIGHT,oy+h+32)
        patch = background[ay:by,ax:bx]
        local_remove = np.zeros(patch.shape[:2],bool)
        local_remove[oy-ay:oy-ay+h,ox-ax:ox-ax+w] = removal
        if region['kind'] == 'distant':
            # O céu atrás do contorno superior, nunca outra silhueta da floresta.
            donors = (pixels[ay:by,ax:bx,0]>125) & ~occupied[ay:by,ax:bx]
        else:
            donors = (pixels[ay:by,ax:bx,0]>125) & ~occupied[ay:by,ax:bx] & ~wood[ay:by,ax:bx]
        donors &= ~ndimage.binary_dilation(local_remove,iterations=3)
        assert donors.any(),region['name']
        patch[:] = restore(patch,local_remove,donors)
        changed[ay:by,ax:bx] |= local_remove
    # Nuvens completas e franja clara removidas; a restauração usa somente céu local.
    for i,box in enumerate(((226,165,378,224),(1292,220,1471,282))):
        x0,y0,x1,y1=box
        mask=(r[y0:y1,x0:x1]>175)&(g[y0:y1,x0:x1]>170)&(b[y0:y1,x0:x1]>110)
        layer=np.zeros((y1-y0,x1-x0,4),np.uint8)
        layer[mask,:3]=pixels[y0:y1,x0:x1][mask]
        layer[mask,3]=255
        regions.append(dict(name=f'cloud_{i}',kind='cloud',origin=(x0,y0),size=(x1-x0,y1-y0),poses=[Image.fromarray(layer)],frames=[0]*32,box=box))
        margin=35
        ax,ay,bx,by=x0-margin,y0-margin,x1+margin,y1+margin
        removal=np.zeros((by-ay,bx-ax),bool)
        removal[margin:margin+y1-y0,margin:margin+x1-x0]=mask
        removal=ndimage.binary_dilation(removal,iterations=10)
        patch=background[ay:by,ax:bx]
        patch[:]=restore_sky(patch,removal)
        changed[ay:by,ax:bx]|=removal
    # Oclusão fixa entre nuvens e folhas: as nuvens passam atrás das árvores.
    br,bg,bb=[background[:,:,i].astype(float) for i in range(3)]
    occlusion=(((bg>br+3)&(br<125)&(bb<100))|wood)[:300]
    foreground=np.zeros((300,WIDTH,4),np.uint8)
    foreground[occlusion,:3]=background[:300][occlusion]
    foreground[occlusion,3]=255
    occluder=dict(name='cloud_occlusion',kind='occluder',origin=(0,0),size=(WIDTH,300),
                  poses=[Image.fromarray(foreground)],frames=[0]*32,box=(0,0,WIDTH,300))
    regions=[region for region in regions if region['kind']=='cloud']+[occluder]+[
        region for region in regions if region['kind']!='cloud']
    # Empacotamento de poses únicas, como nas copas do menu.
    cursor_x=cursor_y=row_height=0
    cells=[(region,i,pose) for region in regions for i,pose in enumerate(region['poses'])]
    for region,i,pose in sorted(cells,key=lambda item:item[2].height,reverse=True):
        if cursor_x+pose.width>ATLAS_WIDTH:
            cursor_y+=row_height
            cursor_x=row_height=0
        region.setdefault('sources',{})[i]=(cursor_x,cursor_y,pose.width,pose.height)
        cursor_x+=pose.width
        row_height=max(row_height,pose.height)
    atlas=Image.new('RGBA',(ATLAS_WIDTH,cursor_y+row_height))
    for region,i,pose in cells:
        atlas.paste(pose,region['sources'][i][:2])
    atlas.save(OUTPUT/'atlas.png')
    Image.fromarray(background).save(OUTPUT/'base.png')
    annotation.save(PREVIEW/'regions.png')
    Image.fromarray(background).save(PREVIEW/'reconstructed.png')
    Image.fromarray(np.uint8(changed)*255).save(PREVIEW/'reconstruction_mask.png')
    assert np.array_equal(background[~changed],pixels[~changed])
    assert np.array_equal(background[wood],pixels[wood])
    packed=lambda data: data[:,:,0].astype(np.uint32)*65536+data[:,:,1].astype(np.uint32)*256+data[:,:,2]
    assert np.isin(np.unique(packed(background)),np.unique(packed(pixels))).all()
    assert set(np.unique(np.array(atlas)[:,:,3])) <= {0,255}
    header=['/* Poses e fases geradas pelo preparador do fundo da seleção. */',
            '#ifndef TRAINER_BACKGROUND_DATA_H','#define TRAINER_BACKGROUND_DATA_H','#include "raylib.h"',
            f'#define TRAINER_BACKGROUND_REGIONS {len(regions)}',f'#define TRAINER_BACKGROUND_ATLAS_HEIGHT {atlas.height}',
            'typedef struct { Vector2 origin; Rectangle poses[6]; unsigned char frames[32]; bool cloud; } TrainerBackgroundRegion;',
            'static const TrainerBackgroundRegion TRAINER_BACKGROUND_DATA[TRAINER_BACKGROUND_REGIONS] = {']
    for region in regions:
        poses=', '.join('{'+','.join(map(str,region['sources'][i]))+'}' for i in range(len(region['poses'])))
        frames=','.join(map(str,region['frames']))
        origin=','.join(map(str,region['origin']))
        header.append(f'    {{{{{origin}}}, {{{poses}}}, {{{frames}}}, {"true" if region["kind"]=="cloud" else "false"}}}, /* {region["name"]} */')
    header+=['};','#endif','']
    (ROOT/'include/trainer_background_data.h').write_text('\n'.join(header),encoding='utf-8')
    report=dict(source_sha256=digest,atlas_size=atlas.size,wind_seconds=WIND_SECONDS,cloud_seconds=CLOUD_SECONDS,
                outside_reconstruction_unchanged=True,wood_unchanged=True,palette_preserved=True,
                regions=[{k:v for k,v in region.items() if k not in ('poses','mask','exposed')} for region in regions])
    (PREVIEW/'inspection.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==digest
    print(f'{len(regions)} regiões; atlas {atlas.size}; fonte preservada.')


if __name__ == '__main__':
    prepare()
