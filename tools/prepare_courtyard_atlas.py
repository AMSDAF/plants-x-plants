"""Recorta e reúne as folhas fornecidas para o mapa do pátio."""

import argparse
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw


def place(atlas, sheet, box, destination, size, *, opaque=False, outline=None):
    sprite = sheet.crop(box)
    if opaque:
        sprite.putalpha(255)
    elif outline:
        mask = Image.new("L", sprite.size)
        ImageDraw.Draw(mask).polygon(outline, fill=255)
        # Descarta os fragmentos de água entre as tábuas e os postes.
        red, green, _, _ = sprite.split()
        water = ImageChops.subtract(green, red).point(lambda delta: 0 if delta else 255)
        sprite.putalpha(ImageChops.multiply(mask, water))
    else:
        sprite.putalpha(sprite.getchannel("A").point(lambda alpha: 255 if alpha >= 185 else 0))
    atlas.paste(sprite.resize(size, Image.Resampling.NEAREST), destination)


def prepare(sources, output):
    garden = Image.open(sources / "Conjunto de Tiles de Jardim Botânico.png").convert("RGBA")
    buildings = Image.open(sources / "Conjunto de Edifícios e Ferramentas RPG.png").convert("RGBA")
    greenhouse = Image.open(sources / "Estufas Pixel Art e Válvulas Industriais.png").convert("RGBA")
    water = Image.open(sources / "Tileset RPG de Água e Pontes.png").convert("RGBA")
    props = Image.open(sources / "Kit de Cenários Pixel Art Retro.png").convert("RGBA")
    npc_path = sources / "Trio de NPCs da Vila Verde.png"
    atlas = Image.new("RGBA", (512, 512))

    # Pisos usam o miolo das peças para evitar contornos repetidos.
    place(atlas, garden, (318, 20, 387, 93), (0, 0), (16, 16), opaque=True)
    place(atlas, garden, (606, 20, 678, 92), (16, 0), (16, 16), opaque=True)
    place(atlas, water, (61, 70, 176, 183), (32, 0), (16, 16), opaque=True)
    place(atlas, water, (544, 550, 593, 663), (48, 0), (8, 16), opaque=True)
    place(atlas, garden, (702, 13, 786, 100), (64, 0), (32, 32), opaque=True)

    place(atlas, buildings, (10, 16, 588, 536), (0, 32), (101, 91))
    place(atlas, buildings, (598, 40, 1319, 536), (112, 32), (132, 91))
    place(atlas, buildings, (1333, 120, 1764, 536), (256, 32), (73, 70))
    place(atlas, greenhouse, (16, 12, 1164, 644), (0, 136), (188, 103))
    place(atlas, greenhouse, (1190, 125, 1763, 642), (208, 136), (107, 96))

    # A madeira é recortada no contorno para compartilhar o mesmo canal.
    place(atlas, water, (911, 157, 1407, 401), (0, 254), (79, 50), outline=[
        (3, 7), (32, 3), (43, 27), (150, 27), (158, 12), (187, 12),
        (195, 27), (300, 27), (309, 12), (339, 12), (347, 27), (450, 27),
        (455, 5), (481, 2), (492, 15), (492, 232), (481, 242), (455, 239),
        (450, 226), (347, 226), (339, 241), (309, 241), (300, 226),
        (195, 226), (187, 241), (158, 241), (150, 226), (43, 226),
        (34, 241), (8, 240), (2, 228)
    ])
    place(atlas, water, (917, 648, 971, 875), (88, 254), (10, 48), outline=[
        (9, 2), (43, 2), (53, 14), (53, 74), (42, 82), (42, 150),
        (52, 160), (52, 222), (7, 225), (1, 211), (1, 14)
    ])
    place(atlas, water, (1340, 648, 1396, 875), (104, 254), (10, 48), outline=[
        (8, 4), (46, 2), (55, 13), (55, 219), (47, 225), (10, 225),
        (1, 215), (1, 157), (10, 149), (10, 83), (1, 76), (1, 14)
    ])

    place(atlas, garden, (13, 904, 142, 1054), (0, 316), (50, 60))
    place(atlas, garden, (145, 901, 271, 1051), (56, 316), (50, 60))
    place(atlas, garden, (358, 938, 414, 1000), (112, 316), (24, 22))
    place(atlas, garden, (1187, 817, 1317, 915), (144, 316), (60, 46))
    place(atlas, props, (20, 201, 229, 473), (216, 316), (23, 30))
    place(atlas, props, (975, 332, 1137, 479), (248, 316), (18, 16))
    place(atlas, props, (1323, 175, 1758, 476), (280, 316), (48, 33))
    place(atlas, props, (241, 284, 503, 477), (336, 316), (27, 20))
    place(atlas, greenhouse, (326, 669, 585, 875), (376, 316), (25, 20))
    place(atlas, props, (1147, 285, 1307, 479), (408, 316), (19, 23))
    npcs = Image.open(npc_path).convert("RGBA")
    for index, (left, right) in enumerate(((0, 820), (820, 1360), (1360, npcs.width))):
        sprite = npcs.crop((left, 0, right, npcs.height))
        bounds = sprite.getchannel("A").point(lambda alpha: 255 if alpha >= 64 else 0).getbbox()
        sprite = sprite.crop(bounds)
        # Resolve os detalhes na escala de jogo e reserva três pixels acima e abaixo.
        width = round(50 * sprite.width / sprite.height)
        sprite = sprite.resize((width, 50), Image.Resampling.BOX)
        sprite.putalpha(sprite.getchannel("A").point(lambda alpha: 255 if alpha >= 112 else 0))
        atlas.paste(sprite, (64 * index + (48 - width) // 2, 387))
    atlas.save(output, optimize=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sources", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    prepare(args.sources, args.output)
