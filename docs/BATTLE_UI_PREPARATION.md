# Combate — interface botânica

Estado: **interface integrada ao combate da campanha**.

A referência aprovada é `assets/ui/battle/approved_reference_v1.png`, cópia da
versão visual v4. A arena mantém as plantas em diagonal e o menu principal
com Atacar, Trocar, Mochila e Fugir. O topo mostra somente o nome do interior.

## Arquivos prontos

| Recurso | Arquivo | Origem |
|---|---|---|
| Cenário do depósito | `assets/maps/battle/depot_v1.png` | Reaproveitado sem edição. |
| Cenário do viveiro | `assets/maps/battle/nursery_v1.png` | Reaproveitado sem edição. |
| Base de terra | `assets/ui/battle/ground_v1.png` | Reaproveitada; duas instâncias na arena. |
| Card de status simples | `assets/ui/battle/status_frame_v2.png` | Nova geração, sem HP ou texto gravado. |
| Painel inferior | `assets/ui/battle/command_panel_v2.png` | Nova geração, com divisor vertical e regiões vazias. |
| Plaquinha do interior | `assets/ui/battle/location_plaque_v1.png` | Nova geração vazia, serve a Depósito e Viveiro. |
| Babosa definitiva | `assets/sprites/plants/aloe_v2.png` | Original fornecido pelo usuário, importado sem edição. |
| Girassol definitivo | `assets/sprites/plants/sunflower_v2.png` | Original fornecido pelo usuário, importado sem edição. |

As seis iniciais continuam nos arquivos `clover_v2.png`, `poison_ivy_v2.png`,
`pepper_v2.png`, `cactus_v2.png`, `orchid_v2.png` e `venus_flytrap_v2.png`
de `assets/sprites/plants/`. O manifesto aponta para esses arquivos existentes;
não há cópias extras nem sprites espelhadas gravadas.

Também são reutilizados o cursor e o botão de rodapé da seleção, o painel
botânico de `UiTheme` e a Pixelify Sans com sua licença. As molduras antigas
mais ornamentadas em `output/imagegen/` não são as escolhidas para a versão final.
O jogo consome `assets/`, sem depender de `output/` ou Downloads.

## Metadados do pacote

Todos ficam em `assets/ui/battle/`:

- `asset_manifest_v1.json`: 18 imagens referenciadas e uma fonte, com hashes,
  dimensões, modos, recortes e insets de nove partes.
- `layout_spec_v1.json`: composição inicial em 1280 × 720, posições, cores,
  tipografia, menus e regras de apresentação a preservar.
- `generation_prompts_v1.json`: prompts completos das três peças novas,
  geradas pelo ImageGen integrado.
- `validation_v1.json`: resultados das conferências de arquivos e limitações.

Os PNGs foram preservados como recebidos ou gerados, sem recorte destrutivo,
redimensionamento ou limpeza de alfa. Os recortes de desenho medidos com alfa
>= 128 excluem margens vazias e resíduos quase transparentes. Usar filtro pontual.
Os insets protegem folhas e cantos; o encaixe final deve ser calibrado no render.

## Texto e estados ficam no código

Os nomes, textos, números de vida e barras de HP não fazem parte das texturas.
Usar a **fonte real Pixelify Sans**, rasterizada no tamanho físico necessário,
sem negrito/itálico artificiais e com posições alinhadas a pixels físicos.

As quatro células simples do menu são desenhadas pela interface, sobre o
lado direito vazio do painel. Não exigem quatro PNGs nem imagens para cada
estado. Creme/verde claro indica disponível ou selecionado; cinza indica
indisponível. O texto “Em breve” e o aviso “Vamos implementar em breve” são
dinâmicos. Mochila e Fugir ficam bloqueados, sem consumir turno.

Atacar abre os três golpes do catálogo atualizado. Trocar abre as posições da equipe,
com as miniaturas das mesmas sprites e HP/estado dinâmicos. A disposição
interna desses submenus foi montada e conferida nas capturas reais: golpes
em três linhas e equipe em uma grade de quatro posições, com ativa, derrotada
e vaga livre identificadas. Não foram necessários novos PNGs.

Fórmula, montagem interna de equipe, substituição obrigatória, espera da CPU e
resultado reutilizam painéis e botões do pacote. Não inventar ações, golpes,
inventário ou fuga para preencher a interface.

## Orientação das plantas

As sprites rivais, incluindo a Babosa, são espelhadas **somente na horizontal
durante o desenho**, mantendo o pivô das raízes. Os arquivos originais, cenário,
plataformas, letras e HUD não são invertidos. As oito espécies foram conferidas
como ativas e rivais nas capturas da arena.

Não foram preparados sprites de costas ou atlas de ataque. A referência usa
as poses frontais existentes. Animações ou efeitos futuros não estão incluídos.
Os arquivos antigos `aloe_guard.png` e `sunflower_guard.png` continuam no
repositório. A exploração usa `aloe_v2.png` para a Babosa e mantém
`sunflower_guard.png` para o Girassol; o combate usa `aloe_v2.png` e
`sunflower_v2.png`.

## Implementação e controles

`Game.battle_assets` possui os dois cenários, terra, status, painel, plaquinha
e os dois bosses definitivos. `battle_screen_assets.c` carrega, verifica dimensões
e descarrega esses recursos, com filtro pontual e recortes do manifesto.
As seis iniciais são emprestadas de `IntroAssets`; as fontes e a seta são
emprestadas de `SelectionUiAssets`. Não há cópia ou liberação dupla dos recursos.
Os atlas são atualizados no tamanho físico do viewport também durante a batalha.

`BattleScreen.scene` é definido pelo encontro e permanece igual ao trocar
plantas; `BattleMenu` guarda apenas o submenu e o foco da interface. O desenho
não altera o estado. Os comandos continuam passando por `Battle_ExecuteMove`,
`Battle_SwitchPlant` e `Battle_SkipParalyzedTurn`, com planejamento por `AI_GetPlannedAction`, sem calcular dano ou alterar HP na tela.

- Setas/WASD e ENTER/Espaço navegam e confirmam; clique confirma ao soltar no mesmo controle.
- Atacar abre os golpes; **1/2/3** continuam disponíveis como atalhos diretos.
- Trocar abre a equipe; **1/2/3/4** escolhem as posições somente nesse menu ou na troca obrigatória.
- **ESC**, Backspace ou Voltar fecham um submenu voluntário sem consumir turno.
- Na raiz, na montagem ou na substituição obrigatória, **ESC** pede confirmação
  antes de sair ao menu. No painel de atributos, ESC apenas fecha a consulta.
- Com o painel aberto, ataques, trocas e resolução pendente ficam bloqueados;
  TAB também fecha. Números da fileira superior e do numpad são equivalentes.
- Na troca obrigatória, a equipe abre automaticamente e somente reservas vivas
  podem entrar. Ações humanas, da CPU e paralisia exibem mensagens com avanço automático em 1,5 s após a animação, com opção de adiantar.
- No beat da mensagem, golpe físico avança e volta, golpe de status pula e golpe
  à distância mantém a planta parada.
- No resultado, ENTER ou o botão retorna à estufa. Ações de combate ficam bloqueadas.

Na campanha, a placa perto do depósito abre o novo modal de abas por planta.
A pergunta resolvida libera a edição; ataque, condições e operador são escolhidos
pelo jogador e só entram em combate após a confirmação daquela planta.
As plantas sem configuração confirmada mantêm os golpes normais.
Consulte [o modal de golpes lógicos](LOGIC_SETUP_UI.md).

Como ferramenta interna, fórmula e montagem de equipe usam fonte Pixelify, painel botânico e o
cenário do encontro escurecido. Na fórmula, setas esquerda/direita ou os cartões
alternam a planta; cada uma preserva sua fórmula e seu golpe de dano escolhido.
Golpes de atributo não recebem bônus de poder. Uma fórmula falsa bloqueia o
golpe escolhido; limites de atributo e frutos também são validados pelo motor.
O menu indica indisponibilidade sem enfileirar ações inválidas. A avaliação V/F e
o bônus ou bloqueio aparecem no próprio ataque lógico, sem ocupar o rodapé.
O atalho TAB fica dentro do painel à esquerda; a mensagem da última ação tem
espaço para três linhas acima da moldura inferior.
**TAB** alterna a consulta de HP, atributos, frutos e status das duas equipes.
Durante uma ação pendente, o painel mostra a ação e o progresso da espera,
impedindo nova entrada de combate até sua resolução.

## Validação executada

Com o ambiente opcional de ferramentas já utilizado no projeto:

```powershell
.\build\trainer_tools\Scripts\python tools/validate_battle_ui_assets.py
```

O validador verifica hashes, existência dos recursos e licença, dimensões,
RGBA/alfa das peças transparentes, centros opacos das molduras, recortes,
insets válidos, oito espécies, associação aos dois cenários e os estados
de disponibilidade descritos no JSON. Não executa a lógica do jogo.
As três peças novas foram inspecionadas visualmente e o divisor do painel
foi medido nas colunas 910–914 da imagem original.

Verificação da implementação com o motor real e OpenGL:

```powershell
cmake --build build --target plants_x_plants battle_check battle_screen_check intro_check name_entry_check
.\build\battle_check.exe
.\build\battle_screen_check.exe
.\build\intro_check.exe
.\build\name_entry_check.exe
```

As capturas ficam em `build/battle_ui_preview/`: depósito/viveiro, todas as
espécies, submenus, quatro posições, espera da CPU, troca obrigatória, fórmula,
montagem interna, resultado e ausência de imagens. Incluem 720p, 1080p e viewport
1024 × 576 em janela 4:3. Os testes verificam também bloqueio dos placeholders,
trocas inválidas, fórmula por planta, terceiro golpe com e sem fruto, bloqueio lógico,
paralisia, consulta de status, fila sem ação duplicada, desenho imutável e liberação dos recursos.

Os três ataques ocupam y=522–680, dentro da área creme do painel. O botão Voltar
fica no alto da coluna esquerda, em `(390,520,120,28)`, junto ao atalho de efeitos.

## Efeitos de ataque v0

Os golpes possuem famílias visuais: folhas, pétalas, espinhos, cipós/raízes, fogo,
raio solar, eletricidade, mordida, avanço físico, buff e debuff. São formas e
partículas desenhadas no raylib, sem novas imagens. As partículas usam grade de
três pixels; o recuo e o movimento da planta ficam alinhados à mesma grade.

Cada golpe válido inicia um efeito de 0,95 s, com impacto em 0,38 s. O HP mostrado
fica no valor anterior até o impacto e converge ao valor já calculado pelo motor
nos 0,45 s seguintes. O número exibido acompanha a barra. Apenas a apresentação é
interpolada: HP real, dano, turnos, precisão e críticos continuam no motor.

Acertos têm recuo e piscada; críticos têm mais partículas douradas e uma tremida
curta na planta atingida. Erros terminam com partículas de passagem sem reação de
acerto nem som de impacto. Golpes fortalecidos só recebem brilho e som extras
quando o motor confirmou o bônus. Buffs/debuffs mostram setas verdes/roxas, e
recuperação líquida de HP recebe partículas verdes. Dano passivo recebe partículas
roxas. Um último impacto conclui a animação antes da tela de derrota.

Som e imagem compartilham o mesmo resultado de golpe e instante de impacto.
Os novos sons são sintetizados no carregamento, com recursos liberados no
encerramento. Nenhum efeito visual ou variação sonora usa o RNG do combate.
O modal de saída pausa o relógio visual e a fila de efeitos sonoros pendentes.

Verificação: `cmake --build build --target battle_fx_check` e
`build/battle_fx_check.exe`. Exercita 24 golpes nos dois lados, erro, crítico,
fortalecimento, HP exibido, eventos inválidos, último impacto, carregamento de
áudio e desenho imutável. Capturas em `build/battle_fx_preview/`.

## Leitura da rodada v0

A barrinha de dois segundos foi removida. Durante a resolução, os comandos são
substituídos por um cartão com a mensagem atual e a indicação **Avança sozinho · ENTER / clique: adiantar**. A esquerda mostra **Rodada em andamento** e a ordem real das
plantas. Durante o efeito, a indicação passa a **Aguarde a animação...**.

Cada confirmação adianta uma única página: uso do golpe, dano ou atributo e efeitos
extras, sem repetir a animação. Segurar a entrada que escolheu o golpe ou confirmar
durante a animação não pula mensagens. TAB consulta os efeitos sem avançar páginas;
ESC fecha essa consulta. Cada página avança sozinha após 1,5 s, contados depois da animação. A consulta de
efeitos e o modal de saída pausam esse intervalo. O fim da batalha aguarda a leitura
do último resultado.

Verificação: `cmake --build build --target battle_flow_check` e
`build/battle_flow_check.exe`; capturas em `build/battle_flow_preview/`.
