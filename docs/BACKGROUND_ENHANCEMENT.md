# Melhoria do background botânico

Fonte preservada: `assets/maps/botanical_courtyard.png`.
Tentativa antiga removida: `assets/maps/botanical_courtyard_enhanced_v1.png`.

Este documento preserva o histórico da tentativa de edição da imagem inteira,
sem aumento de resolução. A imagem experimental foi removida por não ser usada.
O resultado atual por quadrantes
e a correção do canvas estão em [COURTYARD_DETAIL.md](COURTYARD_DETAIL.md).

Preparada com a ferramenta integrada de geração/edição de imagens, usando
a imagem inteira como alvo. O objetivo é definir contornos e detalhes mantendo
o estilo e a composição, sem sobrepor objetos produzidos separadamente.

O jogo utiliza o mapa completo `assets/maps/courtyard_detail/courtyard_complete.png`,
com o original como alternativa. A tentativa antiga não foi integrada: a geração
não garantia a preservação exata das silhuetas nem o alinhamento com colisões
e ponte. Sua remoção não altera regras, colisões ou posições de interação.

## Prompt usado

```text
Use case: precise-object-edit. Asset type: complete top-down oblique pixel-art game background, conservative whole-image restoration/upscale. Image 1 is the EDIT TARGET, not a loose inspiration. Improve the quality and clarity of this EXACT botanical courtyard map as one unified image. Preserve the exact complete composition, 16:9 framing, camera, palette, lighting, shadows, coherent pixel-art visual language and relative positions and sizes of EVERY object. Preserve all walkable paths and their exact outlines, building silhouettes, doors and entrances, canal banks and canal width, the closed/retracted bridge crossing, bottom entrance gate, pergola, planters, trees, flowerbeds, pipes, valve and signs. The source world is 1672x941: every landmark must remain at the same normalized coordinate, especially the canal spanning x907-1045, bridge crossing y525-563, and doors at (278,379), (243,645), (810,561), (1396,330). Only restore definition: resolve soft/blurred pixel clusters into clean intentional pixel-art shapes, improve material readability and consistent fine detail very subtly, remove fuzzy edge artifacts while preserving the organic dense foliage and warm gentle atmosphere. Maintain original contour thickness, natural muted greens, warm paving and terracotta and turquoise water; do not use bold black cartoon outlines, oversaturation, glossy 3D, painterly blur, oversharpen halos, noise or a different style. Do not redesign or simplify the map, do not move or replace anything, do not add objects, do not add characters or text, do not create a new bridge, do not make sprite sheets or separate cutouts. Deliver one seamless polished version of the ENTIRE SAME original map, as high resolution as available, ideally 3344x1882, same aspect ratio, opaque full background, no border, no crop.
```
