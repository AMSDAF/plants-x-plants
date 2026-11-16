"""Recorta as fontes fornecidas e gera retratos e atlas ancorados pelos pés."""
import argparse
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage


OUT = Path(__file__).resolve().parents[1] / "assets/sprites/trainers"
CELL = 256
FEET = (128, 240)
# Ordem do jogo: sul, oeste, leste, norte.
SOURCE_ROWS = (0, 2, 3, 1)


def components(path):
    pixels = np.array(Image.open(path).convert("RGBA"))
    labels, _ = ndimage.label(pixels[:, :, 3] >= 128)
    sizes = np.bincount(labels.ravel())
    regions = [
        (label, box)
        for label, box in enumerate(ndimage.find_objects(labels), 1)
        if sizes[label] >= pixels.shape[0] * pixels.shape[1] * 0.002
    ]
    return pixels, labels, regions


def crop_component(pixels, labels, region):
    label, box = region
    crop = pixels[box].copy()
    mask = labels[box] == label
    crop[~mask] = 0
    crop[mask, 3] = 255
    return Image.fromarray(crop)


def prepare_sheet(path, name):
    pixels, labels, regions = components(path)
    if len(regions) != 12:
        raise ValueError(f"{path}: esperados 12 quadros, encontrados {len(regions)}")
    regions.sort(key=lambda region: region[1][0].start)
    atlas = Image.new("RGBA", (3 * CELL, 4 * CELL))
    for target_row, source_row in enumerate(SOURCE_ROWS):
        row = regions[3 * source_row:3 * source_row + 3]
        row.sort(key=lambda region: region[1][1].start)
        for col, region in enumerate(row):
            frame = crop_component(pixels, labels, region)
            # O eixo do chapéu não oscila com a perna que toca o chão.
            hat = frame.crop((0, 0, frame.width, frame.height * 45 // 100)).getbbox()
            x = FEET[0] - (hat[0] + hat[2]) // 2
            y = FEET[1] - frame.height
            if min(x, y) < 0 or x + frame.width > CELL:
                raise ValueError(f"{path}: quadro maior que a célula")
            atlas.paste(frame, (col * CELL + x, target_row * CELL + y))
    atlas.save(OUT / f"{name}_botanical.png", optimize=True)


def prepare_portrait(path, name):
    pixels, labels, regions = components(path)
    if len(regions) != 1:
        raise ValueError(f"{path}: esperado um personagem isolado")
    portrait = crop_component(pixels, labels, regions[0])
    portrait.thumbnail((256, 256), Image.Resampling.NEAREST)
    portrait = portrait.crop(portrait.getbbox())
    portrait.save(OUT / f"{name}_botanical_portrait.png", optimize=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for argument in ("male_sheet", "female_sheet", "male_portrait", "female_portrait"):
        parser.add_argument(argument, type=Path)
    args = parser.parse_args()
    for trainer in ("male", "female"):
        prepare_sheet(getattr(args, f"{trainer}_sheet"), trainer)
        prepare_portrait(getattr(args, f"{trainer}_portrait"), trainer)
