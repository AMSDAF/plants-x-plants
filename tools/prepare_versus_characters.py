"""Prepara repousos dos quatro personagens dos cartões sem alterar a arte aprovada."""
from pathlib import Path
import hashlib
import json
import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'assets/sprites/versus_illustrations_source.png'
OUTPUT=ROOT/'assets/sprites/versus'
PREVIEW=ROOT/'build/versus_character_preview'
WIDTH,HEIGHT,PAD,FRAMES,ATLAS_WIDTH=482,244,8,64,2048
CONFIGS=[
    dict(name='green_cpu',card=0,box=(28,0,218,203),left=84,right=160,leaf=89,fixed=169,fps=8,phase=0),
    dict(name='robot',card=0,box=(310,60,460,203),fixed=156,fps=6,phase=1.5),
    dict(name='green_player',card=1,box=(17,0,203,203),left=70,right=147,leaf=89,fixed=169,fps=8,phase=2.75),
    dict(name='red_player',card=1,box=(297,22,477,203),left=337,right=425,leaf=101,fixed=171,fps=10,phase=.8),
]


def translate(layer,dx=0,dy=0):
    result=np.zeros_like(layer)
    h,w=layer.shape[:2]
    x0,x1=max(0,dx),min(w,w+dx)
    y0,y1=max(0,dy),min(h,h+dy)
    result[y0:y1,x0:x1]=layer[y0-dy:y1-dy,x0-dx:x1-dx]
    return result


def overlay(parts):
    result=np.zeros_like(parts[0])
    for part in parts:
        mask=part[:,:,3]>0
        result[mask]=part[mask]
    return result


def prepare():
    PREVIEW.mkdir(parents=True,exist_ok=True)
    protected=[SOURCE,OUTPUT/'grass_base.png',OUTPUT/'grass_atlas.png',OUTPUT/'foliage_base.png',
               OUTPUT/'foliage_atlas.png',ROOT/'include/versus_grass_data.h',ROOT/'include/versus_foliage_data.h']
    hashes={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}
    original=Image.open(SOURCE).convert('RGB')
    assert original.size==(1672,941)
    cards=[np.array(original.crop((x,384,x+WIDTH,384+HEIGHT))) for x in (294,886)]
    H,W=HEIGHT+2*PAD,WIDTH+2*PAD
    yy,xx=np.mgrid[:H,:W]; x=xx-PAD; y=yy-PAD
    characters=[]
    for config in CONFIGS:
        card=cards[config['card']]
        rgb=np.pad(card,((PAD,PAD),(PAD,PAD),(0,0)),mode='edge')
        r,g,b=[rgb[:,:,i].astype(float) for i in range(3)]
        x0,y0,x1,y1=config['box']
        area=(x>=x0)&(x<x1)&(y>=y0)&(y<y1)
        seed=area&(r<165)&(g<175)&(b<145)
        labels,_=ndimage.label(seed)
        sizes=np.bincount(labels.ravel());sizes[0]=0
        seed=labels==sizes.argmax()
        mask=ndimage.binary_fill_holes(seed)
        # Inclui só a franja junto ao contorno; o creme do cartão não acompanha o sprite.
        fringe=ndimage.binary_dilation(mask,iterations=1)&area&(r<224)&(g<218)
        mask |= fringe
        layer=np.zeros((H,W,4),np.uint8)
        layer[mask,:3]=rgb[mask];layer[mask,3]=255
        assert mask.any(),config['name']
        Image.fromarray(layer).save(PREVIEW/f'{config["name"]}_layer.png')
        poses=[];records=[]
        for f in range(FRAMES):
            if config['name']=='robot':
                # A base e o pescoço ficam fixos; monitor rígido com ajuste de um pixel.
                head=layer.copy();head[y>=config['fixed']+5]=0
                eyes=(x>=340)&(x<409)&(y>=105)&(y<134)&(r>215)&(g>205)&(b>150)&mask
                blink=1 if f in (41,43) else 2 if f==42 else 0
                if blink:
                    eyes=ndimage.binary_dilation(eyes,iterations=2)
                    for xa,xb in ((340,372),(375,409)):
                        eye=eyes&(x>=xa)&(x<xb)
                        ey=np.where(eye)[0]
                        assert ey.size>0
                        center=int(round(ey.mean()))
                        erase=eye if blink==2 else eye&(abs(yy-center)>1)
                        # Amostra escura da própria tela, sem inventar uma cor de pálpebra.
                        head[erase,:3]=rgb[PAD+100,xx[erase],:3]
                dx=1 if 16<=f<25 else -1 if 48<=f<55 else 0
                dy=-1 if 8<=f<16 or 32<=f<40 else 0
                moved=translate(head,dx,dy)
                fixed=layer.copy();fixed[y<config['fixed']]=0
                pose=overlay((moved,fixed))
                record=dict(frame=f,dx=dx,dy=dy,blink=blink)
            else:
                red=config['name']=='red_player'
                breath=-1 if f%32 in range(4 if red else 8,16 if red else 22) else 0
                leaf=int(round((2 if red else 1.5)*np.sin(2*np.pi*f/(32 if red else 64))))
                gesture=0
                start=38 if red else 45 if config['card']==0 else 49
                for j,v in enumerate((0,1,2,3,3,2,1,0) if red else (0,1,2,2,1,0)):
                    if f==start+j: gesture=v
                leaf_mask=y<config['leaf']+6
                left_mask=(x<config['left'])&(y>=112)&(y<166)
                right_mask=(x>=config['right'])&(y>=112)&(y<167)
                body_mask=(y>=config['leaf']-5)&(y<config['fixed']+5)
                body_mask &= ~(left_mask&(x<config['left']-6))
                body_mask &= ~(right_mask&(x>=config['right']+6))
                parts=[]
                body=layer.copy();body[~body_mask]=0
                parts.append(translate(body,0,breath))
                for arm_mask,up in ((left_mask,gesture),(right_mask,gesture if red else gesture//2)):
                    arm=layer.copy();arm[~arm_mask]=0
                    parts.append(translate(arm,0,breath-up))
                leaves=layer.copy();leaves[~leaf_mask]=0
                # Folhas flexionam pela ponta, com raiz imóvel e amostragem inteira.
                weight=np.clip((config['leaf']-y)/max(1,config['leaf']-y0),0,1)**1.7
                sx=xx-np.rint(leaf*weight).astype(int)
                valid=(sx>=0)&(sx<W)
                moved=np.zeros_like(layer);moved[valid]=leaves[yy[valid],sx[valid]]
                parts.append(translate(moved,0,breath))
                feet=layer.copy();feet[y<config['fixed']]=0
                parts.append(feet)
                pose=overlay(parts)
                record=dict(frame=f,breath=breath,leaf=leaf,gesture=gesture)
            pose[y>=config['fixed']]=layer[y>=config['fixed']]
            assert np.array_equal(pose[y>=config['fixed']],layer[y>=config['fixed']]),config['name']
            # O rosto e o corpo só usam pixels originais; não há escala nem filtro suave.
            assert set(np.unique(pose[:,:,3]))<={0,255}
            if f==0: assert np.array_equal(pose,layer),config['name']
            labels,_=ndimage.label(pose[:,:,3]>0,structure=np.ones((3,3)))
            sizes=np.bincount(labels.ravel());sizes[0]=0
            assert np.count_nonzero(sizes>250)<=2,config['name']
            poses.append(pose);records.append(record)
        # Célula comum para todas as poses: sem mudança de escala, pivô ou enquadramento.
        union=np.any(np.stack([p[:,:,3]>0 for p in poses]),axis=0)
        ys,xs=np.where(union)
        ax,ay,bx,by=int(xs.min())-4,int(ys.min())-4,int(xs.max())+5,int(ys.max())+5
        assert ax>=0 and ay>=0 and bx<=W and by<=H
        unique=[];indices=[];lookup={}
        for pose in poses:
            crop=Image.fromarray(pose[ay:by,ax:bx])
            key=hashlib.sha256(crop.tobytes()).hexdigest()
            if key not in lookup:
                lookup[key]=len(unique);unique.append(crop)
            indices.append(lookup[key])
        assert len(unique)>3
        exposed=np.zeros((H,W),bool)
        for pose in poses: exposed |= mask&(pose[:,:,3]==0)
        # Só a faixa realmente exposta é reconstruída; pés e sombra continuam originais.
        exposed=ndimage.binary_dilation(exposed,iterations=1)&(y<config['fixed'])
        characters.append(dict(**config,origin=(ax-PAD,ay-PAD),size=(bx-ax,by-ay),
                               poses=unique,frames=indices,raw=poses,layer=layer,
                               exposed=exposed,records=records))
    bases=[card.copy() for card in cards]
    for c in range(2):
        removal=np.zeros((H,W),bool)
        occupied=np.zeros((H,W),bool)
        for char in characters:
            if char['card']==c:
                removal|=char['exposed'];occupied|=char['layer'][:,:,3]>0
        rgb=np.pad(cards[c],((PAD,PAD),(PAD,PAD),(0,0)),mode='edge')
        r,g,b=[rgb[:,:,i].astype(float) for i in range(3)]
        donors=(r>230)&(g>220)&(b>160)&~ndimage.binary_dilation(occupied,iterations=5)
        donors &= (y>=0)&(y<165)&(x>=0)&(x<WIDTH)
        # Fundo creme local suave: só substitui os poucos pixels expostos pelo repouso.
        basis=np.stack((x*0+1,x/W,y/H,(x/W)**2,(x/W)*(y/H),(y/H)**2),axis=-1)
        coefficients=np.linalg.lstsq(basis[donors],rgb[donors],rcond=None)[0]
        predicted=np.clip(np.rint(basis@coefficients),0,255).astype(np.uint8)
        base=rgb.copy();base[removal]=predicted[removal]
        bases[c]=base[PAD:PAD+HEIGHT,PAD:PAD+WIDTH]
        Image.fromarray(removal.astype(np.uint8)*255).save(PREVIEW/f'removal_{c}.png')
        assert np.array_equal(base[~removal],rgb[~removal])
        # Neutro composto recupera a ilustração original, fora da franja de fundo restaurada.
        neutral=Image.fromarray(base).convert('RGBA')
        for char in characters:
            if char['card']==c: neutral.alpha_composite(Image.fromarray(char['raw'][0]))
        result=np.array(neutral)[:,:,:3]
        assert np.array_equal(result[occupied],rgb[occupied])
        assert np.array_equal(result[y>=174],rgb[y>=174])
    base=Image.new('RGB',(WIDTH*2,HEIGHT))
    for i,pixels in enumerate(bases):base.paste(Image.fromarray(pixels),(i*WIDTH,0))
    base.save(OUTPUT/'character_base.png')
    cells=[(char,i,pose) for char in characters for i,pose in enumerate(char['poses'])]
    cx=cy=row=0
    for char,i,pose in sorted(cells,key=lambda c:c[2].height,reverse=True):
        w,h=pose.size
        if cx+w>ATLAS_WIDTH:cy+=row;cx=row=0
        char.setdefault('sources',{})[i]=(cx,cy,w,h);cx+=w;row=max(row,h)
    atlas=Image.new('RGBA',(ATLAS_WIDTH,cy+row))
    for char,i,pose in cells:atlas.paste(pose,char['sources'][i][:2])
    atlas.save(OUTPUT/'character_atlas.png')
    max_poses=max(len(c['poses']) for c in characters)
    header=['/* Repousos e pivôs gerados por tools/prepare_versus_characters.py. */',
            '#ifndef VERSUS_CHARACTER_DATA_H','#define VERSUS_CHARACTER_DATA_H','#include "raylib.h"',
            '#define VERSUS_CHARACTER_COUNT 4','#define VERSUS_CHARACTER_FRAMES 64',
            f'#define VERSUS_CHARACTER_ATLAS_WIDTH {ATLAS_WIDTH}',f'#define VERSUS_CHARACTER_ATLAS_HEIGHT {atlas.height}',
            f'typedef struct {{ Vector2 origin; Rectangle poses[{max_poses}]; unsigned char frames[64]; int card; int fps; float phase; int fixed_y; }} VersusCharacterRegion;',
            'static const VersusCharacterRegion VERSUS_CHARACTER_REGIONS[4] = {']
    for char in characters:
        origin=', '.join(map(str,char['origin']))
        poses=', '.join('{'+', '.join(map(str,char['sources'][i]))+'}' for i in range(len(char['poses'])))
        frames=', '.join(map(str,char['frames']))
        header.append(f'    {{{{{origin}}}, {{{poses}}}, {{{frames}}}, {char["card"]}, {char["fps"]}, {char["phase"]:.2f}f, {char["fixed"]}}}, /* {char["name"]} */')
    header+=['};','#endif','']
    path=ROOT/'include/versus_character_data.h';text='\n'.join(header)
    if not path.exists() or path.read_text(encoding='utf-8')!=text:path.write_text(text,encoding='utf-8')
    report=dict(protected_hashes=hashes,atlas_size=atlas.size,
                characters=[{k:v for k,v in c.items() if k not in ('poses','raw','layer','exposed')} for c in characters])
    (PREVIEW/'inspection.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    contact=Image.new('RGB',(1200,4*270),'#263b26');draw=ImageDraw.Draw(contact)
    for row,char in enumerate(characters):
        frames=(0,12,24,42,48 if char['name']!='green_player' else 52)
        for col,f in enumerate(frames):
            card=Image.fromarray(bases[char['card']]).convert('RGBA')
            for other in characters:
                if other['card']==char['card']:card.alpha_composite(Image.fromarray(other['raw'][f]).crop((PAD,PAD,PAD+WIDTH,PAD+HEIGHT)))
            ax,ay=char['origin'];w,h=char['size']
            crop=card.crop((ax-2,ay-2,ax+w+2,ay+h+2))
            scale=min(232/crop.width,240/crop.height)
            crop=crop.resize((round(crop.width*scale),round(crop.height*scale)),Image.Resampling.NEAREST)
            contact.paste(crop.convert('RGB'),(col*240,row*270+24))
            draw.text((col*240,row*270),f'{char["name"]} / {f}',fill='white')
    contact.save(PREVIEW/'contact.png')
    for p in protected:assert hashlib.sha256(p.read_bytes()).hexdigest()==hashes[str(p.relative_to(ROOT))]
    print(f'4 personagens; poses {[len(c["poses"]) for c in characters]}; atlas {atlas.size}; arte e vegetacao intactas.')


if __name__=='__main__':prepare()
