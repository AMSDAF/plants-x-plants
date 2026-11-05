"""Prepara retratos de seleção e respiração localizada sem alterar os originais."""
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
FRAMES = 32
COLUMNS = 8
BACKGROUND = (43, 53, 42, 255)
# Elipses no tecido do tórax; não alcançam rosto, mãos, cinto ou equipamento.
CHEST = {'male': (625, 351, 100, 122), 'female': (553, 421, 78, 120)}


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

    cx, cy, rx, ry = CHEST[name]
    cx = (cx - bounds[0]) * scale + offset[0]
    cy = (cy - bounds[1]) * scale + offset[1]
    rx, ry = rx * scale, ry * scale
    y, x = np.mgrid[:SIZE[1], :SIZE[0]].astype(float)
    radius = ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2
    weight = np.maximum(0, 1 - radius) ** 3
    rgba = np.array(base).astype(float) / 255
    premultiplied = rgba.copy()
    premultiplied[:, :, :3] *= rgba[:, :, 3:4]
    atlas = Image.new('RGBA', (SIZE[0] * COLUMNS, SIZE[1] * (FRAMES // COLUMNS)))
    previews, frames = [], []
    for i in range(FRAMES):
        breath = (1 - np.cos(2 * np.pi * i / FRAMES)) / 2
        # Máximo de 0,75 pixel vertical no centro; campo contínuo com bordas imóveis.
        coords = [y + 0.75 * breath * weight,
                  x - (x - cx) / rx * 0.5 * breath * weight]
        warped = np.stack([ndimage.map_coordinates(premultiplied[:, :, c], coords,
                            order=1, mode='constant', cval=0) for c in range(4)], axis=2)
        warped[:, :, :3] /= np.maximum(warped[:, :, 3:4], 1 / 255)
        result = np.uint8(np.clip(np.rint(warped * 255), 0, 255))
        # Fora da elipse, cópia exata: impede reamostragem de chapéu, botas e mãos.
        result[weight == 0] = np.array(base)[weight == 0]
        frame = Image.fromarray(result)
        frames.append(result)
        atlas.paste(frame, ((i % COLUMNS) * SIZE[0], (i // COLUMNS) * SIZE[1]))
        preview = Image.new('RGBA', SIZE, BACKGROUND)
        preview.alpha_composite(frame)
        previews.append(preview.convert('RGB'))
    atlas.save(OUT / f'{name}_portrait_breath.png')
    previews[0].save(PREVIEW / f'{name}_breath.gif', save_all=True,
                     append_images=previews[1:], duration=160, loop=0)
    previews[0].save(PREVIEW / f'{name}_prepared.png')
    diff = np.max(np.abs(frames[16].astype(int) - frames[0].astype(int)), axis=2)
    Image.fromarray(np.uint8(np.minimum(diff * 8, 255))).save(PREVIEW / f'{name}_motion.png')
    assert all(np.array_equal(f[weight == 0], frames[0][weight == 0]) for f in frames)
    assert np.array_equal(frames[1], frames[-1])
    report = {'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
              'source_size': original.size, 'source_alpha_extrema': original.getchannel('A').getextrema(),
              'components_above_24': count, 'removed_visible_pixels': int(((~keep) & (np.array(original)[:, :, 3] > 0)).sum()),
              'crop': bounds, 'scale': scale, 'offset': offset, 'frame_size': SIZE,
              'frame_count': FRAMES, 'cycle_seconds': 5.12,
              'max_vertical_displacement': 0.75, 'stable_outside_chest': True,
              'loop_symmetric': True}
    (PREVIEW / f'{name}_inspection.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(name, report)


if __name__ == '__main__':
    OUT.mkdir(parents=True, exist_ok=True)
    PREVIEW.mkdir(parents=True, exist_ok=True)
    for name in ('male', 'female'):
        prepare(name)
