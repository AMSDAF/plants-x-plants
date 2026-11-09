"""Separa o par de retratos sem alterar cores, alfa, halo ou arte dos personagens."""
from pathlib import Path
import hashlib
import json

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'assets/sprites/trainer_selection_portraits_v2.png'
OUTPUT = ROOT / 'assets/sprites/trainers'
PREVIEW = ROOT / 'build/selection_portrait_v2_preview'
PADDING = 24


def prepare():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    PREVIEW.mkdir(parents=True, exist_ok=True)
    digest = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    original = Image.open(SOURCE)
    assert original.mode == 'RGBA' and original.size == (1536, 1024)
    pixels = np.array(original)
    report = dict(source_sha256=digest, source_size=original.size, source_mode=original.mode,
                  alpha_extrema=original.getchannel('A').getextrema(),
                  transparent_pixels=int((pixels[:, :, 3] == 0).sum()),
                  feather_applied=False, rgba_preserved=True, characters=[])
    # As metades completas mantêm inclusive o RGB do halo sob alfa zero.
    # Nenhuma seleção por cor, remoção de fundo ou máscara de animação é aplicada.
    for index, name in enumerate(('male', 'female')):
        crop_box = (index*768, 0, (index+1)*768, 1024)
        half = original.crop(crop_box)
        rgba = np.array(half)
        labels, _ = ndimage.label(rgba[:, :, 3] >= 128)
        sizes = np.bincount(labels.ravel())
        sizes[0] = 0
        body = labels == sizes.argmax()
        ys, xs = np.where(body)
        bounds = (int(xs.min())+PADDING, int(ys.min())+PADDING,
                  int(xs.max()-xs.min()+1), int(ys.max()-ys.min()+1))
        prepared = Image.new('RGBA', (768+2*PADDING, 1024+2*PADDING))
        prepared.paste(half, (PADDING, PADDING))
        prepared.save(OUTPUT/f'{name}_portrait_v2.png')
        result = np.array(prepared)
        assert np.array_equal(result[PADDING:-PADDING, PADDING:-PADDING], rgba)
        assert np.array_equal(np.array(Image.open(OUTPUT/f'{name}_portrait_v2.png')), result)
        # O corte central atravessa apenas fundo invisível ou resíduos de alfa 1.
        assert rgba[:, (0, -1), 3].max() <= 1
        assert result[:PADDING, :, 3].max() == 0
        assert result[-PADDING:, :, 3].max() == 0
        Image.fromarray(rgba[:, :, 3]).save(PREVIEW/f'{name}_alpha.png')
        Image.fromarray(body.astype(np.uint8)*255).save(PREVIEW/f'{name}_character_mask.png')
        # Amplificação apenas de diagnóstico: expõe o halo RGB que o alfa oculta.
        half.convert('RGB').save(PREVIEW/f'{name}_rgb_under_alpha.png')
        annotation = prepared.copy()
        draw = ImageDraw.Draw(annotation)
        x,y,w,h = bounds
        draw.rectangle((x,y,x+w-1,y+h-1), outline='magenta', width=2)
        annotation.save(PREVIEW/f'{name}_bounds.png')
        for background, color in (('cream', (247,237,198,255)), ('forest', (24,52,30,255))):
            composite = Image.new('RGBA', prepared.size, color)
            composite.alpha_composite(prepared)
            composite.convert('RGB').save(PREVIEW/f'{name}_{background}.png')
        report['characters'].append(dict(name=name, crop=crop_box, size=prepared.size,
                                         character_bounds=bounds, edge_alpha_max=int(rgba[:, (0,-1), 3].max()),
                                         padding=PADDING))
    header = ['/* Enquadramento corporal, sem halo; gerado por prepare_selection_portraits_v2.py. */',
              '#ifndef SELECTION_PORTRAIT_V2_DATA_H', '#define SELECTION_PORTRAIT_V2_DATA_H',
              '#include "raylib.h"', '#define SELECTION_PORTRAIT_V2_WIDTH 816',
              '#define SELECTION_PORTRAIT_V2_HEIGHT 1072',
              'static const Rectangle SELECTION_PORTRAIT_V2_BOUNDS[2] = {']
    for character in report['characters']:
        header.append('    {'+', '.join(map(str, character['character_bounds']))+'}, /* '+character['name']+' */')
    header += ['};', '#endif', '']
    (ROOT/'include/selection_portrait_v2_data.h').write_text('\n'.join(header), encoding='utf-8')
    (PREVIEW/'inspection.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    assert hashlib.sha256(SOURCE.read_bytes()).hexdigest() == digest
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    prepare()
