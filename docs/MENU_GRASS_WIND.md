# Vento da vegetação do menu

## Fonte e preparação

`tools/prepare_menu_grass.py` usa somente `assets/sprites/menu_background_v2.png`, com alternativa para a grafia fornecida `menu_backround_v2.png` (1672 × 941). A fonte permanece intacta. Pillow, NumPy e SciPy já fazem parte de `tools/trainer_requirements.txt`; não há nova dependência nem geração de arte.

As 17 caixas `REGIONS` são coordenadas da fonte, com limites direitos/inferiores exclusivos. A extração combina sementes de verde escuro e realces verde-lima, conectividade, fechamento de pequenas lacunas e descarte de componentes cortados pelas bordas. Há exclusões locais da copa em `upper_left` e `clearing_left`. Terra, pedras e troncos não entram nos recortes móveis. As máscaras e camadas individuais ficam em `build/menu_grass_preview/` para revisão.

| Região | Caixa na fonte (x0, y0, x1, y1) | Amplitude máxima configurada, pixels da fonte |
|---|---|---|
| far_left | 465, 110, 528, 146 | 2,5 |
| upper_left | 359, 199, 439, 247 | 3,2 |
| upper_right | 1210, 187, 1283, 233 | 3,0 |
| upper_middle | 940, 152, 1016, 201 | 3,0 |
| middle_left | 278, 278, 385, 363 | 5,0 |
| middle_right | 1241, 312, 1338, 388 | 5,2 |
| rock_left | 369, 384, 438, 429 | 3,1 |
| trunk_right | 1365, 393, 1458, 454 | 4,0 |
| clearing_left_upper | 269, 484, 414, 591 | 8,0 |
| clearing_right_upper | 1220, 460, 1358, 556 | 7,0 |
| clearing_left | 220, 606, 343, 714 | 7,6 |
| clearing_right | 1238, 683, 1391, 790 | 8,5 |
| clearing_right_small | 1122, 582, 1211, 641 | 4,0 |
| bottom_left | 443, 829, 554, 916 | 7,0 |
| bottom_right | 1119, 802, 1270, 918 | 8,5 |
| lower_left_small | 367, 780, 473, 834 | 4,2 |
| edge_right | 1541, 522, 1630, 583 | 4,3 |

A remoção inclui três pixels de margem apenas nas cores verdes adjacentes. O chão é reconstruído por ajuste quadrático robusto das cores próximas, com resíduo da textura local e seleção da cor mais próxima presente nessas amostras. Na raiz, são copiadas amostras próximas de chão/sombra. São alterados 68.116 pixels, cerca de 4,33% da imagem. Fora da máscara de remoção, o fundo é idêntico à fonte; nenhuma cor nova é introduzida. O chão originalmente oculto é uma aproximação localizada, não uma recuperação exata.

## Movimento e recursos

Cada touceira tem margem transparente e origem na fonte. O pivô vertical é o último pixel da máscara; o horizontal é a média dos três últimos níveis. Os cinco níveis inferiores permanecem idênticos em todos os quadros. O deslocamento horizontal aumenta progressivamente até a ponta com peso de potência 1,7; não há deslocamento vertical, rotação de retângulo, interpolação ou suavização.

O ciclo tem 32 quadros a 8 FPS, duração de quatro segundos. A rajada dobra para a direita e retorna suavemente. A fase depende de `x/1672*1,45 + y/941*0,35`, produzindo atraso espacial; as amplitudes variam conforme a tabela. A amostragem é periódica, incluindo a passagem 31 → 0. Os deslocamentos são arredondados para pixels da fonte. No tamanho de execução, as pontas maiores deslocam aproximadamente seis pixels. Troncos, raízes, pedras, interface, logo e mascote não são deformados.

`menu_grass_atlas.png` tem 4096 × 1632 RGBA: 32 páginas de 512 × 408, em oito colunas. Consome 25,50 MiB como textura RGBA, sem mipmaps; PNG de aproximadamente 614 KiB. `menu_grass_background.png` tem 1672 × 941, aproximadamente 1,16 MiB em PNG. O cabeçalho gerado `include/menu_grass_data.h` registra retângulos no atlas, origens, pivôs, amplitudes e fases. Não há preparação, alocação de texturas ou deformação de imagens por quadro no jogo.

`MenuScreen` carrega o par uma vez, valida dimensões, aplica filtro pontual e libera cada textura no encerramento. Fundo e recortes usam a mesma transformação de cobertura da janela, inclusive a origem arredondada. O relógio independente avança antes do bloqueio de entrada e não reinicia entre título/menu. O relógio do mascote e a navegação permanecem iguais. Se qualquer recurso estiver ausente ou inválido, ambos são desativados e o fundo original completo é desenhado.

## Copas em primeiro plano e ao fundo

`tools/prepare_menu_trees.py` reutiliza a fonte, extração de grama, dimensões e curva periódica do preparador existente. Não altera os derivados aprovados da grama. As tabelas `FOREGROUND` e `DISTANT` documentam polígonos/caixas e pontos de ligação em coordenadas da fonte: quatro copas grandes nos cantos e oito copas na faixa superior (esquerda, centro e direita).

Cada copa grande combina três campos suaves de aglomerados sobrepostos. A distância aos pontos de ligação aumenta a curvatura; uma guarda mantém folhas próximas a galhos, madeira, grama e bordas da janela imóveis. Copas vizinhas também possuem guarda fixa na interseção. Isso evita recortes rígidos, sobreposição indevida e galhos desconectados. O atlas guarda sete poses por copa grande e três por copa distante, todas com amostragem inteira e alfa binário. As 32 fases do relógio de grama escolhem poses por tabela. A rajada continua para a direita, com atraso espacial; no fundo, uma média circular de sete fases dá mais inércia e deslocamento de apenas 1–2 pixels da fonte. Nas copas grandes, a rajada usada chega a cerca de cinco pixels da fonte, aproximadamente quatro na janela. Não há translação vertical nem rotação de árvore.

`menu_wind_background.png` parte de `menu_grass_background.png`. Somente o contorno efetivamente exposto recebe amostras próximas de chão/céu, incluindo a franja antiga de até dois pixels. São 6.871 pixels adicionais reconstruídos, sem alterar nenhum pixel da reconstrução anterior da grama. A textura oculta é uma aproximação local. Madeira, grama e copas vizinhas são excluídas das amostras. Uma faixa da vegetação encoberta junto à árvore esquerda fica estática para preservar a sobreposição original.

`menu_tree_atlas.png` tem **2048 × 2688 RGBA, 21 MiB**. Os atlas de vento somam **46,5 MiB**; incluindo os 32 MiB do mascote, os atlas animados do menu somam **78,5 MiB**, sem mipmaps. Esses valores não incluem fundos, logo ou recursos de outras telas. `include/menu_tree_data.h` guarda retângulos das poses, origens e a sequência periódica. Texturas são carregadas uma vez e liberadas no encerramento; desenho não muda estado.

O fundo combinado só aparece com os dois pares completos e válidos. Sem fundo/atlas de copas, conserva a grama animada e as árvores originais estáticas. Sem o par da grama, usa o fundo original completo. Todas as camadas compartilham a transformação de cobertura. O relógio do vento e o do mascote não reiniciam ao abrir/fechar o menu.

## Reprodução e validação

Com o ambiente opcional de preparação instalado:

```powershell
.\build\trainer_tools\Scripts\python tools/prepare_menu_grass.py
.\build\trainer_tools\Scripts\python tools/prepare_menu_trees.py
cmake --build build
cmake --build build --target menu_screen_check
.\build\menu_screen_check.exe
.\build\trainer_tools\Scripts\python tools/prepare_menu_grass.py --runtime-preview
.\build\trainer_tools\Scripts\python tools/prepare_menu_trees.py --runtime-preview
git diff --check
```

Todos os diagnósticos ficam em `build/menu_grass_preview/`, ignorado pelo Git:

- `regions.png`, máscaras individuais e `reconstruction_mask.png`: seleção e reconstrução.
- `scene.gif`: cena completa em 1280 × 720, sem interface.
- `contact.png` e `closeup.png`: quadros 31, 0, 1 e curva máxima.
- `title_game.gif` e `game_closeup.png`: capturas reais do renderizador do jogo com OpenGL, incluindo mascote.
- `menu.png` e `static_fallback.png`: menu aberto e comparação do fallback.
- `inspection.json`: geometria, fases, hashes, contagem de pixels e memória.

A preparação verifica bases fixas, margens transparentes, paleta da fonte e ausência de alterações fora da remoção. O verificador gráfico cobre 32 fases, permite diferenças somente nas regiões de copas/grama/mascote, testa os relógios em ENTER/ESC, fallback com recursos ausentes/inválidos e pares incompletos, desenho sem mutação, descarregamento e o fluxo existente de teclado/mouse até seleção/exploração. Os testes isolados do mascote congelam o relógio do vento.

Os novos diagnósticos ficam em `build/menu_tree_preview/`: `regions.png`, máscaras e pesos por copa, `exposed_mask.png`, `animated_pixels.png`, `inspection.json`, `scene.gif` em 1280 × 720, contatos `scene_foreground_contact.png`/`scene_distant_contact.png` e capturas OpenGL `title_game.gif`, contatos correspondentes e `menu.png`. `title_game_foreground_closeup_contact.png` amplia as pontas e ligações das quatro copas grandes. Os contatos incluem 31 → 0 → 1 e a fase de maior curvatura. O preparador verifica em todas as fases que madeira e áreas protegidas da grama conservam os pixels do fundo anterior.

Na validação automatizada de 05/11/2026, a compilação, o verificador OpenGL e `git diff --check` passaram. Foram inspecionadas capturas do título/menu em 1280 × 720 e contatos ampliados da curva máxima e da fronteira do ciclo. A inspeção dessas capturas é distinta do teste manual de interação. Os GIFs usam quantização apenas para a prévia; os PNGs de execução preservam as cores originais.

Na extensão das copas, os 32 quadros OpenGL também foram comparados com as capturas anteriores de grama/mascote: nenhuma diferença fora da máscara das copas projetada na janela (guarda de dois pixels para rasterização). Os hashes das fontes, derivados da grama, mascote e código das demais telas permaneceram iguais. Foram inspecionados os contatos de primeiro plano e árvores distantes, incluindo a passagem do ciclo e as ligações. As folhas distantes conservam a variação de cor já presente na fonte; não foi aplicado desfoque novo.

Validação manual relatada pelo usuário: as animações de grama e copas foram testadas manualmente no jogo e aprovadas como naturais. Essa aprovação visual é registrada separadamente dos resultados automatizados acima.

Touceiras sobrepostas às copas nas bordas e detalhes muito pequenos ao fundo permanecem estáticos para preservar as árvores. A reconstrução pode simplificar discretamente a textura antes encoberta. O movimento inteiro de um pixel produz pequenos degraus próprios da animação pontual; não há filtro para escondê-los.
