# Fundo experimental da seleção

A fonte `assets/sprites/trainer_selection_background.png` permanece intacta.
Na seleção existente (**Campanha**), **F7** alterna a prévia exclusiva do novo
fundo. A prévia não mostra interface nem aceita confirmação; **ESC** retorna ao
menu. F7 novamente recupera os controles e a seleção anterior. Entrar novamente
por Campanha restaura a seleção normal. O menu aprovado não foi modificado.

## Preparação reproduzível

Com o ambiente Python de preparação descrito no README:

```powershell
.\build\trainer_tools\Scripts\python tools/prepare_trainer_selection_background.py
cmake --build build
cmake --build build --target trainer_background_check selection_scene_check
.\build\trainer_background_check.exe
.\build\selection_scene_check.exe
.\build\trainer_tools\Scripts\python tools/preview_trainer_selection_background.py
git diff --check
```

O preparador usa somente Pillow, NumPy e SciPy, já usados pelas ferramentas do
projeto. Não são dependências de execução. Grava `base.png` e `atlas.png` em
`assets/sprites/selection/background/`, além de
`include/trainer_background_data.h`. Máscaras, hash da fonte, regiões,
reconstrução, capturas OpenGL, contatos e GIF ficam em
`build/trainer_selection_preview/`, ignorado pelo Git. Capturas PPM temporárias
evitam compressão PNG custosa e ocupam aproximadamente 1,4 GB.

## Movimento e composição

- 16 touceiras isoladas: deslocamento máximo de 2 a 4 pixels da fonte, com os
  últimos cinco pixels da base ancorados.
- Duas copas laterais: poses de até 5 pixels, reduzidas suavemente perto da
  madeira e das bordas. Somente o verde selecionado é extraído; aberturas de céu
  continuam transparentes. Troncos, raízes, pedras e arbustos junto às raízes
  ficam imóveis.
- Três áreas distantes: até 2 pixels, com base e laterais fixas para não criar
  recortes retangulares móveis no horizonte.
- Vento comum do pipeline do menu, com atraso espacial, 32 fases a 8 FPS e
  período de quatro segundos. Poses únicas compactadas em atlas 2048 × 1246.
- Duas nuvens extraídas, sem recriação de arte: deriva para a direita, cerca de
  14–15 pixels da fonte por segundo. Cada uma percorre a largura da cena mais
  sua largura em 128 segundos. O wrap acontece com a nuvem inteira fora da cena.

Uma máscara fixa de oclusão, extraída do fundo reconstruído, mantém as nuvens
atrás das árvores. A ordem é fundo, nuvens, oclusão e vegetação animada.

A reconstrução da vegetação afeta apenas contornos expostos, com doadores locais
e guardas estruturais. O céu exposto pelas nuvens usa interpolação harmônica a
partir da borda local. Toda restauração é quantizada nas cores dos doadores;
poses usam amostragem inteira, alfa binário e nenhuma filtragem de cores.

`trainer_background.c` carrega e libera as texturas uma vez. Atlas e base
formam um par indivisível, validado por dimensões. Qualquer falha parcial usa a
fonte estática completa. Sem a própria fonte e sem o par animado, resta apenas
a cor de fundo. Todas as camadas compartilham escala uniforme, centralização
e filtro pontual; o desenho não altera estado. O relógio avança na atualização,
inclusive durante bloqueios de entrada, com delta limitado a 0,1 segundo.

## Validação e limites

O verificador usa os módulos reais, eventos raylib e renderização OpenGL oculta.
Exercita F7, confirmação ignorada na prévia, ESC, escolha normal, fallback de
atlas/base, desenho sem mutação e liberação. Captura 512 fases de 0,25 segundo,
mais o fechamento exato e amostras de 127,875 / 128 / 128,125 segundos.

O montador exige igualdade entre início e fim do ciclo, igualdade dos dois
fallbacks com a fonte transformada e estabilidade dos pixels estruturais e
bases selecionadas. Produz `scene_loop.gif` com a cena completa, ciclo real de
128 segundos, 836 × 470 e 4 FPS. A execução usa 8 FPS para vegetação e atualização
contínua das nuvens. A paleta de até 256 cores do GIF reduz o gradiente do céu;
os PNGs de execução preservam as cores da fonte. Contatos ampliados permitem
inspecionar alinhamento, contornos e wraps. Capturas automatizadas e inspeção de
quadros não substituem uma sessão manual prolongada nem cobrem todas as GPUs.

Na etapa inicial do fundo não foram implementados cartões ou controles. A
composição atual os desenha sobre este mesmo fundo; consulte
[TRAINER_SELECTION.md](TRAINER_SELECTION.md). Entrada de nome continua futura.

Validação executada nesta entrega: `cmake --build build`, `git diff --check`,
`trainer_background_check`, `selection_scene_check` e `menu_screen_check`
concluídos com sucesso. Fonte e derivados foram reproduzidos byte a byte.
Foram inspecionados o PNG de cena completa, os contatos ampliados de contorno,
wrap e limite do ciclo, além de quadros decodificados do GIF. Não foram
observados recortes retangulares móveis ou desalinhamento nas amostras
inspecionadas. A verificação gráfica foi por capturas, sem sessão manual
prolongada.
