# Arquitetura — Plants x Plants

Este documento explica a organização do projeto e a responsabilidade de cada arquivo.

O jogo será desenvolvido em C com raylib. A estrutura inicial prioriza simplicidade, divisão de tarefas e separação entre as regras do jogo e a interface gráfica.

> ENTER na tela inicial abre a seleção de personagem, seguida do nome, do prólogo e da Estufa Lógica. As batalhas são encontros contra guardiões da campanha.

## Estrutura de pastas

- `assets/`: fontes, imagens e sons utilizados pelo jogo.
- `docs/`: documentação da arquitetura e backlog.
- `include/`: arquivos `.h`, com estruturas, constantes e declarações de funções.
- `src/`: arquivos `.c`, com a implementação das funções.

Os arquivos `.gitkeep` mantêm as pastas de assets no Git enquanto estiverem vazias. Eles não participam do funcionamento do jogo.

## Responsabilidade dos arquivos

### Inicialização e fluxo geral

| Arquivo | Responsabilidade |
|---|---|
| `src/main.c` | Abrir a janela, executar o loop principal e encerrar o programa. |
| `include/game_canvas.h`, `src/game_canvas.c` | Gerenciar o alvo na resolução física do viewport, mantendo coordenadas lógicas de 1280 × 720, enquadramento, mouse e recortes de interface. O alvo é recriado somente ao mudar de tamanho. |
| `include/ui_theme.h`, `src/ui_theme.c` | Carregar e liberar os dois pesos de Atkinson, a Pixelify dos diálogos e do HUD, as molduras de interação e diálogo e os três retratos, importar o chevron na grade de pixels, atualizar seu relógio visual, medir textos UTF-8 e desenhar molduras creme em degraus ou botânicas compartilhadas. `Game` possui os recursos e os passa explicitamente às telas. |
| `include/exploration_hud.h`, `src/exploration_hud.c` | Desenhar objetivo e aviso botânico inferior com indicação textual de E e seta a partir da descrição somente de leitura da interação. Não seleciona ações nem modifica o mundo. |
| `include/dialogue_ui.h`, `src/dialogue_ui.c` | Compor a arte horizontal botânica, retrato e textos de diálogo, com moldura em nove partes para placas e reserva. Recebe nome, fala, caracteres visíveis e disponibilidade da próxima página; não processa entrada nem modifica o mundo. Recursos de imagem pertencem a `UiTheme`. |
| `include/game.h` | Definir o estado geral do jogo e declarar suas funções de controle. |
| `src/game.c` | Controlar as telas da campanha, a entrada na estufa e os encontros contra guardiões. |

O `main.c` deve permanecer pequeno. Regras de combate, dados de plantas e detalhes das telas devem ficar nos respectivos módulos.

### Tela inicial e exploração

| Arquivos | Responsabilidade |
|---|---|
| `include/menu_screen.h`, `src/menu_screen.c` | Tela inicial em pixel art e relógios contínuos de grama e mascote. Carrega texturas com filtro pontual, calcula limites visíveis uma vez e valida atlas opcional. ENTER abre a seleção de personagem; ESC solicita encerramento. O coordenador protege as transições até soltar entradas. |
| `include/trainer_select_screen.h`, `src/trainer_select_screen.c` | Cartões botânicos de madeira e controles Pixelify sobre `TrainerBackgroundAssets`, retratos de repouso com durações explícitas e relógios independentes e escolha por teclado/mouse. Usa metadados dos atlas de repouso para igualar altura e pés. Retorna confirmar/voltar; clicar no cartão apenas seleciona. F7 exibe somente o fundo, sem reiniciar seu relógio. |
| `include/intro_screen.h`, `src/intro_screen.c` | Pedido de Lia, aceite, seleção de três entre seis plantas, viagem e montagem validada da equipe inicial. Reutiliza o fundo e a interface; `IntroAssets`, pertencente a `Game`, carrega as seis sprites `*_v2.png`, calcula os limites de alfa e usa filtro pontual. Também carrega as seis artes de `assets/ui/plant_selection/`, com recortes fixos, foco independente da seleção e botões em nove partes. A seleção mantém seis atlas Pixelify nos tamanhos físicos do viewport, atualizados fora do desenho quando a resolução muda. O desenho preserva a proporção, alinha as raízes e oferece reserva se faltar uma textura. A entrada usa as mesmas áreas dos novos cartões e botões. Não resolve combate. |
| `include/name_entry_screen.h`, `src/name_entry_screen.c` | Rascunho UTF-8 de até 12 codepoints, foco, edição e transição reversível de 0,6 s; reutiliza o cartão/retrato e os relógios da seleção. |
| `include/relay_ui.h`, `src/relay_ui.c` | Recursos industriais da central, pertencentes a `Game`: quatro texturas, recortes fixos e molduras em nove partes. Compõe progresso, objetivo, cards, interruptores e rodapé com Pixelify emprestada. Reutiliza limites de clique de `world` e oferece fallback geométrico; desenho somente de leitura. |
| `include/logic_setup_screen.h`, `src/logic_setup_screen.c` | Modal da placa: abas por planta, perguntas, editor de ataque/fórmula, listas e confirmação individual. Recebe equipe e recursos emprestados; preserva rascunhos e não calcula dano. `world` mantém a interação e os acertos; `game` coordena abertura e configurações confirmadas. |
| `include/selection_ui.h`, `src/selection_ui.c` | Interface compartilhada de personagem e nome: `SelectionUiAssets`, pertencente a `TrainerSelectAssets`, possui três molduras novas, botão/seta reutilizados e sete atlas Pixelify rasterizados na resolução física, atualizados por `game_update`. Desenha cabeçalho, textos, molduras com recorte comum e botões em nove partes, com reserva geométrica. Também preserva paleta, formas e descarte de entrada das transições. |
| `tools/check_name_entry.c`, `tools/check_name_entry_input.c`, `tools/preview_name_entry.py` | Eventos nativos, verificações de UTF-8/foco/sessão e capturas OpenGL da entrada de nome e seus extremos. |
| `include/player.h`, `src/player.c` | Estado do treinador, direção, posição dos pés, movimento cardinal, relógio de caminhada, recursos dos dois atlas e desenho somente de leitura. |
| `include/world.h`, `src/world.c` | Estado da estufa, interações por proximidade, relógio e entrada dos diálogos, avaliação das fórmulas, colisão da caixa dos pés (16 × 8 pixels) e das portas interiores, câmera de tela inteira, HUD contextual e profundidade do aviso da ponte. |
| `tools/prepare_courtyard_detail.py`, `include/courtyard_detail_data.h` | Unir os quatro quadrantes em 3010 × 1694, com iluminação local da base e costuras de menor diferença. Preserva a fonte e o primeiro resultado aprovado. `WorldAssets` carrega a textura completa opcional; `World.detail_enabled` permite comparação por F8. |
| `tools/check_game_canvas.c` | Verificar resolução física, HiDPI, enquadramento, mouse, recorte, câmera, quadrante, ausência de recurso e ponte em OpenGL. |
| `tools/check_exploration_hud.c` | Verificar tipografia, acentos, estados e alvo compartilhado da interação, centralização, seta, resoluções, fonte de reserva e desenho imutável. Capturas em `build/hud_preview/`. |
| `tools/check_selection_scene.c`, `tools/preview_selection_scene.py` | Verificação opcional com eventos raylib e OpenGL; montagem de prévias temporais. Diagnósticos ficam em `build/selection_preview/`. |
| `tools/check_menu_screen.c` | Verificação opcional de composição, grama, repouso do mascote, entradas e fluxo completo do menu. Gera capturas OpenGL em `build/menu_v2_preview/` e `build/menu_mascot_preview/`. |
| `tools/prepare_menu_trees.py`, `include/menu_tree_data.h` | Extração de quatro copas grandes e oito distantes; poses com ligações fixas, reaproveitamento da curva da grama e reconstrução somente dos contornos expostos. Diagnósticos em `build/menu_tree_preview/`. |
| `tools/prepare_menu_grass.py`, `include/menu_grass_data.h` | Extração de 17 touceiras, reconstrução localizada do fundo e atlas de vento. Máscaras, relatório, contatos e GIFs ficam em `build/menu_grass_preview/`. |
| `tools/prepare_menu_mascot.py`, `include/menu_mascot_data.h` | Extrai partes e gera oito quadros do mascote na resolução original, sem criar cores; metadados compartilhados com a execução. Máscaras e GIFs ficam em `build/menu_mascot_preview/`. |
| `include/trainer_background.h`, `src/trainer_background.c` | Composição do fundo animado: atlas de vento ancorado, nuvens com wrap e fallback estático completo. Centraliza o mesmo escurecimento para seleção de personagem, nome e prólogo; a prévia F7 preserva o cenário original. Recebe o relógio da tela, sem modificar estado durante o desenho. |
| `tools/prepare_trainer_selection_background.py`, `include/trainer_background_data.h` | Extração e reconstrução local da nova fonte; poses e fases próprias, sem alterar recursos do menu aprovado. |
| `tools/check_trainer_background.c`, `tools/preview_trainer_selection_background.py` | Capturas OpenGL do ciclo completo, verificação da prévia e montagem de GIF/contatos em `build/trainer_selection_preview/`. Consulte [os detalhes do fundo](TRAINER_SELECTION_BACKGROUND.md). |

`Game` mantém os estados de tela inicial, seleção, entrada de nome, prólogo, exploração
e batalha, além de `CampaignSession` com personagem, nome e equipe somente em
memória. ENTER na tela inicial abre diretamente a seleção. A confirmação do
personagem abre o nome; um nome válido inicia o prólogo. A abertura apresenta
o pedido de Lia, o aceite, a escolha de três entre seis espécies e a viagem.
A chegada abre o mapa existente da estufa como primeiro destino na vila.
`OpeningStep` e `selection_ui_progress` unificam o indicador em seis passos:
personagem, nome, carta, aceite, equipe e viagem. O desenho deriva da tela/etapa
atual, sem armazenar outro contador; retornar acompanha automaticamente o fluxo.

A home reutiliza os atlas Pixelify de `SelectionUiAssets` para o aviso inicial
e o título de reserva. O prólogo usa seus atlas próprios também nos textos
narrativos; `ui_text_wrap_reveal_pixels` conserva a quebra e a revelação,
alinhando cada linha à resolução física, sem alterar a fonte dos NPCs.
Os guardiões preservam a equipe e abrem diretamente a batalha. A placa próxima
ao depósito controla três desafios opcionais, um por planta. `World.logic_move_mask`
registra as perguntas resolvidas por posição na equipe. `LogicSetupScreen`
pertence a `Game` e mantém abas livres, foco, lista aberta e rascunhos individuais.
Somente a confirmação copia fórmula e ataque para `CampaignSession`; resolver
a pergunta apenas libera o editor. `boosted_move_indices = -1`
mantém golpes normais, sem bônus nem bloqueio por fórmula.
Na seleção inicial, Girassol e Babosa ficam fora da lista por serem guardiões.
As seis plantas jogáveis usam silhuetas provisórias quando não há sprite próprio.
`CampaignSession.has_team` indica a equipe inicial pronta; `has_formula` indica
a existência de pelo menos um golpe lógico confirmado. Os encontros seguintes reutilizam a
equipe restaurada e a fórmula. `NameEntryScreen` preserva o rascunho ao voltar,
enquanto os relógios da seleção continuam. `game.c` aguarda a soltura das teclas
e do mouse após transições, sem bloquear a janela. ESC na seleção ou exploração
retorna à tela inicial; ESC na tela inicial encerra. Consulte
[CAMPAIGN_NAME_ENTRY.md](CAMPAIGN_NAME_ENTRY.md).

Os atlas são carregados em `game_init` e liberados em `game_shutdown`.
`PlayerAssets` mantém somente `male_botanical.png` e `female_botanical.png`:
12 poses, pivô fixo nos pés, filtro pontual e compensação do zoom da câmera.
A escala visual é 0,23 no exterior e 0,36 nos interiores. A Casa de Manutenção
ocupa 68% da largura e altura do quadro, com margem ao redor.
Sem atlas válido, a exploração desenha um marcador geométrico.

`TrainerSelectAssets` mantém somente `male_portrait_idle.png` e
`female_portrait_idle.png`, compartilhados com a entrada de nome. Os quatro
quadros usam os limites corporais e tempos dos headers de metadados, com
relógios independentes. Não há retratos estáticos nem halos separados.
Um atlas ausente ou inválido gera aviso no cartão, sem impedir a navegação.

`MenuScreen` mantém fundo, logo, mascote estático, atlas opcional, recortes e estado de entrada. As fontes são inspecionadas pelo alfa durante a inicialização, sem modificar arquivos. O fundo cobre 1280 × 720 mantendo proporção; logo e mascote usam escala uniforme e filtro pontual. A composição é fixa, sem renderização global em baixa resolução. O atlas do mascote tem 4096 × 2048, oito células de 1024 × 1024 e reprodução a 6 FPS. Seu relógio usa delta time limitado a 0,1 s e avança antes da entrada, inclusive durante bloqueios; abrir/fechar o menu não reinicia a fase. Margens transparentes são compensadas no destino para preservar escala, sombra e pés. Atlas ausente/inválido usa o mascote estático; cada textura é carregada uma vez e liberada no encerramento. Não há painel de opções. ENTER abre a seleção de personagem e ESC solicita encerramento; `game.c` coordena as transições e o bloqueio até soltar entradas. O vento do menu usa 17 touceiras e 12 copas extraídas do fundo, em 32 fases a 8 FPS, com uma transformação e um relógio comuns. O par adicional de copas só é ativado com a grama válida; sua falha mantém árvores estáticas e grama animada. A restauração das copas parte do fundo preparado para a grama e protege suas áreas. O par só é ativado após validar ambas as texturas; qualquer falha preserva o fundo original completo. O relógio da grama também usa delta time e continua na tela inicial. A geração offline e seus limites estão em [MENU_GRASS_WIND.md](MENU_GRASS_WIND.md).

A seleção compõe molduras botânicas e textos dinâmicos sobre `TrainerBackgroundAssets`.
Os limites corporais dos atlas determinam escala e pés; os repousos usam atlas
próprios, durações explícitas e filtro pontual. A antiga floresta em camadas
permanece removida; as novas molduras de personagem e nome são descritas em
[TRAINER_SETUP_DESIGN.md](TRAINER_SETUP_DESIGN.md). A auditoria histórica está
em [SELECTION_CLEANUP.md](SELECTION_CLEANUP.md).

O movimento usa delta time limitado a 0,1 s por quadro. Teclas de eixos diferentes permitem andar na diagonal à mesma velocidade. Passos curtos e colisões por eixo permitem deslizar junto às bordas. A animação avança apenas após deslocamento efetivo. Parado, o treinador volta à pose neutra da direção mantida. Limitações dos ciclos e parâmetros de preparação estão em [TRAINER_TEST.md](TRAINER_TEST.md).

### Plantas e equipes

| Arquivo | Responsabilidade |
|---|---|
| `include/plant.h` | Definir as estruturas de plantas, golpes e equipes, além das operações sobre esses dados. |
| `src/plant.c` | Implementar a criação e a manipulação de plantas e equipes. |
| `include/plant_data.h` | Declarar os IDs das oito espécies, o tamanho da lista inicial e o acesso aos dados e golpes. |
| `src/plant_data.c` | Armazenar nomes, atributos e golpes provisórios de cada espécie e a lista explícita das seis iniciais. |

O catálogo contém os dados usados para criar as plantas. A criação recebe somente o identificador da espécie e copia seus atributos base, sem progressão individual. Durante uma partida, cada planta possui seu próprio estado, como a vida atual.

Por exemplo: dois cactos podem partir dos mesmos atributos do catálogo, mas perder vida de forma independente.

`PlantData_GetStarter(index)` consulta a lista inicial sem depender da ordem dos
IDs do catálogo e retorna `NULL` para índices inválidos. A introdução armazena
índices dessa lista e os converte em IDs ao montar a equipe. A montagem de reserva
na interface de combate consulta a mesma lista; o motor continua aceitando as
oito espécies, pois também controla as equipes dos bosses.

Para aprimorar o elenco, ajuste atributos e golpes em `plant_data.c`; o catálogo
usa inicializadores por ID. Os desenhos provisórios ficam em `intro_screen.c`
e `battle_screen.c`, sem mudar as regras do combate. Nesta etapa não há efeitos
de veneno, queimadura ou captura, nem balanceamento final.


### Regras de combate

| Arquivo | Responsabilidade |
|---|---|
| `include/battle.h` | Definir o estado da batalha, as ações disponíveis e as funções de combate. |
| `src/battle.c` | Validar ações, calcular dano, aplicar ataques, realizar trocas, controlar turnos e identificar o vencedor. |

Todas as regras devem ser verificadas pelo módulo de combate, mesmo quando a interface já impede uma escolha inválida.

Uma ação inválida não deve alterar a batalha nem consumir o turno.

A abertura oferece Trevo, Hera Venenosa, Pimenta, Cacto, Orquídea e Planta Carnívora, com três escolhas distintas obrigatórias. O catálogo definitivo contém oito espécies; Girassol e Babosa são os bosses e ficam fora da seleção inicial. Cada guardião luta sozinho; sua resistência é definida nos atributos de Babosa e Girassol em `plant_data.c`.
Cada planta pode associar duas condições e um operador a um golpe de dano.
Sem aquisição, o índice é -1. `battle.c` avalia a fórmula e valida bônus e
bloqueio apenas quando há um golpe lógico selecionado. `game.c` aplica as
recompensas opcionais da exploração e preserva a configuração entre encontros.
A CPU utiliza a mesma API de validação e dano.

### Adversário controlado pelo computador

| Arquivo | Responsabilidade |
|---|---|
| `include/ai.h` | Declarar as funções de escolha de ações do computador. |
| `src/ai.c` | Consultar a batalha e escolher uma ação válida para o adversário. |

A IA escolhe uma ação, mas não aplica dano nem altera equipes diretamente. Sua escolha deve passar pelas mesmas funções de combate utilizadas pelos jogadores.

### Interface de batalha

| Arquivo | Responsabilidade |
|---|---|
| `include/battle_screen.h` | Declarar os menus, o local do encontro e os recursos próprios da arena, além da entrada e desenho. |
| `src/battle_screen.c` | Arena botânica, menu Atacar/Trocar, três golpes, reservas, placeholders bloqueados, fórmula por planta, status, espera não bloqueante e resultado. Encaminha ações ao motor; desenho somente de leitura. Reutiliza as seis sprites iniciais e os atlas Pixelify já pertencentes a `Game`. |
| `src/battle_screen_assets.c` | Carregar e liberar `Game.battle_assets`: dois cenários, terra, três molduras e dois bosses definitivos. Confere dimensões, calcula limites de alfa uma vez e desenha molduras em nove partes com filtro pontual. Não altera o mundo nem recursos emprestados. |
| `tools/check_battle_screen.c` | Entradas raylib e capturas OpenGL dos dois locais, oito espécies, submenus, ações bloqueadas, quatro posições, substituição, CPU, resultado, resoluções e fallback; confere desenho imutável e liberação. |

A interface apresenta o estado da batalha e encaminha as escolhas do jogador ao módulo de combate.

Ela não deve calcular dano, decidir o vencedor ou alterar diretamente a vida das plantas.

### Configuração e documentação

| Arquivo | Responsabilidade |
|---|---|
| `CMakeLists.txt` | Configurar a compilação, os arquivos-fonte e a integração com raylib. |
| `.gitignore` | Impedir que arquivos gerados pela compilação e configurações locais sejam versionados. |
| `README.md` | Apresentar o jogo, seu escopo e as instruções para executar. |
| `docs/ARCHITECTURE.md` | Explicar a organização e as responsabilidades do projeto. |
| `docs/BACKLOG.md` | Registrar tarefas, prioridades e critérios de conclusão. |

## Fluxo de execução

1. `main.c` abre a janela e inicializa o jogo.
2. A cada quadro, chama as funções de atualização e desenho de `game.c`.
3. `game.c` direciona a execução para tela inicial, seleção de personagem, entrada de nome, prólogo, exploração ou batalha de campanha.
4. Durante uma batalha, a interface captura a ação do jogador ou a IA escolhe uma ação.
5. `battle.c` valida e resolve a ação, atualizando o estado da batalha.
6. A interface desenha o estado atualizado.
7. Ao fechar o jogo, os recursos carregados são liberados e a janela é encerrada.

O jogo continua atualizando e desenhando a janela enquanto aguarda uma escolha. Não devem ser usados loops de espera que congelem a interface.

## Separação entre regras e interface

Os módulos `plant`, `plant_data`, `battle` e `ai` não devem depender da raylib.

Esses módulos trabalham com dados e regras em C. Elementos gráficos, como cores, texturas, posições na tela e cliques, ficam na interface.

Exemplo de divisão:

- A interface identifica que o jogador escolheu o primeiro golpe.
- O combate verifica se a escolha é permitida e aplica o resultado.
- A interface mostra a nova vida e a mensagem da ação.

Desenhar a tela não deve modificar o estado da partida.

## Arquivos `.h` e `.c`

Os arquivos `.h` apresentam o que um módulo disponibiliza para os demais: estruturas, constantes e declarações de funções.

Os arquivos `.c` implementam essas funções e podem conter funções auxiliares privadas.

Convenções:

- Cada `.h` deve possuir proteção contra inclusão repetida.
- Cada `.c` deve incluir seu próprio `.h`, quando houver.
- Arquivos `.c` não devem ser incluídos com `#include`.
- Funções auxiliares usadas apenas dentro de um `.c` devem ser declaradas como `static`.
- O estado da partida deve ser passado às funções, evitando variáveis globais mutáveis.

## Onde editar para cada tarefa

| Tarefa | Arquivos principais |
|---|---|
| Adicionar uma espécie ou ajustar atributos e golpes existentes | `plant_data.h` e `plant_data.c`; `plant.h` caso haja identificadores de espécies. |
| Alterar as estruturas de plantas, golpes ou equipes | `plant.h` e `plant.c`. |
| Criar um novo efeito de golpe | `battle.h` e `battle.c`, além dos dados necessários em `plant.h` e `plant_data.c`. |
| Alterar dano, turnos, trocas ou condições de vitória | `battle.h` e `battle.c`. |
| Alterar as decisões do computador | `ai.c`. |
| Alterar o visual ou os controles da batalha | `battle_screen.h`, `battle_screen.c` e `assets/`. |
| Alterar a navegação entre telas e modos | `game.h` e `game.c`. |
| Adicionar arquivos-fonte à compilação | `CMakeLists.txt`. |

Ao alterar uma estrutura ou a assinatura de uma função, também é necessário revisar os arquivos que a utilizam.

## Escopo do primeiro protótipo

O combate da campanha inclui:

- Encontros contra guardiões controlados pela IA.
- Três plantas iniciais por equipe, com capacidade máxima de quatro.
- Três golpes por planta, incluindo dano e alterações de atributo.
- Troca voluntária consumindo o turno.
- Substituição obrigatória de plantas derrotadas.
- Exibição de vida, turno e mensagem da última ação.
- Identificação do vencedor e retorno à exploração.
- Arena botânica com sprites definitivas e reserva geométrica se faltarem imagens.

Após personagem, nome e prólogo, a campanha abre um mapa com câmera, fachadas e interiores conectados
por portas e escotilha. Guardiões e controles distribuídos liberam
a central de relés, com três etapas independentes e graduais: A, A E B e
(A OU B) E NÃO C. Os três relés ficam disponíveis desde o começo; erros não retrocedem o progresso.
As duas pistas da exploração são opcionais.
A progressão por arenas,
recrutamento e evolução permanece planejada.
Consulte [CAMPAIGN_GREENHOUSE.md](CAMPAIGN_GREENHOUSE.md).

## Convenções do projeto

- Código, identificadores e nomes de arquivos em inglês.
- Comentários, documentação e textos do jogo em português.
- Comentário curto no topo de cada arquivo explicando sua responsabilidade.
- Funções pequenas, com nomes que indiquem sua finalidade.
- Constantes para limites compartilhados, como o tamanho máximo da equipe.
- Commits em inglês, seguindo Conventional Commits.

A documentação deve ser atualizada quando houver mudanças nas responsabilidades dos módulos ou no fluxo do jogo.
