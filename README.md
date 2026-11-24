<div align="center">

<img src="assets/sprites/logo_v2.png" alt="Logo do jogo Plants x Plants" width="390">

# 🌱 Plants x Plants ⚔️

### Uma aventura botânica de exploração, estratégia e lógica proposicional.

Projeto acadêmico desenvolvido em **linguagem C**, com a biblioteca gráfica **raylib**, para **Windows**.

![Linguagem C11](https://img.shields.io/badge/Linguagem-C11-315a6b?style=flat-square)
![raylib 5.5](https://img.shields.io/badge/Biblioteca-raylib%205.5-267d57?style=flat-square)
![Windows](https://img.shields.io/badge/Plataforma-Windows-3274a1?style=flat-square)

**[O jogo](#-sobre-o-jogo) • [Funcionalidades](#-funcionalidades) • [Como jogar](#-como-jogar) • [Instalação](#-instalação-e-execução) • [Documentação](#-documentação)**

</div>

---

## 🎮 Sobre o jogo

**Plants x Plants** combina a exploração de um universo botânico com batalhas por turnos e desafios de **lógica proposicional**. O jogador cria seu personagem, conhece a jardineira **Lia**, escolhe três plantas para formar sua equipe e embarca em uma aventura pela **Estufa Lógica**.

Ao longo da jornada, é necessário explorar ambientes, conversar com personagens, resolver mecanismos e enfrentar adversários. Cada planta possui características e habilidades próprias, incentivando escolhas estratégicas durante os combates.

A lógica proposicional integra a experiência por meio de desafios com conectivos e expressões lógicas, relacionando conceitos estudados em sala de aula às mecânicas do jogo.

## ✨ Funcionalidades

- **Rankings da estufa:** recordes de conclusão mais rápida e de menor dano recebido.
- **Personalização do protagonista:** escolha do personagem e definição do nome.
- **Prólogo narrativo:** apresentação da missão, dos personagens e do universo do jogo.
- **Formação de equipe:** seleção de exatamente **três entre seis plantas**.
- **Exploração da Estufa Lógica:** movimentação, corrida, interação com cenários e acompanhamento de objetivos.
- **Diálogos interativos:** conversas com personagens e orientações ao longo da aventura.
- **Batalhas por turnos:** ataques, troca de plantas, atributos, habilidades e acertos críticos.
- **Golpes lógicos:** configuração de expressões e condições com operadores como **E**, **OU** e **XOR**.
- **Enigmas de cenário:** resolução de mecanismos envolvendo relés, portas, canais e condições lógicas.
- **Confrontos com chefes:** batalhas contra **Babosa** e **Girassol** em ambientes temáticos.
- **Desfecho da aventura:** conclusão dos desafios da estufa, com tela de encerramento e opção de jogar novamente.
- **Apresentação audiovisual:** interface em pixel art, personagens animados, efeitos sonoros e música.

## 🗺️ A aventura

A experiência está organizada em uma sequência de exploração e desafios:

| Etapa | Experiência |
| :--- | :--- |
| **1. Preparação** | Rankings da estufa, criação do personagem, prólogo e escolha das três plantas iniciais. |
| **2. Exploração** | Conhecer a Estufa Lógica, interagir com personagens e descobrir os objetivos. |
| **3. Desafios** | Resolver mecanismos, interpretar condições e avançar pelos ambientes. |
| **4. Confrontos** | Usar estratégias de combate para superar os guardiões da estufa. |
| **5. Conclusão** | Resolver o desafio final e chegar ao encerramento da aventura. |

## 🌿 Plantas do jogo

A equipe inicial é formada por três plantas escolhidas entre seis espécies. Cada uma oferece atributos, golpes e possibilidades estratégicas diferentes.

| Planta | Papel na aventura |
| :--- | :--- |
| 🍀 **Trevo** | Planta selecionável para a equipe inicial |
| 🌿 **Hera Venenosa** | Planta selecionável para a equipe inicial |
| 🌶️ **Pimenta** | Planta selecionável para a equipe inicial |
| 🌵 **Cacto** | Planta selecionável para a equipe inicial |
| 🌸 **Orquídea** | Planta selecionável para a equipe inicial |
| 🌱 **Planta Carnívora** | Planta selecionável para a equipe inicial |
| 🌿 **Babosa** | Chefe encontrado no depósito |
| 🌻 **Girassol** | Chefe encontrado no viveiro |

Os atributos e os golpes das espécies estão definidos em [`src/plant_data.c`](src/plant_data.c), enquanto as regras de batalha ficam em [`src/battle.c`](src/battle.c).

## 🧩 Como jogar

### Começando a aventura

1. Na tela inicial, pressione **Enter**.
2. Nos rankings, alterne entre **mais rápidos** e **menos dano** com as setas esquerda/direita, navegue pelos recordes com as setas cima/baixo e pressione **Enter** para jogar.
3. Selecione o personagem e informe seu nome.
4. Acompanhe o prólogo e escolha **três plantas** para a equipe.
5. Explore a Estufa Lógica, converse com os personagens e acompanhe os objetivos.
6. Resolva os enigmas, vença as batalhas e conclua a jornada.

### ⌨️ Controles

| Contexto | Tecla | Ação |
| :--- | :--- | :--- |
| Tela inicial | `Enter` | Abrir os rankings |
| Rankings | `←` / `→` ou `A` / `D` | Alternar entre tempo e dano |
| Rankings | `↑` / `↓` ou `W` / `S` | Navegar pelos recordes |
| Rankings | `Enter` | Iniciar a aventura |
| Rankings | `Esc` | Voltar ao menu |
| Menus e seleções | `Setas` / `WASD` | Navegar |
| Seleção de plantas | `Espaço` | Selecionar ou remover uma planta |
| Exploração | `WASD` / `Setas` | Movimentar o personagem |
| Exploração | `Shift` | Correr |
| Exploração | `E` | Interagir com objetos e personagens próximos |
| Batalha | `1`, `2`, `3` | Escolher um dos três golpes |
| Batalha | `Q`, `W`, `E`, `R` | Selecionar uma posição para troca de planta |
| Batalha | `Tab` | Exibir ou ocultar os atributos |
| Diálogos e menus | `Enter` | Confirmar ou avançar o texto |
| Áudio | `F9` | Ativar ou silenciar sons e música |
| Exibição | `F11` | Alternar entre janela e tela cheia |
| Exploração | `F8` | Comparar versões do mapa |
| Seleção de personagem | `F7` | Visualizar o cenário animado |
| Menus e exploração | `Esc` | Voltar conforme o contexto |

O mouse também pode ser utilizado nos menus e nas interfaces interativas.

Os recordes das partidas concluídas ficam em `data/leaderboard.csv`, criado na primeira vitória. O arquivo guarda nome, tempo em milissegundos, data e dano total; a tela de rankings apresenta as duas ordens em abas separadas, derivadas do mesmo histórico.

### ⚔️ Sistema de combate

O combate é organizado em turnos e privilegia a combinação entre os atributos de cada planta e as decisões do jogador.

- Uma planta de cada equipe permanece ativa em campo.
- As espécies possuem **vida (HP), ataque, defesa, velocidade e três golpes**.
- A velocidade define quem age primeiro no início do encontro.
- Os golpes podem causar dano ou modificar atributos durante o combate.
- Cada golpe tem uma chance específica de **acerto crítico**, que aumenta seu dano em **50%**.
- O jogador pode trocar a planta ativa, consumindo um turno.
- A batalha termina quando todas as plantas de uma equipe são derrotadas.

### 🧠 Lógica proposicional nas mecânicas

Os desafios do jogo aplicam conceitos como **conjunção (E)**, **disjunção (OU)**, **negação (NÃO)** e condições lógicas.

Na exploração, essas relações aparecem em mecanismos e enigmas. Nos combates, o jogador pode configurar golpes lógicos: após resolver um desafio, seleciona condições e operadores para criar uma expressão que influencia a utilização do golpe fortalecido.

Essa integração foi pensada para transformar conteúdos teóricos em situações práticas de decisão e resolução de problemas.

## 🖥️ Instalação e execução

### Pré-requisitos

Para compilar o jogo no **Windows**, utilize:

| Ferramenta | Requisito |
| :--- | :--- |
| **Git** | Clonar o repositório |
| **CMake** | Versão 3.24 ou superior |
| **GCC / MinGW-w64** | Compilador compatível com C11 |
| **Ninja** | Sistema de compilação |

Verifique as ferramentas no terminal do PowerShell ou do VS Code:

```powershell
git --version
cmake --version
gcc --version
ninja --version
```

> O processo de configuração obtém automaticamente a **raylib 5.5**. É necessária conexão com a internet na primeira configuração.

### 1. Clonar o repositório

```powershell
git clone https://github.com/AMSDAF/plants-x-plants.git
cd plants-x-plants
```

### 2. Configurar a compilação

Execute na pasta principal do projeto:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc
```

### 3. Compilar

```powershell
cmake --build build
```

### 4. Executar

```powershell
.\build\plants_x_plants.exe
```

**Importante:** execute o jogo a partir da pasta principal do repositório, pois imagens, fontes e sons utilizam caminhos relativos.

<details>
<summary><strong>🔧 Solução de problemas</strong></summary>

**Comando não reconhecido:** verifique se a ferramenta foi instalada e adicionada ao `PATH` do Windows. Reabra o terminal após qualquer alteração.

**Problemas ao configurar o compilador:** utilize uma nova pasta de compilação:

```powershell
cmake -S . -B build-clean -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc
cmake --build build-clean
.\build-clean\plants_x_plants.exe
```

**Imagens ou sons não aparecem:** inicie o executável na raiz do projeto e confira a presença da pasta `assets/`.

</details>

## 🛠️ Tecnologias utilizadas

| Tecnologia | Aplicação |
| :--- | :--- |
| **C (C11)** | Implementação das regras, estados e estruturas do jogo |
| **raylib 5.5** | Gráficos 2D, entrada de teclado/mouse e recursos de áudio |
| **CMake** | Configuração e organização da compilação |
| **GCC / MinGW-w64** | Compilação do código-fonte |
| **Ninja** | Execução do processo de build |
| **Git e GitHub** | Versionamento e colaboração entre os integrantes |

## 🗂️ Estrutura do projeto

```text
plants-x-plants/
├── assets/         # Imagens, sprites, fontes e sons
├── docs/           # Documentação técnica
├── include/        # Cabeçalhos da aplicação (.h)
├── src/            # Código-fonte do jogo (.c)
├── tools/          # Ferramentas auxiliares
├── CMakeLists.txt  # Configuração de compilação
└── README.md       # Apresentação e instruções
```

### Principais arquivos

- [`src/game.c`](src/game.c) — coordenação das telas e do fluxo da aventura.
- [`src/world.c`](src/world.c) — exploração e interação com o cenário.
- [`src/plant_data.c`](src/plant_data.c) — catálogo, atributos e golpes das plantas.
- [`src/battle.c`](src/battle.c) — sistema de combate por turnos.
- [`src/ai.c`](src/ai.c) — comportamento dos adversários.

A pasta `build/` é gerada localmente e não faz parte do código versionado.

## 📚 Documentação

A documentação complementar detalha as decisões de design e implementação:

| Tema | Documento |
| :--- | :--- |
| Campanha e exploração | [Estufa Lógica](docs/CAMPAIGN_GREENHOUSE.md) |
| Regras de combate | [Protótipo de combate](docs/BATTLE_PROTOTYPE.md) |
| Interface de batalha | [Interface de combate](docs/BATTLE_UI_PREPARATION.md) |
| Seleção inicial de plantas | [Seleção de plantas](docs/PLANT_SELECTION_DESIGN.md) |
| Objetivos e interface da exploração | [HUD de exploração](docs/HUD_IMPLEMENTATION.md) |
| Diálogos e personagens | [Interface de diálogos](docs/DIALOGUE_UI.md) |
| Menu e animações | [Menu V2](docs/MENU_V2.md) |
| Efeitos sonoros e música | [Sons do jogo](assets/sounds/README.md) |

## 🤝 Trabalho em equipe

**Plants x Plants** é resultado de um projeto acadêmico colaborativo, com divisão de atividades e integração do código por meio de **Git e GitHub**.

O desenvolvimento reuniu programação em C, criação de interfaces, elaboração de cenários, design de mecânicas e aplicação prática de lógica proposicional.

---

<div align="center">

**🌿 Plants x Plants — plante estratégias, resolva desafios e vença batalhas. ⚔️**

</div>
