"""Monta GIF e contatos das capturas OpenGL de copas e arbustos produzidas por versus_screen_check."""
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
PREVIEW = ROOT / 'build/versus_foliage_preview'


def preview():
    frames = [Image.open(PREVIEW / f'runtime_{i:02d}.ppm').convert('RGB') for i in range(64)]
    palette = frames[0].quantize(colors=256)
    indexed = [frame.quantize(palette=palette,dither=Image.Dither.NONE) for frame in frames]
    indexed[0].save(PREVIEW / 'game.gif',save_all=True,append_images=indexed[1:],duration=125,loop=0)
    # Recortes exatos da janela, ampliados somente com vizinho mais próximo.
    boxes = [(154,76,238,158),(1030,72,1152,169),(80,217,206,360),
             (1073,229,1184,354),(441,35,532,112)]
    contact = Image.new('RGB',(1200,160*len(boxes)),'#263b26')
    draw = ImageDraw.Draw(contact)
    for row,box in enumerate(boxes):
        for col,index in enumerate((0,9,21,37,59)):
            crop=frames[index].crop(box)
            scale=min(230/crop.width,130/crop.height)
            crop=crop.resize((round(crop.width*scale),round(crop.height*scale)),Image.Resampling.NEAREST)
            contact.paste(crop,(col*240,row*160+22))
            draw.text((col*240,row*160),f'{index/8:.3f}s',fill='white')
    contact.save(PREVIEW/'game_closeup.png')
    diff=np.abs(np.array(frames[0]).astype(int)-np.array(frames[21]).astype(int)).max(axis=2)
    Image.fromarray(np.uint8(diff>0)*255).save(PREVIEW/'runtime_changes.png')
    print('GIF do jogo e contato em build/versus_foliage_preview/.')


if __name__ == '__main__':
    preview()
