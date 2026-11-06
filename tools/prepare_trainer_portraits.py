"""Prepara somente os retratos detalhados de fallback estático, sem alterar as fontes."""
from pathlib import Path
import hashlib
import json

import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets/sprites/trainers'
PREVIEW = ROOT / 'build/portrait_preview'
SIZE = (320, 480)


def prepare(name):
    source = ROOT / f'assets/sprites/trainer_{name}_portrait.png'
    original = Image.open(source).convert('RGBA')
    pixels = np.array(original)
    labels, count = ndimage.label(pixels[:, :, 3] > 24)
    sizes = np.bincount(labels.ravel())
    sizes[0] = 0
    body = labels == sizes.argmax()
    # Remove apenas resíduos fora do contorno conectado e alfa quase invisível.
    # Não filtra cores claras: os realces da camisa permanecem intactos.
    keep = ndimage.binary_dilation(body, iterations=1) & (pixels[:, :, 3] >= 8)
    pixels[~keep] = 0
    ys, xs = np.where(body)
    bounds = (int(xs.min()) - 2, max(0, int(ys.min()) - 2),
              int(xs.max()) + 3, int(ys.max()) + 3)
    cropped = Image.fromarray(pixels).crop(bounds)
    scale = 454 / cropped.height
    resized = cropped.resize((round(cropped.width * scale), 454), Image.Resampling.LANCZOS)
    offset = ((SIZE[0] - resized.width) // 2, 466 - resized.height)
    base = Image.new('RGBA', SIZE)
    base.paste(resized, offset)
    box = base.getbbox()
    assert box is not None and box[0] > 0 and box[2] < SIZE[0]
    assert box[1] > 0 and 460 <= box[3] <= 466
    base.save(OUT / f'{name}_portrait.png')
    base.save(PREVIEW / f'{name}_prepared.png')

    report = {'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
              'source_size': original.size, 'source_alpha_extrema': original.getchannel('A').getextrema(),
              'components_above_24': count, 'removed_visible_pixels': int(((~keep) & (np.array(original)[:, :, 3] > 0)).sum()),
              'crop': bounds, 'scale': scale, 'offset': offset, 'frame_size': SIZE,
              'static_fallback': True}
    (PREVIEW / f'{name}_inspection.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(name, report)


if __name__ == '__main__':
    OUT.mkdir(parents=True, exist_ok=True)
    PREVIEW.mkdir(parents=True, exist_ok=True)
    for name in ('male', 'female'):
        prepare(name)
