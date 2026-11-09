# Repouso do mascote do menu

## Fonte e preparação

Única fonte de arte: `assets/sprites/menu_mascot.png`, preservada com SHA-256 `9284b3e35c5e45538a69587aff20906198d0ae0fa337e1767122755809dabfdf`. Mede 1312 × 1199; os limites visíveis acima de alfa 32 são `(218, 101)–(1186, 1095)`. A arte ampliada não tem grade perfeitamente uniforme e contém 38.686 cores nos pixels de alfa acima de 224. Por isso, o trabalho mantém a resolução original, sem reduzir nem converter sua paleta.

`tools/prepare_menu_mascot.py` usa Pillow, NumPy e SciPy já presentes em `tools/trainer_requirements.txt`. Extrai máscaras de corpo, folhas esquerda/direita, braços/punhos e região dos pés. Os deslocamentos são inteiros e a amostragem é pelo vizinho mais próximo. A composição escolhe pixels originais, sem misturar RGB ou alfa. A preparação verifica que cada amostra RGBA dos quadros pertence à fonte.

Não houve pintura de áreas ocultas ou criação de detalhes. As juntas usam uma pequena sobreposição do contorno escuro original; o movimento dos braços decai até zero nessa faixa. A revisão inicial encontrou linhas residuais nessas junções, corrigidas antes da integração ao igualar o deslocamento na região sobreposta. Máscaras, contatos de junções e inspeção da emenda ficam em `build/menu_mascot_preview/`.

Saídas necessárias à execução:

- `assets/sprites/menu_mascot_idle.png`: 4096 × 2048, quatro colunas e duas linhas, oito células de 1024 × 1024, RGBA original.
- `include/menu_mascot_data.h`: geometria e tempo compartilhados com a execução.

No quadro neutro, o recorte estático de 968 × 994 começa em `(28, 16)`. Margens transparentes acomodam as pontas em movimento. A linha inferior dos pés fica em y=1010 em todos os quadros.

## Movimento e integração

São oito quadros distintos, a 6 FPS, ciclo de aproximadamente 1,33 s. O corpo sobe no máximo quatro pixels da fonte, equivalentes a cerca de um pixel na janela. A expressão acompanha rigidamente o corpo: olhos e traços não deformam. A parte inferior transita até deslocamento zero antes dos pés. Os punhos têm fases ligeiramente distintas e as folhas flexionam por deslocamentos horizontais que se anulam nas ligações. As pontas movem até aproximadamente dois pixels exibidos. Ambos os pés são cópias exatas da região original em todos os quadros.

`MenuScreen.mascot_time` avança por delta time antes das verificações de entrada, inclusive quando aguarda soltura. ENTER/ESC e limpeza de entradas não reiniciam o relógio. Fundo, logo, painel, navegação, sombra e posição/escala base do mascote permanecem iguais. Não há alocação ou geração de imagens no loop. A textura é carregada uma vez com filtro pontual e descarregada junto dos outros recursos. Atlas ausente, com dimensão incorreta ou incompatível com o recorte da fonte usa a imagem estática existente.

## Reproduzir e validar

Na raiz, com o ambiente de preparação existente:

```powershell
.\build\trainer_tools\Scripts\python tools/prepare_menu_mascot.py
cmake --build build
cmake --build build --target menu_screen_check
.\build\menu_screen_check.exe
.\build\trainer_tools\Scripts\python tools/prepare_menu_mascot.py --runtime-preview
git diff --check
```

Prévias em `build/menu_mascot_preview/`:

- `idle.gif`: inspeção offline em escala próxima à do jogo, com mascote de cerca de 240 pixels.
- `contact.png`, `joints.png`, `loop_boundary.png`: oito poses, ligações ampliadas e sequência 8→1→2.
- `title_idle.gif`, `menu_idle.gif`: recortes 1:1 das capturas OpenGL de ambas as telas.
- `title.png`, `menu.png`: telas completas renderizadas pelo jogo.
- `inspection.json`: hash, máscaras/geometria, limites de quadros, paleta/alfa preservados, pés constantes, rosto rígido e continuidade da emenda.

Somente os GIFs diagnósticos usam paleta indexada, exigida pelo formato; o atlas de execução permanece RGBA sem conversão. Os quadros têm oito estados distintos também na escala efetivamente renderizada. O script de prévia do jogo compara a região inferior de todas as fases para verificar pés e sombra imóveis.

Inspeção visual realizada: quadros do GIF no tamanho aproximado de exibição, contato das oito fases, junções corrigidas, limite do ciclo e capturas OpenGL das telas inicial e menu. Não houve teste manual por uma pessoa. Os testes automatizados verificam geometria, transparência, cores/alfa da fonte, conectividade principal, região dos pés, relógio contínuo, movimento restrito ao mascote e navegação existente. Fixtures isoladas em `build/` verificam carga sem atlas e com atlas de dimensão inválida, sem alterar os assets originais.

Limitações: a respiração é deliberadamente discreta e usa passos de pixel, sem interpolação suave. A região do rosto move rigidamente cerca de um pixel, mantendo a expressão. Pequenas diferenças de rasterização pontual entre a textura original e o atlas podem ocorrer em uma linha interior da pose neutra; no ambiente validado foram 134 pixels, sem mudar posição, pés ou sombra. Nenhuma área foi redesenhada.
