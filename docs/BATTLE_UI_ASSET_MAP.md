# Tela de combate — mapa de assets e preparação

Estado: **assets integrados na arena da campanha**.

Este mapa reúne o necessário para vestir o combate existente com a direção
botânica do jogo: dois cenários, plantas lutadoras, madeira com folhas,
pergaminho creme e Pixelify Sans. Não altera combate, atributos, golpes,
controles, montagem de fórmula ou progressão. O pacote final e sua validação
estão descritos em [BATTLE_UI_PREPARATION.md](BATTLE_UI_PREPARATION.md).

## Resumo do inventário

| Grupo | Quantidade de arquivos | Situação |
|---|---:|---|
| Cenários de combate | 2 | Usados nos encontros do depósito e viveiro. |
| Base de terra | 1 | Gerada com transparência; reutilizável nos dois lados e locais. |
| Molduras finais | 3 | Status simples, painel inferior vazio e plaquinha do interior em `assets/ui/battle/`. |
| Plantas iniciais | 6 | Artes `*_v2.png` compartilhadas entre seleção e combate. |
| Bosses definitivos | 2 | Importados como `aloe_v2.png` e `sunflower_v2.png`, sem modificar as artes antigas. |
| Botões, cursor e emblema | 3 bases reutilizáveis | Existem na seleção do time; não exigem geração nova. |
| Painel vazio alternativo | 1 reutilizável | Existe na entrada do nome; opção caso o painel com divisões não comporte os controles. |
| Fonte | 1 | Pixelify Sans já distribuída com o jogo; atlas calculados na resolução física. |

**O pacote para a versão estática está preparado.** Foram geradas três peças
mais próximas da direção final aprovada; cenários, terra, fonte e sprites foram
reaproveitados. Manifesto, recortes, insets e layout estão em `assets/ui/battle/`.
A implementação reutiliza esses arquivos e foi conferida nas capturas reais.

## Cenários e chão

| Elemento | Fonte preservada | Arquivo preparado | Uso |
|---|---|---|---|
| Depósito | `output/imagegen/battle_depot_v1.png` | `assets/maps/battle/depot_v1.png` | Encontro com a Babosa. |
| Viveiro | `output/imagegen/battle_nursery_v1.png` | `assets/maps/battle/nursery_v1.png` | Encontro com o Girassol. |
| Terra | `output/imagegen/battle_ground_v1.png` | `assets/ui/battle/ground_v1.png` | Duas instâncias, em escalas e posições diferentes. |

Os cenários medem **1672 × 941**, RGB, sem transparência. São cenários próprios
para a batalha, não substitutos dos mapas de exploração. A associação deve vir
do encontro (`WorldEncounter`), pois a planta ativa rival pode mudar para uma
reserva sem mudar o local da luta.

A terra mede **2171 × 724**, RGBA. O recorte visível ocupa `[176,185,1825,400]`.
Usar enquadramento e filtro pontual; não gerar uma terra nova para cada planta.
Uma eventual sombra de contato pode ser desenhada pela interface.

## Molduras iniciais — histórico

As três molduras abaixo pertencem à primeira preparação. Permanecem preservadas
em `output/imagegen/`, mas as versões finais em `assets/ui/battle/` usam status
mais simples e células de menu desenhadas separadamente. Para implementar,
seguir `asset_manifest_v1.json` do pacote final, não estes recortes históricos.

Todos os arquivos abaixo são RGBA, com alfa de 0 a 255, e estão vazios de texto.
Os recortes foram medidos com **alfa >= 128** e usam `[x,y,largura,altura]`.
São referências de preparo: insets de nove partes e áreas seguras de texto
ainda devem ser calibrados durante a montagem visual.

| Função | Fonte em `output/imagegen/` | Canvas | Recorte visível | Destino proposto |
|---|---|---|---|---|
| Nome, vida e estado da ativa | `battle_status_frame_v1.png` | 2172 × 724 | `[130,144,1912,428]` | `assets/ui/battle/status_frame_v1.png` |
| Botão de golpe ou ação | `battle_action_frame_v1.png` | 2172 × 724 | `[215,199,1741,294]` | `assets/ui/battle/action_frame_v1.png` |
| Mensagem e grade de comandos | `battle_commands_frame_v1.png` | 2172 × 724 | `[65,164,2042,387]` | `assets/ui/battle/commands_frame_v1.png` |

Uma única moldura de status atende jogador e guardião; nome, HP, indicadores
de reserva e turno são dinâmicos. Uma única moldura de ação atende os dois
golpes e pode atender às reservas. Não gerar uma textura para cada nome,
valor de HP, golpe, lado ou estado de hover.

### Restrição do painel de comandos

O painel gerado tem uma região grande para mensagens à esquerda e **quatro
compartimentos fixos** à direita. O sistema atual oferece **dois golpes** e
**até quatro posições de troca** (Q/W/E/R), além da informação da fórmula.
O jogador normalmente começa com três plantas, mas a capacidade continua quatro.

Direção solicitada após a primeira proposta: menu principal no estilo Pokémon,
com **Atacar**, **Trocar**, **Mochila** e **Fugir** em uma grade 2 × 2.
Mochila e Fugir ficam desabilitados, com o aviso “Vamos implementar em breve”.
Não implementar inventário ou fuga nesta etapa. Atacar abre a escolha dos dois
golpes; Trocar abre as posições da equipe. Assim, os quatro compartimentos
servem ao menu principal, sem mostrar todos os golpes e reservas ao mesmo tempo.
Os submenus ainda precisam de composição visual própria, usando molduras
existentes e conteúdo dinâmico. As regras e a capacidade das equipes permanecem.

A proposta visual revisada está em `output/imagegen/battle_ui_pokemon_v4.png`;
o prompt está no JSON de mesmo prefixo. A interface segue essa composição no jogo.
O topo mantém somente a plaquinha do interior (Depósito ou Viveiro), sem o
aviso “ESC · Estufa”, conforme o ajuste visual solicitado.
A revisão usa a tipografia da seleção como referência e espelha a Babosa
horizontalmente para orientar o confronto. Na implementação, usar a fonte real
Pixelify Sans e espelhamento somente da sprite rival, sem inverter cenário ou HUD.

## Sprites das oito espécies

As seis artes iniciais são RGBA e já tiveram seus limites de alfa conferidos.
Precisam somente de enquadramento no combate, com proporção preservada e pés
alinhados à terra. As versões `*_v1.png` das plantas não são a direção lutadora
aprovada e não devem ser escolhidas por engano.

| Espécie | Arte definitiva para o combate | Situação |
|---|---|---|
| Trevo | `assets/sprites/plants/clover_v2.png` | No repositório. |
| Hera Venenosa | `assets/sprites/plants/poison_ivy_v2.png` | No repositório. |
| Pimenta | `assets/sprites/plants/pepper_v2.png` | No repositório. |
| Cacto | `assets/sprites/plants/cactus_v2.png` | No repositório. |
| Orquídea | `assets/sprites/plants/orchid_v2.png` | No repositório. |
| Planta Carnívora | `assets/sprites/plants/venus_flytrap_v2.png` | No repositório. |
| Babosa | `assets/sprites/plants/aloe_v2.png`, original do anexo `Angry Pixel Aloe Plant Monster.png` | Importado, RGBA, 1230 × 1278. |
| Girassol | `assets/sprites/plants/sunflower_v2.png`, original do anexo `Angry Pixel Sunflower Sprite.png` | Importado, RGBA, 1223 × 1286. |

Os bosses antigos `assets/sprites/aloe_guard.png` e
`assets/sprites/sunflower_guard.png` já são carregados pelo mundo e usados no
combate atual, mas têm desenho diferente das oito artes aprovadas. Não
sobrescrever esses arquivos ao preparar o combate; importar os novos com nomes
próprios e manter a exploração independente.

As reservas dos guardiões também podem usar as seis espécies iniciais.
O desenho deve selecionar a arte pelo ID da espécie ativa, sem assumir que
o guardião será sempre Babosa ou Girassol durante toda a batalha.

### Ângulo e animações

Temos uma pose frontal de cada planta, não sprites de costas nem atlas de
ataque. A primeira versão pode usar essas poses nos dois lados da arena.
Se a intenção for mostrar as costas da planta do jogador, como em Pokémon,
isso exigirá novas artes; espelhar a imagem não cria uma vista traseira.

Repouso, ataque, dano, troca e derrota podem inicialmente usar deslocamentos,
pulso de cor e transparência, sem imagens adicionais. Atlas de animação ou
efeitos exclusivos por golpe ficam para uma etapa posterior, se solicitados.

## Elementos a reaproveitar

| Elemento | Recurso | Aplicação |
|---|---|---|
| Botão botânico | `assets/ui/plant_selection/footer_button_v1.png` | Confirmar fórmula e retornar à estufa. |
| Cursor | `assets/ui/plant_selection/cursor_v1.png` | Foco ou escolha de uma planta/ação. |
| Emblema de folha | `assets/ui/plant_selection/leaf_badge_v1.png` | Marcador opcional de posição/ativa, com conteúdo dinâmico. |
| Painel creme sem divisões | `assets/ui/trainer_setup/name_panel_v1.png` | Fórmula, resultado ou alternativa ao painel de comandos. |
| Pixelify Sans | `assets/fonts/pixelify_sans_regular.ttf` | Títulos, nomes, HP, golpes, instruções e fórmula. |

Reutilizar os recortes e as regras de montagem documentadas nos manifestos
existentes. O contorno de foco da seleção de plantas foi gerado na proporção
daquele cartão horizontal; não esticá-lo sobre qualquer painel de combate.
Foco, seleção e desabilitado podem ser desenhados em código.

## O que deve continuar dinâmico, sem PNG novo

- Barras de HP: trilho, preenchimento e cores conforme a vida.
- Nome, HP atual/máximo, poder, precisão e atalhos dos golpes.
- Reservas: miniaturas das mesmas sprites, ativa, viva, derrotada e posição vazia.
- Turno do jogador, espera da CPU e substituição obrigatória.
- Texto da última ação, erro, vitória e derrota.
- Condições, operador, valores V/F e realce do golpe fortalecido.
- Estados normal, hover, foco, pressionado, selecionado e desabilitado.
- Sombras, realces e transições simples, se incluídos na montagem.

Não são necessários PNGs de números, ícones de tipos/raridades, barras de
energia, inventário, captura ou elementos que o jogo não possui.

## Telas e estados que precisam ser contemplados

| Situação existente | Assets necessários |
|---|---|
| Montagem da fórmula antes do primeiro encontro | Painel vazio, ações reutilizáveis, fonte e sprite da ativa. |
| Seleção interna de equipe, usada como alternativa quando não existe equipe da campanha | Seis sprites iniciais, molduras reutilizáveis e texto. Não é uma nova seleção do prólogo. |
| Turno normal do jogador | Cenário do local, duas terras, duas sprites ativas, dois status, comandos, reservas e fórmula. |
| Turno da CPU | Mesmas artes; comandos desabilitados e aviso de espera. |
| Substituição obrigatória | Mesmas artes; escolha de reserva viva e golpes bloqueados. |
| Golpe com fórmula verdadeira/falsa | Mesmas artes; valores e realces dinâmicos. |
| Vitória ou derrota | Mesmo cenário e painel reutilizado com resultado e retorno. |
| Falta de textura ou fonte | Reserva geométrica e tipográfica, sem interromper a partida. |

## Preparação concluída e próximos passos

1. Concluído: direção da arena aprovada, menu raiz com quatro opções e submenus
   para golpes e equipe. A disposição interna dos submenus será conferida depois.
2. Concluído: cenários, terra, três molduras finais e bosses organizados em
   `assets/`, preservando fontes, artes anteriores e arquivos originais.
3. Concluído: manifesto, hashes, recortes, insets e medidas iniciais em
   1280 × 720. Nenhum recurso do manifesto depende de `output/`.
4. Integrado: `Game` possui as artes exclusivas da arena e empresta as seis
   iniciais/fontes, sem duplicar recursos ou descarregar duas vezes.
5. Integrado: entrada e apresentação em `battle_screen`; dano, vida, validação
   das ações e escolhas da IA continuam em `battle` e `ai`.
6. Conferido: 720p, 1080p e viewport com barras laterais, com atlas na resolução
   física, filtro pontual e posições alinhadas aos pixels.
7. Verificado pelo alvo `battle_screen_check`: dois locais, oito espécies,
   quatro posições, cliques, CPU, troca inválida/obrigatória, resultado e fallback.

## Conferência realizada neste mapeamento

Foram lidos os estados e controles de `battle_screen`, o fluxo dos encontros e
o catálogo das oito espécies. As seis artes de arena/HUD, os dois bosses em uso
e os dois anexos definitivos foram abertos e inspecionados. Dimensões e alfa das
artes preparadas e das oito sprites definitivas foram medidos por leitura.

Após o mapeamento inicial, foram geradas três molduras finais e copiados os
cenários, a terra, a referência aprovada e os dois bosses para `assets/`.
Os originais foram preservados. `tools/validate_battle_ui_assets.py` verifica os
arquivos e metadados; `battle_screen_check` exercita a implementação com o
motor real. Comandos e responsabilidades estão no guia de interface.
