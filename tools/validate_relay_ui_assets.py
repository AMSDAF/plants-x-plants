"""Confere o pacote industrial de relés sem iniciar ou alterar o jogo."""
import hashlib
import json
from pathlib import Path

from PIL import Image


def validate():
    root = Path(__file__).resolve().parents[1]
    folder = root / "assets/ui/relay_panel"
    manifest = json.loads((folder / "asset_manifest_v1.json").read_text(encoding="utf-8"))
    layout = json.loads((folder / "layout_spec_v1.json").read_text(encoding="utf-8"))
    prompts = json.loads((folder / "generation_prompts_v1.json").read_text(encoding="utf-8"))
    assert manifest["status"] == layout["status"]
    assert manifest["status"] in ("preparado_sem_integracao", "integrado_v0")
    assert len(prompts["assets"]) == 4
    for record in manifest["assets"].values():
        relative = Path(record["file"])
        assert not relative.is_absolute() and ".." not in relative.parts
        path = root / relative
        assert hashlib.sha256(path.read_bytes()).hexdigest() == record["sha256"], path
        with Image.open(path) as picture:
            assert list(picture.size) == record["size_px"] and picture.mode == record["mode"]
            x, y, width, height = record["source_xywh_px"]
            assert 0 <= x < x + width <= picture.width
            assert 0 <= y < y + height <= picture.height
            alpha = picture.convert("RGBA").getchannel("A")
            assert list(alpha.getextrema()) == record["alpha_range"]
            if record["role"] != "reference":
                assert picture.mode == "RGBA" and alpha.getpixel((0, 0)) < 128
                left, top, right, bottom = alpha.point(lambda value: 255 if value >= 128 else 0).getbbox()
                assert x <= left < right <= x + width and y <= top < bottom <= y + height
            if record["role"] == "skin":
                assert alpha.getpixel((picture.width // 2, picture.height // 2)) >= 240
                insets = record["nine_slice_insets_px"]
                assert insets["left"] + insets["right"] < width
                assert insets["top"] + insets["bottom"] < height
    assets = manifest["assets"]
    assert assets["switch_on_v1"]["source_xywh_px"] == assets["switch_off_v1"]["source_xywh_px"]
    assert assets["switch_on_v1"]["size_px"] == assets["switch_off_v1"]["size_px"]
    for key in ("font", "font_license", "cursor"):
        assert (root / manifest["reuse"][key]).is_file()
    assert layout["progress"]["interactive"] is False
    for count, key in ((1, "one"), (2, "two"), (3, "three")):
        assert len(layout["relay_cards"][key]) == 3
        for x, y, width, height in layout["relay_cards"][key]:
            assert 0 <= x < x + width <= 1280 and 0 <= y < y + height <= 720
    print("PASS: 4 assets, referência, hashes, alfa, recortes, nove partes, registro dos estados e recursos reutilizados.")


if __name__ == "__main__":
    validate()
