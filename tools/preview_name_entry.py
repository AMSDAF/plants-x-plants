"""Monta prévias da transição real e confere seus extremos pixel a pixel."""
from pathlib import Path
import json
import numpy as np
from PIL import Image, ImageDraw

ROOT=Path(__file__).resolve().parents[1]
PREVIEW=ROOT/'build/name_entry_preview'


def prepare():
    for gender in range(2):
        selected=Image.open(PREVIEW/f'selection_{gender}.ppm').convert('RGB')
        end=Image.open(PREVIEW/f'selection_end_{gender}.ppm').convert('RGB')
        forward=[Image.open(PREVIEW/f'forward_{gender}_{i:02d}.ppm').convert('RGB') for i in range(13)]
        reverse=[Image.open(PREVIEW/f'reverse_{gender}_{i:02d}.ppm').convert('RGB') for i in range(13)]
        assert np.array_equal(np.array(selected),np.array(forward[0])), 'Início deslocado.'
        assert np.array_equal(np.array(end),np.array(reverse[-1])), 'Volta deslocada.'
        # Trechos temporais; o relógio do cenário continua e não fecha no limite deste GIF.
        for label,frames in (('forward',forward),('reverse',reverse)):
            palette=frames[0].quantize(colors=256)
            indexed=[frame.quantize(palette=palette,dither=Image.Dither.NONE) for frame in frames]
            indexed[0].save(PREVIEW/f'{label}_{gender}.gif',save_all=True,append_images=indexed[1:],
                            duration=[400]+[50]*11+[600],loop=0)
        contact=Image.new('RGB',(4*640,2*380),'#18341e')
        draw=ImageDraw.Draw(contact)
        for row,frames in enumerate((forward,reverse)):
            for column,index in enumerate((0,6,9,12)):
                frame=frames[index].resize((640,360),Image.Resampling.NEAREST)
                contact.paste(frame,(column*640,row*380+20))
                draw.text((column*640+3,row*380+3),f'{"ida" if row==0 else "volta"} / {index*.05:.2f}s',fill='white')
        contact.save(PREVIEW/f'transition_contact_{gender}.png')
        # Recortes ampliados do texto final e da costura retrato/cartão.
        final=Image.open(PREVIEW/f'final_{gender}.png').convert('RGB')
        final.crop((580,286,1080,402)).resize((1000,232),Image.Resampling.NEAREST).save(PREVIEW/f'input_closeup_{gender}.png')
    result=dict(selection_and_transition_start_identical=True,reverse_end_and_selection_identical=True,
                sample_interval_seconds=.05,transition_seconds=.6,
                visual_validation='Inspeção de capturas OpenGL; aprovação manual da transição registrada na documentação.')
    (PREVIEW/'validation.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    print('PASS: extremos idênticos para ambos os personagens; GIFs e contatos gerados.')


if __name__=='__main__':
    prepare()
