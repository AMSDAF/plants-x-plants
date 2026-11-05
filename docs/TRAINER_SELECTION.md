# Seleção de exploradores

## Recursos e preparação

As fontes `assets/sprites/trainer_male_portrait.png` e `trainer_female_portrait.png` são preservadas. `tools/prepare_trainer_portraits.py` usa as dependências de `tools/trainer_requirements.txt` e produz somente derivados de seleção em `assets/sprites/trainers/`:

- `male_portrait.png` e `female_portrait.png`: quadro transparente de 320 × 480, escala uniforme, altura aparente equivalente e pés no mesmo nível.
- `male_portrait_breath.png` e `female_portrait_breath.png`: 32 quadros em oito colunas e quatro linhas (2560 × 1920).

A inspeção das fontes encontrou alfa de 0–255 no masculino e 0–254 no feminino, além de resíduos de alfa muito baixo fora do corpo. A limpeza mantém o maior componente conectado acima de alfa 24 e sua margem de um pixel, removendo alfa abaixo de 8. Não há remoção por cor: os realces claros da camisa permanecem. Chapéus, folhas e botas continuam completos.

Relatórios com hashes SHA-256, limites do recorte, escala e contagem de resíduos ficam em `build/portrait_preview/`, junto das prévias e mapas de diferença. Esses arquivos não são recursos de execução.

## Movimento

O ciclo periódico de 5,12 s altera apenas uma elipse suave no tecido do tórax. A máscara decai até zero com borda contínua. O deslocamento máximo vertical é 0,75 pixel no quadro preparado (aproximadamente 0,62 pixel após a escala na moldura); a expansão horizontal é ainda menor. As amostras usam alfa pré-multiplicado para não gerar franjas. Fora da elipse, cada quadro recebe uma cópia exata do quadro estático: rosto, chapéu, mãos, pernas, botas e mochila não oscilam. A Exploradora começa com deslocamento de fase de 3/8 de ciclo. Não há escala ou salto vertical do retrato inteiro.

A amplitude é deliberadamente discreta e afeta principalmente o tecido central; não representa uma animação articulada de corpo completo. Uma pequena interpolação subpixel ocorre dentro do tórax. A preparação verifica igualdade fora da máscara e simetria das amostras próximas à emenda do ciclo.

## Fluxo e validação

Esquerda/direita e A/D selecionam, assim como clicar no retrato. ENTER e **Confirmar** entram no teste existente de exploração. ESC e **Voltar** retornam ao menu aberto. Um clique iniciado no botão e solto fora dele é cancelado. As transições continuam em `game.c`, aguardando a soltura das entradas sem bloquear a janela.

Validação executada em 05/11/2026:

- `cmake --build build` e `git diff --check`.
- Script de preparação com verificações de alinhamento/máscara e ciclo simétrico.
- Verificador temporário em `build/portrait_selection_check.c`, compilado com os módulos reais e raylib: setas/A-D, seleção por clique sem confirmação, ENTER, ambos os botões, cancelamento ao soltar fora, ESC, reentrada e exploração com ambos os treinadores.
- ENTER e D mantidos durante a transição não confirmam novamente nem movem o personagem; a exploração aguarda ambas as solturas.
- Renderização OpenGL das 32 fases no fundo da seleção, inspeção de chapéus/botas, escala e bordas; desenho sem mutação do estado e fallback estático sem os dois atlas de animação.

As capturas `runtime_00.png` a `runtime_31.png` e `runtime_static.png` ficam em `build/portrait_preview/`. A automação externa de teclado da área de trabalho não acionou o menu; a validação de entrada usou eventos de automação da própria raylib, com janela e contexto OpenGL reais. Não houve teste manual por uma pessoa.

## Floresta em camadas e molduras

As sete fontes fornecidas estão em `assets/sprites/`, sem alterações. `tools/prepare_selection_scene.py` também aceita essas fontes sob `assets/sprites/selection/`, com prioridade para essa pasta. Os derivados ficam separados em `assets/sprites/selection/prepared/`. A preparação usa Pillow, NumPy e SciPy já empregados pelos outros scripts.

A limpeza usa componentes de alfa sólido e reconstrução conectada, com antialias limitado à vizinhança imediata do objeto. Não classifica a cor dos pixels: madeira escura, ramos, folhas e raízes permanecem. Foram inspecionadas as comparações de todas as camadas contra fundos claros e escuros. A moldura tem alfa zero na abertura e no exterior; o centro da placa de madeira é opaco. O terceiro dossel atravessa a metade nominal da folha, por isso os recortes usam divisões medidas. Plantas também usam separações ajustadas. Hashes, limites reais e contatos ficam em `build/selection_preview/`.

O fundo cobre 1280 × 720 por escala uniforme. Árvores e raízes ficam imóveis; cinco dosséis giram discretamente nos pontos de ligação, com vento comum e fases distintas. Quatro plantas de canto têm pivôs abaixo da borda inferior, ocultando o término das raízes. Três nuvens derivam a velocidades distintas e só trocam de lado inteiramente fora da janela. Dez partículas variam tamanho, velocidade, rotação e transparência. Os sprites disponíveis não são usados todos na composição.

As molduras preservam a proporção 2:3. A arte dos retratos usa escala uniforme de 0,83 para caber na abertura, com chapéus e botas completos, e mantém os mesmos atlas e fases de respiração. A seleção interpola o tom quente da madeira e do retrato e exibe um pequeno marcador oliva na placa. Os botões e suas áreas de interação foram preservados. Sombras locais ajudam a leitura sem tornar a abertura um painel opaco.

Validação adicional executada em 05/11/2026:

- `cmake --build build` e `git diff --check`.
- `selection_scene_check`: teclado, mouse, clique sem confirmação, soltura fora do botão, ambos os treinadores na exploração, confirmação, ESC, Voltar, reentrada e entradas mantidas durante a transição.
- Verificação do limite do wrap das nuvens e 12 minutos simulados de vento/partículas, com posições e fases finitas.
- Capturas OpenGL dos dois estados de seleção e 48 fases durante seis segundos, mais fallback sem cenário/moldura/animação e falha parcial sem troncos. O desenho foi verificado sem mutação do estado.
- Inspeção das capturas para enquadramento, alinhamento dos nomes, abertura transparente, ausência de retângulos de névoa e ligação da vegetação. Comparação de hashes confirma que menu, exploração, retratos e fontes anteriores não foram alterados.

Para repetir, compile o alvo opcional `selection_scene_check`, execute-o na raiz e rode `tools/preview_selection_scene.py` com o ambiente Python de preparação. As capturas e `selection_scene.gif` ficam em `build/selection_preview/`. O GIF repete um trecho de seis segundos; a volta ao início do clipe não representa o wrap contínuo usado pelo jogo.

Limitações visuais: os dosséis e plantas usam rotação rígida suave, sem deformar folhas individuais. A arte gerada mantém pequenas diferenças de textura/pixelização entre as camadas. A respiração é ainda mais discreta dentro da moldura (menos de um pixel). A validação de entrada foi automatizada com eventos raylib; não houve teste manual por uma pessoa nesta etapa.

Arquivos desta etapa:

- `src/forest_scene.c`, `include/forest_scene.h`: módulo de cenário em camadas.
- `src/trainer_select_screen.c`, `include/trainer_select_screen.h`: integração, ciclo de vida e molduras.
- `include/selection_scene_data.h`: recortes medidos, gerados pelo script.
- `tools/prepare_selection_scene.py`: derivados e diagnósticos de alfa.
- `tools/check_selection_scene.c`, `tools/preview_selection_scene.py`: verificação gráfica e montagem da prévia.
- `assets/sprites/selection/prepared/`: sete PNGs de execução, separados das fontes.
- `CMakeLists.txt`, `README.md`, `docs/ARCHITECTURE.md` e este relatório: compilação e documentação.
