"""Monta prévias temporais e contato a partir das capturas OpenGL da verificação."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
PREVIEW = ROOT / 'build/selection_preview'


def preview():
    frames = [Image.open(PREVIEW / f'loop_{i:02d}.png').convert('RGB') for i in range(48)]
    # Paleta comum evita cintilação causada pela quantização independente dos quadros.
    palette = frames[0].quantize(colors=256)
    indexed = [frame.quantize(palette=palette, dither=Image.Dither.NONE) for frame in frames]
    indexed[0].save(PREVIEW / 'selection_scene.gif', save_all=True, append_images=indexed[1:],
                    duration=125, loop=0, optimize=True)
    contact = Image.new('RGB', (1280, 720))
    for j, i in enumerate((0, 12, 24, 36)):
        contact.paste(frames[i], ((j % 2) * 640, (j // 2) * 360))
    contact.save(PREVIEW / 'animation_phases.png')
    print('Prévia de seis segundos e contato em build/selection_preview/')


if __name__ == '__main__':
    preview()
