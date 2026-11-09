"""Estende o vento do menu com copas extraídas da mesma fonte, sem alterar a grama."""
import argparse
import hashlib
import json

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

import prepare_menu_grass as grass

ROOT = grass.ROOT
PREVIEW = ROOT / 'build/menu_tree_preview'
BACKGROUND = ROOT / 'assets/sprites/menu_wind_background.png'
ATLAS = ROOT / 'assets/sprites/menu_tree_atlas.png'
ATLAS_WIDTH = 2048
# Contornos de seleção; o recorte final depende das cores e dos objetos protegidos.
FOREGROUND = [
    ('foreground_left_upper', [(0,0),(270,0),(284,24),(254,53),(298,47),(326,66),(340,92),(370,99),(403,146),(403,190),(367,202),(339,200),(330,240),(301,267),(259,260),(242,289),(204,289),(198,279),(178,284),(150,282),(150,241),(102,252),(97,293),(63,305),(0,305)],
     [(92,195,150,.8),(170,198,155,1),(275,214,120,.9)]),
    ('foreground_right_upper', [(1440,0),(1672,0),(1672,369),(1600,369),(1590,352),(1560,356),(1530,330),(1480,340),(1450,333),(1400,330),(1370,315),(1320,315),(1280,294),(1260,280),(1265,241),(1280,211),(1310,202),(1315,171),(1340,155),(1350,137),(1390,130),(1390,98),(1400,75),(1400,45)],
     [(1590,174,170,.8),(1490,277,165,1),(1385,288,130,.9)]),
    ('foreground_left_lower', [(0,525),(70,550),(95,579),(98,605),(160,607),(188,628),(230,651),(230,708),(200,730),(190,767),(170,790),(184,815),(216,814),(228,827),(276,810),(300,815),(326,844),(354,850),(374,876),(407,891),(415,928),(451,941),(0,941)],
     [(100,742,160,1),(139,831,110,.8),(230,934,155,1)]),
    ('foreground_right_lower', [(1672,590),(1615,598),(1600,623),(1560,641),(1550,673),(1500,679),(1460,706),(1440,737),(1410,744),(1410,789),(1424,801),(1450,807),(1450,830),(1380,820),(1375,839),(1345,846),(1315,868),(1295,908),(1270,916),(1270,941),(1672,941)],
     [(1570,820,185,1),(1480,912,170,.85),(1640,920,130,.9)]),
]
DISTANT = [
    ('distant_left_small', (356,46,427,86), (390,79)),
    ('distant_left', (393,6,501,81), (448,57)),
    ('distant_left_large', (495,0,683,65), (585,58)),
    ('distant_middle_left', (647,21,756,85), (705,71)),
    ('distant_middle_right', (903,0,1036,72), (970,62)),
    ('distant_right_large', (1008,0,1205,65), (1101,51)),
    ('distant_right', (1179,29,1274,85), (1220,75)),
    ('distant_right_edge', (1302,38,1411,90), (1351,83)),
]
TIP_CROPS = {
    'foreground_left_upper': (252,130,409,270),
    'foreground_right_upper': (1255,180,1435,325),
    'foreground_left_lower': (95,630,245,785),
    'foreground_right_lower': (1390,665,1580,825),
}


def polygon(points, size):
    image = Image.new('1', size)
    ImageDraw.Draw(image).polygon(points, fill=1)
    return np.array(image, dtype=bool)


def extract(pixels, grass_mask):
    height, width = pixels.shape[:2]
    r, g, b = [pixels[:,:,i].astype(float) for i in range(3)]
    regions = []
    foreground_mask = np.zeros((height,width), dtype=bool)
    for name, points, clusters in FOREGROUND:
        area = ndimage.binary_dilation(polygon(points, (width,height)), iterations=9)
        mask = area & (g > r+4) & (b < g*.70) & ((r < 115) | (b < 58)) & ~grass_mask
        mask = ndimage.binary_fill_holes(mask) & area & (g > r+4) & ~grass_mask
        foreground_mask |= mask
        regions.append(dict(name=name, mask=mask, clusters=clusters, steps=6, distant=False))
    for name, box, anchor in DISTANT:
        x0,y0,x1,y1 = box
        area = np.zeros((height,width), dtype=bool)
        area[y0:y1,x0:x1] = True
        mask = area & ~foreground_mask & (g > r+4) & (g > b+15) & ((r < 125) | (b < 58))
        mask = ndimage.binary_fill_holes(mask) & area & ~foreground_mask & (g > r+4)
        # Mantém a copa ligada ao centro escolhido, descartando arbustos de fundo.
        labels, _ = ndimage.label(mask)
        center_x,center_y = anchor
        seeds = labels[max(0,center_y-25):center_y,center_x-16:center_x+16]
        ids = np.unique(seeds)
        mask &= np.isin(labels, ids[ids != 0])
        regions.append(dict(name=name, mask=mask, clusters=[(*anchor,65,1)], steps=2, distant=True, clip_box=box))
    # Cada pixel da fonte pertence a uma única copa, inclusive nas sobreposições distantes.
    occupied = np.zeros((height,width), dtype=bool)
    for region in regions:
        region['mask'] &= ~occupied
        occupied |= region['mask']
        assert region['mask'].any(), region['name']
    return regions, occupied


def prepare():
    PREVIEW.mkdir(parents=True, exist_ok=True)
    original = Image.open(grass.SOURCE).convert('RGB')
    pixels = np.array(original)
    assert original.size == (1672,941)
    source_hash = hashlib.sha256(grass.SOURCE.read_bytes()).hexdigest()
    grass_hash = hashlib.sha256(grass.ATLAS.read_bytes()).hexdigest()
    # O derivado parte do chão já preparado, nunca de uma segunda restauração independente.
    ground = np.array(Image.open(grass.BACKGROUND).convert('RGB'))
    grass_report = json.loads((grass.PREVIEW/'inspection.json').read_text(encoding='utf-8'))
    assert grass_report['source_sha256'] == source_hash
    grass_mask = np.zeros(pixels.shape[:2], dtype=bool)
    for name, box, _ in grass.REGIONS:
        x0,y0,x1,y1 = box
        grass_mask[y0:y1,x0:x1] |= grass.extract(pixels,name,box)
    grass_changed = np.any(ground != pixels, axis=2)
    # Touceiras parcialmente encobertas pela árvore esquerda continuam estáticas.
    static_grass = np.zeros(pixels.shape[:2],dtype=bool)
    static_grass[455:550,0:210] = True
    static_grass[750:832,165:306] = True
    protected_grass = ndimage.binary_dilation(grass_mask | grass_changed | static_grass, iterations=10)
    regions, all_foliage = extract(pixels, protected_grass)
    r,g,b = [pixels[:,:,i].astype(float) for i in range(3)]
    # Madeira, raízes e pedras permanecem fixas; a faixa de guarda ancora as folhas.
    wood = (r > g+5) & (b < r*.85)
    structural = wood | protected_grass
    branch_distance = ndimage.distance_transform_edt(~structural)
    height,width = pixels.shape[:2]
    yy,xx = np.mgrid[:height,:width]
    screen_edge = np.minimum.reduce((xx, width-1-xx, yy, height-1-yy))
    edge_guard = np.clip(screen_edge/18,0,1)
    exposed = np.zeros((height,width),dtype=bool)
    motion_mask = np.zeros_like(exposed)
    annotation = original.copy()
    draw = ImageDraw.Draw(annotation)
    for region in regions:
        mask = region['mask']
        ys,xs = np.where(mask)
        x0,y0 = max(0,int(xs.min())-10),max(0,int(ys.min())-2)
        x1,y1 = min(width,int(xs.max())+12),min(height,int(ys.max())+3)
        region['origin'] = (x0,y0)
        region['size'] = (grass.align(x1-x0),grass.align(y1-y0))
        region['selection'] = (x0,y0,x1,y1)
        w,h = region['size']
        layer = np.zeros((h,w,4),dtype=np.uint8)
        ly,lx = ys-y0,xs-x0
        layer[ly,lx,:3] = pixels[ys,xs]
        layer[ly,lx,3] = 255
        # Campos de três aglomerados se sobrepõem suavemente, sem cortes móveis entre eles.
        weight = np.zeros((height,width),dtype=float)
        numerator = np.zeros_like(weight)
        denominator = np.zeros_like(weight)
        for ax,ay,radius,strength in region['clusters']:
            distance = np.hypot(xx-ax,yy-ay)
            influence = np.exp(-(distance/radius)**2*1.5)
            numerator += influence*np.clip(distance/radius,0,1)**1.5*strength
            denominator += influence
        weight = numerator/np.maximum(denominator,1e-10)
        guard = 5 if region['distant'] else 12
        weight *= np.clip((branch_distance-guard)/(10 if region['distant'] else 38),0,1)**1.3
        neighbors = all_foliage & ~mask
        neighbor_distance = ndimage.distance_transform_edt(~neighbors)
        weight *= np.clip((neighbor_distance-8)/14,0,1)
        weight *= edge_guard
        if region['distant']:
            # Uma interseção com o horizonte fica fixa, sem arrastar arbustos ou raízes.
            weight *= np.clip((region['clip_box'][3]-1-yy)/7,0,1)
        region['phase'] = x0/1672*1.45 + y0/941*.35
        region['poses'] = []
        local_weight = np.zeros((h,w))
        local_weight[:y1-y0,:x1-x0] = weight[y0:y1,x0:x1]
        y,x = np.mgrid[:h,:w]
        local_exposed = np.zeros((h,w),dtype=bool)
        fixed = local_weight == 0
        maximum_displacement = 0
        for step in range(region['steps']+1):
            dx = np.rint(step*local_weight).astype(int)
            maximum_displacement = max(maximum_displacement,int(dx[layer[:,:,3]>0].max()))
            sx = x-dx
            valid = (sx>=0)&(sx<w)
            pose = np.zeros_like(layer)
            pose[valid] = layer[y[valid],sx[valid]]
            # Preserva os pixels junto a madeira/objetos e a borda da janela.
            pose[fixed] = layer[fixed]
            local_exposed |= (layer[:,:,3]>0)&(pose[:,:,3]==0)
            assert np.array_equal(pose[fixed],layer[fixed])
            changed = np.any(pose != layer,axis=2)
            motion_mask[y0:y1,x0:x1] |= changed[:y1-y0,:x1-x0]
            region['poses'].append(Image.fromarray(pose))
        assert maximum_displacement > 0, region['name']
        assert not np.array_equal(np.array(region['poses'][0]),np.array(region['poses'][-1])), region['name']
        region['maximum_displacement'] = maximum_displacement
        exposed[y0:y1,x0:x1] |= local_exposed[:y1-y0,:x1-x0]
        values = [grass.signal(f,region['phase']) for f in range(grass.FRAME_COUNT)]
        if region['distant']:
            # A inércia da copa distante suaviza a rajada comum, sem outro relógio.
            values = [sum(grass.signal((f-j)%32,region['phase']) for j in range(7))/7 for f in range(32)]
        region['frames'] = np.rint(np.array(values)*region['steps']).astype(int).tolist()
        assert max(abs(np.roll(region['frames'],-1)-region['frames'])) <= 1
        Image.fromarray(np.uint8(mask)*255).save(PREVIEW/f'{region["name"]}_mask.png')
        Image.fromarray(np.uint8(np.clip(weight,0,1)*255)).save(PREVIEW/f'{region["name"]}_weight.png')
        draw.rectangle((x0,y0,x1,y1),outline='#ed507f',width=2)
        draw.text((x0+3,y0+3),region['name'],fill='white',stroke_width=1,stroke_fill='black')
    # A fonte tem uma franja de cor mista de 1–2 pixels: remove a franja antiga
    # somente no contorno exposto, para não deixar uma segunda silhueta imóvel.
    exposed = ndimage.binary_dilation(exposed,iterations=2) & ~protected_grass & ~wood
    assert not (exposed & protected_grass).any()
    assert not (exposed & wood).any()
    assert not (motion_mask & structural).any(), 'Movimento invadiu madeira ou grama.'
    # Só o contorno que chega a ficar exposto precisa de chão/céu por baixo.
    donors = ~ndimage.binary_dilation(all_foliage,iterations=2) & ~structural & (g > r+2)
    _, nearest = ndimage.distance_transform_edt(~donors,return_indices=True)
    background = ground.copy()
    background[exposed] = ground[nearest[0][exposed],nearest[1][exposed]]
    assert np.array_equal(background[~exposed],ground[~exposed])
    assert np.array_equal(background[grass_changed],ground[grass_changed])
    Image.fromarray(background).save(BACKGROUND)
    Image.fromarray(np.uint8(exposed)*255).save(PREVIEW/'exposed_mask.png')
    Image.fromarray(np.uint8(motion_mask | exposed)*255).save(PREVIEW/'animated_pixels.png')
    Image.fromarray(background).save(PREVIEW/'reconstructed.png')
    annotation.save(PREVIEW/'regions.png')
    # Poses únicas por copa, em vez de copiar 32 telas ou grandes áreas imóveis.
    cells = [(region,i,pose) for region in regions for i,pose in enumerate(region['poses'])]
    cursor_x=cursor_y=row_height=0
    for region,i,pose in sorted(cells,key=lambda item:item[2].height,reverse=True):
        w,h = pose.size
        if cursor_x+w > ATLAS_WIDTH:
            cursor_y += row_height
            cursor_x=row_height=0
        region.setdefault('sources',{})[i] = (cursor_x,cursor_y,w,h)
        cursor_x += w
        row_height=max(row_height,h)
    atlas_height=grass.align(cursor_y+row_height)
    atlas=Image.new('RGBA',(ATLAS_WIDTH,atlas_height))
    for region,i,pose in cells:
        x,y,_,_=region['sources'][i]
        atlas.paste(pose,(x,y))
    atlas.save(ATLAS)
    header=['/* Copas, poses e fases geradas por tools/prepare_menu_trees.py. */',
            '#ifndef MENU_TREE_DATA_H','#define MENU_TREE_DATA_H','#include "raylib.h"',
            f'#define MENU_TREE_REGION_COUNT {len(regions)}',f'#define MENU_TREE_ATLAS_WIDTH {ATLAS_WIDTH}',
            f'#define MENU_TREE_ATLAS_HEIGHT {atlas_height}',
            'typedef struct { Vector2 origin; Rectangle poses[7]; unsigned char frames[32]; } MenuTreeRegion;',
            'static const MenuTreeRegion MENU_TREE_REGIONS[MENU_TREE_REGION_COUNT] = {']
    for region in regions:
        poses=', '.join('{'+', '.join(map(str,region['sources'][i]))+'}' for i in range(region['steps']+1))
        frames=', '.join(map(str,region['frames']))
        origin=', '.join(map(str,region['origin']))
        header.append(f'    {{{{{origin}}}, {{{poses}}}, {{{frames}}}}}, /* {region["name"]} */')
    header += ['};','#endif','']
    (ROOT/'include/menu_tree_data.h').write_text('\n'.join(header),encoding='utf-8')
    report={'source_sha256':source_hash,'grass_atlas_sha256':grass_hash,'atlas_size':atlas.size,
            'tree_rgba_bytes':atlas.width*atlas.height*4,'grass_rgba_bytes':grass_report['atlas_rgba_bytes'],
            'exposed_pixels':int(exposed.sum()),'outside_exposed_unchanged':True,
            'regions':[{k:v for k,v in region.items() if k not in ('mask','poses')} for region in regions]}
    (PREVIEW/'inspection.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    scenes=[]
    grass_atlas=Image.open(grass.ATLAS).convert('RGBA')
    for frame in range(32):
        scene=Image.fromarray(background).convert('RGBA')
        # Copas antes da grama: o recorte e a guarda impedem sobreposição com suas bases.
        for region in regions:
            scene.alpha_composite(region['poses'][region['frames'][frame]],region['origin'])
        assert np.array_equal(np.array(scene)[:,:,:3][structural],ground[structural]), 'Estrutura ou grama alterada.'
        for region in grass_report['regions']:
            x,y,w,h=region['source']
            x+=(frame%8)*grass_report['page_size'][0]
            y+=(frame//8)*grass_report['page_size'][1]
            scene.alpha_composite(grass_atlas.crop((x,y,x+w,y+h)),tuple(region['origin']))
        scenes.append(scene.convert('RGB'))
    save_previews(scenes,report,'scene')
    assert hashlib.sha256(grass.ATLAS.read_bytes()).hexdigest()==grass_hash
    assert hashlib.sha256(grass.SOURCE.read_bytes()).hexdigest()==source_hash
    print(f'{len(regions)} copas; atlas {atlas.size}; {atlas.width*atlas.height*4/1048576:.2f} MiB; {exposed.sum()} pixels expostos')


def save_previews(scenes,report,prefix):
    shown=[s.resize((1280,720),Image.Resampling.NEAREST) if s.size!=(1280,720) else s for s in scenes]
    palette=shown[0].quantize(colors=256)
    indexed=[s.quantize(palette=palette,dither=Image.Dither.NONE) for s in shown]
    indexed[0].save(PREVIEW/f'{prefix}.gif',save_all=True,append_images=indexed[1:],duration=125,loop=0)
    for kind in ('foreground','distant','foreground_closeup'):
        regions=[r for r in report['regions'] if r['name'].startswith(kind.split('_')[0])]
        contact=Image.new('RGB',(1280,len(regions)*230),'#263b26')
        draw=ImageDraw.Draw(contact)
        for row,region in enumerate(regions):
            x0,y0,x1,y1=TIP_CROPS[region['name']] if kind=='foreground_closeup' else region['selection']
            if prefix=='title_game':
                scale=1280/1672
                x0,y0,x1,y1=[round(v*scale) for v in (x0,y0,x1,y1)]
            maximum=int(np.argmax(region['frames']))
            for col,frame in enumerate((31,0,1,maximum)):
                crop=scenes[frame].crop((x0,y0,x1,y1))
                scale=min(310/crop.width,205/crop.height)
                crop=crop.resize((round(crop.width*scale),round(crop.height*scale)),Image.Resampling.NEAREST)
                contact.paste(crop,(col*320,row*230+23))
                draw.text((col*320+2,row*230+3),f'{region["name"]} / {frame}',fill='white')
        contact.save(PREVIEW/f'{prefix}_{kind}_contact.png')
    shown[0].save(PREVIEW/f'{prefix}_00.png')
    shown[16].save(PREVIEW/f'{prefix}_16.png')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime-preview',action='store_true')
    args=parser.parse_args()
    if args.runtime_preview:
        report=json.loads((PREVIEW/'inspection.json').read_text(encoding='utf-8'))
        scenes=[Image.open(PREVIEW/f'title_{i:02d}.png').convert('RGB') for i in range(32)]
        save_previews(scenes,report,'title_game')
    else:
        prepare()
