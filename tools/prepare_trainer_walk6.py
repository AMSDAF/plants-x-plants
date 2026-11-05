"""Prepara somente o experimento masculino de seis poses; usa trainer_requirements.txt."""
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

from prepare_trainers import ANCHOR, CELL

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'assets/sprites/trainer_male_walk6_source.png'
APPROVED = ROOT / 'assets/sprites/trainers/male.png'
PREVIEW = ROOT / 'build/trainer_preview/walk6'
OUTPUT = ROOT / 'assets/sprites/trainers/male_walk6.png'
DIRECTIONS = ('south', 'west', 'east', 'north')
CYCLE_SECONDS = 0.56


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def composite(frame, color):
    background = Image.new('RGBA', frame.size, color)
    background.alpha_composite(frame)
    return background.convert('RGB')


def prepare():
    PREVIEW.mkdir(parents=True, exist_ok=True)
    before = {str(path.relative_to(ROOT)): digest(path) for path in (SOURCE, APPROVED)}
    source = Image.open(SOURCE).convert('RGBA')
    assert source.size == (1536, 1024), 'Reinspecionar a grade se a fonte mudar.'
    approved = Image.open(APPROVED).convert('RGBA')
    approved_heights = [approved.crop((x * CELL, y * CELL, (x + 1) * CELL,
                                     (y + 1) * CELL)).getbbox()[3] -
                        approved.crop((x * CELL, y * CELL, (x + 1) * CELL,
                                       (y + 1) * CELL)).getbbox()[1]
                        for y in range(4) for x in range(4)]
    extracted, records = [], []
    masks = Image.new('RGB', source.size)
    for row in range(4):
        for col in range(6):
            crop = (col * 256, row * 256, (col + 1) * 256, (row + 1) * 256)
            pixels = np.array(source.crop(crop))
            alpha = pixels[:, :, 3]
            # O RGB tem fundo marrom, mas o alfa já delimita o personagem.
            # Retira névoa quase transparente sem classificar a cor das roupas.
            labels, _ = ndimage.label(alpha > 32)
            sizes = np.bincount(labels.ravel())
            sizes[0] = 0
            body = labels == sizes.argmax()
            support = ndimage.binary_dilation(body, iterations=2)
            cleaned_alpha = np.rint(np.clip((alpha.astype(float) - 8) / 237, 0, 1) * 255)
            cleaned_alpha[~support] = 0
            pixels[:, :, 3] = cleaned_alpha.astype(np.uint8)
            pixels[cleaned_alpha == 0, :3] = 0
            frame = Image.fromarray(pixels)
            bounds = frame.getbbox()
            assert bounds and 0 < bounds[0] < bounds[2] < 256
            assert 0 < bounds[1] < bounds[3] < 256
            extracted.append(frame)
            masks.paste(Image.fromarray(pixels[:, :, 3]).convert('RGB'), crop[:2])
            records.append({'row': row, 'column': col, 'crop': crop, 'bounds': bounds})
    # Uma única escala para as 24 poses, baseada na altura mediana do atlas aprovado.
    source_height = float(np.median([f.getbbox()[3] - f.getbbox()[1] for f in extracted]))
    scale = float(np.median(approved_heights)) / source_height
    size = round(256 * scale)
    actual_scale = size / 256
    atlas = Image.new('RGBA', (CELL * 6, CELL * 4))
    frames = []
    contact = Image.new('RGB', (6 * 160, 4 * 180), '#7b8375')
    mask_checks = Image.new('RGB', (6 * 256, 4 * 256), '#e8e8ee')
    detail_checks = Image.new('RGB', (6 * 256 * 2, 4 * 256), '#222233')
    for i, frame in enumerate(extracted):
        row, col = divmod(i, 6)
        resized = frame.resize((size, size), Image.Resampling.NEAREST)
        pixels = np.array(resized)
        ys, xs = np.where(pixels[:, :, 3] > 128)
        hat_x = xs[ys < ys.min() + round(65 * actual_scale)]
        foot_x = round((int(hat_x.min()) + int(hat_x.max())) / 2)
        bounds = resized.getbbox()
        offset = (ANCHOR[0] - foot_x, ANCHOR[1] - bounds[3])
        assert 0 <= bounds[0] + offset[0] and bounds[2] + offset[0] <= CELL
        assert 0 <= bounds[1] + offset[1] and bounds[3] + offset[1] <= CELL
        normalized = Image.new('RGBA', (CELL, CELL))
        normalized.paste(resized, offset)
        frames.append(normalized)
        atlas.paste(normalized, (col * CELL, row * CELL))
        thumb = normalized.resize((160, 160), Image.Resampling.NEAREST)
        contact.paste(composite(thumb, '#7b8375'), (col * 160, row * 180))
        ImageDraw.Draw(contact).text((col * 160 + 4, row * 180 + 160),
                                    f'{DIRECTIONS[row]} {col + 1}', fill='white')
        mask_checks.paste(composite(frame, '#eeeeee'), (col * 256, row * 256))
        detail_checks.paste(composite(frame, '#222233'), (col * 512, row * 256))
        detail_checks.paste(composite(frame, '#83afdc'), (col * 512 + 256, row * 256))
        records[i].update({'normalized_bounds': normalized.getbbox(),
                           'axis_source_scaled': foot_x, 'offset': offset})
    masks.save(PREVIEW / 'masks.png')
    source.getchannel('A').save(PREVIEW / 'source_alpha.png')
    contact.save(PREVIEW / 'contact.png')
    mask_checks.save(PREVIEW / 'on_light.png')
    detail_checks.save(PREVIEW / 'on_dark_blue.png')
    # GIF usa unidades de 10 ms: soma 560 ms, como os quatro quadros antigos.
    durations = [90, 100, 90, 90, 100, 90]
    for row, name in enumerate(DIRECTIONS):
        animation = [composite(f.resize((160, 160), Image.Resampling.NEAREST), '#7b8375')
                     for f in frames[row * 6:(row + 1) * 6]]
        animation[0].save(PREVIEW / f'{name}.gif', save_all=True,
                          append_images=animation[1:], duration=durations, loop=0)
        # Prévia A/B no mesmo tamanho e cadência, com as poses originais aprovadas.
        old_sequence = ([0, 1, 3, 1], [0, 1, 0, 3], [0, 1, 0, 3], [0, 1, 3, 1])[row]
        comparison = []
        for tick in range(12):
            canvas = Image.new('RGB', (320, 180), '#7b8375')
            old = old_sequence[tick // 3]
            old_frame = approved.crop((old * CELL, row * CELL, (old + 1) * CELL, (row + 1) * CELL))
            for x, f in ((0, old_frame), (160, frames[row * 6 + tick // 2])):
                canvas.paste(composite(f.resize((160, 160), Image.Resampling.NEAREST), '#7b8375'), (x, 0))
            ImageDraw.Draw(canvas).text((8, 163), 'A: aprovado     B: seis poses', fill='white')
            comparison.append(canvas)
        comparison[0].save(PREVIEW / f'{name}_comparison.gif', save_all=True,
                           append_images=comparison[1:], duration=[50, 40, 50] * 4, loop=0)
    # Só grava um recurso experimental separado, sem tocar os atlas aprovados.
    atlas.save(OUTPUT)
    report = {'source_size': source.size, 'source_mode': source.mode,
              'source_alpha_extrema': source.getchannel('A').getextrema(),
              'source_alpha_percentiles': np.percentile(np.array(source)[:, :, 3],
                                                       [0, 10, 50, 90, 100]).tolist(),
              'preserved_hashes': before, 'uniform_scale': actual_scale,
              'cycle_seconds': CYCLE_SECONDS, 'sequence_one_based': [1, 2, 3, 4, 5, 6],
              'frames': records}
    (PREVIEW / 'inspection.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    assert all(digest(ROOT / path) == value for path, value in before.items())
    print(f'{source.size}, alfa {source.getchannel("A").getextrema()}, escala unica {actual_scale:.6f}')
    print(f'Atlas experimental: {OUTPUT.relative_to(ROOT)}; previas: {PREVIEW.relative_to(ROOT)}')


if __name__ == '__main__':
    prepare()
