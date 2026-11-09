"""Prepara repouso do mascote usando somente pixels da fonte aprovada, sem reamostragem suave."""
from pathlib import Path
import argparse
import hashlib
import json

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'assets/sprites/menu_mascot.png'
OUTPUT = ROOT / 'assets/sprites/menu_mascot_idle.png'
PREVIEW = ROOT / 'build/menu_mascot_preview'
FRAME = 1024
COLUMNS = 4
FPS = 6
CROP = (218, 101, 1186, 1095)
OFFSET = (28, 16)
# Deslocamentos em pixels da fonte: quatro pixels equivalem a cerca de um na janela.
BREATH = (0, 0, -4, -4, -4, -4, 0, 0)
LEFT_SWAY = (0, 4, 8, 4, 0, -4, -8, -4)
RIGHT_SWAY = (0, 4, 4, 0, -4, -8, -8, -4)
LEFT_FIST = (0, 0, -4, -4, 0, 0, 4, 4)
RIGHT_FIST = (0, -4, -4, 0, 0, 4, 4, 0)
DURATIONS = (170, 170, 160, 170, 170, 160, 170, 170)


def save_gif(frames, path):
    # Paleta só no GIF diagnóstico; atlas e quadros permanecem em RGBA.
    palette = frames[0].quantize(colors=256)
    indexed = [f.convert('RGB').quantize(palette=palette, dither=Image.Dither.NONE) for f in frames]
    indexed[0].save(path, save_all=True, append_images=indexed[1:], duration=DURATIONS, loop=0)


def runtime_preview():
    for screen in ('title', 'menu'):
        frames = [Image.open(PREVIEW / f'{screen}_{i:02d}.png').convert('RGB') for i in range(8)]
        arrays = [np.array(frame) for frame in frames]
        assert len({hashlib.sha256(frame.tobytes()).hexdigest() for frame in frames}) == 8
        assert all(np.array_equal(frame[208:], arrays[0][208:]) for frame in arrays), 'Pés ou sombra moveram no jogo.'
        save_gif(frames, PREVIEW / f'{screen}_idle.gif')
    print('GIFs das capturas reais em build/menu_mascot_preview/')


def polygon_mask(size, points):
    image = Image.new('1', size)
    ImageDraw.Draw(image).polygon(points, fill=1)
    return np.array(image, dtype=bool)


def remap(layer, dx, dy, x, y):
    # Amostragem inteira: não cria cores, mistura de alfa ou contornos suavizados.
    sx = np.rint(x - dx).astype(int)
    sy = np.rint(y - dy).astype(int)
    valid = (sx >= 0) & (sx < layer.shape[1]) & (sy >= 0) & (sy < layer.shape[0])
    result = np.zeros_like(layer)
    result[valid] = layer[sy[valid], sx[valid]]
    return result


def composite(parts):
    result = np.zeros((1199, 1312, 4), dtype=np.uint8)
    for part in parts:
        present = part[:, :, 3] > 0
        result[present] = part[present]
    return result


def prepare():
    PREVIEW.mkdir(parents=True, exist_ok=True)
    source_digest = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    original = Image.open(SOURCE).convert('RGBA')
    assert original.size == (1312, 1199), 'Reinspecionar as máscaras se a fonte mudar.'
    pixels = np.array(original)
    alpha = pixels[:, :, 3]
    y, x = np.mgrid[:original.height, :original.width]
    visible = alpha > 32
    ys, xs = np.where(visible)
    assert (int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1) == CROP
    # O menu estático já usa este recorte; margem extra é reservada ao movimento.
    crop_mask = (x >= CROP[0]) & (x < CROP[2]) & (y >= CROP[1]) & (y < CROP[3])
    pixels[~crop_mask] = 0
    allowed_pixels = np.unique(pixels.view(np.uint32))

    left_leaf = (x < 628) & (y < 549)
    right_leaf = (y < 550) & ~left_leaf
    left_arm = polygon_mask(original.size, ((200, 670), (400, 670), (400, 770), (445, 770), (445, 868), (414, 886), (200, 886)))
    right_arm = polygon_mask(original.size, ((883, 680), (1060, 680), (1060, 907), (855, 907), (835, 850), (835, 795), (883, 795)))
    feet = y >= 900
    # As juntas têm pequena sobreposição no contorno escuro existente; não redesenha a arte.
    body = ~(left_leaf | right_leaf | (left_arm & (x < 411)) | (right_arm & (x > 882)))
    masks = {'body': body, 'left_leaf': left_leaf, 'right_leaf': right_leaf,
             'left_arm': left_arm, 'right_arm': right_arm, 'feet': feet}
    layers = {}
    for name, mask in masks.items():
        layer = pixels.copy()
        layer[~mask] = 0
        layers[name] = layer
        Image.fromarray(np.uint8(mask & visible) * 255).save(PREVIEW / f'mask_{name}.png')
    # O primeiro quadro deve preservar exatamente a fonte, incluindo alfa e cores.
    atlas = Image.new('RGBA', (FRAME * COLUMNS, FRAME * 2))
    frames, previews, records = [], [], []
    for i in range(8):
        body_dy = BREATH[i] * np.clip((900 - y) / 44, 0, 1)
        body_dy = np.rint(body_dy)
        moved_body = remap(layers['body'], 0, body_dy, x, y)
        left_weight = np.clip((549 - y) / (549 - 294), 0, 1) ** 2
        right_weight = np.clip((550 - y) / (550 - 101), 0, 1) ** 2
        leaf_left = remap(layers['left_leaf'], np.rint(LEFT_SWAY[i] * left_weight), BREATH[i], x, y)
        leaf_right = remap(layers['right_leaf'], np.rint(RIGHT_SWAY[i] * right_weight), BREATH[i], x, y)
        left_flex = np.clip((411 - x) / 100, 0, 1)
        right_flex = np.clip((x - 883) / 100, 0, 1)
        arm_left = remap(layers['left_arm'], 0, np.rint(body_dy + LEFT_FIST[i] * left_flex), x, y)
        arm_right = remap(layers['right_arm'], 0, np.rint(body_dy + RIGHT_FIST[i] * right_flex), x, y)
        frame_pixels = composite((moved_body, arm_left, arm_right, leaf_left, leaf_right))
        frame_pixels[feet] = pixels[feet]
        if i == 0:
            # Preserva inclusive o RGB das amostras totalmente transparentes da fonte.
            frame_pixels = pixels.copy()
        assert np.array_equal(frame_pixels[feet], pixels[feet])
        assert np.isin(np.unique(frame_pixels.view(np.uint32)), allowed_pixels).all(), 'Cor ou alfa não presente na fonte.'
        # A expressão inteira acompanha rigidamente o tórax, sem remapear olhos/boca.
        face_y = 680 + BREATH[i]
        assert np.array_equal(frame_pixels[face_y:face_y + 140, 480:830], pixels[680:820, 480:830])
        frame = Image.new('RGBA', (FRAME, FRAME))
        expanded = (CROP[0] - 16, CROP[1] - 16, CROP[2] + 16, CROP[3] + 16)
        frame.paste(Image.fromarray(frame_pixels).crop(expanded), (OFFSET[0] - 16, OFFSET[1] - 16))
        bbox = frame.getbbox()
        assert bbox and bbox[0] > 8 and bbox[1] > 8 and bbox[2] < FRAME - 8 and bbox[3] == 1010
        atlas.paste(frame, ((i % COLUMNS) * FRAME, (i // COLUMNS) * FRAME))
        frames.append(frame)
        # Exibe a mesma escala de fonte usada pelo menu: altura visível de 240 pixels.
        scaled = frame.resize((round(FRAME * 240 / 994),) * 2, Image.Resampling.NEAREST)
        bg = Image.new('RGBA', (280, 280), '#d8aa61')
        bg.alpha_composite(scaled, ((280 - scaled.width) // 2, 10))
        previews.append(bg.convert('RGB'))
        labels, _ = ndimage.label(frame_pixels[:, :, 3] > 128)
        sizes = np.bincount(labels.ravel())[1:]
        assert np.count_nonzero(sizes > 200) == 1, 'Parte grande desconectada da silhueta.'
        records.append({'frame': i, 'bounds': bbox, 'breath_source_pixels': BREATH[i],
                        'substantial_components': int(np.count_nonzero(sizes > 200))})
    assert len({hashlib.sha256(f.tobytes()).hexdigest() for f in frames}) == 8
    # Quadro adjacente à emenda não pode saltar mais que os demais passos.
    changes = [int(np.count_nonzero(np.any(np.array(frames[i]) != np.array(frames[(i + 1) % 8]), axis=2))) for i in range(8)]
    assert changes[-1] <= max(changes[:-1])
    atlas.save(OUTPUT)
    contact = Image.new('RGB', (280 * 4, 300 * 2), '#253b28')
    draw = ImageDraw.Draw(contact)
    for i, image in enumerate(previews):
        contact.paste(image, ((i % 4) * 280, (i // 4) * 300))
        draw.text(((i % 4) * 280 + 10, (i // 4) * 300 + 282), f'{i + 1} | {BREATH[i]} px fonte', fill='#f7edc6')
    contact.save(PREVIEW / 'contact.png')
    save_gif(previews, PREVIEW / 'idle.gif')
    seam = Image.new('RGB', (840, 280))
    for j, i in enumerate((7, 0, 1)):
        seam.paste(previews[i], (j * 280, 0))
    seam.save(PREVIEW / 'loop_boundary.png')
    joints = Image.new('RGB', (1200, 660), '#657b55')
    for column, i in enumerate((0, 2, 5)):
        bg = Image.new('RGBA', frames[i].size, '#657b55')
        bg.alpha_composite(frames[i])
        for row, box in enumerate(((170, 630, 360, 830), (610, 640, 820, 850), (340, 420, 600, 590))):
            joints.paste(bg.crop(box).convert('RGB'), (column * 400, row * 220))
    joints.save(PREVIEW / 'joints.png')
    for i, frame in enumerate(frames):
        frame.save(PREVIEW / f'frame_{i:02d}.png')
    report = {'source_sha256': source_digest, 'source_size': original.size, 'source_visible_bounds': CROP,
              'source_opaque_color_count': len(np.unique(pixels[alpha > 224, :3], axis=0)),
              'working_resolution': 'original; sem reducao ou conversao de paleta',
              'frame_size': [FRAME, FRAME], 'atlas_size': atlas.size, 'columns': COLUMNS, 'fps': FPS,
              'offset': OFFSET, 'fixed_baseline': 1010, 'feet_unchanged': True, 'face_rigid': True, 'source_palette_and_alpha_preserved': True,
              'joint_reconstruction': 'sobreposicao de pixels originais no contorno; nenhuma pintura ou preenchimento novo',
              'frame_changes_including_seam': changes, 'frames': records}
    (PREVIEW / 'inspection.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    # Constantes compartilhadas impedem divergência entre a preparação e o desenho.
    header = '''/* Geometria do atlas do mascote, gerada por tools/prepare_menu_mascot.py. */
#ifndef MENU_MASCOT_DATA_H
#define MENU_MASCOT_DATA_H
#define MENU_MASCOT_FRAME_SIZE 1024
#define MENU_MASCOT_FRAME_COUNT 8
#define MENU_MASCOT_COLUMNS 4
#define MENU_MASCOT_FPS 6.0f
#define MENU_MASCOT_CROP_WIDTH 968
#define MENU_MASCOT_CROP_HEIGHT 994
#define MENU_MASCOT_OFFSET_X 28
#define MENU_MASCOT_OFFSET_Y 16
#endif
'''
    (ROOT / 'include/menu_mascot_data.h').write_text(header, encoding='utf-8')
    assert hashlib.sha256(SOURCE.read_bytes()).hexdigest() == source_digest
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime-preview', action='store_true', help='Monta GIFs das capturas geradas por menu_screen_check.')
    args = parser.parse_args()
    if args.runtime_preview:
        runtime_preview()
    else:
        prepare()
