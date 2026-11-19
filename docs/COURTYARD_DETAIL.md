# Mapa botânico melhorado por quadrantes

Os quatro quadrantes do background original foram tratados separadamente e
unidos em `assets/maps/courtyard_detail/courtyard_complete.png`, com
**3010 × 1694 pixels**, aproximadamente **1,8× por eixo** em relação aos
1672 × 941 originais. A preparação não aumenta artificialmente os resultados:
cada imagem gerada tem densidade nativa ligeiramente superior ao fator comum.

A imagem original e as quatro fontes geradas dos quadrantes permanecem preservadas.
Nenhuma regra de circulação, posição de porta, interação ou colisão foi alterada.
A edição pode variar detalhes das formas; a geração não garante equivalência
geométrica pixel a pixel com a fonte.

## Fontes e composição

As quatro fontes `<região>_generated_v1.png` e o mapa completo ficam em
`assets/maps/courtyard_detail/`. As 12 imagens intermediárias
`<região>_source.png`, `<região>_reference_2x.png` e `<região>_matched.png`
foram removidas: o jogo não as carrega e o gerador consegue recriá-las a partir
do mapa original e das quatro fontes mantidas. A referência 2× serve somente
como entrada ampliada para edição, não como resultado final. Executar novamente
`tools/prepare_courtyard_detail.py` recria esses intermediários na mesma pasta.

| Região | Recorte original [esquerda, topo, direita, base] | Resultado gerado |
| --- | --- | --- |
| northwest | [0, 0, 900, 535] | 1627 × 967 |
| northeast | [772, 0, 1672, 535] | 1627 × 967 |
| southwest | [0, 407, 900, 941] | 1628 × 966 |
| southeast | [772, 407, 1672, 941] | 1628 × 966 |

Cada metade tem 64 pixels de contexto além da divisão; regiões vizinhas
compartilham 128 pixels. A preparação ajusta a iluminação local de cada
resultado à fonte original e escolhe costuras contínuas entre áreas parecidas,
evitando contornos fortes. Os pesos dos dois lados são complementares e
somam um, com transição de oito pixels de cada lado da costura.

A composição une as duas regiões superiores, depois as inferiores e finalmente
as duas faixas. O resultado é uma textura RGB completamente opaca, sem quatro
camadas transparentes sobrepostas. Não há reconstrução procedural do piso.

`full_metadata.json` registra dimensões, recortes, densidades, hashes e limites
das costuras. `include/courtyard_detail_data.h` contém as dimensões reais usadas
pelo carregador. O teste antigo de apenas um quadrante (`northwest_detail.png`
e `metadata.json`) e seu trecho exclusivo no gerador foram removidos. A preparação
agora executa somente a composição do mapa completo.
Os prompts das três regiões novas ficam em `prompts.json`, gerados com a
ferramenta integrada de imagem e com o primeiro quadrante como referência de estilo.

## Execução e comparação

Execute `build/plants_x_plants.exe` a partir da raiz. **F8** na exploração alterna
o mapa inteiro entre original e melhorado. **F11** alterna a tela cheia.
A textura melhorada é opcional, validada pelas dimensões e carregada uma vez.
Se faltar ou falhar, o background original continua sendo exibido. A ponte
é desenhada depois do fundo, conforme o estado da missão; NPCs e controles
continuam usando os recursos anteriores.

O mapa original mantém filtro pontual. A versão maior usa filtro bilinear e
`TEXTURE_WRAP_CLAMP`, impedindo a repetição da borda oposta que causava a linha
horizontal no primeiro teste. A renderização por `game_canvas` usa a resolução
física do viewport, preservando coordenadas lógicas de 1280 × 720, proporção,
câmera, mouse e recorte do campo de nome.

## Preparação e verificação

Com Python, Pillow e NumPy no ambiente de preparação do projeto:

```powershell
.\build\trainer_tools\Scripts\python.exe tools/prepare_courtyard_detail.py
cmake --build build --target plants_x_plants game_canvas_check name_entry_check
.\build\game_canvas_check.exe
.\build\name_entry_check.exe
```

O script refaz referências e derivados a partir dos quatro resultados já
gerados. Não chama uma API de imagem. Preserva os resultados gerados e a
imagem original. Interrompe se algum quadrante não tiver resolução suficiente
para o fator comum ou o mesmo enquadramento da referência.

`game_canvas_check` verifica 720p, Full HD, 1440p, HiDPI, mouse, recorte,
câmera, cobertura das quatro regiões, ausência da textura e ponte dinâmica.
Seu teste de borda reproduz o defeito com `TEXTURE_WRAP_REPEAT` e confirma a
correção com o modo de execução. Capturas das quatro posições da câmera ficam
em `build/courtyard_detail_preview/corner_<índice>_1080p.png`.

`name_entry_check` verifica F8, portas, colisões existentes, guardiões, chave,
subsolo, ponte, pistas e conclusão da campanha. As capturas em Full HD da
interface ficam em `build/name_entry_preview/`.

## Prompt da edição integrada

```text
Use case: precise-object-edit. Image 1 is the EXACT EDIT TARGET at 1800 x 1070 pixels, a 2x enlarged northwest region of an existing game map. Produce the edited image at exactly the SAME 1800 x 1070 resolution and framing. This is a faithful high-resolution restoration, NOT a redesign. Reconstruct fine pixel-art definition inside the existing shapes so it is genuinely clearer at this resolution, replacing soft enlarged pixel clusters with deliberate crisp small pixel clusters and subtle readable leaf, tile, wood and stone detail. Keep the SAME established warm detailed pixel-art style, restrained natural greens, original contour weight, light direction and soft shadows. Lock every silhouette, edge position, road boundary, roof shape, door, window, crate, tree, bench, sign, fence post and flowerbed to its current coordinates. Especially preserve the depot roof rectangle and door center near (556,680), pergola pillars, and central tree/planter; do not shift anything even slightly. The rightmost and bottommost 128 pixels are overlap context for adjacent map tiles: preserve their layout and color exactly and do not invent cropped objects. Do not crop, outpaint or reframe. No extra objects, no larger leaves, no bolder black outlines, no chibi/cartoon styling, no saturated recoloring, no painterly smoothing, no blur, no sharpening halos. Preserve the illustration's atmosphere and all fine background vegetation. Sharpen through coherent redrawing of fine detail only. Deliver a single opaque full rectangular image, 1800x1070, no border, no text or watermark.
```
