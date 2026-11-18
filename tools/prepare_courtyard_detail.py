"""Une os quatro quadrantes com iluminação da base e costuras de menor diferença."""

import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
DIRECTORY = ROOT / "assets/maps/courtyard_detail"
QUADRANTS = {
    "northwest": (0, 0, 900, 535),
    "northeast": (772, 0, 1672, 535),
    "southwest": (0, 407, 900, 941),
    "southeast": (772, 407, 1672, 941),
}


def match_lighting(generated, reference):
    """Mantém os detalhes novos com a iluminação e a paleta locais da base."""
    radius = round(12 * generated.width / 900)
    reference = reference.resize(generated.size, Image.Resampling.LANCZOS)
    pixels = np.asarray(generated.convert("RGB"), dtype=np.float32)
    generated_light = np.asarray(generated.convert("RGB").filter(ImageFilter.GaussianBlur(radius)), dtype=np.float32)
    reference_light = np.asarray(reference.filter(ImageFilter.GaussianBlur(radius)), dtype=np.float32)
    corrected = np.clip(pixels + reference_light - generated_light, 0, 255).astype(np.uint8)
    return Image.fromarray(corrected).convert("RGBA")


def minimum_path(cost):
    """Encontra uma costura contínua, avançando no máximo um pixel por linha."""
    energy = cost[:, 0].copy()
    parents = np.zeros(cost.shape, dtype=np.int8)
    candidates = np.arange(cost.shape[0])
    for column in range(1, cost.shape[1]):
        options = np.stack((np.r_[np.inf, energy[:-1]], energy, np.r_[energy[1:], np.inf]))
        choices = np.argmin(options, axis=0)
        energy = options[choices, candidates] + cost[:, column]
        parents[:, column] = choices - 1
    row = int(np.argmin(energy))
    path = np.empty(cost.shape[1], dtype=np.float32)
    for column in range(cost.shape[1] - 1, -1, -1):
        path[column] = row
        if column:
            row += int(parents[row, column])
    return path


def stitch(first, second, offset, reference, feather):
    """Une duas regiões por pesos complementares, sem duplicar transparências."""
    height, first_width, _ = first.shape
    overlap = first_width - offset
    if height != second.shape[0] or overlap <= feather * 2:
        raise ValueError("Quadrantes sem contexto suficiente para a costura.")
    left = first[:, offset:].astype(np.float32)
    right = second[:, :overlap].astype(np.float32)
    gray = reference[:, offset:first_width].astype(np.float32).mean(axis=2)
    edges = np.abs(np.diff(gray, axis=0, prepend=gray[:1])) + np.abs(np.diff(gray, axis=1, prepend=gray[:, :1]))
    cost = np.abs(left - right).mean(axis=2) + 0.75 * edges
    # A costura fica dentro da sobreposição, com contexto dos dois lados.
    margin = feather + 2
    path = minimum_path(cost[:, margin:-margin].T) + margin
    positions = np.arange(overlap, dtype=np.float32)[None, :]
    alpha = np.clip((positions - path[:, None] + feather) / (2 * feather), 0, 1)
    alpha = (alpha * alpha * (3 - 2 * alpha))[:, :, None]
    result = np.empty((height, offset + second.shape[1], 3), dtype=np.uint8)
    result[:, :offset] = first[:, :offset]
    result[:, offset:first_width] = np.round(left * (1 - alpha) + right * alpha).astype(np.uint8)
    result[:, first_width:] = second[:, overlap:]
    return result, path + offset


def prepare_full_map(source):
    # O fator comum é inferior à densidade nativa dos quatro resultados gerados.
    scale = 1.8
    output_size = (round(source.width * scale), round(source.height * scale))
    reference = np.asarray(source.resize(output_size, Image.Resampling.LANCZOS))
    regions, records = {}, []
    for name, box in QUADRANTS.items():
        cropped = source.crop(box)
        cropped.save(DIRECTORY / f"{name}_source.png")
        cropped.resize((cropped.width*2, cropped.height*2), Image.Resampling.NEAREST).save(
            DIRECTORY / f"{name}_reference_2x.png")
        path = DIRECTORY / f"{name}_generated_v1.png"
        generated = Image.open(path).convert("RGBA")
        density = (generated.width / cropped.width, generated.height / cropped.height)
        if min(density) < scale or abs(generated.width / generated.height - cropped.width / cropped.height) > 0.002:
            raise ValueError(f"Resolução ou enquadramento insuficiente: {name}.")
        matched = match_lighting(generated, cropped).convert("RGB")
        matched.save(DIRECTORY / f"{name}_matched.png", optimize=True)
        destination = [round(value * scale) for value in box]
        size = (destination[2] - destination[0], destination[3] - destination[1])
        regions[name] = np.asarray(matched.resize(size, Image.Resampling.LANCZOS))
        records.append({"id": name, "world_box": box, "generated_size": generated.size,
                        "native_density": density, "prepared_box": destination,
                        "generated_sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    right = round(772 * scale)
    bottom = round(407 * scale)
    top_height = round(535 * scale)
    feather = round(8 * scale)
    top, top_seam = stitch(regions["northwest"], regions["northeast"], right, reference[:top_height], feather)
    lower, lower_seam = stitch(regions["southwest"], regions["southeast"], right, reference[bottom:], feather)
    combined, middle_seam = stitch(top.swapaxes(0, 1), lower.swapaxes(0, 1), bottom,
                                  reference.swapaxes(0, 1), feather)
    combined = combined.swapaxes(0, 1)
    Image.fromarray(combined).save(DIRECTORY / "courtyard_complete.png", optimize=True)
    header = """/* Dimensões reais do mapa completo; gerado por prepare_courtyard_detail.py. */
#ifndef COURTYARD_DETAIL_DATA_H
#define COURTYARD_DETAIL_DATA_H

enum { COURTYARD_DETAIL_WIDTH = %d, COURTYARD_DETAIL_HEIGHT = %d,
       COURTYARD_DETAIL_WORLD_WIDTH = 1672, COURTYARD_DETAIL_WORLD_HEIGHT = 941 };

#endif
""" % output_size
    (ROOT / "include/courtyard_detail_data.h").write_text(header, encoding="utf-8")
    metadata = {
        "source": "assets/maps/botanical_courtyard.png",
        "source_sha256": hashlib.sha256((ROOT / "assets/maps/botanical_courtyard.png").read_bytes()).hexdigest(),
        "output": "assets/maps/courtyard_detail/courtyard_complete.png", "output_size": output_size,
        "world_size": source.size, "scale": scale, "quadrants": records,
        "blend_method": "local_lighting_match_and_complementary_minimum_error_seams",
        "seam_feather_world": 8,
        "seams": {
            "top_vertical_x": [float(top_seam.min()/scale), float(top_seam.max()/scale)],
            "bottom_vertical_x": [float(lower_seam.min()/scale), float(lower_seam.max()/scale)],
            "horizontal_y": [float(middle_seam.min()/scale), float(middle_seam.max()/scale)],
        },
        "status": "four_quadrants_complete",
    }
    (DIRECTORY / "full_metadata.json").write_text(json.dumps(metadata, indent=2)+"\n", encoding="utf-8")
    print(f"Complete map ready: {output_size[0]} x {output_size[1]}; four quadrants, no upsampling.")


def prepare():
    source_path = ROOT / "assets/maps/botanical_courtyard.png"
    source = Image.open(source_path).convert("RGB")
    if source.size != (1672, 941):
        raise ValueError("O tamanho da base mudou; revise os recortes e as posições.")
    DIRECTORY.mkdir(parents=True, exist_ok=True)
    prepare_full_map(source)


if __name__ == "__main__":
    prepare()
