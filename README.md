# Plants x Plants 🌱⚔️

Jogo de batalhas por turnos entre plantas, inspirado no sistema de combate de Pokémon e desenvolvido como projeto acadêmico em **C com raylib**, para **Windows**.

Monte sua equipe, evolua suas plantas e escolha seu caminho por diferentes biomas até enfrentar o boss.

## Status

Teste jogável de movimento disponível pelo menu **Campanha**, com escolha de Explorador ou Exploradora. Combate e progressão de campanha continuam previstos; as seções abaixo descrevem o escopo planejado.

## Modos de jogo

### Campanha

O jogador começa com uma planta no estágio de mudinha e constrói sua equipe durante a jornada, recrutando plantas que enfrentou.

A campanha inclui um tutorial e três arenas, escolhidas entre rotas alternativas.

### Versus local

Dois jogadores montam suas equipes e se enfrentam no mesmo computador, alternando suas ações por turno. Não é necessário concluir a campanha para jogar esse modo.

## Sistema de combate

- Cada equipe pode ter até quatro plantas.
- Apenas uma planta de cada equipe fica ativa por vez.
- No turno, o jogador pode usar um golpe ou trocar a planta ativa.
- A troca voluntária consome o turno.
- Quando uma planta é derrotada, o jogador escolhe outra disponível.
- Vence quem derrotar todas as plantas da equipe adversária.

## Progressão da campanha

| Etapa | Progressão |
|---|---|
| Início | Escolha de uma planta inicial no estágio de mudinha. |
| Tutorial | Após a primeira luta, recrutamento de uma das plantas enfrentadas. Uma segunda luta apresenta a troca de plantas. |
| Primeira arena | Escolha de rota, recrutamento da terceira planta e evolução da equipe para o segundo estágio após a vitória. |
| Segunda arena | Nova escolha de rota, recrutamento da quarta planta e evolução da equipe para o estágio final após a vitória. |
| Arena final | Batalha contra o boss da rota escolhida, com a equipe completa. |

A vida da equipe é recuperada entre arenas.

## Arenas e rotas

Existem seis arenas, distribuídas em três etapas, com duas opções de bioma por etapa.

O jogador enfrenta apenas uma arena de cada etapa. Assim, cada campanha possui três arenas, além do tutorial.

Cada bioma terá adversários próprios e uma regra de combate simples. A escolha da rota também influencia quais plantas estarão disponíveis para recrutamento.

Os biomas e seus efeitos ainda serão definidos.

## Recrutamento e evolução

Após a primeira luta do tutorial e as vitórias nas duas primeiras arenas, o jogador escolhe uma das plantas enfrentadas para integrar sua equipe.

Cada espécie possui três estágios evolutivos:

1. Mudinha.
2. Planta jovem.
3. Forma final.

A evolução acontece após a primeira e a segunda arena, sem sistema de experiência ou níveis individuais. Plantas recrutadas entram no estágio atual da equipe.

As evoluções são fixas por espécie e não dependem do bioma escolhido.

## Tecnologias

- **Linguagem:** C.
- **Biblioteca gráfica:** raylib.
- **Plataforma inicial:** Windows.
- **Controle de versão:** Git e GitHub.

## Convenções

- Código e identificadores em inglês.
- Textos do jogo em português.
- Commits em inglês, seguindo Conventional Commits.

## Como executar no Windows

### Ferramentas necessárias

- Git.
- CMake 3.24 ou superior.
- GCC com suporte a C11, usando uma distribuição MinGW-w64, como w64devkit.
- Ninja.

O VS Code é opcional: os comandos abaixo podem ser executados pelo PowerShell.

As ferramentas devem estar disponíveis no PATH. Após instalar ou alterar o PATH, reabra o terminal e, caso esteja usando o VS Code, reinicie o editor.

Verifique a instalação:

```powershell
git --version
cmake --version
gcc --version
ninja --version
```

O ambiente inicial foi validado com:

| Ferramenta | Versão |
|---|---|
| Git | 2.55.0.windows.4 |
| CMake | 4.3.1 |
| GCC | 15.2.0 |
| Ninja | 1.13.2 |

Essas são as versões utilizadas na validação, não requisitos de versões exatas.

### Obter o projeto

```powershell
git clone https://github.com/AMSDAF/plants-x-plants.git
cd plants-x-plants
```

Caso já tenha clonado o repositório, entre na pasta existente.

### Configurar

Na raiz do projeto, execute:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc
```

O CMake baixa a raylib 5.5 automaticamente por meio do Git. Não é necessário instalar a biblioteca separadamente.

A primeira configuração exige conexão com a internet e pode demorar alguns minutos sem exibir novas mensagens.

Aguarde a conclusão:

```text
-- Configuring done
-- Generating done
-- Build files have been written to: ...
```

### Compilar

```powershell
cmake --build build
```

Execute novamente esse comando após alterar o código. O CMake recompila os arquivos necessários.

### Executar

A partir da raiz do projeto:

```powershell
.\build\plants_x_plants.exe
```

A versão atual abre uma tela inicial em pixel art, com logo fixo e mascote em repouso animado. Pressione **ENTER** para abrir o menu e escolha **Campanha** com as setas e ENTER ou com o mouse. **Versus local** permanece desabilitado. O painel creme, a borda verde e o cursor triangular são desenhados em código. Solte ENTER antes de confirmar uma opção; ESC retorna à tela inicial.

O menu usa `assets/sprites/logo_v2.png`, `menu_mascot.png` e `menu_background_v2.png`. A fonte atualmente fornecida tem a grafia `menu_backround_v2.png`, aceita como alternativa sem renomear o original. Logo e mascote são enquadrados pelos limites visíveis do alfa durante a carga; todas as texturas do menu usam filtro pontual. `menu_mascot_idle.png` já acompanha o projeto, com oito quadros a 6 FPS e fallback para a fonte estática. Apenas o mascote anima; o fundo permanece fixo. A seleção de treinadores mantém seus próprios recursos de vegetação e botões de madeira. Consulte [MENU_V2.md](docs/MENU_V2.md) para composição e validação.

Para refazer apenas a animação do mascote, usando o ambiente de preparação descrito abaixo:

```powershell
.\build\trainer_tools\Scripts\python tools/prepare_menu_mascot.py
cmake --build build --target menu_screen_check
.\build\menu_screen_check.exe
.\build\trainer_tools\Scripts\python tools/prepare_menu_mascot.py --runtime-preview
```

O original permanece intacto. Máscaras, contato e GIFs ficam em `build/menu_mascot_preview/`; somente o atlas é gravado em `assets/`, acompanhado dos metadados em `include/menu_mascot_data.h`. Python não é necessário para jogar. Consulte [MENU_MASCOT_IDLE.md](docs/MENU_MASCOT_IDLE.md) para limites e validação.

Na seleção **Escolha seu explorador**, use esquerda/direita ou A/D, ou clique no retrato de **Explorador** ou **Exploradora**. O clique apenas seleciona: pressione **ENTER** ou clique em **Confirmar** para entrar. **Voltar** ou **ESC** retorna ao menu. Os retratos completos têm respiração discreta no tórax e destaque suave de seleção. A exploração usa chão neutro, grade e limites provisórios, sem mapa final ou batalhas.

- **WASD ou setas:** mover em quatro direções, sem diagonais. Com várias teclas mantidas, a prioridade é norte, sul, oeste e leste.
- **ESC na seleção ou exploração:** voltar ao menu, sem fechar o jogo.
- **ESC no menu:** voltar à tela inicial.
- **Sair** no menu ou **X** da janela: encerrar.

Ao mudar de tela, solte as teclas e o botão esquerdo do mouse antes de continuar. Isso evita confirmações e movimento herdados da tela anterior.

Os atlas preparados em `assets/sprites/trainers/` já estão prontos para execução; Python não é necessário para jogar. Execute a partir da raiz para localizar os recursos. Um atlas ausente ou inválido gera diagnóstico no console e um marcador jogável.

### Preparar novamente os sprites (opcional)

As fontes `trainer_male_sheet.png` e `trainer_female_sheet.png` são preservadas. Para refazer os atlas e as prévias:

```powershell
python -m venv build/trainer_tools
.\build\trainer_tools\Scripts\python -m pip install -r tools/trainer_requirements.txt
.\build\trainer_tools\Scripts\python tools/prepare_trainers.py
```

Dependências de preparação: Pillow, NumPy e SciPy. As folhas de contato, oito GIFs direcionais e relatórios de inspeção ficam em `build/trainer_preview/`, ignorado pelo Git. Os dois PNGs preparados são os recursos aprovados. Consulte [o relatório dos sprites](docs/TRAINER_TEST.md) para as sequências e limitações da arte.

Para preparar o experimento masculino de seis poses, sem sobrescrever os atlas aprovados:

```powershell
.\build\trainer_tools\Scripts\python tools/prepare_trainer_walk6.py
```

O script usa as mesmas dependências e grava `male_walk6.png` separadamente, com máscaras e GIFs A/B em `build/trainer_preview/walk6/`. Na exploração com o **Explorador**, **F6** alterna a caminhada aprovada e a experimental. A versão aprovada continua sendo o padrão; repouso e Exploradora permanecem iguais. Sem o atlas experimental, o jogo usa o aprovado. A duração do ciclo masculino é configurável por `PLAYER_MALE_WALK_CYCLE_SECONDS` em `include/player.h` (padrão: 0,56 s para ambas as versões, independentemente da quantidade de poses).

### Preparar novamente os retratos de seleção (opcional)

Os retratos são arte exclusiva da seleção, separados dos sprites de caminhada. Com o mesmo ambiente Python descrito acima:

```powershell
.\build\trainer_tools\Scripts\python tools/prepare_trainer_portraits.py
```

O script preserva `trainer_male_portrait.png` e `trainer_female_portrait.png`, prepara derivados estáticos de 320 × 480 e atlas de respiração em `assets/sprites/trainers/`. Diagnósticos de alfa, hashes, máscaras de movimento e GIFs ficam em `build/portrait_preview/`. Os derivados já estão prontos para execução; Python não é necessário para jogar. Sem um atlas de respiração válido, a seleção usa o retrato estático. Detalhes e validação estão em [TRAINER_SELECTION.md](docs/TRAINER_SELECTION.md).

### Preparar novamente o cenário de seleção (opcional)

O cenário da seleção usa fundo, árvores fixas, nuvens, dosséis, vegetação baixa, folhas e molduras de madeira. Seus recursos são independentes da tela inicial, do menu e da exploração. Para reconstruir os derivados com o mesmo ambiente de preparação:

```powershell
.\build\trainer_tools\Scripts\python tools/prepare_selection_scene.py
```

O script procura as sete fontes em `assets/sprites/selection/` e, se ausentes nessa pasta, em `assets/sprites/`, onde estão os arquivos fornecidos atualmente. Preserva as fontes e grava os derivados em `assets/sprites/selection/prepared/`, além de gerar os retângulos medidos em `include/selection_scene_data.h`. Diagnósticos e comparações em fundos claro/escuro ficam em `build/selection_preview/`. Os recursos preparados já acompanham o projeto; Python é opcional para jogar.

Para executar a verificação gráfica automatizada, a partir da raiz:

```powershell
cmake --build build --target selection_scene_check
.\build\selection_scene_check.exe
.\build\trainer_tools\Scripts\python tools/preview_selection_scene.py
```

O alvo opcional usa eventos de entrada da raylib, testa navegação, transições, fallback e animação, e grava capturas OpenGL. Não substitui teste manual. Consulte [o relatório da seleção](docs/TRAINER_SELECTION.md).

### Arquivos gerados

A pasta `build/` contém arquivos temporários, dependências baixadas e o executável. Ela é ignorada pelo Git e não deve ser adicionada aos commits.

Os comandos de configuração não devem conter caminhos absolutos específicos do computador de um integrante.

### Problemas comuns

**Comando não reconhecido**

Confirme a instalação da ferramenta e sua presença no PATH. Depois, reabra o terminal ou reinicie o VS Code.

Para localizar o GCC utilizado:

```powershell
(Get-Command gcc).Source
```

**Configuração aparentemente parada**

Na primeira execução, o CMake pode estar baixando a raylib sem mostrar progresso. Aguarde o término ou uma mensagem de erro.

**Conflito de gerador ou troca de compilador**

Use outra pasta de build para criar uma configuração independente:

```powershell
cmake -S . -B build-clean -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc
cmake --build build-clean
.\build-clean\plants_x_plants.exe
```

**Aviso de compatibilidade do CMake na raylib**

O ambiente validado exibiu um aviso de depreciação vindo do CMakeLists.txt da raylib. Esse aviso não impediu a configuração nem a compilação.

**Avisos sobre arquivos-fonte vazios**

Nesta etapa, alguns módulos contêm apenas comentários e podem gerar avisos com `-Wpedantic`. Eles serão implementados nas próximas tarefas.
