# Arquitetura — Plantas vs Plantas

Este documento explica a organização do projeto e a responsabilidade de cada arquivo.

O jogo será desenvolvido em C com raylib. A estrutura inicial prioriza simplicidade, divisão de tarefas e separação entre as regras do jogo e a interface gráfica.

> No momento, os arquivos de código possuem apenas comentários de responsabilidade. Este documento descreve a arquitetura planejada.

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
3. `game.c` direciona a execução para a tela atual.
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

Campanha, arenas, recrutamento e evolução serão implementados após a base de combate estar funcionando. Seus módulos serão definidos quando essas funcionalidades entrarem em desenvolvimento.

## Convenções do projeto

- Código, identificadores e nomes de arquivos em inglês.
- Comentários, documentação e textos do jogo em português.
- Comentário curto no topo de cada arquivo explicando sua responsabilidade.
- Funções pequenas, com nomes que indiquem sua finalidade.
- Constantes para limites compartilhados, como o tamanho máximo da equipe.
- Commits em inglês, seguindo Conventional Commits.

A documentação deve ser atualizada quando houver mudanças nas responsabilidades dos módulos ou no fluxo do jogo.