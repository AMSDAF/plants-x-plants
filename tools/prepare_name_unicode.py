"""Gera intervalos de letras e dígitos Unicode usados pela entrada de nome."""
from pathlib import Path
import unicodedata

root = Path(__file__).resolve().parents[1]
ranges = []
start = None
for codepoint in range(0x110000):
    category = unicodedata.category(chr(codepoint))
    keep = category.startswith('L') or category == 'Nd'
    if keep and start is None:
        start = codepoint
    if start is not None and (not keep or codepoint == 0x10ffff):
        ranges.append((start, codepoint-1 if not keep else codepoint))
        start = None
lines = ['/* Letras e dígitos Unicode '+unicodedata.unidata_version+'; gerados por tools/prepare_name_unicode.py. */',
         '#ifndef NAME_UNICODE_DATA_H', '#define NAME_UNICODE_DATA_H',
         'static const int NAME_UNICODE_RANGES[][2] = {']
lines += ['    {0x%X,0x%X},' % pair for pair in ranges]
lines += ['};', '#endif', '']
(root/'include/name_unicode_data.h').write_text('\n'.join(lines), encoding='utf-8')
