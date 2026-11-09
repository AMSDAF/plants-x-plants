"""Monta GIFs e contatos dos repousos usando capturas OpenGL reais do Versus."""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT=Path(__file__).resolve().parents[1]
PREVIEW=ROOT/'build/versus_character_preview'
BOXES=[(248,282,408,452),(465,329,591,452),(687,282,847,452),(902,301,1050,452)]


def gif(paths,path,box=None):
    # Mantém apenas quadros indexados em memória durante a prévia de 32 segundos.
    first=Image.open(paths[0]).convert('RGB')
    if box: first=first.crop(box)
    palette=first.quantize(colors=256)
    indexed=[]
    for source in paths:
        frame=Image.open(source).convert('RGB')
        if box: frame=frame.crop(box)
        indexed.append(frame.quantize(palette=palette,dither=Image.Dither.NONE))
    indexed[0].save(path,save_all=True,append_images=indexed[1:],duration=[120 if i%2==0 else 130 for i in range(len(indexed))],loop=0)


def preview():
    paths=[PREVIEW/f'runtime_{i:02d}.ppm' for i in range(256)]
    gif(paths,PREVIEW/'game.gif')
    gif(paths,PREVIEW/'cards.gif',(208,258,1072,552))
    contact=Image.new('RGB',(1200,4*270),'#263b26');draw=ImageDraw.Draw(contact)
    for actor,box in enumerate(BOXES):
        for col,pose in enumerate((0,12,42,48,52)):
            image=Image.open(PREVIEW/f'actor_{actor}_{pose:02d}.ppm').convert('RGB')
            crop=image.crop(box)
            scale=min(232/crop.width,238/crop.height)
            crop=crop.resize((round(crop.width*scale),round(crop.height*scale)),Image.Resampling.NEAREST)
            contact.paste(crop,(col*240,actor*270+25))
            draw.text((col*240,actor*270),f'Personagem {actor+1} / pose {pose}',fill='white')
    contact.save(PREVIEW/'game_contact.png')
    print('GIF completo, GIF dos cartoes e contato em build/versus_character_preview/.')


if __name__=='__main__':preview()
