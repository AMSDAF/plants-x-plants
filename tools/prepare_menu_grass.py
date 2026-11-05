"""Extrai touceiras do fundo aprovado e prepara vento localizado, sem alterar a fonte."""
from pathlib import Path
import argparse
import hashlib
import json
import math

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage
from scipy.spatial import cKDTree

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'assets/sprites/menu_background_v2.png'
if not SOURCE.exists():
    SOURCE = ROOT / 'assets/sprites/menu_backround_v2.png'
PREVIEW = ROOT / 'build/menu_grass_preview'
BACKGROUND = ROOT / 'assets/sprites/menu_grass_background.png'
ATLAS = ROOT / 'assets/sprites/menu_grass_atlas.png'
FRAME_COUNT = 32
FPS = 8
COLUMNS = 8
PAGE_WIDTH = 512
# Caixas inspecionadas na fonte; amplitude em pixels da fonte, não da janela.
REGIONS = [
    ('far_left', (465, 110, 528, 146), 2.5),
    ('upper_left', (359, 199, 439, 247), 3.2),
    ('upper_right', (1210, 187, 1283, 233), 3.0),
    ('upper_middle', (940, 152, 1016, 201), 3.0),
    ('middle_left', (278, 278, 385, 363), 5.0),
    ('middle_right', (1241, 312, 1338, 388), 5.2),
    ('rock_left', (369, 384, 438, 429), 3.1),
    ('trunk_right', (1365, 393, 1458, 454), 4.0),
    ('clearing_left_upper', (269, 484, 414, 591), 8.0),
    ('clearing_right_upper', (1220, 460, 1358, 556), 7.0),
    ('clearing_left', (220, 606, 343, 714), 7.6),
    ('clearing_right', (1238, 683, 1391, 790), 8.5),
    ('clearing_right_small', (1122, 582, 1211, 641), 4.0),
    ('bottom_left', (443, 829, 554, 916), 7.0),
    ('bottom_right', (1119, 802, 1270, 918), 8.5),
    ('lower_left_small', (367, 780, 473, 834), 4.2),
    ('edge_right', (1541, 522, 1630, 583), 4.3),
]


def align(value, multiple=8):
    return math.ceil(value / multiple) * multiple


def extract(pixels, name, box):
    x0, y0, x1, y1 = box
    patch = pixels[y0:y1, x0:x1]
    r, g, b = [patch[:, :, c].astype(float) for c in range(3)]
    # Sementes locais de verde escuro, sem incluir rocha, terra ou sombra horizontal.
    seed = (g - r > 20) & (r < 105) & (g > 40) & (g > r * 1.26)
    candidate = (g - r > 17) & (r < 118) & (g > r * 1.18)
    highlight = (b < 52) & (r - b > 70) & (g - b > 85) & (g > r + 8)
    seed |= highlight
    candidate |= highlight | ((b < 55) & (r - b > 70) & (g - b > 85) & (g > r + 8))
    mask = ndimage.binary_propagation(seed, mask=candidate)
    mask = ndimage.binary_closing(mask, structure=np.ones((3, 3)))
    mask = ndimage.binary_fill_holes(mask)
    if name == 'clearing_left':
        # A copa à esquerda encobre parte do chão; não entra na máscara da grama.
        yy, xx = np.mgrid[y0:y1, x0:x1]
        mask &= (xx >= 227) & ((yy < 684) | (xx >= 235))
    if name == 'upper_left':
        yy, xx = np.mgrid[y0:y1, x0:x1]
        mask &= (yy >= 211) | (xx >= 390)
    labels, _ = ndimage.label(mask)
    sizes = np.bincount(labels.ravel())
    # Fragmentos que tocam a caixa pertencem a copas vizinhas cortadas pela seleção.
    borders = np.unique(np.concatenate((labels[0], labels[-1], labels[:, 0], labels[:, -1])))
    ids = np.flatnonzero(sizes >= 16)
    ids = ids[(ids != 0) & ~np.isin(ids, borders)]
    mask &= np.isin(labels, ids)
    assert mask.any(), name
    # O preenchimento de lacunas não pode incorporar pixels de pedra ou terra.
    mask &= g > r + 8
    return mask


def reconstruct(pixels, mask, box, base_y):
    x0, y0, x1, y1 = box
    margin = 40
    area = (max(0, x0 - margin), max(0, y0 - margin), min(pixels.shape[1], x1 + margin), min(pixels.shape[0], y1 + margin))
    ax, ay, bx, by = area
    patch = pixels[ay:by, ax:bx]
    r, g, b = [patch[:, :, c].astype(float) for c in range(3)]
    local_mask = np.zeros(patch.shape[:2], dtype=bool)
    local_mask[y0-ay:y1-ay, x0-ax:x1-ax] = mask
    # O halo da fonte é removido só junto às lâminas; rocha/terra/árvore não são doadores.
    removal = ndimage.binary_dilation(local_mask, iterations=3) & (g > r + 1)
    ground = (r > 115) & (g > r + 2) & (g < r + 25) & (b > 52) & ~removal
    assert ground.any()
    yy, xx = np.mgrid[:patch.shape[0], :patch.shape[1]].astype(float)
    xx = (xx - patch.shape[1] / 2) / patch.shape[1]
    yy = (yy - patch.shape[0] / 2) / patch.shape[0]
    basis = np.stack((np.ones_like(xx), xx, yy, xx*xx, xx*yy, yy*yy), axis=-1)
    design = basis[ground]
    colors = patch[ground].astype(float)
    coefficients = np.linalg.lstsq(design, colors, rcond=None)[0]
    for _ in range(4):
        residual = np.linalg.norm(colors - design @ coefficients, axis=1)
        weights = np.minimum(1, 4 / np.maximum(residual, 0.01))
        coefficients = np.linalg.lstsq(design * weights[:, None], colors * weights[:, None], rcond=None)[0]
    predicted = basis @ coefficients
    _, nearest = ndimage.distance_transform_edt(~ground, return_indices=True)
    # Mantém textura local e escolhe cores realmente presentes nas amostras de chão.
    texture = patch[nearest[0], nearest[1]].astype(float) - predicted[nearest[0], nearest[1]]
    desired = predicted[removal] + 0.5 * texture[removal]
    palette = np.unique(patch[ground], axis=0)
    tree = cKDTree(palette.astype(float))
    _, color_index = tree.query(desired)
    restored = patch.copy()
    restored[removal] = palette[color_index]
    # Na base imóvel, clona também a sombra horizontal, sem clarear seu contorno.
    root_ground = (r > 85) & (g > r + 3) & (g < r + 30) & (g < r * 1.28) & ~removal
    _, root_nearest = ndimage.distance_transform_edt(~root_ground, return_indices=True)
    root_zone = removal & (np.mgrid[:patch.shape[0], :patch.shape[1]][0] + ay >= base_y - 6)
    restored[root_zone] = patch[root_nearest[0][root_zone], root_nearest[1][root_zone]]
    return area, restored, removal


def signal(frame, phase):
    theta = 2 * math.pi * frame / FRAME_COUNT - phase
    # Rajada para a direita, com retorno lento; atraso comum dependente da posição.
    return (0.5 - 0.5 * math.cos(theta)) * (0.82 + 0.18 * math.sin(theta + 0.4))


def runtime_preview():
    frames = [Image.open(PREVIEW / f'title_{i:02d}.png').convert('RGB') for i in range(FRAME_COUNT)]
    palette = frames[0].quantize(colors=256)
    indexed = [frame.quantize(palette=palette, dither=Image.Dither.NONE) for frame in frames]
    indexed[0].save(PREVIEW / 'title_game.gif', save_all=True, append_images=indexed[1:], duration=125, loop=0)
    report = json.loads((PREVIEW / 'inspection.json').read_text(encoding='utf-8'))
    featured = [r for r in report['regions'] if r['name'] in ('clearing_left_upper', 'clearing_left', 'clearing_right', 'bottom_right')]
    contact = Image.new('RGB', (1280, len(featured)*240), '#263b26')
    draw = ImageDraw.Draw(contact)
    scale = 1280/1672
    for row, region in enumerate(featured):
        x0,y0,x1,y1 = region['selection']
        box = (round((x0-12)*scale), round((y0-6)*scale), round((x1+12)*scale), round((y1+6)*scale))
        for column, index in enumerate((31, 0, 1, region['maximum_frame'])):
            crop = frames[index].crop(box)
            factor = min(305/crop.width, 210/crop.height)
            crop = crop.resize((round(crop.width*factor), round(crop.height*factor)), Image.Resampling.NEAREST)
            contact.paste(crop, (column*320, row*240+22))
            draw.text((column*320+3,row*240+3), f'{region["name"]} / {index}', fill='white')
    contact.save(PREVIEW / 'game_closeup.png')
    print('GIF do jogo e contato ampliado em build/menu_grass_preview/')


def prepare():
    PREVIEW.mkdir(parents=True, exist_ok=True)
    source_digest = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    original = Image.open(SOURCE).convert('RGB')
    assert original.size == (1672, 941)
    pixels = np.array(original)
    background = pixels.copy()
    total_removal = np.zeros(pixels.shape[:2], dtype=bool)
    total_mask = np.zeros_like(total_removal)
    regions = []
    annotated = original.copy()
    annotation = ImageDraw.Draw(annotated)
    for index, (name, box, amplitude) in enumerate(REGIONS):
        mask = extract(pixels, name, box)
        x0, y0, x1, y1 = box
        ys, xs = np.where(mask)
        base_y = int(ys.max()) + y0
        origin = (x0 + int(xs.min()) - 12, y0 + int(ys.min()) - 4)
        size = (align(int(xs.max() - xs.min() + 1) + 24), align(int(ys.max() - ys.min() + 1) + 8))
        layer = np.zeros((size[1], size[0], 4), dtype=np.uint8)
        sy, sx = ys + y0 - origin[1], xs + x0 - origin[0]
        layer[sy, sx, :3] = pixels[ys + y0, xs + x0]
        layer[sy, sx, 3] = 255
        area, restored, removal = reconstruct(pixels, mask, box, base_y)
        ax, ay, bx, by = area
        background[ay:by, ax:bx][removal] = restored[removal]
        total_removal[ay:by, ax:bx] |= removal
        total_mask[y0:y1, x0:x1] |= mask
        phase = (origin[0] / 1672 * 1.45 + origin[1] / 941 * 0.35)
        tip_offsets = np.rint([amplitude * signal(f, phase) for f in range(FRAME_COUNT)]).astype(int)
        cyclic_steps = np.abs(np.roll(tip_offsets, -1) - tip_offsets)
        assert cyclic_steps.max() <= 2, name
        regions.append({'name': name, 'selection': box, 'origin': origin, 'size': size,
                        'pivot': [float(np.mean(xs[ys >= ys.max() - 2]) + x0), base_y],
                        'amplitude': amplitude, 'phase': phase, 'layer': layer,
                        'maximum_tip_step': int(cyclic_steps.max()),
                        'loop_tip_step': int(cyclic_steps[-1]),
                        'mask_pixels': int(mask.sum()), 'removal_pixels': int(removal.sum()),
                        'maximum_frame': int(np.argmax([signal(f, phase) for f in range(FRAME_COUNT)]))})
        Image.fromarray(np.uint8(mask) * 255).save(PREVIEW / f'{index:02d}_{name}_mask.png')
        Image.fromarray(layer).save(PREVIEW / f'{index:02d}_{name}_layer.png')
        annotation.rectangle(box, outline='#ef674c', width=2)
        annotation.text((x0, y0-12), f'{index+1}: {name}', fill='#ffffff', stroke_width=1, stroke_fill='#192d22')
    assert np.array_equal(background[~total_removal], pixels[~total_removal])
    assert np.all(background[total_mask][:, 0] > 85), 'Silhueta escura residual no fundo.'
    rgb = lambda data: data[:, :, 0].astype(np.uint32)*65536 + data[:, :, 1].astype(np.uint32)*256 + data[:, :, 2]
    assert np.isin(np.unique(rgb(background)), np.unique(rgb(pixels))).all(), 'Cor não presente na fonte.'
    Image.fromarray(background).save(BACKGROUND)
    Image.fromarray(np.uint8(total_removal) * 255).save(PREVIEW / 'reconstruction_mask.png')
    Image.fromarray(np.uint8(total_mask) * 255).save(PREVIEW / 'vegetation_mask.png')
    Image.fromarray(background).save(PREVIEW / 'reconstructed.png')
    annotated.save(PREVIEW / 'regions.png')
    # Empacotamento por prateleiras de alturas decrescentes, sem telas inteiras por fase.
    cursor_x = cursor_y = row_height = 0
    for region in sorted(regions, key=lambda item: item['size'][1], reverse=True):
        w, h = region['size']
        if cursor_x + w > PAGE_WIDTH:
            cursor_y += row_height
            cursor_x = row_height = 0
        region['source'] = [cursor_x, cursor_y, w, h]
        cursor_x += w
        row_height = max(row_height, h)
    page_height = align(cursor_y + row_height)
    atlas = Image.new('RGBA', (PAGE_WIDTH * COLUMNS, page_height * (FRAME_COUNT // COLUMNS)))
    scenes = []
    for frame in range(FRAME_COUNT):
        scene = Image.fromarray(background).convert('RGBA')
        for region in regions:
            layer = region['layer']
            h, w = layer.shape[:2]
            y, x = np.mgrid[:h, :w]
            base = region['pivot'][1] - region['origin'][1]
            weight = np.clip((base - 4 - y) / max(base - 5, 1), 0, 1) ** 1.7
            dx = np.rint(region['amplitude'] * signal(frame, region['phase']) * weight).astype(int)
            sx = x - dx
            valid = (sx >= 0) & (sx < w)
            animated = np.zeros_like(layer)
            animated[valid] = layer[y[valid], sx[valid]]
            assert np.array_equal(animated[max(0, int(base)-4):], layer[max(0, int(base)-4):]), region['name']
            assert not animated[:, 0, 3].any() and not animated[:, -1, 3].any()
            assert not animated[0, :, 3].any() and not animated[-1, :, 3].any()
            sprite = Image.fromarray(animated)
            rx, ry, rw, rh = region['source']
            atlas.paste(sprite, ((frame % COLUMNS) * PAGE_WIDTH + rx, (frame // COLUMNS) * page_height + ry))
            scene.alpha_composite(sprite, region['origin'])
        scenes.append(scene.convert('RGB'))
    atlas.save(ATLAS)
    # Paleta comum apenas no GIF; texturas de execução mantêm os RGB da fonte.
    shown = [scene.resize((1280, 720), Image.Resampling.NEAREST) for scene in scenes]
    palette = shown[0].quantize(colors=256)
    indexed = [frame.quantize(palette=palette, dither=Image.Dither.NONE) for frame in shown]
    indexed[0].save(PREVIEW / 'scene.gif', save_all=True, append_images=indexed[1:], duration=125, loop=0)
    contact = Image.new('RGB', (1200, len(regions) * 150), '#263b26')
    draw = ImageDraw.Draw(contact)
    for row, region in enumerate(regions):
        box = region['selection']
        for column, frame in enumerate((31, 0, 1, region['maximum_frame'], (region['maximum_frame']+8)%32)):
            crop = scenes[frame].crop((box[0]-12, box[1]-6, box[2]+12, box[3]+6))
            crop.thumbnail((230, 125), Image.Resampling.NEAREST)
            contact.paste(crop, (column*240, row*150+20))
            draw.text((column*240+4, row*150+2), f'{region["name"]} / {frame}', fill='white')
    contact.save(PREVIEW / 'contact.png')
    featured = [r for r in regions if r['name'] in ('clearing_left_upper', 'clearing_right_upper', 'clearing_left', 'clearing_right', 'bottom_left', 'bottom_right')]
    closeup = Image.new('RGB', (1280, len(featured)*240), '#263b26')
    draw = ImageDraw.Draw(closeup)
    for row, region in enumerate(featured):
        box = region['selection']
        for column, frame in enumerate((31, 0, 1, region['maximum_frame'])):
            crop = scenes[frame].crop((box[0]-12, box[1]-6, box[2]+12, box[3]+6))
            scale = min(305/crop.width, 210/crop.height)
            crop = crop.resize((round(crop.width*scale), round(crop.height*scale)), Image.Resampling.NEAREST)
            closeup.paste(crop, (column*320, row*240+22))
            draw.text((column*320+3, row*240+3), f'{region["name"]} / {frame}', fill='white')
    closeup.save(PREVIEW / 'closeup.png')
    scenes[0].save(PREVIEW / 'scene_00.png')
    scenes[16].save(PREVIEW / 'scene_16.png')
    report = {'source': str(SOURCE.relative_to(ROOT)), 'source_sha256': source_digest,
              'frame_count': FRAME_COUNT, 'fps': FPS, 'page_size': [PAGE_WIDTH, page_height],
              'atlas_size': atlas.size, 'atlas_rgba_bytes': atlas.width*atlas.height*4,
              'changed_background_pixels': int(total_removal.sum()), 'outside_removal_unchanged': True,
              'fixed_root_rows': 5, 'regions': [{k: v for k,v in r.items() if k != 'layer'} for r in regions]}
    (PREVIEW / 'inspection.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    header = ['/* Geometria e parâmetros gerados por tools/prepare_menu_grass.py. */',
              '#ifndef MENU_GRASS_DATA_H', '#define MENU_GRASS_DATA_H', '#include "raylib.h"',
              f'#define MENU_GRASS_REGION_COUNT {len(regions)}', f'#define MENU_GRASS_FRAME_COUNT {FRAME_COUNT}',
              f'#define MENU_GRASS_FPS {FPS}.0f', f'#define MENU_GRASS_COLUMNS {COLUMNS}',
              f'#define MENU_GRASS_PAGE_WIDTH {PAGE_WIDTH}', f'#define MENU_GRASS_PAGE_HEIGHT {page_height}',
              '#define MENU_GRASS_SOURCE_WIDTH 1672', '#define MENU_GRASS_SOURCE_HEIGHT 941',
              'typedef struct { Rectangle source; Vector2 origin; Vector2 pivot; float amplitude; float phase; } MenuGrassRegion;',
              'static const MenuGrassRegion MENU_GRASS_REGIONS[MENU_GRASS_REGION_COUNT] = {']
    for r in regions:
        source = ', '.join(str(v) for v in r['source'])
        origin = ', '.join(str(v) for v in r['origin'])
        pivot = ', '.join(f'{v:.3f}f' for v in r['pivot'])
        header.append(f'    {{{{{source}}}, {{{origin}}}, {{{pivot}}}, {r["amplitude"]:.3f}f, {r["phase"]:.3f}f}}, /* {r["name"]} */')
    header += ['};', '#endif', '']
    (ROOT / 'include/menu_grass_data.h').write_text('\n'.join(header), encoding='utf-8')
    assert hashlib.sha256(SOURCE.read_bytes()).hexdigest() == source_digest
    print(f'{len(regions)} regiões, atlas {atlas.size}, {atlas.width*atlas.height*4/1048576:.2f} MiB, {total_removal.sum()} pixels reconstruídos')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime-preview', action='store_true', help='Monta GIF das capturas reais de menu_screen_check.')
    args = parser.parse_args()
    if args.runtime_preview:
        runtime_preview()
    else:
        prepare()
