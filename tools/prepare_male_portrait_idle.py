"""Alinha poses masculinas por botas, preserva pernas neutras e prepara atlas de repouso."""
from pathlib import Path
import argparse
import hashlib
import json
import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'assets/sprites/male_portrait_idle_source.png'
OUTPUT = ROOT / 'assets/sprites/trainers/male_portrait_idle.png'
PREVIEW = ROOT / 'build/male_portrait_idle_preview'
CELL = (352, 656)
ANCHOR = (128, 624)
# Durações explícitas, na ordem neutro / inspiração / piscar / relaxado.
DURATIONS = (3.8, .35, .10, .45)
SEAM_Y = 372


def bounds(mask):
    y,x = np.where(mask)
    return (int(x.min()), int(y.min()), int(x.max()+1), int(y.max()+1))


def cream(image):
    base = Image.new('RGBA', image.size, (247,237,198,255))
    base.alpha_composite(image)
    return base.convert('RGB')


def prepare():
    PREVIEW.mkdir(parents=True, exist_ok=True)
    digest = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    original = Image.open(SOURCE).convert('RGBA')
    assert original.size == (1199,1312)
    raw, measurements = [], []
    for i,(left,top,right,bottom) in enumerate(((0,0,599,656),(599,0,1199,656),
                                              (0,656,599,1312),(599,656,1199,1312))):
        patch = np.array(original.crop((left,top,right,bottom)))
        labels,_ = ndimage.label(patch[:,:,3]>=128)
        sizes = np.bincount(labels.ravel())
        sizes[0]=0
        body=labels==sizes.argmax()
        x0,y0,x1,y1=bounds(body)
        yy,xx=np.where(body)
        sole_x=int(round(float(np.median(xx[yy>=y1-4]))))
        offset=(ANCHOR[0]-sole_x,ANCHOR[1]-y1)
        # Recorte medido em cada quadrante; as posições geradas não seguem a mesma grade.
        crop=(max(0,x0-28),max(0,y0-10),min(patch.shape[1],x1+28),min(patch.shape[0],y1+28))
        frame=Image.new('RGBA',CELL)
        frame.paste(Image.fromarray(patch).crop(crop),(crop[0]+offset[0],crop[1]+offset[1]))
        raw.append(frame)
        measurements.append(dict(quadrant=(left,top,right,bottom),body=(x0,y0,x1,y1),
                                 sole_anchor=(sole_x,y1),offset=offset,crop=crop))
        frame.save(PREVIEW/f'aligned_raw_{i}.png')
    neutral=np.array(raw[0])
    frames=[]
    for i,image in enumerate(raw):
        data=np.array(image)
        # O corte atravessa o cinto; pernas, botas e mão inferior são cópias exatas do neutro.
        data[SEAM_Y:]=neutral[SEAM_Y:]
        assert np.array_equal(data[SEAM_Y:],neutral[SEAM_Y:])
        frames.append(Image.fromarray(data))
        frames[-1].save(PREVIEW/f'frame_{i}.png')
    atlas=Image.new('RGBA',(CELL[0]*4,CELL[1]))
    for i,frame in enumerate(frames): atlas.paste(frame,(i*CELL[0],0))
    atlas.save(OUTPUT)
    # Halo estático separado da arte v2: somente pixels fora do corpo opaco.
    static_path=ROOT/'assets/sprites/trainers/male_portrait_v2.png'
    halo=np.array(Image.open(static_path).convert('RGBA'))
    labels,_=ndimage.label(halo[:,:,3]>=128)
    sizes=np.bincount(labels.ravel()); sizes[0]=0
    body_guard=ndimage.binary_dilation(labels==sizes.argmax(),iterations=2)
    halo[body_guard,3]=0
    Image.fromarray(halo).save(OUTPUT.with_name('male_portrait_idle_halo.png'))
    cream(Image.fromarray(halo)).save(PREVIEW/'static_halo_cream.png')
    x0,y0,x1,y1=bounds(neutral[:,:,3]>=128)
    body=(x0,y0,x1-x0,y1-y0)
    header=['/* Atlas, ancoragem e tempos gerados por prepare_male_portrait_idle.py. */',
            '#ifndef MALE_PORTRAIT_IDLE_DATA_H','#define MALE_PORTRAIT_IDLE_DATA_H','#include "raylib.h"',
            f'#define MALE_PORTRAIT_IDLE_WIDTH {CELL[0]}',f'#define MALE_PORTRAIT_IDLE_HEIGHT {CELL[1]}',
            '#define MALE_PORTRAIT_IDLE_FRAMES 4',f'#define MALE_PORTRAIT_IDLE_SECONDS {sum(DURATIONS):.2f}f',
            'static const float MALE_PORTRAIT_IDLE_DURATIONS[4] = {'+', '.join(f'{v:.2f}f' for v in DURATIONS)+'};',
            'static const Rectangle MALE_PORTRAIT_IDLE_BODY = {'+', '.join(map(str,body))+'};',
            '#endif','']
    (ROOT/'include/male_portrait_idle_data.h').write_text('\n'.join(header),encoding='utf-8')
    contact=Image.new('RGB',(CELL[0]*4,CELL[1]),(247,237,198))
    seam_contact=Image.new('RGB',(CELL[0]*4,160*2),(247,237,198))
    for i,frame in enumerate(frames):
        contact.paste(cream(frame),(i*CELL[0],0))
        seam_contact.paste(cream(raw[i]).crop((0,SEAM_Y-48,CELL[0],SEAM_Y+32)).resize((CELL[0],160),Image.Resampling.NEAREST),(i*CELL[0],0))
        seam_contact.paste(cream(frame).crop((0,SEAM_Y-48,CELL[0],SEAM_Y+32)).resize((CELL[0],160),Image.Resampling.NEAREST),(i*CELL[0],160))
    contact.save(PREVIEW/'poses_contact.png')
    seam_contact.save(PREVIEW/'seam_contact.png')
    shown=[cream(frame) for frame in frames]
    palette=shown[0].quantize(colors=256)
    indexed=[frame.quantize(palette=palette,dither=Image.Dither.NONE) for frame in shown]
    indexed[0].save(PREVIEW/'cream_loop.gif',save_all=True,append_images=indexed[1:],
                    duration=[round(v*1000) for v in DURATIONS],loop=0)
    report=dict(source_sha256=digest,source_size=original.size,alpha_extrema=original.getchannel('A').getextrema(),
                cell=CELL,anchor=ANCHOR,body=body,seam_y=SEAM_Y,durations=DURATIONS,
                shared_scale=360/body[3],stationary_lower_body=True,measurements=measurements,
                static_halo_source_sha256=hashlib.sha256(static_path.read_bytes()).hexdigest())
    (PREVIEW/'inspection.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==digest
    print(json.dumps(report,indent=2))


def runtime_preview():
    report=json.loads((PREVIEW/'inspection.json').read_text(encoding='utf-8'))
    frames=[Image.open(PREVIEW/f'runtime_pose_{i}.png').convert('RGB') for i in range(4)]
    end=Image.open(PREVIEW/'runtime_loop_end.png').convert('RGB')
    assert np.array_equal(np.array(frames[0]),np.array(end)), 'Ciclo não fecha.'
    neutral=np.array(frames[0])
    for frame in frames:
        pixels=np.array(frame)
        assert np.array_equal(pixels[374:534,340:550],neutral[374:534,340:550]), 'Pernas/botas moveram.'
        assert np.array_equal(pixels[150:534,730:935],neutral[150:534,730:935]), 'Retrato feminino mudou.'
    fallback=Image.open(PREVIEW/'runtime_fallback.png').convert('RGB')
    fallback_halo=Image.open(PREVIEW/'runtime_fallback_halo.png').convert('RGB')
    assert np.array_equal(np.array(fallback),np.array(fallback_halo))
    # Outros relógios congelados somente nas capturas: isola a emenda do repouso.
    palette=frames[0].quantize(colors=256)
    indexed=[frame.quantize(palette=palette,dither=Image.Dither.NONE) for frame in frames]
    indexed[0].save(PREVIEW/'runtime_scene_loop.gif',save_all=True,append_images=indexed[1:],
                    duration=[round(v*1000) for v in report['durations']],loop=0)
    cards=[frame.crop((280,116,616,596)) for frame in frames]
    palette=cards[0].quantize(colors=256)
    indexed=[frame.quantize(palette=palette,dither=Image.Dither.NONE) for frame in cards]
    indexed[0].save(PREVIEW/'runtime_card_loop.gif',save_all=True,append_images=indexed[1:],
                    duration=[round(v*1000) for v in report['durations']],loop=0)
    contact=Image.new('RGB',(4*336,510),(247,237,198))
    draw=ImageDraw.Draw(contact)
    for i,frame in enumerate(cards):
        contact.paste(frame,(i*336,30))
        draw.text((i*336+8,8),f'{i}: {report["durations"][i]:.2f}s',fill='black')
    contact.save(PREVIEW/'runtime_card_contact.png')
    contact=Image.new('RGB',(4*400,240),(247,237,198))
    for i,frame in enumerate(frames):
        seam=frame.crop((350,335,550,395)).resize((400,120),Image.Resampling.NEAREST)
        face=frame.crop((380,185,515,260)).resize((216,120),Image.Resampling.NEAREST)
        contact.paste(seam,(i*400,0))
        contact.paste(face,(i*400,120))
    contact.save(PREVIEW/'runtime_seam_contact.png')
    with Image.open(PREVIEW/'runtime_card_loop.gif') as gif:
        assert gif.n_frames==4 and gif.info['loop']==0
        durations=[]
        for i in range(4):
            gif.seek(i)
            durations.append(gif.info['duration'])
        assert durations==[round(v*1000) for v in report['durations']]
    validation=dict(lower_body_static=True,female_unchanged=True,loop_first_end_identical=True,
                    partial_fallbacks_identical=True,gif_durations_ms=durations)
    (PREVIEW/'runtime_validation.json').write_text(json.dumps(validation,indent=2),encoding='utf-8')
    print('PASS: ciclo OpenGL, pernas/botas fixas, mulher inalterada e fallback completo; GIFs gerados.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime-preview',action='store_true')
    if parser.parse_args().runtime_preview:
        runtime_preview()
    else:
        prepare()
