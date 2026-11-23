"""Confere o pacote visual do combate por leitura, sem alterar imagens ou o jogo."""
import hashlib
import json
from pathlib import Path

from PIL import Image


def validate():
    root = Path(__file__).resolve().parent.parent
    folder = root / 'assets/ui/battle'
    manifest = json.loads((folder / 'asset_manifest_v1.json').read_text(encoding='utf-8'))
    layout = json.loads((folder / 'layout_spec_v1.json').read_text(encoding='utf-8'))
    json.loads((folder / 'generation_prompts_v1.json').read_text(encoding='utf-8'))
    assert layout['status'] == manifest['status']
    assert layout['status'] in ['preparado_sem_integracao', 'integrado_no_combate']
    assert layout['viewport'] == [1280, 720]
    species = set()
    for record in manifest['assets'].values():
        relative = Path(record['file'])
        assert not relative.is_absolute() and '..' not in relative.parts
        path = root / relative
        assert hashlib.sha256(path.read_bytes()).hexdigest() == record['sha256'], path
        with Image.open(path) as im:
            assert list(im.size) == record['size_px'] and im.mode == record['mode'], path
            x, y, width, height = record['source_xywh_px']
            assert width > 0 and height > 0 and 0 <= x < x+width <= im.width and 0 <= y < y+height <= im.height
            alpha = im.convert('RGBA').getchannel('A')
            assert list(alpha.getextrema()) == record['alpha_range']
            if record['role'] in ['skin', 'ground', 'plant']:
                assert im.mode == 'RGBA' and alpha.getpixel((0, 0)) == 0
                bbox = alpha.point(lambda a: 255 if a >= 128 else 0).getbbox()
                assert bbox and x <= bbox[0] and y <= bbox[1] and x+width >= bbox[2] and y+height >= bbox[3]
            if record['role'] == 'skin':
                assert alpha.getpixel((im.width//2, im.height//2)) >= 240
            if 'nine_slice_insets_px' in record:
                inset = record['nine_slice_insets_px']
                assert inset['left'] + inset['right'] < width and inset['top'] + inset['bottom'] < height
            if record['role'] == 'plant':
                species.add(record['species'])
                assert not record['mirror_in_file']
    assert len(species) == 8
    assert manifest['assets']['SPECIES_ALOE']['enemy_mirror_x']
    font = manifest['font']
    assert hashlib.sha256((root / font['file']).read_bytes()).hexdigest() == font['sha256']
    assert (root / font['license']).is_file()
    assert [button['enabled'] for button in layout['footer']['root_buttons']] == [True, True, False, False]
    for button in layout['footer']['root_buttons']:
        x, y, width, height = button['bounds']
        assert 0 <= x < x+width <= 1280 and 0 <= y < y+height <= 720
    assert len(layout['arena']['locations']) == 2
    for location in layout['arena']['locations']:
        assert (root / location['background']).is_file()
    assert 'output/' not in json.dumps(manifest)
    print('PASS: imagens, 8 espécies, dois cenários, fonte/licença, hashes, alfa, recortes, insets e menu bloqueado.')


if __name__ == '__main__':
    validate()
