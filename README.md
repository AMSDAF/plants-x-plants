# Plants x Plants 🌱⚔️

Jogo de batalhas por turnos entre plantas, inspirado no sistema de combate de Pokémon e desenvolvido como projeto acadêmico em **C com raylib**, para **Windows**.

Monte sua equipe, evolua suas plantas e escolha seu caminho por diferentes biomas até enfrentar o boss.

## Status

O menu **Versus local** abre batalhas contra o computador ou outro jogador, com 12 espécies disponíveis para montar a equipe. O menu **Campanha** abre a Estufa Lógica, uma missão de exploração com escolha de Explorador ou Exploradora. A progressão posterior de campanha é planejada.

## Modos de jogo

### Campanha

O jogador começa com uma planta no estágio de mudinha e constrói sua equipe durante a jornada, recrutando plantas que enfrentou.

A campanha inclui um tutorial e três arenas, escolhidas entre rotas alternativas.

### Versus local

Jogue no mesmo computador em **VS PLAYER** ou enfrente o computador em **VS CPU**. Cada grupo tem uma planta ativa e até três reservas.

## Sistema de combate

- Cada equipe pode ter até quatro plantas.
- Apenas uma planta de cada equipe fica ativa por vez.
- No turno, o jogador pode usar um golpe ou trocar a planta ativa.
- A troca voluntária consome o turno.
- Quando uma planta é derrotada e há reserva viva, o lado derrotado escolhe a substituta e então joga seu turno.
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

Pressione **F11** em qualquer tela para alternar entre janela e tela cheia.
O jogo preserva a proporção 16:9 e ajusta os cliques à área exibida.

A versão atual abre uma tela inicial em pixel art, com logo flutuando suavemente, aviso intermitente e mascote em repouso animado. Pressione **ENTER** para abrir o menu e escolha **Campanha** com as setas e ENTER ou com o mouse. **Versus local** abre a escolha entre **VS CPU** e **VS PLAYER**. O painel creme, a borda verde e o cursor triangular são desenhados em código. Solte ENTER antes de confirmar uma opção; ESC retorna à tela inicial.

O menu usa `assets/sprites/logo_v2.png`, `menu_mascot.png` e `menu_background_v2.png`. A fonte atualmente fornecida tem a grafia `menu_backround_v2.png`, aceita como alternativa sem renomear o original. Logo e mascote são enquadrados pelos limites visíveis do alfa durante a carga; todas as texturas do menu usam filtro pontual. `menu_mascot_idle.png` já acompanha o projeto, com oito quadros a 6 FPS e fallback para a fonte estática. Dezessete touceiras e doze copas extraídas do fundo recebem vento localizado; troncos, raízes e pedras permanecem fixos. A seta de seleção pulsa discretamente sem alterar as áreas de clique. Sem o par válido de fundo reconstruído e atlas de grama, o jogo usa o fundo original completo. A seleção mantém fundo e retratos próprios; seus cartões e controles são desenhados em código. Consulte [MENU_V2.md](docs/MENU_V2.md) para composição e validação.

Para refazer apenas a animação do mascote, usando o ambiente de preparação descrito abaixo:

```powershell
.\build\trainer_tools\Scripts\python tools/prepare_menu_mascot.py
cmake --build build --target menu_screen_check
.\build\menu_screen_check.exe
.\build\trainer_tools\Scripts\python tools/prepare_menu_mascot.py --runtime-preview
```

O original permanece intacto. Máscaras, contato e GIFs ficam em `build/menu_mascot_preview/`; somente o atlas é gravado em `assets/`, acompanhado dos metadados em `include/menu_mascot_data.h`. Python não é necessário para jogar. Consulte [MENU_MASCOT_IDLE.md](docs/MENU_MASCOT_IDLE.md) para limites e validação.

Para refazer o vento da vegetação com o mesmo ambiente Python (copas após a grama):

```powershell
.\build\trainer_tools\Scripts\python tools/prepare_menu_grass.py
.\build\trainer_tools\Scripts\python tools/prepare_menu_trees.py
cmake --build build --target menu_screen_check
.\build\menu_screen_check.exe
.\build\trainer_tools\Scripts\python tools/prepare_menu_grass.py --runtime-preview
.\build\trainer_tools\Scripts\python tools/prepare_menu_trees.py --runtime-preview
```

As máscaras, o GIF de cena completa e os contatos ficam em `build/menu_grass_preview/` e `build/menu_tree_preview/`. Somente o fundo reconstruído, o atlas e os metadados são recursos de execução. Consulte [MENU_GRASS_WIND.md](docs/MENU_GRASS_WIND.md) para regiões, reconstrução, parâmetros e limites.

Na seleção **Escolha seu personagem**, use esquerda/direita ou A/D, ou clique no cartão de **Homem** ou **Mulher**. Os cartões creme, bordas verdes em degraus, cantos laranja e cursor pulsante são desenhados em código sobre o fundo animado. O clique apenas seleciona: pressione **ENTER** ou clique em **Confirmar** para abrir **Como você se chama?**. Digite até 12 caracteres, incluindo acentos portugueses; **TAB/SHIFT+TAB** alterna entre campo e botões. Confirme um nome válido para entrar na Estufa Lógica. **Voltar/ESC** nessa etapa retorna à seleção, preservando o rascunho. Na seleção, **Voltar/ESC** retorna ao menu. Os retratos botânicos preservam a proporção e o alinhamento dos pés.

- **WASD ou setas:** mover em quatro direções, sem diagonais. Com várias teclas mantidas, a prioridade é norte, sul, oeste e leste.
- **Shift na estufa:** correr enquanto se move.
- **E na estufa:** interagir com a válvula, o refletor, a chave, os canais ou a central ao se aproximar.
- **ESC na seleção ou exploração:** voltar ao menu, sem fechar o jogo.
- **ESC no menu:** voltar à tela inicial.
- **Sair** no menu ou **X** da janela: encerrar.

Os exploradores botânicos usam retratos estáticos na seleção e 12 quadros
de caminhada na exploração, com três poses por direção. Os PNGs preparados
estão em `assets/sprites/trainers/`. Consulte a preparação dos sprites em
[TRAINER_TEST.md](docs/TRAINER_TEST.md).

Ao mudar de tela, solte as teclas e o botão esquerdo do mouse antes de continuar. Isso evita confirmações e movimento herdados da tela anterior.

Na estufa, entre nos prédios com **E**. Derrote a Babosa no depósito para
obter a chave da escotilha na casa de manutenção. No subsolo, alimente o
refletor e volte pela escotilha. Na Casa de Controle da Ponte, abra exatamente
um canal e ajuste o dreno externo para atingir pressão 3.
A porta do viveiro exige `Água E Luz E NÃO Alarme`.
Siga os caminhos claros e as placas. O painel indica o setor atual e um
marcador destaca os objetos ao alcance de **E**.
Vença o Girassol no viveiro e registre as duas pistas espalhadas pelo mapa.
Na central, use **1–8** ou clique nos relés e pressione **Enter** para validar
uma regra por vez. Se uma regra anterior falhar, a central volta para ela.
Consulte o [relatório da Estufa Lógica](docs/CAMPAIGN_GREENHOUSE.md).

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

O script usa as mesmas dependências e grava `male_walk6.png` separadamente, com máscaras e GIFs A/B em `build/trainer_preview/walk6/`. Ao usar os sprites legados do **Explorador**, **F6** alterna a caminhada aprovada e a experimental. A versão aprovada continua sendo o padrão; repouso e Exploradora permanecem iguais. Sem o atlas experimental, o jogo usa o aprovado. A duração do ciclo masculino é configurável por `PLAYER_MALE_WALK_CYCLE_SECONDS` em `include/player.h` (padrão: 0,56 s para ambas as versões, independentemente da quantidade de poses).

### Preparar novamente os retratos de seleção (opcional)

Os retratos são arte exclusiva da seleção, separados dos sprites de caminhada. Com o mesmo ambiente Python descrito acima:

```powershell
.\build\trainer_tools\Scripts\python tools/prepare_selection_portraits_v2.py
```

O script preserva `trainer_selection_portraits_v2.png` e divide suas metades em `male_portrait_v2.png` e `female_portrait_v2.png`, com margem transparente, em `assets/sprites/trainers/`. Cores, alfa e halo são copiados sem alteração. Diagnósticos de alfa, enquadramento e composição ficam em `build/selection_portrait_v2_preview/`. Os repousos são preparados por `tools/prepare_male_portrait_idle.py` (padrão masculino; `--character female` para a mulher); ambos preservam fallback estático. Recursos anteriores são preservados. Python não é necessário para jogar. Detalhes e validação estão em [TRAINER_SELECTION.md](docs/TRAINER_SELECTION.md).

### Preparar novamente o cenário de seleção (opcional)

O novo fundo animado já acompanha os cartões em **Campanha**. Pressione **F7**
para ver somente o cenário animado. **ESC** retorna ao menu; F7 recupera a seleção
normal. Preparação, GIF completo e verificações estão em
[TRAINER_SELECTION_BACKGROUND.md](docs/TRAINER_SELECTION_BACKGROUND.md).

O cenário usa somente `trainer_selection_background.png` e os derivados em
`assets/sprites/selection/background/`. Para refazer os recursos:

```powershell
.\build\trainer_tools\Scripts\python tools/prepare_trainer_selection_background.py
```

Fontes e fallback completo são preservados; Python é opcional para jogar.

Para executar a verificação gráfica automatizada, a partir da raiz:

```powershell
cmake --build build --target selection_scene_check
.\build\selection_scene_check.exe
.\build\trainer_tools\Scripts\python tools/preview_selection_scene.py
```

O alvo opcional usa eventos de entrada da raylib, testa navegação, transições, fallback e animação, e grava capturas OpenGL. Não substitui teste manual. Consulte [o relatório da seleção](docs/TRAINER_SELECTION.md).

Na tela Versus, esquerda/direita, A/D ou um clique no cartão selecionam o modo.
**ENTER** ou **Confirmar** abre a montagem da equipe. **Voltar/ESC** retorna ao menu.
Ao reabrir a seleção, o último modo confirmado aparece selecionado.
Monte um grupo de duas a quatro plantas. Clique em uma vaga ou use **Q/W/E/R**
para selecioná-la. Depois, clique em uma espécie ou use **setas** e **Espaço**.
Use **Backspace** ou o botão de remoção para tirar uma reserva e **Enter** para confirmar.
Antes da luta, monte também o **Golpe Lógico**. Use **Q/W** para escolher as condições,
**E** para mudar o operador e **ENTER** para confirmar. Os botões aceitam cliques.
Na arena, use **1/2** para os golpes da espécie, **3** para o Golpe Lógico e
**Q/W/E/R** para trocar de planta. O golpe personalizado é liberado quando a fórmula é verdadeira.
No resultado, **ENTER** reinicia a partida e **M/ESC** retorna ao menu.
Consulte o [relatório do combate](docs/BATTLE_PROTOTYPE.md) para regras e validação.

O cenário gerado aprovado da seleção Versus é
`assets/sprites/versus_background_generated_v1.png`; `versus_background_v1.png`
é usado como alternativa quando disponível. As ilustrações usam recortes da referência
`versus_illustrations_source.png`, sem incorporar seus controles impressos.
Painéis, cantos e cursor reutilizam `selection_ui`, com o mesmo pulso da seleção.

A grama do cenário Versus mantém suas 20 touceiras com bases fixas,
fases próprias e ritmos de 6, 7 ou 8 poses por segundo. As árvores e os arbustos
recebem vento suave em 13 camadas de folhagem, com fases e ritmos independentes.
Troncos, raízes e bases dos arbustos permanecem fixos; os pulsos existentes da UI
são preservados. Os derivados `assets/sprites/versus/grass_base.png` e `grass_atlas.png`
formam um par validado: se faltar ou falhar um deles, o cenário original completo
é exibido. O par adicional `foliage_base.png`/`foliage_atlas.png` só é ativado
com a grama válida. Se falhar, árvores e arbustos ficam estáticos e a grama segue
animando normalmente. A fonte aprovada e os recursos da grama não são alterados.
Python continua opcional para jogar.

Os quatro personagens dos cartões usam repousos próprios, com relógios e fases
independentes. As plantas verdes respiram discretamente, movem as folhas e erguem
os punhos ocasionalmente; a vermelha tem gestos mais frequentes e vigorosos.
O robô faz ajustes de um pixel no monitor e pisca. Todos usam pixels da arte
original, sem escala variável ou filtro suave; pés, bases e sombras ficam fixos.
O par `character_base.png`/`character_atlas.png` é validado ao carregar: se falhar,
as ilustrações originais são exibidas. Os recursos aprovados do cenário são
preservados. Os relógios continuam enquanto entradas mantidas aguardam soltura,
sem modificar o modo selecionado ou as regras de navegação.

As três nuvens do alto se deslocam lentamente para a direita, em ritmos
independentes, com filtro pontual e retorno somente quando estão inteiramente
fora da janela. O relógio próprio não reinicia junto ao ciclo da vegetação.
A nuvem baixa à direita, sobre o gramado acima de VS PLAYER, foi removida por uma
correção localizada do fundo. Nenhuma outra nuvem foi removida. Fonte e recursos
aprovados permanecem intactos; as camadas do céu são desenhadas atrás das folhas.
Se o atlas falhar, as três nuvens superiores ficam estáticas e a correção baixa
continua aplicada. O atlas também contém uma cópia da correção para tolerar a
falha isolada desse recurso.

Para regenerar as camadas e os diagnósticos com o mesmo ambiente de preparação:

```powershell
.\build\trainer_tools\Scripts\python tools/prepare_versus_grass.py
.\build\trainer_tools\Scripts\python tools/prepare_versus_foliage.py
.\build\trainer_tools\Scripts\python tools/prepare_versus_characters.py
.\build\trainer_tools\Scripts\python tools/prepare_versus_clouds.py
```

Máscaras, contato ampliado, GIF e relatório ficam em `build/versus_grass_preview/`.
A verificação abaixo também compara 64 instantes OpenGL, o ciclo de 32 segundos,
os cartões imóveis, o fallback parcial e a navegação existente. Execute
`tools/preview_versus_grass.py` após a verificação para montar o GIF do jogo real.
O teste adicional de folhagem compara a grama pixel a pixel com a versão anterior,
verifica falhas parciais, o retorno do ciclo e os instantes antes/depois da troca
de pose. Seus diagnósticos ficam em `build/versus_foliage_preview/`.

Para montar o GIF e os recortes ampliados da folhagem a partir das capturas reais:

```powershell
.\build\trainer_tools\Scripts\python tools/preview_versus_foliage.py
```

As verificações de personagens isolam 256 poses, conferem pés imóveis, ciclos,
limite de delta time e retorno à ilustração estática. Capturas ficam em
`build/versus_character_preview/`. Para montar GIFs do jogo e dos dois cartões:

```powershell
.\build\trainer_tools\Scripts\python tools/preview_versus_characters.py
```

A verificação das nuvens inspeciona os ciclos longos, as saídas/entradas da janela,
a remoção permanente no local original e os recursos de fallback. Capturas,
comparações antes/depois e relatório ficam em `build/versus_cloud_preview/`.

Para verificar a tela e gerar capturas OpenGL em `build/versus_preview/`:

```powershell
cmake --build build --target versus_screen_check
.\build\versus_screen_check.exe
```

A entrada do nome e sua transição reversível estão documentadas em [CAMPAIGN_NAME_ENTRY.md](docs/CAMPAIGN_NAME_ENTRY.md), incluindo verificações e capturas reproduzíveis. Nome e personagem ficam somente na sessão em memória.

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
