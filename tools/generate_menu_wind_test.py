"""Gera vento localizado na vegetacao real do fundo, sem alterar a fonte.

Dependencias: tools/menu_wind_requirements.txt (somente desenvolvimento).
Uso: python tools/generate_menu_wind_test.py [--preview-dir build/menu_wind_preview]
"""

import argparse
import hashlib
import json
import math
from pathlib import Path

import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageEnhance


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets/sprites/menu_background.png"
OUTPUT = ROOT / "assets/sprites/menu_wind_test"
HEADER = ROOT / "include/menu_wind_test_data.h"
FRAME_COUNT = 64
PLAYBACK_FPS = 12
FRAME_COLUMNS = 8
ATLAS_WIDTH = 2048


def region(name, box, anchor, tip, material, amplitude, phase, polygon=None):
    # Folga para o envelope completo; quadros em multiplos de oito.
    x0, y0, x1, y1 = box
    return dict(name=name, selection=box, origin=(x0 - 12, y0 - 12),
                size=(math.ceil((x1 - x0 + 24) / 8) * 8,
                      math.ceil((y1 - y0 + 24) / 8) * 8),
                anchor=anchor, tip=tip, material=material,
                amplitude=amplitude, phase=phase, polygon=polygon)


# Ordem explicita: frondes distantes, copa, musgo e pontas proximas.
# As duas selecoes aprovadas conservam recorte, fixacao, amplitude e periodo.
REGIONS = [
    region("left_fern", (459, 274, 516, 296), (460, 291), (511, 284), "fern", 3.5, 0.9,
           [(459, 289), (469, 275), (488, 276), (504, 278), (514, 282), (516, 288),
            (499, 290), (493, 296), (478, 290), (459, 296)]),
    region("right_fern", (1287, 454, 1311, 485), (1296, 483), (1301, 457), "fern", 3.0, 3.8,
           [(1287, 482), (1289, 468), (1292, 457), (1303, 454), (1311, 459), (1307, 472), (1305, 485)]),
    region("left_canopy", (201, 23, 238, 60), (218, 23), (215, 56), "leaf", 5.0, 0.0,
           [(201, 23), (238, 23), (237, 38), (228, 45), (224, 55), (215, 60), (207, 50), (203, 38)]),
    region("right_canopy", (1309, 43, 1354, 104), (1340, 43), (1322, 100), "leaf", 5.5, 3.3,
           [(1320, 43), (1354, 43), (1354, 72), (1344, 83), (1336, 95), (1323, 104), (1315, 99), (1309, 68)]),
    region("left_moss", (261, 182, 282, 224), (273, 182), (274, 220), "moss", 4.5, 0.35),
    dict(name="left", origin=(282, 184), size=(64, 80), selection=(302, 198, 326, 252),
         anchor=(314, 198), tip=(315, 251), material="approved_moss", amplitude=4.5, phase=0.0, polygon=None),
    dict(name="right", origin=(1338, 112), size=(64, 80), selection=(1358, 128, 1382, 174),
         anchor=(1370, 128), tip=(1371, 173), material="approved_moss", amplitude=4.5, phase=2.35, polygon=None),
    region("upper_twig", (698, 74, 738, 104), (701, 78), (734, 99), "twig", 5.5, 1.4,
           [(698, 77), (705, 74), (716, 77), (722, 83), (728, 84), (728, 88),
            (731, 92), (735, 95), (736, 102), (730, 101), (724, 96), (717, 94),
            (712, 95), (707, 92), (701, 94), (700, 86), (698, 83)]),
    region("left_twig", (311, 269, 378, 303), (313, 288), (374, 286), "twig", 5.0, 0.55,
           [(311, 282), (329, 273), (349, 269), (370, 277), (378, 289), (357, 293), (353, 303), (330, 297), (311, 292)]),
    region("right_twig", (1413, 280, 1466, 331), (1460, 282), (1418, 326), "twig", 5.5, 3.9,
           [(1443, 280), (1466, 280), (1466, 302), (1444, 311), (1424, 331), (1413, 330), (1415, 302), (1428, 289)]),
]


def silhouette(image, item):
    """Recorta por cor e contorno dentro de selecoes, nunca por retangulo opaco."""
    x0, y0, x1, y1 = item["selection"]
    pixels = np.asarray(image)[y0:y1, x0:x1].astype(np.int16)
    r, g, b = (pixels[:, :, i] for i in range(3))
    if item["material"] == "fern":
        candidate = ((np.minimum(r, g) - b > 24) & (g > 58) & (r > 55))
    elif item["material"] in ("leaf", "twig"):
        # O contorno escuro so pode acompanhar pixels proximos da folha.
        # Evita capturar sombras conectadas de troncos atras da silhueta.
        colored = (r - g > (12 if item["material"] == "twig" else 5)) & (g > 35)
        near_color = cv2.dilate(colored.astype(np.uint8), np.ones((5, 5), np.uint8)) > 0
        candidate = colored | ((g < 30) & near_color)
    else:
        candidate = (r - g > (12 if item["material"] == "twig" else 5)) | (g < 30)
    if item["polygon"]:
        limit = Image.new("L", (x1 - x0, y1 - y0))
        ImageDraw.Draw(limit).polygon([(x - x0, y - y0) for x, y in item["polygon"]], fill=255)
        candidate &= np.asarray(limit) > 0
    count, labels, stats, _ = cv2.connectedComponentsWithStats(candidate.astype(np.uint8), 8)
    assert count > 1, f'Silhueta nao encontrada: {item["name"]}'
    component = 1 + np.argmax(stats[1:, cv2.CC_STAT_AREA])
    local = (labels == component).astype(np.uint8) * 255
    contours, _ = cv2.findContours(local, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    cv2.drawContours(local, contours, -1, 255, cv2.FILLED)
    mask = Image.new("L", image.size)
    mask.paste(Image.fromarray(local), (x0, y0))
    return mask


def animate(layer, item, index):
    """Deforma perpendicularmente ao eixo fixacao-ponta."""
    pixels = np.asarray(layer)
    yy, xx = np.indices((layer.height, layer.width), dtype=np.float32)
    ax, ay = np.array(item["anchor"]) - item["origin"]
    direction = np.array(item["tip"], dtype=float) - item["anchor"]
    length = np.linalg.norm(direction)
    direction /= length
    angle = 2 * math.pi * index / FRAME_COUNT
    if item["material"] == "approved_moss":
        # Mesmo movimento aprovado, com amostragem duas vezes mais frequente.
        tip_y = np.where(pixels[:, :, 3] > 0)[0].max()
        weight = np.clip((yy - ay - 6) / (tip_y - ay - 6), 0, 1) ** 1.6
        dx = np.rint(item["amplitude"] * np.sin(2 * angle + item["phase"]) * weight)
        dy = np.zeros_like(dx)
    else:
        along = (xx - ax) * direction[0] + (yy - ay) * direction[1]
        weight = np.clip((along - 7) / max(length - 7, 1), 0, 1) ** 1.5
        phase = angle + item["phase"]
        if item["material"] == "moss":
            signal = 0.88 * math.sin(phase) + 0.12 * math.sin(2 * phase + 0.4)
        else:
            # Rajada comum lenta e resposta secundaria mais rapida das folhas.
            signal = 0.55 * math.sin(phase) + 0.30 * math.sin(2 * phase + 0.4)
            signal += 0.15 * math.sin(3 * phase + (0.8 if item["material"] == "fern" else 0.2))
        displacement = item["amplitude"] * signal * weight
        dx = np.rint(-direction[1] * displacement)
        dy = np.rint(direction[0] * displacement)
    # Mapeamento inverso preserva a silhueta sem buracos entre pixels.
    sample_x = np.clip(np.rint(xx - dx).astype(int), 0, layer.width - 1)
    sample_y = np.clip(np.rint(yy - dy).astype(int), 0, layer.height - 1)
    result = pixels[sample_y, sample_x].copy()
    result[result[:, :, 3] == 0] = 0
    return Image.fromarray(result), (dx == 0) & (dy == 0)


def write_header(atlas_height):
    """Descritores gerados evitam divergencia entre Python e runtime C."""
    lines = ["/* Gerado por tools/generate_menu_wind_test.py: layout e coordenadas do vento. */",
             "#ifndef MENU_WIND_TEST_DATA_H", "#define MENU_WIND_TEST_DATA_H", "",
             '#include "raylib.h"', "", f"#define WIND_REGION_COUNT {len(REGIONS)}",
             f"#define WIND_FRAME_COUNT {FRAME_COUNT}", f"#define WIND_FRAME_COLUMNS {FRAME_COLUMNS}",
             f"#define WIND_PLAYBACK_FPS {PLAYBACK_FPS}.0f", f"#define WIND_ATLAS_WIDTH {ATLAS_WIDTH}",
             f"#define WIND_ATLAS_HEIGHT {atlas_height}", "#define WIND_SOURCE_WIDTH 1672",
             "#define WIND_SOURCE_HEIGHT 941", "", "typedef struct {", "    Vector2 origin;",
             "    Vector2 frame_size;", "    Vector2 atlas_origin;", "} MenuWindRegion;", "",
             "static const MenuWindRegion WIND_REGIONS[WIND_REGION_COUNT] = {"]
    for item in REGIONS:
        x, y = item["origin"]
        w, h = item["size"]
        sx, sy = item["atlas_origin"]
        lines.append(f"    {{{{{x}, {y}}}, {{{w}, {h}}}, {{{sx}, {sy}}}}}, /* Regiao: {item['name']} */")
    lines += ["};", "", "#endif", ""]
    content = "\n".join(lines)
    if not HEADER.exists() or HEADER.read_text(encoding="utf-8") != content:
        HEADER.write_text(content, encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--preview-dir", type=Path, default=ROOT / "build/menu_wind_preview")
    args = parser.parse_args()
    args.preview_dir.mkdir(parents=True, exist_ok=True)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    original_hash = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    image = Image.open(SOURCE).convert("RGB")
    assert image.size == (1672, 941), "Revisar selecoes se o fundo mudar de tamanho"
    source = np.asarray(image)
    combined_mask = np.zeros((image.height, image.width), dtype=np.uint8)
    combined_envelope = np.zeros_like(combined_mask)
    animations = []
    overview = image.copy()

    for item in REGIONS:
        mask = silhouette(image, item)
        # Mascaras nao se sobrepoem; falha explicita exige rever a selecao.
        assert not np.any((combined_mask > 0) & (np.asarray(mask) > 0)), item["name"]
        combined_mask = np.maximum(combined_mask, np.asarray(mask))
        x, y = item["origin"]
        w, h = item["size"]
        layer = image.convert("RGBA").crop((x, y, x + w, y + h))
        layer.putalpha(mask.crop((x, y, x + w, y + h)))
        pixels = np.array(layer)
        pixels[pixels[:, :, 3] == 0] = 0
        layer = Image.fromarray(pixels)
        layer.save(OUTPUT / (item["name"] + "_extracted.png"))
        mask.crop((x, y, x + w, y + h)).save(OUTPUT / (item["name"] + "_mask.png"))
        frames = []
        loop_start, _ = animate(layer, item, 0)
        loop_end, _ = animate(layer, item, FRAME_COUNT)
        assert loop_start.tobytes() == loop_end.tobytes(), item["name"]
        for i in range(FRAME_COUNT):
            frame, fixed = animate(layer, item, i)
            bounds = frame.getbbox()
            assert bounds and bounds[0] > 0 and bounds[2] < w and bounds[1] > 0 and bounds[3] < h
            assert np.array_equal(np.asarray(frame)[fixed], pixels[fixed])
            frames.append(frame)
        assert len({frame.tobytes() for frame in frames}) > 1, item["name"]
        differences = [int(np.count_nonzero(np.any(np.asarray(frames[i]) != np.asarray(frames[(i+1) % FRAME_COUNT]), axis=2)))
                       for i in range(FRAME_COUNT)]
        item["boundary_changed_pixels"] = differences[-1]
        item["max_step_changed_pixels"] = max(differences)
        assert differences[-1] <= max(differences[:-1]), item["name"]
        envelope = np.maximum.reduce([np.asarray(frame)[:, :, 3] for frame in frames])
        occupied = combined_envelope[y:y + h, x:x + w]
        assert not np.any((envelope > 0) & (occupied > 0)), item["name"]
        occupied[:] = np.maximum(occupied, envelope)
        animations.append(frames)
        d = ImageDraw.Draw(overview)
        d.rectangle(item["selection"], outline="cyan", width=2)
        d.text((x, y - 2), item["name"], fill="white")

    # Empacota por altura sem mudar a ordem de desenho das camadas.
    shelves = []
    atlas_height = 0
    for item in sorted(REGIONS, key=lambda r: r["size"][1], reverse=True):
        w, h = item["size"]
        block_w, block_h = w * FRAME_COLUMNS, h * (FRAME_COUNT // FRAME_COLUMNS)
        shelf = next((s for s in shelves if s["x"] + block_w <= ATLAS_WIDTH), None)
        if shelf is None:
            shelf = dict(x=0, y=atlas_height)
            shelves.append(shelf)
            atlas_height += block_h
        item["atlas_origin"] = (shelf["x"], shelf["y"])
        shelf["x"] += block_w
    assert not np.any(combined_envelope[140:770, 560:1110]), "Area central deve ficar calma"
    assert atlas_height <= 4096
    atlas = Image.new("RGBA", (ATLAS_WIDTH, atlas_height))
    for item, frames in zip(REGIONS, animations):
        x, y = item["atlas_origin"]
        w, h = item["size"]
        for i, frame in enumerate(frames):
            atlas.paste(frame, (x + (i % FRAME_COLUMNS) * w, y + (i // FRAME_COLUMNS) * h))
    atlas.save(OUTPUT / "canopy_atlas.png")
    # Uma unica reconstrucao considera a uniao de TODAS as silhuetas.
    restored = cv2.inpaint(source, combined_mask, 4, cv2.INPAINT_TELEA)
    background = Image.fromarray(restored).convert("RGBA")
    static_composite = background.copy()
    for item in REGIONS:
        static_composite.alpha_composite(
            Image.open(OUTPUT / (item["name"] + "_extracted.png")), item["origin"])
    assert np.array_equal(np.asarray(static_composite.convert("RGB")), source)
    background.convert("RGB").save(OUTPUT / "background.png")
    Image.fromarray(combined_mask).save(OUTPUT / "reconstruction_mask.png")
    overview.save(args.preview_dir / "regions.png")
    write_header(atlas_height)
    # Atlas substitui as sheets antigas; nao manter nem carregar versoes duplicadas.
    for name in ("left_sheet.png", "right_sheet.png"):
        (OUTPUT / name).unlink(missing_ok=True)
    memory = ATLAS_WIDTH * atlas_height * 4 + image.width * image.height * 3
    metadata = dict(source_size=image.size, source_sha256=original_hash, frame_count=FRAME_COUNT,
                    frame_columns=FRAME_COLUMNS, playback_fps=PLAYBACK_FPS,
                    atlas_size=atlas.size, experimental_texture_bytes=memory, regions=REGIONS)
    (OUTPUT / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")

    previews = []
    samples = (0, 8, 16, 24, 32, 48, 62, 63, 0, 1)
    contact = Image.new("RGB", (len(samples) * 144, len(REGIONS) * 128), (25, 28, 20))
    for i in range(FRAME_COUNT):
        composite = background.copy()
        for item, frames in zip(REGIONS, animations):
            composite.alpha_composite(frames[i], item["origin"])
        scale = max(1280 / image.width, 720 / image.height)
        resized = composite.resize((round(image.width * scale), round(image.height * scale)), Image.Resampling.NEAREST)
        view = resized.crop(((resized.width - 1280) // 2, (resized.height - 720) // 2,
                             (resized.width + 1280) // 2, (resized.height + 720) // 2)).convert("RGB")
        view = ImageEnhance.Brightness(view).enhance(0.82)
        previews.append(view)
        if i in samples:
            for column, sample in enumerate(samples):
                if i != sample:
                    continue
                for row, item in enumerate(REGIONS):
                    x, y = item["origin"]
                    w, h = item["size"]
                    detail = composite.crop((x, y, x + w, y + h))
                    detail_scale = min(140 / w, 110 / h)
                    detail = detail.resize((round(w * detail_scale), round(h * detail_scale)), Image.Resampling.NEAREST)
                    contact.paste(detail, (column * 144, row * 128 + 16))
                    ImageDraw.Draw(contact).text((column * 144, row * 128), f'{item["name"]} {i}', fill="white")
        if i in (0, 16, 32, 48, 63):
            view.save(args.preview_dir / f"scene_{i:02}.png")
    contact.save(args.preview_dir / "contact.png")
    assert previews[0].tobytes() != previews[-1].tobytes()
    palette = previews[0].quantize(colors=256)
    gif_frames = [p.quantize(palette=palette, dither=Image.Dither.NONE) for p in previews]
    durations = [round((i + 1) * 100 / PLAYBACK_FPS) * 10 - round(i * 100 / PLAYBACK_FPS) * 10
                 for i in range(FRAME_COUNT)]
    gif_frames[0].save(args.preview_dir / "menu_wind.gif", save_all=True,
                       append_images=gif_frames[1:], duration=durations, loop=0, optimize=False)
    assert hashlib.sha256(SOURCE.read_bytes()).hexdigest() == original_hash
    assert np.array_equal(restored[combined_mask == 0], source[combined_mask == 0])
    print(f"Gerados: {len(REGIONS)} regioes; atlas {atlas.size}; texturas experimentais {memory / 1048576:.2f} MiB")
    print(f"Fonte preservada: SHA-256 {original_hash}; previa: {args.preview_dir}")


if __name__ == "__main__":
    main()
