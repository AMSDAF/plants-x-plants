"""Recorta e limpa as camadas da seleção, preservando as fontes e suas cores."""
from pathlib import Path
import hashlib
import json
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets/sprites/selection/prepared'
PREVIEW = ROOT / 'build/selection_preview'
GRIDS = {'clouds': (2, 3), 'canopies': (3, 2), 'ground_foliage': (3, 2), 'leaf_particles': (4, 3)}
CELL = 512


def clean(image):
    pixels = np.array(image.convert('RGBA'))
    alpha = pixels[:, :, 3]
    # Reconstrói o objeto a partir do alfa sólido, sem classificar a cor da arte.
    labels, _ = ndimage.label(alpha >= 210)
    sizes = np.bincount(labels.ravel())
    keep_ids = np.flatnonzero(sizes >= 18)
    keep_ids = keep_ids[keep_ids != 0]
    seed = np.isin(labels, keep_ids)
    solid = ndimage.binary_propagation(seed, mask=alpha >= 128)
    edge = ndimage.binary_dilation(solid, iterations=1) & (alpha >= 32)
    pixels[~edge] = 0
    # Evita névoa de baixa opacidade; mantém antialias somente junto ao contorno.
    pixels[:, :, 3][solid & (alpha >= 224)] = 255
    return Image.fromarray(pixels)


def source_path(name):
    preferred = ROOT / f'assets/sprites/selection/{name}.png'
    return preferred if preferred.exists() else ROOT / f'assets/sprites/{name}.png'


def preview(name, image):
    thumb = image.copy()
    thumb.thumbnail((640, 640))
    contact = Image.new('RGB', (thumb.width * 2, thumb.height))
    for index, color in enumerate(('#18261f', '#eee8cf')):
        bg = Image.new('RGBA', thumb.size, color)
        bg.alpha_composite(thumb)
        contact.paste(bg.convert('RGB'), (index * thumb.width, 0))
    contact.save(PREVIEW / f'{name}_clean.png')


def prepare():
    OUT.mkdir(parents=True, exist_ok=True)
    PREVIEW.mkdir(parents=True, exist_ok=True)
    report = {}
    for name in ('background_base', 'tree_trunks', 'portrait_frame', *GRIDS):
        source = source_path(name)
        original = Image.open(source).convert('RGBA')
        record = {'source': str(source.relative_to(ROOT)), 'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
                  'size': original.size, 'alpha': original.getchannel('A').getextrema(), 'sprites': []}
        if name == 'background_base':
            image = original.copy()
        elif name in GRIDS:
            cols, rows = GRIDS[name]
            image = Image.new('RGBA', (cols * CELL, rows * CELL))
            for i in range(cols * rows):
                x, y = i % cols, i // cols
                # Separações medidas: o terceiro dossel cruza a metade nominal.
                x_edges = (0, 510, 1019, 1536) if name == 'ground_foliage' else tuple(j * original.width // cols for j in range(cols + 1))
                y_edges = (0, 500, 887) if name == 'canopies' else (0, 520, 1024) if name == 'ground_foliage' else tuple(j * original.height // rows for j in range(rows + 1))
                cell_bounds = (x_edges[x], y_edges[y], x_edges[x + 1], y_edges[y + 1])
                cell = clean(original.crop(cell_bounds))
                bounds = cell.getbbox()
                assert bounds and bounds[0] > 0 and bounds[1] > 0 and bounds[2] < cell.width and bounds[3] < cell.height, (name, i, bounds)
                sprite = cell.crop(bounds)
                sprite.thumbnail((CELL - 8, CELL - 8), Image.Resampling.LANCZOS)
                image.paste(sprite, (x * CELL + 4, y * CELL + 4))
                record['sprites'].append({'index': i, 'source_bounds': [bounds[0] + cell_bounds[0], bounds[1] + cell_bounds[1], bounds[2] + cell_bounds[0], bounds[3] + cell_bounds[1]],
                                          'rect': [x * CELL + 4, y * CELL + 4, sprite.width, sprite.height]})
        else:
            image = clean(original)
            if name == 'portrait_frame':
                # A abertura deve ficar vazia; a placa de madeira permanece sólida.
                assert image.getpixel((512, 768))[3] == 0
                assert image.getpixel((512, 1400))[3] == 255
        image.save(OUT / f'{name}.png')
        preview(name, image)
        record['prepared_alpha'] = image.getchannel('A').getextrema()
        report[name] = record
    (PREVIEW / 'preparation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    # Retângulos medidos: a execução não presume o tamanho nominal da arte gerada.
    lines = ['/* Retângulos gerados por tools/prepare_selection_scene.py; não editar manualmente. */',
             '#ifndef SELECTION_SCENE_DATA_H', '#define SELECTION_SCENE_DATA_H', '', '#include "raylib.h"', '']
    for name in GRIDS:
        lines.append(f'static const Rectangle {name.upper()}_SOURCES[] = {{')
        for sprite in report[name]['sprites']:
            lines.append('    {' + ', '.join(map(str, sprite['rect'])) + '},')
        lines.append('};')
    lines.append('#endif')
    (ROOT / 'include/selection_scene_data.h').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print('Camadas preparadas; recortes e hashes em build/selection_preview/preparation.json')


if __name__ == '__main__':
    prepare()
