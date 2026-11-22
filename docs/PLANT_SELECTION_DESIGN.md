# Seleção de plantas — preparação visual aprovada

Estado: **seleção implementada e validada com os módulos reais do jogo**.

A referência aprovada está em
`assets/ui/plant_selection/approved_reference_v1.png`. A implementação segue a referência; a montagem deve
seguir essa imagem: seis cartões de madeira com folhas, fundo creme, sprites
lutadoras maiores e fonte Pixelify Sans. Não é uma nova direção visual.

## Entregáveis

Todos os recursos ficam em `assets/ui/plant_selection/`:

| Arquivo | Uso |
|---|---|
| `card_normal_v1.png` | Cartão creme com placa de nome, divisor de atributos e placa inferior, todos vazios. |
| `card_selected_v1.png` | Mesmo cartão com corpo verde-claro e placa inferior oliva. |
| `focus_outline_v1.png` | Contorno dourado com interior transparente, sobreposto ao cartão normal ou selecionado. |
| `footer_button_v1.png` | Moldura vazia para Voltar e Confirmar, com extremidades protegidas por nove partes. |
| `leaf_badge_v1.png` | Folha vazia para receber o número 1, 2 ou 3. |
| `cursor_v1.png` | Seta laranja apontando para a direita. |
| `asset_manifest_v1.json` | Dimensões, recortes de origem, alfa e hashes dos PNGs escolhidos. |
| `layout_spec_v1.json` | Posições, tipografia, estados, camadas e mapa das seis espécies. |
| `generation_prompts_v1.json` | Prompts finais usados no ImageGen integrado. |

São imagens de interface, sem nomes, números, atributos ou plantas gravados
nas molduras. Esses elementos serão compostos dinamicamente, com os dados reais
da equipe e do catálogo. As alternativas descartadas da geração não fazem parte
do pacote.

## Medidas da montagem

As coordenadas do JSON usam o canvas lógico existente de **1280 × 720**.
A referência gerada mede 1672 × 941; usar o enquadramento descrito no JSON,
sem esticar a captura para transformá-la em interface.

Os cartões têm caixas de 350 × 208, nas colunas x = 98, 465 e 832 e nas linhas
y = 162 e 388. As posições foram aproximadas a partir da referência aprovada.
O título fica centralizado; o rodapé reserva uma área de 240 × 64 para Voltar,
outra de 350 × 64 para Confirmar e a instrução entre ambas. Ambos ficam em
y = 628; as bordas externas alinham com a grade (x = 98 e x = 1182).

Normal e selecionado usam exatamente o mesmo recorte de origem:
`[118, 81, 1378, 814]`. Enquadrar esse recorte uniformemente dentro da caixa
do cartão. As sprites cabem em 107 × 113, com base em y = 179 relativo ao cartão,
para deixar respiro acima e abaixo. As raízes ficam alinhadas à base da área; folhas, espinhos
e chamas devem caber sem corte. As medidas internas do JSON se referem à caixa
do cartão e exigem conferência visual no futuro render real.

O foco tem recorte próprio `[115, 78, 1382, 816]`, mas compartilha o canvas de
origem com os cartões. Aplicar a mesma escala e origem de registro do cartão:
compensar a diferença de -3 pixels no x e no y da origem, em vez de normalizar
cada textura isoladamente. Isso mantém o contorno alinhado. O manifesto usa
alfa >= 128 para medir limites visíveis e excluir resíduos quase transparentes
de geração; o carregamento deve preferir os recortes registrados.

Os botões devem preservar cantos e folhas. O manifesto traz os insets de origem
para desenho em nove partes: esquerda/direita 260, topo/base 88 pixels.
Dimensionar as extremidades pela escala da altura e alongar só as faixas
centrais. Não comprimir a textura inteira para o botão menor.

## Fonte e conteúdo

Usar `assets/fonts/pixelify_sans_regular.ttf`, já carregada e acessível por
`ui_pixel_font` como reserva. A seleção possui seis atlas próprios em
`IntroAssets.fonts`. Os tamanhos lógicos são 72 para título, 30 para nomes,
28 para botões, atributos e contador, 26 para status/números e 20 para instrução.
A calibração visual aumentou os tamanhos inicialmente propostos, pois os glifos
reais ocupam uma parte menor do tamanho nominal. O reforço horizontal artificial foi removido. Cada atlas é rasterizado no
tamanho físico necessário (por exemplo, fonte 28 vira atlas 42 em 1080p),
com posições alinhadas a pixels físicos. Os atlas são reutilizados e só são
recriados quando a resolução exige outro tamanho. A fonte dos diálogos
permanece independente.
Os quadros de carta, aceite e viagem também usam o atlas próprio de 28 px
lógicos, tanto no nome quanto no texto, evitando reduzir o antigo atlas de
28 para 27 px e ampliá-lo novamente. A quebra e a revelação continuam por
codepoints, com linhas alinhadas a pixels físicos do viewport.
Seleção de personagem, entrada de nome e todas as etapas do prólogo recebem
a mesma sobreposição verde-escura de 72%, aplicada por `trainer_background`.
A prévia F7 continua exibindo somente o cenário original.
A folha numerada fica centrada verticalmente na faixa do nome; os números e
os textos dos botões são centralizados pelos limites visíveis dos glifos.
Medir os nomes acentuados e ajustar somente se necessário, sem remover acentos.

Reutilizar as seis sprites `*_v2.png` já integradas à seleção. O mapa está no
JSON, incluindo Trevo, Hera Venenosa, Pimenta, Cacto, Orquídea e Planta Carnívora.
Os atributos devem vir de `PlantData_GetStarter`; os números registrados na
especificação documentam a referência, não substituem o catálogo.

Fundo e animação continuam pertencendo ao módulo existente
`trainer_background`. Não gerar outra floresta nem mudar a arte das plantas.
Usar filtro pontual e o mesmo canvas com enquadramento 16:9 do jogo.

## Estados e comportamento a preservar

- Normal: cartão creme, sem folha numerada.
- Selecionado: cartão verde-claro, folha com número e status da equipe.
- Foco: contorno dourado e seta à esquerda, independentemente da seleção.
- Selecionado com foco: combinar cartão verde, número, contorno e seta.
- Equipe cheia: manter três escolhas; permitir remover uma escolhida.
- Confirmar: habilitar somente com três plantas; manter o bloqueio de entrada
  nas transições e os controles atuais de teclado e mouse.

Não incluir raridades, tipos, novos golpes ou outras funções. Os recursos pertencem a `Game` por meio de `IntroAssets`, são carregados uma vez
em `intro_assets_load` e liberados em `intro_assets_unload`. O desenho deve continuar somente de leitura.

## Conferência desta preparação

Os seis PNGs foram abertos e inspecionados. Formato RGBA, presença de alfa
transparente, centro transparente do foco, recortes dentro da imagem e hashes
foram verificados. Normal e selecionado têm as mesmas dimensões e recorte
compartilhado. O relatório está em `validation_v1.json`.

A montagem está em `intro_screen.c`, sem novas dependências ou alteração das
regras do combate. Os cartões normais/selecionados mantêm registro comum, o
contorno é sobreposto com compensação da origem e os botões preservam nove
partes. O foco acompanha teclado ou movimento do mouse sem selecionar sozinho.

Validação executável: `cmake --build build --target plants_x_plants intro_check`
e `.\build\intro_check.exe`. O alvo cobre seleção por teclado/mouse, confirmação
bloqueada/habilitada, limite de três, remoção e renumeração, retorno às etapas,
entrada mantida, preservação da equipe nos dois bosses, desenho imutável e
liberação das molduras. Capturas em `build/intro_preview/` incluem 0/1/2/3
escolhas, foco em escolhida, 1920 × 1080, viewport 1024 × 576 em janela 4:3,
reserva sem as molduras novas e reserva sem Pixelify.

A comparação visual conferiu a referência aprovada e o render real no mesmo
estado (três escolhidas e foco em Cacto). Cartões, botões e textos cabem no
quadro; nomes longos permanecem legíveis. A fonte real é Pixelify Sans, enquanto
a imagem de conceito contém letras sintetizadas; as formas não são idênticas.
As sprites originais foram preservadas, sem redesenhar as plantas para copiar
as pequenas diferenças da geração do conceito.

## Cabeçalho e rodapé do prólogo

Carta, aceite, seleção e viagem compartilham o título grande Pixelify e os seis
marcadores de progresso, o subtítulo contextual e os botões de madeira na
mesma altura. Nas etapas de texto, as bordas externas dos botões acompanham
o quadro: Voltar em x = 120 e avançar em x = 810, terminando em x = 1160.
Na seleção, acompanham a grade de cartões. A instrução fica centralizada no
espaço entre os botões. O rótulo superior “Plants x Plants / Prólogo” foi removido.
Na seleção, a instrução mantém os controles das plantas; nas outras etapas,
indica ENTER ou clique para completar/avançar o texto. O botão direito mostra
“Completar texto” enquanto a fala aparece e a ação da etapa quando termina.
As regras de avanço em dois toques e retorno continuam iguais.

O progresso começa antes da carta: personagem (1/6), nome (2/6), carta (3/6),
aceite (4/6), seleção do time (5/6) e viagem (6/6). `selection_ui_progress`
desenha a mesma barrinha em todas as telas, a partir da etapa atual, inclusive
quando o jogador volta.

As capturas `letter_1080p.png`, `accept_1080p.png` e `travel_1080p.png` em
`build/intro_preview/` verificam essa composição, além da seleção.
