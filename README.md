# Plants x Plants 🌱⚔️

Jogo de batalhas por turnos entre plantas, inspirado no sistema de combate de Pokémon e desenvolvido como projeto acadêmico em **C com raylib**, para **Windows**.

Monte sua equipe, evolua suas plantas e escolha seu caminho por diferentes biomas até enfrentar o boss.

## Status

A tela inicial abre diretamente a seleção de Explorador ou Exploradora ao pressionar **ENTER**. Depois do nome, um prólogo apresenta o pedido de ajuda de Lia, o aceite, a escolha de três entre seis plantas e a viagem até a vila. A chegada abre a Estufa Lógica, com exploração e batalhas contra guardiões. A progressão posterior de campanha é planejada.

## Modos de jogo

### Campanha

O jogador começa escolhendo três companheiras entre Trevo, Hera Venenosa, Pimenta, Cacto, Orquídea e Planta Carnívora. A evolução e o recrutamento durante a jornada permanecem planejados.

O elenco definitivo tem oito espécies: as seis iniciais acima e os bosses
**Babosa**, no depósito, e **Girassol**, no viveiro. Os bosses não aparecem na
seleção inicial. Cada boss luta sozinho contra a equipe: Babosa tem 275 HP/46 ATQ e
Girassol 225 HP/50 ATQ, com defesa e velocidade preservadas. As espécies antigas fora desse elenco foram retiradas do catálogo.

Atributos e três golpes por espécie ficam centralizados em `src/plant_data.c`.
O combate inclui golpes de dano e de atributo, fórmulas individuais, habilidades
e efeitos de status. Na seleção inicial, os cartões usam as novas sprites
`*_v2.png` de `assets/sprites/plants/`, com transparência, proporção preservada e
filtro pontual. O enquadramento considera o alfa da imagem; se ela faltar, o cartão
usa o desenho geométrico de reserva. Os cartões seguem a referência aprovada: molduras
de madeira com folhas, fundo creme ou verde-claro para as escolhidas, folha
numerada, foco dourado com seta e fonte Pixelify Sans. Os botões de rodapé
preservam cantos e folhas. A confirmação só é habilitada com três plantas. As quatro etapas do prólogo
compartilham título pixelado, progresso, subtítulo, instruções e botões de
madeira; o rótulo superior da marca foi removido.
Consulte [o desenho da seleção](docs/PLANT_SELECTION_DESIGN.md). O combate usa
as oito sprites definitivas, cenários próprios do depósito/viveiro e o menu
Atacar/Trocar, com Mochila e Fugir bloqueados. As artes antigas dos bosses são
preservadas na exploração. Consulte [a interface de combate](docs/BATTLE_UI_PREPARATION.md).

A campanha inclui um tutorial e três arenas, escolhidas entre rotas alternativas.

## Sistema de combate

- Cada equipe pode ter até quatro plantas.
- Apenas uma planta de cada equipe fica ativa por vez.
- No turno, o jogador pode usar o golpe fortalecido quando a fórmula for verdadeira ou trocar a planta ativa.
- A planta ativa com maior velocidade começa cada encontro; depois, os turnos alternam.
- Cada golpe possui sua própria chance de crítico, que aumenta o dano em 50%.
- A troca voluntária consome o turno.
- Quando uma planta é derrotada e há reserva viva, o lado derrotado escolhe a substituta e então joga seu turno.
- Vence quem derrotar todas as plantas da equipe adversária.

## Progressão da campanha

| Etapa | Progressão |
|---|---|
| Início | Prólogo e escolha de três entre seis plantas iniciais. |
| Tutorial | Apresentação dos ataques e das trocas com a equipe inicial. |
| Primeira arena | Escolha de rota e evolução da equipe para o segundo estágio após a vitória (planejado). |
| Segunda arena | Nova escolha de rota e evolução da equipe para o estágio final após a vitória (planejado). |
| Arena final | Batalha contra o boss da rota escolhida, com a equipe completa. |

A vida da equipe é recuperada entre arenas.

## Arenas e rotas

Existem seis arenas, distribuídas em três etapas, com duas opções de bioma por etapa.

O jogador enfrenta apenas uma arena de cada etapa. Assim, cada campanha possui três arenas, além do tutorial.

Cada bioma terá adversários próprios e uma regra de combate simples. A escolha da rota também influencia quais plantas estarão disponíveis para recrutamento.

Os biomas e seus efeitos ainda serão definidos.

## Recrutamento e evolução

Recrutamento e evolução ainda não estão implementados. Com a equipe inicial de três plantas, os momentos de recrutamento durante a campanha serão redefinidos, respeitando o limite de quatro integrantes.

Cada espécie possui três estágios evolutivos:

1. Mudinha.
2. Planta jovem.
3. Forma final.

A evolução acontece após a primeira e a segunda arena. Plantas recrutadas entram no estágio atual da equipe.

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

A versão atual abre uma tela inicial em pixel art, com logo flutuando suavemente, aviso intermitente e mascote em repouso animado. **ENTER** abre diretamente **Escolha seu personagem**, sem menu de opções. Solte ENTER antes de confirmar o personagem. **ESC** na seleção ou na exploração retorna à tela inicial; **ESC** na tela inicial ou o **X** da janela encerra o jogo.

O menu usa `assets/sprites/logo_v2.png`, `menu_mascot.png` e `menu_background_v2.png`. A fonte atualmente fornecida tem a grafia `menu_backround_v2.png`, aceita como alternativa sem renomear o original. Logo e mascote são enquadrados pelos limites visíveis do alfa durante a carga; todas as texturas do menu usam filtro pontual. `menu_mascot_idle.png` já acompanha o projeto, com oito quadros a 6 FPS e fallback para a fonte estática. Dezessete touceiras e doze copas extraídas do fundo recebem vento localizado; troncos, raízes e pedras permanecem fixos. Sem o par válido de fundo reconstruído e atlas de grama, o jogo usa o fundo original completo. A seleção mantém fundo e retratos próprios; seus cartões e controles são desenhados em código. Consulte [MENU_V2.md](docs/MENU_V2.md) para composição e validação.

O aviso “Pressione ENTER” da home usa Pixelify Sans, como as seleções. A fonte é
rasterizada na resolução física da tela. Carta, aceite e viagem também usam
atlas próprios nos textos para preservar a nitidez ao ampliar a janela.

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

Na tela **Escolha seu personagem**, use esquerda/direita ou A/D, ou clique no cartão de **Homem** ou **Mulher**. As duas telas usam molduras botânicas de madeira, pergaminho creme, títulos Pixelify e os mesmos botões do prólogo sobre o fundo animado escurecido. A escolha recebe fundo verde-claro, contorno dourado e seta laranja. O clique apenas seleciona: pressione **ENTER** ou clique em **Confirmar** para abrir **Como você se chama?**. Digite até 12 caracteres, incluindo acentos portugueses; **TAB/SHIFT+TAB** alterna entre campo e botões. Confirme um nome válido para abrir o prólogo antes da Estufa Lógica. **Voltar/ESC** nessa etapa retorna à seleção, preservando o rascunho. Na seleção, **Voltar/ESC** retorna à tela inicial. Os retratos animados de repouso preservam a proporção e o alinhamento dos pés. Consulte [a interface de personagem e nome](docs/TRAINER_SETUP_DESIGN.md).

- **WASD ou setas:** mover em quatro direções, sem diagonais. Com várias teclas mantidas, a prioridade é norte, sul, oeste e leste.
- **Shift na estufa:** correr enquanto se move.
- **F8 na estufa:** comparar o mapa original e a versão completa com mais detalhe.
- **F11:** alternar a tela cheia; o desenho agora usa a resolução física do viewport.
- **F9:** silenciar ou reativar efeitos sonoros e música. Consulte [os sons do jogo](assets/sounds/README.md).
- **E na estufa:** interagir com a válvula, o refletor, a chave, os canais ou a central ao se aproximar.
- **ESC na seleção ou exploração:** voltar à tela inicial, sem fechar o jogo.
- **ESC na tela inicial** ou **X** da janela: encerrar.

Os exploradores usam retratos animados de repouso na seleção e 12 quadros
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

O HUD de exploração usa molduras botânicas de madeira, fundo creme, fonte
Pixelify Sans e uma seta em pixel art para o alvo. Objetivo/local, pistas e
condições da ponte e do viveiro seguem a mesma estética. A instrução de
interação aparece no centro inferior, com moldura de madeira e folhas e fonte
Pixelify Sans. A frase “Pressione E para…” aparece só quando a ação está disponível. A fonte Atkinson Hyperlegible Next acompanha o jogo e é usada
nas demais telas, sem instalação no Windows. Detalhes em [HUD_IMPLEMENTATION.md](docs/HUD_IMPLEMENTATION.md).

Os diálogos usam fonte Pixelify Sans, moldura de madeira com folhas, texto à esquerda e retrato com
nome à direita. Lia (jardineira), Bento (técnico) e Íris (pesquisadora) têm retratos próprios.
Cada um se apresenta no primeiro contato e usa o nome escolhido pelo jogador.
Os pedidos de ajuda, as dicas seguintes e os agradecimentos acompanham o progresso.
Ao resolver a central, Íris agradece antes da tela de conclusão.
Se uma imagem faltar, aparece a inicial do personagem. O texto aparece letra por letra; E/Enter completa a fala e,
no próximo toque, avança ou fecha. ESC fecha a conversa. Textos e nomes continuam dinâmicos. Consulte
[DIALOGUE_UI.md](docs/DIALOGUE_UI.md).

Os quatro quadrantes melhorados estão integrados ao mapa completo, com
3010 × 1694 pixels. O background original foi preservado. Veja a resolução real,
o preparo e as verificações em [COURTYARD_DETAIL.md](docs/COURTYARD_DETAIL.md).

Os atlas preparados em `assets/sprites/trainers/` já estão prontos para execução; Python não é necessário para jogar. Execute a partir da raiz para localizar os recursos. Um atlas ausente ou inválido gera diagnóstico no console e um marcador jogável.

### Recursos dos personagens

Somente quatro PNGs de personagem são mantidos em `assets/sprites/trainers/`:

- `male_botanical.png` e `female_botanical.png`: caminhada e pose parada na exploração.
- `male_portrait_idle.png` e `female_portrait_idle.png`: quatro poses animadas na seleção e na entrada de nome.

Os atlas de repouso são utilizados diretamente, sem retratos estáticos ou halos
separados. Na falta de um retrato válido, o cartão apresenta um aviso; na falta
de um atlas botânico válido, a exploração usa um marcador geométrico.
As fontes e variantes antigas foram removidas. Os quatro PNGs preparados são
os recursos mantidos; não há geração dos retratos de repouso a partir de fontes
incluídas no repositório. Consulte [TRAINER_SELECTION.md](docs/TRAINER_SELECTION.md)
e [TRAINER_TEST.md](docs/TRAINER_TEST.md).

Para executar os demais geradores de arte, use o ambiente opcional:

```powershell
python -m venv build/trainer_tools
.\build\trainer_tools\Scripts\python -m pip install -r tools/trainer_requirements.txt
```

### Preparar novamente o cenário de seleção (opcional)

O fundo animado acompanha os cartões na seleção de personagem. Pressione **F7**
para ver somente o cenário animado. **ESC** retorna à tela inicial; F7 recupera a seleção
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

Após o nome, **ENTER** completa o texto da carta e, no próximo toque, avança.
O pedido de Lia é seguido do aceite, da escolha de exatamente três entre seis
plantas e da viagem à vila. Use **setas/WASD** e **Espaço**, ou clique nos cartões,
para selecionar ou remover plantas; **ENTER** confirma somente com três escolhidas.
**Voltar/ESC** retorna à etapa anterior. A primeira planta escolhida começa ativa.
A chegada abre o mapa existente da estufa; não há um mapa adicional de vila.

A placa perto do depósito abre o modal de golpes lógicos. As abas mostram as
três plantas e podem ser escolhidas em qualquer ordem. Cada planta responde
sua própria pergunta (OU, E ou NÃO); erros permitem tentar novamente.
Acertar libera o editor daquela planta, sem aplicar um golpe automaticamente.
Escolha um ataque de dano, duas condições diferentes e o operador **E, OU ou XOR**.
**Confirmar configuração** salva apenas a planta selecionada; é possível editar
depois. Perguntas resolvidas e rascunhos são preservados ao alternar abas ou
fechar com **Voltar/ESC**, durante a sessão atual.
Use clique, **TAB/setas** e **ENTER/Espaço**. Nas listas, **ESC** fecha somente a
lista; fora delas, fecha o modal. Golpes de atributo e condições repetidas ficam
indisponíveis. A fórmula verdadeira concede o bônus do operador; falsa bloqueia
somente o golpe lógico confirmado. Sem confirmação, todos os ataques continuam
normais. O encontro abre direto, sem configuração obrigatória.
Durante a batalha, **1/2/3** usam golpes e **Q/W/E/R** trocam a planta ativa.
**ENTER** no resultado ou **ESC** retorna à estufa. Consulte
[o relatório do combate](docs/BATTLE_PROTOTYPE.md).

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

### Verificar a introdução

```powershell
cmake --build build --target intro_check battle_check
.\build\intro_check.exe
.\build\battle_check.exe
```

O primeiro alvo verifica o fluxo do nome à chegada, as seis candidatas jogáveis,
a seleção de três, o retorno entre etapas, o reinício da sessão e a equipe na
primeira batalha. As capturas ficam em `build/intro_preview/`. O prólogo reutiliza
o fundo e as fontes existentes, com as sprites das iniciais nos cartões e silhuetas
provisórias quando falta um recurso.
