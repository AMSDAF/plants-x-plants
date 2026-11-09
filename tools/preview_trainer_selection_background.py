"""Monta GIF completo e diagnósticos a partir de capturas OpenGL da prévia real."""
from pathlib import Path
import json
import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
PREVIEW = ROOT / 'build/trainer_selection_preview'


def prepare():
    report = json.loads((PREVIEW/'inspection.json').read_text(encoding='utf-8'))
    # 128 segundos completos: 32 ciclos de vento e uma volta de ambas as nuvens.
    first = Image.open(PREVIEW/'runtime_000.ppm').convert('RGB')
    last = Image.open(PREVIEW/'runtime_512.ppm').convert('RGB')
    assert np.array_equal(np.array(first), np.array(last)), 'Ciclo não fecha.'
    source = np.array(Image.open(ROOT/'assets/sprites/trainer_selection_background.png').convert('RGB'))
    scale = max(1280/1672,720/941)
    yy,xx = np.mgrid[:720,:1280]
    sx=np.clip(((xx+.5-(1280-1672*scale)/2)/scale).astype(int),0,1671)
    sy=np.clip(((yy+.5-(720-941*scale)/2)/scale).astype(int),0,940)
    r,g,b=[source[:,:,i].astype(float) for i in range(3)]
    structural=(r>g+7)&(r<195)&(b<r*.9)
    for region in report['regions']:
        if region['kind']=='grass':
            x0,y0,x1,y1=region['box']
            mask=np.array(Image.open(PREVIEW/f'{region["name"]}_mask.png'))>0
            base=np.where(mask)[0].max()
            structural|=mask & (np.mgrid[:941,:1672][0]>=base-4)
    structural=structural[sy,sx]
    reference=np.array(first)[structural]
    # Ambas as falhas parciais precisam produzir exatamente a mesma imagem estática completa.
    fallback_a = Image.open(PREVIEW/'fallback_atlas.png').convert('RGB')
    fallback_b = Image.open(PREVIEW/'fallback_base.png').convert('RGB')
    assert np.array_equal(np.array(fallback_a),np.array(fallback_b))
    # Compara o fallback ao mesmo mapeamento pontual da fonte, sem cortes ou partes ausentes.
    assert np.array_equal(np.array(fallback_a),source[sy,sx])
    palette = first.resize((836,470),Image.Resampling.NEAREST).quantize(colors=256)
    frames=[]
    for i in range(512):
        with Image.open(PREVIEW/f'runtime_{i:03d}.ppm') as image:
            assert np.array_equal(np.array(image)[structural],reference), f'Estrutura moveu: {i}'
            shown=image.convert('RGB').resize((836,470),Image.Resampling.NEAREST)
            frames.append(shown.quantize(palette=palette,dither=Image.Dither.NONE))
    frames[0].save(PREVIEW/'scene_loop.gif',save_all=True,append_images=frames[1:],duration=250,loop=0,optimize=False)
    # Contato de limites do vento e do ciclo completo, incluindo 127,875 -> 128 -> 128,125.
    crops=[('copas',(0,0,265,205)),('grama',(5,545,230,625)),
           ('distante',(600,225,790,350)),('nuvem',(140,105,310,180))]
    contact=Image.new('RGB',(1200,len(crops)*235),'#263b26')
    draw=ImageDraw.Draw(contact)
    paths=['boundary_0.png','boundary_1.png','boundary_2.png','runtime_008.ppm']
    for row,(name,box) in enumerate(crops):
        for column,path in enumerate(paths):
            with Image.open(PREVIEW/path) as image:
                crop=image.crop(box)
                factor=min(295/crop.width,205/crop.height)
                crop=crop.resize((round(crop.width*factor),round(crop.height*factor)),Image.Resampling.NEAREST)
                contact.paste(crop,(column*300,row*235+25))
                draw.text((column*300+2,row*235+4),f'{name} / {path}',fill='white')
    contact.save(PREVIEW/'boundary_contact.png')
    # Inclui todos os wraps intermediários: cada nuvem tem uma fase inicial própria.
    wrap_indices=[]
    for region in report['regions']:
        if region['kind']=='cloud':
            span=1672+region['size'][0]
            t=(1672-region['origin'][0])/span*128
            wrap_indices.append(round(t*4))
    wrap_contact=Image.new('RGB',(1680,len(wrap_indices)*250),'#263b26')
    draw=ImageDraw.Draw(wrap_contact)
    for row,index in enumerate(wrap_indices):
        for col,frame in enumerate((index-1,index,index+1)):
            with Image.open(PREVIEW/f'runtime_{frame:03d}.ppm') as image:
                for side,left in enumerate((0,1000)):
                    crop=image.crop((left,90,left+280,310))
                    wrap_contact.paste(crop,(col*560+side*280,row*250+25))
                draw.text((col*560+2,row*250+4),f'wrap {row} / {frame*.25:.2f}s / esquerda | direita',fill='white')
    wrap_contact.save(PREVIEW/'cloud_wrap_contact.png')
    # Inspeção do próprio GIF decodificado, incluindo o último quadro antes do loop.
    gif_contact=Image.new('RGB',(1672,3*490),'#263b26')
    draw=ImageDraw.Draw(gif_contact)
    with Image.open(PREVIEW/'scene_loop.gif') as gif:
        assert gif.n_frames==512 and gif.info['loop']==0
        for cell,index in enumerate((0,8,104,405,510,511)):
            gif.seek(index)
            gif_contact.paste(gif.convert('RGB'),((cell%2)*836,(cell//2)*490+20))
            draw.text(((cell%2)*836+3,(cell//2)*490+3),f'GIF / {index*.25:.2f}s',fill='white')
    gif_contact.save(PREVIEW/'gif_contact.png')
    diagnostics=dict(full_cycle_seconds=128,gif_fps=4,gif_size=[836,470],
                     runtime_first_last_identical=True,partial_fallbacks_identical=True,
                     fallback_matches_source=True,structural_pixels_static=True,
                     wrap_frame_indices=wrap_indices)
    (PREVIEW/'runtime_validation.json').write_text(json.dumps(diagnostics,indent=2),encoding='utf-8')
    print('GIF OpenGL de 128 s, contatos de limites e diagnóstico gerados.')


if __name__=='__main__':
    prepare()
