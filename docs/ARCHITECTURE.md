# Arquitetura — Plants x Plants

Este documento explica a organização do projeto e a responsabilidade de cada arquivo.

O jogo será desenvolvido em C com raylib. A estrutura inicial prioriza simplicidade, divisão de tarefas e separação entre as regras do jogo e a interface gráfica.

> A tela inicial e o menu em pixel art têm vento localizado na grama/copas e repouso animado do mascote. O teste provisório de exploração está implementado e a seleção de treinadores conserva suas animações. Os módulos de combate abaixo ainda descrevem a arquitetura planejada.

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
| `include/game.h` | Definir o estado geral do jogo e declarar suas funções de controle. |
| `src/game.c` | Controlar as telas, os modos de jogo, o início das partidas e o reinício. |

O `main.c` deve permanecer pequeno. Regras de combate, dados de plantas e detalhes das telas devem ficar nos respectivos módulos.

### Menu e teste de exploração

| Arquivos | Responsabilidade |
|---|---|
| `include/menu_screen.h`, `src/menu_screen.c` | Tela inicial/menu em pixel art e relógios contínuos de grama e mascote. Carrega texturas com filtro pontual, calcula limites visíveis uma vez e valida atlas opcional. Desenha painel/cursor em código e devolve ações de Campanha/Sair. Versus local permanece desabilitado. Protege as trocas título/menu até soltar entradas. |
| `include/trainer_select_screen.h`, `src/trainer_select_screen.c` | Cartões e controles geométricos em pixel art sobre `TrainerBackgroundAssets`, retratos v2, repousos com durações explícitas e relógios independentes e escolha por teclado/mouse. Usa limites corporais gerados para igualar altura/pés sem medir o halo. Retorna confirmar/voltar; clicar no cartão apenas seleciona. F7 exibe somente o fundo, sem reiniciar seu relógio. |
| `include/name_entry_screen.h`, `src/name_entry_screen.c` | Rascunho UTF-8 de até 12 codepoints, foco, edição e transição reversível de 0,6 s; reutiliza o cartão/retrato e os relógios da seleção. |
| `include/selection_ui.h`, `src/selection_ui.c` | Paleta e geometria em degraus compartilhadas entre seleção e nome, centralização de texto e descarte de entrada. |
| `tools/check_name_entry.c`, `tools/check_name_entry_input.c`, `tools/preview_name_entry.py` | Eventos nativos, verificações de UTF-8/foco/sessão e capturas OpenGL da entrada de nome e seus extremos. |
| `include/player.h`, `src/player.c` | Estado do treinador, direção, posição dos pés, movimento cardinal, relógio de caminhada, recursos dos dois atlas e desenho somente de leitura. |
| `include/world.h`, `src/world.c` | Área provisória, grade, limites e restrição da caixa dos pés (16 × 8 pixels). |
| `tools/prepare_trainers.py` | Limpeza e normalização dos sprites, sem alterar as fontes; gera atlas de execução e prévias de inspeção. |
| `tools/prepare_trainer_walk6.py` | Experimento isolado de extração e normalização de seis poses masculinas; gera outro atlas, máscaras e prévias A/B. |
| `tools/prepare_trainer_portraits.py` | Regenera os retratos detalhados de fallback estático; não gera mais a respiração procedural antiga. Não altera fontes nem atlas de caminhada. |
| `tools/check_selection_scene.c`, `tools/preview_selection_scene.py` | Verificação opcional com eventos raylib e OpenGL; montagem de prévias temporais. Diagnósticos ficam em `build/selection_preview/`. |
| `tools/check_menu_screen.c` | Verificação opcional de composição, grama, repouso do mascote, entradas e fluxo completo do menu. Gera capturas OpenGL em `build/menu_v2_preview/` e `build/menu_mascot_preview/`. |
| `tools/prepare_menu_trees.py`, `include/menu_tree_data.h` | Extração de quatro copas grandes e oito distantes; poses com ligações fixas, reaproveitamento da curva da grama e reconstrução somente dos contornos expostos. Diagnósticos em `build/menu_tree_preview/`. |
| `tools/prepare_menu_grass.py`, `include/menu_grass_data.h` | Extração de 17 touceiras, reconstrução localizada do fundo e atlas de vento. Máscaras, relatório, contatos e GIFs ficam em `build/menu_grass_preview/`. |
| `tools/prepare_menu_mascot.py`, `include/menu_mascot_data.h` | Extrai partes e gera oito quadros do mascote na resolução original, sem criar cores; metadados compartilhados com a execução. Máscaras e GIFs ficam em `build/menu_mascot_preview/`. |
| `include/trainer_background.h`, `src/trainer_background.c` | Composição do fundo da seleção normal e da prévia F7: atlas de vento ancorado, nuvens com wrap e fallback estático completo. Usa o relógio da seleção, sem modificar estado durante o desenho. |
| `tools/prepare_trainer_selection_background.py`, `include/trainer_background_data.h` | Extração e reconstrução local da nova fonte; poses e fases próprias, sem alterar recursos do menu aprovado. |
| `tools/check_trainer_background.c`, `tools/preview_trainer_selection_background.py` | Capturas OpenGL do ciclo completo, verificação da prévia e montagem de GIF/contatos em `build/trainer_selection_preview/`. Consulte [os detalhes do fundo](TRAINER_SELECTION_BACKGROUND.md). |

`Game` mantém os estados de menu, seleção, entrada de nome e exploração, além de `CampaignSession` com personagem e nome confirmado somente em memória. A confirmação da seleção abre a entrada de nome; um nome válido inicia a exploração. `NameEntryScreen` mantém o rascunho ao voltar, enquanto `TrainerSelectScreen` continua avançando fundo e repousos. Consulte [CAMPAIGN_NAME_ENTRY.md](CAMPAIGN_NAME_ENTRY.md). `game.c` faz todas as transições e cancela ativações pendentes. Após cada transição, aguarda a soltura das teclas de navegação/confirmação e do mouse, sem bloquear o loop da janela. ESC na exploração ou seleção retorna ao menu aberto.

Os atlas de caminhada são carregados uma vez em `game_init` e liberados em `game_shutdown`, usados somente pelo jogador na exploração. Recursos ausentes ou com dimensões incorretas recebem diagnóstico e marcador substituto. A posição representa os pés: o desenho usa o mesmo pivô em todas as poses, escala uniforme 0,30 e filtro pontual. A colisão ignora chapéu e mochila.

`TrainerSelectAssets` carrega retratos v2 e os atlas opcionais de repouso com halos estáticos separados, todos com filtro pontual e enquadramento corporal gerado por `tools/prepare_selection_portraits_v2.py`. A textura completa preserva o halo e as margens, enquanto somente o corpo determina escala e alinhamento dos pés. Se um derivado v2 faltar, usa o retrato detalhado anterior, também estático. Nenhum atlas ou máscara de respiração anterior é aplicado. `prepare_male_portrait_idle.py` gera as quatro poses alinhadas e seus tempos; o relógio próprio continua ao trocar cartões. Falhas no atlas/halo usam o respectivo retrato estático. A mulher começa com fase de 2,20 s, sem mudar os tempos masculinos. A confirmação e o bloqueio até soltar entradas permanecem coordenados por `game.c`.

`MenuScreen` mantém fundo, logo, mascote estático, atlas opcional, recortes e estado de entrada. As fontes são inspecionadas pelo alfa durante a inicialização, sem modificar arquivos. O fundo cobre 1280 × 720 mantendo proporção; logo e mascote usam escala uniforme e filtro pontual. A composição é fixa, sem renderização global em baixa resolução. O atlas do mascote tem 4096 × 2048, oito células de 1024 × 1024 e reprodução a 6 FPS. Seu relógio usa delta time limitado a 0,1 s e avança antes da entrada, inclusive durante bloqueios; abrir/fechar o menu não reinicia a fase. Margens transparentes são compensadas no destino para preservar escala, sombra e pés. Atlas ausente/inválido usa o mascote estático; cada textura é carregada uma vez e liberada no encerramento. As áreas das opções são constantes. Um bloqueio local protege ENTER/ESC entre título e menu, enquanto `game.c` continua coordenando as transições para outras telas. A confirmação por teclado mantém feedback de cor de 0,12 s sem bloquear a janela. O vento do menu usa 17 touceiras e 12 copas extraídas do fundo, em 32 fases a 8 FPS, com uma transformação e um relógio comuns. O par adicional de copas só é ativado com a grama válida; sua falha mantém árvores estáticas e grama animada. A restauração das copas parte do fundo preparado para a grama e protege suas áreas. O par só é ativado após validar ambas as texturas; qualquer falha preserva o fundo original completo. O relógio da grama também usa delta time e continua entre título/menu. A geração offline e seus limites estão em [MENU_GRASS_WIND.md](MENU_GRASS_WIND.md).

A seleção desenha cartões e controles em código sobre `TrainerBackgroundAssets`.
Os limites corporais sem halo determinam escala e pés; os repousos usam atlas
próprios, durações explícitas e filtro pontual. Não carrega nem atualiza a antiga
floresta em camadas, moldura ou botões de madeira. A auditoria está em
[SELECTION_CLEANUP.md](SELECTION_CLEANUP.md).

`PlayerAssets` também pode carregar `male_walk6.png` (1920 × 1280). A tecla F6 altera somente a opção local `Player.use_walk6` do Explorador durante a exploração. `player_draw` usa o atlas experimental apenas em movimento, com fallback para o aprovado quando ausente ou inválido. Repouso e personagem feminino na exploração continuam usando os recursos anteriores. O ciclo masculino usa uma duração configurável, independente das quatro ou seis poses; trocar a opção preserva a fase, a velocidade e a posição. A versão aprovada é o padrão em cada entrada na exploração.

O movimento usa delta time, limitado a 0,1 s por quadro para evitar saltos após pausas. A prioridade fixa norte/sul/oeste/leste resolve teclas simultâneas. A animação avança apenas após deslocamento efetivo; contra um limite ou parado, volta à pose neutra da direção mantida. Limitações dos ciclos e parâmetros de preparação estão em [TRAINER_TEST.md](TRAINER_TEST.md).

### Plantas e equipes

| Arquivo | Responsabilidade |
|---|---|
| `include/plant.h` | Definir as estruturas de plantas, golpes e equipes, além das operações sobre esses dados. |
| `src/plant.c` | Implementar a criação e a manipulação de plantas e equipes. |
| `include/plant_data.h` | Declarar o acesso ao catálogo de espécies, evoluções e golpes. |
| `src/plant_data.c` | Armazenar nomes, atributos e golpes de cada espécie e estágio evolutivo. |

O catálogo contém os dados usados para criar as plantas. Durante uma partida, cada planta possui seu próprio estado, como a vida atual.

Por exemplo: dois cactos podem partir dos mesmos atributos do catálogo, mas perder vida de forma independente.

### Regras de combate

| Arquivo | Responsabilidade |
|---|---|
| `include/battle.h` | Definir o estado da batalha, as ações disponíveis e as funções de combate. |
| `src/battle.c` | Validar ações, calcular dano, aplicar ataques, realizar trocas, controlar turnos e identificar o vencedor. |

Todas as regras devem ser verificadas pelo módulo de combate, mesmo quando a interface já impede uma escolha inválida.

Uma ação inválida não deve alterar a batalha nem consumir o turno.

### Adversário controlado pelo computador

| Arquivo | Responsabilidade |
|---|---|
| `include/ai.h` | Declarar as funções de escolha de ações do computador. |
| `src/ai.c` | Consultar a batalha e escolher uma ação válida para o adversário. |

A IA escolhe uma ação, mas não aplica dano nem altera equipes diretamente. Sua escolha deve passar pelas mesmas funções de combate utilizadas pelos jogadores.

### Interface de batalha

| Arquivo | Responsabilidade |
|---|---|
| `include/battle_screen.h` | Declarar as funções de entrada e desenho da tela de batalha. |
| `src/battle_screen.c` | Capturar comandos e desenhar plantas, vida, golpes, turnos e mensagens usando raylib. |

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
3. `game.c` direciona a execução para menu, seleção ou exploração. Campanha abre o teste de movimento; combate ainda não é acessível pelo menu.
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

A primeira entrega terá uma batalha completa, com:

- Modo contra o computador e versus local.
- Duas plantas por equipe, com capacidade máxima prevista de quatro.
- Dois golpes de dano por planta.
- Troca voluntária consumindo o turno.
- Substituição obrigatória de plantas derrotadas.
- Exibição de vida, turno e mensagem da última ação.
- Identificação do vencedor e opção de reiniciar.
- Visual provisório, sem depender de sprites finalizados.

O botão Campanha atualmente abre somente o teste provisório de movimento. Campanha completa, arenas, recrutamento e evolução continuam fora deste teste e não foram implementados.

## Convenções do projeto

- Código, identificadores e nomes de arquivos em inglês.
- Comentários, documentação e textos do jogo em português.
- Comentário curto no topo de cada arquivo explicando sua responsabilidade.
- Funções pequenas, com nomes que indiquem sua finalidade.
- Constantes para limites compartilhados, como o tamanho máximo da equipe.
- Commits em inglês, seguindo Conventional Commits.

A documentação deve ser atualizada quando houver mudanças nas responsabilidades dos módulos ou no fluxo do jogo.
