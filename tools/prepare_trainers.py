"""Prepara os treinadores sem modificar as fontes. Dependências: trainer_requirements.txt."""
from pathlib import Path
import hashlib
import json

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets/sprites/trainers'
PREVIEW = ROOT / 'build/trainer_preview'
CELL = 320
ANCHOR = (160, 304)
# Colunas baseadas na inspeção: evita repetir as passadas abertas quase idênticas.
SEQUENCES = {
    'male': [[0, 1, 3, 1], [0, 1, 0, 3], [0, 1, 0, 3], [0, 1, 3, 1]],
    # A quarta pose frontal feminina é menor; não entra no ciclo de execução.
    'female': [[0, 1, 0, 1], [0, 1, 0, 3], [0, 1, 0, 3], [0, 1, 3, 1]],
}
IDLE = [1, 1, 3, 1]


def prepare(name):
    source = ROOT / f'assets/sprites/trainer_{name}_sheet.png'
    image = Image.open(source).convert('RGBA')
    width, height = image.size
    atlas = Image.new('RGBA', (CELL * 4, CELL * 4))
    frames = []
    report = {'source_size': image.size, 'alpha_extrema': image.getchannel('A').getextrema(),
              'sequence_zero_based': SEQUENCES[name], 'idle_zero_based': IDLE,
              'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(), 'frames': []}
    for row in range(4):
        direction = []
        for col in range(4):
            # Divisões proporcionais: a fonte não precisa ter dimensões múltiplas de quatro.
            box = (col * width // 4, row * height // 4,
                   (col + 1) * width // 4, (row + 1) * height // 4)
            pixels = np.array(image.crop(box))
            labels, _ = ndimage.label(pixels[:, :, 3] > 32)
            sizes = np.bincount(labels.ravel())
            sizes[0] = 0
            main = int(sizes.argmax())
            body = labels == main
            ys, xs = np.where(body)
            # Apenas o corpo conectado; halo alfa é mantido até dois pixels de distância.
            keep = ndimage.binary_dilation(body, iterations=2)
            pixels[~keep] = 0
            # As manchas sobre o chapéu têm cor saturada artificial e estão separadas
            # ou tocam somente a borda superior. Limpeza localizada, nunca na planta/mochila.
            top = int(ys.min())
            for y in range(top, min(top + 10, pixels.shape[0])):
                for x in range(pixels.shape[1]):
                    r, g, b, a = map(int, pixels[y, x])
                    if a and ((r > 170 and r > g * 1.8 and r > b * 1.8)
                              or (g > 65 and g > r * 1.3 and b < 45)):
                        pixels[y, x] = 0
            cleaned = Image.fromarray(pixels)
            bounds = cleaned.getbbox()
            # Projeta o eixo do chapéu até o chão: seguir somente a bota mais baixa
            # fazia o corpo saltar lateralmente quando a perna de apoio mudava.
            # A mochila fica abaixo desta faixa e não desloca o eixo horizontal.
            feet_y = bounds[3]
            axis_x = xs[ys < int(ys.min()) + 65]
            foot_x = int(round((int(axis_x.min()) + int(axis_x.max())) / 2))
            frame = Image.new('RGBA', (CELL, CELL))
            offset = (ANCHOR[0] - foot_x, ANCHOR[1] - feet_y)
            frame.paste(cleaned, offset)
            assert frame.getbbox() is not None
            assert offset[0] + bounds[0] >= 0 and offset[0] + bounds[2] <= CELL
            assert offset[1] + bounds[1] >= 0 and offset[1] + bounds[3] <= CELL
            atlas.paste(frame, (col * CELL, row * CELL))
            direction.append(frame)
            report['frames'].append({'row': row, 'column': col, 'crop': box,
                                     'body_bounds': bounds, 'foot_source': [foot_x, feet_y],
                                     'removed_pixels': int(np.count_nonzero((pixels[:, :, 3] == 0) &
                                         (np.array(image.crop(box))[:, :, 3] > 0)))})
        frames.append(direction)
    atlas.save(OUT / f'{name}.png')
    contact = Image.new('RGB', (640, 720), '#7b8375')
    draw = ImageDraw.Draw(contact)
    for row, direction in enumerate(frames):
        for col, frame in enumerate(direction):
            thumb = frame.resize((160, 160), Image.Resampling.NEAREST)
            contact.paste(thumb, (col * 160, row * 180), thumb)
            draw.line((col * 160, row * 180 + 152, col * 160 + 159, row * 180 + 152), fill='#bcc5ad')
            draw.text((col * 160 + 4, row * 180 + 160), f'{row + 1}:{col + 1}', fill='white')
        animation = []
        for col in SEQUENCES[name][row]:
            canvas = Image.new('RGB', (160, 160), '#7b8375')
            thumb = direction[col].resize((160, 160), Image.Resampling.NEAREST)
            canvas.paste(thumb, (0, 0), thumb)
            animation.append(canvas)
        animation[0].save(PREVIEW / f'{name}_{["south", "west", "east", "north"][row]}.gif',
                          save_all=True, append_images=animation[1:], duration=140, loop=0)
    contact.save(PREVIEW / f'{name}_contact.png')
    (PREVIEW / f'{name}_inspection.json').write_text(json.dumps(report, indent=2))
    print(name, report['source_size'], 'alpha', report['alpha_extrema'])


if __name__ == '__main__':
    OUT.mkdir(parents=True, exist_ok=True)
    PREVIEW.mkdir(parents=True, exist_ok=True)
    for name in ('male', 'female'):
        prepare(name)
