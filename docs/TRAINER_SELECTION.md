# Seleção de exploradores

## Composição atual

**Campanha** abre “Escolha seu personagem” sobre o fundo animado aprovado.
Cartões iguais de 336 × 480, controles e título são formas raylib, sem textura
da folha de referência. A seleção muda apenas borda, cantos laranja e cursor;
hitboxes e relógio do fundo permanecem estáveis. “Homem” e “Mulher” reutilizam
os retratos simplificados com filtro pontual, proporção preservada, altura
corporal de 360 pixels e pés em y=526. O homem tem repouso em quatro poses;
a mulher permanece estática. Os controles, F7 e a confirmação para
a exploração permanecem disponíveis. `selection_scene_check` salva ambos os
estados em `build/selection_preview/selected_male.png` e `selected_female.png`.
As seções de molduras de madeira e floresta anterior abaixo são históricas.

## Retratos simplificados v2

`tools/prepare_selection_portraits_v2.py` divide a fonte RGBA de 1536 × 1024
em duas metades completas, com 24 pixels extras de margem. Gera dois PNGs
816 × 1072 e `include/selection_portrait_v2_data.h`, sem remover pixels nem
alterar cores/alfa. O maior componente de alfa ≥128 mede somente o corpo;
nenhuma máscara é aplicada à arte. A fonte tem alfa 0–254; o halo marrom amplo
está principalmente em RGB sob alfa zero e, por isso, quase não aparece no
cartão creme. Não houve recorte retangular visível que exigisse suavização.
Diagnósticos ficam em `build/selection_portrait_v2_preview/`. Após executar
`selection_scene_check`, use o preparador com `--check-runtime` para comparar
a arte estática nas capturas. As fontes e atlas detalhados antigos permanecem
no projeto, mas seus atlas não são carregados pela seleção atual.

## Repouso masculino

`tools/prepare_male_portrait_idle.py` preserva a fonte RGBA
`male_portrait_idle_source.png` (1199 × 1312) e mede cada quadrante separadamente.
Alinha as solas e um pivô horizontal comum, sem redimensionar poses. O atlas
`male_portrait_idle.png` contém quatro células 352 × 656. A parte inferior do
neutro é reutilizada a partir da linha 372, junto ao cinto, mantendo pernas e
botas idênticas. A escala comum é 360/586; não há deformação, rotação ou salto
vertical do corpo inteiro. `male_portrait_idle_halo.png` preserva separadamente
o halo do retrato v2, retirando somente seu corpo opaco; essa camada fica fixa.

Tempos finais: neutro **3,80 s**, inspiração **0,35 s**, piscar **0,10 s** e
relaxado **0,45 s**; ciclo de **4,70 s**. A pausa neutra inicial de 1,80 s foi
ampliada para reduzir o piscar de cerca de 22 para 13 vezes por minuto. As
quatro poses ainda têm pequenas diferenças de rosto/camisa, sem interpolação.
O relógio avança inclusive sem seleção masculina, durante bloqueios e na prévia
F7. Falha no atlas ou halo usa o retrato estático existente. A mulher e os
recursos da exploração não mudam.

```powershell
.\build\trainer_tools\Scripts\python tools/prepare_male_portrait_idle.py
cmake --build build
cmake --build build --target selection_scene_check
.\build\selection_scene_check.exe
.\build\trainer_tools\Scripts\python tools/prepare_male_portrait_idle.py --runtime-preview
git diff --check
```

O preparador usa Pillow, NumPy e SciPy já disponíveis. Metadados ficam em
`include/male_portrait_idle_data.h`; medidas, máscaras, contatos de emenda,
GIFs e capturas OpenGL ficam em `build/male_portrait_idle_preview/`. O GIF
`runtime_card_loop.gif` usa o cartão creme real; os outros relógios ficam
congelados somente nas capturas para isolar a emenda do repouso. Verificações
comparam pernas/botas, mulher, começo/fim e fallback, além dos limites dos
tempos e continuidade do relógio. A revisão técnica usa capturas ampliadas.

**Aprovação visual manual:** o usuário aprovou o repouso masculino antes deste
commit, conforme informado na solicitação de registro. Essa aprovação se limita
à animação masculina; as demais mudanças mantêm os registros de validação
automatizada e inspeção de capturas descritos nesta documentação.

## Recursos e preparação anteriores (histórico)

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
