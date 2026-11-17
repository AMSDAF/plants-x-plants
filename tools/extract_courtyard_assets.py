"""Extrai referências e silhuetas do pátio sem alterar a imagem ou o jogo."""

import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets/maps/botanical_courtyard.png"
OUTPUT = ROOT / "assets/maps/courtyard_extracted"

# Contornos em coordenadas da imagem original. São máscaras de edição, não colisões.
# Partes ocultas e objetos encostados permanecem como aparecem na referência.
ELEMENTS = [
    ("depot", "Depósito com ferramentas e caixas", "buildings", [(87, 284), (120, 277), (120, 194), (168, 152), (365, 152), (408, 192), (408, 297), (399, 301), (407, 390), (320, 390), (302, 374), (224, 373), (224, 393), (164, 393), (164, 381), (119, 382), (119, 367), (87, 367)]),
    ("maintenance_house", "Casa de manutenção com telhado verde", "buildings", [(90, 460), (120, 458), (120, 439), (162, 439), (162, 460), (276, 455), (294, 450), (328, 479), (321, 586), (294, 639), (212, 641), (211, 620), (100, 621), (93, 563), (77, 553)]),
    ("bridge_control_house", "Casa de controle com dreno", "buildings", [(711, 535), (725, 535), (725, 504), (736, 490), (763, 490), (763, 451), (798, 412), (868, 405), (914, 439), (914, 552), (758, 552), (758, 565), (711, 565)]),
    ("large_greenhouse", "Viveiro de vidro com trepadeiras", "buildings", [(1089, 186), (1120, 149), (1120, 131), (1195, 46), (1195, 13), (1223, 3), (1383, 3), (1393, 0), (1406, 3), (1555, 3), (1567, 32), (1583, 36), (1649, 111), (1656, 195), (1664, 198), (1664, 213), (1656, 213), (1656, 300), (1465, 300), (1465, 325), (1324, 325), (1324, 306), (1091, 306)]),
    ("small_greenhouse", "Estufa menor com tubulações", "buildings", [(1082, 672), (1082, 664), (1095, 649), (1208, 649), (1208, 580), (1228, 562), (1228, 539), (1284, 489), (1307, 473), (1554, 473), (1597, 509), (1597, 594), (1615, 594), (1634, 612), (1634, 672), (1624, 688), (1624, 724), (1598, 724), (1598, 679), (1414, 679), (1414, 687), (1357, 687), (1357, 679), (1233, 679), (1233, 694), (1211, 694), (1211, 710), (1134, 710), (1134, 672), (1112, 672), (1112, 709), (1082, 709)]),
    ("pergola", "Pergolado com flores pendentes", "structures", [(459, 69), (746, 69), (746, 83), (735, 85), (746, 103), (746, 118), (730, 119), (730, 197), (704, 197), (704, 133), (667, 133), (667, 195), (639, 195), (639, 133), (575, 133), (575, 196), (549, 196), (549, 134), (512, 134), (512, 199), (478, 199), (478, 120), (463, 119), (463, 102), (475, 94), (459, 85)]),
    ("south_gate", "Portão sul e pilares com lanternas", "structures", [(484, 800), (530, 800), (531, 825), (545, 825), (559, 817), (598, 817), (617, 828), (624, 799), (665, 799), (670, 902), (620, 902), (620, 890), (532, 890), (532, 902), (484, 902)]),
    ("east_gate", "Portão pequeno da estufa", "structures", [(1106, 533), (1134, 533), (1134, 549), (1149, 544), (1173, 545), (1181, 551), (1181, 533), (1206, 533), (1206, 599), (1180, 599), (1180, 588), (1134, 588), (1134, 598), (1106, 598)]),
    ("central_tree_planter", "Árvore central e canteiro amarelo", "vegetation", [(579, 274), (591, 256), (593, 238), (616, 225), (630, 212), (657, 213), (670, 222), (692, 226), (701, 242), (710, 247), (715, 272), (706, 290), (692, 305), (681, 309), (723, 309), (729, 326), (729, 397), (721, 403), (579, 403), (574, 390), (574, 326), (583, 311), (614, 309), (594, 298)]),
    ("tree_southwest", "Árvore do sudoeste", "vegetation", [(200, 768), (221, 745), (225, 725), (247, 707), (272, 700), (294, 713), (304, 732), (318, 740), (323, 764), (311, 786), (292, 798), (285, 816), (290, 824), (277, 827), (265, 818), (252, 825), (240, 820), (247, 802), (224, 797), (211, 785)]),
    ("tree_southeast", "Árvore do sudeste", "vegetation", [(1353, 775), (1371, 751), (1388, 746), (1396, 729), (1421, 721), (1448, 731), (1458, 746), (1477, 757), (1484, 783), (1473, 803), (1450, 819), (1441, 823), (1442, 836), (1454, 843), (1437, 847), (1423, 840), (1412, 848), (1399, 840), (1409, 826), (1384, 820), (1366, 804)]),
    ("shrub_southwest", "Arbusto junto ao muro sul", "vegetation", [(278, 894), (286, 878), (299, 869), (316, 864), (335, 873), (342, 888), (345, 904), (327, 913), (297, 913), (279, 907)]),
    ("shrub_southeast", "Arbusto junto ao canal sul", "vegetation", [(830, 896), (840, 883), (854, 873), (877, 873), (895, 884), (901, 902), (899, 918), (882, 928), (855, 928), (839, 917)]),
    ("white_flower_bush", "Arbusto de flores brancas", "vegetation", [(1507, 381), (1515, 365), (1528, 350), (1548, 343), (1570, 351), (1587, 370), (1594, 387), (1585, 401), (1567, 410), (1532, 409), (1513, 399)]),
    ("pink_flower_bush", "Arbusto de flores rosas", "vegetation", [(1468, 423), (1480, 404), (1498, 393), (1520, 392), (1543, 404), (1557, 426), (1553, 446), (1536, 461), (1508, 463), (1483, 449)]),
    ("flowerbed_southwest", "Canteiro sudoeste com banco", "planters", [(391, 640), (400, 621), (420, 611), (432, 602), (453, 610), (468, 603), (495, 614), (514, 626), (533, 647), (536, 697), (521, 711), (391, 711)]),
    ("flowerbed_southeast", "Canteiro sudeste de flores mistas", "planters", [(621, 648), (633, 627), (657, 618), (680, 607), (704, 619), (726, 610), (748, 617), (767, 604), (789, 610), (810, 622), (830, 641), (833, 691), (819, 710), (635, 717), (621, 701)]),
    ("planter_pink_west", "Floreira rosa oeste", "planters", [(432, 278), (442, 263), (456, 251), (478, 249), (497, 256), (512, 274), (520, 287), (520, 333), (509, 345), (448, 345), (439, 333), (432, 328)]),
    ("planter_pink_east", "Floreira rosa leste", "planters", [(779, 293), (789, 278), (808, 268), (831, 260), (854, 269), (869, 290), (876, 306), (876, 340), (861, 350), (796, 350), (787, 340), (779, 335)]),
    ("planter_gate_left", "Floreira esquerda do portão sul", "planters", [(374, 881), (382, 864), (401, 855), (425, 849), (453, 856), (473, 870), (484, 884), (484, 913), (374, 913)]),
    ("planter_gate_right", "Floreira direita do portão sul", "planters", [(669, 880), (680, 863), (705, 851), (731, 850), (753, 860), (772, 877), (780, 890), (780, 915), (669, 915)]),
    ("planter_greenhouse_left", "Floreira esquerda do viveiro", "planters", [(1189, 315), (1200, 303), (1222, 290), (1247, 291), (1272, 304), (1288, 319), (1288, 347), (1189, 347)]),
    ("planter_greenhouse_right", "Floreira direita do viveiro", "planters", [(1510, 316), (1524, 303), (1548, 291), (1571, 290), (1598, 305), (1614, 319), (1614, 347), (1510, 347)]),
    ("planter_small_left", "Floreira esquerda da estufa menor", "planters", [(1257, 686), (1269, 672), (1288, 660), (1307, 667), (1329, 682), (1329, 716), (1257, 716)]),
    ("planter_small_right", "Floreira direita da estufa menor", "planters", [(1448, 684), (1462, 669), (1485, 658), (1507, 668), (1528, 685), (1528, 717), (1448, 717)]),
    ("sign_south", "Placa da entrada sul", "props", [(456, 744), (506, 744), (506, 775), (491, 775), (491, 795), (472, 795), (472, 775), (456, 775)]),
    ("sign_central", "Placa do pátio", "props", [(479, 398), (530, 398), (530, 430), (514, 430), (514, 450), (497, 450), (497, 430), (479, 430)]),
    ("sign_nursery", "Placa do viveiro", "props", [(1246, 358), (1299, 358), (1299, 390), (1284, 390), (1284, 406), (1267, 406), (1267, 390), (1246, 390)]),
    ("bench_left", "Banco esquerdo da árvore central", "props", [(581, 393), (640, 393), (640, 421), (635, 421), (635, 430), (623, 430), (623, 422), (594, 422), (594, 430), (582, 430)]),
    ("bench_right", "Banco direito da árvore central", "props", [(669, 393), (725, 393), (725, 422), (718, 422), (718, 430), (708, 430), (708, 422), (684, 422), (684, 430), (673, 430), (673, 422), (669, 422)]),
    ("bench_southwest", "Banco do canteiro sudoeste", "props", [(460, 690), (520, 690), (520, 720), (513, 720), (513, 727), (502, 727), (502, 720), (474, 720), (474, 727), (463, 727)]),
    ("fence_west", "Trecho de cerca de madeira", "structures", [(104, 704), (117, 704), (117, 715), (142, 715), (142, 705), (154, 705), (154, 715), (181, 715), (181, 706), (194, 706), (194, 716), (214, 716), (214, 745), (194, 745), (194, 751), (181, 751), (181, 743), (154, 743), (154, 751), (142, 751), (142, 743), (117, 743), (117, 750), (104, 750)]),
]

TILES = [
    ("paving", "Amostra do piso do pátio", (520, 482, 584, 546)),
    ("grass", "Amostra de grama", (642, 728, 674, 760)),
    ("canal_water", "Amostra de água do canal", (940, 335, 996, 399)),
    ("canal_lilies", "Vitórias-régias com água", (940, 213, 993, 273)),
    ("canal_bank", "Borda oeste do canal", (896, 332, 931, 396)),
    ("south_wall", "Trecho do muro sul", (685, 824, 767, 850)),
]


def extract(output):
    source = Image.open(SOURCE).convert("RGBA")
    if source.size != (1672, 941):
        raise ValueError("A fonte mudou de tamanho; revise os contornos antes de extrair.")
    for folder in ("cutouts", "references", "masks", "terrain"):
        (output / folder).mkdir(parents=True, exist_ok=True)

    entries = []
    overview = source.convert("RGB")
    draw = ImageDraw.Draw(overview)
    for index, (name, label, group, polygon) in enumerate(ELEMENTS, 1):
        left = min(x for x, _ in polygon)
        top = min(y for _, y in polygon)
        right = max(x for x, _ in polygon) + 1
        bottom = max(y for _, y in polygon) + 1
        box = (left, top, right, bottom)
        reference = source.crop(box)
        mask = Image.new("L", reference.size, 0)
        local_polygon = [(x - left, y - top) for x, y in polygon]
        ImageDraw.Draw(mask).polygon(local_polygon, fill=255)
        sprite = reference.copy()
        sprite.putalpha(mask)
        reference.save(output / "references" / f"{name}.png")
        mask.save(output / "masks" / f"{name}.png")
        sprite.save(output / "cutouts" / f"{name}.png")
        # Confere que o recorte não altera nenhum pixel visível da fonte.
        if ImageChops.difference(sprite.convert("RGB"), reference.convert("RGB")).getbbox():
            raise ValueError(f"Pixels alterados no recorte: {name}")
        if mask.getextrema() != (0, 255):
            raise ValueError(f"Máscara sem transparência: {name}")
        entries.append({
            "id": name, "label": label, "group": group,
            "file": f"cutouts/{name}.png", "reference": f"references/{name}.png",
            "mask": f"masks/{name}.png", "source_rect": [left, top, right-left, bottom-top],
            "polygon_local": local_polygon,
            "anchor_local": [(right-left)//2, bottom-top-1],
            "anchor_kind": "bottom_center_canvas",
            "mask_quality": "manual_approximate", "collision": None,
        })
        draw.rectangle((left, top, right-1, bottom-1), outline="#ff5c59", width=2)
        tag = f"{index:02}"
        draw.rectangle((left, top, left+25, top+16), fill="#10271c")
        draw.text((left+3, top+2), tag, fill="white")

    for name, label, box in TILES:
        source.crop(box).save(output / "terrain" / f"{name}.png")
        entries.append({
            "id": name, "label": label, "group": "terrain",
            "file": f"terrain/{name}.png",
            "source_rect": [box[0], box[1], box[2]-box[0], box[3]-box[1]],
            "tileable": False, "collision": None,
        })
    overview.save(output / "source_overview.png")

    cell_w, cell_h = 256, 218
    sheet = Image.new("RGB", (cell_w*4, cell_h*((len(ELEMENTS)+3)//4)), "#20352e")
    painter = ImageDraw.Draw(sheet)
    font = ImageFont.load_default()
    for index, item in enumerate(entries[:len(ELEMENTS)]):
        x, y = (index % 4)*cell_w, (index // 4)*cell_h
        for cy in range(y+24, y+cell_h-8, 12):
            for cx in range(x+8, x+cell_w-8, 12):
                color = "#334940" if ((cx-x)//12+(cy-y)//12) % 2 else "#496056"
                painter.rectangle((cx, cy, min(cx+11, x+cell_w-9), min(cy+11, y+cell_h-9)), fill=color)
        sprite = Image.open(output / item["file"]).convert("RGBA")
        sprite.thumbnail((cell_w-24, cell_h-44), Image.Resampling.NEAREST)
        sheet.paste(sprite, (x+(cell_w-sprite.width)//2, y+28+(cell_h-44-sprite.height)//2), sprite)
        painter.text((x+8, y+7), f"{index+1:02} {item['id']}", font=font, fill="#fff3cf")
    sheet.save(output / "contact_sheet.png")

    manifest = {
        "source": SOURCE.relative_to(ROOT).as_posix(),
        "source_sha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
        "source_size": list(source.size),
        "method": "original_pixels_manual_polygon_masks",
        "notes": "Máscaras aproximadas para edição, não hitboxes. Sem reconstrução de partes ocultas. Amostras de terreno não são tiles seamless.",
        "assets": entries,
    }
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False)+"\n", encoding="utf-8")
    print(f"Extraídos {len(ELEMENTS)} elementos com alfa e {len(TILES)} amostras de terreno.")
    print(f"Destino: {output.relative_to(ROOT) if output.is_relative_to(ROOT) else output}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=OUTPUT)
    args = parser.parse_args()
    extract(args.output.resolve())
